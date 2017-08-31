/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   OV16880mipiraw_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * [201501] PDAF MP Version 
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov16880mipiraw_Sensor.h"

#define PFX "OV16880_camera_sensor"
#define LOG_1 LOG_INF("OV16880,MIPI 4LANE,PDAF\n")
#define LOG_2 LOG_INF("preview 2664*1500@30fps,888Mbps/lane; video 5328*3000@30fps,1390Mbps/lane; capture 16M@30fps,1390Mbps/lane\n")
#define LOG_INF(fmt, args...)   pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##args)
#define LOGE(format, args...)   pr_err("[%s] " format, __FUNCTION__, ##args)

#define sensor_mirror
//#define sensor_flip

static DEFINE_SPINLOCK(imgsensor_drv_lock);

extern bool read_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size);

//#define PDAF_TEST 1
#ifdef PDAF_TEST
extern bool wrtie_eeprom(kal_uint16 addr, BYTE data[],kal_uint32 size );
char data[4096]= {
0x50,	0x44,	0x30,	0x31,	0x7e,	0xff,	0xd6,	0x0,	0xe4,	0x1,	0x0,	0x0,	0x4,	0x0,	0x14,	0x0 ,
0x53,	0x4,	0x53,	0x4,	0x6b,	0x4,	0x85,	0x4,	0x9d,	0x4,	0xb4,	0x4,	0xb6,	0x4,	0xa4,	0x4 ,
0x76,	0x4,	0x4c,	0x4,	0x1d,	0x4,	0xea,	0x3,	0xb9,	0x3,	0x99,	0x3,	0x80,	0x3,	0x75,	0x3 ,
0x76,	0x3,	0x75,	0x3,	0x7b,	0x3,	0x87,	0x3,	0x7c,	0x4,	0x83,	0x4,	0xb0,	0x4,	0xdd,	0x4 ,
0xfe,	0x4,	0x9,	0x5,	0x2,	0x5,	0xdc,	0x4,	0x9b,	0x4,	0x5d,	0x4,	0x1b,	0x4,	0xda,	0x3 ,
0x9d,	0x3,	0x74,	0x3,	0x56,	0x3,	0x48,	0x3,	0x45,	0x3,	0x42,	0x3,	0x4e,	0x3,	0x5d,	0x3 ,
0x89,	0x4,	0x99,	0x4,	0xc2,	0x4,	0xf1,	0x4,	0xd,	0x5,	0x16,	0x5,	0x9,	0x5,	0xdd,	0x4 ,
0x99,	0x4,	0x4c,	0x4,	0x8,	0x4,	0xc8,	0x3,	0x8a,	0x3,	0x5f,	0x3,	0x48,	0x3,	0x39,	0x3 ,
0x36,	0x3,	0x32,	0x3,	0x38,	0x3,	0x40,	0x3,	0x80,	0x4,	0x82,	0x4,	0x9e,	0x4,	0xc2,	0x4 ,
0xd4,	0x4,	0xce,	0x4,	0xc3,	0x4,	0xaa,	0x4,	0x74,	0x4,	0x32,	0x4,	0xfa,	0x3,	0xc3,	0x3 ,
0x94,	0x3,	0x73,	0x3,	0x5c,	0x3,	0x53,	0x3,	0x4d,	0x3,	0x44,	0x3,	0x41,	0x3,	0x4e,	0x3 ,
0x74,	0x0,	0x7b,	0x0,	0x85,	0x0,	0x91,	0x0,	0x9d,	0x0,	0xaa,	0x0,	0xb5,	0x0,	0xbc,	0x0 ,
0xbe,	0x0,	0xbd,	0x0,	0xb7,	0x0,	0xae,	0x0,	0xa1,	0x0,	0x95,	0x0,	0x8a,	0x0,	0x80,	0x0 ,
0x78,	0x0,	0x71,	0x0,	0x6a,	0x0,	0x65,	0x0,	0x7b,	0x0,	0x84,	0x0,	0x91,	0x0,	0xa1,	0x0 ,
0xb1,	0x0,	0xc2,	0x0,	0xd0,	0x0,	0xd8,	0x0,	0xdb,	0x0,	0xd9,	0x0,	0xcf,	0x0,	0xc2,	0x0 ,
0xb1,	0x0,	0xa2,	0x0,	0x92,	0x0,	0x85,	0x0,	0x7b,	0x0,	0x73,	0x0,	0x6c,	0x0,	0x67,	0x0 ,
0x7a,	0x0,	0x84,	0x0,	0x90,	0x0,	0x9f,	0x0,	0xaf,	0x0,	0xbe,	0x0,	0xca,	0x0,	0xd3,	0x0 ,
0xd5,	0x0,	0xd0,	0x0,	0xc7,	0x0,	0xba,	0x0,	0xaa,	0x0,	0x9c,	0x0,	0x8e,	0x0,	0x82,	0x0 ,
0x79,	0x0,	0x70,	0x0,	0x6a,	0x0,	0x65,	0x0,	0x73,	0x0,	0x7b,	0x0,	0x85,	0x0,	0x8f,	0x0 ,
0x9a,	0x0,	0xa3,	0x0,	0xab,	0x0,	0xb0,	0x0,	0xb1,	0x0,	0xad,	0x0,	0xa7,	0x0,	0x9f,	0x0 ,
0x95,	0x0,	0x8b,	0x0,	0x81,	0x0,	0x79,	0x0,	0x72,	0x0,	0x6b,	0x0,	0x64,	0x0,	0x5f,	0x0 ,
0x6b,	0x0,	0x72,	0x0,	0x79,	0x0,	0x80,	0x0,	0x88,	0x0,	0x91,	0x0,	0x9a,	0x0,	0xa2,	0x0 ,
0xab,	0x0,	0xb0,	0x0,	0xb2,	0x0,	0xb2,	0x0,	0xae,	0x0,	0xa6,	0x0,	0x9e,	0x0,	0x94,	0x0 ,
0x8b,	0x0,	0x83,	0x0,	0x7a,	0x0,	0x73,	0x0,	0x6e,	0x0,	0x75,	0x0,	0x7c,	0x0,	0x84,	0x0 ,
0x8e,	0x0,	0x9a,	0x0,	0xa6,	0x0,	0xb2,	0x0,	0xbf,	0x0,	0xc7,	0x0,	0xca,	0x0,	0xc9,	0x0 ,
0xc4,	0x0,	0xbb,	0x0,	0xb0,	0x0,	0xa3,	0x0,	0x97,	0x0,	0x8d,	0x0,	0x83,	0x0,	0x7b,	0x0 ,
0x6c,	0x0,	0x73,	0x0,	0x7a,	0x0,	0x81,	0x0,	0x8b,	0x0,	0x96,	0x0,	0xa1,	0x0,	0xae,	0x0 ,
0xb9,	0x0,	0xc2,	0x0,	0xc6,	0x0,	0xc5,	0x0,	0xc1,	0x0,	0xba,	0x0,	0xae,	0x0,	0xa2,	0x0 ,
0x97,	0x0,	0x8d,	0x0,	0x84,	0x0,	0x7c,	0x0,	0x67,	0x0,	0x6d,	0x0,	0x73,	0x0,	0x79,	0x0 ,
0x80,	0x0,	0x87,	0x0,	0x90,	0x0,	0x97,	0x0,	0x9f,	0x0,	0xa5,	0x0,	0xa8,	0x0,	0xa9,	0x0 ,
0xa6,	0x0,	0xa1,	0x0,	0x99,	0x0,	0x92,	0x0,	0x8a,	0x0,	0x83,	0x0,	0x7b,	0x0,	0x73,	0x0 ,
0x50,	0x44,	0x30,	0x32,	0x7e,	0xff,	0xd6,	0x0,	0x1a,	0x3,	0x0,	0x0,	0x8,	0x8,	0xa,	0x0 ,
0x6e,	0xba,	0x4a,	0x95,	0x47,	0x87,	0xd5,	0x89,	0xfd,	0x82,	0xe,	0x88,	0xb8,	0x90,	0x1,	0xb6,
0x18,	0xb1,	0x1a,	0x8a,	0x64,	0x84,	0x84,	0x81,	0x68,	0x7f,	0xde,	0x81,	0xae,	0x87,	0xcf,	0xaa,
0xcc,	0xbe,	0x6b,	0x8c,	0x15,	0x85,	0xec,	0x84,	0x2,	0x7c,	0x27,	0x85,	0xcf,	0x8d,	0x32,	0xaa,
0x15,	0xcc,	0xa5,	0x89,	0x31,	0x86,	0xc,	0x81,	0x6,	0x7e,	0x52,	0x86,	0x11,	0x85,	0x40,	0xa8,
0x3a,	0xbb,	0xae,	0x87,	0xa5,	0x89,	0x1b,	0x87,	0xe9,	0x80,	0x3f,	0x86,	0xdb,	0x87,	0xaf,	0xa8,
0x3b,	0xbb,	0xa5,	0x89,	0xc5,	0x85,	0xa,	0x81,	0xcb,	0x7f,	0xa,	0x81,	0xdc,	0x89,	0xed,	0xb6,
0xae,	0xbc,	0x22,	0x99,	0x90,	0x8b,	0x14,	0x82,	0x87,	0x81,	0x40,	0x8c,	0x2a,	0x90,	0x61,	0xb6,
0xf1,	0xb3,	0xc3,	0x96,	0x1a,	0x8a,	0x7e,	0x8c,	0x60,	0x84,	0xe8,	0x90,	0x1,	0x98,	0x15,	0xcc,
0x78,	0x4,	0x26,	0x2,	0x58,	0x2,	0x8a,	0x2,	0xbc,	0x2,	0xee,	0x2,	0x20,	0x3,	0x52,	0x3 ,
0x84,	0x3,	0xb6,	0x3,	0xe8,	0x3,	0x4c,	0x4c,	0x52,	0x56,	0x5c,	0x60,	0x64,	0x66,	0x6a,	0x72,
0x3e,	0x44,	0x4a,	0x4e,	0x52,	0x58,	0x5e,	0x64,	0x6a,	0x6e,	0x38,	0x3c,	0x42,	0x48,	0x4e,	0x56,
0x5a,	0x60,	0x66,	0x6c,	0x32,	0x38,	0x3e,	0x44,	0x4a,	0x50,	0x54,	0x5a,	0x62,	0x66,	0x32,	0x3a,
0x40,	0x46,	0x4c,	0x50,	0x58,	0x5e,	0x64,	0x6a,	0x34,	0x3e,	0x42,	0x48,	0x4e,	0x54,	0x5a,	0x5e,
0x66,	0x6a,	0x3e,	0x44,	0x4a,	0x50,	0x56,	0x5c,	0x60,	0x66,	0x6a,	0x70,	0x46,	0x4c,	0x50,	0x52,
0x5a,	0x5c,	0x62,	0x64,	0x6a,	0x6e,	0x4a,	0x52,	0x52,	0x58,	0x5a,	0x60,	0x64,	0x6a,	0x6e,	0x74,
0x3c,	0x42,	0x48,	0x4c,	0x52,	0x58,	0x60,	0x64,	0x6a,	0x70,	0x34,	0x3a,	0x42,	0x48,	0x4e,	0x54,
0x5a,	0x5e,	0x66,	0x6a,	0x2e,	0x36,	0x3a,	0x42,	0x48,	0x4e,	0x54,	0x5a,	0x60,	0x66,	0x30,	0x36,
0x3a,	0x44,	0x4a,	0x50,	0x56,	0x5a,	0x62,	0x68,	0x34,	0x3a,	0x40,	0x48,	0x4c,	0x52,	0x5a,	0x60,
0x66,	0x6a,	0x3c,	0x42,	0x48,	0x4e,	0x54,	0x5a,	0x60,	0x66,	0x6c,	0x70,	0x46,	0x4c,	0x50,	0x56,
0x5a,	0x60,	0x64,	0x68,	0x6c,	0x70,	0x4c,	0x50,	0x56,	0x58,	0x5c,	0x62,	0x66,	0x68,	0x6a,	0x74,
0x3e,	0x42,	0x4a,	0x50,	0x54,	0x5a,	0x60,	0x66,	0x6c,	0x70,	0x34,	0x3c,	0x42,	0x4a,	0x4e,	0x54,
0x5a,	0x60,	0x64,	0x6c,	0x30,	0x34,	0x3a,	0x40,	0x46,	0x4e,	0x54,	0x5a,	0x5e,	0x64,	0x2e,	0x36,
0x3a,	0x42,	0x46,	0x4e,	0x54,	0x5c,	0x62,	0x68,	0x34,	0x3a,	0x42,	0x48,	0x4e,	0x54,	0x58,	0x5e,
0x66,	0x6a,	0x3c,	0x44,	0x4a,	0x50,	0x54,	0x5a,	0x60,	0x66,	0x6a,	0x70,	0x48,	0x4c,	0x50,	0x56,
0x58,	0x60,	0x64,	0x68,	0x6c,	0x72,	0x50,	0x52,	0x56,	0x58,	0x5e,	0x60,	0x64,	0x6a,	0x6e,	0x72,
0x3c,	0x42,	0x48,	0x4e,	0x54,	0x5a,	0x60,	0x66,	0x6a,	0x70,	0x34,	0x3a,	0x42,	0x46,	0x4e,	0x54,
0x5a,	0x60,	0x64,	0x68,	0x2c,	0x34,	0x3a,	0x3e,	0x46,	0x4c,	0x54,	0x58,	0x5e,	0x64,	0x2c,	0x34,
0x3a,	0x40,	0x46,	0x4c,	0x54,	0x58,	0x60,	0x66,	0x34,	0x3a,	0x42,	0x46,	0x4e,	0x54,	0x58,	0x5e,
0x64,	0x6a,	0x3c,	0x42,	0x48,	0x50,	0x54,	0x5a,	0x62,	0x66,	0x6c,	0x72,	0x48,	0x4e,	0x50,	0x58,
0x5c,	0x60,	0x64,	0x68,	0x6e,	0x74,	0x52,	0x52,	0x56,	0x5a,	0x5e,	0x60,	0x66,	0x6a,	0x72,	0x76,
0x3c,	0x42,	0x48,	0x4e,	0x54,	0x5a,	0x60,	0x66,	0x6c,	0x70,	0x36,	0x3c,	0x42,	0x48,	0x4e,	0x54,
0x5a,	0x60,	0x64,	0x6a,	0x2e,	0x34,	0x3a,	0x40,	0x46,	0x4c,	0x52,	0x58,	0x5c,	0x64,	0x2e,	0x34,
0x3c,	0x40,	0x48,	0x4e,	0x52,	0x5a,	0x60,	0x66,	0x34,	0x3c,	0x42,	0x48,	0x4c,	0x52,	0x5a,	0x5e,
0x66,	0x6a,	0x3e,	0x44,	0x4a,	0x50,	0x56,	0x5a,	0x62,	0x68,	0x6e,	0x72,	0x48,	0x4c,	0x52,	0x56,
0x5a,	0x60,	0x66,	0x68,	0x6e,	0x72,	0x4e,	0x52,	0x54,	0x58,	0x5c,	0x62,	0x68,	0x6a,	0x70,	0x72,
0x3e,	0x44,	0x48,	0x4e,	0x54,	0x5a,	0x60,	0x66,	0x6c,	0x72,	0x36,	0x3c,	0x42,	0x4a,	0x50,	0x56,
0x5a,	0x60,	0x66,	0x6c,	0x2e,	0x34,	0x3c,	0x42,	0x48,	0x4e,	0x54,	0x5a,	0x60,	0x66,	0x2e,	0x36,
0x3c,	0x42,	0x4a,	0x4e,	0x54,	0x5c,	0x62,	0x66,	0x34,	0x3a,	0x42,	0x48,	0x4e,	0x54,	0x5a,	0x60,
0x66,	0x6c,	0x3e,	0x44,	0x4a,	0x50,	0x58,	0x5c,	0x60,	0x66,	0x6e,	0x72,	0x4a,	0x4e,	0x52,	0x58,
0x5c,	0x60,	0x64,	0x68,	0x6c,	0x72,	0x4e,	0x50,	0x56,	0x56,	0x5c,	0x60,	0x64,	0x6c,	0x6a,	0x74,
0x40,	0x46,	0x4a,	0x50,	0x54,	0x5a,	0x60,	0x66,	0x6a,	0x6e,	0x3a,	0x3e,	0x46,	0x4a,	0x50,	0x56,
0x5c,	0x62,	0x68,	0x6c,	0x30,	0x36,	0x3e,	0x42,	0x4a,	0x4e,	0x56,	0x5c,	0x60,	0x68,	0x30,	0x38,
0x3e,	0x44,	0x4c,	0x50,	0x56,	0x5e,	0x62,	0x68,	0x38,	0x3c,	0x44,	0x4a,	0x50,	0x56,	0x5a,	0x60,
0x66,	0x6a,	0x40,	0x46,	0x4a,	0x50,	0x56,	0x5a,	0x62,	0x66,	0x6c,	0x72,	0x4a,	0x4c,	0x52,	0x56,
0x5e,	0x60,	0x64,	0x6a,	0x6c,	0x6e,	0x4a,	0x4c,	0x52,	0x52,	0x5a,	0x60,	0x64,	0x68,	0x6e,	0x6c,
0x40,	0x46,	0x4e,	0x52,	0x56,	0x5c,	0x62,	0x66,	0x6c,	0x70,	0x3a,	0x40,	0x44,	0x4c,	0x50,	0x58,
0x5c,	0x62,	0x68,	0x6e,	0x34,	0x3a,	0x40,	0x44,	0x4e,	0x52,	0x58,	0x5c,	0x62,	0x66,	0x34,	0x3a,
0x40,	0x44,	0x4c,	0x52,	0x58,	0x5e,	0x64,	0x6a,	0x3a,	0x40,	0x44,	0x4a,	0x50,	0x56,	0x5a,	0x60,
0x66,	0x6c,	0x40,	0x46,	0x4e,	0x52,	0x58,	0x5c,	0x60,	0x66,	0x6c,	0x70,	0x4c,	0x4e,	0x52,	0x56,
0x5a,	0x60,	0x62,	0x66,	0x6c,	0x6c,	0x50,	0x44,	0x30,	0x33,	0x7e,	0xff,	0xd6,	0x0,	0x5a,	0x0 ,
0x0,	0x0,	0xdf,	0x2,	0x62,	0x6a,	0x70,	0x74,	0x7a,	0x7e,	0x84,	0x8a,	0x90,	0x96,	0xd0,	0x14,
0xb8,	0xb,	0x10,	0x0,	0x8,	0x23,	0x40,	0x40,	0x10,	0x10,	0x53,	0x2e,	0x8,	0x23,	0x3c,	0x23,
0x18,	0x27,	0x2c,	0x27,	0xc,	0x37,	0x38,	0x37,	0x1c,	0x3b,	0x28,	0x3b,	0x1c,	0x43,	0x28,	0x43,
0xc,	0x47,	0x38,	0x47,	0x18,	0x57,	0x2c,	0x57,	0x8,	0x5b,	0x3c,	0x5b,	0x8,	0x27,	0x3c,	0x27,
0x18,	0x2b,	0x2c,	0x2b,	0xc,	0x33,	0x38,	0x33,	0x1c,	0x37,	0x28,	0x37,	0x1c,	0x47,	0x28,	0x47,
0xc,	0x4b,	0x38,	0x4b,	0x18,	0x53,	0x2c,	0x53,	0x8,	0x57,	0x3c,	0x57};
char data2[4096]= {0};
#endif

static imgsensor_info_struct imgsensor_info = {
	.sensor_id = OV16880_SENSOR_ID,
	.checksum_value = 0xfe9e1a79,

	.pre = {
		.pclk = 288000000,											//record different mode's pclk
		.linelength = 2512,		/*OV16880 Note: linelength/4,it means line length per lane*/
		.framelength = 3824,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2336,		//record different mode's width of grabwindow
		.grabwindow_height = 1752,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.cap = { /* 95:line 5312, 52/35:line 5336 */
		.pclk = 576000000,
		.linelength = 5024, /*OV16880 Note: linelength/4,it means line length per lane*/
		.framelength = 3824,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4672,
		.grabwindow_height = 3504,
		.mipi_data_lp2hs_settle_dc = 105,//unit , ns
		.max_framerate = 300,
		},
	.cap1 = {
		.pclk = 288000000,
		.linelength = 5024, /*OV16880 Note: linelength/4,it means line length per lane*/
		.framelength = 3824,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4672,
		.grabwindow_height = 3504,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 150,//less than 13M(include 13M),cap1 max framerate is 24fps,16M max framerate is 20fps, 20M max framerate is 15fps  
	},
  .cap2 = {
		.pclk = 288000000,
		.linelength = 5024, /*OV16880 Note: linelength/4,it means line length per lane*/
		.framelength = 3824,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4672,
		.grabwindow_height = 3504,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 150,//less than 13M(include 13M),cap1 max framerate is 24fps,16M max framerate is 20fps, 20M max framerate is 15fps  
  },
	.normal_video = {
		.pclk = 576000000,
		.linelength = 5024, /*OV16880 Note: linelength/4,it means line length per lane*/
		.framelength = 3824,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4272,
		.grabwindow_height = 2404,
		.mipi_data_lp2hs_settle_dc = 105,//unit , ns
		.max_framerate = 300,
	},

	.hs_video = {
		.pclk = 576000000,
		.linelength = 5024,/*OV16880 Note: linelength/4,it means line length per lane*/
		.framelength = 956,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 1200,
	},

	.slim_video = {
		.pclk = 576000000,
		.linelength = 5024,/*OV16880 Note: linelength/4,it means line length per lane*/
		.framelength = 956,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 1200,
	},
	.margin = 8,
	.min_shutter = 4,
	.max_frame_length = 0x7fff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 1,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 2,
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_2MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x6c,0x20,0xff},
	.i2c_speed = 400,// i2c read/write speed
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x3D0,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x20,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{{ 4704, 3536,	   0,     8, 4704, 3520, 2352, 1760,  8, 2, 2336, 1752,	  0,	0, 2336, 1752}, // Preview
 { 4704, 3536,	   0,     8, 4704, 3520, 4704, 3520, 16, 8, 4672, 3504,	  0,	0, 4672, 3504}, // capture
 { 4704, 3536,	   0,     8, 4704, 3520, 4704, 3520, 216, 558, 4272, 2404,	  0,	0, 4272, 2404}, // video
 { 4704, 3536,	1056,  1040, 2592, 1456, 1296,  728,  8, 4, 1280,  720,	  0,	0, 1280,  720}, //hight speed video
 { 4704, 3536,	1056,  1040, 2592, 1456, 1296,  728,  8, 4, 1280,  720,	  0,	0, 1280,  720}}; // slim video

/*VC1 for HDR(N/A) , VC2 for PDAF(VC1,DT=0X2b), unit : 8bit*/
/*VC Data [0x70, 0x5DA]*/
static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
{/* Preview mode setting */
 {0x02, 0x0a,   0x00,   0x00, 0x00, 0x00,
  0x00, 0x2b, 0x0000, 0x0000, 0x01, 0x00, 0x0000, 0x0000,
  0x02, 0x00, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000},
  /* Capture mode setting */
 {0x02, 0x0a,   0x00,   0x00, 0x00, 0x00,
  0x00, 0x2b, 0x0000, 0x0000, 0x01, 0x00, 0x0000, 0x0000,
  0x01, 0x2b, 0x015E, 0x0340, 0x03, 0x00, 0x0000, 0x0000},
  /* Video mode setting */
 {0x02, 0x0a,   0x00,   0x00, 0x00, 0x00,
  0x00, 0x2b, 0x0000, 0x0000, 0x01, 0x00, 0x0000, 0x0000,
  0x02, 0x00, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000}
};

static SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
    .i4OffsetX =  96,
    .i4OffsetY = 88,
    .i4PitchX  = 32,
    .i4PitchY  = 32,
    .i4PairNum  =8,
    .i4SubBlkW  =16,
    .i4SubBlkH  =8,
    .i4PosL = {{110,94},{126,94},{102,98},{118,98},{110,110},{126,110},{102,114},{118,114}},
    .i4PosR = {{110,90},{126,90},{102,102},{118,102},{110,106},{126,106},{102,118},{118,118}},
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
//    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pusendcmd , 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

/*
static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
//    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
}
*/

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
//    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pusendcmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
    //iReadRegI2CTiming(pusendcmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
    return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
//    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pusendcmd , 3, imgsensor.i2c_write_id);
    //iWriteRegI2CTiming(pusendcmd , 3, imgsensor.i2c_write_id, imgsensor_info.i2c_speed);
}


