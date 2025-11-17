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
#ifndef SBUYA_SGM37864_DTNAME
#define SBUYA_SGM37864_DTNAME "mediatek,flashlights_sgm37864"
#endif
#ifndef SBUYA_SGM37864_DTNAME_I2C
#define SBUYA_SGM37864_DTNAME_I2C "mediatek,strobe_main"
#endif
#define SBUYA_SGM37864_NAME "flashlights-sgm37864"

#define SGM37864_DRIVER_VERSION "V1.1.0"

#define SGM37864_REG_BOOST_CONFIG     (0x07)
#define SGM37864_BIT_SOFT_RST_MASK    (~(1<<7))
#define SGM37864_BIT_SOFT_RST_ENABLE  (1<<7)
#define SGM37864_BIT_SOFT_RST_DISABLE (0<<7)

/* define registers */
#define SGM37864_REG_ENABLE           (0x01)
#define SGM37864_MASK_ENABLE_LED1     (0x01)
#define SGM37864_MASK_ENABLE_LED2     (0x02)
#define SGM37864_DISABLE              (0x00)
#define SGM37864_ENABLE_LED1          (0x01)
#define SGM37864_ENABLE_LED1_TORCH    (0x09)
#define SGM37864_ENABLE_LED1_FLASH    (0x0D)
#define SGM37864_ENABLE_LED2          (0x02)
#define SGM37864_ENABLE_LED2_TORCH    (0x0A)
#define SGM37864_ENABLE_LED2_FLASH    (0x0E)

#define SGM37864_REG_DUMMY            (0x0A)

#define SGM37864_REG_DEVICES_ID       (0x0C)
#define SGM37864_VER_DEVICES_ID       (0x11)

#define SGM37864_REG_TORCH_LEVEL_LED1 (0x05)
#define SGM37864_REG_FLASH_LEVEL_LED1 (0x03)
#define SGM37864_REG_TORCH_LEVEL_LED2 (0x06)
#define SGM37864_REG_FLASH_LEVEL_LED2 (0x04)

#define SGM37864_REG_TIMING_CONF      (0x08)
#define SGM37864_TORCH_RAMP_TIME      (0x10)
#define SGM37864_FLASH_TIMEOUT        (0x09)
#define SGM37864_CHIP_STANDBY         (0x80)

/* define channel, level */
#define SGM37864_CHANNEL_NUM          2
#define SGM37864_CHANNEL_CH1          0
#define SGM37864_LEVEL_NUM            26
#define SGM37864_LEVEL_TORCH          7

#define AW_I2C_RETRIES                5
#define AW_I2C_RETRY_DELAY            2

/* define mutex and work queue */
static DEFINE_MUTEX(sgm37864_mutex);
static struct work_struct sgm37864_work_ch1;

struct i2c_client *sgm37864_flashlight_client;

/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *sgm37864_i2c_client;

/* platform data
* torch_pin_enable: TX1/TORCH pin isa hardware TORCH enable
* pam_sync_pin_enable: TX2 Mode The ENVM/TX2 is a PAM Sync. on input
* thermal_comp_mode_enable: LEDI/NTC pin in Thermal Comparator Mode
* strobe_pin_disable: STROBE Input disabled
* vout_mode_enable: Voltage Out Mode enable
*/
struct sgm37864_platform_data {
	u8 torch_pin_enable;
	u8 pam_sync_pin_enable;
	u8 thermal_comp_mode_enable;
	u8 strobe_pin_disable;
	u8 vout_mode_enable;
};

/* sgm37864 chip data */
struct sgm37864_chip_data {
	struct i2c_client *client;
	struct sgm37864_platform_data *pdata;
	struct mutex lock;
	u8 last_flag;
	u8 no_pdata;
};

/******************************************************************************
 * sgm37864 operations
 *****************************************************************************/
static const int sgm37864_current[SGM37864_LEVEL_NUM] = {
         27,   74,  114,  161,  200,  247, 286, 333, 372, 419,
        458,  505,  544,  583,  630,  669, 716, 756, 803, 842,
        889,  928,  975, 1014, 1061, 1100
};

