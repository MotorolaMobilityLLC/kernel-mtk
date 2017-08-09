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
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_flashlight_type.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>

/*#include <cust_gpio_usage.h>*/
#include <mt_gpio.h>
#include <gpio_const.h>
/******************************************************************************
 * GPIO configuration
******************************************************************************/
#define GPIO_CAMERA_FLASH_EN_PIN			(GPIO90 | 0x80000000)
#define GPIO_CAMERA_FLASH_EN_PIN_M_CLK		GPIO_MODE_03
#define GPIO_CAMERA_FLASH_EN_PIN_M_EINT		GPIO_MODE_01
#define GPIO_CAMERA_FLASH_EN_PIN_M_GPIO		GPIO_MODE_00
#define GPIO_CAMERA_FLASH_EN_PIN_CLK		CLK_OUT1
#define GPIO_CAMERA_FLASH_EN_PIN_FREQ		GPIO_CLKSRC_NONE

/******************************************************************************
 * Debug configuration
******************************************************************************/

#define TAG_NAME "[leds_strobe.c]"
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)

/*#define DEBUG_LEDS_STROBE*/
#ifdef DEBUG_LEDS_STROBE
	#define PK_DBG PK_DBG_FUNC
#else
	#define PK_DBG(a, ...)
#endif

/******************************************************************************
 * local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */


static u32 strobe_Res;
static u32 strobe_Timeus;
static BOOL g_strobe_On;

static int g_timeOutTimeMs;

static DEFINE_MUTEX(g_strobeSem);


#define STROBE_DEVICE_ID 0xC6


static struct work_struct workTimeOut;

/* #define FLASH_GPIO_ENF GPIO12 */
/* #define FLASH_GPIO_ENT GPIO13 */
#define FLASH_GPIO_EN GPIO_CAMERA_FLASH_EN_PIN

/*****************************************************************************
Functions
*****************************************************************************/
static void work_timeOutFunc(struct work_struct *data);

static struct i2c_client *LM3643_i2c_client;




struct LM3643_platform_data {
	u8 torch_pin_enable;	/* 1:  TX1/TORCH pin isa hardware TORCH enable */
	u8 pam_sync_pin_enable;	/* 1:  TX2 Mode The ENVM/TX2 is a PAM Sync. on input */
	u8 thermal_comp_mode_enable;	/* 1: LEDI/NTC pin in Thermal Comparator Mode */
	u8 strobe_pin_disable;	/* 1 : STROBE Input disabled */
	u8 vout_mode_enable;	/* 1 : Voltage Out Mode enable */
};

struct LM3643_chip_data {
	struct i2c_client *client;

	/* struct led_classdev cdev_flash; */
	/* struct led_classdev cdev_torch; */
	/* struct led_classdev cdev_indicator; */

	struct LM3643_platform_data *pdata;
	struct mutex lock;

	u8 last_flag;
	u8 no_pdata;
};

static int i2c_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret = 0;
	struct LM3643_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);

	if (ret < 0)
		PK_DBG("failed writing at 0x%02x\n", reg);
	return ret;
}
/*
static int i2c_read_reg(struct i2c_client *client, u8 reg)
{
	int val=0;
	struct LM3643_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	val = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&chip->lock);


	return val;
}
*/



static int LM3643_chip_init(struct LM3643_chip_data *chip)
{


	return 0;
}

static int LM3643_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct LM3643_chip_data *chip;
	struct LM3643_platform_data *pdata = client->dev.platform_data;

	int err = -1;

	PK_DBG("LM3643_probe start--->.\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		PK_DBG("LM3643 i2c functionality check fail.\n");
		return err;
	}

	chip = kzalloc(sizeof(struct LM3643_chip_data), GFP_KERNEL);
	chip->client = client;

	mutex_init(&chip->lock);
	i2c_set_clientdata(client, chip);

	if (pdata == NULL) {	/* values are set to Zero. */
		PK_DBG("LM3643 Platform data does not exist\n");
		pdata = kzalloc(sizeof(struct LM3643_platform_data), GFP_KERNEL);
		chip->pdata = pdata;
		chip->no_pdata = 1;
	}

	chip->pdata = pdata;
	if (LM3643_chip_init(chip) < 0)
		goto err_chip_init;

	LM3643_i2c_client = client;
	PK_DBG("LM3643 Initializing is done\n");

	return 0;

err_chip_init:
	i2c_set_clientdata(client, NULL);
	kfree(chip);
	PK_DBG("LM3643 probe is failed\n");
	return -ENODEV;
}

