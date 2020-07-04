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

#define DEBUG
#define pr_fmt(fmt) "fingerprint %s +%d, " fmt, __func__,__LINE__

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

#define FPC_DRIVER_FOR_ISEE

#ifdef FPC_DRIVER_FOR_ISEE
#include "teei_fp.h"
#include "tee_client_api.h"
#endif

#include "ontim/ontim_dev_dgb.h"
#define FPC_HW_INFO "FPC1520"
DEV_ATTR_DECLARE(fingersensor)
DEV_ATTR_DEFINE("vendor", FPC_HW_INFO)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(fingersensor, fingersensor, 8);


#ifdef FPC_DRIVER_FOR_ISEE
struct TEEC_UUID vendor_uuid = { 0x7778c03f, 0xc30c, 0x4dd0,
    { 0xa3, 0x19, 0xea, 0x29, 0x64, 0x3d, 0x4d, 0x4b }};
#endif


enum pinctrl_pinnum_t{
    FPC_RST_PIN,
    FPC_EINT_PIN,
    FPC_SPI_CS_PIN,
    FPC_SPI_CLK_PIN,
    FPC_SPI_MO_PIN,
    FPC_SPI_MI_PIN
};

static const char * const pinctrl_names[] = {
    "fpsensor_fpc_rst",
    "fpsensor_fpc_eint",
    "fpsensor_spi_cs",
    "fpsensor_spi_clk",
    "fpsensor_spi_mo",
    "fpsensor_spi_mi"
};

#define FPC_RESET_LOW_US 5000
#define FPC_RESET_HIGH1_US 100
#define FPC_RESET_HIGH2_US 5000
#define FPC_TTW_HOLD_TIME 1000

#define GET_STR_BYSTATUS(status) (status==0? "Success":"Failure")
#define ERROR_LOG       (0)
#define INFO_LOG        (1)
#define DEBUG_LOG       (2)

