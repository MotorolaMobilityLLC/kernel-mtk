/*
* Copyright (C) 2022 Awinic Inc.
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
#include <linux/of.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include "flashlight.h"
#include "flashlight-dt.h"
#include "flashlight-core.h"

/* device tree should be defined in flashlight-dt.h */
#ifndef AW36518_DTNAME
#define AW36518_DTNAME "awinic,aw36518"
#endif
#ifndef AW36518_DTNAME_I2C
#define AW36518_DTNAME_I2C "awinic,aw36518"
#endif
#define AW36518_NAME "aw36518"

#define AW36518_DRIVER_VERSION "V1.1.0"

#define AW36518_REG_BOOST_CONFIG        (0x07)
#define AW36518_BIT_SOFT_RST_MASK       (~(1<<7))
#define AW36518_BIT_SOFT_RST_ENABLE     (1<<7)
#define AW36518_BIT_SOFT_RST_DISABLE    (0<<7)

/* define registers */
#define AW36518_REG_CHIPID              (0x00)
#define AW36518_CHIPID                  (0x30)
#define AW36518_REG_ENABLE              (0x01)
#define AW36518_MASK_ENABLE_LED1        (0x01)
#define AW36518_DISABLE                 (0x00)
#define AW36518_ENABLE_LED1             (0x03)
#define AW36518_ENABLE_LED1_TORCH       (0x0B)
#define AW36518_REG_DEVICE_ID           (0x0C)
#define AW36518_DEVICE_ID               (0x0A)
#define AW36518_ENABLE_LED1_FLASH       (0x0F)
#define AW36518_REG_DUMMY               (0x09)
#define AW36518_REG_FLAG1               (0x0A)
#define AW36518_REG_FLAG2               (0x0B)

#define AW36518_REG_FLASH_LEVEL_LED1    (0x03)
#define AW36518_REG_TORCH_LEVEL_LED1    (0x05)

#define AW36518_REG_CTRL1               (0x31)
#define AW36518_REG_CTRL2               (0x69)
#define AW36518_REG_CHIP_VENDOR_ID      (0x25)
#define AW36518_CHIP_VENDOR_ID          (0x04)

#define AW36518_REG_TIMING_CONF         (0x08)
#define AW36518_TORCH_RAMP_TIME         (0x10) //100b  128ms
#define AW36518_FLASH_TIMEOUT           (0x0A) //1010b 600ms
#define AW36518_CHIP_STANDBY            (0x80)
#define AW36518_HW_TIMEOUT              (600)

/* define channel, level */
#define AW36518_CHANNEL_NUM				1
#define AW36518_CHANNEL_CH1				0

#define AW36518_LEVEL_NUM				30
#define AW36518_LEVEL_TORCH				7

#define AW_I2C_RETRIES					5
#define AW_I2C_RETRY_DELAY				2

/* define mutex and work queue */
static DEFINE_MUTEX(aw36518_mutex);
static struct work_struct aw36518_work_ch1;

struct i2c_client *aw36518_flashlight_client;

/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *aw36518_i2c_client;

/* platform data
* torch_pin_enable: TX1/TORCH pin isa hardware TORCH enable
* pam_sync_pin_enable: TX2 Mode The ENVM/TX2 is a PAM Sync. on input
* thermal_comp_mode_enable: LEDI/NTC pin in Thermal Comparator Mode
* strobe_pin_disable: STROBE Input disabled
* vout_mode_enable: Voltage Out Mode enable
*/
struct aw36518_platform_data {
	u8 torch_pin_enable;
	u8 pam_sync_pin_enable;
	u8 thermal_comp_mode_enable;
	u8 strobe_pin_disable;
	u8 vout_mode_enable;
};

/* aw36518 chip data */
struct aw36518_chip_data {
	struct i2c_client *client;
	struct aw36518_platform_data *pdata;
	struct mutex lock;
	unsigned int chipID;
	unsigned int deviceID;
	bool is_mode_switch_init;
	u8 last_flag;
	u8 no_pdata;
};