static void set_dummy(void)
{
	 LOG_INF("dummyline = %d, dummypixels = %d ", imgsensor.dummy_line, imgsensor.dummy_pixel);
  //set vertical_total_size, means framelength
	write_cmos_sensor_8(0x380e, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);	 
	
	//set horizontal_total_size, means linelength 
	write_cmos_sensor_8(0x380c, (imgsensor.line_length) >> 8);
	write_cmos_sensor_8(0x380d, (imgsensor.line_length) & 0xFF);

}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{

	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	LOG_INF("framerate = %d, min framelength should enable = %d \n", framerate,min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	{
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */


static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;

    spin_lock(&imgsensor_drv_lock);
    if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
        imgsensor.frame_length = shutter + imgsensor_info.margin;
    else
        imgsensor.frame_length = imgsensor.min_frame_length;
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;
    spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

    if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
		} else {
			// Extend frame length
			write_cmos_sensor_8(0x380e, imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);
		}

	// Update Shutter
	write_cmos_sensor_8(0x3502, (shutter) & 0xFF);
	write_cmos_sensor_8(0x3501, (shutter >> 8) & 0x7F);	  
	//write_cmos_sensor_8(0x3500, (shutter >> 12) & 0x0F);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}	/*	write_shutter  */



/*************************************************************************
* FUNCTION
*	set_shutter
*
* DESCRIPTION
*	This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*	iShutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}	/*	set_shutter */


/*
static kal_uint16 gain2reg(const kal_uint16 gain)
{
	 kal_uint16 reg_gain = 0x0;
	
	reg_gain = ((gain / BASEGAIN) << 4) + ((gain % BASEGAIN) * 16 / BASEGAIN);
	reg_gain = reg_gain & 0xFFFF;
    //reg_gain = gain/2;lx_mark
    return (kal_uint16)reg_gain;
}
*/

/*************************************************************************
* FUNCTION
*	set_gain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*	iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	
	
	//kal_uint16 reg_gain = 0, i = 0;

	/*
	* sensor gain 1x = 128
	* max gain = 0x7ff = 15.992x <16x
	* here we just use 0x3508 analog gain 1 bit[3:2].
	* 16x~32x should use 0x3508 analog gain 0 bit[1:0]
	*/
/*
	if (gain < BASEGAIN || gain >= 16 * BASEGAIN) {
		LOG_INF("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain >= 16 * BASEGAIN)
			gain = 15.9 * BASEGAIN;		 
	}
 */
	/*reg_gain = gain2reg(gain);*/
	
/*
	gain *= 2;
	gain = (gain & 0x7ff);
		
	for(i = 1; i <= 3; i++){
		if(gain >= 0x100){
			reg_gain = reg_gain + 1;
			gain = gain/2;			
		}
	}

	reg_gain = (reg_gain << 2);
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain; 
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor_8(0x350B, gain);
	write_cmos_sensor_8(0x350A, reg_gain);
	*/

	/*
	* sensor gain 1x = 0x10
	* max gain = 0xf8 = 15.5x
	*/
	kal_uint16 reg_gain = 0;
	
	reg_gain = gain*16/BASEGAIN;
		if(reg_gain < 0x10)
		{
			reg_gain = 0x10;
		}
		if(reg_gain > 0xf8)
		{
			reg_gain = 0xf8;
		}
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain; 
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor_8(0x350a, reg_gain >> 8);
	write_cmos_sensor_8(0x350b, reg_gain & 0xFF);
	
	return gain;
}	/*	set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
#if 1
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
	if (imgsensor.ihdr_en) {

		spin_lock(&imgsensor_drv_lock);
			if (le > imgsensor.min_frame_length - imgsensor_info.margin)
				imgsensor.frame_length = le + imgsensor_info.margin;
			else
				imgsensor.frame_length = imgsensor.min_frame_length;
			if (imgsensor.frame_length > imgsensor_info.max_frame_length)
				imgsensor.frame_length = imgsensor_info.max_frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (le < imgsensor_info.min_shutter) le = imgsensor_info.min_shutter;
			if (se < imgsensor_info.min_shutter) se = imgsensor_info.min_shutter;


		// Extend frame length first
		/*
		write_cmos_sensor_8(0x380e, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);

		write_cmos_sensor_8(0x3502, (le << 4) & 0xFF);
		write_cmos_sensor_8(0x3501, (le >> 4) & 0xFF);	 
		write_cmos_sensor_8(0x3500, (le >> 12) & 0x0F);
		
		write_cmos_sensor_8(0x3508, (se << 4) & 0xFF); 
		write_cmos_sensor_8(0x3507, (se >> 4) & 0xFF);
		write_cmos_sensor_8(0x3506, (se >> 12) & 0x0F); 
		*/
		
		write_cmos_sensor_8(0x380e, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);

		write_cmos_sensor_8(0x3502, (le ) & 0xFF);
		write_cmos_sensor_8(0x3501, (le >> 8) & 0x7F);	 
		//write_cmos_sensor_8(0x3500, (le >> 12) & 0x0F);
		
		write_cmos_sensor_8(0x3508, (se ) & 0xFF); 
		write_cmos_sensor_8(0x3507, (se >> 8) & 0x7F);
		//write_cmos_sensor_8(0x3506, (se >> 12) & 0x0F); 
		
	LOG_INF("iHDR:imgsensor.frame_length=%d\n",imgsensor.frame_length);
		set_gain(gain);
	}

#endif

}


/*
static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d", image_mirror);
	spin_lock(&imgsensor_drv_lock);
    imgsensor.mirror= image_mirror;
    spin_unlock(&imgsensor_drv_lock);
    //;;flip_off_mirror_on
		//6c 3820 81
		//6c 3821 06
		//6c 4000 17
		
    switch (image_mirror) {
		case IMAGE_NORMAL:
			//write_cmos_sensor_8(0x3820,((read_cmos_sensor_8(0x3820) & 0xF9) | 0x00));
			//write_cmos_sensor_8(0x3821,((read_cmos_sensor_8(0x3821) & 0xF9) | 0x06));
			break;
		case IMAGE_H_MIRROR:
			//write_cmos_sensor_8(0x3820,((read_cmos_sensor_8(0x3820) & 0xF9) | 0x00));
			//write_cmos_sensor_8(0x3821,((read_cmos_sensor_8(0x3821) & 0xF9) | 0x00));
			break;
		case IMAGE_V_MIRROR:
			//write_cmos_sensor_8(0x3820,((read_cmos_sensor_8(0x3820) & 0xF9) | 0x06));
			//write_cmos_sensor_8(0x3821,((read_cmos_sensor_8(0x3821) & 0xF9) | 0x06));		
			break;
		case IMAGE_HV_MIRROR:
			//write_cmos_sensor_8(0x3820,((read_cmos_sensor_8(0x3820) & 0xF9) | 0x06));
			//write_cmos_sensor_8(0x3821,((read_cmos_sensor_8(0x3821) & 0xF9) | 0x00));
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
    }

}
*/

