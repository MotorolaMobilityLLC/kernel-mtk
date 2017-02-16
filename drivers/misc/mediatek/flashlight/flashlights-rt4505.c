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
#ifndef RT4505_DTNAME_I2C
#define RT4505_DTNAME_I2C "mediatek,flashlights_rt4505_i2c"
#endif

#define RT4505_NAME "flashlights-rt4505"

/* define registers */
#define RT4505_REG_ENABLE   (0x0A)
#define RT4505_ENABLE_TORCH (0x71)
#define RT4505_ENABLE_FLASH (0x77)
#define RT4505_DISABLE      (0x70)

#define RT4505_REG_LEVEL (0x09)

#define RT4505_REG_RESET (0x00)
#define RT4505_FLASH_RESET (0x80)

#define RT4505_REG_FLASH_FEATURE (0x08)
#define RT4505_FLASH_TIMEOUT (0x07)

/* define level */
#define RT4505_LEVEL_NUM 18
#define RT4505_LEVEL_TORCH 4

/* define mutex and work queue */
static DEFINE_MUTEX(rt4505_mutex);
static struct work_struct rt4505_work;

/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *rt4505_i2c_client;

/* platform data */
struct rt4505_platform_data {
	u8 torch_pin_enable;         /* 1: TX1/TORCH pin isa hardware TORCH enable */
	u8 pam_sync_pin_enable;      /* 1: TX2 Mode The ENVM/TX2 is a PAM Sync. on input */
	u8 thermal_comp_mode_enable; /* 1: LEDI/NTC pin in Thermal Comparator Mode */
	u8 strobe_pin_disable;       /* 1: STROBE Input disabled */
	u8 vout_mode_enable;         /* 1: Voltage Out Mode enable */
};

/* rt4505 chip data */
struct rt4505_chip_data {
	struct i2c_client *client;
	struct rt4505_platform_data *pdata;
	struct mutex lock;
	u8 last_flag;
	u8 no_pdata;
};


/******************************************************************************
 * rt4505 operations
 *****************************************************************************/

/*
 * flashlight current (mA)
 * {49.6 , 93.74, 140.63 , 187.5, 281.25 , 375   , 468.75 , 562.5, 656.25, 750,
 * 843.75, 937.5, 1031.25, 1125 , 1218.75, 1312.5, 1406.25, 1500}
 */
static const unsigned char rt4505_flash_level[RT4505_LEVEL_NUM] = {
	0x00, 0x20, 0x40, 0x60, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

static int rt4505_level = -1;

static int rt4505_is_torch(int level)
{
	if (level >= RT4505_LEVEL_TORCH)
		return -1;

	return 0;
}

static int rt4505_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= RT4505_LEVEL_NUM)
		level = RT4505_LEVEL_NUM - 1;

	return level;
}

/* i2c wrapper function */
static int rt4505_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	struct rt4505_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);

	if (ret < 0)
		fl_err("failed writing at 0x%02x\n", reg);

	return ret;
}

/* flashlight enable function */
static int rt4505_enable(void)
{
	unsigned char reg, val;

	reg = RT4505_REG_ENABLE;
	if (!rt4505_is_torch(rt4505_level)) {
		/* torch mode */
		val = RT4505_ENABLE_TORCH;
	} else {
		/* flash mode */
		val = RT4505_ENABLE_FLASH;
	}

	return rt4505_write_reg(rt4505_i2c_client, reg, val);
}

/* flashlight disable function */
static int rt4505_disable(void)
{
	unsigned char reg, val;

	reg = RT4505_REG_ENABLE;
	val = RT4505_DISABLE;

	return rt4505_write_reg(rt4505_i2c_client, reg, val);
}

/* set flashlight level */
static int rt4505_set_level(int level)
{
	unsigned char reg, val;

	level = rt4505_verify_level(level);
	rt4505_level = level;

	reg = RT4505_REG_LEVEL;
	val = rt4505_flash_level[level];

	return rt4505_write_reg(rt4505_i2c_client, reg, val);
}

/* flashlight init */
int rt4505_init(void)
{
	int ret;
	unsigned char reg, val;

	/* reset chip */
	reg = RT4505_REG_RESET;
	val = RT4505_FLASH_RESET;
	ret = rt4505_write_reg(rt4505_i2c_client, reg, val);

	/* set flash timeout */
	reg = RT4505_REG_FLASH_FEATURE;
	val = RT4505_FLASH_TIMEOUT;
	ret = rt4505_write_reg(rt4505_i2c_client, reg, val);

	return ret;
}