/******************************************************************************
 * aw36518 operations
 *****************************************************************************/
static const int aw36518_current[AW36518_LEVEL_NUM] = {//y=18 + 62*x
	10,   28,   46,   64,   82,   100,  118,  136,  198,  260,
	322,  384,  446,  508,  570,  632,  694,  756,  818,  880,
	942,  1004, 1066, 1128, 1190, 1252, 1314, 1376, 1438, 1500
};

static const unsigned char aw36518_torch_level[AW36518_LEVEL_NUM] = {
	0x06, 0x12, 0x1D, 0x29, 0x35, 0x41, 0x4D, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char aw36518_flash_level[AW36518_LEVEL_NUM] = {
	0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x16, 0x21, 0x2B,
	0x36, 0x40, 0x4B, 0x56, 0x60, 0x6B, 0x75, 0x80, 0x8A, 0x95,
	0x9F, 0xAA, 0xB5, 0xBF, 0xCA, 0xD4, 0xDF, 0xE9, 0xF4, 0xFF
};

static volatile unsigned char aw36518_reg_enable;
static volatile int aw36518_level_ch1 = -1;

static int aw36518_is_torch(int level)
{
	if (level >= AW36518_LEVEL_TORCH)
		return -1;

	return 0;
}

static int aw36518_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= AW36518_LEVEL_NUM)
		level = AW36518_LEVEL_NUM - 1;

	return level;
}

/* i2c wrapper function */
static int aw36518_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char val)
{
	int ret;
	unsigned char cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_smbus_write_byte_data(client, reg, val);
		if (ret < 0) {
			pr_info("%s: i2c_write addr=0x%02X, data=0x%02X, cnt=%d, error=%d\n",
				   __func__, reg, val, cnt, ret);
		} else {
			break;
		}
		cnt++;
		msleep(AW_I2C_RETRY_DELAY);
	}

	return ret;
}

static int aw36518_i2c_read(struct i2c_client *client, unsigned char reg, unsigned char *val)
{
	int ret;
	unsigned char cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_smbus_read_byte_data(client, reg);
		if (ret < 0) {
			pr_info("%s: i2c_read addr=0x%02X, cnt=%d, error=%d\n",
				   __func__, reg, cnt, ret);
		} else {
			*val = ret;
			break;
		}
		cnt++;
		msleep(AW_I2C_RETRY_DELAY);
	}

	return ret;
}

static void aw36518_mode_cfg_init(void)
{
	struct aw36518_chip_data *pdata = aw36518_i2c_client->dev.driver_data;

	if (pdata->is_mode_switch_init)
	{
		aw36518_i2c_write(aw36518_i2c_client, AW36518_REG_CTRL2, 0x02);
		aw36518_i2c_write(aw36518_i2c_client, AW36518_REG_CTRL1, 0x0C);
	}
}

static void aw36518_soft_reset(void)
{
	unsigned char reg_val;

	aw36518_i2c_read(aw36518_i2c_client, AW36518_REG_BOOST_CONFIG, &reg_val);
	reg_val &= AW36518_BIT_SOFT_RST_MASK;
	reg_val |= AW36518_BIT_SOFT_RST_ENABLE;
	aw36518_i2c_write(aw36518_i2c_client, AW36518_REG_BOOST_CONFIG, reg_val);
	msleep(5);
}

/* flashlight enable function */
static int aw36518_enable_ch1(void)
{
	unsigned char reg, val;

	aw36518_mode_cfg_init();
	reg = AW36518_REG_ENABLE;
	if (!aw36518_is_torch(aw36518_level_ch1)) {
		/* torch mode */
		pr_info("%s enter torch mode set.\n", __func__);
		aw36518_reg_enable |= AW36518_ENABLE_LED1_TORCH;
	} else {
		/* flash mode */
		pr_info("%s enter flash mode set.\n", __func__);
		aw36518_reg_enable |= AW36518_ENABLE_LED1_FLASH;
	}

	val = aw36518_reg_enable;

	return aw36518_i2c_write(aw36518_i2c_client, reg, val);
}

