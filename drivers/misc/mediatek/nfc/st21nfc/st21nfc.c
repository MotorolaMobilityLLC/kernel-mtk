/*
 * Copyright (C) 2013 ST Microelectronics S.A.
 * Copyright (C) 2010 Stollmann E+V GmbH
 * Copyright (C) 2010 Trusted Logic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include "st21nfc.h"
#include <linux/of_gpio.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#define MAX_BUFFER_SIZE	256

#define DRIVER_VERSION "1.0.9"

/* define the active state of the WAKEUP pin */
#define ST21_IRQ_ACTIVE_HIGH 1
#define ST21_IRQ_ACTIVE_LOW 0

#define I2C_ID_NAME "st21nfc"

/* prototypes */
static irqreturn_t st21nfc_dev_irq_handler(int irq, void *dev_id);
/*
 * The platform data member 'polarity_mode' defines
 * how the wakeup pin is configured and handled.
 * it can take the following values :
 *	 IRQF_TRIGGER_RISING
 *   IRQF_TRIGGER_FALLING
 *   IRQF_TRIGGER_HIGH
 *   IRQF_TRIGGER_LOW
 */

struct st21nfc_platform {
	struct mutex read_mutex;
	struct i2c_client *client;
	int irq_gpio;
	int reset_gpio;
	int ena_gpio;
	int polarity_mode;
	int active_polarity; /* either 0 (low-active) or 1 (high-active)  */
};

/*  NFC IRQ */
static u32 nfc_irq;
static bool irqIsAttached;

static bool device_open; /* Is device open? */
static bool enable_debug_log;

struct st21nfc_dev {
	wait_queue_head_t read_wq;
	struct miscdevice st21nfc_device;
	bool irq_enabled;
	struct st21nfc_platform platform_data;
	spinlock_t irq_enabled_lock;
};

static int st21nfc_loc_set_polaritymode(struct st21nfc_dev *st21nfc_dev,
					int mode)
{

	struct i2c_client *client = st21nfc_dev->platform_data.client;
	unsigned int irq_type;
	int ret;

	if (enable_debug_log)
		pr_info("%s:%d mode %d", __FILE__, __LINE__, mode);

	st21nfc_dev->platform_data.polarity_mode = mode;
	/* setup irq_flags */
	switch (mode) {
	case IRQF_TRIGGER_RISING:
		irq_type = IRQ_TYPE_EDGE_RISING;
		st21nfc_dev->platform_data.active_polarity = 1;
		break;
	case IRQF_TRIGGER_FALLING:
		irq_type = IRQ_TYPE_EDGE_FALLING;
		st21nfc_dev->platform_data.active_polarity = 0;
		break;
	case IRQF_TRIGGER_HIGH:
		irq_type = IRQ_TYPE_LEVEL_HIGH;
		st21nfc_dev->platform_data.active_polarity = 1;
		break;
	case IRQF_TRIGGER_LOW:
		irq_type = IRQ_TYPE_LEVEL_LOW;
		st21nfc_dev->platform_data.active_polarity = 0;
		break;
	default:
		irq_type = IRQF_TRIGGER_FALLING;
		st21nfc_dev->platform_data.active_polarity = 0;
		break;
	}
	if (irqIsAttached) {
		free_irq(client->irq, st21nfc_dev);
		irqIsAttached = false;
	}
	ret = irq_set_irq_type(client->irq, irq_type);
	if (ret) {
		pr_err("%s : set_irq_type failed!!!!!!!\n", __FILE__);
		return -ENODEV;
	}

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	if (enable_debug_log)
		pr_debug("%s : requesting IRQ %d\n", __func__, client->irq);

	st21nfc_dev->irq_enabled = true;

	ret = request_irq(client->irq, st21nfc_dev_irq_handler,
			  st21nfc_dev->platform_data.polarity_mode,
			  client->name, st21nfc_dev);

	if (!ret)
		irqIsAttached = true;

	if (enable_debug_log)
		pr_info("%s:%d ret %d", __FILE__, __LINE__, ret);
	return ret;
}

