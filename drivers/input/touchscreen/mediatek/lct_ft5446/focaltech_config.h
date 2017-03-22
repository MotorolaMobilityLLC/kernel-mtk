/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2016, FocalTech Systems, Ltd., all rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/************************************************************************
*
* File Name: focaltech_config.h
*
*    Author: Focaltech Driver Team
*
*   Created: 2016-08-08
*
*  Abstract: global configurations
*
*   Version: v1.0
*
************************************************************************/
#ifndef _LINUX_FOCLATECH_CONFIG_H_
#define _LINUX_FOCLATECH_CONFIG_H_

/**************************************************/
/****** G: A, I: B, S: C, U: D  ******************/
/****** chip type defines, do not modify *********/
#define _FT8716     0x8716
#define _FT8607     0x8607
#define _FT8006     0x8006
#define _FT5436     0x5436
#define _FT5446     0x5446
#define _FT5346     0x5346
#define _FT5446I    0x5446B
#define _FT5346I    0x5346B
#define _FT3327     0x3327
#define _FT3427     0x3427
#define _FT6236U    0x6236D
#define _FT6336G    0x6336A
#define _FT6336U    0x6336D
#define _FT6436U    0x6436D
#define _FT5826S    0x5836C
#define _FT5626     0x5626
#define _FT5726     0x5726

/*************************************************/

/*
 * choose your ic chip type of focaltech
 */
#define FTS_CHIP_TYPE   _FT5446

/******************* Enables *********************/
/*********** 1 to enable, 0 to disable ***********/

/*
 * show debug log info
 * enable it for debug, disable it for release
 */
#define FTS_DEBUG_EN                            1

/*
 * Linux MultiTouch Protocol
 * 1: Protocol B(default), 0: Protocol A
 */
#define FTS_MT_PROTOCOL_B_EN                    1

/*
 * Report Pressure in multitouch
 * 1:enable(default),0:disable
*/
#define FTS_REPORT_PRESSURE_EN                  1

/*
 * Force touch support
 * different pressure for multitouch
 * 1: true pressure for force touch
 * 0: constant pressure(default)
 */
#define FTS_FORCE_TOUCH_EN                      0

/*
 * Gesture function enable
 * default: disable
 */
#define FTS_GESTURE_EN                          0

/*
 * ESD check & protection
 * default: disable
 */
#define FTS_ESDCHECK_EN                         0

/*
 * Production test enable
 * 1: enable, 0:disable(default)
 */
#define FTS_TEST_EN                             0

/*
 * Glove mode enable
 * 1: enable, 0:disable(default)
 */
#define FTS_GLOVE_EN                            0
/*
 * cover enable
 * 1: enable, 0:disable(default)
 */
#define FTS_COVER_EN                            0
/*
 * Charger enable
 * 1: enable, 0:disable(default)
 */
#define FTS_CHARGER_EN                          0

/*
 * Proximity sensor
 * default: disable
 */
#define FTS_PSENSOR_EN                          0

/*
 * Nodes for tools, please keep enable
 */
#define FTS_SYSFS_NODE_EN                       1
#define FTS_APK_NODE_EN                         1

/*
 * Customer power enable
 * enable it when customer need control TP power
 * default: disable
 */
#define FTS_POWER_SOURCE_CUST_EN                1

/****************************************************/

/*
 * max touch points number
 * default: 10
 */
#define FTS_MAX_POINTS                          10

/********************** Upgrade ****************************/
/*
 * auto upgrade, please keep enable
 */
#define FTS_AUTO_UPGRADE_EN                     1

/*
 * auto upgrade for lcd cfg
 * default: 0
 */
#define FTS_AUTO_UPGRADE_FOR_LCD_CFG_EN         0

/* auto cb check
 * default: disable
 */
#define FTS_AUTO_CLB_EN                         0

/*
 * FW_APP.i file for upgrade
 * define your own fw_app, the sample one is invalid
 */
#define FTS_UPGRADE_FW_APP                      "include/firmware/FT5446_LQ_L3600_V12_D01_20170321_app.i"

/*
 * auto upgrade with app.bin in sdcard
 */
#define FTS_UPGRADE_WITH_APP_BIN_EN             0

/*
 * app.bin files for upgrade with app bin file
 * this file must put in sdcard
 */
#define FTS_UPGRADE_FW_APP_BIN                  "app.bin"

/*
 * lcd_cfg.i file for lcd cfg upgrade
 * define your own lcd_cfg.i, the sample one is invalid
 */
//Note: Not used! Please Ignore it!
//#define FTS_UPGRADE_LCD_CFG                     "include/firmware/lcd_cfg.i"

/*
 * auto upgrade with app.bin in sdcard
 */
#define FTS_UPGRADE_LCD_CFG_BIN_EN              0

/*
 * lcd_cfg.bin files for upgrade with lcd cfg bin file
 * this file must put in sdcard
 */
#define FTS_UPGRADE_LCD_CFG_BIN                 "lcd_cfg.bin"

/*
 * vendor_id(s) for the ic
 * you need confirm vendor_id for upgrade
 * if only one vendor, ignore vendor_2_id, otherwise
 * you need define both of them
 */
#define FTS_VENDOR_1_ID                         0x59
#define FTS_VENDOR_2_ID                         0x59

/* show upgrade time in log
 * default: disable
 */
#define FTS_GET_UPGRADE_TIME                    0

/*
 * upgrade ping-pong test for debug
 * enable it for upgrade debug if needed
 * default: disable
 */
#define FTS_UPGRADE_PINGPONG_TEST               0
/* pingpong or error test times, default: 1000 */
#define FTS_UPGRADE_TEST_NUMBER                 1000

/*********************************************************/

#endif /* _LINUX_FOCLATECH_CONFIG_H_ */
