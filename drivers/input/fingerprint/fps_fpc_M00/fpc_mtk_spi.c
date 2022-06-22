/*
 * FPC Fingerprint sensor device driver
 *
 * This driver will control the platform resources that the FPC fingerprint
 * sensor needs to operate. The major things are probing the sensor to check
 * that it is actually connected and let the Kernel know this and with that also
 * enabling and disabling of regulators, enabling and disabling of platform
 * clocks.
 * *
 * The driver will expose most of its available functionality in sysfs which
 * enables dynamic control of these features from eg. a user space process.
 *
 * The sensor's IRQ events will be pushed to Kernel's event handling system and
 * are exposed in the drivers event node. This makes it possible for a user
 * space process to poll the input node and receive IRQ events easily. Usually
 * this node is available under /dev/input/eventX where 'X' is a number given by
 * the event system. A user space process will need to traverse all the event
 * nodes and ask for its parent's name (through EVIOCGNAME) which should match
 * the value in device tree named input-device-name.
 *
 *
 * Copyright (c) 2018 Fingerprint Cards AB <tech@fingerprints.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/pm_wakeup.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>


#ifdef FPC_DRIVER_FOR_ISEE
#include "teei_fp.h"
#include "tee_client_api.h"
#endif

#include "ontim/ontim_dev_dgb.h"
#define FPS_HW_INFO "FPC1553"
DEV_ATTR_DECLARE(fingersensor)
DEV_ATTR_DEFINE("vendor", FPS_HW_INFO)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(fingersensor, fingersensor, 8);

#ifdef FPC_DRIVER_FOR_ISEE
struct TEEC_UUID uuid_ta_fpc = { 0x7778c03f, 0xc30c, 0x4dd0,
	{0xa3, 0x19, 0xea, 0x29, 0x64, 0x3d, 0x4d, 0x4b}
};
#endif

#define FPC1022_CHIP 0x1000
#define FPC1022_CHIP_MASK_SENSOR_TYPE 0xf000
#define FPC_RESET_LOW_US 6000
#define FPC_POWEROFF_SLEEP_US 1000
#define FPC_RESET_HIGH1_US 100
#define FPC_RESET_HIGH2_US 5000
#define FPC_TTW_HOLD_TIME 1000
#define     FPC102X_REG_HWID      252
u32 spi_speed = 1 * 1000000;

#define CONFIG_FPC_COMPAT    1

static const char * const pctl_names[] = {
	"fpsensor_rst_low",
	"fpsensor_rst_high",
};

struct fpc_data {
	struct device *dev;
	struct spi_device *spidev;
	struct pinctrl *pinctrl_fpc;
	struct pinctrl_state *pinctrl_state[ARRAY_SIZE(pctl_names)];
	int irq_gpio;
	int rst_gpio;
	int power_ctl_gpio;
	bool wakeup_enabled;
	struct wakeup_source *ttw_wl;

	#ifdef CONFIG_FPC_COMPAT
    bool compatible_enabled;
	#endif
};

static DEFINE_MUTEX(spidev_set_gpio_mutex);

extern void mt_spi_disable_master_clk(struct spi_device *spidev);
extern void mt_spi_enable_master_clk(struct spi_device *spidev);
bool fpc1022_fp_exist = false;

bool g_fpsensor_clocks_enabled;
struct spi_device *g_fpsensor_spidev;

static irqreturn_t fpc_irq_handler(int irq, void *handle);


static int select_pin_ctl(struct fpc_data *fpc, const char *name)
{
	size_t i;
	int rc;
	struct device *dev = fpc->dev;
	for (i = 0; i < ARRAY_SIZE(fpc->pinctrl_state); i++) {
		const char *n = pctl_names[i];
		if (!strncmp(n, name, strlen(n))) {
			mutex_lock(&spidev_set_gpio_mutex);
			rc = pinctrl_select_state(fpc->pinctrl_fpc, fpc->pinctrl_state[i]);
			mutex_unlock(&spidev_set_gpio_mutex);
			if (rc)
				dev_err(dev, "cannot select '%s'\n", name);
			else
				dev_dbg(dev, "Selected '%s'\n", name);
			goto exit;
		}
	}
	rc = -EINVAL;
	dev_err(dev, "%s:'%s' not found\n", __func__, name);
exit:
	return rc;
}

int set_clks(bool enable)
{
	static int g_fpsensor_clocks_enabled = 0;

	if(g_fpsensor_clocks_enabled == enable)
		return g_fpsensor_clocks_enabled;
	if (enable) {
		mt_spi_enable_master_clk(g_fpsensor_spidev);
		g_fpsensor_clocks_enabled = true;
	} else {
		mt_spi_disable_master_clk(g_fpsensor_spidev);
		g_fpsensor_clocks_enabled = false;
	}

	return g_fpsensor_clocks_enabled;
}

EXPORT_SYMBOL(set_clks);

static int hw_reset(struct  fpc_data *fpc)
{
	int irq_gpio;
	struct device *dev = fpc->dev;

	select_pin_ctl(fpc, "fpsensor_rst_low");
	usleep_range(FPC_RESET_LOW_US, FPC_RESET_LOW_US + 100);

	select_pin_ctl(fpc, "fpsensor_rst_high");
	usleep_range(FPC_RESET_HIGH2_US, FPC_RESET_HIGH2_US + 100);

	irq_gpio = gpio_get_value(fpc->irq_gpio);
	dev_info(dev, "IRQ after reset %d\n", irq_gpio);

	dev_info( dev, "Using GPIO#%d as IRQ.\n", fpc->irq_gpio );
	dev_info( dev, "Using GPIO#%d as RST.\n", fpc->rst_gpio );

	return 0;
}


static int spi_read_hwid(struct spi_device *spi, u8 * rx_buf)
{
	int error;
	struct spi_message msg;
	struct spi_transfer *xfer;
	u8 tmp_buf[10] = { 0 };
	u8 addr = FPC102X_REG_HWID;

	xfer = kzalloc(sizeof(*xfer) * 2, GFP_KERNEL);
	if (xfer == NULL) {
		dev_err(&spi->dev, "%s, no memory for SPI transfer\n",
			__func__);
		return -ENOMEM;
	}

	spi_message_init(&msg);

	tmp_buf[0] = addr;
	xfer[0].tx_buf = tmp_buf;
	xfer[0].len = 1;
	xfer[0].delay_usecs = 5;

#ifdef CONFIG_SPI_MT65XX
	xfer[0].speed_hz = spi_speed;
	pr_err("%s %d, now spi-clock:%d\n",
	       __func__, __LINE__, xfer[0].speed_hz);
#endif

	spi_message_add_tail(&xfer[0], &msg);

	xfer[1].tx_buf = tmp_buf + 2;
	xfer[1].rx_buf = tmp_buf + 4;
	xfer[1].len = 2;
	xfer[1].delay_usecs = 5;

#ifdef CONFIG_SPI_MT65XX
	xfer[1].speed_hz = spi_speed;
#endif

	spi_message_add_tail(&xfer[1], &msg);
	error = spi_sync(spi, &msg);
	if (error)
		dev_err(&spi->dev, "%s : spi_sync failed.\n", __func__);

	memcpy(rx_buf, (tmp_buf + 4), 2);

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;

	return 0;
}

static int check_hwid(struct spi_device *spi)
{
	int error = 0;
	u32 time_out = 0;
	u8 tmp_buf[2] = { 0 };
	u16 hardware_id;

	do {
		spi_read_hwid(spi, tmp_buf);
		pr_info("%s, fpc1520 chip version is 0x%x, 0x%x\n",
		       __func__, tmp_buf[0], tmp_buf[1]);

		time_out++;

		hardware_id = ((tmp_buf[0] << 8) | (tmp_buf[1]));
		pr_err("fpc hardware_id[0]= 0x%x id[1]=0x%x\n", tmp_buf[0],
		       tmp_buf[1]);

		if (hardware_id == 0x2012) {
			pr_err("fpc match hardware_id = 0x%x is true\n",
			       hardware_id);
			error = 0;
		} else {
			pr_err("fpc match hardware_id = 0x%x is failed\n",
			       hardware_id);
			error = -1;
		}

		if (!error) {
			pr_info("fpc %s, fpc1553_S160 chip version check pass, time_out=%d\n",
			       __func__, time_out);
			return 0;
		}
	} while (time_out < 2);

	pr_info("%s, fpc1553_S160 chip version read failed, time_out=%d\n",
	       __func__, time_out);
	//spi_fingerprint = spi;
	return -1;
}

static int fpc_power_supply(struct fpc_data *fpc)
{
	int ret = 0;	
	struct device *dev = &fpc->spidev->dev;
	struct device_node *node = of_find_compatible_node(NULL, NULL, "mediatek,fpc-eint");

	dev_info(dev, "fp Power init start");

	if(!node){
		dev_err(dev, "no of node found\n");
		return -EINVAL;
	}

	fpc->power_ctl_gpio = of_get_named_gpio(node, "fpc,gpio-power", 0);

	if (fpc->power_ctl_gpio < 0) {
        dev_err(dev,"failed to get fpsensor,gpio_vcc_en \n");
        return fpc->power_ctl_gpio;
    }
	ret = devm_gpio_request(dev, fpc->power_ctl_gpio, "fpc,gpio-power");
    if (ret) {
        dev_err(dev,"failed to request gpio_vcc_en %d\n", fpc->power_ctl_gpio);
        // goto dts_init_exit;
        gpio_free(fpc->power_ctl_gpio);
        ret = devm_gpio_request(dev, fpc->power_ctl_gpio, "fpc,gpio-power");
        dev_info(dev,"the value of request fpc->power_ctl_gpio is %d\n", ret);
        return ret;
   }
	
   ret = gpio_direction_output(fpc->power_ctl_gpio, 1);
   dev_info(dev, "the value of fpc->power_ctl_gpio  is %d\n", fpc->power_ctl_gpio);
   dev_info(dev, "the value after set fpc->power_ctl_gpio 1 is %d\n", gpio_get_value(fpc->power_ctl_gpio));
   return ret;
}


static ssize_t hw_reset_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct  fpc_data *fpc = dev_get_drvdata(dev);

	if (!strncmp(buf, "reset", strlen("reset"))) {
		rc = hw_reset(fpc);
		return rc ? rc : count;
	}
	else
		return -EINVAL;
}
static DEVICE_ATTR(hw_reset, S_IWUSR, NULL, hw_reset_set);

/**
 * sysfs node for controlling whether the driver is allowed
 * to wake up the platform on interrupt.
 */
