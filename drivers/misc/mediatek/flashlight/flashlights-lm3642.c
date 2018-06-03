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

/* device tree should be defined in flashlight-dt.h */
#ifndef LM3642_DTNAME
#define LM3642_DTNAME "mediatek,flashlights_lm3642"
#endif
#ifndef LM3642_DTNAME_I2C
#define LM3642_DTNAME_I2C "mediatek,strobe_main"
#endif

#define LM3642_NAME "flashlights-lm3642"

/* define registers */
#define LM3642_REG_ENABLE (0x01)
#define LM3642_MASK_ENABLE_LED1 (0x01)
#define LM3642_DISABLE (0x00)
#define LM3642_ENABLE_LED1 (0x01)
#define LM3642_ENABLE_LED1_TORCH (0x09)
#define LM3642_ENABLE_LED1_FLASH (0x0D)

#define LM3642_REG_TORCH_LEVEL_LED1 (0x05)
#define LM3642_REG_FLASH_LEVEL_LED1 (0x03)

#define LM3642_REG_TIMING_CONF (0x08)
#define LM3642_TORCH_RAMP_TIME (0x10)
#define LM3642_FLASH_TIMEOUT   (0x0F)

/* define channel, level */
#define LM3642_CHANNEL_NUM 1
#define LM3642_CHANNEL_CH1 0

#define LM3642_LEVEL_NUM 16
#define LM3642_LEVEL_TORCH 2

/* define mutex and work queue */
static DEFINE_MUTEX(lm3642_mutex);
static struct work_struct lm3642_work_ch1;

static int g_bLtVersion;


/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *lm3642_i2c_client;

/* platform data */
struct lm3642_platform_data {
	u8 torch_pin_enable;         /* 1: TX1/TORCH pin isa hardware TORCH enable */
	u8 pam_sync_pin_enable;      /* 1: TX2 Mode The ENVM/TX2 is a PAM Sync. on input */
	u8 thermal_comp_mode_enable; /* 1: LEDI/NTC pin in Thermal Comparator Mode */
	u8 strobe_pin_disable;       /* 1: STROBE Input disabled */
	u8 vout_mode_enable;         /* 1: Voltage Out Mode enable */
};

/* lm3642 chip data */
struct lm3642_chip_data {
	struct i2c_client *client;
	struct lm3642_platform_data *pdata;
	struct mutex lock;
	u8 last_flag;
	u8 no_pdata;
};

/******************************************************************************
 * lm3642 operations
 *****************************************************************************/
static volatile unsigned char lm3642_reg_enable;
static volatile int lm3642_level_ch1 = -1;

static int lm3642_is_torch(int level)
{
	if (level <= LM3642_LEVEL_TORCH)
		return 1;

	return 0;
}

/* i2c wrapper function */
static int lm3642_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	struct lm3642_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);

	if (ret < 0)
		fl_err("failed writing at 0x%02x\n", reg);

	return ret;
}

static int lm3642_read_reg(struct i2c_client *client, u8 reg)
{
	int val = 0;
	struct lm3642_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	val = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&chip->lock);


	return val;
}

int readReg(int reg)
{
	int val;

	val = lm3642_read_reg(lm3642_i2c_client, reg);
	return (int)val;
}

/* flashlight enable function */
static int lm3642_enable_ch1(void)
{
	int val;

	if (lm3642_is_torch(lm3642_level_ch1)) {
		/* torch mode */
		if (g_bLtVersion == 1) {
			if (lm3642_level_ch1 == 0)
				val = 3;
			else if (lm3642_level_ch1 == 1)
				val = 5;
			else	/* if(g_duty==2) */
				val = 7;
		} else {
			if (lm3642_level_ch1 == 0)
				val = 1;
			else if (lm3642_level_ch1 == 1)
				val = 2;
			else	/* if(g_duty==2) */
				val = 3;
		}

		lm3642_write_reg(lm3642_i2c_client, 0x09, val << 4);
		lm3642_write_reg(lm3642_i2c_client, 0x0A, 0x02);
	} else {
		/* flash mode */
		lm3642_write_reg(lm3642_i2c_client, 0x09, lm3642_level_ch1 - 1);
		lm3642_write_reg(lm3642_i2c_client, 0x0A, 0x03);
	}

	return 0;
}