static int LM3643_remove(struct i2c_client *client)
{
	struct LM3643_chip_data *chip = i2c_get_clientdata(client);

	if (chip->no_pdata)
		kfree(chip->pdata);
	kfree(chip);
	return 0;
}


#define LM3643_NAME "leds-LM3643"
static const struct i2c_device_id LM3643_id[] = {
	{LM3643_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id LM3643_of_match[] = {
	{.compatible = "mediatek,strobe_main"},
	{},
};
#endif

static struct i2c_driver LM3643_i2c_driver = {
	.driver = {
		   .name = LM3643_NAME,
#ifdef CONFIG_OF
		   .of_match_table = LM3643_of_match,
#endif
		   },
	.probe = LM3643_probe,
	.remove = LM3643_remove,
	.id_table = LM3643_id,
};
static int __init LM3643_init(void)
{
	PK_DBG("LM3643_init\n");
	return i2c_add_driver(&LM3643_i2c_driver);
}

static void __exit LM3643_exit(void)
{
	i2c_del_driver(&LM3643_i2c_driver);
}


module_init(LM3643_init);
module_exit(LM3643_exit);

MODULE_DESCRIPTION("Flash driver for LM3643");
MODULE_AUTHOR("pw <pengwei@mediatek.com>");
MODULE_LICENSE("GPL v2");


/*****************************************************************************
Dual-flash functions
*****************************************************************************/
enum
{
	e_DutyNum = 26,
};

static bool g_IsTorch[26] =	{ 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
				  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				  0, 0, 0, 0, 0, 0 };
static char g_TorchDutyCode[26] =	{ 0x0F, 0x20, 0x31, 0x42, 0x52, 0x63,
					  0x74, 0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, 0x00 };

static char g_FlashDutyCode[26] =	{ 0x01, 0x03, 0x05, 0x07, 0x09, 0x0B,
					  0x0D, 0x10, 0x14, 0x19, 0x1D, 0x21,
					  0x25, 0x2A, 0x2E, 0x32, 0x37, 0x3B,
					  0x3F, 0x43, 0x48, 0x4C, 0x50, 0x54,
					  0x59, 0x5D };
static char g_EnableReg;
static int g_duty1 = -1;
static int g_duty2 = -1;

static int LM3643_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	return i2c_write_reg(client, reg, val);
}
/*
static int LM3643_read_reg(struct i2c_client *client, u8 reg)
{
	int val;

	val = i2c_read_reg(client, reg);
	PK_DBG("LM3643_read_reg reg=%d(0x%x) val=%d(0x%x)\n", reg, reg, val, val);
	return 0;
}
*/
int flashEnable_lm3643_1(void)
{
	int ret;
	char buf[2];

	buf[0] = 0x01; /* Enable Register */
	if (g_IsTorch[g_duty1] == 1) /* LED1 in torch mode */
		g_EnableReg |= (0x09);
	else
		g_EnableReg |= (0x0D);
	buf[1] = g_EnableReg;
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);
	return ret;
}

int flashEnable_lm3643_2(void)
{
	int ret;
	char buf[2];

	buf[0] = 0x01; /* Enable Register */
	if (g_IsTorch[g_duty2] == 1) /* LED2 in torch mode */
		g_EnableReg |= (0x0A);
	else
		g_EnableReg |= (0x0E);
	buf[1] = g_EnableReg;
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);
	return ret;
}

int flashDisable_lm3643_1(void)
{
	int ret;
	char buf[2];

	buf[0] = 0x01; /* Enable Register */
	if ((g_EnableReg&0x02) == 0x02) /* LED2 enabled */
		g_EnableReg &= (~0x01);
	else
		g_EnableReg &= (~0x0D);
	buf[1] = g_EnableReg;
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);
	return ret;
}

int flashDisable_lm3643_2(void)
{
	int ret;
	char buf[2];

	buf[0] = 0x01; /* Enable Register */
	if ((g_EnableReg&0x01) == 0x01) /* LED1 enabled */
		g_EnableReg &= (~0x02);
	else
		g_EnableReg &= (~0x0E);
	buf[1] = g_EnableReg;
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);
	return ret;
}

int setDuty_lm3643_1(int duty)
{
	int ret;
	char buf[2];

	if (duty < 0)
		duty = 0;
	else if (duty >= e_DutyNum)
		duty = e_DutyNum-1;

	g_duty1 = duty;
	buf[0] = 0x05; /* LED1 Torch Brightness Register */
	buf[1] = g_TorchDutyCode[duty];
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);

	buf[0] = 0x03; /* LED1 Flash Brightness Register */
	buf[1] = g_FlashDutyCode[duty];
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);
	return ret;
}