// I(mA) = level*2.11+2.35 level= 0 ~ 30   max = 65.65 mA
// I(mA) = level*1.97+7.33 level= 31 ~ 255
//aw36515 torch current 5, 17, 29, 41, 53, 65,-- 80
static const unsigned char sgm37864_torch_level[SGM37864_LEVEL_NUM] = {
	0x01, 0x07, 0x0D, 0x12, 0x18, 0x1E, 0x25, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// I(mA) = level*8.04+9.45 level= 0 ~ 30  max = 250.65 mA
// I(mA) = level*7.51+28.34 level= 31 ~ 255
static const unsigned char sgm37864_flash_level[SGM37864_LEVEL_NUM] = {
	0x02, 0x08, 0x0D, 0x13, 0x18, 0x1E, 0x22, 0x29, 0x2E, 0x34,
	0x39, 0x3F, 0x45, 0x4A, 0x50, 0x55, 0x5C, 0x61, 0x67, 0x6C,
	0x73, 0x78, 0x7E, 0x83, 0x8A, 0x8F};

static volatile unsigned char sgm37864_reg_enable;
static volatile int sgm37864_level_ch1 = -1;

static int sgm37864_is_torch(int level)
{

	if (level >= SGM37864_LEVEL_TORCH)
		return -1;

	return 0;
}

static int sgm37864_verify_level(int level)
{

	if (level < 0)
		level = 0;
	else if (level >= SGM37864_LEVEL_NUM)
		level = SGM37864_LEVEL_NUM - 1;

	return level;
}

/* i2c wrapper function */
static int sgm37864_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char val)
{
	int ret;
	unsigned char cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_smbus_write_byte_data(client, reg, val);
		if (ret < 0) {
			pr_info("%s: SGM37864 i2c_write addr=0x%02X, data=0x%02X, cnt=%d, error=%d\n",
				   __func__, reg, val, cnt, ret);
		} else {
			break;
		}
		cnt++;
		msleep(AW_I2C_RETRY_DELAY);
	}

	return ret;
}

