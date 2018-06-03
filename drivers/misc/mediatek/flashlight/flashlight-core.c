/*
 * Flashlight Core
 *
 * Copyright (C) 2015 MediaTek Inc.
 *
 * Author: Simon Wang <Simon-TCH.Wang@mediatek.com>
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
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#ifdef CONFIG_COMPAT
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#include "flashlight.h"
#include <mach/upmu_sw.h> /* PT */

#ifdef CONFIG_MTK_FLASHLIGHT_DLPT
#include <mtk_pbm.h> /* DLPT */
#endif


/******************************************************************************
 * Definition
 *****************************************************************************/
static struct flashlight_operations
	*fl_ops[FLASHLIGHT_TYPE_MAX][FLASHLIGHT_CT_MAX][FLASHLIGHT_PART_MAX];
static DEFINE_MUTEX(fl_mutex);

/* power variables */
static int fl_level_low_bat[FLASHLIGHT_TYPE_MAX][FLASHLIGHT_CT_MAX];
#ifdef CONFIG_MTK_FLASHLIGHT_PT
#ifdef CONFIG_MTK_FLASHLIGHT_PT_STRICT
static int pt_strict = 1;
#else
static int pt_strict; /* always be zero from C standard */
#endif
#endif

/* charger variables */
static int charger_status[FLASHLIGHT_TYPE_MAX][FLASHLIGHT_CT_MAX][FLASHLIGHT_PART_MAX];

/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
int fl_init(void)
{
	int i, j, k;

	for (i = 0; i < FLASHLIGHT_TYPE_MAX; i++) {
		for (j = 0; j < FLASHLIGHT_CT_MAX; j++) {
			fl_level_low_bat[i][j] = -1;
			for (k = 0; k < FLASHLIGHT_PART_MAX; k++) {
				charger_status[i][j][k] = FLASHLIGHT_CHARGER_READY;
				fl_ops[i][j][k] = NULL;
			}
		}
	}

	return 0;
}

int fl_open(void)
{
	int i, j, k;

	for (i = 0; i < FLASHLIGHT_TYPE_MAX; i++) {
		for (j = 0; j < FLASHLIGHT_CT_MAX; j++) {
			for (k = 0; k < FLASHLIGHT_PART_MAX; k++) {
				mutex_lock(&fl_mutex);
				if (fl_ops[i][j][k]) {
					fl_dbg("fl_open (%d,%d,%d)\n", i, j, k);
					fl_ops[i][j][k]->flashlight_open(NULL);
				}
				mutex_unlock(&fl_mutex);
			}
		}
	}

	return 0;
}

int fl_release(void)
{
	int i, j, k;

	for (i = 0; i < FLASHLIGHT_TYPE_MAX; i++) {
		for (j = 0; j < FLASHLIGHT_CT_MAX; j++) {
			for (k = 0; k < FLASHLIGHT_PART_MAX; k++) {
				mutex_lock(&fl_mutex);
				if (fl_ops[i][j][k]) {
					fl_dbg("fl_release (%d,%d,%d)\n", i, j, k);
					fl_ops[i][j][k]->flashlight_release(NULL);
				}
				mutex_unlock(&fl_mutex);
			}
		}
	}

	return 0;
}

/* get id */
int fl_get_type_id(int type_index)
{
	if (type_index < 0 || type_index > (FLASHLIGHT_TYPE_MAX - 1)) {
		fl_err("color temperature id (%d) is not valid\n", type_index);
		return -1;
	}

	return type_index + 1;
}
EXPORT_SYMBOL(fl_get_type_id);

int fl_get_ct_id(int ct_index)
{
	if (ct_index < 0 || ct_index > (FLASHLIGHT_CT_MAX - 1)) {
		fl_err("color temperature id (%d) is not valid\n", ct_index);
		return -1;
	}

	return ct_index + 1;
}
EXPORT_SYMBOL(fl_get_ct_id);

int fl_get_part_id(int part_index)
{
	if (part_index < 0 || part_index > (FLASHLIGHT_PART_MAX - 1)) {
		fl_err("color temperature id (%d) is not valid\n", part_index);
		return -1;
	}

	return part_index + 1;
}
EXPORT_SYMBOL(fl_get_part_id);

/* get index */
int fl_get_type_index(int type_id)
{
	if (type_id < 1 || type_id > FLASHLIGHT_TYPE_MAX) {
		fl_err("color temperature id (%d) is not valid\n", type_id);
		return -1;
	}

	return type_id - 1;
}
EXPORT_SYMBOL(fl_get_type_index);