static ssize_t wakeup_enable_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct  fpc_data *fpc = dev_get_drvdata(dev);
	ssize_t ret = count;

	if (!strncmp(buf, "enable", strlen("enable"))) {
		fpc->wakeup_enabled = true;
		smp_wmb();
	} else if (!strncmp(buf, "disable", strlen("disable"))) {
		fpc->wakeup_enabled = false;
		smp_wmb();
	}
	else
		ret = -EINVAL;

	return ret;
}
static DEVICE_ATTR(wakeup_enable, S_IWUSR, NULL, wakeup_enable_set);

/**
 * sysf node to check the interrupt status of the sensor, the interrupt
 * handler should perform sysf_notify to allow userland to poll the node.
 */
static ssize_t irq_get(struct device *device,
			struct device_attribute *attribute,
			char* buffer)
{
	struct fpc_data *fpc = dev_get_drvdata(device);
	int irq = gpio_get_value(fpc->irq_gpio);

	return scnprintf(buffer, PAGE_SIZE, "%i\n", irq);
}

/**
 * writing to the irq node will just drop a printk message
 * and return success, used for latency measurement.
 */
static ssize_t irq_ack(struct device *device,
			struct device_attribute *attribute,
			const char *buffer, size_t count)
{
	struct fpc_data *fpc = dev_get_drvdata(device);
	dev_dbg(fpc->dev, "%s\n", __func__);

	return count;
}
static DEVICE_ATTR(irq, S_IRUSR | S_IWUSR, irq_get, irq_ack);

