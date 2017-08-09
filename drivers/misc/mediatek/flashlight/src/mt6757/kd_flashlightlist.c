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

#ifdef WIN32
#include "win_test.h"
#include "stdio.h"
#include "kd_flashlight.h"
#else
#ifdef CONFIG_COMPAT

#include <linux/fs.h>
#include <linux/compat.h>

#endif

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
#include "kd_flashlight_type.h"
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/upmu_sw.h>
#endif
#ifdef CONFIG_COMPAT
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include "kd_flashlight.h"



/******************************************************************************
 * Definition
******************************************************************************/

/* device name and major number */
#define FLASHLIGHT_DEVNAME            "kd_camera_flashlight"

/******************************************************************************
 * Debug configuration
******************************************************************************/
#ifdef WIN32
#define logI(fmt, ...)    {printf(fmt, __VA_ARGS__); printf("\n"); }
#else
#define PFX "[KD_CAMERA_FLASHLIGHT]"
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(PFX "%s: " fmt, __func__ , ##arg)

/*#define DEBUG_KD_STROBE*/
#ifdef DEBUG_KD_STROBE
#define logI PK_DBG_FUNC
#else
#define logI(a, ...)
#endif
#endif

#define POWER_THROTTLING 1
#define DLPT_FEATURE 1

#if DLPT_FEATURE
#include <mach/mt_pbm.h>
#endif
/* ============================== */
/* variables */
/* ============================== */
static FLASHLIGHT_FUNCTION_STRUCT
	*g_pFlashInitFunc[e_Max_Sensor_Dev_Num][e_Max_Strobe_Num_Per_Dev][e_Max_Part_Num_Per_Dev];
static int gLowBatDuty[e_Max_Sensor_Dev_Num][e_Max_Strobe_Num_Per_Dev];
static int g_strobePartId[e_Max_Sensor_Dev_Num][e_Max_Strobe_Num_Per_Dev];
static DEFINE_MUTEX(g_mutex);
/* ============================== */
/* Pinctrl */
/* ============================== */
static struct pinctrl *flashlight_pinctrl;
static struct pinctrl_state *flashlight_hwen_high;
static struct pinctrl_state *flashlight_hwen_low;
static struct pinctrl_state *flashlight_torch_high;
static struct pinctrl_state *flashlight_torch_low;
static struct pinctrl_state *flashlight_flash_high;
static struct pinctrl_state *flashlight_flash_low;

int flashlight_gpio_init(struct platform_device *pdev)
{
	int ret = 0;

	flashlight_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(flashlight_pinctrl)) {
		logI("Cannot find flashlight pinctrl!");
		ret = PTR_ERR(flashlight_pinctrl);
	}
	/* Flashlight HWEN pin initialization */
	flashlight_hwen_high = pinctrl_lookup_state(flashlight_pinctrl, "hwen_high");
	if (IS_ERR(flashlight_hwen_high)) {
		ret = PTR_ERR(flashlight_hwen_high);
		logI("%s : init err, flashlight_hwen_high\n", __func__);
	}

	flashlight_hwen_low = pinctrl_lookup_state(flashlight_pinctrl, "hwen_low");
	if (IS_ERR(flashlight_hwen_low)) {
		ret = PTR_ERR(flashlight_hwen_low);
		logI("%s : init err, flashlight_hwen_low\n", __func__);
	}

	/* Flashlight TORCH pin initialization */
	flashlight_torch_high = pinctrl_lookup_state(flashlight_pinctrl, "torch_high");
	if (IS_ERR(flashlight_torch_high)) {
		ret = PTR_ERR(flashlight_torch_high);
		logI("%s : init err, flashlight_torch_high\n", __func__);
	}

	flashlight_torch_low = pinctrl_lookup_state(flashlight_pinctrl, "torch_low");
	if (IS_ERR(flashlight_torch_low)) {
		ret = PTR_ERR(flashlight_torch_low);
		logI("%s : init err, flashlight_torch_low\n", __func__);
	}

	/* Flashlight FLASH pin initialization */
	flashlight_flash_high = pinctrl_lookup_state(flashlight_pinctrl, "flash_high");
	if (IS_ERR(flashlight_flash_high)) {
		ret = PTR_ERR(flashlight_flash_high);
		logI("%s : init err, flashlight_flash_high\n", __func__);
	}

	flashlight_flash_low = pinctrl_lookup_state(flashlight_pinctrl, "flash_low");
	if (IS_ERR(flashlight_flash_low)) {
		ret = PTR_ERR(flashlight_flash_low);
		logI("%s : init err, flashlight_flash_low\n", __func__);
	}

	return ret;
}