/*************************************************************************
* FUNCTION
*	night_mode
*
* DESCRIPTION
*	This function night mode of sensor.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/


static void sensor_init(void)
{
	LOG_INF("E\n");

#ifdef PDAF_TEST
	//int j = 0;
	int size=0x600;
	//memset(data, (char)0x2, 1024);
	//memset(data+1024, (char)0x3, 1024);
	//memset(data+2048, (char)0x4, 2048);
	//memset(data2, (char)0x0, 4096);
	
	wrtie_eeprom(0x0100, data, size);
	//read_eeprom(0x0000, data2, size);
	//printk("final data2 ");
	//for(j=0;j<size;j++)
	//	printk(" %d\n",data2[j]);
	//printk("\n");
#endif

	write_cmos_sensor_8(0x301e, 0x00);
	write_cmos_sensor_8(0x0103, 0x01); 
	mDELAY(2); //Delay 1ms
	write_cmos_sensor_8(0x0300, 0x00);
	write_cmos_sensor_8(0x0303, 0x00);
	write_cmos_sensor_8(0x0304, 0x07);
	write_cmos_sensor_8(0x030e, 0x02);
	write_cmos_sensor_8(0x0312, 0x03);
	write_cmos_sensor_8(0x0313, 0x03);
	write_cmos_sensor_8(0x031e, 0x09);
	write_cmos_sensor_8(0x3002, 0x00);
	write_cmos_sensor_8(0x3009, 0x06);
	write_cmos_sensor_8(0x3010, 0x00);
	write_cmos_sensor_8(0x3011, 0x04);
	write_cmos_sensor_8(0x3012, 0x41);
	write_cmos_sensor_8(0x3013, 0xcc);
	write_cmos_sensor_8(0x3017, 0xf0);
	write_cmos_sensor_8(0x3018, 0xda);
	write_cmos_sensor_8(0x3019, 0xf1);
	write_cmos_sensor_8(0x301a, 0xf2);
	write_cmos_sensor_8(0x301b, 0x96);
	write_cmos_sensor_8(0x301d, 0x02);
	write_cmos_sensor_8(0x3022, 0x0e);
	write_cmos_sensor_8(0x3023, 0xb0);
	write_cmos_sensor_8(0x3028, 0xc3);
	write_cmos_sensor_8(0x3031, 0x68);
	write_cmos_sensor_8(0x3400, 0x08);
	write_cmos_sensor_8(0x3507, 0x02);
	write_cmos_sensor_8(0x3508, 0x00);
	write_cmos_sensor_8(0x3509, 0x12);
	write_cmos_sensor_8(0x350a, 0x00);
	write_cmos_sensor_8(0x350b, 0x40);
	write_cmos_sensor_8(0x3549, 0x12);
	write_cmos_sensor_8(0x354e, 0x00);
	write_cmos_sensor_8(0x354f, 0x10);
	write_cmos_sensor_8(0x3600, 0x12);
	write_cmos_sensor_8(0x3603, 0xc0);
	write_cmos_sensor_8(0x3604, 0x2c);
	write_cmos_sensor_8(0x3606, 0x55);
	write_cmos_sensor_8(0x3607, 0x05);
	write_cmos_sensor_8(0x360a, 0x6d);
	write_cmos_sensor_8(0x360d, 0x05);
	write_cmos_sensor_8(0x360e, 0x07);
	write_cmos_sensor_8(0x360f, 0x44);
	write_cmos_sensor_8(0x3622, 0x79);
	write_cmos_sensor_8(0x3623, 0x57);
	write_cmos_sensor_8(0x3624, 0x50);
	write_cmos_sensor_8(0x362b, 0x77);
	write_cmos_sensor_8(0x3641, 0x01);
	write_cmos_sensor_8(0x3660, 0xc0);
	write_cmos_sensor_8(0x3661, 0x04);
	write_cmos_sensor_8(0x3661, 0x04);
	write_cmos_sensor_8(0x3662, 0x00);
	write_cmos_sensor_8(0x3663, 0x20);
	write_cmos_sensor_8(0x3664, 0x08);
	write_cmos_sensor_8(0x3667, 0x00);
	write_cmos_sensor_8(0x366a, 0x54);
	write_cmos_sensor_8(0x366c, 0x10);
	write_cmos_sensor_8(0x3708, 0x42);// ;new add 01172016
	write_cmos_sensor_8(0x3709, 0x1c);
	write_cmos_sensor_8(0x3718, 0x8c);
	write_cmos_sensor_8(0x3719, 0x00);
	write_cmos_sensor_8(0x371a, 0x04);
	write_cmos_sensor_8(0x3725, 0xf0);
	write_cmos_sensor_8(0x3728, 0x22);
	write_cmos_sensor_8(0x372a, 0x20);
	write_cmos_sensor_8(0x372b, 0x40);
	write_cmos_sensor_8(0x3730, 0x00);
	write_cmos_sensor_8(0x3731, 0x00);
	write_cmos_sensor_8(0x3732, 0x00);
	write_cmos_sensor_8(0x3733, 0x00);
	write_cmos_sensor_8(0x3748, 0x00);
	write_cmos_sensor_8(0x3767, 0x30);// ;new add 01172016
	write_cmos_sensor_8(0x3772, 0x00);
	write_cmos_sensor_8(0x3773, 0x00);
	write_cmos_sensor_8(0x3774, 0x40);
	write_cmos_sensor_8(0x3775, 0x81);
	write_cmos_sensor_8(0x3776, 0x31);
	write_cmos_sensor_8(0x3777, 0x06);
	write_cmos_sensor_8(0x3778, 0xa0);
	write_cmos_sensor_8(0x377f, 0x31);
	write_cmos_sensor_8(0x378d, 0x39);
	write_cmos_sensor_8(0x3790, 0xcc);
	write_cmos_sensor_8(0x3791, 0xcc);
	write_cmos_sensor_8(0x379c, 0x02);
	write_cmos_sensor_8(0x379d, 0x00);
	write_cmos_sensor_8(0x379e, 0x00);
	write_cmos_sensor_8(0x379f, 0x1e);
	write_cmos_sensor_8(0x37a0, 0x22);
	write_cmos_sensor_8(0x37a4, 0x00);
	write_cmos_sensor_8(0x37b0, 0x48);
	write_cmos_sensor_8(0x37b3, 0x62);
	write_cmos_sensor_8(0x37b6, 0x22);
	write_cmos_sensor_8(0x37b9, 0x23);
	write_cmos_sensor_8(0x37c3, 0x07);
	write_cmos_sensor_8(0x37c6, 0x35);
	write_cmos_sensor_8(0x37c8, 0x00);
	write_cmos_sensor_8(0x37c9, 0x00);
	write_cmos_sensor_8(0x37cc, 0x93);
	write_cmos_sensor_8(0x3810, 0x00);
	write_cmos_sensor_8(0x3812, 0x00);
	write_cmos_sensor_8(0x3837, 0x02);
	write_cmos_sensor_8(0x3d85, 0x17);
	write_cmos_sensor_8(0x3d8c, 0x79);
	write_cmos_sensor_8(0x3d8d, 0x7f);
	write_cmos_sensor_8(0x3f00, 0x50);
	write_cmos_sensor_8(0x3f85, 0x00);
	write_cmos_sensor_8(0x3f86, 0x00);
	write_cmos_sensor_8(0x3f87, 0x05);
	write_cmos_sensor_8(0x3f9e, 0x00);
	write_cmos_sensor_8(0x3f9f, 0x00);
	write_cmos_sensor_8(0x4000, 0x17);
	write_cmos_sensor_8(0x4001, 0x60);
	write_cmos_sensor_8(0x4003, 0x40);
	write_cmos_sensor_8(0x4008, 0x00);
	write_cmos_sensor_8(0x400f, 0x09);
	write_cmos_sensor_8(0x4010, 0xf0);
	write_cmos_sensor_8(0x4011, 0xf0);
	write_cmos_sensor_8(0x4013, 0x02);
	write_cmos_sensor_8(0x4018, 0x04);
	write_cmos_sensor_8(0x4019, 0x04);
	write_cmos_sensor_8(0x401a, 0x48);
	write_cmos_sensor_8(0x4020, 0x08);
	write_cmos_sensor_8(0x4022, 0x08);
	write_cmos_sensor_8(0x4024, 0x08);
	write_cmos_sensor_8(0x4026, 0x08);
	write_cmos_sensor_8(0x4028, 0x08);
	write_cmos_sensor_8(0x402a, 0x08);
	write_cmos_sensor_8(0x402c, 0x08);
	write_cmos_sensor_8(0x402e, 0x08);
	write_cmos_sensor_8(0x4030, 0x08);
	write_cmos_sensor_8(0x4032, 0x08);
	write_cmos_sensor_8(0x4034, 0x08);
	write_cmos_sensor_8(0x4036, 0x08);
	write_cmos_sensor_8(0x4038, 0x08);
	write_cmos_sensor_8(0x403a, 0x08);
	write_cmos_sensor_8(0x403c, 0x08);
	write_cmos_sensor_8(0x403e, 0x08);
	write_cmos_sensor_8(0x4040, 0x00);
	write_cmos_sensor_8(0x4041, 0x00);
	write_cmos_sensor_8(0x4042, 0x00);
	write_cmos_sensor_8(0x4043, 0x00);
	write_cmos_sensor_8(0x4044, 0x00);
	write_cmos_sensor_8(0x4045, 0x00);
	write_cmos_sensor_8(0x4046, 0x00);
	write_cmos_sensor_8(0x4047, 0x00);
	write_cmos_sensor_8(0x40b0, 0x00);
	write_cmos_sensor_8(0x40b1, 0x00);
	write_cmos_sensor_8(0x40b2, 0x00);
	write_cmos_sensor_8(0x40b3, 0x00);
	write_cmos_sensor_8(0x40b4, 0x00);
	write_cmos_sensor_8(0x40b5, 0x00);
	write_cmos_sensor_8(0x40b6, 0x00);
	write_cmos_sensor_8(0x40b7, 0x00);
	write_cmos_sensor_8(0x4052, 0x00);
	write_cmos_sensor_8(0x4053, 0x80);
	write_cmos_sensor_8(0x4054, 0x00);
	write_cmos_sensor_8(0x4055, 0x80);
	write_cmos_sensor_8(0x4056, 0x00);
	write_cmos_sensor_8(0x4057, 0x80);
	write_cmos_sensor_8(0x4058, 0x00);
	write_cmos_sensor_8(0x4059, 0x80);
	write_cmos_sensor_8(0x4202, 0x00);
	write_cmos_sensor_8(0x4203, 0x00);
	write_cmos_sensor_8(0x4066, 0x02);
	write_cmos_sensor_8(0x4509, 0x07);
	write_cmos_sensor_8(0x4605, 0x00);
	write_cmos_sensor_8(0x4641, 0x1e);
	write_cmos_sensor_8(0x4645, 0x00);
	write_cmos_sensor_8(0x4800, 0x04);
	write_cmos_sensor_8(0x4809, 0x2b);
	write_cmos_sensor_8(0x4812, 0x2b);
	write_cmos_sensor_8(0x4813, 0x90);
	write_cmos_sensor_8(0x4833, 0x18);
	write_cmos_sensor_8(0x484b, 0x01);
	write_cmos_sensor_8(0x4850, 0x7c);
	write_cmos_sensor_8(0x4852, 0x06);
	write_cmos_sensor_8(0x4856, 0x58);
	write_cmos_sensor_8(0x4857, 0xaa);
	write_cmos_sensor_8(0x4862, 0x0a);
	write_cmos_sensor_8(0x4867, 0x02);
	write_cmos_sensor_8(0x4869, 0x18);
	write_cmos_sensor_8(0x486a, 0x6a);
	write_cmos_sensor_8(0x486e, 0x03);
	write_cmos_sensor_8(0x486f, 0x55);
	write_cmos_sensor_8(0x4875, 0xf0);
	write_cmos_sensor_8(0x4b05, 0x93);
	write_cmos_sensor_8(0x4b06, 0x00);
	write_cmos_sensor_8(0x4c01, 0x5f);
	write_cmos_sensor_8(0x4d00, 0x04);
	write_cmos_sensor_8(0x4d01, 0x22);
	write_cmos_sensor_8(0x4d02, 0xb7);
	write_cmos_sensor_8(0x4d03, 0xca);
	write_cmos_sensor_8(0x4d04, 0x30);
	write_cmos_sensor_8(0x4d05, 0x1d);
	write_cmos_sensor_8(0x4f00, 0x1c);
	write_cmos_sensor_8(0x4f03, 0x50);
	write_cmos_sensor_8(0x4f04, 0x01);
	write_cmos_sensor_8(0x4f05, 0x7c);
	write_cmos_sensor_8(0x4f08, 0x00);
	write_cmos_sensor_8(0x4f09, 0x60);
	write_cmos_sensor_8(0x4f0a, 0x00);
	write_cmos_sensor_8(0x4f0b, 0x30);
	write_cmos_sensor_8(0x4f14, 0xf0);
	write_cmos_sensor_8(0x4f15, 0xf0);
	write_cmos_sensor_8(0x4f16, 0xf0);
	write_cmos_sensor_8(0x4f17, 0xf0);
	write_cmos_sensor_8(0x4f18, 0xf0);
	write_cmos_sensor_8(0x4f19, 0x00);
	write_cmos_sensor_8(0x4f1a, 0x00);
	write_cmos_sensor_8(0x4f20, 0x07);
	write_cmos_sensor_8(0x4f21, 0xd0);
	write_cmos_sensor_8(0x5000, 0x9b);//;disable LSC
	write_cmos_sensor_8(0x5001, 0x4e);
	write_cmos_sensor_8(0x5002, 0x0c);
	write_cmos_sensor_8(0x501d, 0x00);
	write_cmos_sensor_8(0x5020, 0x03);
	write_cmos_sensor_8(0x5022, 0x91);
	write_cmos_sensor_8(0x5023, 0x00);
	write_cmos_sensor_8(0x5030, 0x00);
	write_cmos_sensor_8(0x5056, 0x00);
	write_cmos_sensor_8(0x5063, 0x00);
	write_cmos_sensor_8(0x5200, 0x02);
	write_cmos_sensor_8(0x5202, 0x00);
	write_cmos_sensor_8(0x5204, 0x00);
	write_cmos_sensor_8(0x5209, 0x81);
	write_cmos_sensor_8(0x520e, 0x31);
	write_cmos_sensor_8(0x5280, 0x00);
	write_cmos_sensor_8(0x5400, 0x01);
	write_cmos_sensor_8(0x5401, 0x00);
	write_cmos_sensor_8(0x5504, 0x00);
	write_cmos_sensor_8(0x5505, 0x00);
	write_cmos_sensor_8(0x5506, 0x00);
	write_cmos_sensor_8(0x5507, 0x00);
	write_cmos_sensor_8(0x550c, 0x00);
	write_cmos_sensor_8(0x550d, 0x00);
	write_cmos_sensor_8(0x550e, 0x00);
	write_cmos_sensor_8(0x550f, 0x00);
	write_cmos_sensor_8(0x5514, 0x00);
	write_cmos_sensor_8(0x5515, 0x00);
	write_cmos_sensor_8(0x5516, 0x00);
	write_cmos_sensor_8(0x5517, 0x00);
	write_cmos_sensor_8(0x551c, 0x00);
	write_cmos_sensor_8(0x551d, 0x00);
	write_cmos_sensor_8(0x551e, 0x00);
	write_cmos_sensor_8(0x551f, 0x00);
	write_cmos_sensor_8(0x5524, 0x00);
	write_cmos_sensor_8(0x5525, 0x00);
	write_cmos_sensor_8(0x5526, 0x00);
	write_cmos_sensor_8(0x5527, 0x00);
	write_cmos_sensor_8(0x552c, 0x00);
	write_cmos_sensor_8(0x552d, 0x00);
	write_cmos_sensor_8(0x552e, 0x00);
	write_cmos_sensor_8(0x552f, 0x00);
	write_cmos_sensor_8(0x5534, 0x00);
	write_cmos_sensor_8(0x5535, 0x00);
	write_cmos_sensor_8(0x5536, 0x00);
	write_cmos_sensor_8(0x5537, 0x00);
	write_cmos_sensor_8(0x553c, 0x00);
	write_cmos_sensor_8(0x553d, 0x00);
	write_cmos_sensor_8(0x553e, 0x00);
	write_cmos_sensor_8(0x553f, 0x00);
	write_cmos_sensor_8(0x5544, 0x00);
	write_cmos_sensor_8(0x5545, 0x00);
	write_cmos_sensor_8(0x5546, 0x00);
	write_cmos_sensor_8(0x5547, 0x00);
	write_cmos_sensor_8(0x554c, 0x00);
	write_cmos_sensor_8(0x554d, 0x00);
	write_cmos_sensor_8(0x554e, 0x00);
	write_cmos_sensor_8(0x554f, 0x00);
	write_cmos_sensor_8(0x5554, 0x00);
	write_cmos_sensor_8(0x5555, 0x00);
	write_cmos_sensor_8(0x5556, 0x00);
	write_cmos_sensor_8(0x5557, 0x00);
	write_cmos_sensor_8(0x555c, 0x00);
	write_cmos_sensor_8(0x555d, 0x00);
	write_cmos_sensor_8(0x555e, 0x00);
	write_cmos_sensor_8(0x555f, 0x00);
	write_cmos_sensor_8(0x5564, 0x00);
	write_cmos_sensor_8(0x5565, 0x00);
	write_cmos_sensor_8(0x5566, 0x00);
	write_cmos_sensor_8(0x5567, 0x00);
	write_cmos_sensor_8(0x556c, 0x00);
	write_cmos_sensor_8(0x556d, 0x00);
	write_cmos_sensor_8(0x556e, 0x00);
	write_cmos_sensor_8(0x556f, 0x00);
	write_cmos_sensor_8(0x5574, 0x00);
	write_cmos_sensor_8(0x5575, 0x00);
	write_cmos_sensor_8(0x5576, 0x00);
	write_cmos_sensor_8(0x5577, 0x00);
	write_cmos_sensor_8(0x557c, 0x00);
	write_cmos_sensor_8(0x557d, 0x00);
	write_cmos_sensor_8(0x557e, 0x00);
	write_cmos_sensor_8(0x557f, 0x00);
	write_cmos_sensor_8(0x5584, 0x00);
	write_cmos_sensor_8(0x5585, 0x00);
	write_cmos_sensor_8(0x5586, 0x00);
	write_cmos_sensor_8(0x5587, 0x00);
	write_cmos_sensor_8(0x558c, 0x00);
	write_cmos_sensor_8(0x558d, 0x00);
	write_cmos_sensor_8(0x558e, 0x00);
	write_cmos_sensor_8(0x558f, 0x00);
	write_cmos_sensor_8(0x5594, 0x00);
	write_cmos_sensor_8(0x5595, 0x00);
	write_cmos_sensor_8(0x5596, 0x00);
	write_cmos_sensor_8(0x5597, 0x00);
	write_cmos_sensor_8(0x559c, 0x00);
	write_cmos_sensor_8(0x559d, 0x00);
	write_cmos_sensor_8(0x559e, 0x00);
	write_cmos_sensor_8(0x559f, 0x00);
	write_cmos_sensor_8(0x55a4, 0x00);
	write_cmos_sensor_8(0x55a5, 0x00);
	write_cmos_sensor_8(0x55a6, 0x00);
	write_cmos_sensor_8(0x55a7, 0x00);
	write_cmos_sensor_8(0x55ac, 0x00);
	write_cmos_sensor_8(0x55ad, 0x00);
	write_cmos_sensor_8(0x55ae, 0x00);
	write_cmos_sensor_8(0x55af, 0x00);
	write_cmos_sensor_8(0x55b4, 0x00);
	write_cmos_sensor_8(0x55b5, 0x00);
	write_cmos_sensor_8(0x55b6, 0x00);
	write_cmos_sensor_8(0x55b7, 0x00);
	write_cmos_sensor_8(0x55bc, 0x00);
	write_cmos_sensor_8(0x55bd, 0x00);
	write_cmos_sensor_8(0x55be, 0x00);
	write_cmos_sensor_8(0x55bf, 0x00);
	write_cmos_sensor_8(0x55c4, 0x00);
	write_cmos_sensor_8(0x55c5, 0x00);
	write_cmos_sensor_8(0x55c6, 0x00);
	write_cmos_sensor_8(0x55c7, 0x00);
	write_cmos_sensor_8(0x55cc, 0x00);
	write_cmos_sensor_8(0x55cd, 0x00);
	write_cmos_sensor_8(0x55ce, 0x00);
	write_cmos_sensor_8(0x55cf, 0x00);
	write_cmos_sensor_8(0x55d4, 0x00);
	write_cmos_sensor_8(0x55d5, 0x00);
	write_cmos_sensor_8(0x55d6, 0x00);
	write_cmos_sensor_8(0x55d7, 0x00);
	write_cmos_sensor_8(0x55dc, 0x00);
	write_cmos_sensor_8(0x55dd, 0x00);
	write_cmos_sensor_8(0x55de, 0x00);
	write_cmos_sensor_8(0x55df, 0x00);
	write_cmos_sensor_8(0x55e4, 0x00);
	write_cmos_sensor_8(0x55e5, 0x00);
	write_cmos_sensor_8(0x55e6, 0x00);
	write_cmos_sensor_8(0x55e7, 0x00);
	write_cmos_sensor_8(0x55ec, 0x00);
	write_cmos_sensor_8(0x55ed, 0x00);
	write_cmos_sensor_8(0x55ee, 0x00);
	write_cmos_sensor_8(0x55ef, 0x00);
	write_cmos_sensor_8(0x55f4, 0x00);
	write_cmos_sensor_8(0x55f5, 0x00);
	write_cmos_sensor_8(0x55f6, 0x00);
	write_cmos_sensor_8(0x55f7, 0x00);
	write_cmos_sensor_8(0x55fc, 0x00);
	write_cmos_sensor_8(0x55fd, 0x00);
	write_cmos_sensor_8(0x55fe, 0x00);
	write_cmos_sensor_8(0x55ff, 0x00);
	write_cmos_sensor_8(0x5600, 0x30);
	write_cmos_sensor_8(0x560f, 0xfc);
	write_cmos_sensor_8(0x5610, 0xf0);
	write_cmos_sensor_8(0x5611, 0x10);
	write_cmos_sensor_8(0x562f, 0xfc);
	write_cmos_sensor_8(0x5630, 0xf0);
	write_cmos_sensor_8(0x5631, 0x10);
	write_cmos_sensor_8(0x564f, 0xfc);
	write_cmos_sensor_8(0x5650, 0xf0);
	write_cmos_sensor_8(0x5651, 0x10);
	//write_cmos_sensor_8(0x566f, 0xfc);
	write_cmos_sensor_8(0x5670, 0xf0);
	write_cmos_sensor_8(0x5671, 0x10);
	write_cmos_sensor_8(0x5694, 0xc0);
	write_cmos_sensor_8(0x5695, 0x00);
	write_cmos_sensor_8(0x5696, 0x00);
	write_cmos_sensor_8(0x5697, 0x08);
	write_cmos_sensor_8(0x5698, 0x00);
	write_cmos_sensor_8(0x5699, 0x70);
	write_cmos_sensor_8(0x569a, 0x11);
	write_cmos_sensor_8(0x569b, 0xf0);
	write_cmos_sensor_8(0x569c, 0x00);
	write_cmos_sensor_8(0x569d, 0x68);
	write_cmos_sensor_8(0x569e, 0x0d);
	write_cmos_sensor_8(0x569f, 0x68);
	write_cmos_sensor_8(0x56a1, 0xff);
	write_cmos_sensor_8(0x5bd9, 0x01);
	write_cmos_sensor_8(0x5d04, 0x01);
	write_cmos_sensor_8(0x5d05, 0x00);
	write_cmos_sensor_8(0x5d06, 0x01);
	write_cmos_sensor_8(0x5d07, 0x00);
	write_cmos_sensor_8(0x5d12, 0x38);
	write_cmos_sensor_8(0x5d13, 0x38);
	write_cmos_sensor_8(0x5d14, 0x38);
	write_cmos_sensor_8(0x5d15, 0x38);
	write_cmos_sensor_8(0x5d16, 0x38);
	write_cmos_sensor_8(0x5d17, 0x38);
	write_cmos_sensor_8(0x5d18, 0x38);
	write_cmos_sensor_8(0x5d19, 0x38);
	write_cmos_sensor_8(0x5d1a, 0x38);
	write_cmos_sensor_8(0x5d1b, 0x38);
	write_cmos_sensor_8(0x5d1c, 0x00);
	write_cmos_sensor_8(0x5d1d, 0x00);
	write_cmos_sensor_8(0x5d1e, 0x00);
	write_cmos_sensor_8(0x5d1f, 0x00);
	write_cmos_sensor_8(0x5d20, 0x16);
	write_cmos_sensor_8(0x5d21, 0x20);
	write_cmos_sensor_8(0x5d22, 0x10);
	write_cmos_sensor_8(0x5d23, 0xa0);
	write_cmos_sensor_8(0x5d28, 0xc0);
	write_cmos_sensor_8(0x5d29, 0x80);
	write_cmos_sensor_8(0x5d2a, 0x00);
	write_cmos_sensor_8(0x5d2b, 0x70);
	write_cmos_sensor_8(0x5d2c, 0x11);
	write_cmos_sensor_8(0x5d2d, 0xf0);
	write_cmos_sensor_8(0x5d2e, 0x00);
	write_cmos_sensor_8(0x5d2f, 0x68);
	write_cmos_sensor_8(0x5d30, 0x0d);
	write_cmos_sensor_8(0x5d31, 0x68);
	write_cmos_sensor_8(0x5d32, 0x00);
	write_cmos_sensor_8(0x5d38, 0x70);
	write_cmos_sensor_8(0x5c80, 0x06);
	write_cmos_sensor_8(0x5c81, 0x90);
	write_cmos_sensor_8(0x5c82, 0x09);
	write_cmos_sensor_8(0x5c83, 0x5f);
	write_cmos_sensor_8(0x5c85, 0x6c);
	write_cmos_sensor_8(0x5601, 0x04);
	write_cmos_sensor_8(0x5602, 0x02);
	write_cmos_sensor_8(0x5603, 0x01);
	write_cmos_sensor_8(0x5604, 0x04);
	write_cmos_sensor_8(0x5605, 0x02);
	write_cmos_sensor_8(0x5606, 0x01);
	write_cmos_sensor_8(0x5b80, 0x00);
	write_cmos_sensor_8(0x5b81, 0x04);
	write_cmos_sensor_8(0x5b82, 0x00);
	write_cmos_sensor_8(0x5b83, 0x08);
	write_cmos_sensor_8(0x5b84, 0x00);
	write_cmos_sensor_8(0x5b85, 0x0c);
	write_cmos_sensor_8(0x5b86, 0x00);
	write_cmos_sensor_8(0x5b87, 0x16);
	write_cmos_sensor_8(0x5b88, 0x00);
	write_cmos_sensor_8(0x5b89, 0x28);
	write_cmos_sensor_8(0x5b8a, 0x00);
	write_cmos_sensor_8(0x5b8b, 0x40);
	write_cmos_sensor_8(0x5b8c, 0x1a);
	write_cmos_sensor_8(0x5b8d, 0x1a);
	write_cmos_sensor_8(0x5b8e, 0x1a);
	write_cmos_sensor_8(0x5b8f, 0x1a);
	write_cmos_sensor_8(0x5b90, 0x1a);
	write_cmos_sensor_8(0x5b91, 0x1a);
	write_cmos_sensor_8(0x5b92, 0x1a);
	write_cmos_sensor_8(0x5b93, 0x1a);
	write_cmos_sensor_8(0x5b94, 0x1a);
	write_cmos_sensor_8(0x5b95, 0x1a);
	write_cmos_sensor_8(0x5b96, 0x1a);
	write_cmos_sensor_8(0x5b97, 0x1a);
	write_cmos_sensor_8(0x2800, 0x60);
	write_cmos_sensor_8(0x2801, 0x4c);
	write_cmos_sensor_8(0x2802, 0x45);
	write_cmos_sensor_8(0x2803, 0x41);
	write_cmos_sensor_8(0x2804, 0x3b);
	write_cmos_sensor_8(0x2805, 0x3a);
	write_cmos_sensor_8(0x2806, 0x39);
	write_cmos_sensor_8(0x2807, 0x37);
	write_cmos_sensor_8(0x2808, 0x36);
	write_cmos_sensor_8(0x2809, 0x38);
	write_cmos_sensor_8(0x280a, 0x3b);
	write_cmos_sensor_8(0x280b, 0x3d);
	write_cmos_sensor_8(0x280c, 0x43);
	write_cmos_sensor_8(0x280d, 0x4a);
	write_cmos_sensor_8(0x280e, 0x52);
	write_cmos_sensor_8(0x280f, 0x61);
	write_cmos_sensor_8(0x2810, 0x47);
	write_cmos_sensor_8(0x2811, 0x37);
	write_cmos_sensor_8(0x2812, 0x33);
	write_cmos_sensor_8(0x2813, 0x2e);
	write_cmos_sensor_8(0x2814, 0x2b);
	write_cmos_sensor_8(0x2815, 0x27);
	write_cmos_sensor_8(0x2816, 0x26);
	write_cmos_sensor_8(0x2817, 0x26);
	write_cmos_sensor_8(0x2818, 0x26);
	write_cmos_sensor_8(0x2819, 0x27);
	write_cmos_sensor_8(0x281a, 0x29);
	write_cmos_sensor_8(0x281b, 0x2c);
	write_cmos_sensor_8(0x281c, 0x30);
	write_cmos_sensor_8(0x281d, 0x35);
	write_cmos_sensor_8(0x281e, 0x3b);
	write_cmos_sensor_8(0x281f, 0x4c);
	write_cmos_sensor_8(0x2820, 0x3a);
	write_cmos_sensor_8(0x2821, 0x2d);
	write_cmos_sensor_8(0x2822, 0x29);
	write_cmos_sensor_8(0x2823, 0x24);
	write_cmos_sensor_8(0x2824, 0x20);
	write_cmos_sensor_8(0x2825, 0x1d);
	write_cmos_sensor_8(0x2826, 0x1b);
	write_cmos_sensor_8(0x2827, 0x1a);
	write_cmos_sensor_8(0x2828, 0x1b);
	write_cmos_sensor_8(0x2829, 0x1c);
	write_cmos_sensor_8(0x282a, 0x1e);
	write_cmos_sensor_8(0x282b, 0x21);
	write_cmos_sensor_8(0x282c, 0x26);
	write_cmos_sensor_8(0x282d, 0x2b);
	write_cmos_sensor_8(0x282e, 0x30);
	write_cmos_sensor_8(0x282f, 0x3f);
	write_cmos_sensor_8(0x2830, 0x2f);
	write_cmos_sensor_8(0x2831, 0x24);
	write_cmos_sensor_8(0x2832, 0x20);
	write_cmos_sensor_8(0x2833, 0x1a);
	write_cmos_sensor_8(0x2834, 0x16);
	write_cmos_sensor_8(0x2835, 0x14);
	write_cmos_sensor_8(0x2836, 0x12);
	write_cmos_sensor_8(0x2837, 0x11);
	write_cmos_sensor_8(0x2838, 0x11);
	write_cmos_sensor_8(0x2839, 0x13);
	write_cmos_sensor_8(0x283a, 0x15);
	write_cmos_sensor_8(0x283b, 0x18);
	write_cmos_sensor_8(0x283c, 0x1c);
	write_cmos_sensor_8(0x283d, 0x22);
	write_cmos_sensor_8(0x283e, 0x27);
	write_cmos_sensor_8(0x283f, 0x34);
	write_cmos_sensor_8(0x2840, 0x28);
	write_cmos_sensor_8(0x2841, 0x1d);
	write_cmos_sensor_8(0x2842, 0x18);
	write_cmos_sensor_8(0x2843, 0x13);
	write_cmos_sensor_8(0x2844, 0x0f);
	write_cmos_sensor_8(0x2845, 0x0d);
	write_cmos_sensor_8(0x2846, 0x0b);
	write_cmos_sensor_8(0x2847, 0x0a);
	write_cmos_sensor_8(0x2848, 0x0a);
	write_cmos_sensor_8(0x2849, 0x0b);
	write_cmos_sensor_8(0x284a, 0x0e);
	write_cmos_sensor_8(0x284b, 0x11);
	write_cmos_sensor_8(0x284c, 0x15);
	write_cmos_sensor_8(0x284d, 0x1a);
	write_cmos_sensor_8(0x284e, 0x20);
	write_cmos_sensor_8(0x284f, 0x2c);
	write_cmos_sensor_8(0x2850, 0x21);
	write_cmos_sensor_8(0x2851, 0x17);
	write_cmos_sensor_8(0x2852, 0x12);
	write_cmos_sensor_8(0x2853, 0x0e);
	write_cmos_sensor_8(0x2854, 0x0b);
	write_cmos_sensor_8(0x2855, 0x08);
	write_cmos_sensor_8(0x2856, 0x06);
	write_cmos_sensor_8(0x2857, 0x05);
	write_cmos_sensor_8(0x2858, 0x05);
	write_cmos_sensor_8(0x2859, 0x07);
	write_cmos_sensor_8(0x285a, 0x09);
	write_cmos_sensor_8(0x285b, 0x0c);
	write_cmos_sensor_8(0x285c, 0x10);
	write_cmos_sensor_8(0x285d, 0x15);
	write_cmos_sensor_8(0x285e, 0x1a);
	write_cmos_sensor_8(0x285f, 0x26);
	write_cmos_sensor_8(0x2860, 0x1d);
	write_cmos_sensor_8(0x2861, 0x14);
	write_cmos_sensor_8(0x2862, 0x10);
	write_cmos_sensor_8(0x2863, 0x0b);
	write_cmos_sensor_8(0x2864, 0x07);
	write_cmos_sensor_8(0x2865, 0x05);
	write_cmos_sensor_8(0x2866, 0x03);
	write_cmos_sensor_8(0x2867, 0x02);
	write_cmos_sensor_8(0x2868, 0x02);
	write_cmos_sensor_8(0x2869, 0x03);
	write_cmos_sensor_8(0x286a, 0x05);
	write_cmos_sensor_8(0x286b, 0x08);
	write_cmos_sensor_8(0x286c, 0x0c);
	write_cmos_sensor_8(0x286d, 0x12);
	write_cmos_sensor_8(0x286e, 0x17);
	write_cmos_sensor_8(0x286f, 0x22);
	write_cmos_sensor_8(0x2870, 0x1c);
	write_cmos_sensor_8(0x2871, 0x13);
	write_cmos_sensor_8(0x2872, 0x0e);
	write_cmos_sensor_8(0x2873, 0x09);
	write_cmos_sensor_8(0x2874, 0x06);
	write_cmos_sensor_8(0x2875, 0x03);
	write_cmos_sensor_8(0x2876, 0x01);
	write_cmos_sensor_8(0x2877, 0x01);
	write_cmos_sensor_8(0x2878, 0x01);
	write_cmos_sensor_8(0x2879, 0x02);
	write_cmos_sensor_8(0x287a, 0x04);
	write_cmos_sensor_8(0x287b, 0x06);
	write_cmos_sensor_8(0x287c, 0x0a);
	write_cmos_sensor_8(0x287d, 0x10);
	write_cmos_sensor_8(0x287e, 0x15);
	write_cmos_sensor_8(0x287f, 0x21);
	write_cmos_sensor_8(0x2880, 0x1c);
	write_cmos_sensor_8(0x2881, 0x13);
	write_cmos_sensor_8(0x2882, 0x0e);
	write_cmos_sensor_8(0x2883, 0x09);
	write_cmos_sensor_8(0x2884, 0x06);
	write_cmos_sensor_8(0x2885, 0x03);
	write_cmos_sensor_8(0x2886, 0x01);
	write_cmos_sensor_8(0x2887, 0x01);
	write_cmos_sensor_8(0x2888, 0x01);
	write_cmos_sensor_8(0x2889, 0x02);
	write_cmos_sensor_8(0x288a, 0x03);
	write_cmos_sensor_8(0x288b, 0x06);
	write_cmos_sensor_8(0x288c, 0x0a);
	write_cmos_sensor_8(0x288d, 0x0f);
	write_cmos_sensor_8(0x288e, 0x15);
	write_cmos_sensor_8(0x288f, 0x20);
	write_cmos_sensor_8(0x2890, 0x1e);
	write_cmos_sensor_8(0x2891, 0x15);
	write_cmos_sensor_8(0x2892, 0x10);
	write_cmos_sensor_8(0x2893, 0x0b);
	write_cmos_sensor_8(0x2894, 0x08);
	write_cmos_sensor_8(0x2895, 0x05);
	write_cmos_sensor_8(0x2896, 0x03);
	write_cmos_sensor_8(0x2897, 0x02);
	write_cmos_sensor_8(0x2898, 0x03);
	write_cmos_sensor_8(0x2899, 0x04);
	write_cmos_sensor_8(0x289a, 0x06);
	write_cmos_sensor_8(0x289b, 0x08);
	write_cmos_sensor_8(0x289c, 0x0c);
	write_cmos_sensor_8(0x289d, 0x11);
	write_cmos_sensor_8(0x289e, 0x16);
	write_cmos_sensor_8(0x289f, 0x21);
	write_cmos_sensor_8(0x28a0, 0x22);
	write_cmos_sensor_8(0x28a1, 0x18);
	write_cmos_sensor_8(0x28a2, 0x13);
	write_cmos_sensor_8(0x28a3, 0x0f);
	write_cmos_sensor_8(0x28a4, 0x0b);
	write_cmos_sensor_8(0x28a5, 0x09);
	write_cmos_sensor_8(0x28a6, 0x07);
	write_cmos_sensor_8(0x28a7, 0x06);
	write_cmos_sensor_8(0x28a8, 0x06);
	write_cmos_sensor_8(0x28a9, 0x07);
	write_cmos_sensor_8(0x28aa, 0x09);
	write_cmos_sensor_8(0x28ab, 0x0c);
	write_cmos_sensor_8(0x28ac, 0x10);
	write_cmos_sensor_8(0x28ad, 0x15);
	write_cmos_sensor_8(0x28ae, 0x1b);
	write_cmos_sensor_8(0x28af, 0x26);
	write_cmos_sensor_8(0x28b0, 0x29);
	write_cmos_sensor_8(0x28b1, 0x1e);
	write_cmos_sensor_8(0x28b2, 0x19);
	write_cmos_sensor_8(0x28b3, 0x14);
	write_cmos_sensor_8(0x28b4, 0x11);
	write_cmos_sensor_8(0x28b5, 0x0e);
	write_cmos_sensor_8(0x28b6, 0x0c);
	write_cmos_sensor_8(0x28b7, 0x0b);
	write_cmos_sensor_8(0x28b8, 0x0b);
	write_cmos_sensor_8(0x28b9, 0x0c);
	write_cmos_sensor_8(0x28ba, 0x0e);
	write_cmos_sensor_8(0x28bb, 0x11);
	write_cmos_sensor_8(0x28bc, 0x16);
	write_cmos_sensor_8(0x28bd, 0x1b);
	write_cmos_sensor_8(0x28be, 0x20);
	write_cmos_sensor_8(0x28bf, 0x2d);
	write_cmos_sensor_8(0x28c0, 0x30);
	write_cmos_sensor_8(0x28c1, 0x26);
	write_cmos_sensor_8(0x28c2, 0x21);
	write_cmos_sensor_8(0x28c3, 0x1b);
	write_cmos_sensor_8(0x28c4, 0x18);
	write_cmos_sensor_8(0x28c5, 0x15);
	write_cmos_sensor_8(0x28c6, 0x13);
	write_cmos_sensor_8(0x28c7, 0x12);
	write_cmos_sensor_8(0x28c8, 0x12);
	write_cmos_sensor_8(0x28c9, 0x13);
	write_cmos_sensor_8(0x28ca, 0x16);
	write_cmos_sensor_8(0x28cb, 0x18);
	write_cmos_sensor_8(0x28cc, 0x1d);
	write_cmos_sensor_8(0x28cd, 0x23);
	write_cmos_sensor_8(0x28ce, 0x28);
	write_cmos_sensor_8(0x28cf, 0x33);
	write_cmos_sensor_8(0x28d0, 0x3c);
	write_cmos_sensor_8(0x28d1, 0x2f);
	write_cmos_sensor_8(0x28d2, 0x2a);
	write_cmos_sensor_8(0x28d3, 0x26);
	write_cmos_sensor_8(0x28d4, 0x22);
	write_cmos_sensor_8(0x28d5, 0x1e);
	write_cmos_sensor_8(0x28d6, 0x1c);
	write_cmos_sensor_8(0x28d7, 0x1c);
	write_cmos_sensor_8(0x28d8, 0x1c);
	write_cmos_sensor_8(0x28d9, 0x1d);
	write_cmos_sensor_8(0x28da, 0x1f);
	write_cmos_sensor_8(0x28db, 0x22);
	write_cmos_sensor_8(0x28dc, 0x27);
	write_cmos_sensor_8(0x28dd, 0x2c);
	write_cmos_sensor_8(0x28de, 0x31);
	write_cmos_sensor_8(0x28df, 0x40);
	write_cmos_sensor_8(0x28e0, 0x4a);
	write_cmos_sensor_8(0x28e1, 0x3a);
	write_cmos_sensor_8(0x28e2, 0x34);
	write_cmos_sensor_8(0x28e3, 0x2f);
	write_cmos_sensor_8(0x28e4, 0x2c);
	write_cmos_sensor_8(0x28e5, 0x28);
	write_cmos_sensor_8(0x28e6, 0x27);
	write_cmos_sensor_8(0x28e7, 0x26);
	write_cmos_sensor_8(0x28e8, 0x27);
	write_cmos_sensor_8(0x28e9, 0x27);
	write_cmos_sensor_8(0x28ea, 0x29);
	write_cmos_sensor_8(0x28eb, 0x2d);
	write_cmos_sensor_8(0x28ec, 0x31);
	write_cmos_sensor_8(0x28ed, 0x37);
	write_cmos_sensor_8(0x28ee, 0x3d);
	write_cmos_sensor_8(0x28ef, 0x4e);
	write_cmos_sensor_8(0x28f0, 0x5c);
	write_cmos_sensor_8(0x28f1, 0x4d);
	write_cmos_sensor_8(0x28f2, 0x46);
	write_cmos_sensor_8(0x28f3, 0x3d);
	write_cmos_sensor_8(0x28f4, 0x3a);
	write_cmos_sensor_8(0x28f5, 0x36);
	write_cmos_sensor_8(0x28f6, 0x35);
	write_cmos_sensor_8(0x28f7, 0x33);
	write_cmos_sensor_8(0x28f8, 0x34);
	write_cmos_sensor_8(0x28f9, 0x35);
	write_cmos_sensor_8(0x28fa, 0x37);
	write_cmos_sensor_8(0x28fb, 0x3a);
	write_cmos_sensor_8(0x28fc, 0x40);
	write_cmos_sensor_8(0x28fd, 0x48);
	write_cmos_sensor_8(0x28fe, 0x50);
	write_cmos_sensor_8(0x28ff, 0x64);
	write_cmos_sensor_8(0x2900, 0x74);
	write_cmos_sensor_8(0x2901, 0x7d);
	write_cmos_sensor_8(0x2902, 0x7c);
	write_cmos_sensor_8(0x2903, 0x7b);
	write_cmos_sensor_8(0x2904, 0x7c);
	write_cmos_sensor_8(0x2905, 0x7b);
	write_cmos_sensor_8(0x2906, 0x7b);
	write_cmos_sensor_8(0x2907, 0x79);
	write_cmos_sensor_8(0x2908, 0x7c);
	write_cmos_sensor_8(0x2909, 0x7a);
	write_cmos_sensor_8(0x290a, 0x79);
	write_cmos_sensor_8(0x290b, 0x7b);
	write_cmos_sensor_8(0x290c, 0x7a);
	write_cmos_sensor_8(0x290d, 0x7b);
	write_cmos_sensor_8(0x290e, 0x79);
	write_cmos_sensor_8(0x290f, 0x7d);
	write_cmos_sensor_8(0x2910, 0x7f);
	write_cmos_sensor_8(0x2911, 0x7d);
	write_cmos_sensor_8(0x2912, 0x7d);
	write_cmos_sensor_8(0x2913, 0x7d);
	write_cmos_sensor_8(0x2914, 0x7c);
	write_cmos_sensor_8(0x2915, 0x7c);
	write_cmos_sensor_8(0x2916, 0x7c);
	write_cmos_sensor_8(0x2917, 0x7c);
	write_cmos_sensor_8(0x2918, 0x7b);
	write_cmos_sensor_8(0x2919, 0x7c);
	write_cmos_sensor_8(0x291a, 0x7c);
	write_cmos_sensor_8(0x291b, 0x7d);
	write_cmos_sensor_8(0x291c, 0x7d);
	write_cmos_sensor_8(0x291d, 0x7c);
	write_cmos_sensor_8(0x291e, 0x7d);
	write_cmos_sensor_8(0x291f, 0x7c);
	write_cmos_sensor_8(0x2920, 0x7e);
	write_cmos_sensor_8(0x2921, 0x7d);
	write_cmos_sensor_8(0x2922, 0x7c);
	write_cmos_sensor_8(0x2923, 0x7d);
	write_cmos_sensor_8(0x2924, 0x7d);
	write_cmos_sensor_8(0x2925, 0x7d);
	write_cmos_sensor_8(0x2926, 0x7d);
	write_cmos_sensor_8(0x2927, 0x7c);
	write_cmos_sensor_8(0x2928, 0x7d);
	write_cmos_sensor_8(0x2929, 0x7d);
	write_cmos_sensor_8(0x292a, 0x7d);
	write_cmos_sensor_8(0x292b, 0x7d);
	write_cmos_sensor_8(0x292c, 0x7d);
	write_cmos_sensor_8(0x292d, 0x7d);
	write_cmos_sensor_8(0x292e, 0x7d);
	write_cmos_sensor_8(0x292f, 0x7f);
	write_cmos_sensor_8(0x2930, 0x80);
	write_cmos_sensor_8(0x2931, 0x7d);
	write_cmos_sensor_8(0x2932, 0x7c);
	write_cmos_sensor_8(0x2933, 0x7d);
	write_cmos_sensor_8(0x2934, 0x7d);
	write_cmos_sensor_8(0x2935, 0x7e);
	write_cmos_sensor_8(0x2936, 0x7e);
	write_cmos_sensor_8(0x2937, 0x7e);
	write_cmos_sensor_8(0x2938, 0x7e);
	write_cmos_sensor_8(0x2939, 0x7e);
	write_cmos_sensor_8(0x293a, 0x7e);
	write_cmos_sensor_8(0x293b, 0x7e);
	write_cmos_sensor_8(0x293c, 0x7d);
	write_cmos_sensor_8(0x293d, 0x7d);
	write_cmos_sensor_8(0x293e, 0x7d);
	write_cmos_sensor_8(0x293f, 0x80);
	write_cmos_sensor_8(0x2940, 0x80);
	write_cmos_sensor_8(0x2941, 0x7d);
	write_cmos_sensor_8(0x2942, 0x7d);
	write_cmos_sensor_8(0x2943, 0x7d);
	write_cmos_sensor_8(0x2944, 0x7f);
	write_cmos_sensor_8(0x2945, 0x7f);
	write_cmos_sensor_8(0x2946, 0x80);
	write_cmos_sensor_8(0x2947, 0x80);
	write_cmos_sensor_8(0x2948, 0x80);
	write_cmos_sensor_8(0x2949, 0x80);
	write_cmos_sensor_8(0x294a, 0x7f);
	write_cmos_sensor_8(0x294b, 0x7f);
	write_cmos_sensor_8(0x294c, 0x7e);
	write_cmos_sensor_8(0x294d, 0x7e);
	write_cmos_sensor_8(0x294e, 0x7e);
	write_cmos_sensor_8(0x294f, 0x80);
	write_cmos_sensor_8(0x2950, 0x7e);
	write_cmos_sensor_8(0x2951, 0x7d);
	write_cmos_sensor_8(0x2952, 0x7d);
	write_cmos_sensor_8(0x2953, 0x7e);
	write_cmos_sensor_8(0x2954, 0x7f);
	write_cmos_sensor_8(0x2955, 0x80);
	write_cmos_sensor_8(0x2956, 0x81);
	write_cmos_sensor_8(0x2957, 0x81);
	write_cmos_sensor_8(0x2958, 0x81);
	write_cmos_sensor_8(0x2959, 0x81);
	write_cmos_sensor_8(0x295a, 0x81);
	write_cmos_sensor_8(0x295b, 0x80);
	write_cmos_sensor_8(0x295c, 0x7f);
	write_cmos_sensor_8(0x295d, 0x7e);
	write_cmos_sensor_8(0x295e, 0x7e);
	write_cmos_sensor_8(0x295f, 0x81);
	write_cmos_sensor_8(0x2960, 0x80);
	write_cmos_sensor_8(0x2961, 0x7c);
	write_cmos_sensor_8(0x2962, 0x7c);
	write_cmos_sensor_8(0x2963, 0x7e);
	write_cmos_sensor_8(0x2964, 0x7f);
	write_cmos_sensor_8(0x2965, 0x80);
	write_cmos_sensor_8(0x2966, 0x81);
	write_cmos_sensor_8(0x2967, 0x81);
	write_cmos_sensor_8(0x2968, 0x81);
	write_cmos_sensor_8(0x2969, 0x82);
	write_cmos_sensor_8(0x296a, 0x81);
	write_cmos_sensor_8(0x296b, 0x81);
	write_cmos_sensor_8(0x296c, 0x80);
	write_cmos_sensor_8(0x296d, 0x7e);
	write_cmos_sensor_8(0x296e, 0x7e);
	write_cmos_sensor_8(0x296f, 0x80);
	write_cmos_sensor_8(0x2970, 0x7e);
	write_cmos_sensor_8(0x2971, 0x7c);
	write_cmos_sensor_8(0x2972, 0x7c);
	write_cmos_sensor_8(0x2973, 0x7e);
	write_cmos_sensor_8(0x2974, 0x7f);
	write_cmos_sensor_8(0x2975, 0x80);
	write_cmos_sensor_8(0x2976, 0x81);
	write_cmos_sensor_8(0x2977, 0x80);
	write_cmos_sensor_8(0x2978, 0x81);
	write_cmos_sensor_8(0x2979, 0x81);
	write_cmos_sensor_8(0x297a, 0x81);
	write_cmos_sensor_8(0x297b, 0x81);
	write_cmos_sensor_8(0x297c, 0x80);
	write_cmos_sensor_8(0x297d, 0x7e);
	write_cmos_sensor_8(0x297e, 0x7e);
	write_cmos_sensor_8(0x297f, 0x81);
	write_cmos_sensor_8(0x2980, 0x7f);
	write_cmos_sensor_8(0x2981, 0x7b);
	write_cmos_sensor_8(0x2982, 0x7c);
	write_cmos_sensor_8(0x2983, 0x7d);
	write_cmos_sensor_8(0x2984, 0x7f);
	write_cmos_sensor_8(0x2985, 0x80);
	write_cmos_sensor_8(0x2986, 0x81);
	write_cmos_sensor_8(0x2987, 0x81);
	write_cmos_sensor_8(0x2988, 0x81);
	write_cmos_sensor_8(0x2989, 0x81);
	write_cmos_sensor_8(0x298a, 0x81);
	write_cmos_sensor_8(0x298b, 0x81);
	write_cmos_sensor_8(0x298c, 0x80);
	write_cmos_sensor_8(0x298d, 0x7e);
	write_cmos_sensor_8(0x298e, 0x7e);
	write_cmos_sensor_8(0x298f, 0x81);
	write_cmos_sensor_8(0x2990, 0x7e);
	write_cmos_sensor_8(0x2991, 0x7c);
	write_cmos_sensor_8(0x2992, 0x7c);
	write_cmos_sensor_8(0x2993, 0x7d);
	write_cmos_sensor_8(0x2994, 0x7f);
	write_cmos_sensor_8(0x2995, 0x7f);
	write_cmos_sensor_8(0x2996, 0x80);
	write_cmos_sensor_8(0x2997, 0x81);
	write_cmos_sensor_8(0x2998, 0x81);
	write_cmos_sensor_8(0x2999, 0x81);
	write_cmos_sensor_8(0x299a, 0x81);
	write_cmos_sensor_8(0x299b, 0x80);
	write_cmos_sensor_8(0x299c, 0x80);
	write_cmos_sensor_8(0x299d, 0x7e);
	write_cmos_sensor_8(0x299e, 0x7f);
	write_cmos_sensor_8(0x299f, 0x81);
	write_cmos_sensor_8(0x29a0, 0x81);
	write_cmos_sensor_8(0x29a1, 0x7b);
	write_cmos_sensor_8(0x29a2, 0x7c);
	write_cmos_sensor_8(0x29a3, 0x7d);
	write_cmos_sensor_8(0x29a4, 0x7e);
	write_cmos_sensor_8(0x29a5, 0x7e);
	write_cmos_sensor_8(0x29a6, 0x7f);
	write_cmos_sensor_8(0x29a7, 0x80);
	write_cmos_sensor_8(0x29a8, 0x80);
	write_cmos_sensor_8(0x29a9, 0x80);
	write_cmos_sensor_8(0x29aa, 0x80);
	write_cmos_sensor_8(0x29ab, 0x80);
	write_cmos_sensor_8(0x29ac, 0x7f);
	write_cmos_sensor_8(0x29ad, 0x7e);
	write_cmos_sensor_8(0x29ae, 0x7e);
	write_cmos_sensor_8(0x29af, 0x81);
	write_cmos_sensor_8(0x29b0, 0x7e);
	write_cmos_sensor_8(0x29b1, 0x7c);
	write_cmos_sensor_8(0x29b2, 0x7b);
	write_cmos_sensor_8(0x29b3, 0x7c);
	write_cmos_sensor_8(0x29b4, 0x7d);
	write_cmos_sensor_8(0x29b5, 0x7d);
	write_cmos_sensor_8(0x29b6, 0x7d);
	write_cmos_sensor_8(0x29b7, 0x7e);
	write_cmos_sensor_8(0x29b8, 0x7e);
	write_cmos_sensor_8(0x29b9, 0x7f);
	write_cmos_sensor_8(0x29ba, 0x7e);
	write_cmos_sensor_8(0x29bb, 0x7e);
	write_cmos_sensor_8(0x29bc, 0x7e);
	write_cmos_sensor_8(0x29bd, 0x7d);
	write_cmos_sensor_8(0x29be, 0x7e);
	write_cmos_sensor_8(0x29bf, 0x81);
	write_cmos_sensor_8(0x29c0, 0x81);
	write_cmos_sensor_8(0x29c1, 0x7c);
	write_cmos_sensor_8(0x29c2, 0x7c);
	write_cmos_sensor_8(0x29c3, 0x7c);
	write_cmos_sensor_8(0x29c4, 0x7c);
	write_cmos_sensor_8(0x29c5, 0x7c);
	write_cmos_sensor_8(0x29c6, 0x7c);
	write_cmos_sensor_8(0x29c7, 0x7c);
	write_cmos_sensor_8(0x29c8, 0x7d);
	write_cmos_sensor_8(0x29c9, 0x7d);
	write_cmos_sensor_8(0x29ca, 0x7d);
	write_cmos_sensor_8(0x29cb, 0x7d);
	write_cmos_sensor_8(0x29cc, 0x7d);
	write_cmos_sensor_8(0x29cd, 0x7d);
	write_cmos_sensor_8(0x29ce, 0x7e);
	write_cmos_sensor_8(0x29cf, 0x82);
	write_cmos_sensor_8(0x29d0, 0x80);
	write_cmos_sensor_8(0x29d1, 0x7d);
	write_cmos_sensor_8(0x29d2, 0x7c);
	write_cmos_sensor_8(0x29d3, 0x7c);
	write_cmos_sensor_8(0x29d4, 0x7c);
	write_cmos_sensor_8(0x29d5, 0x7c);
	write_cmos_sensor_8(0x29d6, 0x7c);
	write_cmos_sensor_8(0x29d7, 0x7c);
	write_cmos_sensor_8(0x29d8, 0x7c);
	write_cmos_sensor_8(0x29d9, 0x7d);
	write_cmos_sensor_8(0x29da, 0x7d);
	write_cmos_sensor_8(0x29db, 0x7e);
	write_cmos_sensor_8(0x29dc, 0x7e);
	write_cmos_sensor_8(0x29dd, 0x7e);
	write_cmos_sensor_8(0x29de, 0x7f);
	write_cmos_sensor_8(0x29df, 0x81);
	write_cmos_sensor_8(0x29e0, 0x82);
	write_cmos_sensor_8(0x29e1, 0x7d);
	write_cmos_sensor_8(0x29e2, 0x7b);
	write_cmos_sensor_8(0x29e3, 0x7c);
	write_cmos_sensor_8(0x29e4, 0x7b);
	write_cmos_sensor_8(0x29e5, 0x7b);
	write_cmos_sensor_8(0x29e6, 0x7b);
	write_cmos_sensor_8(0x29e7, 0x7a);
	write_cmos_sensor_8(0x29e8, 0x7a);
	write_cmos_sensor_8(0x29e9, 0x7b);
	write_cmos_sensor_8(0x29ea, 0x7c);
	write_cmos_sensor_8(0x29eb, 0x7c);
	write_cmos_sensor_8(0x29ec, 0x7d);
	write_cmos_sensor_8(0x29ed, 0x7d);
	write_cmos_sensor_8(0x29ee, 0x7f);
	write_cmos_sensor_8(0x29ef, 0x82);
	write_cmos_sensor_8(0x29f0, 0x7e);
	write_cmos_sensor_8(0x29f1, 0x86);
	write_cmos_sensor_8(0x29f2, 0x86);
	write_cmos_sensor_8(0x29f3, 0x84);
	write_cmos_sensor_8(0x29f4, 0x85);
	write_cmos_sensor_8(0x29f5, 0x85);
	write_cmos_sensor_8(0x29f6, 0x83);
	write_cmos_sensor_8(0x29f7, 0x84);
	write_cmos_sensor_8(0x29f8, 0x84);
	write_cmos_sensor_8(0x29f9, 0x85);
	write_cmos_sensor_8(0x29fa, 0x85);
	write_cmos_sensor_8(0x29fb, 0x87);
	write_cmos_sensor_8(0x29fc, 0x87);
	write_cmos_sensor_8(0x29fd, 0x88);
	write_cmos_sensor_8(0x29fe, 0x88);
	write_cmos_sensor_8(0x29ff, 0x80);
	write_cmos_sensor_8(0x2a00, 0x7f);
	write_cmos_sensor_8(0x2a01, 0x85);
	write_cmos_sensor_8(0x2a02, 0x83);
	write_cmos_sensor_8(0x2a03, 0x82);
	write_cmos_sensor_8(0x2a04, 0x83);
	write_cmos_sensor_8(0x2a05, 0x81);
	write_cmos_sensor_8(0x2a06, 0x82);
	write_cmos_sensor_8(0x2a07, 0x81);
	write_cmos_sensor_8(0x2a08, 0x82);
	write_cmos_sensor_8(0x2a09, 0x81);
	write_cmos_sensor_8(0x2a0a, 0x83);
	write_cmos_sensor_8(0x2a0b, 0x82);
	write_cmos_sensor_8(0x2a0c, 0x83);
	write_cmos_sensor_8(0x2a0d, 0x83);
	write_cmos_sensor_8(0x2a0e, 0x83);
	write_cmos_sensor_8(0x2a0f, 0x82);
	write_cmos_sensor_8(0x2a10, 0x82);
	write_cmos_sensor_8(0x2a11, 0x86);
	write_cmos_sensor_8(0x2a12, 0x85);
	write_cmos_sensor_8(0x2a13, 0x85);
	write_cmos_sensor_8(0x2a14, 0x84);
	write_cmos_sensor_8(0x2a15, 0x84);
	write_cmos_sensor_8(0x2a16, 0x83);
	write_cmos_sensor_8(0x2a17, 0x83);
	write_cmos_sensor_8(0x2a18, 0x82);
	write_cmos_sensor_8(0x2a19, 0x83);
	write_cmos_sensor_8(0x2a1a, 0x83);
	write_cmos_sensor_8(0x2a1b, 0x84);
	write_cmos_sensor_8(0x2a1c, 0x84);
	write_cmos_sensor_8(0x2a1d, 0x84);
	write_cmos_sensor_8(0x2a1e, 0x86);
	write_cmos_sensor_8(0x2a1f, 0x83);
	write_cmos_sensor_8(0x2a20, 0x82);
	write_cmos_sensor_8(0x2a21, 0x84);
	write_cmos_sensor_8(0x2a22, 0x83);
	write_cmos_sensor_8(0x2a23, 0x82);
	write_cmos_sensor_8(0x2a24, 0x82);
	write_cmos_sensor_8(0x2a25, 0x81);
	write_cmos_sensor_8(0x2a26, 0x81);
	write_cmos_sensor_8(0x2a27, 0x81);
	write_cmos_sensor_8(0x2a28, 0x81);
	write_cmos_sensor_8(0x2a29, 0x81);
	write_cmos_sensor_8(0x2a2a, 0x81);
	write_cmos_sensor_8(0x2a2b, 0x82);
	write_cmos_sensor_8(0x2a2c, 0x82);
	write_cmos_sensor_8(0x2a2d, 0x83);
	write_cmos_sensor_8(0x2a2e, 0x84);
	write_cmos_sensor_8(0x2a2f, 0x81);
	write_cmos_sensor_8(0x2a30, 0x81);
	write_cmos_sensor_8(0x2a31, 0x83);
	write_cmos_sensor_8(0x2a32, 0x81);
	write_cmos_sensor_8(0x2a33, 0x81);
	write_cmos_sensor_8(0x2a34, 0x80);
	write_cmos_sensor_8(0x2a35, 0x80);
	write_cmos_sensor_8(0x2a36, 0x80);
	write_cmos_sensor_8(0x2a37, 0x80);
	write_cmos_sensor_8(0x2a38, 0x7f);
	write_cmos_sensor_8(0x2a39, 0x80);
	write_cmos_sensor_8(0x2a3a, 0x80);
	write_cmos_sensor_8(0x2a3b, 0x80);
	write_cmos_sensor_8(0x2a3c, 0x80);
	write_cmos_sensor_8(0x2a3d, 0x81);
	write_cmos_sensor_8(0x2a3e, 0x83);
	write_cmos_sensor_8(0x2a3f, 0x80);
	write_cmos_sensor_8(0x2a40, 0x80);
	write_cmos_sensor_8(0x2a41, 0x81);
	write_cmos_sensor_8(0x2a42, 0x81);
	write_cmos_sensor_8(0x2a43, 0x7f);
	write_cmos_sensor_8(0x2a44, 0x80);
	write_cmos_sensor_8(0x2a45, 0x7f);
	write_cmos_sensor_8(0x2a46, 0x80);
	write_cmos_sensor_8(0x2a47, 0x7f);
	write_cmos_sensor_8(0x2a48, 0x80);
	write_cmos_sensor_8(0x2a49, 0x7f);
	write_cmos_sensor_8(0x2a4a, 0x80);
	write_cmos_sensor_8(0x2a4b, 0x7f);
	write_cmos_sensor_8(0x2a4c, 0x80);
	write_cmos_sensor_8(0x2a4d, 0x80);
	write_cmos_sensor_8(0x2a4e, 0x82);
	write_cmos_sensor_8(0x2a4f, 0x7f);
	write_cmos_sensor_8(0x2a50, 0x7f);
	write_cmos_sensor_8(0x2a51, 0x82);
	write_cmos_sensor_8(0x2a52, 0x80);
	write_cmos_sensor_8(0x2a53, 0x80);
	write_cmos_sensor_8(0x2a54, 0x7f);
	write_cmos_sensor_8(0x2a55, 0x80);
	write_cmos_sensor_8(0x2a56, 0x80);
	write_cmos_sensor_8(0x2a57, 0x80);
	write_cmos_sensor_8(0x2a58, 0x80);
	write_cmos_sensor_8(0x2a59, 0x80);
	write_cmos_sensor_8(0x2a5a, 0x80);
	write_cmos_sensor_8(0x2a5b, 0x7f);
	write_cmos_sensor_8(0x2a5c, 0x7f);
	write_cmos_sensor_8(0x2a5d, 0x80);
	write_cmos_sensor_8(0x2a5e, 0x81);
	write_cmos_sensor_8(0x2a5f, 0x7f);
	write_cmos_sensor_8(0x2a60, 0x7f);
	write_cmos_sensor_8(0x2a61, 0x81);
	write_cmos_sensor_8(0x2a62, 0x80);
	write_cmos_sensor_8(0x2a63, 0x7f);
	write_cmos_sensor_8(0x2a64, 0x80);
	write_cmos_sensor_8(0x2a65, 0x80);
	write_cmos_sensor_8(0x2a66, 0x80);
	write_cmos_sensor_8(0x2a67, 0x81);
	write_cmos_sensor_8(0x2a68, 0x80);
	write_cmos_sensor_8(0x2a69, 0x81);
	write_cmos_sensor_8(0x2a6a, 0x80);
	write_cmos_sensor_8(0x2a6b, 0x80);
	write_cmos_sensor_8(0x2a6c, 0x80);
	write_cmos_sensor_8(0x2a6d, 0x80);
	write_cmos_sensor_8(0x2a6e, 0x81);
	write_cmos_sensor_8(0x2a6f, 0x7e);
	write_cmos_sensor_8(0x2a70, 0x7e);
	write_cmos_sensor_8(0x2a71, 0x80);
	write_cmos_sensor_8(0x2a72, 0x7f);
	write_cmos_sensor_8(0x2a73, 0x80);
	write_cmos_sensor_8(0x2a74, 0x80);
	write_cmos_sensor_8(0x2a75, 0x81);
	write_cmos_sensor_8(0x2a76, 0x81);
	write_cmos_sensor_8(0x2a77, 0x80);
	write_cmos_sensor_8(0x2a78, 0x80);
	write_cmos_sensor_8(0x2a79, 0x81);
	write_cmos_sensor_8(0x2a7a, 0x80);
	write_cmos_sensor_8(0x2a7b, 0x7f);
	write_cmos_sensor_8(0x2a7c, 0x7f);
	write_cmos_sensor_8(0x2a7d, 0x7f);
	write_cmos_sensor_8(0x2a7e, 0x81);
	write_cmos_sensor_8(0x2a7f, 0x7e);
	write_cmos_sensor_8(0x2a80, 0x7e);
	write_cmos_sensor_8(0x2a81, 0x81);
	write_cmos_sensor_8(0x2a82, 0x7f);
	write_cmos_sensor_8(0x2a83, 0x80);
	write_cmos_sensor_8(0x2a84, 0x80);
	write_cmos_sensor_8(0x2a85, 0x81);
	write_cmos_sensor_8(0x2a86, 0x81);
	write_cmos_sensor_8(0x2a87, 0x81);
	write_cmos_sensor_8(0x2a88, 0x80);
	write_cmos_sensor_8(0x2a89, 0x81);
	write_cmos_sensor_8(0x2a8a, 0x80);
	write_cmos_sensor_8(0x2a8b, 0x80);
	write_cmos_sensor_8(0x2a8c, 0x7f);
	write_cmos_sensor_8(0x2a8d, 0x7f);
	write_cmos_sensor_8(0x2a8e, 0x80);
	write_cmos_sensor_8(0x2a8f, 0x7e);
	write_cmos_sensor_8(0x2a90, 0x7f);
	write_cmos_sensor_8(0x2a91, 0x81);
	write_cmos_sensor_8(0x2a92, 0x80);
	write_cmos_sensor_8(0x2a93, 0x80);
	write_cmos_sensor_8(0x2a94, 0x80);
	write_cmos_sensor_8(0x2a95, 0x80);
	write_cmos_sensor_8(0x2a96, 0x81);
	write_cmos_sensor_8(0x2a97, 0x81);
	write_cmos_sensor_8(0x2a98, 0x81);
	write_cmos_sensor_8(0x2a99, 0x81);
	write_cmos_sensor_8(0x2a9a, 0x81);
	write_cmos_sensor_8(0x2a9b, 0x80);
	write_cmos_sensor_8(0x2a9c, 0x80);
	write_cmos_sensor_8(0x2a9d, 0x80);
	write_cmos_sensor_8(0x2a9e, 0x81);
	write_cmos_sensor_8(0x2a9f, 0x7e);
	write_cmos_sensor_8(0x2aa0, 0x7f);
	write_cmos_sensor_8(0x2aa1, 0x81);
	write_cmos_sensor_8(0x2aa2, 0x80);
	write_cmos_sensor_8(0x2aa3, 0x80);
	write_cmos_sensor_8(0x2aa4, 0x80);
	write_cmos_sensor_8(0x2aa5, 0x80);
	write_cmos_sensor_8(0x2aa6, 0x80);
	write_cmos_sensor_8(0x2aa7, 0x80);
	write_cmos_sensor_8(0x2aa8, 0x80);
	write_cmos_sensor_8(0x2aa9, 0x80);
	write_cmos_sensor_8(0x2aaa, 0x80);
	write_cmos_sensor_8(0x2aab, 0x80);
	write_cmos_sensor_8(0x2aac, 0x7f);
	write_cmos_sensor_8(0x2aad, 0x80);
	write_cmos_sensor_8(0x2aae, 0x81);
	write_cmos_sensor_8(0x2aaf, 0x7e);
	write_cmos_sensor_8(0x2ab0, 0x80);
	write_cmos_sensor_8(0x2ab1, 0x82);
	write_cmos_sensor_8(0x2ab2, 0x81);
	write_cmos_sensor_8(0x2ab3, 0x80);
	write_cmos_sensor_8(0x2ab4, 0x80);
	write_cmos_sensor_8(0x2ab5, 0x80);
	write_cmos_sensor_8(0x2ab6, 0x7f);
	write_cmos_sensor_8(0x2ab7, 0x7f);
	write_cmos_sensor_8(0x2ab8, 0x7f);
	write_cmos_sensor_8(0x2ab9, 0x7f);
	write_cmos_sensor_8(0x2aba, 0x7f);
	write_cmos_sensor_8(0x2abb, 0x7f);
	write_cmos_sensor_8(0x2abc, 0x80);
	write_cmos_sensor_8(0x2abd, 0x80);
	write_cmos_sensor_8(0x2abe, 0x81);
	write_cmos_sensor_8(0x2abf, 0x7f);
	write_cmos_sensor_8(0x2ac0, 0x81);
	write_cmos_sensor_8(0x2ac1, 0x83);
	write_cmos_sensor_8(0x2ac2, 0x82);
	write_cmos_sensor_8(0x2ac3, 0x81);
	write_cmos_sensor_8(0x2ac4, 0x80);
	write_cmos_sensor_8(0x2ac5, 0x80);
	write_cmos_sensor_8(0x2ac6, 0x80);
	write_cmos_sensor_8(0x2ac7, 0x80);
	write_cmos_sensor_8(0x2ac8, 0x7f);
	write_cmos_sensor_8(0x2ac9, 0x80);
	write_cmos_sensor_8(0x2aca, 0x80);
	write_cmos_sensor_8(0x2acb, 0x80);
	write_cmos_sensor_8(0x2acc, 0x80);
	write_cmos_sensor_8(0x2acd, 0x81);
	write_cmos_sensor_8(0x2ace, 0x82);
	write_cmos_sensor_8(0x2acf, 0x80);
	write_cmos_sensor_8(0x2ad0, 0x83);
	write_cmos_sensor_8(0x2ad1, 0x85);
	write_cmos_sensor_8(0x2ad2, 0x83);
	write_cmos_sensor_8(0x2ad3, 0x82);
	write_cmos_sensor_8(0x2ad4, 0x82);
	write_cmos_sensor_8(0x2ad5, 0x81);
	write_cmos_sensor_8(0x2ad6, 0x81);
	write_cmos_sensor_8(0x2ad7, 0x80);
	write_cmos_sensor_8(0x2ad8, 0x80);
	write_cmos_sensor_8(0x2ad9, 0x81);
	write_cmos_sensor_8(0x2ada, 0x81);
	write_cmos_sensor_8(0x2adb, 0x81);
	write_cmos_sensor_8(0x2adc, 0x82);
	write_cmos_sensor_8(0x2add, 0x82);
	write_cmos_sensor_8(0x2ade, 0x83);
	write_cmos_sensor_8(0x2adf, 0x81);
	write_cmos_sensor_8(0x2ae0, 0x83);
	write_cmos_sensor_8(0x2ae1, 0x87);
	write_cmos_sensor_8(0x2ae2, 0x85);
	write_cmos_sensor_8(0x2ae3, 0x85);
	write_cmos_sensor_8(0x2ae4, 0x84);
	write_cmos_sensor_8(0x2ae5, 0x83);
	write_cmos_sensor_8(0x2ae6, 0x83);
	write_cmos_sensor_8(0x2ae7, 0x83);
	write_cmos_sensor_8(0x2ae8, 0x82);
	write_cmos_sensor_8(0x2ae9, 0x83);
	write_cmos_sensor_8(0x2aea, 0x83);
	write_cmos_sensor_8(0x2aeb, 0x84);
	write_cmos_sensor_8(0x2aec, 0x84);
	write_cmos_sensor_8(0x2aed, 0x84);
	write_cmos_sensor_8(0x2aee, 0x85);
	write_cmos_sensor_8(0x2aef, 0x82);
	write_cmos_sensor_8(0x2af0, 0x84);
	write_cmos_sensor_8(0x2af1, 0x86);
	write_cmos_sensor_8(0x2af2, 0x86);
	write_cmos_sensor_8(0x2af3, 0x86);
	write_cmos_sensor_8(0x2af4, 0x84);
	write_cmos_sensor_8(0x2af5, 0x87);
	write_cmos_sensor_8(0x2af6, 0x84);
	write_cmos_sensor_8(0x2af7, 0x84);
	write_cmos_sensor_8(0x2af8, 0x84);
	write_cmos_sensor_8(0x2af9, 0x84);
	write_cmos_sensor_8(0x2afa, 0x85);
	write_cmos_sensor_8(0x2afb, 0x84);
	write_cmos_sensor_8(0x2afc, 0x85);
	write_cmos_sensor_8(0x2afd, 0x85);
	write_cmos_sensor_8(0x2afe, 0x86);
	write_cmos_sensor_8(0x2aff, 0x81);
	write_cmos_sensor_8(0x2b05, 0x03);
	write_cmos_sensor_8(0x2b06, 0x0c);
	write_cmos_sensor_8(0x2b07, 0x02);
	write_cmos_sensor_8(0x2b08, 0x0b);
	
		//@@new_setting_2015_12_17
	write_cmos_sensor_8(0x5000, 0x9b);
	write_cmos_sensor_8(0x5002, 0x0c);
	write_cmos_sensor_8(0x5b85, 0x0c);
	write_cmos_sensor_8(0x5b87, 0x16);
	write_cmos_sensor_8(0x5b89, 0x28);
	write_cmos_sensor_8(0x5b8b, 0x40);
	write_cmos_sensor_8(0x5b8f, 0x1a);
	write_cmos_sensor_8(0x5b90, 0x1a);
	write_cmos_sensor_8(0x5b91, 0x1a);
	write_cmos_sensor_8(0x5b95, 0x1a);
	write_cmos_sensor_8(0x5b96, 0x1a);
	write_cmos_sensor_8(0x5b97, 0x1a);
	
//	;;@@,0xPD_Restore_On,0x(Default);
	write_cmos_sensor_8(0x5001, 0x4e);
	write_cmos_sensor_8(0x3018, 0xda);
	write_cmos_sensor_8(0x366c, 0x10);
	write_cmos_sensor_8(0x3641, 0x01);
	write_cmos_sensor_8(0x5d29, 0x80);
	write_cmos_sensor_8(0x0100, 0x00);

}	/*	sensor_init  */