static ssize_t clk_enable_set(struct device *device,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return set_clks((*buf == '1')) ? : count;
}
static DEVICE_ATTR(clk_enable, S_IWUSR, NULL, clk_enable_set);

static ssize_t hwinfo_set(struct device *device,
        struct device_attribute *attr, const char *buf, size_t count)
{
	struct fpc_data *fpc = dev_get_drvdata(device);
	CHECK_THIS_DEV_DEBUG_AREADY_EXIT();
	REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();

	dev_dbg(fpc->dev, "set hwinfo\n");

	return count;
}

static DEVICE_ATTR(hwinfo, S_IWUSR, NULL, hwinfo_set);

#ifdef CONFIG_FPC_COMPAT
static ssize_t compatible_all_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int fpc_sensor_exit  = 0;
	int i;
	struct device_node *node_eint, *fpc_node, *node;
	int irqf = 0;
	int irq_num = 0;
	int rc = 0;
	struct fpc_data *fpc  = dev_get_drvdata(dev);
	struct spi_device *spidev = fpc->spidev;
    struct platform_device *pdev = NULL;
	dev_err(dev, "compatible all enter %d\n", fpc->compatible_enabled);

	if(!strncmp(buf, "enable", strlen("enable")) && fpc->compatible_enabled != 1){

        node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint-pinctrl");
        if (node){
            pdev = of_find_device_by_node(node);
        }else{
            dev_err(dev, "cannot find mediatek,fingerprint-pinctrl node\n");
        }

        fpc->compatible_enabled = 1;

		fpc->pinctrl_fpc = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR(fpc->pinctrl_fpc)) {
			rc = PTR_ERR(fpc->pinctrl_fpc);
			dev_err(fpc->dev, "Cannot find pinctrl_fpc rc = %d.\n", rc);
			set_clks(false);
			return rc;
		}

		for (i = 0; i < ARRAY_SIZE(fpc->pinctrl_state); i++) {
			const char *n = pctl_names[i];
			struct pinctrl_state *state = pinctrl_lookup_state(fpc->pinctrl_fpc, n);
			if (IS_ERR(state)) {
				dev_err(dev, "cannot find '%s'\n", n);
				rc = -EINVAL;
				set_clks(false);
				return rc;
			}
			dev_info(dev, "found pin control %s\n", n);
			fpc->pinctrl_state[i] = state;
		}

		fpc_power_supply(fpc);
		//g_fpsensor_clocks_enabled = false;
		set_clks(true);
		(void)hw_reset(fpc);

		fpc_sensor_exit = check_hwid(spidev);
		if (fpc_sensor_exit < 0) {
				/*pr_notice("%s: %d get chipid fail. now exit\n",
					  __func__, __LINE__);
				devm_pinctrl_put(fpc->pinctrl_fpc);
				gpio_free(fpc->rst_gpio);
				set_clks(false);

				fpc1022_fp_exist = false;
				spidev->dev.of_node = fpc_node;
				return -EAGAIN;*/
				pr_notice("%s: %d get chipid fail. retry... \n", __func__, __LINE__);
				set_clks(false);
				gpio_direction_output(fpc->power_ctl_gpio, 0);
				usleep_range(FPC_POWEROFF_SLEEP_US, FPC_POWEROFF_SLEEP_US + 100);
				gpio_direction_output(fpc->power_ctl_gpio, 1);
				usleep_range(FPC_POWEROFF_SLEEP_US, FPC_POWEROFF_SLEEP_US + 100);
				set_clks(true);
				(void)hw_reset(fpc);
				if(check_hwid(spidev))
					pr_notice("%s: %d get chipid fail. \n", __func__, __LINE__);
		}
		fpc1022_fp_exist = true;
		node_eint = of_find_compatible_node(NULL, NULL, "mediatek,fpc-eint");
		if (node_eint == NULL) {
			rc = -EINVAL;
			dev_err(fpc->dev, "cannot find node_eint rc = %d.\n", rc);
			set_clks(false);
			return rc;
		}

		fpc->irq_gpio = of_get_named_gpio(node_eint, "fpc,gpio-irq", 0);
		dev_info(dev, "irq_gpio=%d\n", fpc->irq_gpio);