int flashlight_gpio_set(int pin , int state)
{
	int ret = 0;

	if (IS_ERR(flashlight_pinctrl)) {
		logI("%s : set err, flashlight_pinctrl not available\n", __func__);
		return -1;
	}

	switch (pin) {
	case FLASHLIGHT_PIN_HWEN:
		if (state == STATE_LOW && !IS_ERR(flashlight_hwen_low))
			pinctrl_select_state(flashlight_pinctrl, flashlight_hwen_low);
		else if (state == STATE_HIGH && !IS_ERR(flashlight_hwen_high))
			pinctrl_select_state(flashlight_pinctrl, flashlight_hwen_high);
		else
			logI("%s : set err, pin(%d) state(%d)\n", __func__, pin, state);
		break;
	case FLASHLIGHT_PIN_TORCH:
		if (state == STATE_LOW && !IS_ERR(flashlight_torch_low))
			pinctrl_select_state(flashlight_pinctrl, flashlight_torch_low);
		else if (state == STATE_HIGH && !IS_ERR(flashlight_torch_high))
			pinctrl_select_state(flashlight_pinctrl, flashlight_torch_high);
		else
			logI("%s : set err, pin(%d) state(%d)\n", __func__, pin, state);
		break;
	case FLASHLIGHT_PIN_FLASH:
		if (state == STATE_LOW && !IS_ERR(flashlight_flash_low))
			pinctrl_select_state(flashlight_pinctrl, flashlight_flash_low);
		else if (state == STATE_HIGH && !IS_ERR(flashlight_flash_high))
			pinctrl_select_state(flashlight_pinctrl, flashlight_flash_high);
		else
			logI("%s : set err, pin(%d) state(%d)\n", __func__, pin, state);
		break;
	default:
			logI("%s : set err, pin(%d) state(%d)\n", __func__, pin, state);
		break;
	}
	logI("%s : pin(%d) state(%d)\n", __func__, pin, state);
	return ret;
}

/* ============================== */
/* functions */
/* ============================== */
int globalInit(void)
{
	int i;
	int j;
	int k;

	logI("globalInit");
	for (i = 0; i < e_Max_Sensor_Dev_Num; i++)
		for (j = 0; j < e_Max_Strobe_Num_Per_Dev; j++) {
			gLowBatDuty[i][j] = -1;
			g_strobePartId[i][j] = 1;
			for (k = 0; k < e_Max_Part_Num_Per_Dev; k++)
				g_pFlashInitFunc[i][j][k] = 0;
		}
	return 0;
}

int checkAndRelease(void)
{
	int i;
	int j;
	int k;

	mutex_lock(&g_mutex);
	for (i = 0; i < e_Max_Sensor_Dev_Num; i++)
		for (j = 0; j < e_Max_Strobe_Num_Per_Dev; j++)
			for (k = 0; k < e_Max_Part_Num_Per_Dev; k++) {
				if (g_pFlashInitFunc[i][j][k] != 0) {
					logI("checkAndRelease %d %d %d", i, j, k);
					g_pFlashInitFunc[i][j][k]->flashlight_release(0);
					g_pFlashInitFunc[i][j][k] = 0;
				}
			}
	mutex_unlock(&g_mutex);
	return 0;
}

