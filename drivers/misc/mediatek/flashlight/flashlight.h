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

#ifndef _FLASHLIGHT_H
#define _FLASHLIGHT_H

#include <linux/ioctl.h>
#include <linux/types.h>

/* debug macro */
#define FLASHLIGHT_PREFIX "[FLASHLIGHT]"
#define fl_err(fmt, arg...)      pr_err(FLASHLIGHT_PREFIX " %s: " fmt, __func__, ##arg)

#ifdef DEBUG
#define fl_info                  fl_err
#define fl_dbg                   fl_err
#else
#define fl_info(fmt, arg...)     pr_info(FLASHLIGHT_PREFIX " %s: " fmt, __func__, ##arg)
#define fl_dbg(fmt, arg...)      pr_debug(FLASHLIGHT_PREFIX " %s: " fmt, __func__, ##arg)
#endif

/* names */
#define FLASHLIGHT_CORE "flashlight_core"
#define FLASHLIGHT_DEVNAME "flashlight"

/* protocol version */
#define FLASHLIGHT_PROTOCOL_VERSION 1

/* scenario */
#define FLASHLIGHT_SCENARIO_CAMERA_MASK 1
#define FLASHLIGHT_SCENARIO_DECOUPLE_MASK 2
#define FLASHLIGHT_SCENARIO_FLASHLIGHT (0 << 0)
#define FLASHLIGHT_SCENARIO_CAMERA     (1 << 0)
#define FLASHLIGHT_SCENARIO_COUPLE     (0 << 1)
#define FLASHLIGHT_SCENARIO_DECOUPLE   (1 << 1)

/* power throttling */
#define PT_NOTIFY_NUM 3
#define PT_NOTIFY_LOW_VOL  0
#define PT_NOTIFY_LOW_BAT  1
#define PT_NOTIFY_OVER_CUR 2

/* charger status */
#define FLASHLIGHT_CHARGER_NUM    4
#define FLASHLIGHT_CHARGER_TYPE   0
#define FLASHLIGHT_CHARGER_CT     1
#define FLASHLIGHT_CHARGER_PART   2
#define FLASHLIGHT_CHARGER_STATUS 3

#define FLASHLIGHT_CHARGER_NOT_READY 0
#define FLASHLIGHT_CHARGER_READY     1

#define FLASHLIGHT_CHARGER_STATUS_TMPBUF_SIZE 9
#define FLASHLIGHT_CHARGER_STATUS_BUF_SIZE \
	(FLASHLIGHT_TYPE_MAX * FLASHLIGHT_CT_MAX * FLASHLIGHT_PART_MAX * \
	 FLASHLIGHT_CHARGER_STATUS_TMPBUF_SIZE + 1)

/* flashlight devices */
#define FLASHLIGHT_NAME_SIZE 32 /* flashlight device name */
#define FLASHLIGHT_DEVICE_NUM 8 /* flashlight device number */
#define FLASHLIGHT_TYPE_MAX 2
#define FLASHLIGHT_CT_MAX 2
#define FLASHLIGHT_PART_MAX 2
struct flashlight_device_id {
	int type;
	int ct;
	int part;
	char name[FLASHLIGHT_NAME_SIZE]; /* device name */
	int channel;                     /* device channel */
	int decouple;                    /* device decouple */
};

extern const struct flashlight_device_id flashlight_id[];

/* flashlight arguments */
#define FLASHLIGHT_ARG_NUM   5
#define FLASHLIGHT_ARG_TYPE  0
#define FLASHLIGHT_ARG_CT    1
#define FLASHLIGHT_ARG_PART  2
#define FLASHLIGHT_ARG_LEVEL 3
#define FLASHLIGHT_ARG_DUR   4

#define FLASHLIGHT_ARG_TYPE_MAX  FLASHLIGHT_TYPE_MAX
#define FLASHLIGHT_ARG_CT_MAX    FLASHLIGHT_CT_MAX
#define FLASHLIGHT_ARG_PART_MAX  FLASHLIGHT_PART_MAX
#define FLASHLIGHT_ARG_LEVEL_MAX 255
#define FLASHLIGHT_ARG_DUR_MAX   400 /* ms */

struct flashlight_arg {
	int type;
	int ct;
	int part;
	int channel;
	int level;
	int dur;
};

struct flashlight_user_arg {
	int type_id;
	int ct_id;
	int arg;
};

struct flashlight_dev_arg {
	int channel;
	int arg;
};

/* flashlight operations */
struct flashlight_operations {
	int (*flashlight_open)(void *pArg);
	int (*flashlight_release)(void *pArg);
	int (*flashlight_ioctl)(unsigned int cmd, unsigned long arg);
	ssize_t (*flashlight_strobe_store)(struct flashlight_arg arg);
	int (*flashlight_set_driver)(int scenario);
};

/* flashlight resiger */
int flashlight_dev_register(const char *name, struct flashlight_operations *dev_ops);
int flashlight_dev_unregister(const char *name);

/* get id and index */
int flashlight_get_type_id(int type_index);
int flashlight_get_ct_id(int ct_index);
int flashlight_get_part_id(int part_index);
int flashlight_get_type_index(int type_id);
int flashlight_get_ct_index(int ct_id);
int flashlight_get_part_index(int part_id);

