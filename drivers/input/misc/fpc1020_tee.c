/*
 * FPC1020 Fingerprint sensor device driver
 *
 * This driver will control the platform resources that the FPC fingerprint
 * sensor needs to operate. The major things are probing the sensor to check
 * that it is actually connected and let the Kernel know this and with that also
 * enabling and disabling of regulators, enabling and disabling of platform
 * clocks, controlling GPIOs such as SPI chip select, sensor reset line, sensor
 * IRQ line, MISO and MOSI lines.
 *
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
 * This driver will NOT send any SPI commands to the sensor it only controls the
 * electrical parts.
 *
 *
 * Copyright (c) 2015 Fingerprint Cards AB <tech@fingerprints.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/wakelock.h>

#define FPC1020_RESET_LOW_US 1000
#define FPC1020_RESET_HIGH1_US 100
#define FPC1020_RESET_HIGH2_US 1250
#define PWR_ON_STEP_SLEEP 100
#define PWR_ON_STEP_RANGE1 100
#define PWR_ON_STEP_RANGE2 900
#define FPC_TTW_HOLD_TIME 1000
#define NUM_PARAMS_REG_ENABLE_SET 2

#define FPC_IRQ_INPUT_DEV_NAME  "fpc1020_input"
 
//#define FPC_IRQ_KEY_WAKEUP      KEY_F18 /* 188*/
//#define FPC_IRQ_KEY_ANDR_BACK   KEY_BACK
#define FPC_IRQ_KEY_RIGHT       (0x22E)
#define FPC_IRQ_KEY_LEFT        (0x22F)
#define FPC_IRQ_KEY_DOWN        (0x230)
#define FPC_IRQ_KEY_UP          (0x231)
#define FPC_IRQ_KEY_CLICK       (0x232)
#define FPC_IRQ_KEY_HOLD        (0x233)
struct fpc1020_data {
	struct device *dev;
	struct spi_device *spi;
	struct input_dev *input_dev;

	struct wake_lock ttw_wl;
	int irq_gpio;
	int rst_gpio;
	struct mutex lock;
	bool wakeup_enabled;
	struct device_node *irq_node;
	int		irq;

	struct work_struct irq_worker;
};

extern void mt_spi_enable_master_clk(struct spi_device* spidev);
extern void mt_spi_disable_master_clk(struct spi_device* spidev);

static int hw_reset(struct  fpc1020_data *fpc1020)
{
	int irq_gpio;
	struct device *dev = fpc1020->dev;

	gpio_export(fpc1020->rst_gpio, 1);
	usleep_range(FPC1020_RESET_HIGH1_US, FPC1020_RESET_HIGH1_US + 100);
	gpio_export(fpc1020->rst_gpio, 0);
	usleep_range(FPC1020_RESET_LOW_US, FPC1020_RESET_LOW_US + 100);
	gpio_export(fpc1020->rst_gpio, 1);
	usleep_range(FPC1020_RESET_HIGH1_US, FPC1020_RESET_HIGH1_US + 100);

	irq_gpio = gpio_get_value(fpc1020->irq_gpio);
	dev_info(dev, "IRQ after reset %d\n", irq_gpio);

	return 0;
}

static ssize_t hw_reset_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	struct  fpc1020_data *fpc1020 = dev_get_drvdata(dev);

	if (!strncmp(buf, "reset", strlen("reset")))
		rc = hw_reset(fpc1020);
	else
		return -EINVAL;
	return rc ? rc : count;
}
static DEVICE_ATTR(hw_reset, S_IWUSR, NULL, hw_reset_set);

static ssize_t clk_enable_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	struct  fpc1020_data *fpc1020 = dev_get_drvdata(dev);

	if (*buf == 0x30) {
		mt_spi_disable_master_clk(fpc1020->spi);
		pr_info("%s: disable\n", __func__);
	} else {
		mt_spi_enable_master_clk(fpc1020->spi);
		pr_info("%s: enable\n", __func__);
	}

    return count;
}
static DEVICE_ATTR(clk_enable, S_IWUSR|S_IWGRP, NULL, clk_enable_set);