#if 0
		irq_num = irq_of_parse_and_map(node_eint, 0);/*get irq num*/
#else
		irq_num = gpio_to_irq(fpc->irq_gpio);
#endif
		dev_info(dev, "irq_num=%d\n", irq_num);
		if (!irq_num) {
			rc = -EINVAL;
			dev_err(fpc->dev, "get irq_num error rc = %d.\n", rc);
			set_clks(false);
			return rc;
		}


		dev_dbg(dev, "Using GPIO#%d as IRQ.\n", fpc->irq_gpio);
		dev_dbg(dev, "Using GPIO#%d as RST.\n", fpc->rst_gpio);

		fpc->wakeup_enabled = false;

		irqf = IRQF_TRIGGER_HIGH | IRQF_ONESHOT;
		if (of_property_read_bool(dev->of_node, "fpc,enable-wakeup")) {
			//irqf |= IRQF_NO_SUSPEND;
			device_init_wakeup(dev, 1);
		}
		rc = devm_request_threaded_irq(dev, irq_num,
			NULL, fpc_irq_handler, irqf,
			dev_name(dev), fpc);
		if (rc) {
			dev_err(dev, "could not request irq %d\n", irq_num);
			set_clks(false);
			return rc;
		}
		dev_dbg(dev, "requested irq %d\n", irq_num);

		/* Request that the interrupt should be wakeable */
		enable_irq_wake(irq_num);
		//wakeup_source_init(&fpc->ttw_wl, "fpc_ttw_wl");
		//fpc->compatible_enabled = 1;
		rc = hw_reset(fpc);
		dev_info(dev, "%s: ok\n", __func__);
		return count;

	}else if(!strncmp(buf, "disable", strlen("disable")) && fpc->compatible_enabled != 0){
		//sysfs_remove_group(&spidev->dev.kobj, &fpc_attribute_group);
		//wakeup_source_trash(&fpc->ttw_wl);
		fpc->compatible_enabled = 0;
		wakeup_source_remove(fpc->ttw_wl);
		//gpio_free(fpc->rst_gpio);
		if(gpio_is_valid(fpc->irq_gpio)){
			gpio_free(fpc->irq_gpio);
			dev_dbg(dev, "Release IRQ GPIO#%d.\n", fpc->rst_gpio);
		}
		gpio_direction_output(fpc->power_ctl_gpio, 0);
		dev_info(dev, "cutoff power fpc->power_ctl_gpio = %d\n", gpio_get_value(fpc->power_ctl_gpio));
		if(gpio_is_valid(fpc->power_ctl_gpio)){
			gpio_free(fpc->power_ctl_gpio);
			dev_dbg(dev, "Release POWER GPIO#%d.\n", fpc->power_ctl_gpio);
		}
		//fpc->compatible_enabled = 0;
		return count;
	}
	return count;
}
static DEVICE_ATTR(compatible_all, S_IWUSR, NULL, compatible_all_set);
#endif