/* flashlight uninit */
int rt4505_uninit(void)
{
	rt4505_disable();

	return 0;
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer rt4505_timer;
static unsigned int rt4505_timeout_ms;

static void rt4505_work_disable(struct work_struct *data)
{
	fl_dbg("work queue callback\n");
	rt4505_disable();
}

static enum hrtimer_restart rt4505_timer_func(struct hrtimer *timer)
{
	schedule_work(&rt4505_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int rt4505_ioctl(unsigned int cmd, unsigned long arg)
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
		rt4505_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		fl_dbg("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		rt4505_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		fl_dbg("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (rt4505_timeout_ms) {
				ktime = ktime_set(rt4505_timeout_ms / 1000,
						(rt4505_timeout_ms % 1000) * 1000000);
				hrtimer_start(&rt4505_timer, ktime, HRTIMER_MODE_REL);
			}
			rt4505_enable();
		} else {
			rt4505_disable();
			hrtimer_cancel(&rt4505_timer);
		}
		break;
	default:
		fl_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int rt4505_open(void *pArg)
{
	/* Actual behavior move to set driver function since power saving issue */
	return 0;
}

static int rt4505_release(void *pArg)
{
	/* uninit chip and clear usage count */
	mutex_lock(&rt4505_mutex);
	use_count--;
	if (!use_count)
		rt4505_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&rt4505_mutex);

	fl_dbg("Release: %d\n", use_count);

	return 0;
}

static int rt4505_set_driver(void)
{
	/* init chip and set usage count */
	mutex_lock(&rt4505_mutex);
	if (!use_count)
		rt4505_init();
	use_count++;
	mutex_unlock(&rt4505_mutex);

	fl_dbg("Set driver: %d\n", use_count);

	return 0;
}

static ssize_t rt4505_strobe_store(struct flashlight_arg arg)
{
	rt4505_set_driver();
	rt4505_set_level(arg.level);
	rt4505_enable();
	msleep(arg.dur);
	rt4505_disable();
	rt4505_release(NULL);

	return 0;
}

static struct flashlight_operations rt4505_ops = {
	rt4505_open,
	rt4505_release,
	rt4505_ioctl,
	rt4505_strobe_store,
	rt4505_set_driver
};


/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int rt4505_chip_init(struct rt4505_chip_data *chip)
{
	/* NOTE: Chip initialication move to "set driver" operation for power saving issue.
	 * rt4505_init();
	 */

	return 0;
}

static int rt4505_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct rt4505_chip_data *chip;
	struct rt4505_platform_data *pdata = client->dev.platform_data;
	int err;

	fl_dbg("Probe start.\n");

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		fl_err("Failed to check i2c functionality.\n");
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct rt4505_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	/* init platform data */
	if (!pdata) {
		fl_dbg("Platform data does not exist\n");
		pdata = kzalloc(sizeof(struct rt4505_platform_data), GFP_KERNEL);
		if (!pdata) {
			fl_err("Failed to allocate memory.\n");
			err = -ENOMEM;
			goto err_init_pdata;
		}
		chip->no_pdata = 1;
	}
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);
	rt4505_i2c_client = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* init work queue */
	INIT_WORK(&rt4505_work, rt4505_work_disable);

	/* init timer */
	hrtimer_init(&rt4505_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	rt4505_timer.function = rt4505_timer_func;
	rt4505_timeout_ms = 100;

	/* init chip hw */
	rt4505_chip_init(chip);

	/* register flashlight operations */
	if (flashlight_dev_register(RT4505_NAME, &rt4505_ops)) {
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

static int rt4505_i2c_remove(struct i2c_client *client)
{
	struct rt4505_chip_data *chip = i2c_get_clientdata(client);

	fl_dbg("Remove start.\n");

	/* flush work queue */
	flush_work(&rt4505_work);

	/* unregister flashlight operations */
	flashlight_dev_unregister(RT4505_NAME);

	/* free resource */
	if (chip->no_pdata)
		kfree(chip->pdata);
	kfree(chip);

	fl_dbg("Remove done.\n");

	return 0;
}

static const struct i2c_device_id rt4505_i2c_id[] = {
	{RT4505_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, rt4505_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id rt4505_i2c_of_match[] = {
	{.compatible = RT4505_DTNAME_I2C},
	{},
};
MODULE_DEVICE_TABLE(of, rt4505_i2c_of_match);
#endif

static struct i2c_driver rt4505_i2c_driver = {
	.driver = {
		   .name = RT4505_NAME,
#ifdef CONFIG_OF
		   .of_match_table = rt4505_i2c_of_match,
#endif
		   },
	.probe = rt4505_i2c_probe,
	.remove = rt4505_i2c_remove,
	.id_table = rt4505_i2c_id,
};

module_i2c_driver(rt4505_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight RT4505 Driver");