int getSensorDevIndex(int sensorDev)
{
	if (sensorDev == e_CAMERA_MAIN_SENSOR)
		return 0;
	else if (sensorDev == e_CAMERA_SUB_SENSOR)
		return 1;
	else if (sensorDev == e_CAMERA_MAIN_2_SENSOR)
		return 2;
	logI("sensorDev=%d is wrong", sensorDev);
	return -1;
}

int getStrobeIndex(int strobeId)
{
	if (strobeId < 1 || strobeId > 2) {
		logI("strobeId=%d is wrong", strobeId);
		return -1;
	}
	return strobeId - 1;
}

int getPartIndex(int partId)
{
	if (partId < 1 || partId > 2) {
		logI("partId=%d is wrong", partId);
		return -1;
	}
	return partId - 1;
}

MINT32 default_flashlight_open(void *pArg)
{
	logI("[default_flashlight_open] E ~");
	return 0;
}

MINT32 default_flashlight_release(void *pArg)
{
	logI("[default_flashlight_release] E ~");
	return 0;
}

MINT32 default_flashlight_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int iFlashType = (int)FLASHLIGHT_NONE;
	kdStrobeDrvArg kdArg;
	unsigned long copyRet;

	copyRet = copy_from_user(&kdArg, (void *)arg, sizeof(kdStrobeDrvArg));


	switch (cmd) {
	case FLASHLIGHTIOC_G_FLASHTYPE:
		iFlashType = FLASHLIGHT_NONE;
		kdArg.arg = iFlashType;
		if (copy_to_user((void __user *)arg, (void *)&kdArg, sizeof(kdStrobeDrvArg))) {
			logI("[FLASHLIGHTIOC_G_FLASHTYPE] ioctl copy to user failed ~");
			return -EFAULT;
		}
		break;
	default:
		logI("[default_flashlight_ioctl] ~");
		break;
	}
	return i4RetValue;
}

FLASHLIGHT_FUNCTION_STRUCT defaultFlashlightFunc = {
	default_flashlight_open,
	default_flashlight_release,
	default_flashlight_ioctl,
};

UINT32 strobeInit_dummy(FLASHLIGHT_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &defaultFlashlightFunc;
	return 0;
}

/* ======================================================================== */
static int setFlashDrv(int sensorDev, int strobeId)
{
	int partId;
	int sensorDevIndex;
	int strobeIndex;
	int partIndex;
	FLASHLIGHT_FUNCTION_STRUCT **ppF = 0;

	sensorDevIndex = getSensorDevIndex(sensorDev);
	strobeIndex = getStrobeIndex(strobeId);
	if (sensorDevIndex < 0 || strobeIndex < 0)
		return -1;
	partId = g_strobePartId[sensorDevIndex][strobeIndex];
	partIndex = getPartIndex(partId);
	if (partIndex < 0)
		return -1;

	logI("setFlashDrv sensorDev=%d, strobeId=%d, partId=%d ~", sensorDev, strobeId, partId);

	ppF = &g_pFlashInitFunc[sensorDevIndex][strobeIndex][partIndex];
	if (sensorDev == e_CAMERA_MAIN_SENSOR) {
#if defined(DUMMY_FLASHLIGHT)
		strobeInit_dummy(ppF);
#else
		if (strobeId == 1) {
			if (partId == 1)
				constantFlashlightInit(ppF);
			else if (partId == 2)
				strobeInit_main_sid1_part2(ppF);
		} else if (strobeId == 2) {
			if (partId == 1)
				strobeInit_main_sid2_part1(ppF);

			else if (partId == 2)
				strobeInit_main_sid2_part2(ppF);
		}
#endif
	} else if (sensorDev == e_CAMERA_SUB_SENSOR) {
		if (strobeId == 1) {
			if (partId == 1)
				subStrobeInit(ppF);
			else if (partId == 2)
				strobeInit_sub_sid1_part2(ppF);
		} else if (strobeId == 2) {
			if (partId == 1)
				strobeInit_sub_sid2_part1(ppF);
			else if (partId == 2)
				strobeInit_sub_sid2_part2(ppF);
		}
	}


	if ((*ppF) != 0) {
		(*ppF)->flashlight_open(0);
		logI("setFlashDrv ok %d", __LINE__);
	} else {
		logI("set function pointer not found!!");
		return -1;
	}
	return 0;
}
#if POWER_THROTTLING