static struct attribute *fpc_attributes[] = {
	&dev_attr_hw_reset.attr,
	&dev_attr_wakeup_enable.attr,
	&dev_attr_clk_enable.attr,
	&dev_attr_hwinfo.attr,
	&dev_attr_irq.attr,
#ifdef CONFIG_FPC_COMPAT
	&dev_attr_compatible_all.attr,
#endif
	NULL
};

static const struct attribute_group fpc_attribute_group = {
	.attrs = fpc_attributes,
};

static irqreturn_t fpc_irq_handler(int irq, void *handle)
{
	struct fpc_data *fpc = handle;
	struct device *dev = fpc->dev;
	static int current_level = 0; // We assume low level from start
	current_level = !current_level;

	if (current_level) {
		dev_dbg(dev, "Reconfigure irq to trigger in low level\n");
		irq_set_irq_type(irq, IRQF_TRIGGER_LOW);
	} else {
		dev_dbg(dev, "Reconfigure irq to trigger in high level\n");
		irq_set_irq_type(irq, IRQF_TRIGGER_HIGH);
	}

	/* Make sure 'wakeup_enabled' is updated before using it
	** since this is interrupt context (other thread...) */
	smp_rmb();
	if (fpc->wakeup_enabled) {
		__pm_wakeup_event(fpc->ttw_wl, FPC_TTW_HOLD_TIME);
	}

	sysfs_notify(&fpc->dev->kobj, NULL, dev_attr_irq.attr.name);

	return IRQ_HANDLED;
}