/* verify functionis */
int flashlight_type_index_verify(int type_index);
int flashlight_ct_index_verify(int ct_index);
int flashlight_part_index_verify(int part_index);
int flashlight_index_verify(int type_index, int ct_index, int part_index);

/* flash type enum */
typedef enum {
	FLASHLIGHT_NONE = 0,
	FLASHLIGHT_LED_ONOFF,    /* LED always on/off */
	FLASHLIGHT_LED_CONSTANT, /* CONSTANT type LED */
	FLASHLIGHT_LED_PEAK,     /* peak strobe type LED */
	FLASHLIGHT_LED_TORCH,    /* LED turn on when switch FLASH_ON */
	FLASHLIGHT_XENON_SCR,    /* SCR strobe type Xenon */
	FLASHLIGHT_XENON_IGBT    /* IGBT strobe type Xenon */
} FLASHLIGHT_TYPE_ENUM;

/* ioctl magic number */
#define FLASHLIGHT_MAGIC 'S'

/* ioctl protocol version 0. */
#define FLASHLIGHTIOC_T_ENABLE             _IOW(FLASHLIGHT_MAGIC, 5, int)
#define FLASHLIGHTIOC_T_LEVEL              _IOW(FLASHLIGHT_MAGIC, 10, int)
#define FLASHLIGHTIOC_T_FLASHTIME          _IOW(FLASHLIGHT_MAGIC, 15, int)
#define FLASHLIGHTIOC_T_STATE              _IOW(FLASHLIGHT_MAGIC, 20, int)
#define FLASHLIGHTIOC_G_FLASHTYPE          _IOR(FLASHLIGHT_MAGIC, 25, int)
#define FLASHLIGHTIOC_X_SET_DRIVER         _IOWR(FLASHLIGHT_MAGIC, 30, int)
#define FLASHLIGHTIOC_T_DELAY              _IOW(FLASHLIGHT_MAGIC, 35, int)

/* ioctl protocol version 1. */
#define FLASH_IOC_SET_TIME_OUT_TIME_MS     _IOR(FLASHLIGHT_MAGIC, 100, int)
#define FLASH_IOC_SET_STEP                 _IOR(FLASHLIGHT_MAGIC, 105, int)
#define FLASH_IOC_SET_DUTY                 _IOR(FLASHLIGHT_MAGIC, 110, int)
#define FLASH_IOC_SET_ONOFF                _IOR(FLASHLIGHT_MAGIC, 115, int)
#define FLASH_IOC_UNINIT                   _IOR(FLASHLIGHT_MAGIC, 120, int)

#define FLASH_IOC_PRE_ON                   _IOR(FLASHLIGHT_MAGIC, 125, int)
#define FLASH_IOC_GET_PRE_ON_TIME_MS       _IOR(FLASHLIGHT_MAGIC, 130, int)
#define FLASH_IOC_GET_PRE_ON_TIME_MS_DUTY  _IOR(FLASHLIGHT_MAGIC, 131, int)

#define FLASH_IOC_SET_REG_ADR              _IOR(FLASHLIGHT_MAGIC, 135, int)
#define FLASH_IOC_SET_REG_VAL              _IOR(FLASHLIGHT_MAGIC, 140, int)
#define FLASH_IOC_SET_REG                  _IOR(FLASHLIGHT_MAGIC, 145, int)
#define FLASH_IOC_GET_REG                  _IOR(FLASHLIGHT_MAGIC, 150, int)

#define FLASH_IOC_GET_MAIN_PART_ID         _IOR(FLASHLIGHT_MAGIC, 155, int)
#define FLASH_IOC_GET_SUB_PART_ID          _IOR(FLASHLIGHT_MAGIC, 160, int)
#define FLASH_IOC_GET_MAIN2_PART_ID        _IOR(FLASHLIGHT_MAGIC, 165, int)
#define FLASH_IOC_GET_PART_ID              _IOR(FLASHLIGHT_MAGIC, 166, int)

#define FLASH_IOC_HAS_LOW_POWER_DETECT     _IOR(FLASHLIGHT_MAGIC, 170, int)
#define FLASH_IOC_LOW_POWER_DETECT_START   _IOR(FLASHLIGHT_MAGIC, 175, int)
#define FLASH_IOC_LOW_POWER_DETECT_END     _IOR(FLASHLIGHT_MAGIC, 180, int)
#define FLASH_IOC_IS_LOW_POWER             _IOR(FLASHLIGHT_MAGIC, 182, int)

#define FLASH_IOC_GET_ERR                  _IOR(FLASHLIGHT_MAGIC, 185, int)
#define FLASH_IOC_GET_PROTOCOL_VERSION     _IOR(FLASHLIGHT_MAGIC, 190, int)

#define FLASH_IOC_IS_CHARGER_IN            _IOR(FLASHLIGHT_MAGIC, 195, int)
#define FLASH_IOC_IS_OTG_USE               _IOR(FLASHLIGHT_MAGIC, 200, int)
#define FLASH_IOC_GET_FLASH_DRIVER_NAME_ID _IOR(FLASHLIGHT_MAGIC, 205, int)

/* ioctl protocol version 1.1 */
#define FLASH_IOC_IS_CHARGER_READY         _IOR(FLASHLIGHT_MAGIC, 210, int)

#endif /* _FLASHLIGHT_H */