static void st21nfc_disable_irq(struct st21nfc_dev *st21nfc_dev)
{
	unsigned long flags;

	spin_lock_irqsave(&st21nfc_dev->irq_enabled_lock, flags);
	if (st21nfc_dev->irq_enabled) {
		disable_irq_nosync(st21nfc_dev->platform_data.client->irq);
		st21nfc_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&st21nfc_dev->irq_enabled_lock, flags);
}

static irqreturn_t st21nfc_dev_irq_handler(int irq, void *dev_id)
{
	struct st21nfc_dev *st21nfc_dev = dev_id;

	st21nfc_disable_irq(st21nfc_dev);

	/* Wake up waiting readers */
	wake_up(&st21nfc_dev->read_wq);

	return IRQ_HANDLED;
}

static ssize_t st21nfc_dev_read(struct file *filp, char __user *buf,
				size_t count, loff_t *offset)
{
	struct st21nfc_dev *st21nfc_dev = container_of(filp->private_data,
						       struct st21nfc_dev,
						       st21nfc_device);
	char tmp[MAX_BUFFER_SIZE];
	int ret;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (enable_debug_log)
		pr_debug("%s : reading %zu bytes.\n", __func__, count);

	mutex_lock(&st21nfc_dev->platform_data.read_mutex);

	/* Read data */
	ret = i2c_master_recv(st21nfc_dev->platform_data.client, tmp, count);
	mutex_unlock(&st21nfc_dev->platform_data.read_mutex);

	if (ret < 0) {
		pr_err("%s: i2c_master_recv returned %d\n", __func__, ret);
		return ret;
	}
	if (ret > count) {
		pr_err("%s: received too many bytes from i2c (%d)\n",
		       __func__, ret);
		return -EIO;
	}
	if (copy_to_user(buf, tmp, ret)) {
		pr_warn("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}
	return ret;
}

static ssize_t st21nfc_dev_write(struct file *filp, const char __user *buf,
				 size_t count, loff_t *offset)
{
	struct st21nfc_dev *st21nfc_dev;
	char tmp[MAX_BUFFER_SIZE];
	int ret = count;

	st21nfc_dev = container_of(filp->private_data,
				   struct st21nfc_dev, st21nfc_device);
	if (enable_debug_log) {
		pr_debug("%s: st21nfc_dev ptr %p\n", __func__, st21nfc_dev);
		pr_debug("%s : writing %zu bytes.\n", __func__, count);
	}

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(tmp, buf, count)) {
		pr_err("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	/* Write data */
	ret = i2c_master_send(st21nfc_dev->platform_data.client, tmp, count);
	if (ret != count) {
		pr_err("%s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}
	return ret;
}

static int st21nfc_dev_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct st21nfc_dev *st21nfc_dev = NULL;

	if (enable_debug_log)
		pr_info("%s:%d dev_open", __FILE__, __LINE__);

	if (device_open) {
		ret = -EBUSY;
		if (enable_debug_log)
			pr_debug("%s : device already opened ret= %d\n", __func__, ret);
	} else {
		device_open = true;
		st21nfc_dev = container_of(filp->private_data,
								   struct st21nfc_dev,
								   st21nfc_device);

		if (enable_debug_log) {
			pr_debug("%s : %d,%d ", __func__, imajor(inode), iminor(inode));
			pr_debug("%s: st21nfc_dev ptr %p\n", __func__, st21nfc_dev);
		}
	}
	return ret;
}


static int st21nfc_release(struct inode *inode, struct file *file)
{
	device_open = false;
	if (enable_debug_log)
		pr_debug("%s : device_open  = %d\n", __func__, device_open);

	return 0;
}

static long st21nfc_dev_ioctl(struct file *filp, unsigned int cmd,
			      unsigned long arg)
{
	struct st21nfc_dev *st21nfc_dev = container_of(filp->private_data,
						       struct st21nfc_dev,
						       st21nfc_device);

	int ret = 0;

	switch (cmd) {

	case ST21NFC_SET_POLARITY_FALLING:
		pr_info(" ### ST21NFC_SET_POLARITY_FALLING ###");
		st21nfc_loc_set_polaritymode(st21nfc_dev, IRQF_TRIGGER_FALLING);
		break;

	case ST21NFC_SET_POLARITY_RISING:
		pr_info(" ### ST21NFC_SET_POLARITY_RISING ###\n");
		st21nfc_loc_set_polaritymode(st21nfc_dev, IRQF_TRIGGER_RISING);
		break;

	case ST21NFC_SET_POLARITY_LOW:
		pr_info(" ### ST21NFC_SET_POLARITY_LOW ###");
		st21nfc_loc_set_polaritymode(st21nfc_dev, IRQF_TRIGGER_LOW);
		break;

	case ST21NFC_SET_POLARITY_HIGH:
		pr_info(" ### ST21NFC_SET_POLARITY_HIGH ###");
		st21nfc_loc_set_polaritymode(st21nfc_dev, IRQF_TRIGGER_HIGH);
		break;

	case ST21NFC_PULSE_RESET:
		pr_info("%s Double Pulse Request\n", __func__);
		if (st21nfc_dev->platform_data.reset_gpio != 0) {
			/* pulse low for 20 millisecs */
			pr_info("Pulse Request gpio is %d\n", st21nfc_dev->platform_data.reset_gpio);
			gpio_set_value(st21nfc_dev->platform_data.reset_gpio,
				       0);
			msleep(20);
			gpio_set_value(st21nfc_dev->platform_data.reset_gpio,
				       1);
			msleep(20);
			/* pulse low for 20 millisecs */
			gpio_set_value(st21nfc_dev->platform_data.reset_gpio,
					0);
			msleep(20);
			gpio_set_value(st21nfc_dev->platform_data.reset_gpio,
					1);
			pr_info("%s done Double Pulse Request\n", __func__);
		}
		break;

	case ST21NFC_GET_WAKEUP:
		/* deliver state of Wake_up_pin as return value of ioctl */
		ret = gpio_get_value(st21nfc_dev->platform_data.irq_gpio);
		/*
		 * ret shall be equal to 1 if gpio level equals to polarity.
		 * Warning: depending on gpio_get_value implementation,
		 * it can returns a value different than 1 in case of high level
		 */
		if (((ret == 0)
		     && (st21nfc_dev->platform_data.active_polarity == 0))
		    || ((ret > 0)
			&& (st21nfc_dev->platform_data.active_polarity == 1))) {
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case ST21NFC_GET_POLARITY:
		ret = st21nfc_dev->platform_data.polarity_mode;
		if (enable_debug_log)
			pr_debug("%s get polarity %d\n", __func__, ret);
		break;
	case ST21NFC_RECOVERY:
		/* For ST21NFCD usage only */
		pr_info("%s Recovery Request\n", __func__);
		if (st21nfc_dev->platform_data.reset_gpio != 0) {
			/* pulse low for 20 millisecs */
			gpio_set_value(st21nfc_dev->platform_data.reset_gpio, 0);
			msleep(20);
			/* during the reset, force IRQ OUT as DH output instead of input in normal usage */
			ret = gpio_direction_output(st21nfc_dev->platform_data.irq_gpio, 1);
			if (ret) {
				pr_err("%s : gpio_direction_output failed\n", __func__);
				ret = -ENODEV;
				break;
			}
			gpio_set_value(st21nfc_dev->platform_data.irq_gpio, 1);
			msleep(20);
			gpio_set_value(st21nfc_dev->platform_data.reset_gpio, 1);
			pr_info("%s done Pulse Request\n", __func__);
		}
		msleep(20);
		gpio_set_value(st21nfc_dev->platform_data.irq_gpio, 0);
		msleep(20);
		gpio_set_value(st21nfc_dev->platform_data.irq_gpio, 1);
		msleep(20);
		gpio_set_value(st21nfc_dev->platform_data.irq_gpio, 0);
		msleep(20);
		pr_info("%s Recovery procedure finished\n", __func__);
		ret = gpio_direction_input(st21nfc_dev->platform_data.irq_gpio);
		if (ret) {
			pr_err("%s : gpio_direction_input failed\n", __func__);
			ret = -ENODEV;
		}
		break;
	default:
		pr_err("%s bad ioctl %u\n", __func__, cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static unsigned int st21nfc_poll(struct file *file, poll_table *wait)
{
	struct st21nfc_dev *st21nfc_dev = container_of(file->private_data,
						       struct st21nfc_dev,
						       st21nfc_device);
	unsigned int mask = 0;
	int pinlev = 0;

	/* wait for Wake_up_pin == high  */
	poll_wait(file, &st21nfc_dev->read_wq, wait);

	pinlev = gpio_get_value(st21nfc_dev->platform_data.irq_gpio);

	if (((pinlev == 0) && (st21nfc_dev->platform_data.active_polarity == 0))
	    || ((pinlev > 0)
		&& (st21nfc_dev->platform_data.active_polarity == 1))) {

		mask = POLLIN | POLLRDNORM;	/* signal data avail */
		st21nfc_disable_irq(st21nfc_dev);
	} else {
		/* Wake_up_pin  is low. Activate ISR  */
		if (!st21nfc_dev->irq_enabled) {
			if (enable_debug_log)
				pr_debug("%s enable irq\n", __func__);
			st21nfc_dev->irq_enabled = true;
			enable_irq(st21nfc_dev->platform_data.client->irq);
		} else {
			if (enable_debug_log)
				pr_debug("%s irq already enabled\n", __func__);
		}

	}

	return mask;
}

static int st21nfc_platform_probe(struct platform_device *pdev)
{
	if (enable_debug_log)
		pr_debug("st21nfc_platform_probe\n");
	return 0;
}

static int st21nfc_platform_remove(struct platform_device *pdev)
{
	if (enable_debug_log)
		pr_debug("st21nfc_platform_remove\n");
	return 0;
}

static const struct file_operations st21nfc_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = st21nfc_dev_read,
	.write = st21nfc_dev_write,
	.open = st21nfc_dev_open,
	.poll = st21nfc_poll,
	.release = st21nfc_release,

	.unlocked_ioctl = st21nfc_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = st21nfc_dev_ioctl
#endif
};

static ssize_t st21nfc_show_i2c_addr(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);

	if (client != NULL)
		return sprintf(buf, "0x%.2x\n", client->addr);
	return 0;
}				/* st21nfc_show_i2c_addr() */

static ssize_t st21nfc_change_i2c_addr(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{

	struct st21nfc_dev *data = dev_get_drvdata(dev);
	long new_addr = 0;

	if (data != NULL && data->platform_data.client != NULL) {
		if (!kstrtol(buf, 10, &new_addr)) {
			mutex_lock(&data->platform_data.read_mutex);
			data->platform_data.client->addr = new_addr;
			mutex_unlock(&data->platform_data.read_mutex);
			return count;
		}
		return -EINVAL;
	}
	return 0;
}				/* st21nfc_change_i2c_addr() */

static ssize_t st21nfc_version(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", DRIVER_VERSION);
}				/* st21nfc_version */

static DEVICE_ATTR(i2c_addr, S_IRUGO | S_IWUSR, st21nfc_show_i2c_addr,
		   st21nfc_change_i2c_addr);

static DEVICE_ATTR(version, S_IRUGO, st21nfc_version, NULL);

static struct attribute *st21nfc_attrs[] = {
	&dev_attr_i2c_addr.attr,
	&dev_attr_version.attr,
	NULL,
};

static struct attribute_group st21nfc_attr_grp = {
	.attrs = st21nfc_attrs,
};

#ifdef CONFIG_OF
static int nfc_parse_dt(struct device *dev, struct st21nfc_platform_data *pdata)
{
	int r = 0;
	struct device_node *np = dev->of_node;

	np = of_find_compatible_node(NULL, NULL, "mediatek,nfc-gpio-v2");

	if (np) {
#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
	r = of_get_named_gpio(np, "gpio-rst-std", 0);
	if (r < 0)
		pr_err("%s: get NFC RST GPIO failed (%d)", __FILE__, r);
	else
		pdata->reset_gpio = r;

	r = of_get_named_gpio(np, "gpio-irq-std", 0);
	if (r < 0)
		pr_err("%s: get NFC IRQ GPIO failed (%d)", __FILE__, r);
	else
		pdata->irq_gpio = r;
	r = 0;
#else
	of_property_read_u32_array(np, "gpio-rst",
		&(pdata->reset_gpio),
		1);

	of_property_read_u32_array(np, "gpio-irq",
		&(pdata->irq_gpio), 1);
#endif
	} else {
		if (enable_debug_log)
			pr_debug("%s : get gpio num err.\n", __func__);
		return -1;
	}

	pdata->polarity_mode = 0;
	pr_info("[dsc]%s : get reset_gpio[%d], irq_gpio[%d], polarity_mode[%d]\n",
	       __func__, pdata->reset_gpio, pdata->irq_gpio, pdata->polarity_mode);
	return r;
}
#else
static int nfc_parse_dt(struct device *dev, struct st21nfc_platform_data *pdata)
{
	return 0;
}
#endif


static int st21nfc_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret;
	struct st21nfc_platform_data *platform_data;
	struct st21nfc_dev *st21nfc_dev;
	struct device_node *node;
	struct gpio_desc *desc;

	if (client->dev.of_node) {
		platform_data = devm_kzalloc(&client->dev,
			sizeof(struct st21nfc_platform_data), GFP_KERNEL);
		if (!platform_data)
			return -ENOMEM;

		pr_info("%s : Parse st21nfc DTS\n", __func__);
		ret = nfc_parse_dt(&client->dev, platform_data);
		if (ret) {
			pr_err("%s : ret =%d\n", __func__, ret);
			return ret;
		}
		pr_info("%s : Parsed st21nfc DTS %d %d\n",
			__func__, platform_data->reset_gpio, platform_data->irq_gpio);
	} else {
		platform_data = client->dev.platform_data;
		pr_err("%s : No st21nfc DTS\n", __func__);
	}
	if (!platform_data)
		return -EINVAL;

	dev_dbg(&client->dev, "nfc-nci probe: %s, inside nfc-nci flags = %x\n",
	__func__, client->flags);

	if (platform_data == NULL) {
		dev_err(&client->dev, "nfc-nci probe: failed\n");
		return -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return -ENODEV;
	}
	pr_info("%s : after i2c_check_functionality %d %d\n",
		__func__, platform_data->reset_gpio, platform_data->irq_gpio);

	client->adapter->timeout = msecs_to_jiffies(3 * 10);	/* 30ms */
	client->adapter->retries = 0;

	st21nfc_dev = kzalloc(sizeof(*st21nfc_dev), GFP_KERNEL);
	if (st21nfc_dev == NULL) {
		ret = -ENOMEM;
		goto err_exit;
	}

	if (enable_debug_log)
		pr_debug("%s : dev_cb_addr %p\n", __func__, st21nfc_dev);
	pr_info("%s : dev_cb_addr %p\n", __func__, st21nfc_dev);

	/* store for later use */
	st21nfc_dev->platform_data.irq_gpio = platform_data->irq_gpio;
	st21nfc_dev->platform_data.ena_gpio = platform_data->ena_gpio;
	st21nfc_dev->platform_data.reset_gpio = platform_data->reset_gpio;
	st21nfc_dev->platform_data.polarity_mode = platform_data->polarity_mode;
	st21nfc_dev->platform_data.client = client;

	pr_err("%s gpio_request, ret is %d %d %d %d // %d %d %d %d\n", __func__,
		st21nfc_dev->platform_data.irq_gpio,
		st21nfc_dev->platform_data.ena_gpio,
		st21nfc_dev->platform_data.reset_gpio,
		st21nfc_dev->platform_data.polarity_mode,

		platform_data->irq_gpio,
		platform_data->ena_gpio,
		platform_data->reset_gpio,
		platform_data->polarity_mode);

	desc = gpio_to_desc(platform_data->irq_gpio);
	if (!desc)
		pr_err("gpio_desc is null\n");
	else
		pr_err("gpio_desc isn't null\n");

	if (gpio_is_valid(platform_data->irq_gpio))
		pr_err("gpio number %d is valid\n", platform_data->irq_gpio);

	if (gpio_is_valid(platform_data->reset_gpio))
		pr_err("gpio number %d is valid\n", platform_data->reset_gpio);
	ret = gpio_request(platform_data->irq_gpio,
#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
		"gpio-irq-std"
#else
		"gpio-irq"
#endif
	);
	if (ret) {
		pr_err("%s : gpio_request failed\n", __FILE__);
		goto err_free_buffer;
	}
	pr_info("%s : IRQ GPIO = %d\n", __func__, platform_data->irq_gpio);
	ret = gpio_direction_input(platform_data->irq_gpio);
	if (ret) {
		pr_err("%s : gpio_direction_input failed\n", __FILE__);
		ret = -ENODEV;
		goto err_free_buffer;
	}

	st21nfc_dev->platform_data.client->irq = platform_data->irq_gpio;

	/* initialize irqIsAttached variable */
	irqIsAttached = false;

	/* initialize device_open variable */
	device_open = 0;

	/* handle optional RESET */
	if (platform_data->reset_gpio != 0) {
		ret = gpio_request(platform_data->reset_gpio,
#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
		"gpio-rst-std"
#else
		"gpio-rst"
#endif
		);
		if (ret) {
			pr_err("%s : reset gpio_request failed\n", __FILE__);
			ret = -ENODEV;
			goto err_free_buffer;
		}
		pr_info("%s : RST GPIO = %d\n", __func__, platform_data->reset_gpio);
		ret = gpio_direction_output(platform_data->reset_gpio, 1);
		if (ret) {
			pr_err("%s : reset gpio_direction_output failed\n",
			       __FILE__);
			ret = -ENODEV;
			goto err_free_buffer;
		}
		/* low active */
		gpio_set_value(st21nfc_dev->platform_data.reset_gpio, 1);
	}

	/* set up optional ENA gpio */
	if (platform_data->ena_gpio != 0) {
		ret = gpio_request(platform_data->ena_gpio, "st21nfc_ena");
		if (ret) {
			pr_err("%s : ena gpio_request failed\n", __FILE__);
			ret = -ENODEV;
			goto err_free_buffer;
		}
		ret = gpio_direction_output(platform_data->ena_gpio, 1);
		if (ret) {
			pr_err("%s : ena gpio_direction_output failed\n",
			       __FILE__);
			ret = -ENODEV;
			goto err_free_buffer;
		}
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,irq_nfc-eint");

	if (node) {

		nfc_irq = irq_of_parse_and_map(node, 0);

		client->irq = nfc_irq;

		pr_info("%s : MT IRQ GPIO = %d\n", __func__, client->irq);

		enable_irq_wake(client->irq);

	} else {
		pr_err("%s : can not find NFC eint compatible node\n",
		       __func__);
	}
	/* init mutex and queues */
	init_waitqueue_head(&st21nfc_dev->read_wq);
	mutex_init(&st21nfc_dev->platform_data.read_mutex);
	spin_lock_init(&st21nfc_dev->irq_enabled_lock);

	st21nfc_dev->st21nfc_device.minor = MISC_DYNAMIC_MINOR;
	st21nfc_dev->st21nfc_device.name = I2C_ID_NAME;
	st21nfc_dev->st21nfc_device.fops = &st21nfc_dev_fops;
	st21nfc_dev->st21nfc_device.parent = &client->dev;

	i2c_set_clientdata(client, st21nfc_dev);
	ret = misc_register(&st21nfc_dev->st21nfc_device);
	pr_info("ret of misc_register:%d\n", ret);
	if (ret) {
		pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	if (sysfs_create_group(&client->dev.kobj, &st21nfc_attr_grp)) {
		pr_err("%s : sysfs_create_group failed\n", __FILE__);
		goto err_request_irq_failed;
	}
	st21nfc_disable_irq(st21nfc_dev);
	return 0;

err_request_irq_failed:
	misc_deregister(&st21nfc_dev->st21nfc_device);
err_misc_register:
	mutex_destroy(&st21nfc_dev->platform_data.read_mutex);
err_free_buffer:
	kfree(st21nfc_dev);
err_exit:
	gpio_free(platform_data->irq_gpio);
	if (platform_data->ena_gpio != 0)
		gpio_free(platform_data->ena_gpio);
	return ret;
}

static int st21nfc_remove(struct i2c_client *client)
{
	struct st21nfc_dev *st21nfc_dev;

	st21nfc_dev = i2c_get_clientdata(client);
	free_irq(client->irq, st21nfc_dev);
	misc_deregister(&st21nfc_dev->st21nfc_device);
	mutex_destroy(&st21nfc_dev->platform_data.read_mutex);
	gpio_free(st21nfc_dev->platform_data.irq_gpio);
	if (st21nfc_dev->platform_data.ena_gpio != 0)
		gpio_free(st21nfc_dev->platform_data.ena_gpio);
	kfree(st21nfc_dev);

	return 0;
}

static const struct i2c_device_id st21nfc_id[] = {
	{"st21nfc", 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id nfc_switch_of_match[] = {
	{.compatible = "mediatek,nfc"},
	{},
};
#endif

static struct i2c_driver st21nfc_driver = {
	.id_table = st21nfc_id,
	.probe = st21nfc_probe,
	.remove = st21nfc_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = I2C_ID_NAME,
#ifdef CONFIG_OF
		   .of_match_table = nfc_switch_of_match,
#endif
		   },
};

/*  platform driver */
static const struct of_device_id nfc_dev_of_match[] = {
	{.compatible = "mediatek,nfc-gpio-v2",},
	{},
};


static struct platform_driver st21nfc_platform_driver = {
	.probe = st21nfc_platform_probe,
	.remove = st21nfc_platform_remove,
	.driver = {
		   .name = I2C_ID_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = nfc_dev_of_match,
#endif
		   },
};

/*
 * module load/unload record keeping
 */

static int __init st21nfc_dev_init(void)
{
	if (enable_debug_log)
		pr_debug("Loading st21nfc driver\n");
	platform_driver_register(&st21nfc_platform_driver);
	if (enable_debug_log)
		pr_debug("Loading st21nfc platform driver\n");

	return i2c_add_driver(&st21nfc_driver);
}

module_init(st21nfc_dev_init);

static void __exit st21nfc_dev_exit(void)
{
	if (enable_debug_log)
		pr_debug("Unloading st21nfc driver\n");
	i2c_del_driver(&st21nfc_driver);
}

module_exit(st21nfc_dev_exit);

MODULE_AUTHOR("Norbert Kawulski");
MODULE_DESCRIPTION("NFC ST21NFC driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
