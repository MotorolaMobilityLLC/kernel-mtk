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

#define FPC_RESET_LOW_US 5000
#define FPC_RESET_HIGH1_US 100
#define FPC_RESET_HIGH2_US 5000
#define FPC_TTW_HOLD_TIME 1000

#define REE_READ_HWID

static const char * const pctl_names[] = {
	"fpsensor_fpc_rst_low",
	"fpsensor_fpc_rst_high",
};

struct fpc_data {
	struct device *dev;
	struct spi_device *spidev;
	struct pinctrl *pinctrl_fpc;
	struct pinctrl_state *pinctrl_state[ARRAY_SIZE(pctl_names)];
	int irq_gpio;
	int irq_num;
	int rst_gpio;
	bool wakeup_enabled;
	bool init;
	struct wakeup_source ttw_wl;
};

static DEFINE_MUTEX(spidev_set_gpio_mutex);

extern void mt_spi_disable_master_clk(struct spi_device *spidev);
extern void mt_spi_enable_master_clk(struct spi_device *spidev);

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

static int set_clks(struct fpc_data *fpc, bool enable)
{
	int rc = 0;

	if (enable) {
		mt_spi_enable_master_clk(fpc->spidev);
		rc = 1;
	} else {
		mt_spi_disable_master_clk(fpc->spidev);
		rc = 0;
	}

	return rc;
}

static int hw_reset(struct  fpc_data *fpc)
{
	int irq_gpio;
	struct device *dev = fpc->dev;

	select_pin_ctl(fpc, "fpsensor_fpc_rst_high");
	usleep_range(FPC_RESET_HIGH1_US, FPC_RESET_HIGH1_US + 100);

	select_pin_ctl(fpc, "fpsensor_fpc_rst_low");
	usleep_range(FPC_RESET_LOW_US, FPC_RESET_LOW_US + 100);

	select_pin_ctl(fpc, "fpsensor_fpc_rst_high");
	usleep_range(FPC_RESET_HIGH2_US, FPC_RESET_HIGH2_US + 100);

	irq_gpio = gpio_get_value(fpc->irq_gpio);
	dev_info(dev, "IRQ after reset %d\n", irq_gpio);

	dev_info(dev, "Using GPIO#%d as IRQ.\n", fpc->irq_gpio);
	dev_info(dev, "Using GPIO#%d as RST.\n", fpc->rst_gpio);

	return 0;
}

static ssize_t hw_reset_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct  fpc_data *fpc = dev_get_drvdata(dev);

	if (!strncmp(buf, "reset", strlen("reset"))) {
		rc = hw_reset(fpc);
		return rc ? rc : count;
	} else {
		return -EINVAL;
	}
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
	} else {
		ret = -EINVAL;
	}
	return ret;
}
static DEVICE_ATTR(wakeup_enable, S_IWUSR, NULL, wakeup_enable_set);

static ssize_t active_get(struct device *device,
			struct device_attribute *attribute,
			char *buffer)
{
	struct fpc_data *fpc = dev_get_drvdata(device);
	int result = 0;

	if (fpc->init) {
		result = 1;
	}

	return scnprintf(buffer, PAGE_SIZE, "%i\n", result);
}
static DEVICE_ATTR(active, S_IRUSR, active_get, NULL);

/**
 * sysf node to check the interrupt status of the sensor, the interrupt
 * handler should perform sysf_notify to allow userland to poll the node.
 */
static ssize_t irq_get(struct device *device,
			struct device_attribute *attribute,
			char *buffer)
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
	struct fpc_data *fpc = dev_get_drvdata(device);
	return set_clks(fpc, (*buf == '1')) ? : count;
}
static DEVICE_ATTR(clk_enable, S_IWUSR, NULL, clk_enable_set);

static struct attribute *fpc_attributes[] = {
	&dev_attr_hw_reset.attr,
	&dev_attr_wakeup_enable.attr,
	&dev_attr_clk_enable.attr,
	&dev_attr_irq.attr,
	&dev_attr_active.attr,
	NULL
};