static int mtk6797_probe(struct spi_device *spidev)
{
	struct device *dev = &spidev->dev;
	struct device_node *fpc_node;
	struct fpc_data *fpc;
	int rc = 0;

#ifndef CONFIG_FPC_COMPAT
	size_t i;
	int fpc_sensor_exit  = 0;
	int irqf = 0;
	int irq_num = 0;
	struct device_node *node_eint, *node;
    struct platform_device *pdev = NULL;
#endif
	dev_info(dev, "%s, %s\n", __func__, __LINE__);

	fpc_node = spidev->dev.of_node;
	spidev->dev.of_node = of_find_compatible_node(NULL, NULL, "mediatek,fpsensor");
	if (!spidev->dev.of_node) {
		dev_err(dev, "no of node found\n");
		rc = -EINVAL;
		return rc;
	}

	fpc = devm_kzalloc(dev, sizeof(*fpc), GFP_KERNEL);
	if (!fpc) {
		dev_err(dev, "failed to allocate memory for struct fpc_data\n");
		rc = -ENOMEM;
		return rc;
	}

	fpc->dev = dev;
	dev_set_drvdata(dev, fpc);
	fpc->spidev = spidev;
	fpc->spidev->irq = 0; /*SPI_MODE_0*/
	fpc->spidev->mode = SPI_MODE_0;
	fpc->spidev->bits_per_word = 8;
	fpc->spidev->max_speed_hz = 1 * 1000 * 1000;
	g_fpsensor_spidev = spidev;

#ifndef CONFIG_FPC_COMPAT

    node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint-pinctrl");
    if (node)
    {
            pdev = of_find_device_by_node(node);
    }
	fpc->pinctrl_fpc = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(fpc->pinctrl_fpc)) {
		rc = PTR_ERR(fpc->pinctrl_fpc);
		dev_err(fpc->dev, "Cannot find pinctrl_fpc rc = %d.\n", rc);
		set_clks(false);
		return rc;
	}

	for (i = 0; i < ARRAY_SIZE(fpc->pinctrl_state); i++) {
		const char *n = pctl_names[i];
		struct pinctrl_state *state = pinctrl_lookup_state(fpc->pinctrl_fpc, n);
		if (IS_ERR(state)) {
			dev_err(dev, "cannot find '%s'\n", n);
			rc = -EINVAL;
			set_clks(false);
			return rc;
		}
		dev_info(dev, "found pin control %s\n", n);
		fpc->pinctrl_state[i] = state;
	}

	fpc_power_supply(fpc);
	//g_fpsensor_clocks_enabled = false;
	set_clks(true);
	(void)hw_reset(fpc);
	fpc_sensor_exit = check_hwid(spidev);
	if (fpc_sensor_exit < 0) {
			pr_notice("%s: %d get chipid fail. now exit\n",
				  __func__, __LINE__);
			devm_pinctrl_put(fpc->pinctrl_fpc);
			gpio_free(fpc->rst_gpio);
			set_clks(false);

			fpc1022_fp_exist = false;
			spidev->dev.of_node = fpc_node;
            dev_info(dev, "%s, [tep-2] %s\n", __func__, __LINE__);
			return -EAGAIN;
	}
    dev_info(dev, "%s, [tep-1] %s\n", __func__, __LINE__);
	fpc1022_fp_exist = true;
	node_eint = of_find_compatible_node(NULL, NULL, "mediatek,fpsensor_fp_eint");
	if (node_eint == NULL) {
		rc = -EINVAL;
		dev_err(fpc->dev, "cannot find node_eint rc = %d.\n", rc);
		set_clks(false);
		return rc;
	}

	fpc->irq_gpio = of_get_named_gpio(node_eint, "fpc,gpio-irq", 0);
	dev_info(dev, "irq_gpio=%d\n", fpc->irq_gpio);