int fl_get_ct_index(int ct_id)
{
	if (ct_id < 1 || ct_id > FLASHLIGHT_CT_MAX) {
		fl_err("color temperature id (%d) is not valid\n", ct_id);
		return -1;
	}

	return ct_id - 1;
}
EXPORT_SYMBOL(fl_get_ct_index);

int fl_get_part_index(int part_id)
{
	if (part_id < 1 || part_id > FLASHLIGHT_PART_MAX) {
		fl_err("part id (%d) is not valid\n", part_id);
		return -1;
	}

	return part_id - 1;
}
EXPORT_SYMBOL(fl_get_part_index);

/* verify function */
int flashlight_type_index_verify(int type_index)
{
	if (type_index < 0 || type_index >= FLASHLIGHT_ARG_TYPE_MAX) {
		fl_err("type index (%d) is not valid\n", type_index);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(flashlight_type_index_verify);

int flashlight_ct_index_verify(int ct_index)
{
	if (ct_index < 0 || ct_index >= FLASHLIGHT_ARG_CT_MAX) {
		fl_err("ct index (%d) is not valid\n", ct_index);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(flashlight_ct_index_verify);

int flashlight_part_index_verify(int part_index)
{
	if (part_index < 0 || part_index >= FLASHLIGHT_ARG_PART_MAX) {
		fl_err("part index (%d) is not valid\n", part_index);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(flashlight_part_index_verify);

int flashlight_index_verify(int type_index, int ct_index, int part_index)
{
	if (flashlight_type_index_verify(type_index) ||
			flashlight_ct_index_verify(ct_index) ||
			flashlight_part_index_verify(part_index))
		return -1;
	return 0;
}
EXPORT_SYMBOL(flashlight_index_verify);

static int flashlight_arg_verify(struct flashlight_arg fl_arg)
{
	if (flashlight_index_verify(fl_arg.type, fl_arg.ct, fl_arg.part))
		return -1;
	if (fl_arg.level < -1 || fl_arg.level > FLASHLIGHT_ARG_LEVEL_MAX) {
		fl_err("level (%d) is not valid\n", fl_arg.level);
		return -1;
	}
	if (fl_arg.dur < 0 || fl_arg.dur > FLASHLIGHT_ARG_DUR_MAX) {
		fl_err("duration (%d) is not valid\n", fl_arg.dur);
		return -1;
	}

	return 0;
}

/* register function */
int flashlight_dev_register(const char *name, struct flashlight_operations *dev_ops)
{
	int type_index, ct_index, part_index;
	int i;

	for (i = 0; i < FLASHLIGHT_DEVICE_NUM; i++) {
		if (!strncmp(name, flashlight_id[i].name, FLASHLIGHT_NAME_SIZE)) {
			type_index = flashlight_id[i].type;
			ct_index = flashlight_id[i].ct;
			part_index = flashlight_id[i].part;

			if (flashlight_index_verify(type_index, ct_index, part_index)) {
				fl_err("Failed to register device (%s)\n", flashlight_id[i].name);
				continue;
			}

			fl_dbg("%s %d %d %d\n",
					flashlight_id[i].name, type_index, ct_index, part_index);

			mutex_lock(&fl_mutex);
			fl_ops[type_index][ct_index][part_index] = dev_ops;
			mutex_unlock(&fl_mutex);
		}
	}

	return 0;
}
EXPORT_SYMBOL(flashlight_dev_register);

int flashlight_dev_unregister(const char *name)
{
	int type_index, ct_index, part_index;
	int i;

	for (i = 0; i < FLASHLIGHT_DEVICE_NUM; i++) {
		if (!strncmp(name, flashlight_id[i].name, FLASHLIGHT_NAME_SIZE)) {
			type_index = flashlight_id[i].type;
			ct_index = flashlight_id[i].ct;
			part_index = flashlight_id[i].part;

			if (flashlight_index_verify(type_index, ct_index, part_index)) {
				fl_err("Failed to unregister device (%s)\n", flashlight_id[i].name);
				continue;
			}

			fl_dbg("%s %d %d %d\n",
					flashlight_id[i].name, type_index, ct_index, part_index);

			mutex_lock(&fl_mutex);
			fl_ops[type_index][ct_index][part_index] = NULL;
			mutex_unlock(&fl_mutex);
		}
	}

	return 0;
}
EXPORT_SYMBOL(flashlight_dev_unregister);

/******************************************************************************
 * Vsync IRQ
 *****************************************************************************/
/*
 * LED flash control for high current capture mode
 * which is used by "imgsensor/src/[PLAT]/kd_sensorlist.c"
 */
ssize_t strobe_VDIrq(void)
{
	return 0;
}
EXPORT_SYMBOL(strobe_VDIrq);


/******************************************************************************
 * Charger Status
 *****************************************************************************/
static int flashlight_update_charger_status(
		int type_index, int ct_index, int part_index)
{
	struct flashlight_operations *pf;
	struct flashlight_user_arg fl_arg;

	if (flashlight_index_verify(type_index, ct_index, part_index)) {
		fl_err("Failed with error index\n");
		return -1;
	}

	mutex_lock(&fl_mutex);
	pf = fl_ops[type_index][ct_index][part_index];
	mutex_unlock(&fl_mutex);
	if (!pf) {
		fl_info("Failed with no flashlight ops\n");
		return -1;
	}

	fl_arg.type_id = fl_get_type_id(type_index);
	fl_arg.ct_id = fl_get_ct_id(ct_index);

	if (pf->flashlight_ioctl(FLASH_IOC_IS_CHARGER_READY, (unsigned long)&fl_arg))
		fl_info("Failed to get charger status.\n");
	else
		charger_status[type_index][ct_index][part_index] = fl_arg.arg;

	return charger_status[type_index][ct_index][part_index];
}


/******************************************************************************
 * Power throttling
 *****************************************************************************/
#ifdef CONFIG_MTK_FLASHLIGHT_PT
static int pt_low_vol = LOW_BATTERY_LEVEL_0;
static int pt_low_bat = BATTERY_PERCENT_LEVEL_0;
static int pt_over_cur = BATTERY_OC_LEVEL_0;

static int flashlight_set_low_level(void)
{
	struct flashlight_user_arg fl_arg;
	int i, j, k;

	fl_info("Tune down flashlight level since power throttling: (%d,%d,%d)\n",
			pt_low_vol, pt_low_bat, pt_over_cur);

	for (i = 0; i < FLASHLIGHT_TYPE_MAX; i++) {
		for (j = 0; j < FLASHLIGHT_CT_MAX; j++) {
			for (k = 0; k < FLASHLIGHT_PART_MAX; k++) {
				mutex_lock(&fl_mutex);
				if (fl_ops[i][j][k]) {
					fl_arg.type_id = fl_get_type_id(i);
					fl_arg.ct_id = fl_get_ct_id(j);
					fl_arg.arg = fl_level_low_bat[i][j];
					fl_ops[i][j][k]->flashlight_ioctl(
							FLASH_IOC_SET_DUTY, (unsigned long)&fl_arg);
				}
				mutex_unlock(&fl_mutex);
			}
		}
	}

	return 0;
}

static int flashlight_disable(void)
{
	struct flashlight_user_arg fl_arg;
	int i, j, k;

	fl_info("Disable flashlight since power throttling: (%d,%d,%d)\n",
			pt_low_vol, pt_low_bat, pt_over_cur);

	for (i = 0; i < FLASHLIGHT_TYPE_MAX; i++) {
		for (j = 0; j < FLASHLIGHT_CT_MAX; j++) {
			for (k = 0; k < FLASHLIGHT_PART_MAX; k++) {
				mutex_lock(&fl_mutex);
				if (fl_ops[i][j][k]) {
					fl_arg.type_id = fl_get_type_id(i);
					fl_arg.ct_id = fl_get_ct_id(j);
					fl_arg.arg = 0;
					fl_ops[i][j][k]->flashlight_ioctl(
							FLASH_IOC_SET_ONOFF, (unsigned long)&fl_arg);
				}
				mutex_unlock(&fl_mutex);
			}
		}
	}

	return 0;
}

static int pt_arg_verify(int pt_low_vol, int pt_low_bat, int pt_over_cur)
{
	if (pt_low_vol < LOW_BATTERY_LEVEL_0 || pt_low_vol > LOW_BATTERY_LEVEL_2) {
		fl_err("PT low voltage (%d) is not valid\n", pt_low_vol);
		return -1;
	}
	if (pt_low_bat < BATTERY_PERCENT_LEVEL_0 || pt_low_bat > BATTERY_PERCENT_LEVEL_1) {
		fl_err("PT low battery (%d) is not valid\n", pt_low_bat);
		return -1;
	}
	if (pt_over_cur < BATTERY_OC_LEVEL_0 || pt_over_cur > BATTERY_OC_LEVEL_1) {
		fl_err("PT over current (%d) is not valid\n", pt_over_cur);
		return -1;
	}

	return 0;
}

static int pt_trigger(void)
{
	if (pt_strict)
		flashlight_disable();
	else
		flashlight_set_low_level();

	return 0;
}

static void pt_low_vol_callback(LOW_BATTERY_LEVEL level)
{
	if (level == LOW_BATTERY_LEVEL_0) {
		pt_low_vol = LOW_BATTERY_LEVEL_0;
	} else if (level == LOW_BATTERY_LEVEL_1) {
		pt_trigger();
		pt_low_vol = LOW_BATTERY_LEVEL_1;
	} else if (level == LOW_BATTERY_LEVEL_2) {
		pt_trigger();
		pt_low_vol = LOW_BATTERY_LEVEL_2;
	} else {
		/* unlimit cpu and gpu */
	}
}

static void pt_low_bat_callback(BATTERY_PERCENT_LEVEL level)
{
	if (level == BATTERY_PERCENT_LEVEL_0) {
		pt_low_bat = BATTERY_PERCENT_LEVEL_0;
	} else if (level == BATTERY_PERCENT_LEVEL_1) {
		pt_trigger();
		pt_low_bat = BATTERY_PERCENT_LEVEL_1;
	} else {
		/* unlimit cpu and gpu*/
	}
}

static void pt_oc_callback(BATTERY_OC_LEVEL level)
{
	if (level == BATTERY_OC_LEVEL_1) {
		pt_trigger();
		pt_over_cur = BATTERY_OC_LEVEL_1;
	} else {
		pt_over_cur = BATTERY_OC_LEVEL_0;
	}
}
#endif


/******************************************************************************
 * Kernel file operations
 *****************************************************************************/
/*
 * WARNING:
 * Init flow:
 * The flow in user space
 *  (1) get part id. (Then part id array is ready.)
 *  (2) register flashlight operations. (Then ioctl is ready.)
 *
 * Enable flow:
 *  (1) set timeout
 *  (2) set duty
 *  (3) enable
 */
static long _flashlight_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct flashlight_operations *pf;
	struct flashlight_user_arg fl_arg;
	int type_index, ct_index, part_index;
	int ret = 0;

	if (copy_from_user(&fl_arg, (void __user *)arg, sizeof(struct flashlight_user_arg))) {
		fl_err("Failed copy arguments from user.\n");
		return -EFAULT;
	}

	/* get type, color temperature and part index */
	type_index = fl_get_type_index(fl_arg.type_id);
	ct_index = fl_get_ct_index(fl_arg.ct_id);
#if 0
	/* TODO: userspace could passing part id */
	part_index = fl_get_part_index(fl_arg.part_id);
#endif
	part_index = fl_get_part_index(1);

	if (flashlight_index_verify(type_index, ct_index, part_index)) {
		fl_err("Failed with error index\n");
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_GET_PROTOCOL_VERSION:
		fl_dbg("FLASH_IOC_GET_PROTOCOL_VERSION(%d,%d,%d)\n",
				type_index, ct_index, part_index);
		ret = FLASHLIGHT_PROTOCOL_VERSION;
		break;

	case FLASH_IOC_IS_LOW_POWER:
		fl_arg.arg = 0;
#ifdef CONFIG_MTK_FLASHLIGHT_PT
		if (pt_low_bat != BATTERY_PERCENT_LEVEL_0
				|| pt_low_vol != LOW_BATTERY_LEVEL_0
				|| pt_over_cur != BATTERY_OC_LEVEL_0) {
			fl_arg.arg = 1;
			if (pt_strict)
				fl_arg.arg = 2;
			fl_dbg("Pt status: (%d,%d,%d)\n",
					pt_low_vol, pt_low_bat, pt_over_cur);
		}
#endif
		fl_dbg("FLASH_IOC_IS_LOW_POWER(%d,%d,%d): %d\n",
				type_index, ct_index, part_index, fl_arg.arg);
		if (copy_to_user((void __user *)arg, (void *)&fl_arg,
					sizeof(struct flashlight_user_arg))) {
			fl_err("Failed to copy power status to user.\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_LOW_POWER_DETECT_START:
		fl_dbg("FLASH_IOC_LOW_POWER_DETECT_START(%d,%d,%d)\n",
				type_index, ct_index, part_index);
		mutex_lock(&fl_mutex);
		fl_level_low_bat[type_index][ct_index] = fl_arg.arg;
		mutex_unlock(&fl_mutex);
		break;

	case FLASH_IOC_LOW_POWER_DETECT_END:
		fl_dbg("FLASH_IOC_LOW_POWER_DETECT_END(%d,%d,%d)\n",
				type_index, ct_index, part_index);
		mutex_lock(&fl_mutex);
		fl_level_low_bat[type_index][ct_index] = -1;
		mutex_unlock(&fl_mutex);
		break;

	case FLASH_IOC_IS_CHARGER_READY:
		flashlight_update_charger_status(type_index, ct_index, part_index);
		fl_arg.arg = charger_status[type_index][ct_index][part_index];
		fl_dbg("FLASH_IOC_IS_CHARGER_READY(%d,%d,%d): %d\n",
				type_index, ct_index, part_index, fl_arg.arg);
		if (copy_to_user((void __user *)arg, (void *)&fl_arg,
					sizeof(struct flashlight_user_arg))) {
			fl_err("Failed to copy power status to user.\n");
			return -EFAULT;
		}
		break;

	case FLASHLIGHTIOC_X_SET_DRIVER:
		fl_dbg("FLASHLIGHTIOC_X_SET_DRIVER(%d,%d,%d): %d\n",
				type_index, ct_index, part_index, fl_arg.arg);
		mutex_lock(&fl_mutex);
		pf = fl_ops[type_index][ct_index][part_index];
		mutex_unlock(&fl_mutex);
		if (pf) {
			pf->flashlight_set_driver(fl_arg.arg);
		} else {
			fl_info("Failed with no flashlight ops\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_GET_PART_ID:
	case FLASH_IOC_GET_MAIN_PART_ID:
	case FLASH_IOC_GET_SUB_PART_ID:
	case FLASH_IOC_GET_MAIN2_PART_ID:
		fl_arg.arg = fl_get_part_id(part_index);
		fl_dbg("FLASH_IOC_GET_[TYPE]_PART_ID(%d,%d,%d): %d\n",
				type_index, ct_index, part_index, fl_arg.arg);
		if (copy_to_user((void __user *)arg, (void *)&fl_arg,
					sizeof(struct flashlight_user_arg))) {
			fl_err("Failed to copy part id to user.\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_SET_ONOFF:
		fl_dbg("FLASH_IOC_SET_ONOFF(%d,%d,%d)\n",
				type_index, ct_index, part_index);
		mutex_lock(&fl_mutex);
		pf = fl_ops[type_index][ct_index][part_index];
		mutex_unlock(&fl_mutex);
		if (pf) {
#ifdef CONFIG_MTK_FLASHLIGHT_DLPT
			kicker_pbm_by_flash(fl_arg.arg);
#endif
#ifdef CONFIG_MTK_FLASHLIGHT_PT
		if (pt_low_bat != BATTERY_PERCENT_LEVEL_0
				|| pt_low_vol != LOW_BATTERY_LEVEL_0
				|| pt_over_cur != BATTERY_OC_LEVEL_0)
			fl_dbg("Pt status: (%d,%d,%d)\n",
					pt_low_vol, pt_low_bat, pt_over_cur);
#endif
			ret = pf->flashlight_ioctl(cmd, (unsigned long)&fl_arg);
			if (ret) {
				fl_err("Failed to set on/off.\n");
				return -EFAULT;
			}
		} else {
			fl_info("Failed with no flashlight ops\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_UNINIT:
		fl_dbg("FLASH_IOC_UNINIT(%d,%d,%d)\n",
				type_index, ct_index, part_index);
		mutex_lock(&fl_mutex);
		pf = fl_ops[type_index][ct_index][part_index];
		mutex_unlock(&fl_mutex);
		if (pf) {
			ret = pf->flashlight_release(NULL);
			if (ret) {
				fl_err("Failed to uninit.\n");
				return -EFAULT;
			}
		} else {
			fl_info("Failed with no flashlight ops\n");
			return -EFAULT;
		}
		break;

	default:
		mutex_lock(&fl_mutex);
		pf = fl_ops[type_index][ct_index][part_index];
		mutex_unlock(&fl_mutex);
		if (pf)
			ret = pf->flashlight_ioctl(cmd, (unsigned long)&fl_arg);
		else {
			fl_info("Failed with no flashlight ops\n");
			return -ENOTTY;
		}
		break;
	}

	return ret;
}

static long flashlight_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return _flashlight_ioctl(file, cmd, arg);
}

#ifdef CONFIG_COMPAT
static long flashlight_compat_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	return _flashlight_ioctl(filep, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int flashlight_open(struct inode *inode, struct file *file)
{
	return fl_open();
}

static int flashlight_release(struct inode *inode, struct file *file)
{
	return fl_release();
}

static const struct file_operations flashlight_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = flashlight_ioctl,
	.open = flashlight_open,
	.release = flashlight_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = flashlight_compat_ioctl,
#endif
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
struct flashlight_data {
	spinlock_t lock;
	wait_queue_head_t read_wait;
	struct mutex mutex;
};

static struct flashlight_data flashlight_pdata;
static struct class *flashlight_class;
static struct device *flashlight_device;
static dev_t flashlight_devno;
static struct cdev *flashlight_cdev;

/* flashlight strobe sysfs */
static ssize_t flashlight_strobe_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	fl_dbg("Strobe show.\n");

	return scnprintf(buf, PAGE_SIZE,
			"[TYPE] [CT] [PART] [LEVEL] [DURATION(ms)]\n");
}

static ssize_t flashlight_strobe_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct flashlight_operations *pf;
	struct flashlight_arg fl_arg;
	s32 num;
	int count = 0;
	char delim[] = " ";
	char *token, *cur = (char *)buf;
	int ret;

	fl_dbg("Strobe store.\n");

	while (cur) {
		token = strsep(&cur, delim);
		ret = kstrtos32(token, 10, &num);
		if (ret) {
			fl_err("Error arguments.\n");
			goto unlock;
		}

		if (count == FLASHLIGHT_ARG_TYPE)
			fl_arg.type = (int)num;
		else if (count == FLASHLIGHT_ARG_CT)
			fl_arg.ct = (int)num;
		else if (count == FLASHLIGHT_ARG_PART)
			fl_arg.part = (int)num;
		else if (count == FLASHLIGHT_ARG_LEVEL)
			fl_arg.level = (int)num;
		else if (count == FLASHLIGHT_ARG_DUR)
			fl_arg.dur = (int)num;
		else {
			count++;
			break;
		}

		count++;
	}

	/* verify data */
	if (count != FLASHLIGHT_ARG_NUM) {
		fl_err("Error argument number: (%d)\n", count);
		ret = -1;
		goto unlock;
	}
	if (flashlight_arg_verify(fl_arg)) {
		fl_err("Error arguments.\n");
		ret = -1;
		goto unlock;
	}

	fl_dbg("(%d, %d, %d), (%d, %d)\n",
			fl_arg.type, fl_arg.ct, fl_arg.part, fl_arg.level, fl_arg.dur);

	/* call callback function */
	pf = fl_ops[fl_arg.type][fl_arg.ct][fl_arg.part];
	if (pf) {
		pf->flashlight_strobe_store(fl_arg);
		ret = size;
	} else {
		fl_info("Failed with no flashlight ops\n");
		ret = -1;
	}

unlock:
	return ret;
}

static DEVICE_ATTR_RW(flashlight_strobe);

/* pt status sysfs */
static ssize_t flashlight_pt_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	fl_dbg("Power throttling show.\n");

#ifdef CONFIG_MTK_FLASHLIGHT_PT
	return scnprintf(buf, PAGE_SIZE,
			"[LOW_VOLTAGE] [LOW_BATTERY] [OVER_CURRENT]\n%d %d %d\n",
			pt_low_vol, pt_low_bat, pt_over_cur);
#else
	return scnprintf(buf, PAGE_SIZE,
			"No support power throttling.\n");
#endif
}

static ssize_t flashlight_pt_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int low_vol = LOW_BATTERY_LEVEL_0;
	int low_bat = BATTERY_PERCENT_LEVEL_0;
	int over_cur = BATTERY_OC_LEVEL_0;
	u32 num;
	int count = 0;
	char delim[] = " ";
	char *token, *cur = (char *)buf;
	int ret;

	fl_dbg("Power throttling store.\n");

	mutex_lock(&flashlight_pdata.mutex);

	while (cur) {
		token = strsep(&cur, delim);
		ret = kstrtou32(token, 10, &num);
		if (ret) {
			fl_err("Error arguments.\n");
			goto unlock;
		}

		if (count == PT_NOTIFY_LOW_VOL)
			low_vol = (int)num;
		else if (count == PT_NOTIFY_LOW_BAT)
			low_bat = (int)num;
		else if (count == PT_NOTIFY_OVER_CUR)
			over_cur = (int)num;
		else {
			count++;
			break;
		}

		count++;
	}

	/* verify data */
	if (count != PT_NOTIFY_NUM) {
		fl_err("Error argument number: (%d)\n", count);
		ret = -1;
		goto unlock;
	}

#ifdef CONFIG_MTK_FLASHLIGHT_PT
	if (pt_arg_verify(low_vol, low_bat, over_cur)) {
		fl_err("Error arguments.\n");
		ret = -1;
		goto unlock;
	}
	fl_dbg("(%d, %d, %d)\n", low_vol, low_bat, over_cur);

	/* call callback function */
	pt_low_vol_callback(low_vol);
	pt_low_bat_callback(low_bat);
	pt_oc_callback(over_cur);
#endif

	ret = size;
unlock:
	mutex_unlock(&flashlight_pdata.mutex);
	return ret;
}

static DEVICE_ATTR_RW(flashlight_pt);

/* charger status sysfs */
static ssize_t flashlight_charger_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	int i, j, k;
	char status[FLASHLIGHT_CHARGER_STATUS_BUF_SIZE];
	char status_tmp[FLASHLIGHT_CHARGER_STATUS_TMPBUF_SIZE];

	fl_dbg("Charger status show.\n");

	memset(status, '\0', FLASHLIGHT_CHARGER_STATUS_BUF_SIZE);
	for (i = 0; i < FLASHLIGHT_TYPE_MAX; i++)
		for (j = 0; j < FLASHLIGHT_CT_MAX; j++)
			for (k = 0; k < FLASHLIGHT_PART_MAX; k++) {
				flashlight_update_charger_status(i, j, k);
				snprintf(status_tmp,
						FLASHLIGHT_CHARGER_STATUS_TMPBUF_SIZE,
						"%d %d %d %d\n", i, j, k, charger_status[i][j][k]);
				strcat(status, status_tmp);
			}

	return scnprintf(buf, PAGE_SIZE,
			"[TYPE] [CT] [PART] [CHARGER_STATUS]\n%s", status);
}

static ssize_t flashlight_charger_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct flashlight_arg fl_arg;
	int charger_status_tmp = 0;
	s32 num;
	int count = 0;
	char delim[] = " ";
	char *token, *cur = (char *)buf;
	int ret;

	fl_dbg("Charger status store.\n");

	memset(&fl_arg, 0, sizeof(struct flashlight_arg));

	while (cur) {
		token = strsep(&cur, delim);
		ret = kstrtos32(token, 10, &num);
		if (ret) {
			fl_err("Error arguments.\n");
			goto unlock;
		}

		if (count == FLASHLIGHT_CHARGER_TYPE)
			fl_arg.type = (int)num;
		else if (count == FLASHLIGHT_CHARGER_CT)
			fl_arg.ct = (int)num;
		else if (count == FLASHLIGHT_CHARGER_PART)
			fl_arg.part = (int)num;
		else if (count == FLASHLIGHT_CHARGER_STATUS)
			charger_status_tmp = (int)num;
		else {
			count++;
			break;
		}

		count++;
	}

	/* verify data */
	if (count != FLASHLIGHT_CHARGER_NUM) {
		fl_err("Error argument number: (%d)\n", count);
		ret = -1;
		goto unlock;
	}
	if (flashlight_index_verify(fl_arg.type, fl_arg.ct, fl_arg.part)) {
		fl_err("Error arguments.\n");
		ret = -1;
		goto unlock;
	}
	if (charger_status_tmp < FLASHLIGHT_CHARGER_NOT_READY ||
			charger_status_tmp > FLASHLIGHT_CHARGER_READY) {
		fl_err("Error arguments charger status(%d).\n", charger_status_tmp);
		ret = -1;
		goto unlock;
	}

	fl_dbg("(%d, %d, %d), (%d)\n",
			fl_arg.type, fl_arg.ct, fl_arg.part, charger_status_tmp);

	/* store charger status */
	charger_status[fl_arg.type][fl_arg.ct][fl_arg.part] = charger_status_tmp;

	ret = size;
unlock:
	return ret;
}

static DEVICE_ATTR_RW(flashlight_charger);

static int flashlight_probe(struct platform_device *dev)
{
	fl_dbg("Probe start.\n");

	/* allocate char device number */
	if (alloc_chrdev_region(&flashlight_devno, 0, 1, FLASHLIGHT_DEVNAME)) {
		fl_err("Failed to allocate char device region.\n");
		goto err_allocate_chrdev;
	}
	fl_dbg("Allocate major number and minor number: (%d, %d)\n",
			MAJOR(flashlight_devno),
			MINOR(flashlight_devno));

	/* allocate char device */
	flashlight_cdev = cdev_alloc();
	if (!flashlight_cdev) {
		fl_err("Failed to allcoate cdev\n");
		goto err_allocate_cdev;
	}
	flashlight_cdev->ops = &flashlight_fops;
	flashlight_cdev->owner = THIS_MODULE;

	/* add char device to the system */
	if (cdev_add(flashlight_cdev, flashlight_devno, 1)) {
		fl_err("Failed to add cdev\n");
		goto err_add_cdev;
	}

	/* create class */
	flashlight_class = class_create(THIS_MODULE, FLASHLIGHT_CORE);
	if (IS_ERR(flashlight_class)) {
		fl_err("Failed to create class (%d)\n", (int)PTR_ERR(flashlight_class));
		goto err_create_class;
	}

	/* create device */
	flashlight_device =
	    device_create(flashlight_class, NULL, flashlight_devno, NULL, FLASHLIGHT_DEVNAME);
	if (!flashlight_device) {
		fl_err("Failed to create device\n");
		goto err_create_device;
	}

	/* create device file */
	if (device_create_file(flashlight_device, &dev_attr_flashlight_strobe)) {
		fl_err("Failed to create device file(strobe)\n");
		goto err_create_strobe_device_file;
	}
	if (device_create_file(flashlight_device, &dev_attr_flashlight_pt)) {
		fl_err("Failed to create device file(pt)\n");
		goto err_create_pt_device_file;
	}
	if (device_create_file(flashlight_device, &dev_attr_flashlight_charger)) {
		fl_err("Failed to create device file(charger)\n");
		goto err_create_charger_device_file;
	}

	/* initialize spin lock, wait queue, mutex, pin control */
	spin_lock_init(&flashlight_pdata.lock);
	init_waitqueue_head(&flashlight_pdata.read_wait);
	mutex_init(&flashlight_pdata.mutex);

	/* init flashlight operations */
	fl_init();

	fl_dbg("Probe done.\n");

	return 0;

err_create_charger_device_file:
	device_remove_file(flashlight_device, &dev_attr_flashlight_pt);
err_create_pt_device_file:
	device_remove_file(flashlight_device, &dev_attr_flashlight_strobe);
err_create_strobe_device_file:
	device_destroy(flashlight_class, flashlight_devno);
err_create_device:
	class_destroy(flashlight_class);
err_create_class:
err_add_cdev:
	cdev_del(flashlight_cdev);
err_allocate_cdev:
	unregister_chrdev_region(flashlight_devno, 1);
err_allocate_chrdev:
	return -1;
}

static int flashlight_remove(struct platform_device *dev)
{
	/* remove device file */
	device_remove_file(flashlight_device, &dev_attr_flashlight_charger);
	device_remove_file(flashlight_device, &dev_attr_flashlight_pt);
	device_remove_file(flashlight_device, &dev_attr_flashlight_strobe);
	/* remove device */
	device_destroy(flashlight_class, flashlight_devno);
	/* remove class */
	class_destroy(flashlight_class);
	/* remove char device */
	cdev_del(flashlight_cdev);
	/* unregister char device number */
	unregister_chrdev_region(flashlight_devno, 1);

	return 0;
}

static void flashlight_shutdown(struct platform_device *dev)
{
	fl_release();
}

#ifdef CONFIG_OF
static const struct of_device_id flashlight_of_match[] = {
	{.compatible = "mediatek,flashlight_core"},
	{},
};
MODULE_DEVICE_TABLE(of, flashlight_of_match);
#else
static struct platform_device flashlight_platform_device[] = {
	{
		.name = FLASHLIGHT_DEVNAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, flashlight_platform_device);
#endif

static struct platform_driver flashlight_platform_driver = {
	.probe = flashlight_probe,
	.remove = flashlight_remove,
	.shutdown = flashlight_shutdown,
	.driver = {
		   .name = FLASHLIGHT_DEVNAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = flashlight_of_match,
#endif
	},
};

static int __init flashlight_init(void)
{
	int ret;

	fl_dbg("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&flashlight_platform_device);
	if (ret) {
		fl_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&flashlight_platform_driver);
	if (ret) {
		fl_err("Failed to register platform driver\n");
		return ret;
	}

#ifdef CONFIG_MTK_FLASHLIGHT_PT
	register_low_battery_notify(&pt_low_vol_callback, LOW_BATTERY_PRIO_FLASHLIGHT);
	register_battery_percent_notify(&pt_low_bat_callback, BATTERY_PERCENT_PRIO_FLASHLIGHT);
	register_battery_oc_notify(&pt_oc_callback, BATTERY_OC_PRIO_FLASHLIGHT);
#endif

	fl_dbg("Init done.\n");

	return 0;
}

static void __exit flashlight_exit(void)
{
	fl_dbg("Exit start.\n");

	platform_driver_unregister(&flashlight_platform_driver);

	fl_dbg("Exit done.\n");
}

module_init(flashlight_init);
module_exit(flashlight_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight Core Driver");