/* flashlight disable function */
static int aw36518_disable_ch1(void)
{
	unsigned char reg, val;

	aw36518_mode_cfg_init();
	reg = AW36518_REG_ENABLE;
	aw36518_reg_enable &= (~AW36518_ENABLE_LED1_FLASH);
	val = aw36518_reg_enable;

	return aw36518_i2c_write(aw36518_i2c_client, reg, val);
}

static int aw36518_get_flag1(void)
{
    unsigned char val;
    return aw36518_i2c_read(aw36518_i2c_client, AW36518_REG_FLAG1, &val);
}

static int aw36518_get_flag2(void)
{
    unsigned char val;
    return aw36518_i2c_read(aw36518_i2c_client, AW36518_REG_FLAG2, &val);
}

static int aw36518_enable(int channel)
{
	if (channel == AW36518_CHANNEL_CH1)
		aw36518_enable_ch1();
	else {
		pr_err("%s, Error channel\n", __func__);
		return -1;
	}

	return 0;
}

static int aw36518_disable(int channel)
{
	if (channel == AW36518_CHANNEL_CH1)
		aw36518_disable_ch1();
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* set flashlight level */
static int aw36518_set_level_ch1(int level)
{
	int ret;
	unsigned char reg, val;

	level = aw36518_verify_level(level);

	/* set torch brightness level */
	reg = AW36518_REG_TORCH_LEVEL_LED1;
	val = aw36518_torch_level[level];
	ret = aw36518_i2c_write(aw36518_i2c_client, reg, val);

	aw36518_level_ch1 = level;

	/* set flash brightness level */
	reg = AW36518_REG_FLASH_LEVEL_LED1;
	val = aw36518_flash_level[level];
	ret = aw36518_i2c_write(aw36518_i2c_client, reg, val);

	return ret;
}

static int aw36518_set_level(int channel, int level)
{
	if (channel == AW36518_CHANNEL_CH1)
		aw36518_set_level_ch1(level);
	else {
		pr_err("%s, Error channel\n", __func__);
		return -1;
	}

	return 0;
}
int aw36518_read_vendor_id(void)
{
	unsigned char val, reg_data;
	struct aw36518_chip_data *pdata = aw36518_i2c_client->dev.driver_data;

	usleep_range(2000, 2500);

	/* read chip vendor information */
	aw36518_i2c_read(aw36518_i2c_client, AW36518_REG_CHIP_VENDOR_ID, &val);
	pr_info("aw36518 0x25 vendorID = 0x%2x\n", val);

	aw36518_i2c_read(aw36518_i2c_client, AW36518_REG_DUMMY, &reg_data);
	pr_info("aw36518 0x09 ver = 0x%2x\n", reg_data);

	if (((val & 0x04) == AW36518_CHIP_VENDOR_ID) && !(reg_data & 0x01)) {
		pr_info("aw36518 is_mode_switch_init is true\n");
		pdata->is_mode_switch_init = true;
	} else {
		pdata->is_mode_switch_init = false;
		pr_info("aw36518 is_mode_switch_init is false\n");
	}

	return 0;
}
/* flashlight init */
int aw36518_init(void)
{
	int ret;
	unsigned char reg, val;

	/* clear enable register */
	aw36518_mode_cfg_init();
	reg = AW36518_REG_ENABLE;
	val = AW36518_DISABLE;
	ret = aw36518_i2c_write(aw36518_i2c_client, reg, val);

	aw36518_reg_enable = val;

	/* set torch current ramp time and flash timeout */
	reg = AW36518_REG_TIMING_CONF;
	val = AW36518_TORCH_RAMP_TIME | AW36518_FLASH_TIMEOUT;
	ret = aw36518_i2c_write(aw36518_i2c_client, reg, val);

	return ret;
}

/* flashlight uninit */
int aw36518_uninit(void)
{
	aw36518_disable(AW36518_CHANNEL_CH1);
	aw36518_get_flag1();
	aw36518_get_flag2();
	return 0;
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer aw36518_timer_ch1;
static unsigned int aw36518_timeout_ms[AW36518_CHANNEL_NUM];

static void aw36518_work_disable_ch1(struct work_struct *data)
{
	pr_info("%s, ht work queue callback\n", __func__);

	aw36518_disable_ch1();
}

static enum hrtimer_restart aw36518_timer_func_ch1(struct hrtimer *timer)
{
	schedule_work(&aw36518_work_ch1);
	return HRTIMER_NORESTART;
}


int aw36518_timer_start(int channel, ktime_t ktime)
{
	if (channel == AW36518_CHANNEL_CH1)
		hrtimer_start(&aw36518_timer_ch1, ktime, HRTIMER_MODE_REL);
	else {
		pr_err("%s, Error channel\n", __func__);
		return -1;
	}

	return 0;
}

int aw36518_timer_cancel(int channel)
{
	if (channel == AW36518_CHANNEL_CH1)
		hrtimer_cancel(&aw36518_timer_ch1);
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int aw36518_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	/* verify channel */
	if (channel < 0 || channel >= AW36518_CHANNEL_NUM) {
		pr_err("%s, Failed with error channel\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_info("%s, FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				__func__, channel, (int)fl_arg->arg);
		aw36518_timeout_ms[channel] = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_info("%s. FLASH_IOC_SET_DUTY(%d): %d\n",
				__func__, channel, (int)fl_arg->arg);
		aw36518_set_level(channel, fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_info("%s. FLASH_IOC_SET_ONOFF(%d): %d\n",
				__func__, channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (aw36518_timeout_ms[channel]) {
				ktime =
				ktime_set(aw36518_timeout_ms[channel] / 1000,
				(aw36518_timeout_ms[channel] % 1000) * 1000000);
				aw36518_timer_start(channel, ktime);
			}
			aw36518_enable(channel);
		} else {
			aw36518_disable(channel);
			aw36518_timer_cancel(channel);
		}
		break;

	case FLASH_IOC_GET_DUTY_NUMBER:
#if defined(CONFIG_MOT_CANCUNN_CAMERA_PROJECT)
		fl_arg->arg = AW36518_LEVEL_NUM - 6;
#else
		fl_arg->arg = AW36518_LEVEL_NUM;
#endif
		pr_info("%s. FLASH_IOC_GET_DUTY_NUMBER(%d): %d\n",
				__func__, channel, (int)fl_arg->arg);
		break;

	case FLASH_IOC_GET_MAX_TORCH_DUTY:
		pr_debug("FLASH_IOC_GET_MAX_TORCH_DUTY(%d)\n", channel);
		fl_arg->arg = AW36518_LEVEL_TORCH - 1;
		break;

	case FLASH_IOC_GET_DUTY_CURRENT:
		fl_arg->arg = aw36518_verify_level(fl_arg->arg);
		pr_debug("FLASH_IOC_GET_DUTY_CURRENT(%d): %d\n",
				channel, (int)fl_arg->arg);
		fl_arg->arg = aw36518_current[fl_arg->arg];
		break;

	case FLASH_IOC_GET_HW_TIMEOUT:
		pr_debug("FLASH_IOC_GET_HW_TIMEOUT(%d)\n", channel);
		fl_arg->arg = AW36518_HW_TIMEOUT;
		break;

	case FLASH_IOC_GET_HW_FAULT:
		pr_debug("FLASH_IOC_GET_HW_FAULT(%d)\n", channel);
		fl_arg->arg = aw36518_get_flag1();
		break;

	case FLASH_IOC_GET_HW_FAULT2:
		pr_debug("FLASH_IOC_GET_HW_FAULT2(%d)\n", channel);
		fl_arg->arg = aw36518_get_flag2();
		break;

	default:
		pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int aw36518_open(void)
{
	/* Actual behavior move to set driver function */
	/*since power saving issue */
	return 0;
}

static int aw36518_release(void)
{
	/* uninit chip and clear usage count */
	mutex_lock(&aw36518_mutex);
	use_count--;
	if (!use_count)
		aw36518_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&aw36518_mutex);

	pr_info("Release: %d\n", use_count);

	return 0;
}

static int aw36518_set_driver(int set)
{
	/* init chip and set usage count */
	mutex_lock(&aw36518_mutex);
	if (!use_count)
		aw36518_init();
	use_count++;
	mutex_unlock(&aw36518_mutex);

	pr_info("Set driver: %d\n", use_count);

	return 0;
}

static ssize_t aw36518_strobe_store(struct flashlight_arg arg)
{
	aw36518_set_driver(1);
	aw36518_set_level(arg.ct, arg.level);
	aw36518_enable(arg.ct);
	msleep(arg.dur);
	aw36518_disable(arg.ct);
	aw36518_set_driver(0);

	return 0;
}

static struct flashlight_operations aw36518_ops = {
	aw36518_open,
	aw36518_release,
	aw36518_ioctl,
	aw36518_strobe_store,
	aw36518_set_driver
};

/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int aw36518_chip_init(struct aw36518_chip_data *chip)
{
	/* NOTE: Chip initialication move to
	*"set driver" operation for power saving issue.
	*/
	int ret = 0;

	ret = aw36518_init();

	return ret;
}

/***************************************************************************/
/*AW36518 Debug file */
/***************************************************************************/
static ssize_t
aw36518_get_reg(struct device *cd, struct device_attribute *attr, char *buf)
{
	unsigned char reg_val;
	unsigned char i;
	ssize_t len = 0;

	for (i = 0; i < 0x0E; i++) {
		aw36518_i2c_read(aw36518_i2c_client, i, &reg_val);
		len += snprintf(buf+len, PAGE_SIZE-len,
			"reg0x%2X = 0x%2X\n", i, reg_val);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\r\n");
	return len;
}

static ssize_t aw36518_set_reg(struct device *cd,
		struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int databuf[2];

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		aw36518_i2c_write(aw36518_i2c_client, databuf[0], databuf[1]);
	return len;
}

static DEVICE_ATTR(reg, 0660, aw36518_get_reg, aw36518_set_reg);

static int aw36518_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	err = device_create_file(dev, &dev_attr_reg);

	return err;
}

static int
aw36518_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct aw36518_chip_data *chip;
	struct aw36518_platform_data *pdata = client->dev.platform_data;
	int err;

	pr_info("%s Probe start.\n", __func__);

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s, Failed to check i2c functionality.\n", __func__);
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct aw36518_chip_data), GFP_KERNEL);
	if (!chip) {
		pr_err("%s, Failed to kzalloc memory.\n", __func__);
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	/* init platform data */
	if (!pdata) {
		pr_err("%s, Platform data does not exist\n", __func__);
		pdata =
		kzalloc(sizeof(struct aw36518_platform_data), GFP_KERNEL);
		if (!pdata) {
			pr_err("%s, Failed to kzalloc memory.\n", __func__);
			err = -ENOMEM;
			goto err_init_pdata;
		}
		chip->no_pdata = 1;
	}
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);
	aw36518_i2c_client = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* read 0x25 vendor id */
	aw36518_read_vendor_id();

	/* soft rst */
	aw36518_soft_reset();

	/* init work queue */
	INIT_WORK(&aw36518_work_ch1, aw36518_work_disable_ch1);

	/* init timer */
	hrtimer_init(&aw36518_timer_ch1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aw36518_timer_ch1.function = aw36518_timer_func_ch1;
	aw36518_timeout_ms[AW36518_CHANNEL_CH1] = 100;

	/* init chip hw */
	aw36518_chip_init(chip);

	/* register flashlight operations */
	if (flashlight_dev_register(AW36518_NAME, &aw36518_ops)) {
		pr_err("%s,Failed to register flashlight device.\n", __func__);
		err = -EFAULT;
		goto err_free;
	}

	/* clear usage count */
	use_count = 0;

	aw36518_create_sysfs(client);

	pr_info("%s Probe done.\n", __func__);

	return 0;

err_free:
	kfree(chip->pdata);
err_init_pdata:
	i2c_set_clientdata(client, NULL);
	kfree(chip);
err_out:
	return err;
}

static int aw36518_i2c_remove(struct i2c_client *client)
{
	struct aw36518_chip_data *chip = i2c_get_clientdata(client);

	pr_info("Remove start.\n");

	/* flush work queue */
	flush_work(&aw36518_work_ch1);

	/* unregister flashlight operations */
	flashlight_dev_unregister(AW36518_NAME);

	/* free resource */
	if (chip->no_pdata)
		kfree(chip->pdata);
	kfree(chip);

	pr_info("Remove done.\n");

	return 0;
}

static void aw36518_i2c_shutdown(struct i2c_client *client)
{
	pr_info("aw36518 shutdown start.\n");

	aw36518_mode_cfg_init();

	aw36518_i2c_write(aw36518_i2c_client, AW36518_REG_ENABLE,
						AW36518_CHIP_STANDBY);

	pr_info("aw36518 shutdown done.\n");
}


static const struct i2c_device_id aw36518_i2c_id[] = {
	{AW36518_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id aw36518_i2c_of_match[] = {
	{.compatible = AW36518_DTNAME_I2C},
	{},
};
#endif

static struct i2c_driver aw36518_i2c_driver = {
	.driver = {
		   .name = AW36518_NAME,
#ifdef CONFIG_OF
		   .of_match_table = aw36518_i2c_of_match,
#endif
		   },
	.probe = aw36518_i2c_probe,
	.remove = aw36518_i2c_remove,
	.shutdown = aw36518_i2c_shutdown,
	.id_table = aw36518_i2c_id,
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int aw36518_probe(struct platform_device *dev)
{
	pr_info("%s Probe start.\n", __func__);

	if (i2c_add_driver(&aw36518_i2c_driver)) {
		pr_err("Failed to add i2c driver.\n");
		return -1;
	}

	pr_info("%s Probe done.\n", __func__);

	return 0;
}

static int aw36518_remove(struct platform_device *dev)
{
	pr_info("Remove start.\n");

	i2c_del_driver(&aw36518_i2c_driver);

	pr_info("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id aw36518_of_match[] = {
	{.compatible = AW36518_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, aw36518_of_match);
#else
static struct platform_device aw36518_platform_device[] = {
	{
		.name = AW36518_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, aw36518_platform_device);
#endif

static struct platform_driver aw36518_platform_driver = {
	.probe = aw36518_probe,
	.remove = aw36518_remove,
	.driver = {
		.name = AW36518_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = aw36518_of_match,
#endif
	},
};

static int __init flashlight_aw36518_init(void)
{
	int ret;

	pr_info("%s driver version %s.\n", __func__, AW36518_DRIVER_VERSION);

#ifndef CONFIG_OF
	ret = platform_device_register(&aw36518_platform_device);
	if (ret) {
		pr_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&aw36518_platform_driver);
	if (ret) {
		pr_err("Failed to register platform driver\n");
		return ret;
	}

	pr_info("flashlight_aw36518 Init done.\n");

	return 0;
}

static void __exit flashlight_aw36518_exit(void)
{
	pr_info("flashlight_aw36518-Exit start.\n");

	platform_driver_unregister(&aw36518_platform_driver);

	pr_info("flashlight_aw36518 Exit done.\n");
}

module_init(flashlight_aw36518_init);
module_exit(flashlight_aw36518_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Awinic");
MODULE_DESCRIPTION("AW36518 Flashlight Driver");