static int gLowPowerVbat = LOW_BATTERY_LEVEL_0;
static int gLowPowerPer = BATTERY_PERCENT_LEVEL_0;
static int gLowPowerOc = BATTERY_OC_LEVEL_0;
/*
static int decFlash(void)
{
	int i;
	int j;
	int k;
	int duty;

	for (i = 0; i < e_Max_Sensor_Dev_Num; i++)
		for (j = 0; j < e_Max_Strobe_Num_Per_Dev; j++)
			for (k = 0; k < e_Max_Part_Num_Per_Dev; k++) {
				if (g_pFlashInitFunc[i][j][k] != 0) {
					if (gLowBatDuty[i][j] != -1) {
						duty = gLowBatDuty[i][j];
						logI("decFlash i,j,k,duty %d %d %d %d", i, j, k,
						     duty);
						g_pFlashInitFunc[i][j][k]->flashlight_ioctl
						    (FLASH_IOC_SET_DUTY, duty);
					}
				}
			}
	return 0;
}
*/
static int closeFlash(void)
{
	int i;
	int j;
	int k;

	mutex_lock(&g_mutex);
	logI("closeFlash ln=%d, (%d/%d/%d)", __LINE__, gLowPowerVbat, gLowPowerPer, gLowPowerOc);
	for (i = 0; i < e_Max_Sensor_Dev_Num; i++) {
		for (j = 0; j < e_Max_Strobe_Num_Per_Dev; j++) {
			for (k = 0; k < e_Max_Part_Num_Per_Dev; k++) {
				if (g_pFlashInitFunc[i][j][k] != 0) {
					logI("closeFlash i,j,k %d %d %d", i, j, k);
					g_pFlashInitFunc[i][j][k]->flashlight_ioctl
					    (FLASH_IOC_SET_ONOFF, 0);
				}
			}
		}
	}
	mutex_unlock(&g_mutex);
	return 0;
}

static void Lbat_protection_powerlimit_flash(LOW_BATTERY_LEVEL level)
{
/*
	logI("Lbat_protection_powerlimit_flash %d (%d %d %d %d)\n", level, LOW_BATTERY_LEVEL_0,
	     LOW_BATTERY_LEVEL_1, LOW_BATTERY_LEVEL_2, __LINE__);
	logI("Lbat_protection_powerlimit_flash %d (%d %d %d %d)\n", level, LOW_BATTERY_LEVEL_0,
	     LOW_BATTERY_LEVEL_1, LOW_BATTERY_LEVEL_2, __LINE__);
*/
	if (level == LOW_BATTERY_LEVEL_0) {
		gLowPowerVbat = LOW_BATTERY_LEVEL_0;
	} else if (level == LOW_BATTERY_LEVEL_1) {
		closeFlash();
		gLowPowerVbat = LOW_BATTERY_LEVEL_1;

	} else if (level == LOW_BATTERY_LEVEL_2) {
		closeFlash();
		gLowPowerVbat = LOW_BATTERY_LEVEL_2;
	} else {
		/* unlimit cpu and gpu */
	}
}

static void bat_per_protection_powerlimit_flashlight(BATTERY_PERCENT_LEVEL level)
{
/*
	logI("bat_per_protection_powerlimit_flashlight %d (%d %d %d)\n", level,
	     BATTERY_PERCENT_LEVEL_0, BATTERY_PERCENT_LEVEL_1, __LINE__);
	logI("bat_per_protection_powerlimit_flashlight %d (%d %d %d)\n", level,
	     BATTERY_PERCENT_LEVEL_0, BATTERY_PERCENT_LEVEL_1, __LINE__);
*/
	if (level == BATTERY_PERCENT_LEVEL_0) {
		gLowPowerPer = BATTERY_PERCENT_LEVEL_0;
	} else if (level == BATTERY_PERCENT_LEVEL_1) {
		closeFlash();
		gLowPowerPer = BATTERY_PERCENT_LEVEL_1;
	} else {
		/*unlimit cpu and gpu*/
	}
}