static ssize_t spi_owner_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}
static DEVICE_ATTR(spi_owner, S_IWUSR|S_IWGRP, NULL, spi_owner_set);

static ssize_t pinctl_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}
static DEVICE_ATTR(pinctl_set, S_IWUSR|S_IWGRP, NULL, pinctl_set);

static ssize_t fabric_vote_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    return 0;
}
static DEVICE_ATTR(fabric_vote, S_IWUSR|S_IWGRP, NULL, fabric_vote_set);

static ssize_t regulator_enable_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}
static DEVICE_ATTR(regulator_enable, S_IWUSR|S_IWGRP, NULL, regulator_enable_set);

static ssize_t spi_bus_lock_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}
static DEVICE_ATTR(bus_lock, S_IWUSR|S_IWGRP, NULL, spi_bus_lock_set);

static ssize_t spi_prepare_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}
static DEVICE_ATTR(spi_prepare, S_IWUSR|S_IWGRP, NULL, spi_prepare_set);

/**
 * sysfs node for controlling whether the driver is allowed
 * to wake up the platform on interrupt.
 */
static ssize_t wakeup_enable_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct  fpc1020_data *fpc1020 = dev_get_drvdata(dev);

	if (!strncmp(buf, "enable", strlen("enable"))) {
		fpc1020->wakeup_enabled = true;
		smp_wmb();
	} else if (!strncmp(buf, "disable", strlen("disable"))) {
		fpc1020->wakeup_enabled = false;
		smp_wmb();
	} else
		return -EINVAL;

	return count;
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
	struct fpc1020_data *fpc1020 = dev_get_drvdata(device);
	int irq = gpio_get_value(fpc1020->irq_gpio);
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
	struct fpc1020_data *fpc1020 = dev_get_drvdata(device);
	dev_dbg(fpc1020->dev, "%s\n", __func__);
	return count;
}

static DEVICE_ATTR(irq, S_IRUSR | S_IWUSR, irq_get, irq_ack);

int fpc_irq_navigation_event(struct  fpc1020_data *fpc1020, int val)
{
    //pr_info("fpc_irq_navigation_event:%d \n", val);

    if (fpc1020->input_dev == NULL) {
        pr_err("fpc_irq_navigation_event,input_dev:null \n");
        return -ENODEV;
    }

    input_report_key(fpc1020->input_dev, val, 1);
    input_report_key(fpc1020->input_dev, val, 0);
    input_sync(fpc1020->input_dev);

    return 0;
}

static ssize_t navigation_event_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
    //pr_info("navigation_event_set, len:%d, buf:%s \n", count, buf);

    struct  fpc1020_data *fpc1020 = dev_get_drvdata(dev);
    u64 val;

    if (kstrtou64(buf, 0, &val)) {
        pr_err("navigation_event_set, kstrtou64 failed \n");
        return 0;
    }

    fpc_irq_navigation_event(fpc1020, (int)val);
    return count;
}
static DEVICE_ATTR(navigation_event, S_IWUSR, NULL, navigation_event_set);

static void fpc_input_dev_init(struct fpc1020_data *fpc1020) {
    fpc1020->input_dev = input_allocate_device();
    if (!fpc1020->input_dev) {
        pr_err("input_allocate_device failed \n");
        return;
    }

    fpc1020->input_dev->name = FPC_IRQ_INPUT_DEV_NAME;

    set_bit(EV_KEY,	fpc1020->input_dev->evbit);

    set_bit(FPC_IRQ_KEY_CLICK, fpc1020->input_dev->keybit);
    set_bit(FPC_IRQ_KEY_HOLD, fpc1020->input_dev->keybit);

    if (input_register_device(fpc1020->input_dev)) {
        pr_err("input_register_device failed \n");
        input_free_device(fpc1020->input_dev);
        fpc1020->input_dev = NULL;
    }
}

