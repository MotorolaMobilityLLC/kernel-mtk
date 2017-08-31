/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>

#include "flashlight.h"
#include "flashlight-dt.h"

/* define device tree */
/* TODO: modify temp device tree name */
#ifndef DUMMY_DTNAME_I2C
#define DUMMY_DTNAME_I2C "mediatek,flashlights_dummy_i2c"
#endif

/* TODO: define driver name */
#define DUMMY_NAME "flashlights-dummy"

/* define registers */
/* TODO: define register */

/* define mutex and work queue */
static DEFINE_MUTEX(dummy_mutex);
static struct work_struct dummy_work;

/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *dummy_i2c_client;

/* platform data */
struct dummy_platform_data {
	u8 torch_pin_enable;         /* 1: TX1/TORCH pin isa hardware TORCH enable */
	u8 pam_sync_pin_enable;      /* 1: TX2 Mode The ENVM/TX2 is a PAM Sync. on input */
	u8 thermal_comp_mode_enable; /* 1: LEDI/NTC pin in Thermal Comparator Mode */
	u8 strobe_pin_disable;       /* 1: STROBE Input disabled */
	u8 vout_mode_enable;         /* 1: Voltage Out Mode enable */
};

/* dummy chip data */
struct dummy_chip_data {
	struct i2c_client *client;
	struct dummy_platform_data *pdata;
	struct mutex lock;
	u8 last_flag;
	u8 no_pdata;
};


/******************************************************************************
 * dummy operations
 *****************************************************************************/

/* i2c wrapper function */
static int dummy_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	struct dummy_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);

	if (ret < 0)
		fl_err("failed writing at 0x%02x\n", reg);

	return ret;
}

/* flashlight enable function */
static int dummy_enable(void)
{
	unsigned char reg = 0, val = 0;

	/* TODO: wrap enable function */

	return dummy_write_reg(dummy_i2c_client, reg, val);
}

/* flashlight disable function */
static int dummy_disable(void)
{
	unsigned char reg = 0, val = 0;

	/* TODO: wrap disable function */

	return dummy_write_reg(dummy_i2c_client, reg, val);
}

/* set flashlight level */
static int dummy_set_level(int level)
{
	unsigned char reg = 0, val = 0;

	/* TODO: wrap set level function */

	return dummy_write_reg(dummy_i2c_client, reg, val);
}

/* flashlight init */
static int dummy_init(void)
{
	unsigned char reg = 0, val = 0;

	/* TODO: wrap init function */

	return dummy_write_reg(dummy_i2c_client, reg, val);
}

/* flashlight uninit */
static int dummy_uninit(void)
{
	unsigned char reg = 0, val = 0;

	/* TODO: wrap uninit function */

	return dummy_write_reg(dummy_i2c_client, reg, val);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer dummy_timer;
static unsigned int dummy_timeout_ms;

static void dummy_work_disable(struct work_struct *data)
{
	fl_dbg("work queue callback\n");
	dummy_disable();
}

static enum hrtimer_restart dummy_timer_func(struct hrtimer *timer)
{
	schedule_work(&dummy_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int dummy_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		fl_dbg("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		dummy_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		fl_dbg("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		dummy_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		fl_dbg("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (dummy_timeout_ms) {
				ktime = ktime_set(dummy_timeout_ms / 1000,
						(dummy_timeout_ms % 1000) * 1000000);
				hrtimer_start(&dummy_timer, ktime, HRTIMER_MODE_REL);
			}
			dummy_enable();
		} else {
			dummy_disable();
			hrtimer_cancel(&dummy_timer);
		}
		break;
	default:
		fl_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int dummy_open(void *pArg)
{
	/* Actual behavior move to set driver function since power saving issue */
	return 0;
}

static int dummy_release(void *pArg)
{
	/* uninit chip and clear usage count */
	mutex_lock(&dummy_mutex);
	use_count--;
	if (!use_count)
		dummy_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&dummy_mutex);

	fl_dbg("Release: %d\n", use_count);

	return 0;
}

static int dummy_set_driver(void)
{
	/* init chip and set usage count */
	mutex_lock(&dummy_mutex);
	if (!use_count)
		dummy_init();
	use_count++;
	mutex_unlock(&dummy_mutex);

	fl_dbg("Set driver: %d\n", use_count);

	return 0;
}

static ssize_t dummy_strobe_store(struct flashlight_arg arg)
{
	dummy_set_driver();
	dummy_set_level(arg.level);
	dummy_enable();
	msleep(arg.dur);
	dummy_disable();
	dummy_release(NULL);

	return 0;
}

static struct flashlight_operations dummy_ops = {
	dummy_open,
	dummy_release,
	dummy_ioctl,
	dummy_strobe_store,
	dummy_set_driver
};


/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int dummy_chip_init(struct dummy_chip_data *chip)
{
	/* NOTE: Chip initialication move to "set driver" operation for power saving issue.
	 * dummy_init();
	 */

	return 0;
}

static int dummy_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct dummy_chip_data *chip;
	struct dummy_platform_data *pdata = client->dev.platform_data;
	int err;

	fl_dbg("Probe start.\n");

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		fl_err("Failed to check i2c functionality.\n");
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct dummy_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	/* init platform data */
	if (!pdata) {
		fl_dbg("Platform data does not exist\n");
		pdata = kzalloc(sizeof(struct dummy_platform_data), GFP_KERNEL);
		if (!pdata) {
			fl_err("Failed to allocate memory.\n");
			err = -ENOMEM;
			goto err_init_pdata;
		}
		chip->no_pdata = 1;
	}
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);
	dummy_i2c_client = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* init work queue */
	INIT_WORK(&dummy_work, dummy_work_disable);

	/* init timer */
	hrtimer_init(&dummy_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dummy_timer.function = dummy_timer_func;
	dummy_timeout_ms = 100;

	/* init chip hw */
	dummy_chip_init(chip);

	/* register flashlight operations */
	if (flashlight_dev_register(DUMMY_NAME, &dummy_ops)) {
		err = -EFAULT;
		goto err_free;
	}

	/* clear usage count */
	use_count = 0;

	fl_dbg("Probe done.\n");

	return 0;

err_free:
	kfree(chip->pdata);
err_init_pdata:
	i2c_set_clientdata(client, NULL);
	kfree(chip);
err_out:
	return err;
}

static int dummy_i2c_remove(struct i2c_client *client)
{
	struct dummy_chip_data *chip = i2c_get_clientdata(client);

	fl_dbg("Remove start.\n");

	/* flush work queue */
	flush_work(&dummy_work);

	/* unregister flashlight operations */
	flashlight_dev_unregister(DUMMY_NAME);

	/* free resource */
	if (chip->no_pdata)
		kfree(chip->pdata);
	kfree(chip);

	fl_dbg("Remove done.\n");

	return 0;
}

static const struct i2c_device_id dummy_i2c_id[] = {
	{DUMMY_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, dummy_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id dummy_i2c_of_match[] = {
	{.compatible = DUMMY_DTNAME_I2C},
	{},
};
MODULE_DEVICE_TABLE(of, dummy_i2c_of_match);
#endif

static struct i2c_driver dummy_i2c_driver = {
	.driver = {
		   .name = DUMMY_NAME,
#ifdef CONFIG_OF
		   .of_match_table = dummy_i2c_of_match,
#endif
		   },
	.probe = dummy_i2c_probe,
	.remove = dummy_i2c_remove,
	.id_table = dummy_i2c_id,
};

module_i2c_driver(dummy_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight DUMMY Driver");