static void bat_oc_protection_powerlimit(BATTERY_OC_LEVEL level)
{
/*
    logI("bat_oc_protection_powerlimit %d (%d %d %d)\n", level, BATTERY_OC_LEVEL_0, BATTERY_OC_LEVEL_1,__LINE__);
    logI("bat_oc_protection_powerlimit %d (%d %d %d)\n", level, BATTERY_OC_LEVEL_0, BATTERY_OC_LEVEL_1,__LINE__);
*/
    if (level == BATTERY_OC_LEVEL_1){
	closeFlash();
	gLowPowerOc=BATTERY_OC_LEVEL_1;
    }
    else{
	gLowPowerOc=BATTERY_OC_LEVEL_0;
    }
}


#endif

/* ======================================================================== */

static long flashlight_ioctl_core(struct file *file, unsigned int cmd, unsigned long arg)
{
	int partId;
	int sensorDevIndex;
	int strobeIndex;
	int partIndex;
	int i4RetValue = 0;
	kdStrobeDrvArg kdArg;
	unsigned long copyRet;

	copyRet = copy_from_user(&kdArg, (void *)arg, sizeof(kdStrobeDrvArg));
	logI("flashlight_ioctl cmd=0x%x(nr=%d), senorDev=0x%x ledId=0x%x arg=0x%lx", cmd,
	     _IOC_NR(cmd), kdArg.sensorDev, kdArg.strobeId, (unsigned long)kdArg.arg);
	sensorDevIndex = getSensorDevIndex(kdArg.sensorDev);
	strobeIndex = getStrobeIndex(kdArg.strobeId);
	if (sensorDevIndex < 0 || strobeIndex < 0)
		return -1;
	partId = g_strobePartId[sensorDevIndex][strobeIndex];
	partIndex = getPartIndex(partId);
	if (partIndex < 0)
		return -1;



	switch (cmd) {
	case FLASH_IOC_GET_PROTOCOL_VERSION:
		i4RetValue = 1;
		break;
	case FLASH_IOC_IS_LOW_POWER:
		logI("FLASH_IOC_IS_LOW_POWER");
		{
			int isLow = 0;
#if POWER_THROTTLING
			if (gLowPowerPer != BATTERY_PERCENT_LEVEL_0
			|| gLowPowerVbat != LOW_BATTERY_LEVEL_0
			|| gLowPowerOc != BATTERY_OC_LEVEL_0)
				isLow = 1;
			logI("FLASH_IOC_IS_LOW_POWER %d %d %d", gLowPowerPer, gLowPowerVbat, isLow);
#endif
			kdArg.arg = isLow;
			if (copy_to_user
			    ((void __user *)arg, (void *)&kdArg, sizeof(kdStrobeDrvArg))) {
				logI("[FLASH_IOC_IS_LOW_POWER] ioctl copy to user failed ~");
				return -EFAULT;
			}
		}
		break;

	case FLASH_IOC_LOW_POWER_DETECT_START:
		logI("FLASH_IOC_LOW_POWER_DETECT_START");
		gLowBatDuty[sensorDevIndex][strobeIndex] = kdArg.arg;
		break;

	case FLASH_IOC_LOW_POWER_DETECT_END:
		logI("FLASH_IOC_LOW_POWER_DETECT_END");
		gLowBatDuty[sensorDevIndex][strobeIndex] = -1;
		break;
	case FLASHLIGHTIOC_X_SET_DRIVER:
		i4RetValue = setFlashDrv(kdArg.sensorDev, kdArg.strobeId);
		break;
	case FLASH_IOC_GET_PART_ID:
	case FLASH_IOC_GET_MAIN_PART_ID:
	case FLASH_IOC_GET_SUB_PART_ID:
	case FLASH_IOC_GET_MAIN2_PART_ID:
		{
			int partId;

			partId = strobe_getPartId(kdArg.sensorDev, kdArg.strobeId);
			g_strobePartId[sensorDevIndex][strobeIndex] = partId;
			kdArg.arg = partId;
			if (copy_to_user
			    ((void __user *)arg, (void *)&kdArg, sizeof(kdStrobeDrvArg))) {
				logI("[FLASH_IOC_GET_PART_ID] ioctl copy to user failed ~");
				return -EFAULT;
			}
			logI("FLASH_IOC_GET_PART_ID line=%d partId=%d", __LINE__, partId);
		}
		break;
	case FLASH_IOC_SET_ONOFF:
		{
			FLASHLIGHT_FUNCTION_STRUCT *pF;

			pF = g_pFlashInitFunc[sensorDevIndex][strobeIndex][partIndex];
			if (pF != 0) {
#if DLPT_FEATURE
				kicker_pbm_by_flash(kdArg.arg);
#endif
				if (!(gLowPowerVbat == LOW_BATTERY_LEVEL_0 &&
				gLowPowerPer == BATTERY_PERCENT_LEVEL_0 &&
				gLowPowerOc == BATTERY_OC_LEVEL_0))
					logI("[FLASH_IOC_SET_ONOFF] PT condition (%d, %d, %d)",
					gLowPowerVbat, gLowPowerPer, gLowPowerOc);
				i4RetValue = pF->flashlight_ioctl(cmd, kdArg.arg);
			} else {
				logI("[FLASH_IOC_SET_ONOFF] function pointer is wrong -");
			}
		}
		break;
	case FLASH_IOC_UNINIT:
		{
			FLASHLIGHT_FUNCTION_STRUCT *pF;

			pF = g_pFlashInitFunc[sensorDevIndex][strobeIndex][partIndex];
			if (pF != 0) {
				i4RetValue = pF->flashlight_release((void *)0);
				pF = 0;

			} else {
				logI("[FLASH_IOC_UNINIT] function pointer is wrong ~");
			}
		}
		break;
	default:
		{
			FLASHLIGHT_FUNCTION_STRUCT *pF;

			pF = g_pFlashInitFunc[sensorDevIndex][strobeIndex][partIndex];
			if (pF != 0)
				i4RetValue = pF->flashlight_ioctl(cmd, kdArg.arg);
			else
				logI("[default] function pointer is wrong ~");
		}
		break;
	}
	return i4RetValue;
}


