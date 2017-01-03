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
#include <linux/errno.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_flashlight_type.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#ifdef CONFIG_COMPAT
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include "kd_flashlight.h"
/******************************************************************************
 * Debug configuration
******************************************************************************/
#define TAG_NAME "[sub_strobe.c]"
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)

#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#else
#define PK_DBG(a, ...)
#endif

#include <mt_gpio.h>
#include <gpio_const.h>
/******************************************************************************
 * GPIO configuration
******************************************************************************/
#define GPIO_CAMERA_FRONT_FLASH_EN_PIN			(GPIO49 | 0x80000000)
#define GPIO_CAMERA_FRONT_FLASH_EN_PIN_M_CLK		GPIO_MODE_03
#define GPIO_CAMERA_FRONT_FLASH_EN_PIN_M_EINT		GPIO_MODE_01
#define GPIO_CAMERA_FRONT_FLASH_EN_PIN_M_GPIO		GPIO_MODE_00
#define GPIO_CAMERA_FRONT_FLASH_EN_PIN_CLK		CLK_OUT1
#define GPIO_CAMERA_FRONT_FLASH_EN_PIN_FREQ		GPIO_CLKSRC_NONE

static struct work_struct workTimeOut;

#define FLASH_GPIO_EN GPIO_CAMERA_FRONT_FLASH_EN_PIN
/*****************************************************************************
Functions
*****************************************************************************/
static void work_timeOutFunc(struct work_struct *data);

/******************************************************************************
 * local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */
static u32 strobe_Res;
static int g_timeOutTimeMs;
static DEFINE_MUTEX(g_strobeSem);

static int flashEnable_wd3100(void)
{
	int ret = 0;

	PK_DBG(" flashEnable_wd3100 line=%d\n", __LINE__);
	mt_set_gpio_mode(FLASH_GPIO_EN, 0);
	mt_set_gpio_dir(FLASH_GPIO_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(FLASH_GPIO_EN, 1);
	//flashlight_gpio_set(FLASH_GPIO_EN, STATE_HIGH);
	return ret;
}

static int flashDisable_wd3100(void)
{
	int ret = 0;

	PK_DBG(" flashDisable_wd3100 line=%d\n", __LINE__);
	mt_set_gpio_mode(FLASH_GPIO_EN, 0);
	mt_set_gpio_dir(FLASH_GPIO_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(FLASH_GPIO_EN, 0);
	//flashlight_gpio_set(FLASH_GPIO_EN, STATE_LOW);
	return ret;
}

static int FL_Enable(void)
{
	flashEnable_wd3100();
	return 0;
}

static int FL_Disable(void)
{
	flashDisable_wd3100();
	return 0;
}

static int FL_Init(void)
{
	PK_DBG(" FL_Init line=%d\n", __LINE__);
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	flashDisable_wd3100();
	return 0;
}

static int FL_Uninit(void)
{
	FL_Disable();
	return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/
static struct hrtimer g_timeOutTimer;

static int g_b1stInit = 1;

static void work_timeOutFunc(struct work_struct *data)
{
	//FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}

static enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

static void timerInit(void)
{
	if (g_b1stInit == 1) {
		g_b1stInit = 0;

		INIT_WORK(&workTimeOut, work_timeOutFunc);
		g_timeOutTimeMs = 1000;
		hrtimer_init(&g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		g_timeOutTimer.function = ledTimeOutCallback;
	}
}
static int sub_strobe_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	/* kal_uint8 valTemp; */
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC, 0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC, 0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC, 0, int));
	PK_DBG
	    ("constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",
	     __LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);
	switch (cmd) {

	case FLASH_IOC_SET_ONOFF:
		PK_DBG("FLASHLIGHT_ONOFF: %d\n", (int)arg);
		if (arg == 1) {
			if (g_timeOutTimeMs != 0) {
				ktime_t ktime;

				ktime = ktime_set(0, g_timeOutTimeMs * 1000000);
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

static int sub_strobe_open(void *pArg)
{
	int i4RetValue = 0;

	PK_DBG("sub flash open line=%d\n", __LINE__);
	if (0 == strobe_Res) {
		FL_Init();
		timerInit();
	}
	PK_DBG("sub flash open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);

	if (strobe_Res) {
		PK_DBG(" busy!\n");
		i4RetValue = -EBUSY;
	} else {
		strobe_Res += 1;
	}

	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("sub flash  open line=%d\n", __LINE__);

	return i4RetValue;

}

static int sub_strobe_release(void *pArg)
{
	PK_DBG("sub flash release");
	if (strobe_Res) {
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		spin_unlock_irq(&g_strobeSMPLock);
		FL_Uninit();
	}
	PK_DBG(" Done\n");
	return 0;
}

FLASHLIGHT_FUNCTION_STRUCT subStrobeFunc = {
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};


MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &subStrobeFunc;
	return 0;
}