static const struct attribute_group const fpc_attribute_group = {
	.attrs = fpc_attributes,
};

static irqreturn_t fpc_irq_handler(int irq, void *handle)
{
	struct fpc_data *fpc = handle;
	struct device *dev = fpc->dev;
	static int current_level;

	current_level = !current_level;
	if (current_level) {
		dev_dbg(dev, "Reconfigure irq to trigger in low level\n");
		irq_set_irq_type(irq, IRQF_TRIGGER_LOW);
	} else {
		dev_dbg(dev, "Reconfigure irq to trigger in high level\n");
		irq_set_irq_type(irq, IRQF_TRIGGER_HIGH);
	}

	/* Make sure 'wakeup_enabled' is updated before using it
	** since this is interrupt context (other thread...)
	*/
	smp_rmb();

	if (fpc->wakeup_enabled)
		__pm_wakeup_event(&fpc->ttw_wl, FPC_TTW_HOLD_TIME);

	sysfs_notify(&fpc->dev->kobj, NULL, dev_attr_irq.attr.name);

	return IRQ_HANDLED;
}

#ifdef REE_READ_HWID
int fpc1020_read_hwid(struct spi_device *spidev)
{
	struct spi_message msg;
	u8 temp_buffer[32];
	int error = 0;

	struct spi_transfer cmd = {
		.cs_change = 0,
		.delay_usecs = 0,
		.speed_hz = 4800000u,
		.tx_buf = temp_buffer,
		.rx_buf = temp_buffer,
		.len    = 16,
		.tx_dma = 0,
		.rx_dma = 0,
		.bits_per_word = 8,
	};

	temp_buffer[0] = 0xFC;
	spi_message_init(&msg);
	spi_message_add_tail(&cmd,  &msg);

	error = spi_sync(spidev, &msg);

	printk("%s : spi_sync error:%d.\n", __func__, error);

	printk("fpc hwid: 0x%x,  0x%x,  0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x,0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
				temp_buffer[0], temp_buffer[1], temp_buffer[2], temp_buffer[3],
				temp_buffer[4], temp_buffer[5],
				temp_buffer[6], temp_buffer[7],
				temp_buffer[8], temp_buffer[9],
				temp_buffer[10], temp_buffer[11],
				temp_buffer[12], temp_buffer[13],
				temp_buffer[14], temp_buffer[15]);

	if (temp_buffer[1] == 0x10) {
		if (temp_buffer[2] == 0x12 ||
			temp_buffer[2] == 0x13 ||
			temp_buffer[2] == 0x22 ||
			temp_buffer[2] == 0x23)
		{
			return 0;
		}
	}

	return -EINVAL;

}
#endif
/* MMI_STOPSHIP <FPC driver>: Temp extern gspidev */
struct spi_device* gspidev;
static int mtk6797_probe(struct spi_device *spidev)
{
	struct device *dev = &spidev->dev;
	struct device_node *node_eint;
	struct fpc_data *fpc;
	int irqf = 0;
	int irq_num = 0;
	int rc = 0;
	size_t i;

	dev_dbg(dev, "%s\n", __func__);

/* MMI_STOPSHIP <FPC driver>: Temp extern gspidev */
        gspidev = spidev;
	spidev->dev.of_node = of_find_compatible_node(NULL,
						NULL, "mediatek,fingerprint");
	if (!spidev->dev.of_node) {
		dev_err(dev, "no of node found\n");
		rc = -EINVAL;
		goto exit;
	}

	fpc = devm_kzalloc(dev, sizeof(*fpc), GFP_KERNEL);
	if (!fpc) {
		dev_err(dev, "failed to allocate memory for struct fpc_data\n");
		rc = -ENOMEM;
		goto exit;
	}

	fpc->dev = dev;
	dev_set_drvdata(dev, fpc);
	fpc->spidev = spidev;
	fpc->spidev->irq = 0; /*SPI_MODE_0*/

	fpc->pinctrl_fpc = devm_pinctrl_get(&spidev->dev);
	if (IS_ERR(fpc->pinctrl_fpc)) {
		rc = PTR_ERR(fpc->pinctrl_fpc);
		dev_err(fpc->dev, "Cannot find pinctrl_fpc rc = %d.\n", rc);
		goto exit;
	}

	for (i = 0; i < ARRAY_SIZE(fpc->pinctrl_state); i++) {
		const char *n = pctl_names[i];
		struct pinctrl_state *state = pinctrl_lookup_state(fpc->pinctrl_fpc, n);
		if (IS_ERR(state)) {
			dev_err(dev, "cannot find '%s'\n", n);
			rc = -EINVAL;
			goto exit;
		}
		dev_info(dev, "found pin control %s\n", n);
		fpc->pinctrl_state[i] = state;
	}

	hw_reset(fpc);
	if (0 != fpc1020_read_hwid(spidev)) {
		rc = -EINVAL;
		dev_err(fpc->dev, "fpc1020_read_hwid failed.\n");
		devm_pinctrl_put(fpc->pinctrl_fpc);
		devm_kfree(dev, fpc);
		goto exit;
	}

	node_eint = of_find_compatible_node(NULL, NULL,
				"mediatek,fpsensor_fp_eint");
	if (node_eint == NULL) {
		rc = -EINVAL;
		dev_err(fpc->dev, "cannot find node_eint rc = %d.\n", rc);
		goto exit;
	}

	fpc->irq_gpio = of_get_named_gpio(node_eint, "int-gpios", 0);
	irq_num = irq_of_parse_and_map(node_eint, 0);/*get irq num*/
	if (!irq_num) {
		rc = -EINVAL;
		dev_err(fpc->dev, "get irq_num error rc = %d.\n", rc);
		goto exit;
	}

	fpc->irq_num = irq_num;

	//set_clks(fpc, true);

	dev_dbg(dev, "Using GPIO#%d as IRQ.\n", fpc->irq_gpio);
	dev_dbg(dev, "Using GPIO#%d as RST.\n", fpc->rst_gpio);

	fpc->wakeup_enabled = false;

	irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT;
	if (of_property_read_bool(dev->of_node, "fpc,enable-wakeup")) {
		irqf |= IRQF_NO_SUSPEND;
		device_init_wakeup(dev, 1);
	}
	rc = devm_request_threaded_irq(dev, irq_num,
		NULL, fpc_irq_handler, irqf,
		dev_name(dev), fpc);
	if (rc) {
		dev_err(dev, "could not request irq %d\n", irq_num);
		goto exit;
	}
	dev_info(dev, "requested irq %d\n", irq_num);

	/* Request that the interrupt should be wakeable */
	enable_irq_wake(irq_num);
	wakeup_source_init(&fpc->ttw_wl, "fpc_ttw_wl");

	rc = sysfs_create_group(&dev->kobj, &fpc_attribute_group);
	if (rc) {
		dev_err(dev, "could not create sysfs\n");
		goto exit;
	}

	fpc->init = true;

	dev_info(dev, "%s: ok\n", __func__);

exit:
	return rc;
}

static int mtk6797_remove(struct spi_device *spidev)
{
	struct  fpc_data *fpc = dev_get_drvdata(&spidev->dev);

	sysfs_remove_group(&spidev->dev.kobj, &fpc_attribute_group);
	wakeup_source_trash(&fpc->ttw_wl);
	dev_info(&spidev->dev, "%s\n", __func__);

	return 0;
}

static struct of_device_id mt6797_of_match[] = {
	{ .compatible = "fpc,fpc_spi", },
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
	if (status < 0)
		printk("%s, fpc_sensor_init failed.\n", __func__);

	return status;
}
module_init(fpc_sensor_init);

static void __exit fpc_sensor_exit(void)
{
	spi_unregister_driver(&mtk6797_driver);
}
module_exit(fpc_sensor_exit);

MODULE_LICENSE("GPL");