static void preview_setting(void)
{
	/*
	* @@ OV16880 4lane 2336x1752 30fps hvbin
	* 100 99 2336 1752
	* 102 3601 1770 ;Pather tool use only
	* Xclk 24Mhz
	* Pclk clock frequency: 288Mhz
	* linelength = 2512(0x9d0)
	* framelength = 3824(0xef0)
	* grabwindow_width  = 2336
	* grabwindow_height = 1752
	* max_framerate: 30fps	
	* mipi_datarate per lane: 768Mbps
	*/
	
	write_cmos_sensor_8(0x0100, 0x00);
	write_cmos_sensor_8(0x0302, 0x20);
	write_cmos_sensor_8(0x030f, 0x07);
	write_cmos_sensor_8(0x3501, 0x07);
	write_cmos_sensor_8(0x3502, 0x72);
	write_cmos_sensor_8(0x3726, 0x22);
	write_cmos_sensor_8(0x3727, 0x44);
	write_cmos_sensor_8(0x3729, 0x22);
	write_cmos_sensor_8(0x372f, 0x89);
	write_cmos_sensor_8(0x3760, 0x90);
	write_cmos_sensor_8(0x37a1, 0x00);
	write_cmos_sensor_8(0x37a5, 0xaa);
	write_cmos_sensor_8(0x37a2, 0x00);
	write_cmos_sensor_8(0x37a3, 0x42);
	write_cmos_sensor_8(0x3800, 0x00);
	write_cmos_sensor_8(0x3801, 0x00);
	write_cmos_sensor_8(0x3802, 0x00);
	write_cmos_sensor_8(0x3803, 0x08);
	write_cmos_sensor_8(0x3804, 0x12);
	write_cmos_sensor_8(0x3805, 0x5f);
	write_cmos_sensor_8(0x3806, 0x0d);
	write_cmos_sensor_8(0x3807, 0xc7);
	write_cmos_sensor_8(0x3808, 0x09);
	write_cmos_sensor_8(0x3809, 0x20);
	write_cmos_sensor_8(0x380a, 0x06);
	write_cmos_sensor_8(0x380b, 0xd8);
	write_cmos_sensor_8(0x380c, 0x09);
	write_cmos_sensor_8(0x380d, 0xd0);
	write_cmos_sensor_8(0x380e, 0x0E);
	write_cmos_sensor_8(0x380f, 0xF0);
	write_cmos_sensor_8(0x3811, 0x08);
	write_cmos_sensor_8(0x3813, 0x02);
	write_cmos_sensor_8(0x3814, 0x31);
	write_cmos_sensor_8(0x3815, 0x31);
	write_cmos_sensor_8(0x3820, 0x81);
	write_cmos_sensor_8(0x3821, 0x06);
	write_cmos_sensor_8(0x3836, 0x0c);
	write_cmos_sensor_8(0x3841, 0x02);
	write_cmos_sensor_8(0x4006, 0x01);
	write_cmos_sensor_8(0x4007, 0x80);
	write_cmos_sensor_8(0x4009, 0x05);
	write_cmos_sensor_8(0x4050, 0x00);
	write_cmos_sensor_8(0x4051, 0x01);
	write_cmos_sensor_8(0x4501, 0x3c);
	write_cmos_sensor_8(0x4837, 0x14);
	write_cmos_sensor_8(0x5201, 0x62);
	write_cmos_sensor_8(0x5203, 0x08);
	write_cmos_sensor_8(0x5205, 0x48);
	write_cmos_sensor_8(0x5500, 0x80);
	write_cmos_sensor_8(0x5501, 0x80);
	write_cmos_sensor_8(0x5502, 0x80);
	write_cmos_sensor_8(0x5503, 0x80);
	write_cmos_sensor_8(0x5508, 0x80);
	write_cmos_sensor_8(0x5509, 0x80);
	write_cmos_sensor_8(0x550a, 0x80);
	write_cmos_sensor_8(0x550b, 0x80);
	write_cmos_sensor_8(0x5510, 0x08);
	write_cmos_sensor_8(0x5511, 0x08);
	write_cmos_sensor_8(0x5512, 0x08);
	write_cmos_sensor_8(0x5513, 0x08);
	write_cmos_sensor_8(0x5518, 0x08);
	write_cmos_sensor_8(0x5519, 0x08);
	write_cmos_sensor_8(0x551a, 0x08);
	write_cmos_sensor_8(0x551b, 0x08);
	write_cmos_sensor_8(0x5520, 0x80);
	write_cmos_sensor_8(0x5521, 0x80);
	write_cmos_sensor_8(0x5522, 0x80);
	write_cmos_sensor_8(0x5523, 0x80);
	write_cmos_sensor_8(0x5528, 0x80);
	write_cmos_sensor_8(0x5529, 0x80);
	write_cmos_sensor_8(0x552a, 0x80);
	write_cmos_sensor_8(0x552b, 0x80);
	write_cmos_sensor_8(0x5530, 0x08);
	write_cmos_sensor_8(0x5531, 0x08);
	write_cmos_sensor_8(0x5532, 0x08);
	write_cmos_sensor_8(0x5533, 0x08);
	write_cmos_sensor_8(0x5538, 0x08);
	write_cmos_sensor_8(0x5539, 0x08);
	write_cmos_sensor_8(0x553a, 0x08);
	write_cmos_sensor_8(0x553b, 0x08);
	write_cmos_sensor_8(0x5540, 0x80);
	write_cmos_sensor_8(0x5541, 0x80);
	write_cmos_sensor_8(0x5542, 0x80);
	write_cmos_sensor_8(0x5543, 0x80);
	write_cmos_sensor_8(0x5548, 0x80);
	write_cmos_sensor_8(0x5549, 0x80);
	write_cmos_sensor_8(0x554a, 0x80);
	write_cmos_sensor_8(0x554b, 0x80);
	write_cmos_sensor_8(0x5550, 0x08);
	write_cmos_sensor_8(0x5551, 0x08);
	write_cmos_sensor_8(0x5552, 0x08);
	write_cmos_sensor_8(0x5553, 0x08);
	write_cmos_sensor_8(0x5558, 0x08);
	write_cmos_sensor_8(0x5559, 0x08);
	write_cmos_sensor_8(0x555a, 0x08);
	write_cmos_sensor_8(0x555b, 0x08);
	write_cmos_sensor_8(0x5560, 0x80);
	write_cmos_sensor_8(0x5561, 0x80);
	write_cmos_sensor_8(0x5562, 0x80);
	write_cmos_sensor_8(0x5563, 0x80);
	write_cmos_sensor_8(0x5568, 0x80);
	write_cmos_sensor_8(0x5569, 0x80);
	write_cmos_sensor_8(0x556a, 0x80);
	write_cmos_sensor_8(0x556b, 0x80);
	write_cmos_sensor_8(0x5570, 0x08);
	write_cmos_sensor_8(0x5571, 0x08);
	write_cmos_sensor_8(0x5572, 0x08);
	write_cmos_sensor_8(0x5573, 0x08);
	write_cmos_sensor_8(0x5578, 0x08);
	write_cmos_sensor_8(0x5579, 0x08);
	write_cmos_sensor_8(0x557a, 0x08);
	write_cmos_sensor_8(0x557b, 0x08);
	write_cmos_sensor_8(0x5580, 0x80);
	write_cmos_sensor_8(0x5581, 0x80);
	write_cmos_sensor_8(0x5582, 0x80);
	write_cmos_sensor_8(0x5583, 0x80);
	write_cmos_sensor_8(0x5588, 0x80);
	write_cmos_sensor_8(0x5589, 0x80);
	write_cmos_sensor_8(0x558a, 0x80);
	write_cmos_sensor_8(0x558b, 0x80);
	write_cmos_sensor_8(0x5590, 0x08);
	write_cmos_sensor_8(0x5591, 0x08);
	write_cmos_sensor_8(0x5592, 0x08);
	write_cmos_sensor_8(0x5593, 0x08);
	write_cmos_sensor_8(0x5598, 0x08);
	write_cmos_sensor_8(0x5599, 0x08);
	write_cmos_sensor_8(0x559a, 0x08);
	write_cmos_sensor_8(0x559b, 0x08);
	write_cmos_sensor_8(0x55a0, 0x80);
	write_cmos_sensor_8(0x55a1, 0x80);
	write_cmos_sensor_8(0x55a2, 0x80);
	write_cmos_sensor_8(0x55a3, 0x80);
	write_cmos_sensor_8(0x55a8, 0x80);
	write_cmos_sensor_8(0x55a9, 0x80);
	write_cmos_sensor_8(0x55aa, 0x80);
	write_cmos_sensor_8(0x55ab, 0x80);
	write_cmos_sensor_8(0x55b0, 0x08);
	write_cmos_sensor_8(0x55b1, 0x08);
	write_cmos_sensor_8(0x55b2, 0x08);
	write_cmos_sensor_8(0x55b3, 0x08);
	write_cmos_sensor_8(0x55b8, 0x08);
	write_cmos_sensor_8(0x55b9, 0x08);
	write_cmos_sensor_8(0x55ba, 0x08);
	write_cmos_sensor_8(0x55bb, 0x08);
	write_cmos_sensor_8(0x55c0, 0x80);
	write_cmos_sensor_8(0x55c1, 0x80);
	write_cmos_sensor_8(0x55c2, 0x80);
	write_cmos_sensor_8(0x55c3, 0x80);
	write_cmos_sensor_8(0x55c8, 0x80);
	write_cmos_sensor_8(0x55c9, 0x80);
	write_cmos_sensor_8(0x55ca, 0x80);
	write_cmos_sensor_8(0x55cb, 0x80);
	write_cmos_sensor_8(0x55d0, 0x08);
	write_cmos_sensor_8(0x55d1, 0x08);
	write_cmos_sensor_8(0x55d2, 0x08);
	write_cmos_sensor_8(0x55d3, 0x08);
	write_cmos_sensor_8(0x55d8, 0x08);
	write_cmos_sensor_8(0x55d9, 0x08);
	write_cmos_sensor_8(0x55da, 0x08);
	write_cmos_sensor_8(0x55db, 0x08);
	write_cmos_sensor_8(0x55e0, 0x80);
	write_cmos_sensor_8(0x55e1, 0x80);
	write_cmos_sensor_8(0x55e2, 0x80);
	write_cmos_sensor_8(0x55e3, 0x80);
	write_cmos_sensor_8(0x55e8, 0x80);
	write_cmos_sensor_8(0x55e9, 0x80);
	write_cmos_sensor_8(0x55ea, 0x80);
	write_cmos_sensor_8(0x55eb, 0x80);
	write_cmos_sensor_8(0x55f0, 0x08);
	write_cmos_sensor_8(0x55f1, 0x08);
	write_cmos_sensor_8(0x55f2, 0x08);
	write_cmos_sensor_8(0x55f3, 0x08);
	write_cmos_sensor_8(0x55f8, 0x08);
	write_cmos_sensor_8(0x55f9, 0x08);
	write_cmos_sensor_8(0x55fa, 0x08);
	write_cmos_sensor_8(0x55fb, 0x08);
	write_cmos_sensor_8(0x5690, 0x00);
	write_cmos_sensor_8(0x5691, 0x00);
	write_cmos_sensor_8(0x5692, 0x00);
	write_cmos_sensor_8(0x5693, 0x04);
	write_cmos_sensor_8(0x5d24, 0x00);
	write_cmos_sensor_8(0x5d25, 0x00);
	write_cmos_sensor_8(0x5d26, 0x00);
	write_cmos_sensor_8(0x5d27, 0x04);
	write_cmos_sensor_8(0x5d3a, 0x50);
	//flip_off_mirror_on
	#ifdef sensor_flip
		#ifdef sensor_mirror
		write_cmos_sensor_8(0x3820, 0xc5);
		write_cmos_sensor_8(0x3821, 0x06);
		write_cmos_sensor_8(0x4000, 0x57);
		#else
		write_cmos_sensor_8(0x3820, 0xc5);
		write_cmos_sensor_8(0x3821, 0x02);
		write_cmos_sensor_8(0x4000, 0x57);
		#endif	
	#else
		#ifdef sensor_mirror
		write_cmos_sensor_8(0x3820, 0x81);
		write_cmos_sensor_8(0x3821, 0x06);
		write_cmos_sensor_8(0x4000, 0x17);
		#else
		write_cmos_sensor_8(0x3820, 0x81);
		write_cmos_sensor_8(0x3821, 0x02);
		write_cmos_sensor_8(0x4000, 0x17);
		#endif	
	#endif
	//align_old_setting
	write_cmos_sensor_8(0x5000, 0x9b);
	write_cmos_sensor_8(0x5002, 0x1c);
	write_cmos_sensor_8(0x5b85, 0x10);
	write_cmos_sensor_8(0x5b87, 0x20);
	write_cmos_sensor_8(0x5b89, 0x40);
	write_cmos_sensor_8(0x5b8b, 0x80);
	write_cmos_sensor_8(0x5b8f, 0x18);
	write_cmos_sensor_8(0x5b90, 0x18);
	write_cmos_sensor_8(0x5b91, 0x18);
	write_cmos_sensor_8(0x5b95, 0x18);
	write_cmos_sensor_8(0x5b96, 0x18);
	write_cmos_sensor_8(0x5b97, 0x18);
	//Disable PD
	write_cmos_sensor_8(0x5001, 0x4a);
	write_cmos_sensor_8(0x3018, 0x5a);
	write_cmos_sensor_8(0x366c, 0x00);
	write_cmos_sensor_8(0x3641, 0x00);
	write_cmos_sensor_8(0x5d29, 0x00);
	//add full_15_difference (default)
	write_cmos_sensor_8(0x030a, 0x00);
	write_cmos_sensor_8(0x0311, 0x00);
	write_cmos_sensor_8(0x3606, 0x55);
	write_cmos_sensor_8(0x3607, 0x05);
	write_cmos_sensor_8(0x360b, 0x48);
	write_cmos_sensor_8(0x360c, 0x18);
	write_cmos_sensor_8(0x3720, 0x88);
	write_cmos_sensor_8(0x0100, 0x01);
	mDELAY(40);
	
}	/*	preview_setting  */