static int lm3642_enable(int channel)
{
	if (channel == LM3642_CHANNEL_CH1)
		lm3642_enable_ch1();
	else {
		fl_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* flashlight disable function */
static int lm3642_disable_ch1(void)
{
	int ret = 0;

	lm3642_write_reg(lm3642_i2c_client, 0x0A, 0x00);

	return ret;
}

static int lm3642_disable(int channel)
{
	if (channel == LM3642_CHANNEL_CH1)
		lm3642_disable_ch1();
	else {
		fl_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* set flashlight level */
static int lm3642_set_level_ch1(int level)
{
	int ret = 0;

	lm3642_level_ch1 = level;

	return ret;
}

static int lm3642_set_level(int channel, int level)
{
	if (channel == LM3642_CHANNEL_CH1)
		lm3642_set_level_ch1(level);
	else {
		fl_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* flashlight init */
int lm3642_init(void)
{
	int ret = 0;
	int regVal0;

	lm3642_write_reg(lm3642_i2c_client, 0x0A, 0x00);
	lm3642_write_reg(lm3642_i2c_client, 0x08, 0x47);
	lm3642_write_reg(lm3642_i2c_client, 0x09, 0x35);

	regVal0 = lm3642_read_reg(lm3642_i2c_client, 0);
	if (regVal0 == 1)
		g_bLtVersion = 1;
	else
		g_bLtVersion = 0;

	fl_info("regVal0=%d isLtVer=%d\n", regVal0, g_bLtVersion);

	return ret;
}

/* flashlight uninit */
int lm3642_uninit(void)
{
	lm3642_disable(LM3642_CHANNEL_CH1);

	return 0;
}


/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer lm3642_timer_ch1;
static unsigned int lm3642_timeout_ms[LM3642_CHANNEL_NUM];

static void lm3642_work_disable_ch1(struct work_struct *data)
{
	fl_dbg("ht work queue callback\n");
	lm3642_disable_ch1();
}

static enum hrtimer_restart lm3642_timer_func_ch1(struct hrtimer *timer)
{
	schedule_work(&lm3642_work_ch1);
	return HRTIMER_NORESTART;
}

int lm3642_timer_start(int channel, ktime_t ktime)
{
	if (channel == LM3642_CHANNEL_CH1)
		hrtimer_start(&lm3642_timer_ch1, ktime, HRTIMER_MODE_REL);
	else {
		fl_err("Error channel\n");
		return -1;
	}

	return 0;
}

int lm3642_timer_cancel(int channel)
{
	if (channel == LM3642_CHANNEL_CH1)
		hrtimer_cancel(&lm3642_timer_ch1);
	else {
		fl_err("Error channel\n");
		return -1;
	}

	return 0;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int lm3642_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	/* verify channel */
	if (channel < 0 || channel >= LM3642_CHANNEL_NUM) {
		fl_err("Failed with error channel\n");
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		fl_dbg("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		lm3642_timeout_ms[channel] = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		fl_dbg("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		lm3642_set_level(channel, fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		fl_dbg("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (lm3642_timeout_ms[channel]) {
				ktime = ktime_set(lm3642_timeout_ms[channel] / 1000,
						(lm3642_timeout_ms[channel] % 1000) * 1000000);
				lm3642_timer_start(channel, ktime);
			}
			lm3642_enable(channel);
		} else {
			lm3642_disable(channel);
			lm3642_timer_cancel(channel);
		}
		break;

	default:
		fl_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int lm3642_open(void *pArg)
{
	/* Actual behavior move to set driver function since power saving issue */
	return 0;
}

static int lm3642_release(void *pArg)
{
	/* uninit chip and clear usage count */
	mutex_lock(&lm3642_mutex);
	use_count--;
	if (!use_count)
		lm3642_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&lm3642_mutex);

	fl_dbg("Release: %d\n", use_count);

	return 0;
}

static int lm3642_set_driver(void)
{
	/* init chip and set usage count */
	mutex_lock(&lm3642_mutex);
	if (!use_count)
		lm3642_init();
	use_count++;
	mutex_unlock(&lm3642_mutex);

	fl_dbg("Set driver: %d\n", use_count);

	return 0;
}

static ssize_t lm3642_strobe_store(struct flashlight_arg arg)
{
	lm3642_set_driver();
	lm3642_set_level(arg.ct, arg.level);
	lm3642_enable(arg.ct);
	msleep(arg.dur);
	lm3642_disable(arg.ct);
	lm3642_release(NULL);

	return 0;
}

static struct flashlight_operations lm3642_ops = {
	lm3642_open,
	lm3642_release,
	lm3642_ioctl,
	lm3642_strobe_store,
	lm3642_set_driver
};


/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int lm3642_chip_init(struct lm3642_chip_data *chip)
{
	/* NOTE: Chip initialication move to "set driver" operation for power saving issue.
	 * lm3642_init();
	 */

	return 0;
}

static int lm3642_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct lm3642_chip_data *chip;
	struct lm3642_platform_data *pdata = client->dev.platform_data;
	int err;

	fl_dbg("Probe start.\n");

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		fl_err("Failed to check i2c functionality.\n");
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct lm3642_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	/* init platform data */
	if (!pdata) {
		fl_dbg("Platform data does not exist\n");
		pdata = kzalloc(sizeof(struct lm3642_platform_data), GFP_KERNEL);
		if (!pdata) {
			fl_err("Failed to allocate memory.\n");
			err = -ENOMEM;
			goto err_init_pdata;
		}
		chip->no_pdata = 1;
	}
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);
	lm3642_i2c_client = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* init work queue */
	INIT_WORK(&lm3642_work_ch1, lm3642_work_disable_ch1);

	/* init timer */
	hrtimer_init(&lm3642_timer_ch1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	lm3642_timer_ch1.function = lm3642_timer_func_ch1;
	lm3642_timeout_ms[LM3642_CHANNEL_CH1] = 1000;

	/* init chip hw */
	lm3642_chip_init(chip);

	/* register flashlight operations */
	if (flashlight_dev_register(LM3642_NAME, &lm3642_ops)) {
		fl_err("Failed to register flashlight device.\n");
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

static int lm3642_i2c_remove(struct i2c_client *client)
{
	struct lm3642_chip_data *chip = i2c_get_clientdata(client);

	fl_dbg("Remove start.\n");

	/* flush work queue */
	flush_work(&lm3642_work_ch1);

	/* unregister flashlight operations */
	flashlight_dev_unregister(LM3642_NAME);

	/* free resource */
	if (chip->no_pdata)
		kfree(chip->pdata);
	kfree(chip);

	fl_dbg("Remove done.\n");

	return 0;
}

static const struct i2c_device_id lm3642_i2c_id[] = {
	{LM3642_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id lm3642_i2c_of_match[] = {
	{.compatible = LM3642_DTNAME_I2C},
	{},
};
#endif

static struct i2c_driver lm3642_i2c_driver = {
	.driver = {
		   .name = LM3642_NAME,
#ifdef CONFIG_OF
		   .of_match_table = lm3642_i2c_of_match,
#endif
		   },
	.probe = lm3642_i2c_probe,
	.remove = lm3642_i2c_remove,
	.id_table = lm3642_i2c_id,
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int lm3642_probe(struct platform_device *dev)
{
	fl_dbg("Probe start.\n");

	if (i2c_add_driver(&lm3642_i2c_driver)) {
		fl_dbg("Failed to add i2c driver.\n");
		return -1;
	}

	fl_dbg("Probe done.\n");

	return 0;
}

static int lm3642_remove(struct platform_device *dev)
{
	fl_dbg("Remove start.\n");

	i2c_del_driver(&lm3642_i2c_driver);

	fl_dbg("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id lm3642_of_match[] = {
	{.compatible = LM3642_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, lm3642_of_match);
#else
static struct platform_device lm3642_platform_device[] = {
	{
		.name = LM3642_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, lm3642_platform_device);
#endif

static struct platform_driver lm3642_platform_driver = {
	.probe = lm3642_probe,
	.remove = lm3642_remove,
	.driver = {
		.name = LM3642_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = lm3642_of_match,
#endif
	},
};

static int __init flashlight_lm3642_init(void)
{
	int ret;

	fl_dbg("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&lm3642_platform_device);
	if (ret) {
		fl_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&lm3642_platform_driver);
	if (ret) {
		fl_err("Failed to register platform driver\n");
		return ret;
	}

	fl_dbg("Init done.\n");

	return 0;
}

static void __exit flashlight_lm3642_exit(void)
{
	fl_dbg("Exit start.\n");

	platform_driver_unregister(&lm3642_platform_driver);

	fl_dbg("Exit done.\n");
}

module_init(flashlight_lm3642_init);
module_exit(flashlight_lm3642_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xi Chen <xixi.chen@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight LM3642 Driver");