#if 0
	irq_num = irq_of_parse_and_map(node_eint, 0);/*get irq num*/
#else
	irq_num = gpio_to_irq(fpc->irq_gpio);
#endif
	dev_info(dev, "irq_num=%d\n", irq_num);
	if (!irq_num) {
		rc = -EINVAL;
		dev_err(fpc->dev, "get irq_num error rc = %d.\n", rc);
		set_clks(false);
		return rc;
	}

	dev_dbg(dev, "Using GPIO#%d as IRQ.\n", fpc->irq_gpio);
	dev_dbg(dev, "Using GPIO#%d as RST.\n", fpc->rst_gpio);

	fpc->wakeup_enabled = false;

	irqf = IRQF_TRIGGER_HIGH | IRQF_ONESHOT;
	if (of_property_read_bool(dev->of_node, "fpc,enable-wakeup")) {
		irqf |= IRQF_NO_SUSPEND;
		device_init_wakeup(dev, 1);
	}
	rc = devm_request_threaded_irq(dev, irq_num,
		NULL, fpc_irq_handler, irqf,
		dev_name(dev), fpc);
	if (rc) {
		dev_err(dev, "could not request irq %d\n", irq_num);
		set_clks(false);
		return rc;
	}
	dev_dbg(dev, "requested irq %d\n", irq_num);

	/* Request that the interrupt should be wakeable */
	enable_irq_wake(irq_num);
	//wakeup_source_init(&fpc->ttw_wl, "fpc_ttw_wl");
	fpc->ttw_wl = wakeup_source_create("fpc_ttw_wl");
	wakeup_source_add(fpc->ttw_wl);

	rc = sysfs_create_group(&dev->kobj, &fpc_attribute_group);
	if (rc) {
		dev_err(dev, "could not create sysfs\n");
		set_clks(false);
		return rc;
	}

	(void)hw_reset(fpc);
	dev_info(dev, "%s: ok\n", __func__);
	return rc;
#else
	fpc->ttw_wl = wakeup_source_create("fpc_ttw_wl");
	wakeup_source_add(fpc->ttw_wl);
	rc = sysfs_create_group(&dev->kobj, &fpc_attribute_group);
	if (rc) {
		dev_err(dev, "could not create sysfs\n");
		return rc;
	}
	dev_info(dev, "%s: ok\n", __func__);
	return rc;
#endif 

}

static int mtk6797_remove(struct spi_device *spidev)
{
	struct  fpc_data *fpc = dev_get_drvdata(&spidev->dev);

	sysfs_remove_group(&spidev->dev.kobj, &fpc_attribute_group);
	//wakeup_source_trash(&fpc->ttw_wl);
	wakeup_source_remove(fpc->ttw_wl);
	dev_info(&spidev->dev, "%s\n", __func__);

	return 0;
}

static struct of_device_id mt6797_of_match[] = {
	{ .compatible = "mediatek,fpsensor", },
	{}
};
MODULE_DEVICE_TABLE(of, mt6797_of_match);

static struct spi_driver mtk6797_driver = {
	.driver = {
		.name	= "fpc_spi",
		.bus = &spi_bus_type,
		.owner	= THIS_MODULE,
		.of_match_table = mt6797_of_match,
	},
	.probe	= mtk6797_probe,
	.remove	= mtk6797_remove
};

static int __init fpc_sensor_init(void)
{
	int status;

	status = spi_register_driver(&mtk6797_driver);
	if (status < 0) {
		printk("%s, fpc_sensor_init failed.\n", __func__);
	}

#ifdef FPC_DRIVER_FOR_ISEE
	memcpy(&uuid_fp, &uuid_ta_fpc, sizeof(struct TEEC_UUID));
#endif

	return status;
}
module_init(fpc_sensor_init);

static void __exit fpc_sensor_exit(void)
{
	spi_unregister_driver(&mtk6797_driver);
}
module_exit(fpc_sensor_exit);

MODULE_LICENSE("GPL");