static void fpc_input_dev_destroy(struct fpc1020_data *fpc1020) {
    if (fpc1020->input_dev) {
        input_free_device(fpc1020->input_dev);
        fpc1020->input_dev = NULL;
    }
}

static struct attribute *attributes[] = {
    &dev_attr_pinctl_set.attr,
    &dev_attr_spi_owner.attr,
    &dev_attr_spi_prepare.attr,
    &dev_attr_fabric_vote.attr,
    &dev_attr_regulator_enable.attr,
    &dev_attr_bus_lock.attr,
    &dev_attr_hw_reset.attr,
    &dev_attr_wakeup_enable.attr,
    &dev_attr_clk_enable.attr,
    &dev_attr_irq.attr,
    &dev_attr_navigation_event.attr,
    NULL
};

static void irq_work_function(struct work_struct* work)
{
	struct fpc1020_data *fpc1020 = container_of(work, struct fpc1020_data, irq_worker);

	/* Make sure 'wakeup_enabled' is updated before using it
	** since this is interrupt context (other thread...) */
	smp_rmb();

	if (fpc1020->wakeup_enabled) {
		wake_lock_timeout(&fpc1020->ttw_wl,
					msecs_to_jiffies(FPC_TTW_HOLD_TIME));
	}

	sysfs_notify(&fpc1020->dev->kobj, NULL, dev_attr_irq.attr.name);
}

static const struct attribute_group attribute_group = {
	.attrs = attributes,
};

static irqreturn_t fpc1020_irq_handler(int irq, void *handle)
{
	struct fpc1020_data *fpc1020 = handle;

	schedule_work(&fpc1020->irq_worker);

	return IRQ_HANDLED;
}

static int fpc1020_request_named_gpio(struct fpc1020_data *fpc1020,
		const char *label, int *gpio)
{
	struct device *dev = fpc1020->dev;
	struct device_node *np = dev->of_node;
	int rc = of_property_read_u32(np, label, gpio);
	if (rc) {
		dev_err(dev, "failed to get '%s'\n", label);
		return rc;
	}
	rc = devm_gpio_request(dev, *gpio, label);
	if (rc) {
		dev_err(dev, "failed to request gpio %d\n", *gpio);
		return rc;
	}
	dev_dbg(dev, "%s %d\n", label, *gpio);
	return 0;
}

static struct of_device_id fpc1020_of_match[] = {
	{ .compatible = "fpc,fpc1020", },
	{}
};

