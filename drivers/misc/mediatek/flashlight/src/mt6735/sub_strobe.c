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

#if defined(CONFIG_LCT_CAMERA_KERNEL)/*jijin.wang add for LCT*/
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
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#ifdef CONFIG_COMPAT
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include <linux/delay.h>
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

#else

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
#include "kd_camera_typedef.h"
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
/* availible parameter */
/* ANDROID_LOG_ASSERT */
/* ANDROID_LOG_ERROR */
/* ANDROID_LOG_WARNING */
/* ANDROID_LOG_INFO */
/* ANDROID_LOG_DEBUG */
/* ANDROID_LOG_VERBOSE */
#define TAG_NAME "[sub_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_WARN(fmt, arg...)        pr_warn(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_NOTICE(fmt, arg...)      pr_notice(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_INFO(fmt, arg...)        pr_info(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_TRC_FUNC(f)              pr_debug(TAG_NAME "<%s>\n", __func__)
#define PK_TRC_VERBOSE(fmt, arg...) pr_debug(TAG_NAME fmt, ##arg)
#define PK_ERROR(fmt, arg...)       pr_err(TAG_NAME "%s: " fmt, __func__ , ##arg)

#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_VER PK_TRC_VERBOSE
#define PK_ERR PK_ERROR
#else
#define PK_DBG(a, ...)
#define PK_VER(a, ...)
#define PK_ERR(a, ...)
#endif


#endif
#if defined(CONFIG_LCT_CAMERA_KERNEL)/*jijin.wang add for LCT*/
static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */
static u32 strobe_Res;
static u32 strobe_Timeus;
static bool g_strobe_On;
static int g_timeOutTimeMs;
static struct work_struct workTimeOut;
static void work_timeOutFunc(struct work_struct *data);
static void open_AM3640_pluse(void);
enum
{
	e_DutyNum = 17,
};

static int g_duty1 = -1;
/***************************************************************
*open_AM3640_pluse jijin.wang add for AW3640
*duty2 is pluse num(0~15)
*ILED is LED current
*ILED = 12.5(16 - duty2)
****************************************************************/
void open_AM3640_pluse(void){
	int i = 0,duty2 = 11;
	for(i = 0;i< duty2;i++){
		flashlight_gpio_set(SUB_FLASHLIGHT_ENF_PIN,STATE_HIGH);
		udelay(1);
		flashlight_gpio_set(SUB_FLASHLIGHT_ENF_PIN,STATE_LOW);	
		udelay(1);
	}
	flashlight_gpio_set(SUB_FLASHLIGHT_ENF_PIN,STATE_HIGH);
}
//static int g_duty2 = -1;
#if 1//wangjijin add for sub flashlight node
int FL_set_subflashlight(unsigned int onoff)
{
	PK_DBG("<%s:%d>%d\n", __func__, __LINE__, onoff);
	if(onoff){
		open_AM3640_pluse();
		//flashlight_gpio_set(SUB_FLASHLIGHT_ENF_PIN,STATE_HIGH);
	}else{
		flashlight_gpio_set(SUB_FLASHLIGHT_ENF_PIN,STATE_LOW);
	}
	return 0;
}
EXPORT_SYMBOL(FL_set_subflashlight);
#endif 
int SUB_FL_Enable(void)
{
	open_AM3640_pluse();
	//flashlight_gpio_set(SUB_FLASHLIGHT_ENF_PIN,STATE_HIGH);
	return 0;
}

int SUB_FL_Disable(void)
{
	flashlight_gpio_set(SUB_FLASHLIGHT_ENF_PIN,STATE_LOW);
	return 0;
}

int SUB_FL_dim_duty(u32 duty)
{
	g_duty1=duty;
	PK_DBG(" SUB_FL_dim_duty line=%d\n", __LINE__);
	return 0;
}

int SUB_FL_Init(void)
{
	PK_DBG("SUB_FL_Init line = %d\n",__LINE__);
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	return 0;
}

int SUB_FL_Uninit(void)
{
	SUB_FL_Disable();
	return 0;
}


/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
	SUB_FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}



enum hrtimer_restart sub_ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}
static struct hrtimer g_timeOutTimer;
void SUB_timerInit(void)
{
	static int init_flag;

	if (init_flag == 0) {
		init_flag = 1;
		INIT_WORK(&workTimeOut, work_timeOutFunc);
		g_timeOutTimeMs = 1000;
		hrtimer_init(&g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		g_timeOutTimer.function = sub_ledTimeOutCallback;
	}
}
#endif

static int sub_strobe_ioctl(unsigned int cmd, unsigned long arg)
{
#if defined(CONFIG_LCT_CAMERA_KERNEL)/*jijin.wang add for LCT*/
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;

	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC, 0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC, 0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC, 0, int));
	switch (cmd) {

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n", (int)arg);
		g_timeOutTimeMs = arg;
		break;


	case FLASH_IOC_SET_DUTY:
		PK_DBG("FLASHLIGHT_DUTY: %d\n", (int)arg);
		SUB_FL_dim_duty(arg);
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
			SUB_FL_Enable();
		} else {
			SUB_FL_Disable();
			hrtimer_cancel(&g_timeOutTimer);
		}
		break;
	default:
		PK_DBG(" No such command\n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
#else
	PK_DBG("sub dummy ioctl");
	return 0;
#endif
}

static int sub_strobe_open(void *pArg)
{
#if defined(CONFIG_LCT_CAMERA_KERNEL)/*jijin.wang add for LCT*/
	int i4RetValue = 0;

	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res) {
		SUB_FL_Init();
		SUB_timerInit();
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
#else
	PK_DBG("sub dummy open");
	return 0;
#endif
}

static int sub_strobe_release(void *pArg)
{
#if defined(CONFIG_LCT_CAMERA_KERNEL)/*jijin.wang add for LCT*/
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res) {
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = false;

		spin_unlock_irq(&g_strobeSMPLock);

		SUB_FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;
#else
	PK_DBG("sub dummy release");
	return 0;
#endif
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