static long flashlight_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err;
/* int dir; */
	err = flashlight_ioctl_core(file, cmd, arg);
	/* dir  = _IOC_DIR(cmd); */
	/* if(dir &_IOC_READ) */
	{
		/* copy_to_user */
	}
	return err;
}

#ifdef CONFIG_COMPAT

/*
static int compat_arg_struct_user32_to_kernel(
			struct StrobeDrvArg __user *data32,
			struct compat_StrobeDrvArg __user *data)
{
	compat_int_t i;
	int err=0;

	err |= get_user(i, &data32->sensorDev);
	err |= put_user(i, &data->sensorDev);

	err |= get_user(i, &data32->arg);
	err |= put_user(i, &data->arg);

	return err;
}

static int compat_arg_struct_kernel_to_user32(
			struct StrobeDrvArg __user *data32,
			struct compat_StrobeDrvArg __user *data)

{
	compat_int_t i;
	int err=0;

    err |= get_user(i, &data->sensorDev);
	err |= put_user(i, &data32->sensorDev);
	err |= get_user(i, &data->arg);
	err |= put_user(i, &data32->arg);

	return err;
}*/



static long my_ioctl_compat(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int err;
	/* int copyRet; */
	kdStrobeDrvArg *pUObj;

	logI("flash my_ioctl_compat2 line=%d cmd=%d arg=%ld\n", __LINE__, cmd, arg);
	pUObj = compat_ptr(arg);

	/*
	   kdStrobeDrvArg* pUObj;
	   pUObj = compat_ptr(arg);
	   kdStrobeDrvArg obj;
	   copyRet = copy_from_user(&obj , (void *)pUObj , sizeof(kdStrobeDrvArg));
	   logI("strobe arg %d %d %d\n", obj.sensorDev, obj.strobeId, obj.arg);
	   obj.arg = 23411;
	   copy_to_user((void __user *) arg , (void*)&obj , sizeof(kdStrobeDrvArg));
	 */




	/* data = compat_alloc_user_space(sizeof(*data)); */
	/* if (sys_data == NULL) */
	/* return -EFAULT; */
	/* err = compat_arg_struct_user32_to_kernel(data32, data); */
	/* arg2 = (unsigned long)data32; */
	err = flashlight_ioctl_core(filep, cmd, (unsigned long)pUObj);

	return err;

}
#endif