static int fpc1020_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	int rc = 0;
	struct device_node *np = dev->of_node;
	u32 ints[2] = { 0, 0 };
	int irqf = 0;
	struct fpc1020_data *fpc1020 = devm_kzalloc(dev, sizeof(*fpc1020),
			GFP_KERNEL);
	if (!fpc1020) {
		dev_err(dev,
			"failed to allocate memory for struct fpc1020_data\n");
		rc = -ENOMEM;
		goto exit;
	}

	fpc1020->dev = dev;
	dev_set_drvdata(dev, fpc1020);
	fpc1020->spi = spi;
	mt_spi_enable_master_clk(spi);

	INIT_WORK(&fpc1020->irq_worker, irq_work_function);

	if (!np) {
		dev_err(dev, "no of node found\n");
		rc = -EINVAL;
		goto exit;
	}

	rc = fpc1020_request_named_gpio(fpc1020, "fpc,gpio_irq",
			&fpc1020->irq_gpio);
	if (rc)
		goto exit;
	rc = fpc1020_request_named_gpio(fpc1020, "fpc,gpio_rst",
			&fpc1020->rst_gpio);
	if (rc)
		goto exit;
	gpio_direction_output(fpc1020->rst_gpio, 1);
	gpio_export(fpc1020->rst_gpio, 1);

	fpc1020->wakeup_enabled = false;
	irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT;
	if (of_property_read_bool(dev->of_node, "fpc,enable-wakeup")) {
		irqf |= IRQF_NO_SUSPEND;
		device_init_wakeup(dev, 1);
	}

    	fpc_input_dev_init(fpc1020);
	mutex_init(&fpc1020->lock);

	np = of_find_compatible_node(NULL, NULL, "mediatek, fpc_int-eint");
	if (np) {
		of_property_read_u32_array(np, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);

		fpc1020->irq = irq_of_parse_and_map(np, 0);
		pr_info("%s fpc1020->irq=%d\n", __func__,fpc1020->irq);
		rc = devm_request_threaded_irq(dev,fpc1020->irq,
			NULL, fpc1020_irq_handler, irqf,
			dev_name(dev), fpc1020);
		if (rc) {
			dev_err(dev, "could not request irq %d\n",fpc1020->irq);
			goto exit;
		}
	} else {
		rc = -1;
		dev_err(dev, "of_find_matching_node fail\n");
		goto exit;
	}
	enable_irq_wake(fpc1020->irq);

	wake_lock_init(&fpc1020->ttw_wl, WAKE_LOCK_SUSPEND, "fpc_ttw_wl");

	rc = sysfs_create_group(&dev->kobj, &attribute_group);
	if (rc) {
		dev_err(dev, "could not create sysfs\n");
		goto exit;
	}

	if (of_property_read_bool(dev->of_node, "fpc,enable-on-boot")) {
		dev_info(dev, "Enabling hardware\n");
	}

	dev_info(dev, "%s: ok\n", __func__);
	hw_reset(fpc1020);
exit:
	return rc;
}

static int fpc1020_remove(struct spi_device *spi)
{
	struct  fpc1020_data *fpc1020 = dev_get_drvdata(&spi->dev);

	sysfs_remove_group(&spi->dev.kobj, &attribute_group);
	mutex_destroy(&fpc1020->lock);
	wake_lock_destroy(&fpc1020->ttw_wl);
    fpc_input_dev_destroy(fpc1020);
	dev_info(&spi->dev, "%s\n", __func__);
	return 0;
}

static int fpc1020_suspend(struct device *dev)
{
	struct  fpc1020_data *fpc1020 = dev_get_drvdata(dev);
	mt_spi_disable_master_clk(fpc1020->spi);
	return 0;
}

static int fpc1020_resume(struct device *dev)
{
	struct  fpc1020_data *fpc1020 = dev_get_drvdata(dev);
	mt_spi_enable_master_clk(fpc1020->spi);
	return 0;
}

static const struct dev_pm_ops fpc1020_pm_ops = {
	.suspend = fpc1020_suspend,
	.resume = fpc1020_resume,
};

MODULE_DEVICE_TABLE(of, fpc1020_of_match);

static struct spi_driver fpc1020_driver = {
	.driver = {
		.name	= "fpc1020",
		.owner	= THIS_MODULE,
		.of_match_table = fpc1020_of_match,
		.pm = &fpc1020_pm_ops,
	},
	.probe		= fpc1020_probe,
	.remove		= fpc1020_remove,
};

static int __init fpc1020_init(void)
{
	int rc = spi_register_driver(&fpc1020_driver);
	if (!rc)
		pr_info("%s OK\n", __func__);
	else
		pr_err("%s %d\n", __func__, rc);
	return rc;
}

static void __exit fpc1020_exit(void)
{
	pr_info("%s\n", __func__);
	spi_unregister_driver(&fpc1020_driver);
}

module_init(fpc1020_init);
module_exit(fpc1020_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aleksej Makarov");
MODULE_AUTHOR("Henrik Tillman <henrik.tillman@fingerprints.com>");
MODULE_DESCRIPTION("FPC1020 Fingerprint sensor device driver.");