static void normal_capture_setting(void)
{
//	int retry=0;

	LOG_INF("E! ");

	/*
	 * @@ OV16880 4lane 4672x3504 30fps
	 * 100 99 4672 3504
	 * 102 3601 bb8 ;Pather tool use only
	 * Pclk clock frequency: 576Mhz
	 * linelength = 5024(0x13a0)
	 * framelength = 3824(0xef0)
	 * grabwindow_width  = 4672
	 * grabwindow_height = 3504
	 * max_framerate: 30fps	
	 * mipi_datarate per lane: 1440Mbps
	 */

	write_cmos_sensor_8(0x0100, 0x00);
	write_cmos_sensor_8(0x0302, 0x3c);
	write_cmos_sensor_8(0x030f, 0x03);
	write_cmos_sensor_8(0x3501, 0x0e);
	write_cmos_sensor_8(0x3502, 0xea);
	write_cmos_sensor_8(0x3726, 0x00);
	write_cmos_sensor_8(0x3727, 0x22);
	write_cmos_sensor_8(0x3729, 0x00);
	write_cmos_sensor_8(0x372f, 0x92);
	write_cmos_sensor_8(0x3760, 0x92);
	write_cmos_sensor_8(0x37a1, 0x02);
	write_cmos_sensor_8(0x37a5, 0x96);
	write_cmos_sensor_8(0x37a2, 0x04);
	write_cmos_sensor_8(0x37a3, 0x23);
	write_cmos_sensor_8(0x3800, 0x00);
	write_cmos_sensor_8(0x3801, 0x00);
	write_cmos_sensor_8(0x3802, 0x00);
	write_cmos_sensor_8(0x3803, 0x08);
	write_cmos_sensor_8(0x3804, 0x12);
	write_cmos_sensor_8(0x3805, 0x5f);
	write_cmos_sensor_8(0x3806, 0x0d);
	write_cmos_sensor_8(0x3807, 0xc7);
	write_cmos_sensor_8(0x3808, 0x12);
	write_cmos_sensor_8(0x3809, 0x40);
	write_cmos_sensor_8(0x380a, 0x0d);
	write_cmos_sensor_8(0x380b, 0xb0);
	write_cmos_sensor_8(0x380c, 0x13);
	write_cmos_sensor_8(0x380d, 0xa0);
	write_cmos_sensor_8(0x380e, 0x0e);
	write_cmos_sensor_8(0x380f, 0xf0);
	write_cmos_sensor_8(0x3810, 0x0);
	write_cmos_sensor_8(0x3811, 0x10);
	write_cmos_sensor_8(0x3812, 0x0);
	write_cmos_sensor_8(0x3813, 0x08);
	write_cmos_sensor_8(0x3814, 0x11);
	write_cmos_sensor_8(0x3815, 0x11);
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x3836, 0x14);
	write_cmos_sensor_8(0x3841, 0x04);
	write_cmos_sensor_8(0x4006, 0x00);
	write_cmos_sensor_8(0x4007, 0x00);
	write_cmos_sensor_8(0x4009, 0x0f);
	write_cmos_sensor_8(0x4050, 0x04);
	write_cmos_sensor_8(0x4051, 0x0f);
	write_cmos_sensor_8(0x4501, 0x38);
	write_cmos_sensor_8(0x4837, 0x0b);
	write_cmos_sensor_8(0x5201, 0x71);
	write_cmos_sensor_8(0x5203, 0x10);
	write_cmos_sensor_8(0x5205, 0x90);
	write_cmos_sensor_8(0x5500, 0x00);
	write_cmos_sensor_8(0x5501, 0x00);
	write_cmos_sensor_8(0x5502, 0x00);
	write_cmos_sensor_8(0x5503, 0x00);
	write_cmos_sensor_8(0x5508, 0x20);
	write_cmos_sensor_8(0x5509, 0x00);
	write_cmos_sensor_8(0x550a, 0x20);
	write_cmos_sensor_8(0x550b, 0x00);
	write_cmos_sensor_8(0x5510, 0x00);
	write_cmos_sensor_8(0x5511, 0x00);
	write_cmos_sensor_8(0x5512, 0x00);
	write_cmos_sensor_8(0x5513, 0x00);
	write_cmos_sensor_8(0x5518, 0x20);
	write_cmos_sensor_8(0x5519, 0x00);
	write_cmos_sensor_8(0x551a, 0x20);
	write_cmos_sensor_8(0x551b, 0x00);
	write_cmos_sensor_8(0x5520, 0x00);
	write_cmos_sensor_8(0x5521, 0x00);
	write_cmos_sensor_8(0x5522, 0x00);
	write_cmos_sensor_8(0x5523, 0x00);
	write_cmos_sensor_8(0x5528, 0x00);
	write_cmos_sensor_8(0x5529, 0x20);
	write_cmos_sensor_8(0x552a, 0x00);
	write_cmos_sensor_8(0x552b, 0x20);
	write_cmos_sensor_8(0x5530, 0x00);
	write_cmos_sensor_8(0x5531, 0x00);
	write_cmos_sensor_8(0x5532, 0x00);
	write_cmos_sensor_8(0x5533, 0x00);
	write_cmos_sensor_8(0x5538, 0x00);
	write_cmos_sensor_8(0x5539, 0x20);
	write_cmos_sensor_8(0x553a, 0x00);
	write_cmos_sensor_8(0x553b, 0x20);
	write_cmos_sensor_8(0x5540, 0x00);
	write_cmos_sensor_8(0x5541, 0x00);
	write_cmos_sensor_8(0x5542, 0x00);
	write_cmos_sensor_8(0x5543, 0x00);
	write_cmos_sensor_8(0x5548, 0x20);
	write_cmos_sensor_8(0x5549, 0x00);
	write_cmos_sensor_8(0x554a, 0x20);
	write_cmos_sensor_8(0x554b, 0x00);
	write_cmos_sensor_8(0x5550, 0x00);
	write_cmos_sensor_8(0x5551, 0x00);
	write_cmos_sensor_8(0x5552, 0x00);
	write_cmos_sensor_8(0x5553, 0x00);
	write_cmos_sensor_8(0x5558, 0x20);
	write_cmos_sensor_8(0x5559, 0x00);
	write_cmos_sensor_8(0x555a, 0x20);
	write_cmos_sensor_8(0x555b, 0x00);
	write_cmos_sensor_8(0x5560, 0x00);
	write_cmos_sensor_8(0x5561, 0x00);
	write_cmos_sensor_8(0x5562, 0x00);
	write_cmos_sensor_8(0x5563, 0x00);
	write_cmos_sensor_8(0x5568, 0x00);
	write_cmos_sensor_8(0x5569, 0x20);
	write_cmos_sensor_8(0x556a, 0x00);
	write_cmos_sensor_8(0x556b, 0x20);
	write_cmos_sensor_8(0x5570, 0x00);
	write_cmos_sensor_8(0x5571, 0x00);
	write_cmos_sensor_8(0x5572, 0x00);
	write_cmos_sensor_8(0x5573, 0x00);
	write_cmos_sensor_8(0x5578, 0x00);
	write_cmos_sensor_8(0x5579, 0x20);
	write_cmos_sensor_8(0x557a, 0x00);
	write_cmos_sensor_8(0x557b, 0x20);
	write_cmos_sensor_8(0x5580, 0x00);
	write_cmos_sensor_8(0x5581, 0x00);
	write_cmos_sensor_8(0x5582, 0x00);
	write_cmos_sensor_8(0x5583, 0x00);
	write_cmos_sensor_8(0x5588, 0x20);
	write_cmos_sensor_8(0x5589, 0x00);
	write_cmos_sensor_8(0x558a, 0x20);
	write_cmos_sensor_8(0x558b, 0x00);
	write_cmos_sensor_8(0x5590, 0x00);
	write_cmos_sensor_8(0x5591, 0x00);
	write_cmos_sensor_8(0x5592, 0x00);
	write_cmos_sensor_8(0x5593, 0x00);
	write_cmos_sensor_8(0x5598, 0x20);
	write_cmos_sensor_8(0x5599, 0x00);
	write_cmos_sensor_8(0x559a, 0x20);
	write_cmos_sensor_8(0x559b, 0x00);
	write_cmos_sensor_8(0x55a0, 0x00);
	write_cmos_sensor_8(0x55a1, 0x00);
	write_cmos_sensor_8(0x55a2, 0x00);
	write_cmos_sensor_8(0x55a3, 0x00);
	write_cmos_sensor_8(0x55a8, 0x00);
	write_cmos_sensor_8(0x55a9, 0x20);
	write_cmos_sensor_8(0x55aa, 0x00);
	write_cmos_sensor_8(0x55ab, 0x20);
	write_cmos_sensor_8(0x55b0, 0x00);
	write_cmos_sensor_8(0x55b1, 0x00);
	write_cmos_sensor_8(0x55b2, 0x00);
	write_cmos_sensor_8(0x55b3, 0x00);
	write_cmos_sensor_8(0x55b8, 0x00);
	write_cmos_sensor_8(0x55b9, 0x20);
	write_cmos_sensor_8(0x55ba, 0x00);
	write_cmos_sensor_8(0x55bb, 0x20);
	write_cmos_sensor_8(0x55c0, 0x00);
	write_cmos_sensor_8(0x55c1, 0x00);
	write_cmos_sensor_8(0x55c2, 0x00);
	write_cmos_sensor_8(0x55c3, 0x00);
	write_cmos_sensor_8(0x55c8, 0x20);
	write_cmos_sensor_8(0x55c9, 0x00);
	write_cmos_sensor_8(0x55ca, 0x20);
	write_cmos_sensor_8(0x55cb, 0x00);
	write_cmos_sensor_8(0x55d0, 0x00);
	write_cmos_sensor_8(0x55d1, 0x00);
	write_cmos_sensor_8(0x55d2, 0x00);
	write_cmos_sensor_8(0x55d3, 0x00);
	write_cmos_sensor_8(0x55d8, 0x20);
	write_cmos_sensor_8(0x55d9, 0x00);
	write_cmos_sensor_8(0x55da, 0x20);
	write_cmos_sensor_8(0x55db, 0x00);
	write_cmos_sensor_8(0x55e0, 0x00);
	write_cmos_sensor_8(0x55e1, 0x00);
	write_cmos_sensor_8(0x55e2, 0x00);
	write_cmos_sensor_8(0x55e3, 0x00);
	write_cmos_sensor_8(0x55e8, 0x00);
	write_cmos_sensor_8(0x55e9, 0x20);
	write_cmos_sensor_8(0x55ea, 0x00);
	write_cmos_sensor_8(0x55eb, 0x20);
	write_cmos_sensor_8(0x55f0, 0x00);
	write_cmos_sensor_8(0x55f1, 0x00);
	write_cmos_sensor_8(0x55f2, 0x00);
	write_cmos_sensor_8(0x55f3, 0x00);
	write_cmos_sensor_8(0x55f8, 0x00);
	write_cmos_sensor_8(0x55f9, 0x20);
	write_cmos_sensor_8(0x55fa, 0x00);
	write_cmos_sensor_8(0x55fb, 0x20);
	write_cmos_sensor_8(0x5690, 0x00);
	write_cmos_sensor_8(0x5691, 0x00);
	write_cmos_sensor_8(0x5692, 0x00);
	write_cmos_sensor_8(0x5693, 0x08);
	write_cmos_sensor_8(0x5d24, 0x00);
	write_cmos_sensor_8(0x5d25, 0x00);
	write_cmos_sensor_8(0x5d26, 0x00);
	write_cmos_sensor_8(0x5d27, 0x08);
	write_cmos_sensor_8(0x5d3a, 0x58);