/* debug log setting */
static u8 debug_level = ERROR_LOG; //DEBUG_LOG;
#define fpsensor_log(level, fmt, args...) do { \
    if (debug_level >= level) {\
        if( level == DEBUG_LOG) { \
            printk("fingerprint %s +%d, " fmt, __func__, __LINE__, ##args); \
        } else { \
            printk("fingerprint: " fmt, ##args); \
        }\
    } \
} while (0)
#define func_entry()    do{fpsensor_log(DEBUG_LOG, "%s, %d, entry\n", __func__, __LINE__);}while(0)
#define func_exit()     do{fpsensor_log(DEBUG_LOG, "%s, %d, exit\n", __func__, __LINE__);}while(0)


struct fpc_data {
    struct device *dev;
    struct platform_device *pdev;
    struct pinctrl *fpc_pinctrl;
    struct pinctrl_state *fpc_rst_low;
    struct pinctrl_state *fpc_rst_high;
    struct pinctrl_state *fpc_eint_low;
    struct pinctrl_state *spi_cs_low;
    struct pinctrl_state *spi_cs_high;
    struct pinctrl_state *spi_clk_low;
    struct pinctrl_state *spi_clk_high;
    struct pinctrl_state *spi_mo_low;
    struct pinctrl_state *spi_mi_low;
    int prop_wakeup;
    int irq_gpio;
    int irq_num;
    int rst_gpio;
    bool wakeup_enabled;
    struct wakeup_source ttw_wl;
    bool clocks_enabled;
};

static DEFINE_MUTEX(fpc_set_gpio_mutex);

//extern void mt_spi_disable_master_clk(struct spi_device *spidev);
//extern void mt_spi_enable_master_clk(struct spi_device *spidev);
extern void fpsensor_spi_clk_enable(u8 bonoff);

static int fpc_dts_pinctrl_init( struct fpc_data * fpc);
static void fpc_dts_pinctrl_config( struct fpc_data *fpc, enum pinctrl_pinnum_t pinnum, int config);

static int fpc_dts_irq_init(struct fpc_data *fpc);
static irqreturn_t fpc_irq_handler(int irq, void *handle);

#if 0
static int select_pin_ctl(struct fpc_data *fpc, const char *name)
{
    size_t i;
    int rc;
    struct device *dev = fpc->dev;
    for (i = 0; i < ARRAY_SIZE(fpc->pinctrl_state); i++) {
        const char *n = pctl_names[i];
        if (!strncmp(n, name, strlen(n))) {
            mutex_lock(&fpc_set_gpio_mutex);
            rc = pinctrl_select_state(fpc->fpc_pinctrl, fpc->pinctrl_state[i]);
            mutex_unlock(&fpc_set_gpio_mutex);
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
#endif
static int set_clks(struct fpc_data *fpc, bool enable)
{
    int rc = 0;
    if(fpc->clocks_enabled == enable)
        return rc;
    if (enable) {
        //mt_spi_enable_master_clk(fpc->spidev);
        fpsensor_spi_clk_enable(1);
        fpc->clocks_enabled = true;
        rc = 1;
    } else {
        fpsensor_spi_clk_enable(0);
        //mt_spi_disable_master_clk(fpc->spidev);
        fpc->clocks_enabled = false;
        rc = 0;
    }

    return rc;
}

static int hw_reset(struct  fpc_data *fpc)
{
    int irq_value = 0;

    fpc_dts_pinctrl_config( fpc, FPC_RST_PIN, 1);
    usleep_range(FPC_RESET_HIGH1_US, FPC_RESET_HIGH1_US + 100);

    fpc_dts_pinctrl_config( fpc, FPC_RST_PIN, 0);
    usleep_range(FPC_RESET_LOW_US, FPC_RESET_LOW_US + 100);

    fpc_dts_pinctrl_config( fpc, FPC_RST_PIN, 1);
    usleep_range(FPC_RESET_HIGH2_US, FPC_RESET_HIGH2_US + 100);

    irq_value = gpio_get_value(fpc->irq_gpio);
    fpsensor_log(DEBUG_LOG, "IRQ after reset is %d\n", irq_value);

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

    fpsensor_log(DEBUG_LOG, "config=%s\n", buf);

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
    struct fpc_data *fpc = dev_get_drvdata(device);
    return set_clks(fpc, (*buf == '1')) ? : count;
}
static DEVICE_ATTR(clk_enable, S_IWUSR, NULL, clk_enable_set);


static ssize_t sensor_init(struct device *device,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct fpc_data *fpc = dev_get_drvdata(device);
    int ret = -1;
    int irqf = 0;

    fpsensor_log( INFO_LOG, "entry\n");

    if( !fpc) {
        fpsensor_log(ERROR_LOG, "data is null, init failed");
        return -EINVAL;
    }
    ret = fpc_dts_pinctrl_init(fpc);
    if( ret) {
        fpsensor_log(ERROR_LOG, "fpc init failed\n");
        goto init_exit;
    }
    ret = fpc_dts_irq_init(fpc);
    if( ret) {
        fpsensor_log(ERROR_LOG, "fpc init failed\n");
        goto init_exit;
    }

    fpc->clocks_enabled = false;
    fpc->wakeup_enabled = false;

    irqf = IRQF_TRIGGER_HIGH | IRQF_ONESHOT;
    if( fpc->prop_wakeup){
        irqf |= IRQF_NO_SUSPEND;
        device_init_wakeup(fpc->dev, 1);
    }
    ret = devm_request_threaded_irq( fpc->dev, fpc->irq_num,
            NULL, fpc_irq_handler, irqf,
            dev_name(fpc->dev), fpc);
    if (ret) {
        fpsensor_log( ERROR_LOG, "could not request irq %d\n", fpc->irq_num);
        goto init_exit;
    }
    fpsensor_log(INFO_LOG, "requested irq, irq_gpio=%d, irq_num=%d\n", fpc->irq_gpio, fpc->irq_num);

    /* Request that the interrupt should be wakeable */
    enable_irq_wake(fpc->irq_num);
    wakeup_source_init(&fpc->ttw_wl, "fpc_ttw_wl");

#ifdef FPC_DRIVER_FOR_ISEE
    memcpy(&uuid_fp, &vendor_uuid, sizeof(struct TEEC_UUID));
#endif

    (void)hw_reset(fpc);

    fpsensor_log( INFO_LOG, "exit\n");
init_exit:
    return ret ? ret : count;
}

static ssize_t sensor_release(struct device *device,
        struct device_attribute *attribute,
        char* buffer)
{
    struct fpc_data *fpc = dev_get_drvdata(device);

    fpsensor_log(INFO_LOG, "entry\n");
    if( fpc->irq_num) {
        disable_irq_wake(fpc->irq_num);
        free_irq(fpc->irq_num, fpc);
        fpc->irq_num = 0;
    }
    fpsensor_log(INFO_LOG, "exit\n");

    return 1;
}

static DEVICE_ATTR(sensor, S_IRUSR | S_IWUSR, sensor_release, sensor_init);

static ssize_t hwinfo_set(struct device *device,
        struct device_attribute *attr, const char *buf, size_t count)
{
    //struct fpc_data *fpc = dev_get_drvdata(device);
    CHECK_THIS_DEV_DEBUG_AREADY_EXIT();
    REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();

    fpsensor_log(INFO_LOG, "set hwinfo");

    return count;
}

static DEVICE_ATTR(hwinfo, S_IWUSR, NULL, hwinfo_set);

static struct attribute *fpc_attributes[] = {
    &dev_attr_hw_reset.attr,
    &dev_attr_wakeup_enable.attr,
    &dev_attr_clk_enable.attr,
    &dev_attr_irq.attr,
    &dev_attr_sensor.attr,
    &dev_attr_hwinfo.attr,
    NULL
};

static const struct attribute_group const fpc_attribute_group = {
    .attrs = fpc_attributes,
};
static irqreturn_t fpc_irq_handler(int irq, void *handle)
{
    struct fpc_data *fpc = handle;
    struct device *dev = fpc->dev;

    static int current_level = 0; // We assume low level from start

    current_level = !current_level;

    if (current_level) {
        fpsensor_log(DEBUG_LOG, "Reconfigure irq to trigger in low level\n");
        irq_set_irq_type(irq, IRQF_TRIGGER_LOW);
    } else {
        fpsensor_log(DEBUG_LOG, "Reconfigure irq to trigger in high level\n");
        irq_set_irq_type(irq, IRQF_TRIGGER_HIGH);
    }

    /* Make sure 'wakeup_enabled' is updated before using it
     ** since this is interrupt context (other thread...) */
    smp_rmb();
    if (fpc->wakeup_enabled) {
        __pm_wakeup_event(&fpc->ttw_wl, FPC_TTW_HOLD_TIME);
    }

    sysfs_notify(&dev->kobj, NULL, dev_attr_irq.attr.name);

    return IRQ_HANDLED;
}
static int fpc_dts_irq_init(struct fpc_data *fpc)
{
    struct device_node *node;
    u32 ints[2] = {0, 0};

    func_entry();

    fpc_dts_pinctrl_config(fpc, FPC_EINT_PIN, 0);

    node = of_find_compatible_node(NULL, NULL, "mediatek,fpc-eint");
    if ( node) {
        fpc->irq_gpio = of_get_named_gpio(node, "fpc,gpio-irq", 0);

        of_property_read_u32_array( node, "debounce", ints, ARRAY_SIZE(ints));
        fpsensor_log(DEBUG_LOG,"debounce[0]=%d(gpio), debounce[1]=%d\n", ints[0], ints[1]);
        //fpc->irq_gpio = ints[0];

        fpc->irq_num = irq_of_parse_and_map(node, 0);  // get irq number
        if (!fpc->irq_num) {
            fpsensor_log(ERROR_LOG,"fpc irq_of_parse_and_map failed, irq_gpio=%d, irq_num=%d\n",
                    fpc->irq_gpio, fpc->irq_num);
            return -EINVAL;
        }
        fpsensor_log(INFO_LOG,"fpc->irq_gpio=%d, fpc->irq_num=%d\n", fpc->irq_gpio,
                fpc->irq_num);
    } else {
        fpsensor_log(ERROR_LOG,"Canot find node!\n");
        return -EINVAL;
    }

    func_exit();

    return 0;
}
static void fpc_dts_pinctrl_config( struct fpc_data *fpc, enum pinctrl_pinnum_t pinnum, int config)
{
    mutex_lock(&fpc_set_gpio_mutex);

    fpsensor_log(DEBUG_LOG, "Pinctrl Config: config=%d, pinctrl_name=%s%s\n", config,
            pinctrl_names[pinnum], config?"_high":"_low");

    if (pinnum == FPC_RST_PIN) {
        if (!config) {
            pinctrl_select_state(fpc->fpc_pinctrl, fpc->fpc_rst_low);
        } else {
            pinctrl_select_state(fpc->fpc_pinctrl, fpc->fpc_rst_high);
        }
    } else if ( pinnum == FPC_SPI_CS_PIN) {
        if(config) {
            pinctrl_select_state(fpc->fpc_pinctrl, fpc->spi_cs_high);
        } else {
            pinctrl_select_state(fpc->fpc_pinctrl, fpc->spi_cs_low);
        }
    } else if (pinnum == FPC_SPI_MO_PIN) {
        pinctrl_select_state(fpc->fpc_pinctrl, fpc->spi_mo_low);
    } else if (pinnum == FPC_SPI_MI_PIN) {
        pinctrl_select_state(fpc->fpc_pinctrl, fpc->spi_mi_low);
    } else if (pinnum == FPC_SPI_CLK_PIN) {
        pinctrl_select_state(fpc->fpc_pinctrl, fpc->spi_clk_low);
    } else if (pinnum == FPC_EINT_PIN) {
        if (!config) {
            pinctrl_select_state(fpc->fpc_pinctrl, fpc->fpc_eint_low);
        } else {
            fpsensor_log(ERROR_LOG, "%s not config(%d)\n", pinctrl_names[pinnum-1], config);
        }
    } else {
        fpsensor_log(ERROR_LOG, "%s(%d) not found\n", pinctrl_names[pinnum-1], config);
    }

    mutex_unlock(&fpc_set_gpio_mutex);
}

static int fpc_dts_pinctrl_init( struct fpc_data * fpc)
{
    struct device_node *node = NULL;
    struct platform_device *pdev = NULL;
    int ret = 0;

    node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint-pinctrl");
    if (node) {
        pdev = of_find_device_by_node(node);
        if(pdev) {
            fpc->fpc_pinctrl = devm_pinctrl_get(&pdev->dev);
            if (IS_ERR(fpc->fpc_pinctrl)) {
                ret = PTR_ERR(fpc->fpc_pinctrl);
                fpsensor_log(ERROR_LOG,"Cannot find pinctrl!\n");
                goto dts_init_exit;
            }
        } else {
            ret = -ENODEV;
            fpsensor_log(ERROR_LOG,"Cannot find pinctrl device!\n");
            goto dts_init_exit;
        }
        fpc->fpc_eint_low = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_eint_low");
        if (IS_ERR(fpc->fpc_eint_low)) {
            ret = PTR_ERR(fpc->fpc_eint_low);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_eint_low!\n");
            goto dts_init_exit;
        }

        fpc->fpc_rst_low = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_rst_low");
        if (IS_ERR(fpc->fpc_rst_low)) {
            ret = PTR_ERR(fpc->fpc_rst_low);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_rst_low!\n");
            goto dts_init_exit;
        }

        fpc->fpc_rst_high = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_rst_high");
        if (IS_ERR(fpc->fpc_rst_high)) {
            ret = PTR_ERR(fpc->fpc_rst_high);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_rst_high!\n");
            goto dts_init_exit;
        }
        fpc->spi_cs_low = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_spi_cs_low");
        if (IS_ERR(fpc->spi_cs_low)) {
            ret = PTR_ERR(fpc->spi_cs_low);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_spi_cs_low!\n");
            goto dts_init_exit;
        }

        fpc->spi_cs_high = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_spi_cs_high");
        if (IS_ERR(fpc->spi_cs_high)) {
            ret = PTR_ERR(fpc->spi_cs_high);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_spi_cs_high!\n");
            goto dts_init_exit;
        }
        fpc->spi_clk_low = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_spi_clk_low");
        if (IS_ERR(fpc->spi_clk_low)) {
            ret = PTR_ERR(fpc->spi_clk_low);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_spi_clk_low!\n");
            goto dts_init_exit;
        }

        fpc->spi_clk_high = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_spi_clk_high");
        if (IS_ERR(fpc->spi_clk_high)) {
            ret = PTR_ERR(fpc->spi_clk_high);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_spi_clk_high!\n");
            goto dts_init_exit;
        }

        fpc->spi_mo_low = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_spi_mo_low");
        if (IS_ERR(fpc->spi_mo_low)) {
            ret = PTR_ERR(fpc->spi_mo_low);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_spi_mo_low!\n");
            goto dts_init_exit;
        }

        fpc->spi_mi_low = pinctrl_lookup_state(fpc->fpc_pinctrl, "fpsensor_spi_mi_low");
        if (IS_ERR(fpc->spi_mi_low)) {
            ret = PTR_ERR(fpc->spi_mi_low);
            fpsensor_log(ERROR_LOG, "Cannot find fpc pinctrl fpsensor_spi_mi_low!\n");
            goto dts_init_exit;
        }

        fpc_dts_pinctrl_config(fpc, FPC_RST_PIN, 1);
        fpc_dts_pinctrl_config(fpc, FPC_EINT_PIN, 0);

        fpc_dts_pinctrl_config(fpc, FPC_SPI_CS_PIN, 1);
        fpc_dts_pinctrl_config(fpc, FPC_SPI_CLK_PIN, 0);
        fpc_dts_pinctrl_config(fpc, FPC_SPI_MO_PIN, 0);
        fpc_dts_pinctrl_config(fpc, FPC_SPI_MI_PIN, 0);
    } else {
        fpsensor_log(ERROR_LOG,"Cannot find node!\n");
        ret = -ENODEV;
    }

dts_init_exit:
    return ret;
}
#if 0
int fpc_init_test( struct device *device)
{
    struct fpc_data *fpc = dev_get_drvdata(device);
    int ret = -1;
    int irqf = 0;

    ret = fpc_dts_pinctrl_init(fpc);
    if( ret) {
        fpsensor_log(ERROR_LOG, "fpc init failed\n");
        goto fpc_init_test_exit;
    }
    ret = fpc_dts_irq_init(fpc);
    if( ret) {
        fpsensor_log(ERROR_LOG, "fpc init failed\n");
        goto fpc_init_test_exit;
    }

    fpc->clocks_enabled = false;
    fpc->wakeup_enabled = false;

    irqf = IRQF_TRIGGER_HIGH | IRQF_ONESHOT;
    if( fpc->prop_wakeup){
        irqf |= IRQF_NO_SUSPEND;
        device_init_wakeup(fpc->dev, 1);
    }
    ret = devm_request_threaded_irq( fpc->dev, fpc->irq_num,
            NULL, fpc_irq_handler, irqf,
            dev_name(fpc->dev), fpc);
    if (ret) {
        fpsensor_log( ERROR_LOG, "could not request irq %d\n", fpc->irq_num);
        goto fpc_init_test_exit;
    }
    fpsensor_log(ERROR_LOG, "requested irq, irq_gpio=%d, irq_num=%d\n", fpc->irq_gpio, fpc->irq_num);

    /* Request that the interrupt should be wakeable */
    enable_irq_wake(fpc->irq_num);
    wakeup_source_init(&fpc->ttw_wl, "fpc_ttw_wl");

#ifdef FPC_DRIVER_FOR_ISEE
    memcpy(&uuid_fp, &vendor_uuid, sizeof(struct TEEC_UUID));
#endif

    (void)hw_reset(fpc);

fpc_init_test_exit:
    return ret;
}
#endif
static int fpc_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct fpc_data *fpc;
    int rc = 0;

    fpsensor_log( DEBUG_LOG, "entry\n");

    fpc = devm_kzalloc(dev, sizeof(*fpc), GFP_KERNEL);
    if (!fpc) {
        fpsensor_log( ERROR_LOG, "failed to allocate memory for struct fpc_data\n");
        rc = -ENOMEM;
        goto fpc_probe_exit;
    }
    memset( fpc, 0, sizeof(struct fpc_data));

    fpc->dev = dev;
    fpc->pdev = pdev;

    if (of_property_read_bool(dev->of_node, "fpc,enable-wakeup")) {
        fpc->prop_wakeup = 1;
    } else {
        fpc->prop_wakeup = 0;
    }
    fpsensor_log(DEBUG_LOG, "prop_wakeup=%d\n", fpc->prop_wakeup);

    dev_set_drvdata(dev, fpc);

    rc = sysfs_create_group(&dev->kobj, &fpc_attribute_group);
    if (rc) {
        fpsensor_log( ERROR_LOG, "create sysfs failed\n");
        goto fpc_probe_exit;
    }
#if 0
    rc = fpc_init_test( dev);
    pr_err("%s +%d, fpc init, rc=%d\n", __func__, __LINE__, rc);
#endif
    fpsensor_log(INFO_LOG, "FPC Module Probing Success\n");

    return rc;

fpc_probe_exit:
    if( fpc) {
        kfree(fpc);
    }
    pr_info("FPC Module Probing Failure, ret=%d\n", rc);

    return rc;
}

static int fpc_remove(struct platform_device *pdev)
{
    struct  fpc_data *fpc = dev_get_drvdata(&pdev->dev);

    sysfs_remove_group(&pdev->dev.kobj, &fpc_attribute_group);
    wakeup_source_trash(&fpc->ttw_wl);

    fpsensor_log(INFO_LOG, "FPC Module Remove\n");

    return 0;
}
#ifdef CONFIG_OF
static struct of_device_id fpc_of_match[] = {
    { .compatible = "mediatek,fingerprint-fpc", },
    {}
};
MODULE_DEVICE_TABLE(of, fpc_of_match);
#endif

static struct platform_driver fpc_plat_driver =
{
    .driver = {
        .name   = "fpc",
        .bus    = &platform_bus_type,
        .owner	= THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = fpc_of_match,
#endif
    },
    .probe	= fpc_probe,
    .remove	= fpc_remove,
};

static int __init fpc_init(void)
{
    int status;

    status = platform_driver_register(&fpc_plat_driver);

    fpsensor_log(INFO_LOG, "FPC Module Init, %s\n", GET_STR_BYSTATUS(status));

    return status;
}
module_init(fpc_init);

static void __exit fpc_exit(void)
{
    platform_driver_unregister(&fpc_plat_driver);

    fpsensor_log(INFO_LOG, "FPC Module Exit\n");
}
module_exit(fpc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("fingerprint driver, fpc, sw30.1");
MODULE_ALIAS("fingerprint:fpc-drivers");