static int sgm37864_i2c_read(struct i2c_client *client, unsigned char reg, unsigned char *val)
{
	int ret;
	unsigned char cnt = 0;

	while (cnt < AW_I2C_RETRIES) {
		ret = i2c_smbus_read_byte_data(client, reg);
		if (ret < 0) {
			pr_info("%s: SGM37864 i2c_read addr=0x%02X, cnt=%d, error=%d\n",
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

static void sgm37864_soft_reset(void)
{
	unsigned char reg_val;

	sgm37864_i2c_read(sgm37864_i2c_client, SGM37864_REG_BOOST_CONFIG, &reg_val);
	reg_val &= SGM37864_BIT_SOFT_RST_MASK;
	reg_val |= SGM37864_BIT_SOFT_RST_ENABLE;
	sgm37864_i2c_write(sgm37864_i2c_client, SGM37864_REG_BOOST_CONFIG, reg_val);
	msleep(5);
}

/* flashlight enable function */
static int sgm37864_enable_ch1(void)
{
	unsigned char reg, val;

	reg = SGM37864_REG_ENABLE;
	if (!sgm37864_is_torch(sgm37864_level_ch1)) {
		/* torch mode */
		sgm37864_reg_enable |= SGM37864_ENABLE_LED1_TORCH;
	} else {
		/* flash mode */
		sgm37864_reg_enable |= SGM37864_ENABLE_LED1_FLASH;
	}

	val = sgm37864_reg_enable;

	return sgm37864_i2c_write(sgm37864_i2c_client, reg, val);
}

static int sgm37864_enable(int channel)
{

	if (channel == SGM37864_CHANNEL_CH1)
		sgm37864_enable_ch1();
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* flashlight disable function */
static int sgm37864_disable_ch1(void)
{
	unsigned char reg, val;

	reg = SGM37864_REG_ENABLE;
	if (sgm37864_reg_enable & SGM37864_MASK_ENABLE_LED2) {
		/* if LED 2 is enable, disable LED 1 */
		sgm37864_reg_enable &= (~SGM37864_ENABLE_LED1);
	} else {
		/* if LED 2 is enable, disable LED 1 and clear mode */
		sgm37864_reg_enable &= (~SGM37864_ENABLE_LED1_FLASH);
	}
	val = sgm37864_reg_enable;

	return sgm37864_i2c_write(sgm37864_i2c_client, reg, val);
}

static int sgm37864_disable(int channel)
{

	if (channel == SGM37864_CHANNEL_CH1)
		sgm37864_disable_ch1();
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* set flashlight level */
static int sgm37864_set_level_ch1(int level)
{
	int ret;
	unsigned char reg, val;

	level = sgm37864_verify_level(level);
	pr_err("sbuya flashlight sgm37864_set_level_ch1\n");
	/* set torch brightness level */
	reg = SGM37864_REG_TORCH_LEVEL_LED1;
	val = sgm37864_torch_level[level];
	ret = sgm37864_i2c_write(sgm37864_i2c_client, reg, val);

	sgm37864_level_ch1 = level;

	/* set flash brightness level */
	reg = SGM37864_REG_FLASH_LEVEL_LED1;
	val = sgm37864_flash_level[level];
	ret = sgm37864_i2c_write(sgm37864_i2c_client, reg, val);

	return ret;
}

static int sgm37864_set_level(int channel, int level)
{
	if (channel == SGM37864_CHANNEL_CH1)
		sgm37864_set_level_ch1(level);
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* flashlight init */
int sgm37864_init(void)
{
	int ret;
	unsigned char reg, val;

	usleep_range(2000, 2500);

	/* clear enable register */
	reg = SGM37864_REG_ENABLE;
	val = SGM37864_DISABLE;
	ret = sgm37864_i2c_write(sgm37864_i2c_client, reg, val);

	sgm37864_reg_enable = val;

	/* set torch current ramp time and flash timeout */
	reg = SGM37864_REG_TIMING_CONF;
	val = SGM37864_TORCH_RAMP_TIME | SGM37864_FLASH_TIMEOUT;
	ret = sgm37864_i2c_write(sgm37864_i2c_client, reg, val);

	return ret;
}

/* flashlight uninit */
int sgm37864_uninit(void)
{
	sgm37864_disable(SGM37864_CHANNEL_CH1);

	return 0;
}


/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer sgm37864_timer_ch1;
static unsigned int sgm37864_timeout_ms[SGM37864_CHANNEL_NUM];

static void sgm37864_work_disable_ch1(struct work_struct *data)
{
	pr_info("ht work queue callback\n");
	sgm37864_disable_ch1();
}



static enum hrtimer_restart sgm37864_timer_func_ch1(struct hrtimer *timer)
{
	schedule_work(&sgm37864_work_ch1);
	return HRTIMER_NORESTART;
}

int sgm37864_timer_start(int channel, ktime_t ktime)
{
	if (channel == SGM37864_CHANNEL_CH1)
		hrtimer_start(&sgm37864_timer_ch1, ktime, HRTIMER_MODE_REL);
	else {
		pr_err("SGM37864 Error channel\n");
		return -1;
	}

	return 0;
}

int sgm37864_timer_cancel(int channel)
{
	if (channel == SGM37864_CHANNEL_CH1)
		hrtimer_cancel(&sgm37864_timer_ch1);
	else {
		pr_err("SGM37864 Error channel\n");
		return -1;
	}

	return 0;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int sgm37864_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	/* verify channel */
	if (channel < 0 || channel >= SGM37864_CHANNEL_NUM) {
		pr_err("SGM37864 Failed with error channel\n");
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_info("SGM37864 FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		sgm37864_timeout_ms[channel] = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_info("SGM37864 FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		sgm37864_set_level(channel, fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_info("SGM37864 FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (sgm37864_timeout_ms[channel]) {
				ktime =
				ktime_set(sgm37864_timeout_ms[channel] / 1000,
				(sgm37864_timeout_ms[channel] % 1000) * 1000000);
				sgm37864_timer_start(channel, ktime);
			}
			sgm37864_enable(channel);
		} else {
			sgm37864_disable(channel);
			sgm37864_timer_cancel(channel);
		}
		break;
	case FLASH_IOC_GET_DUTY_NUMBER:
		pr_info("FLASH_IOC_GET_DUTY_NUMBER\n");
		fl_arg->arg = SGM37864_LEVEL_NUM;
		break;
	case FLASH_IOC_GET_MAX_TORCH_DUTY:
		pr_info("FLASH_IOC_GET_MAX_TORCH_DUTY(%d)\n", channel);
		fl_arg->arg = SGM37864_LEVEL_TORCH - 1;
		break;

	case FLASH_IOC_GET_DUTY_CURRENT:
		fl_arg->arg = sgm37864_verify_level(fl_arg->arg);
		pr_info("FLASH_IOC_GET_DUTY_CURRENT(%d): %d\n",
				channel, (int)fl_arg->arg);
		fl_arg->arg = sgm37864_current[fl_arg->arg];
		break;

	case FLASH_IOC_GET_HW_TIMEOUT:
		pr_info("FLASH_IOC_GET_HW_TIMEOUT(%d)\n", channel);
		fl_arg->arg = 1600;
		break;

	default:
		pr_err("SGM37864 No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int sgm37864_open(void)
{
	/* Actual behavior move to set driver function */
	/* since power saving issue */
	return 0;
}

static int sgm37864_release(void)
{
	/* uninit chip and clear usage count */
	mutex_lock(&sgm37864_mutex);
	use_count--;
	if (!use_count)
		sgm37864_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&sgm37864_mutex);

	pr_info("SGM37864 Release: %d\n", use_count);

	return 0;
}

static int sgm37864_set_driver(int set)
{
	/* init chip and set usage count */
	mutex_lock(&sgm37864_mutex);
	if (!use_count)
		sgm37864_init();
	use_count++;
	mutex_unlock(&sgm37864_mutex);

	pr_info("Set SGM37864 driver: %d\n", use_count);

	return 0;
}

static ssize_t sgm37864_strobe_store(struct flashlight_arg arg)
{
	sgm37864_set_driver(1);
	sgm37864_set_level(arg.ct, arg.level);
	sgm37864_enable(arg.ct);
	msleep(arg.dur);
	sgm37864_disable(arg.ct);
	sgm37864_set_driver(0);

	return 0;
}

static struct flashlight_operations sgm37864_ops = {
	sgm37864_open,
	sgm37864_release,
	sgm37864_ioctl,
	sgm37864_strobe_store,
	sgm37864_set_driver
};


/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int sgm37864_chip_init(struct sgm37864_chip_data *chip)
{
	/* NOTE: Chip initialication move to
	*"set driver" operation for power saving issue.
	* sgm37864_init();
	*/

	return 0;
}

/***************************************************************************/
/*SGM37864 Debug file */
/***************************************************************************/
static ssize_t
sgm37864_get_reg(struct device *cd, struct device_attribute *attr, char *buf)
{
	unsigned char reg_val;
	unsigned char i;
	ssize_t len = 0;

	for (i = 0; i < 0x0E; i++) {
		sgm37864_i2c_read(sgm37864_i2c_client, i, &reg_val);
		len += snprintf(buf+len, PAGE_SIZE-len,
			"reg0x%2X = 0x%2X\n", i, reg_val);
	}
	len += snprintf(buf+len, PAGE_SIZE-len, "\r\n");
	return len;
}

static ssize_t sgm37864_set_reg(struct device *cd,
		struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int databuf[2];

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2)
		sgm37864_i2c_write(sgm37864_i2c_client, databuf[0], databuf[1]);
	return len;
}

static DEVICE_ATTR(reg, 0660, sgm37864_get_reg, sgm37864_set_reg);

static int sgm37864_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	err = device_create_file(dev, &dev_attr_reg);

	return err;
}

static int
sgm37864_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sgm37864_chip_data *chip;
	struct sgm37864_platform_data *pdata = client->dev.platform_data;
	int err;
	unsigned char device_id;
	int dev_id;

	pr_info("%s Probe SGM37864 start.\n", __func__);

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("SGM37864 Failed to check i2c functionality.\n");
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct sgm37864_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	/* init platform data */
	if (!pdata) {
		pr_err("SGM37864 Platform data does not exist\n");
		pdata =
		kzalloc(sizeof(struct sgm37864_platform_data), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err_init_pdata;
		}
		chip->no_pdata = 1;
	}
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);
	sgm37864_i2c_client = client;
	pr_info("%s Probe SGM37864 start.\n", __func__,sgm37864_i2c_client->addr);

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* soft rst */
	sgm37864_soft_reset();

	/* init work queue */
	INIT_WORK(&sgm37864_work_ch1, sgm37864_work_disable_ch1);

	/* init timer */
	hrtimer_init(&sgm37864_timer_ch1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	sgm37864_timer_ch1.function = sgm37864_timer_func_ch1;
	sgm37864_timeout_ms[SGM37864_CHANNEL_CH1] = 100;

	/* init chip hw */
	sgm37864_chip_init(chip);

	dev_id = sgm37864_i2c_read(sgm37864_i2c_client, SGM37864_REG_DEVICES_ID, &device_id);
	pr_info("%s SGM37864_DEVICES_ID 0x%2x\n", __func__, dev_id);

	if (dev_id != SGM37864_VER_DEVICES_ID) {
		pr_info("%s is not sgm37864 flashlight", __func__);
		err = -ENODEV;
		goto err_free;
	}

	/* register flashlight operations */
	if (flashlight_dev_register(SBUYA_SGM37864_NAME, &sgm37864_ops)) {
		pr_err("SGM37864 Failed to register flashlight device.\n");
		err = -EFAULT;
		goto err_free;
	}

	/* clear usage count */
	use_count = 0;

	sgm37864_create_sysfs(client);

	pr_info("%s Probe SGM37864 done.\n", __func__);

	return 0;

err_free:
	kfree(chip->pdata);
err_init_pdata:
	i2c_set_clientdata(client, NULL);
	kfree(chip);
err_out:
	return err;
}

static int sgm37864_i2c_remove(struct i2c_client *client)
{
	struct sgm37864_chip_data *chip = i2c_get_clientdata(client);

	pr_info("SGM37864 Remove start.\n");

	/* flush work queue */
	flush_work(&sgm37864_work_ch1);

	/* unregister flashlight operations */
	flashlight_dev_unregister(SBUYA_SGM37864_NAME);

	/* free resource */
	if (chip->no_pdata)
		kfree(chip->pdata);
	kfree(chip);

	pr_info("SGM37864 Remove done.\n");

	return 0;
}

static void sgm37864_i2c_shutdown(struct i2c_client *client)
{
	pr_info("sgm37864 shutdown start.\n");

	sgm37864_i2c_write(sgm37864_i2c_client, SGM37864_REG_ENABLE,
						SGM37864_CHIP_STANDBY);

	sgm37864_disable(SGM37864_CHANNEL_CH1);
	sgm37864_timer_cancel(SGM37864_CHANNEL_CH1);

	pr_info("sgm37864 shutdown done.\n");
}

static const struct i2c_device_id sgm37864_i2c_id[] = {
	{SBUYA_SGM37864_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id sgm37864_i2c_of_match[] = {
	{.compatible = SBUYA_SGM37864_DTNAME_I2C},
	{},
};
#endif

static struct i2c_driver sgm37864_i2c_driver = {
	.driver = {
		   .name = SBUYA_SGM37864_NAME,
#ifdef CONFIG_OF
		   .of_match_table = sgm37864_i2c_of_match,
#endif
		   },
	.probe = sgm37864_i2c_probe,
	.remove = sgm37864_i2c_remove,
	.shutdown = sgm37864_i2c_shutdown,
	.id_table = sgm37864_i2c_id,
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int sgm37864_probe(struct platform_device *dev)
{
	pr_info("%s Probe SGM37864 start.\n", __func__);

	if (i2c_add_driver(&sgm37864_i2c_driver)) {
		pr_err("SGM37864 Failed to add i2c driver.\n");
		return -1;
	}

	pr_info("%s Probe SGM37864 done.\n", __func__);

	return 0;
}

static int sgm37864_remove(struct platform_device *dev)
{
	pr_info("SGM37864 Remove start.\n");

	i2c_del_driver(&sgm37864_i2c_driver);

	pr_info("SGM37864 Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id sgm37864_of_match[] = {
	{.compatible = SBUYA_SGM37864_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, sgm37864_of_match);
#else
static struct platform_device sgm37864_platform_device[] = {
	{
		.name = SBUYA_SGM37864_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, sgm37864_platform_device);
#endif

static struct platform_driver sgm37864_platform_driver = {
	.probe = sgm37864_probe,
	.remove = sgm37864_remove,
	.driver = {
		.name = SBUYA_SGM37864_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = sgm37864_of_match,
#endif
	},
};

static int __init flashlight_sgm37864_init(void)
{
	int ret;

	pr_info("%s sgm37864 driver version %s.\n", __func__, SGM37864_DRIVER_VERSION);

#ifndef CONFIG_OF
	ret = platform_device_register(&sgm37864_platform_device);
	if (ret) {
		pr_err("SGM37864 Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&sgm37864_platform_driver);
	if (ret) {
		pr_err("SGM37864 Failed to register platform driver\n");
		return ret;
	}

	pr_info("flashlight_sgm37864 Init done.\n");

	return 0;
}

static void __exit flashlight_sgm37864_exit(void)
{
	pr_info("flashlight_sgm37864-Exit start.\n");

	platform_driver_unregister(&sgm37864_platform_driver);

	pr_info("flashlight_sgm37864 Exit done.\n");
}

module_init(flashlight_sgm37864_init);
module_exit(flashlight_sgm37864_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Awinic");
MODULE_DESCRIPTION("SGM37864 Flashlight Driver");