#ifdef sensor_flip
	#ifdef sensor_mirror
	write_cmos_sensor_8(0x3820, 0xc4);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x4000, 0x57);
	#else
	write_cmos_sensor_8(0x3820, 0xc4);
	write_cmos_sensor_8(0x3821, 0x00);
	write_cmos_sensor_8(0x4000, 0x57);
	#endif	
#else
	#ifdef sensor_mirror
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x4000, 0x17);
	#else
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x00);
	write_cmos_sensor_8(0x4000, 0x17);
	#endif	
#endif	
	//new_setting_2015_12_17
	write_cmos_sensor_8(0x5000, 0x9b);
	write_cmos_sensor_8(0x5002, 0x0c);
	write_cmos_sensor_8(0x5b85, 0x0c);
	write_cmos_sensor_8(0x5b87, 0x16);
	write_cmos_sensor_8(0x5b89, 0x28);
	write_cmos_sensor_8(0x5b8b, 0x40);
	write_cmos_sensor_8(0x5b8f, 0x1a);
	write_cmos_sensor_8(0x5b90, 0x1a);
	write_cmos_sensor_8(0x5b91, 0x1a);
	write_cmos_sensor_8(0x5b95, 0x1a);
	write_cmos_sensor_8(0x5b96, 0x1a);
	write_cmos_sensor_8(0x5b97, 0x1a);
	//PD_Restore_On (Default)
	write_cmos_sensor_8(0x5001, 0x4e);
	write_cmos_sensor_8(0x3018, 0xda);
	write_cmos_sensor_8(0x366c, 0x10);
	write_cmos_sensor_8(0x3641, 0x01);
	write_cmos_sensor_8(0x5d29, 0x80);
	//add full_15_difference (default), 0x00); 
	write_cmos_sensor_8(0x030a, 0x00);
	write_cmos_sensor_8(0x0311, 0x00);
	write_cmos_sensor_8(0x3606, 0x55);
	write_cmos_sensor_8(0x3607, 0x05);
	write_cmos_sensor_8(0x360b, 0x48);
	write_cmos_sensor_8(0x360c, 0x18);
	write_cmos_sensor_8(0x3720, 0x88);

	/*PDAF*/
	/*PDAF focus windows range*/
	if(imgsensor.pdaf_mode == 1){
		LOG_INF( "PDAF mode!");
		write_cmos_sensor_8(0x0302, 0x3d);/*full speed - increase a bit mipi speed for VC mode*/
		write_cmos_sensor_8(0x3641, 0x00);
		write_cmos_sensor_8(0x3661, 0x0f);
		write_cmos_sensor_8(0x4605, 0x03);
		write_cmos_sensor_8(0x4640, 0x00);/*Package amount*/
		write_cmos_sensor_8(0x4641, 0x23);/*14x8 = 112 package*/
		write_cmos_sensor_8(0x4643, 0x08);/*Fix H-size*/
		write_cmos_sensor_8(0x4645, 0x04);
		//write_cmos_sensor_8(0x4809, 0x2b);/*Type data = raw10*/
		write_cmos_sensor_8(0x4813, 0x98);
		write_cmos_sensor_8(0x486e, 0x07);
		write_cmos_sensor_8(0x5001, 0x4a); /*DPC disable:0x40. DPC enable:0x00 */
		write_cmos_sensor_8(0x5d29, 0x00);
	}
	write_cmos_sensor_8(0x0100, 0x01);	
	mDELAY(40);
	
  LOG_INF( "Exit!");
}

static void pip_capture_setting(void)
{
//	int retry=0;
	LOG_INF( "OV16880 PIP setting Enter!");

 	/*
	 *@@ OV16880 4lane 4672x3504 15fps
	 *100 99 4672 3504
	 *102 3601 bb8 ;Pather tool use only
	 *Pclk clock frequency: 288Mhz
	 *linelength = 5024(0x13a0)
	 *framelength = 3824(0xef0)
	 *grabwindow_width  = 4672
	 *grabwindow_height = 3504
	 *max_framerate: 15fps	
	 *mipi_datarate per lane: 720Mbps
	 */
	write_cmos_sensor_8(0x0100, 0x00);
	write_cmos_sensor_8(0x0302, 0x3c);
	write_cmos_sensor_8(0x030f, 0x03);
	write_cmos_sensor_8(0x3501, 0x0e);
	write_cmos_sensor_8(0x3502, 0xea);
	write_cmos_sensor_8(0x3726, 0x00);
	write_cmos_sensor_8(0x3727, 0x22);
	write_cmos_sensor_8(0x3729, 0x00);
	write_cmos_sensor_8(0x372f, 0x92);
	write_cmos_sensor_8(0x3760, 0x92);
	write_cmos_sensor_8(0x37a1, 0x02);
	write_cmos_sensor_8(0x37a5, 0x96);
	write_cmos_sensor_8(0x37a2, 0x04);
	write_cmos_sensor_8(0x37a3, 0x23);
	write_cmos_sensor_8(0x3800, 0x00);
	write_cmos_sensor_8(0x3801, 0x00);
	write_cmos_sensor_8(0x3802, 0x00);
	write_cmos_sensor_8(0x3803, 0x08);
	write_cmos_sensor_8(0x3804, 0x12);
	write_cmos_sensor_8(0x3805, 0x5f);
	write_cmos_sensor_8(0x3806, 0x0d);
	write_cmos_sensor_8(0x3807, 0xc7);
	write_cmos_sensor_8(0x3808, 0x12);
	write_cmos_sensor_8(0x3809, 0x40);
	write_cmos_sensor_8(0x380a, 0x0d);
	write_cmos_sensor_8(0x380b, 0xb0);
	write_cmos_sensor_8(0x380c, 0x13);
	write_cmos_sensor_8(0x380d, 0xa0);
	write_cmos_sensor_8(0x380e, 0x0e);
	write_cmos_sensor_8(0x380f, 0xf0);
	write_cmos_sensor_8(0x3811, 0x10);
	write_cmos_sensor_8(0x3813, 0x08);
	write_cmos_sensor_8(0x3814, 0x11);
	write_cmos_sensor_8(0x3815, 0x11);
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x3836, 0x14);
	write_cmos_sensor_8(0x3841, 0x04);
	write_cmos_sensor_8(0x4006, 0x00);
	write_cmos_sensor_8(0x4007, 0x00);
	write_cmos_sensor_8(0x4009, 0x0f);
	write_cmos_sensor_8(0x4050, 0x04);
	write_cmos_sensor_8(0x4051, 0x0f);
	write_cmos_sensor_8(0x4501, 0x38);
	write_cmos_sensor_8(0x4837, 0x16);
	write_cmos_sensor_8(0x5201, 0x71);
	write_cmos_sensor_8(0x5203, 0x10);
	write_cmos_sensor_8(0x5205, 0x90);
	write_cmos_sensor_8(0x5500, 0x00);
	write_cmos_sensor_8(0x5501, 0x00);
	write_cmos_sensor_8(0x5502, 0x00);
	write_cmos_sensor_8(0x5503, 0x00);
	write_cmos_sensor_8(0x5508, 0x20);
	write_cmos_sensor_8(0x5509, 0x00);
	write_cmos_sensor_8(0x550a, 0x20);
	write_cmos_sensor_8(0x550b, 0x00);
	write_cmos_sensor_8(0x5510, 0x00);
	write_cmos_sensor_8(0x5511, 0x00);
	write_cmos_sensor_8(0x5512, 0x00);
	write_cmos_sensor_8(0x5513, 0x00);
	write_cmos_sensor_8(0x5518, 0x20);
	write_cmos_sensor_8(0x5519, 0x00);
	write_cmos_sensor_8(0x551a, 0x20);
	write_cmos_sensor_8(0x551b, 0x00);
	write_cmos_sensor_8(0x5520, 0x00);
	write_cmos_sensor_8(0x5521, 0x00);
	write_cmos_sensor_8(0x5522, 0x00);
	write_cmos_sensor_8(0x5523, 0x00);
	write_cmos_sensor_8(0x5528, 0x00);
	write_cmos_sensor_8(0x5529, 0x20);
	write_cmos_sensor_8(0x552a, 0x00);
	write_cmos_sensor_8(0x552b, 0x20);
	write_cmos_sensor_8(0x5530, 0x00);
	write_cmos_sensor_8(0x5531, 0x00);
	write_cmos_sensor_8(0x5532, 0x00);
	write_cmos_sensor_8(0x5533, 0x00);
	write_cmos_sensor_8(0x5538, 0x00);
	write_cmos_sensor_8(0x5539, 0x20);
	write_cmos_sensor_8(0x553a, 0x00);
	write_cmos_sensor_8(0x553b, 0x20);
	write_cmos_sensor_8(0x5540, 0x00);
	write_cmos_sensor_8(0x5541, 0x00);
	write_cmos_sensor_8(0x5542, 0x00);
	write_cmos_sensor_8(0x5543, 0x00);
	write_cmos_sensor_8(0x5548, 0x20);
	write_cmos_sensor_8(0x5549, 0x00);
	write_cmos_sensor_8(0x554a, 0x20);
	write_cmos_sensor_8(0x554b, 0x00);
	write_cmos_sensor_8(0x5550, 0x00);
	write_cmos_sensor_8(0x5551, 0x00);
	write_cmos_sensor_8(0x5552, 0x00);
	write_cmos_sensor_8(0x5553, 0x00);
	write_cmos_sensor_8(0x5558, 0x20);
	write_cmos_sensor_8(0x5559, 0x00);
	write_cmos_sensor_8(0x555a, 0x20);
	write_cmos_sensor_8(0x555b, 0x00);
	write_cmos_sensor_8(0x5560, 0x00);
	write_cmos_sensor_8(0x5561, 0x00);
	write_cmos_sensor_8(0x5562, 0x00);
	write_cmos_sensor_8(0x5563, 0x00);
	write_cmos_sensor_8(0x5568, 0x00);
	write_cmos_sensor_8(0x5569, 0x20);
	write_cmos_sensor_8(0x556a, 0x00);
	write_cmos_sensor_8(0x556b, 0x20);
	write_cmos_sensor_8(0x5570, 0x00);
	write_cmos_sensor_8(0x5571, 0x00);
	write_cmos_sensor_8(0x5572, 0x00);
	write_cmos_sensor_8(0x5573, 0x00);
	write_cmos_sensor_8(0x5578, 0x00);
	write_cmos_sensor_8(0x5579, 0x20);
	write_cmos_sensor_8(0x557a, 0x00);
	write_cmos_sensor_8(0x557b, 0x20);
	write_cmos_sensor_8(0x5580, 0x00);
	write_cmos_sensor_8(0x5581, 0x00);
	write_cmos_sensor_8(0x5582, 0x00);
	write_cmos_sensor_8(0x5583, 0x00);
	write_cmos_sensor_8(0x5588, 0x20);
	write_cmos_sensor_8(0x5589, 0x00);
	write_cmos_sensor_8(0x558a, 0x20);
	write_cmos_sensor_8(0x558b, 0x00);
	write_cmos_sensor_8(0x5590, 0x00);
	write_cmos_sensor_8(0x5591, 0x00);
	write_cmos_sensor_8(0x5592, 0x00);
	write_cmos_sensor_8(0x5593, 0x00);
	write_cmos_sensor_8(0x5598, 0x20);
	write_cmos_sensor_8(0x5599, 0x00);
	write_cmos_sensor_8(0x559a, 0x20);
	write_cmos_sensor_8(0x559b, 0x00);
	write_cmos_sensor_8(0x55a0, 0x00);
	write_cmos_sensor_8(0x55a1, 0x00);
	write_cmos_sensor_8(0x55a2, 0x00);
	write_cmos_sensor_8(0x55a3, 0x00);
	write_cmos_sensor_8(0x55a8, 0x00);
	write_cmos_sensor_8(0x55a9, 0x20);
	write_cmos_sensor_8(0x55aa, 0x00);
	write_cmos_sensor_8(0x55ab, 0x20);
	write_cmos_sensor_8(0x55b0, 0x00);
	write_cmos_sensor_8(0x55b1, 0x00);
	write_cmos_sensor_8(0x55b2, 0x00);
	write_cmos_sensor_8(0x55b3, 0x00);
	write_cmos_sensor_8(0x55b8, 0x00);
	write_cmos_sensor_8(0x55b9, 0x20);
	write_cmos_sensor_8(0x55ba, 0x00);
	write_cmos_sensor_8(0x55bb, 0x20);
	write_cmos_sensor_8(0x55c0, 0x00);
	write_cmos_sensor_8(0x55c1, 0x00);
	write_cmos_sensor_8(0x55c2, 0x00);
	write_cmos_sensor_8(0x55c3, 0x00);
	write_cmos_sensor_8(0x55c8, 0x20);
	write_cmos_sensor_8(0x55c9, 0x00);
	write_cmos_sensor_8(0x55ca, 0x20);
	write_cmos_sensor_8(0x55cb, 0x00);
	write_cmos_sensor_8(0x55d0, 0x00);
	write_cmos_sensor_8(0x55d1, 0x00);
	write_cmos_sensor_8(0x55d2, 0x00);
	write_cmos_sensor_8(0x55d3, 0x00);
	write_cmos_sensor_8(0x55d8, 0x20);
	write_cmos_sensor_8(0x55d9, 0x00);
	write_cmos_sensor_8(0x55da, 0x20);
	write_cmos_sensor_8(0x55db, 0x00);
	write_cmos_sensor_8(0x55e0, 0x00);
	write_cmos_sensor_8(0x55e1, 0x00);
	write_cmos_sensor_8(0x55e2, 0x00);
	write_cmos_sensor_8(0x55e3, 0x00);
	write_cmos_sensor_8(0x55e8, 0x00);
	write_cmos_sensor_8(0x55e9, 0x20);
	write_cmos_sensor_8(0x55ea, 0x00);
	write_cmos_sensor_8(0x55eb, 0x20);
	write_cmos_sensor_8(0x55f0, 0x00);
	write_cmos_sensor_8(0x55f1, 0x00);
	write_cmos_sensor_8(0x55f2, 0x00);
	write_cmos_sensor_8(0x55f3, 0x00);
	write_cmos_sensor_8(0x55f8, 0x00);
	write_cmos_sensor_8(0x55f9, 0x20);
	write_cmos_sensor_8(0x55fa, 0x00);
	write_cmos_sensor_8(0x55fb, 0x20);
	write_cmos_sensor_8(0x5690, 0x00);
	write_cmos_sensor_8(0x5691, 0x00);
	write_cmos_sensor_8(0x5692, 0x00);
	write_cmos_sensor_8(0x5693, 0x08);
	write_cmos_sensor_8(0x5d24, 0x00);
	write_cmos_sensor_8(0x5d25, 0x00);
	write_cmos_sensor_8(0x5d26, 0x00);
	write_cmos_sensor_8(0x5d27, 0x08);
	write_cmos_sensor_8(0x5d3a, 0x58);
	