static int flashlight_open(struct inode *inode, struct file *file)
{
	int i4RetValue = 0;
	static int bInited;

	if (bInited == 0) {
		globalInit();
		bInited = 1;
	}
	logI("[flashlight_open] E ~");
	return i4RetValue;
}

static int flashlight_release(struct inode *inode, struct file *file)
{
	logI("[flashlight_release] E ~");

	checkAndRelease();

	return 0;
}


#ifdef WIN32
int fl_open(struct inode *inode, struct file *file)
{
	return flashlight_open(inode, file);
}

int fl_release(struct inode *inode, struct file *file)
{
	return flashlight_release(inode, file);
}

long fl_ioctrl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return flashlight_ioctl(file, cmd, arg);
}

#else
/* ======================================================================== */
/* ======================================================================== */
/* ======================================================================== */
/* Kernel interface */
static const struct file_operations flashlight_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = flashlight_ioctl,
	.open = flashlight_open,
	.release = flashlight_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = my_ioctl_compat,
#endif
};

/* ======================================================================== */
/* Driver interface */
/* ======================================================================== */
struct flashlight_data {
	spinlock_t lock;
	wait_queue_head_t read_wait;
	struct semaphore sem;
};
static struct class *flashlight_class;
static struct device *flashlight_device;
static struct flashlight_data flashlight_private;
static dev_t flashlight_devno;
static struct cdev *g_pFlash_CharDrv;
/* ======================================================================== */
#define ALLOC_DEVNO
static int flashlight_probe(struct platform_device *dev)
{
	int ret = 0, err = 0;

	logI("[flashlight_probe] start ~");

#ifdef ALLOC_DEVNO
	ret = alloc_chrdev_region(&flashlight_devno, 0, 1, FLASHLIGHT_DEVNAME);
	if (ret) {
		logI("[flashlight_probe] alloc_chrdev_region fail: %d ~", ret);
		goto flashlight_probe_error;
	} else {
		logI("[flashlight_probe] major: %d, minor: %d ~", MAJOR(flashlight_devno),
		     MINOR(flashlight_devno));
	}
	/* Allocate driver */
	g_pFlash_CharDrv = cdev_alloc();
	if (NULL == g_pFlash_CharDrv) {
		unregister_chrdev_region(flashlight_devno, 1);

		logI("Allocate mem for kobject failed\n");

		return -1;
	}
	cdev_init(g_pFlash_CharDrv, &flashlight_fops);
	g_pFlash_CharDrv->owner = THIS_MODULE;
	err = cdev_add(g_pFlash_CharDrv, flashlight_devno, 1);
	if (err) {
		logI("[flashlight_probe] cdev_add fail: %d ~", err);
		goto flashlight_probe_error;
	}
#else
#define FLASHLIGHT_MAJOR 242
	ret = register_chrdev(FLASHLIGHT_MAJOR, FLASHLIGHT_DEVNAME, &flashlight_fops);
	if (ret != 0) {
		logI("[flashlight_probe] Unable to register chardev on major=%d (%d) ~",
		     FLASHLIGHT_MAJOR, ret);
		return ret;
	}
	flashlight_devno = MKDEV(FLASHLIGHT_MAJOR, 0);
#endif


	flashlight_class = class_create(THIS_MODULE, "flashlightdrv");
	if (IS_ERR(flashlight_class)) {
		logI("[flashlight_probe] Unable to create class, err = %d ~",
		     (int)PTR_ERR(flashlight_class));
		goto flashlight_probe_error;
	}

	flashlight_device =
	    device_create(flashlight_class, NULL, flashlight_devno, NULL, FLASHLIGHT_DEVNAME);
	if (NULL == flashlight_device) {
		logI("[flashlight_probe] device_create fail ~");
		goto flashlight_probe_error;
	}

	/* initialize members */
	spin_lock_init(&flashlight_private.lock);
	init_waitqueue_head(&flashlight_private.read_wait);
	/* init_MUTEX(&flashlight_private.sem); */
	sema_init(&flashlight_private.sem, 1);
	/* GPIO pinctrl initial */
	flashlight_gpio_init(dev);

	logI("[flashlight_probe] Done ~");
	return 0;

flashlight_probe_error:
#ifdef ALLOC_DEVNO
	if (err == 0)
		cdev_del(g_pFlash_CharDrv);
	if (ret == 0)
		unregister_chrdev_region(flashlight_devno, 1);
#else
	if (ret == 0)
		unregister_chrdev(MAJOR(flashlight_devno), FLASHLIGHT_DEVNAME);
#endif
	return -1;
}