int setDuty_lm3643_2(int duty)
{
	int ret;
	char buf[2];

	if (duty < 0)
		duty = 0;
	else if (duty >= e_DutyNum)
		duty = e_DutyNum-1;

	g_duty2 = duty;
	buf[0] = 0x06; /* LED2 Torch Brightness Register */
	buf[1] = g_TorchDutyCode[duty];
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);

	buf[0] = 0x04; /* LED2 Flash Brightness Register */
	buf[1] = g_FlashDutyCode[duty];
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);
	return ret;
}

int init_lm3643(void)
{
	int ret;
	char buf[2];

	flashlight_gpio_set(FLASHLIGHT_PIN_HWEN, STATE_HIGH);

	buf[0] = 0x01; /* Enable Register */
	buf[1] = 0x00;
	g_EnableReg = buf[1];
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);

	buf[0] = 0x03; /* LED1 Flash Brightness Register */
	buf[1] = 0x3F;
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);

	buf[0] = 0x05; /* LED2 Flash Brightness Register */
	buf[1] = 0x3F;
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);

	buf[0] = 0x08; /* Timing Configuration Register */
	buf[1] = 0x1F;
	ret = LM3643_write_reg(LM3643_i2c_client, buf[0], buf[1]);
	return ret;
}

int FL_Enable(void)
{
	PK_DBG(" FL_Enable line=%d\n", __LINE__);
	return flashEnable_lm3643_1();
}



int FL_Disable(void)
{
	PK_DBG(" FL_Disable line=%d\n", __LINE__);
	return flashDisable_lm3643_1();
}

int FL_dim_duty(kal_uint32 duty)
{
	PK_DBG(" FL_dim_duty line=%d\n", __LINE__);
	return setDuty_lm3643_1(duty);
}

int FL_Init(void)
{
	PK_DBG(" FL_Init line=%d\n", __LINE__);
	return init_lm3643();
}

int FL_Uninit(void)
{
	FL_Disable();
	flashlight_gpio_set(FLASHLIGHT_PIN_HWEN, STATE_LOW);
	return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
	FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}



enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}
static struct hrtimer g_timeOutTimer;
void timerInit(void)
{
	static int init_flag;

	if (init_flag == 0) {
		init_flag = 1;
		INIT_WORK(&workTimeOut, work_timeOutFunc);
		g_timeOutTimeMs = 1000;
		hrtimer_init(&g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		g_timeOutTimer.function = ledTimeOutCallback;
	}
}



static int constant_flashlight_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;

	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC, 0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC, 0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC, 0, int));
/*	PK_DBG
	    ("LM3643 constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",
	     __LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);
*/
	switch (cmd) {

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n", (int)arg);
		g_timeOutTimeMs = arg;
		break;


	case FLASH_IOC_SET_DUTY:
		PK_DBG("FLASHLIGHT_DUTY: %d\n", (int)arg);
		FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n", (int)arg);

		break;

	case FLASH_IOC_SET_ONOFF:
		PK_DBG("FLASHLIGHT_ONOFF: %d\n", (int)arg);
		if (arg == 1) {

			int s;
			int ms;

			if (g_timeOutTimeMs > 1000) {
				s = g_timeOutTimeMs / 1000;
				ms = g_timeOutTimeMs - s * 1000;
			} else {
				s = 0;
				ms = g_timeOutTimeMs;
			}

			if (g_timeOutTimeMs != 0) {
				ktime_t ktime;

				ktime = ktime_set(s, ms * 1000000);
				hrtimer_start(&g_timeOutTimer, ktime, HRTIMER_MODE_REL);
			}
			FL_Enable();
		} else {
			FL_Disable();
			hrtimer_cancel(&g_timeOutTimer);
		}
		break;
	default:
		PK_DBG(" No such command\n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
}




static int constant_flashlight_open(void *pArg)
{
	int i4RetValue = 0;

	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res) {
		FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if (strobe_Res) {
		PK_DBG(" busy!\n");
		i4RetValue = -EBUSY;
	} else {
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;

}


static int constant_flashlight_release(void *pArg)
{
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res) {
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;

}


FLASHLIGHT_FUNCTION_STRUCT constantFlashlightFunc = {
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};


MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &constantFlashlightFunc;
	return 0;
}



/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{

	return 0;
}
EXPORT_SYMBOL(strobe_VDIrq);