#ifdef sensor_flip
#ifdef sensor_mirror
write_cmos_sensor_8(0x3820, 0xc4);
write_cmos_sensor_8(0x3821, 0x04);
write_cmos_sensor_8(0x4000, 0x57);
#else
write_cmos_sensor_8(0x3820, 0xc4);
write_cmos_sensor_8(0x3821, 0x00);
write_cmos_sensor_8(0x4000, 0x57);
#endif	
#else
#ifdef sensor_mirror
write_cmos_sensor_8(0x3820, 0x80);
write_cmos_sensor_8(0x3821, 0x04);
write_cmos_sensor_8(0x4000, 0x17);
#else
write_cmos_sensor_8(0x3820, 0x80);
write_cmos_sensor_8(0x3821, 0x00);
write_cmos_sensor_8(0x4000, 0x17);
#endif	
#endif	

	//base_on_03b
	write_cmos_sensor_8(0x5000, 0x9b);
	write_cmos_sensor_8(0x5002, 0x0c);
	write_cmos_sensor_8(0x5b85, 0x0c);
	write_cmos_sensor_8(0x5b87, 0x16);
	write_cmos_sensor_8(0x5b89, 0x28);
	write_cmos_sensor_8(0x5b8b, 0x40);
	write_cmos_sensor_8(0x5b8f, 0x1a);
	write_cmos_sensor_8(0x5b90, 0x1a);
	write_cmos_sensor_8(0x5b91, 0x1a);
	write_cmos_sensor_8(0x5b95, 0x1a);
	write_cmos_sensor_8(0x5b96, 0x1a);
	write_cmos_sensor_8(0x5b97, 0x1a);
	// PD_Restore_On (Default)
	write_cmos_sensor_8(0x5001, 0x4e);
	write_cmos_sensor_8(0x3018, 0xda);
	write_cmos_sensor_8(0x366c, 0x10);
	write_cmos_sensor_8(0x3641, 0x01);
	write_cmos_sensor_8(0x5d29, 0x80);
	// add full_15_difference
	write_cmos_sensor_8(0x030a, 0x01);
	write_cmos_sensor_8(0x0311, 0x01);
	write_cmos_sensor_8(0x3606, 0x22);
	write_cmos_sensor_8(0x3607, 0x02);
	write_cmos_sensor_8(0x360b, 0x4f);
	write_cmos_sensor_8(0x360c, 0x1f);
	write_cmos_sensor_8(0x3720, 0x44);
	write_cmos_sensor_8(0x0100, 0x01);

}

static void pip_capture_15fps_setting(void)
{

	LOG_INF( "OV16880 PIP setting Enter!");

 	/*
	 *@@ OV16880 4lane 4672x3504 15fps
	 *100 99 4672 3504
	 *102 3601 bb8 ;Pather tool use only
	 *Pclk clock frequency: 288Mhz
	 *linelength = 5024(0x13a0)
	 *framelength = 3824(0xef0)
	 *grabwindow_width  = 4672
	 *grabwindow_height = 3504
	 *max_framerate: 15fps	
	 *mipi_datarate per lane: 720Mbps
	 */
	write_cmos_sensor_8(0x0100, 0x00);
	write_cmos_sensor_8(0x0302, 0x3c);
	write_cmos_sensor_8(0x030f, 0x03);
	write_cmos_sensor_8(0x3501, 0x0e);
	write_cmos_sensor_8(0x3502, 0xea);
	write_cmos_sensor_8(0x3726, 0x00);
	write_cmos_sensor_8(0x3727, 0x22);
	write_cmos_sensor_8(0x3729, 0x00);
	write_cmos_sensor_8(0x372f, 0x92);
	write_cmos_sensor_8(0x3760, 0x92);
	write_cmos_sensor_8(0x37a1, 0x02);
	write_cmos_sensor_8(0x37a5, 0x96);
	write_cmos_sensor_8(0x37a2, 0x04);
	write_cmos_sensor_8(0x37a3, 0x23);
	write_cmos_sensor_8(0x3800, 0x00);
	write_cmos_sensor_8(0x3801, 0x00);
	write_cmos_sensor_8(0x3802, 0x00);
	write_cmos_sensor_8(0x3803, 0x08);
	write_cmos_sensor_8(0x3804, 0x12);
	write_cmos_sensor_8(0x3805, 0x5f);
	write_cmos_sensor_8(0x3806, 0x0d);
	write_cmos_sensor_8(0x3807, 0xc7);
	write_cmos_sensor_8(0x3808, 0x12);
	write_cmos_sensor_8(0x3809, 0x40);
	write_cmos_sensor_8(0x380a, 0x0d);
	write_cmos_sensor_8(0x380b, 0xb0);
	write_cmos_sensor_8(0x380c, 0x13);
	write_cmos_sensor_8(0x380d, 0xa0);
	write_cmos_sensor_8(0x380e, 0x0e);
	write_cmos_sensor_8(0x380f, 0xf0);
	write_cmos_sensor_8(0x3811, 0x10);
	write_cmos_sensor_8(0x3813, 0x08);
	write_cmos_sensor_8(0x3814, 0x11);
	write_cmos_sensor_8(0x3815, 0x11);
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x3836, 0x14);
	write_cmos_sensor_8(0x3841, 0x04);
	write_cmos_sensor_8(0x4006, 0x00);
	write_cmos_sensor_8(0x4007, 0x00);
	write_cmos_sensor_8(0x4009, 0x0f);
	write_cmos_sensor_8(0x4050, 0x04);
	write_cmos_sensor_8(0x4051, 0x0f);
	write_cmos_sensor_8(0x4501, 0x38);
	write_cmos_sensor_8(0x4837, 0x16);
	write_cmos_sensor_8(0x5201, 0x71);
	write_cmos_sensor_8(0x5203, 0x10);
	write_cmos_sensor_8(0x5205, 0x90);
	write_cmos_sensor_8(0x5500, 0x00);
	write_cmos_sensor_8(0x5501, 0x00);
	write_cmos_sensor_8(0x5502, 0x00);
	write_cmos_sensor_8(0x5503, 0x00);
	write_cmos_sensor_8(0x5508, 0x20);
	write_cmos_sensor_8(0x5509, 0x00);
	write_cmos_sensor_8(0x550a, 0x20);
	write_cmos_sensor_8(0x550b, 0x00);
	write_cmos_sensor_8(0x5510, 0x00);
	write_cmos_sensor_8(0x5511, 0x00);
	write_cmos_sensor_8(0x5512, 0x00);
	write_cmos_sensor_8(0x5513, 0x00);
	write_cmos_sensor_8(0x5518, 0x20);
	write_cmos_sensor_8(0x5519, 0x00);
	write_cmos_sensor_8(0x551a, 0x20);
	write_cmos_sensor_8(0x551b, 0x00);
	write_cmos_sensor_8(0x5520, 0x00);
	write_cmos_sensor_8(0x5521, 0x00);
	write_cmos_sensor_8(0x5522, 0x00);
	write_cmos_sensor_8(0x5523, 0x00);
	write_cmos_sensor_8(0x5528, 0x00);
	write_cmos_sensor_8(0x5529, 0x20);
	write_cmos_sensor_8(0x552a, 0x00);
	write_cmos_sensor_8(0x552b, 0x20);
	write_cmos_sensor_8(0x5530, 0x00);
	write_cmos_sensor_8(0x5531, 0x00);
	write_cmos_sensor_8(0x5532, 0x00);
	write_cmos_sensor_8(0x5533, 0x00);
	write_cmos_sensor_8(0x5538, 0x00);
	write_cmos_sensor_8(0x5539, 0x20);
	write_cmos_sensor_8(0x553a, 0x00);
	write_cmos_sensor_8(0x553b, 0x20);
	write_cmos_sensor_8(0x5540, 0x00);
	write_cmos_sensor_8(0x5541, 0x00);
	write_cmos_sensor_8(0x5542, 0x00);
	write_cmos_sensor_8(0x5543, 0x00);
	write_cmos_sensor_8(0x5548, 0x20);
	write_cmos_sensor_8(0x5549, 0x00);
	write_cmos_sensor_8(0x554a, 0x20);
	write_cmos_sensor_8(0x554b, 0x00);
	write_cmos_sensor_8(0x5550, 0x00);
	write_cmos_sensor_8(0x5551, 0x00);
	write_cmos_sensor_8(0x5552, 0x00);
	write_cmos_sensor_8(0x5553, 0x00);
	write_cmos_sensor_8(0x5558, 0x20);
	write_cmos_sensor_8(0x5559, 0x00);
	write_cmos_sensor_8(0x555a, 0x20);
	write_cmos_sensor_8(0x555b, 0x00);
	write_cmos_sensor_8(0x5560, 0x00);
	write_cmos_sensor_8(0x5561, 0x00);
	write_cmos_sensor_8(0x5562, 0x00);
	write_cmos_sensor_8(0x5563, 0x00);
	write_cmos_sensor_8(0x5568, 0x00);
	write_cmos_sensor_8(0x5569, 0x20);
	write_cmos_sensor_8(0x556a, 0x00);
	write_cmos_sensor_8(0x556b, 0x20);
	write_cmos_sensor_8(0x5570, 0x00);
	write_cmos_sensor_8(0x5571, 0x00);
	write_cmos_sensor_8(0x5572, 0x00);
	write_cmos_sensor_8(0x5573, 0x00);
	write_cmos_sensor_8(0x5578, 0x00);
	write_cmos_sensor_8(0x5579, 0x20);
	write_cmos_sensor_8(0x557a, 0x00);
	write_cmos_sensor_8(0x557b, 0x20);
	write_cmos_sensor_8(0x5580, 0x00);
	write_cmos_sensor_8(0x5581, 0x00);
	write_cmos_sensor_8(0x5582, 0x00);
	write_cmos_sensor_8(0x5583, 0x00);
	write_cmos_sensor_8(0x5588, 0x20);
	write_cmos_sensor_8(0x5589, 0x00);
	write_cmos_sensor_8(0x558a, 0x20);
	write_cmos_sensor_8(0x558b, 0x00);
	write_cmos_sensor_8(0x5590, 0x00);
	write_cmos_sensor_8(0x5591, 0x00);
	write_cmos_sensor_8(0x5592, 0x00);
	write_cmos_sensor_8(0x5593, 0x00);
	write_cmos_sensor_8(0x5598, 0x20);
	write_cmos_sensor_8(0x5599, 0x00);
	write_cmos_sensor_8(0x559a, 0x20);
	write_cmos_sensor_8(0x559b, 0x00);
	write_cmos_sensor_8(0x55a0, 0x00);
	write_cmos_sensor_8(0x55a1, 0x00);
	write_cmos_sensor_8(0x55a2, 0x00);
	write_cmos_sensor_8(0x55a3, 0x00);
	write_cmos_sensor_8(0x55a8, 0x00);
	write_cmos_sensor_8(0x55a9, 0x20);
	write_cmos_sensor_8(0x55aa, 0x00);
	write_cmos_sensor_8(0x55ab, 0x20);
	write_cmos_sensor_8(0x55b0, 0x00);
	write_cmos_sensor_8(0x55b1, 0x00);
	write_cmos_sensor_8(0x55b2, 0x00);
	write_cmos_sensor_8(0x55b3, 0x00);
	write_cmos_sensor_8(0x55b8, 0x00);
	write_cmos_sensor_8(0x55b9, 0x20);
	write_cmos_sensor_8(0x55ba, 0x00);
	write_cmos_sensor_8(0x55bb, 0x20);
	write_cmos_sensor_8(0x55c0, 0x00);
	write_cmos_sensor_8(0x55c1, 0x00);
	write_cmos_sensor_8(0x55c2, 0x00);
	write_cmos_sensor_8(0x55c3, 0x00);
	write_cmos_sensor_8(0x55c8, 0x20);
	write_cmos_sensor_8(0x55c9, 0x00);
	write_cmos_sensor_8(0x55ca, 0x20);
	write_cmos_sensor_8(0x55cb, 0x00);
	write_cmos_sensor_8(0x55d0, 0x00);
	write_cmos_sensor_8(0x55d1, 0x00);
	write_cmos_sensor_8(0x55d2, 0x00);
	write_cmos_sensor_8(0x55d3, 0x00);
	write_cmos_sensor_8(0x55d8, 0x20);
	write_cmos_sensor_8(0x55d9, 0x00);
	write_cmos_sensor_8(0x55da, 0x20);
	write_cmos_sensor_8(0x55db, 0x00);
	write_cmos_sensor_8(0x55e0, 0x00);
	write_cmos_sensor_8(0x55e1, 0x00);
	write_cmos_sensor_8(0x55e2, 0x00);
	write_cmos_sensor_8(0x55e3, 0x00);
	write_cmos_sensor_8(0x55e8, 0x00);
	write_cmos_sensor_8(0x55e9, 0x20);
	write_cmos_sensor_8(0x55ea, 0x00);
	write_cmos_sensor_8(0x55eb, 0x20);
	write_cmos_sensor_8(0x55f0, 0x00);
	write_cmos_sensor_8(0x55f1, 0x00);
	write_cmos_sensor_8(0x55f2, 0x00);
	write_cmos_sensor_8(0x55f3, 0x00);
	write_cmos_sensor_8(0x55f8, 0x00);
	write_cmos_sensor_8(0x55f9, 0x20);
	write_cmos_sensor_8(0x55fa, 0x00);
	write_cmos_sensor_8(0x55fb, 0x20);
	write_cmos_sensor_8(0x5690, 0x00);
	write_cmos_sensor_8(0x5691, 0x00);
	write_cmos_sensor_8(0x5692, 0x00);
	write_cmos_sensor_8(0x5693, 0x08);
	write_cmos_sensor_8(0x5d24, 0x00);
	write_cmos_sensor_8(0x5d25, 0x00);
	write_cmos_sensor_8(0x5d26, 0x00);
	write_cmos_sensor_8(0x5d27, 0x08);
	write_cmos_sensor_8(0x5d3a, 0x58);

#ifdef sensor_flip
	#ifdef sensor_mirror
	write_cmos_sensor_8(0x3820, 0xc4);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x4000, 0x57);
	#else
	write_cmos_sensor_8(0x3820, 0xc4);
	write_cmos_sensor_8(0x3821, 0x00);
	write_cmos_sensor_8(0x4000, 0x57);
	#endif	
#else
	#ifdef sensor_mirror
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x4000, 0x17);
	#else
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x00);
	write_cmos_sensor_8(0x4000, 0x17);
	#endif	
#endif	

	//base_on_03b
	write_cmos_sensor_8(0x5000, 0x9b);
	write_cmos_sensor_8(0x5002, 0x0c);
	write_cmos_sensor_8(0x5b85, 0x0c);
	write_cmos_sensor_8(0x5b87, 0x16);
	write_cmos_sensor_8(0x5b89, 0x28);
	write_cmos_sensor_8(0x5b8b, 0x40);
	write_cmos_sensor_8(0x5b8f, 0x1a);
	write_cmos_sensor_8(0x5b90, 0x1a);
	write_cmos_sensor_8(0x5b91, 0x1a);
	write_cmos_sensor_8(0x5b95, 0x1a);
	write_cmos_sensor_8(0x5b96, 0x1a);
	write_cmos_sensor_8(0x5b97, 0x1a);
	// PD_Restore_On (Default)
	write_cmos_sensor_8(0x5001, 0x4e);
	write_cmos_sensor_8(0x3018, 0xda);
	write_cmos_sensor_8(0x366c, 0x10);
	write_cmos_sensor_8(0x3641, 0x01);
	write_cmos_sensor_8(0x5d29, 0x80);
	// add full_15_difference
	write_cmos_sensor_8(0x030a, 0x01);
	write_cmos_sensor_8(0x0311, 0x01);
	write_cmos_sensor_8(0x3606, 0x22);
	write_cmos_sensor_8(0x3607, 0x02);
	write_cmos_sensor_8(0x360b, 0x4f);
	write_cmos_sensor_8(0x360c, 0x1f);
	write_cmos_sensor_8(0x3720, 0x44);
	write_cmos_sensor_8(0x0100, 0x01);

}

static void capture_setting(kal_uint16 currefps)
{

	if(currefps==300)
		normal_capture_setting();
	else if(currefps==200) // PIP
		pip_capture_setting();
	else if(currefps==150)
        pip_capture_15fps_setting();
  else// default
	normal_capture_setting();
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);

	write_cmos_sensor_8(0x0100,0x00);

	write_cmos_sensor_8(0x0302,0x3c);
	write_cmos_sensor_8(0x030f,0x03);
	write_cmos_sensor_8(0x3501,0x0e);
	write_cmos_sensor_8(0x3502,0xea);
	write_cmos_sensor_8(0x3726,0x00);
	write_cmos_sensor_8(0x3727,0x22);
	write_cmos_sensor_8(0x3729,0x00);
	write_cmos_sensor_8(0x372f,0x92);
	write_cmos_sensor_8(0x3760,0x92);
	write_cmos_sensor_8(0x37a1,0x02);
	write_cmos_sensor_8(0x37a5,0x96);
	write_cmos_sensor_8(0x37a2,0x04);
	write_cmos_sensor_8(0x37a3,0x23);
	write_cmos_sensor_8(0x3800,0x00);
	write_cmos_sensor_8(0x3801,0x00);
	write_cmos_sensor_8(0x3802,0x00);
	write_cmos_sensor_8(0x3803,0x08);
	write_cmos_sensor_8(0x3804,0x12);
	write_cmos_sensor_8(0x3805,0x5f);
	write_cmos_sensor_8(0x3806,0x0d);
	write_cmos_sensor_8(0x3807,0xc7);
	write_cmos_sensor_8(0x3808,0x10);
	write_cmos_sensor_8(0x3809,0xb0);
	write_cmos_sensor_8(0x380a,0x09);
	write_cmos_sensor_8(0x380b,0x64);
	write_cmos_sensor_8(0x380c,0x13);
	write_cmos_sensor_8(0x380d,0xa0);
	write_cmos_sensor_8(0x380e,0x0e);
	write_cmos_sensor_8(0x380f,0xf0);
	write_cmos_sensor_8(0x3811,0xd8);
	write_cmos_sensor_8(0x3812,0x02);
	write_cmos_sensor_8(0x3813,0x2e);
	write_cmos_sensor_8(0x3814,0x11);
	write_cmos_sensor_8(0x3815,0x11);
	write_cmos_sensor_8(0x3820,0x80);
	write_cmos_sensor_8(0x3821,0x04);
	write_cmos_sensor_8(0x3836,0x14);
	write_cmos_sensor_8(0x3841,0x04);
	write_cmos_sensor_8(0x4006,0x00);
	write_cmos_sensor_8(0x4007,0x00);
	write_cmos_sensor_8(0x4009,0x0f);
	write_cmos_sensor_8(0x4050,0x04);
	write_cmos_sensor_8(0x4051,0x0f);
	write_cmos_sensor_8(0x4501,0x38);
	write_cmos_sensor_8(0x4837,0x0b);
	write_cmos_sensor_8(0x5201,0x71);
	write_cmos_sensor_8(0x5203,0x10);
	write_cmos_sensor_8(0x5205,0x90);
	write_cmos_sensor_8(0x5500,0x00);
	write_cmos_sensor_8(0x5501,0x00);
	write_cmos_sensor_8(0x5502,0x00);
	write_cmos_sensor_8(0x5503,0x00);
	write_cmos_sensor_8(0x5508,0x20);
	write_cmos_sensor_8(0x5509,0x00);
	write_cmos_sensor_8(0x550a,0x20);
	write_cmos_sensor_8(0x550b,0x00);
	write_cmos_sensor_8(0x5510,0x00);
	write_cmos_sensor_8(0x5511,0x00);
	write_cmos_sensor_8(0x5512,0x00);
	write_cmos_sensor_8(0x5513,0x00);
	write_cmos_sensor_8(0x5518,0x20);
	write_cmos_sensor_8(0x5519,0x00);
	write_cmos_sensor_8(0x551a,0x20);
	write_cmos_sensor_8(0x551b,0x00);
	write_cmos_sensor_8(0x5520,0x00);
	write_cmos_sensor_8(0x5521,0x00);
	write_cmos_sensor_8(0x5522,0x00);
	write_cmos_sensor_8(0x5523,0x00);
	write_cmos_sensor_8(0x5528,0x00);
	write_cmos_sensor_8(0x5529,0x20);
	write_cmos_sensor_8(0x552a,0x00);
	write_cmos_sensor_8(0x552b,0x20);
	write_cmos_sensor_8(0x5530,0x00);
	write_cmos_sensor_8(0x5531,0x00);
	write_cmos_sensor_8(0x5532,0x00);
	write_cmos_sensor_8(0x5533,0x00);
	write_cmos_sensor_8(0x5538,0x00);
	write_cmos_sensor_8(0x5539,0x20);
	write_cmos_sensor_8(0x553a,0x00);
	write_cmos_sensor_8(0x553b,0x20);
	write_cmos_sensor_8(0x5540,0x00);
	write_cmos_sensor_8(0x5541,0x00);
	write_cmos_sensor_8(0x5542,0x00);
	write_cmos_sensor_8(0x5543,0x00);
	write_cmos_sensor_8(0x5548,0x20);
	write_cmos_sensor_8(0x5549,0x00);
	write_cmos_sensor_8(0x554a,0x20);
	write_cmos_sensor_8(0x554b,0x00);
	write_cmos_sensor_8(0x5550,0x00);
	write_cmos_sensor_8(0x5551,0x00);
	write_cmos_sensor_8(0x5552,0x00);
	write_cmos_sensor_8(0x5553,0x00);
	write_cmos_sensor_8(0x5558,0x20);
	write_cmos_sensor_8(0x5559,0x00);
	write_cmos_sensor_8(0x555a,0x20);
	write_cmos_sensor_8(0x555b,0x00);
	write_cmos_sensor_8(0x5560,0x00);
	write_cmos_sensor_8(0x5561,0x00);
	write_cmos_sensor_8(0x5562,0x00);
	write_cmos_sensor_8(0x5563,0x00);
	write_cmos_sensor_8(0x5568,0x00);
	write_cmos_sensor_8(0x5569,0x20);
	write_cmos_sensor_8(0x556a,0x00);
	write_cmos_sensor_8(0x556b,0x20);
	write_cmos_sensor_8(0x5570,0x00);
	write_cmos_sensor_8(0x5571,0x00);
	write_cmos_sensor_8(0x5572,0x00);
	write_cmos_sensor_8(0x5573,0x00);
	write_cmos_sensor_8(0x5578,0x00);
	write_cmos_sensor_8(0x5579,0x20);
	write_cmos_sensor_8(0x557a,0x00);
	write_cmos_sensor_8(0x557b,0x20);
	write_cmos_sensor_8(0x5580,0x00);
	write_cmos_sensor_8(0x5581,0x00);
	write_cmos_sensor_8(0x5582,0x00);
	write_cmos_sensor_8(0x5583,0x00);
	write_cmos_sensor_8(0x5588,0x20);
	write_cmos_sensor_8(0x5589,0x00);
	write_cmos_sensor_8(0x558a,0x20);
	write_cmos_sensor_8(0x558b,0x00);
	write_cmos_sensor_8(0x5590,0x00);
	write_cmos_sensor_8(0x5591,0x00);
	write_cmos_sensor_8(0x5592,0x00);
	write_cmos_sensor_8(0x5593,0x00);
	write_cmos_sensor_8(0x5598,0x20);
	write_cmos_sensor_8(0x5599,0x00);
	write_cmos_sensor_8(0x559a,0x20);
	write_cmos_sensor_8(0x559b,0x00);
	write_cmos_sensor_8(0x55a0,0x00);
	write_cmos_sensor_8(0x55a1,0x00);
	write_cmos_sensor_8(0x55a2,0x00);
	write_cmos_sensor_8(0x55a3,0x00);
	write_cmos_sensor_8(0x55a8,0x00);
	write_cmos_sensor_8(0x55a9,0x20);
	write_cmos_sensor_8(0x55aa,0x00);
	write_cmos_sensor_8(0x55ab,0x20);
	write_cmos_sensor_8(0x55b0,0x00);
	write_cmos_sensor_8(0x55b1,0x00);
	write_cmos_sensor_8(0x55b2,0x00);
	write_cmos_sensor_8(0x55b3,0x00);
	write_cmos_sensor_8(0x55b8,0x00);
	write_cmos_sensor_8(0x55b9,0x20);
	write_cmos_sensor_8(0x55ba,0x00);
	write_cmos_sensor_8(0x55bb,0x20);
	write_cmos_sensor_8(0x55c0,0x00);
	write_cmos_sensor_8(0x55c1,0x00);
	write_cmos_sensor_8(0x55c2,0x00);
	write_cmos_sensor_8(0x55c3,0x00);
	write_cmos_sensor_8(0x55c8,0x20);
	write_cmos_sensor_8(0x55c9,0x00);
	write_cmos_sensor_8(0x55ca,0x20);
	write_cmos_sensor_8(0x55cb,0x00);
	write_cmos_sensor_8(0x55d0,0x00);
	write_cmos_sensor_8(0x55d1,0x00);
	write_cmos_sensor_8(0x55d2,0x00);
	write_cmos_sensor_8(0x55d3,0x00);
	write_cmos_sensor_8(0x55d8,0x20);
	write_cmos_sensor_8(0x55d9,0x00);
	write_cmos_sensor_8(0x55da,0x20);
	write_cmos_sensor_8(0x55db,0x00);
	write_cmos_sensor_8(0x55e0,0x00);
	write_cmos_sensor_8(0x55e1,0x00);
	write_cmos_sensor_8(0x55e2,0x00);
	write_cmos_sensor_8(0x55e3,0x00);
	write_cmos_sensor_8(0x55e8,0x00);
	write_cmos_sensor_8(0x55e9,0x20);
	write_cmos_sensor_8(0x55ea,0x00);
	write_cmos_sensor_8(0x55eb,0x20);
	write_cmos_sensor_8(0x55f0,0x00);
	write_cmos_sensor_8(0x55f1,0x00);
	write_cmos_sensor_8(0x55f2,0x00);
	write_cmos_sensor_8(0x55f3,0x00);
	write_cmos_sensor_8(0x55f8,0x00);
	write_cmos_sensor_8(0x55f9,0x20);
	write_cmos_sensor_8(0x55fa,0x00);
	write_cmos_sensor_8(0x55fb,0x20);
	write_cmos_sensor_8(0x5690,0x00);
	write_cmos_sensor_8(0x5691,0x00);
	write_cmos_sensor_8(0x5692,0x00);
	write_cmos_sensor_8(0x5693,0x08);
	write_cmos_sensor_8(0x5d24,0x00);
	write_cmos_sensor_8(0x5d25,0x00);
	write_cmos_sensor_8(0x5d26,0x00);
	write_cmos_sensor_8(0x5d27,0x08);
	write_cmos_sensor_8(0x5d3a,0x58);