static int flashlight_remove(struct platform_device *dev)
{

	logI("[flashlight_probe] start\n");

#ifdef ALLOC_DEVNO
	cdev_del(g_pFlash_CharDrv);
	unregister_chrdev_region(flashlight_devno, 1);
#else
	unregister_chrdev(MAJOR(flashlight_devno), FLASHLIGHT_DEVNAME);
#endif
	device_destroy(flashlight_class, flashlight_devno);
	class_destroy(flashlight_class);

	logI("[flashlight_probe] Done ~");
	return 0;
}

static void flashlight_shutdown(struct platform_device *dev)
{

	logI("[flashlight_shutdown] start\n");
	checkAndRelease();
	logI("[flashlight_shutdown] Done ~");
}

#ifdef CONFIG_OF
static const struct of_device_id FLASHLIGHT_of_match[] = {
	{.compatible = "mediatek,mt6757-flashlight"},
	{},
};
#endif

static struct platform_driver flashlight_platform_driver = {
	.probe = flashlight_probe,
	.remove = flashlight_remove,
	.shutdown = flashlight_shutdown,
	.driver = {
		   .name = FLASHLIGHT_DEVNAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
	.of_match_table = FLASHLIGHT_of_match,
#endif
		   },
};

static struct platform_device flashlight_platform_device = {
	.name = FLASHLIGHT_DEVNAME,
	.id = 0,
	.dev = {
		}
};

static int __init flashlight_init(void)
{
	int ret = 0;

	logI("[flashlight_probe] start ~");

	ret = platform_device_register(&flashlight_platform_device);
	if (ret) {
		logI("[flashlight_probe] platform_device_register fail ~");
		return ret;
	}

	ret = platform_driver_register(&flashlight_platform_driver);
	if (ret) {
		logI("[flashlight_probe] platform_driver_register fail ~");
		return ret;
	}
#if POWER_THROTTLING
	register_low_battery_notify(&Lbat_protection_powerlimit_flash, LOW_BATTERY_PRIO_FLASHLIGHT);
	register_battery_percent_notify(&bat_per_protection_powerlimit_flashlight, BATTERY_PERCENT_PRIO_FLASHLIGHT);
	register_battery_oc_notify(&bat_oc_protection_powerlimit, BATTERY_OC_PRIO_FLASHLIGHT);
#endif
	logI("[flashlight_probe] done! ~");
	return ret;
}

static void __exit flashlight_exit(void)
{
	logI("[flashlight_probe] start ~");
	platform_driver_unregister(&flashlight_platform_driver);
	/* to flush work queue */
	/* flush_scheduled_work(); */
	logI("[flashlight_probe] done! ~");
}

/* ======================================================== */
module_init(flashlight_init);
module_exit(flashlight_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jackie Su <jackie.su@mediatek.com>");
MODULE_DESCRIPTION("Flashlight control Driver");

#endif