#ifdef sensor_flip
	#ifdef sensor_mirror
	write_cmos_sensor_8(0x3820, 0xc4);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x4000, 0x57);
	#else
	write_cmos_sensor_8(0x3820, 0xc4);
	write_cmos_sensor_8(0x3821, 0x00);
	write_cmos_sensor_8(0x4000, 0x57);
	#endif	
#else
	#ifdef sensor_mirror
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x04);
	write_cmos_sensor_8(0x4000, 0x17);
	#else
	write_cmos_sensor_8(0x3820, 0x80);
	write_cmos_sensor_8(0x3821, 0x00);
	write_cmos_sensor_8(0x4000, 0x17);
	#endif	
#endif

	//@@align_old_setting
	write_cmos_sensor_8(0x5000,0x9b);
	write_cmos_sensor_8(0x5002,0x1c);

	write_cmos_sensor_8(0x5b85,0x10);
	write_cmos_sensor_8(0x5b87,0x20);
	write_cmos_sensor_8(0x5b89,0x40);
	write_cmos_sensor_8(0x5b8b,0x80);

	write_cmos_sensor_8(0x5b8f,0x18);
	write_cmos_sensor_8(0x5b90,0x18);
	write_cmos_sensor_8(0x5b91,0x18);
	write_cmos_sensor_8(0x5b95,0x18);
	write_cmos_sensor_8(0x5b96,0x18);
	write_cmos_sensor_8(0x5b97,0x18);

	//Disable,0xPD
	write_cmos_sensor_8(0x5001,0x4a);
	write_cmos_sensor_8(0x3018,0x5a);
	write_cmos_sensor_8(0x366c,0x00);
	write_cmos_sensor_8(0x3641,0x00);
	write_cmos_sensor_8(0x5d29,0x00);
//;;add full_15_difference (default)
	write_cmos_sensor_8(0x030a, 0x00);
	write_cmos_sensor_8(0x0311, 0x00);
	write_cmos_sensor_8(0x3606, 0x55);
	write_cmos_sensor_8(0x3607, 0x05);
	write_cmos_sensor_8(0x360b, 0x48);
	write_cmos_sensor_8(0x360c, 0x18);
	write_cmos_sensor_8(0x3720, 0x88);

	write_cmos_sensor_8(0x0100,0x01);
	mdelay(40);
  LOG_INF( "Exit!");

}
static void hs_video_setting(void)
{
	//int retry=0;
	LOG_INF("E\n");
	
	/*
	* @@ OV16880 4lane 1280x720 120fps
	* 100 99 1280 720
	* 102 3601 2ee0 ;Pather tool use only
	* Pclk clock frequency: 576Mhz
	* linelength = 5024(0x13a0)
	* framelength = 956(0x3bc)
	* grabwindow_width  = 1280
	* grabwindow_height = 720
	* max_framerate: 120fps	
	* mipi_datarate per lane: 768Mbps
	*/
	/* Sensor Setting	*/

	write_cmos_sensor_8(0x0100, 0x00);
	write_cmos_sensor_8(0x0302, 0x20);
	write_cmos_sensor_8(0x030f, 0x03);
	write_cmos_sensor_8(0x3501, 0x03);
	write_cmos_sensor_8(0x3502, 0xb6);
	write_cmos_sensor_8(0x3726, 0x22);
	write_cmos_sensor_8(0x3727, 0x44);
	write_cmos_sensor_8(0x3729, 0x22);
	write_cmos_sensor_8(0x372f, 0x89);
	write_cmos_sensor_8(0x3760, 0x90);
	write_cmos_sensor_8(0x37a1, 0x00);
	write_cmos_sensor_8(0x37a5, 0xaa);
	write_cmos_sensor_8(0x37a2, 0x00);
	write_cmos_sensor_8(0x37a3, 0x42);
	write_cmos_sensor_8(0x3800, 0x04);
	write_cmos_sensor_8(0x3801, 0x20);
	write_cmos_sensor_8(0x3802, 0x04);
	write_cmos_sensor_8(0x3803, 0x10);
	write_cmos_sensor_8(0x3804, 0x0e);
	write_cmos_sensor_8(0x3805, 0x3f);
	write_cmos_sensor_8(0x3806, 0x09);
	write_cmos_sensor_8(0x3807, 0xbf);
	write_cmos_sensor_8(0x3808, 0x05);
	write_cmos_sensor_8(0x3809, 0x00);
	write_cmos_sensor_8(0x380a, 0x02);
	write_cmos_sensor_8(0x380b, 0xd0);
	write_cmos_sensor_8(0x380c, 0x13);
	write_cmos_sensor_8(0x380d, 0xa0);
	write_cmos_sensor_8(0x380e, 0x03);
	write_cmos_sensor_8(0x380f, 0xbc);
	write_cmos_sensor_8(0x3810, 0x0);
	write_cmos_sensor_8(0x3811, 0x08);
	write_cmos_sensor_8(0x3812, 0x0);
	write_cmos_sensor_8(0x3813, 0x04);
	write_cmos_sensor_8(0x3814, 0x31);
	write_cmos_sensor_8(0x3815, 0x31);
	write_cmos_sensor_8(0x3820, 0x81);
	write_cmos_sensor_8(0x3821, 0x06);
	write_cmos_sensor_8(0x3836, 0x0c);
	write_cmos_sensor_8(0x3841, 0x02);
	write_cmos_sensor_8(0x4006, 0x00);
	write_cmos_sensor_8(0x4007, 0x00);
	write_cmos_sensor_8(0x4009, 0x05);
	write_cmos_sensor_8(0x4050, 0x00);
	write_cmos_sensor_8(0x4051, 0x01);
	write_cmos_sensor_8(0x4501, 0x3c);
	write_cmos_sensor_8(0x4837, 0x14);
	write_cmos_sensor_8(0x5201, 0x62);
	write_cmos_sensor_8(0x5203, 0x08);
	write_cmos_sensor_8(0x5205, 0x48);
	write_cmos_sensor_8(0x5500, 0x80);
	write_cmos_sensor_8(0x5501, 0x80);
	write_cmos_sensor_8(0x5502, 0x80);
	write_cmos_sensor_8(0x5503, 0x80);
	write_cmos_sensor_8(0x5508, 0x80);
	write_cmos_sensor_8(0x5509, 0x80);
	write_cmos_sensor_8(0x550a, 0x80);
	write_cmos_sensor_8(0x550b, 0x80);
	write_cmos_sensor_8(0x5510, 0x08);
	write_cmos_sensor_8(0x5511, 0x08);
	write_cmos_sensor_8(0x5512, 0x08);
	write_cmos_sensor_8(0x5513, 0x08);
	write_cmos_sensor_8(0x5518, 0x08);
	write_cmos_sensor_8(0x5519, 0x08);
	write_cmos_sensor_8(0x551a, 0x08);
	write_cmos_sensor_8(0x551b, 0x08);
	write_cmos_sensor_8(0x5520, 0x80);
	write_cmos_sensor_8(0x5521, 0x80);
	write_cmos_sensor_8(0x5522, 0x80);
	write_cmos_sensor_8(0x5523, 0x80);
	write_cmos_sensor_8(0x5528, 0x80);
	write_cmos_sensor_8(0x5529, 0x80);
	write_cmos_sensor_8(0x552a, 0x80);
	write_cmos_sensor_8(0x552b, 0x80);
	write_cmos_sensor_8(0x5530, 0x08);
	write_cmos_sensor_8(0x5531, 0x08);
	write_cmos_sensor_8(0x5532, 0x08);
	write_cmos_sensor_8(0x5533, 0x08);
	write_cmos_sensor_8(0x5538, 0x08);
	write_cmos_sensor_8(0x5539, 0x08);
	write_cmos_sensor_8(0x553a, 0x08);
	write_cmos_sensor_8(0x553b, 0x08);
	write_cmos_sensor_8(0x5540, 0x80);
	write_cmos_sensor_8(0x5541, 0x80);
	write_cmos_sensor_8(0x5542, 0x80);
	write_cmos_sensor_8(0x5543, 0x80);
	write_cmos_sensor_8(0x5548, 0x80);
	write_cmos_sensor_8(0x5549, 0x80);
	write_cmos_sensor_8(0x554a, 0x80);
	write_cmos_sensor_8(0x554b, 0x80);
	write_cmos_sensor_8(0x5550, 0x08);
	write_cmos_sensor_8(0x5551, 0x08);
	write_cmos_sensor_8(0x5552, 0x08);
	write_cmos_sensor_8(0x5553, 0x08);
	write_cmos_sensor_8(0x5558, 0x08);
	write_cmos_sensor_8(0x5559, 0x08);
	write_cmos_sensor_8(0x555a, 0x08);
	write_cmos_sensor_8(0x555b, 0x08);
	write_cmos_sensor_8(0x5560, 0x80);
	write_cmos_sensor_8(0x5561, 0x80);
	write_cmos_sensor_8(0x5562, 0x80);
	write_cmos_sensor_8(0x5563, 0x80);
	write_cmos_sensor_8(0x5568, 0x80);
	write_cmos_sensor_8(0x5569, 0x80);
	write_cmos_sensor_8(0x556a, 0x80);
	write_cmos_sensor_8(0x556b, 0x80);
	write_cmos_sensor_8(0x5570, 0x08);
	write_cmos_sensor_8(0x5571, 0x08);
	write_cmos_sensor_8(0x5572, 0x08);
	write_cmos_sensor_8(0x5573, 0x08);
	write_cmos_sensor_8(0x5578, 0x08);
	write_cmos_sensor_8(0x5579, 0x08);
	write_cmos_sensor_8(0x557a, 0x08);
	write_cmos_sensor_8(0x557b, 0x08);
	write_cmos_sensor_8(0x5580, 0x80);
	write_cmos_sensor_8(0x5581, 0x80);
	write_cmos_sensor_8(0x5582, 0x80);
	write_cmos_sensor_8(0x5583, 0x80);
	write_cmos_sensor_8(0x5588, 0x80);
	write_cmos_sensor_8(0x5589, 0x80);
	write_cmos_sensor_8(0x558a, 0x80);
	write_cmos_sensor_8(0x558b, 0x80);
	write_cmos_sensor_8(0x5590, 0x08);
	write_cmos_sensor_8(0x5591, 0x08);
	write_cmos_sensor_8(0x5592, 0x08);
	write_cmos_sensor_8(0x5593, 0x08);
	write_cmos_sensor_8(0x5598, 0x08);
	write_cmos_sensor_8(0x5599, 0x08);
	write_cmos_sensor_8(0x559a, 0x08);
	write_cmos_sensor_8(0x559b, 0x08);
	write_cmos_sensor_8(0x55a0, 0x80);
	write_cmos_sensor_8(0x55a1, 0x80);
	write_cmos_sensor_8(0x55a2, 0x80);
	write_cmos_sensor_8(0x55a3, 0x80);
	write_cmos_sensor_8(0x55a8, 0x80);
	write_cmos_sensor_8(0x55a9, 0x80);
	write_cmos_sensor_8(0x55aa, 0x80);
	write_cmos_sensor_8(0x55ab, 0x80);
	write_cmos_sensor_8(0x55b0, 0x08);
	write_cmos_sensor_8(0x55b1, 0x08);
	write_cmos_sensor_8(0x55b2, 0x08);
	write_cmos_sensor_8(0x55b3, 0x08);
	write_cmos_sensor_8(0x55b8, 0x08);
	write_cmos_sensor_8(0x55b9, 0x08);
	write_cmos_sensor_8(0x55ba, 0x08);
	write_cmos_sensor_8(0x55bb, 0x08);
	write_cmos_sensor_8(0x55c0, 0x80);
	write_cmos_sensor_8(0x55c1, 0x80);
	write_cmos_sensor_8(0x55c2, 0x80);
	write_cmos_sensor_8(0x55c3, 0x80);
	write_cmos_sensor_8(0x55c8, 0x80);
	write_cmos_sensor_8(0x55c9, 0x80);
	write_cmos_sensor_8(0x55ca, 0x80);
	write_cmos_sensor_8(0x55cb, 0x80);
	write_cmos_sensor_8(0x55d0, 0x08);
	write_cmos_sensor_8(0x55d1, 0x08);
	write_cmos_sensor_8(0x55d2, 0x08);
	write_cmos_sensor_8(0x55d3, 0x08);
	write_cmos_sensor_8(0x55d8, 0x08);
	write_cmos_sensor_8(0x55d9, 0x08);
	write_cmos_sensor_8(0x55da, 0x08);
	write_cmos_sensor_8(0x55db, 0x08);
	write_cmos_sensor_8(0x55e0, 0x80);
	write_cmos_sensor_8(0x55e1, 0x80);
	write_cmos_sensor_8(0x55e2, 0x80);
	write_cmos_sensor_8(0x55e3, 0x80);
	write_cmos_sensor_8(0x55e8, 0x80);
	write_cmos_sensor_8(0x55e9, 0x80);
	write_cmos_sensor_8(0x55ea, 0x80);
	write_cmos_sensor_8(0x55eb, 0x80);
	write_cmos_sensor_8(0x55f0, 0x08);
	write_cmos_sensor_8(0x55f1, 0x08);
	write_cmos_sensor_8(0x55f2, 0x08);
	write_cmos_sensor_8(0x55f3, 0x08);
	write_cmos_sensor_8(0x55f8, 0x08);
	write_cmos_sensor_8(0x55f9, 0x08);
	write_cmos_sensor_8(0x55fa, 0x08);
	write_cmos_sensor_8(0x55fb, 0x08);
	write_cmos_sensor_8(0x5690, 0x02);
	write_cmos_sensor_8(0x5691, 0x10);
	write_cmos_sensor_8(0x5692, 0x02);
	write_cmos_sensor_8(0x5693, 0x08);
	write_cmos_sensor_8(0x5d24, 0x02);
	write_cmos_sensor_8(0x5d25, 0x10);
	write_cmos_sensor_8(0x5d26, 0x02);
	write_cmos_sensor_8(0x5d27, 0x08);
	write_cmos_sensor_8(0x5d3a, 0x58);
#ifdef sensor_flip
	#ifdef sensor_mirror
	write_cmos_sensor_8(0x3820, 0xc5);
	write_cmos_sensor_8(0x3821, 0x06);
	write_cmos_sensor_8(0x4000, 0x57);
	#else
	write_cmos_sensor_8(0x3820, 0xc5);
	write_cmos_sensor_8(0x3821, 0x02);
	write_cmos_sensor_8(0x4000, 0x57);
	#endif	
#else
	#ifdef sensor_mirror
	write_cmos_sensor_8(0x3820, 0x81);
	write_cmos_sensor_8(0x3821, 0x06);
	write_cmos_sensor_8(0x4000, 0x17);
	#else
	write_cmos_sensor_8(0x3820, 0x81);
	write_cmos_sensor_8(0x3821, 0x02);
	write_cmos_sensor_8(0x4000, 0x17);
	#endif	
#endif	
	//align_old_setting
	write_cmos_sensor_8(0x5000, 0x9b);
	write_cmos_sensor_8(0x5002, 0x1c);
	write_cmos_sensor_8(0x5b85, 0x10);
	write_cmos_sensor_8(0x5b87, 0x20);
	write_cmos_sensor_8(0x5b89, 0x40);
	write_cmos_sensor_8(0x5b8b, 0x80);
	write_cmos_sensor_8(0x5b8f, 0x18);
	write_cmos_sensor_8(0x5b90, 0x18);
	write_cmos_sensor_8(0x5b91, 0x18);
	write_cmos_sensor_8(0x5b95, 0x18);
	write_cmos_sensor_8(0x5b96, 0x18);
	write_cmos_sensor_8(0x5b97, 0x18);
	//Disable PD
	write_cmos_sensor_8(0x5001, 0x4a);
	write_cmos_sensor_8(0x3018, 0x5a);
	write_cmos_sensor_8(0x366c, 0x00);
	write_cmos_sensor_8(0x3641, 0x00);
	write_cmos_sensor_8(0x5d29, 0x00);
	//add full_15_difference (default)
	write_cmos_sensor_8(0x030a, 0x00);
	write_cmos_sensor_8(0x0311, 0x00);
	write_cmos_sensor_8(0x3606, 0x55);
	write_cmos_sensor_8(0x3607, 0x05);
	write_cmos_sensor_8(0x360b, 0x48);
	write_cmos_sensor_8(0x360c, 0x18);
	write_cmos_sensor_8(0x3720, 0x88);
	write_cmos_sensor_8(0x0100, 0x01);

	mDELAY(40);
	
}

static void slim_video_setting(void)
{
	LOG_INF("E\n");

  preview_setting();
}



/*************************************************************************
* FUNCTION
*	get_imgsensor_id
*
* DESCRIPTION
*	This function get the sensor ID
*
* PARAMETERS
*	*sensorID : return the sensor ID
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {	
			*sensor_id = (read_cmos_sensor_8(0x300A) << 16) | (read_cmos_sensor_8(0x300B)<<8) | read_cmos_sensor_8(0x300C);
			LOG_INF("Read sensor id fail, write id:0x%x ,sensor Id:0x%x\n", imgsensor.i2c_write_id, *sensor_id);//lx_revised
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);	  
				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail, write id:0x%x ,sensor Id:0x%x\n", imgsensor.i2c_write_id,*sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		// if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF 
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
	//const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;
	LOG_1;
	LOG_2;
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = (read_cmos_sensor_8(0x300A) << 16) | (read_cmos_sensor_8(0x300B)<<8) | read_cmos_sensor_8(0x300C);
			LOG_INF("Read sensor id fail, write id:0x%x ,sensor Id:0x%x\n", imgsensor.i2c_write_id, sensor_id);//lx_revised
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, write id:0x%x ,sensor Id:0x%x\n", imgsensor.i2c_write_id,sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

	/* initail sequence write in  */
	sensor_init();

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = KAL_FALSE;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}	/*	open  */



/*************************************************************************
* FUNCTION
*	close
*
* DESCRIPTION
*
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/

	return ERROR_NONE;
}	/*	close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
	//set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}	/*	preview   */

/*************************************************************************
* FUNCTION
*	capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;

    if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) // 30fps
    {
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else if(imgsensor.current_fps == 240)//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
    {
		if (imgsensor.current_fps != imgsensor_info.cap1.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap1.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else //PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
    {
		if (imgsensor.current_fps != imgsensor_info.cap2.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap1.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}

	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("Caputre fps:%d\n",imgsensor.current_fps);
	capture_setting(imgsensor.current_fps);
    //set_mirror_flip(IMAGE_NORMAL);

	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	//set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	//set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}	/*	hs_video   */


static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	//set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}	/*	slim_video	 */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;

	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d", scenario_id);


	//sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; /* not use */
	//sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; /* not use */
	//imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate; /* not use */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 2; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode*/
	sensor_info->HDR_Support = 0; /*0: NO HDR, 1: iHDR, 2:mvHDR, 3:zHDR*/
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;	// 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

			sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

			break;
		default:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			capture(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			hs_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			slim_video(image_window, sensor_config_data);
			break;
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps,1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d ", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)
		imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			if(framerate==300)
			{
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
			else
			{
			frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*framerate = imgsensor_info.normal_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*framerate = imgsensor_info.cap.max_framerate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*framerate = imgsensor_info.hs_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*framerate = imgsensor_info.slim_video.max_framerate;
			break;
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor_8(0x5280, 0x80);
		write_cmos_sensor_8(0x5000, 0x00);
	} else {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor_8(0x5280, 0x00);
		write_cmos_sensor_8(0x5000, 0x08);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
							 UINT8 *feature_para,UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16=(UINT16 *) feature_para;
	UINT16 *feature_data_16=(UINT16 *) feature_para;
	UINT32 *feature_return_para_32=(UINT32 *) feature_para;
	UINT32 *feature_data_32=(UINT32 *) feature_para;
	unsigned long long *feature_data=(unsigned long long *) feature_para;

	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	SET_PD_BLOCK_INFO_T *PDAFinfo;
	SENSOR_VC_INFO_STRUCT *pvcinfo; 
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("feature_id = %d", feature_id);
	switch (feature_id) 
	{
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk,imgsensor.current_fps);
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			set_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			break;
		case SENSOR_FEATURE_SET_GAIN:
			set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			if((sensor_reg_data->RegData>>8)>0)
			write_cmos_sensor_8(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			else
			write_cmos_sensor_8(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
		// if EEPROM does not exist in camera module.
			*feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
			set_video_mode(*feature_data);
			break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			get_imgsensor_id(feature_return_para_32);
			break;
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_GET_PDAF_DATA:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
			read_eeprom((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_FRAMERATE:
			LOG_INF("current fps :%d\n", (UINT32)*feature_data);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.current_fps = *feature_data;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_SET_HDR:
			//LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data_16);
			LOG_INF("Warning! Not Support IHDR Feature");
			spin_lock(&imgsensor_drv_lock);
			//imgsensor.ihdr_en = (BOOL)*feature_data_16;
			imgsensor.ihdr_en = KAL_FALSE;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:
			LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data);
			wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data_32) 
			{
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
				break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
				break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
				break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
				break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
				break;
			}
			break;
		/*PDAF*/ 
		case SENSOR_FEATURE_GET_VC_INFO:
				LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
				pvcinfo = (SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
				switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[1],sizeof(SENSOR_VC_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[2],sizeof(SENSOR_VC_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[0],sizeof(SENSOR_VC_INFO_STRUCT));
					break;
				}
				break;
		case SENSOR_FEATURE_GET_PDAF_INFO:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n", (UINT32)*feature_data);
			PDAFinfo= (SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data) 
			{
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(SET_PD_BLOCK_INFO_T));
				break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
				break;
			}
			break;
		case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
			LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n", *feature_data);
			//PDAF capacity enable or not, OV16880 only full size support PDAF
			switch (*feature_data) 
			{
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
				break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0; // video & capture use same setting
				break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
				break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
				break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
				break;
				default:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
				break;
			}
			break;
		case SENSOR_FEATURE_SET_PDAF:
			LOG_INF("PDAF mode :%d\n", *feature_data_16);
			imgsensor.pdaf_mode= *feature_data_16;
			break;
		/*End of PDAF*/
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
			LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
			ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
			break;
		default:
			break;
	}

	return ERROR_NONE;
}	/*	feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 OV16880MIPISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	OV16880_MIPI_RAW_SensorInit	*/
