////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_common.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_DRV_COMMON_H__
#define __MSTAR_DRV_COMMON_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif //CONFIG_HAS_EARLYSUSPEND
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/time.h>
#include <linux/input/mt.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h> // TODO : For use Android 6.0 on MTK platform  20160812 ,open,zhangwenkang.wt
#include <asm/irq.h>
#include <asm/io.h>
#ifdef CONFIG_ARCH_MT6580
#include <pmic_drv.h>
#include <cust_i2c.h>
#endif
/*--------------------------------------------------------------------------*/
/* TOUCH DEVICE DRIVER RELEASE VERSION                                      */
/*--------------------------------------------------------------------------*/

#define DEVICE_DRIVER_RELEASE_VERSION   ("3.14.0.0")


/*--------------------------------------------------------------------------*/
/* COMPILE OPTION DEFINITION                                                */
/*--------------------------------------------------------------------------*/

/*
 * Note.
 * The below compile option is used to enable the specific device driver code handling for distinct smart phone developer platform.
 * For running on Spreadtrum platform, please define the compile option CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM.
 * For running on Qualcomm platform, please define the compile option CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM.
 * For running on MediaTek platform, please define the compile option CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM.
 */
//#define CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
//#define CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM
#define CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM


/*
 * Note.
 * The below compile option is used to enable code handling for specific MTK platform which use Android 6.0 upward.
 * This compile option is used for MTK platform only.
 * By default, this compile option is disabled.
 */
#define CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD/*20160811 .open,zhangwenkang*/


/*
 * Note.
 * The below compile option is used to enable the specific device driver code handling to make sure main board can supply power to touch ic for some specific BB chip of MTK(EX. MT6582)/SPRD(EX. SC7715)/QCOM(EX. MSM8610).
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_REGULATOR_POWER_ON

/*
 * Note.
 * The below compile option is used to enable touch pin control for specific SPRD/QCOM platform.
 * This compile option is used for specific SPRD/QCOM platform only.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_TOUCH_PIN_CONTROL

/*
 * Note.
 * The below compile option is used to distinguish different workqueue scheduling mechanism when firmware report finger touch to device driver by IRQ interrupt.
 * For MTK platform, there are two type of workqueue scheduling mechanism available.
 * Please refer to the related code which is enclosed by compile option CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM in mstar_drv_platform_porting_layer.c for more detail.
 * This compile option is used for MTK platform only.
 * By default, this compile option is disabled.
 */
//#define CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM

/*
 * Note.
 * The below compile option is used to apply DMA mode for read/write data between device driver and firmware by I2C.
 * The DMA mode is used to reslove I2C read/write 8 bytes limitation for specific MTK BB chip(EX. MT6589/MT6572/...)
 * This compile option is used for MTK platform only.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_DMA_IIC


/*
 * Note.
 * The below compile option is used to enable the specific device driver code handling when touch panel support virtual key(EX. Menu, Home, Back, Search).
 * If this compile option is not defined, the function for virtual key handling will be disabled.
 * By default, this compile option is enabled.
 */
#define CONFIG_TP_HAVE_KEY

/*
 * Note.
 * Since specific MTK BB chip report virtual key touch by using coordinate instead of key code, the below compile option is used to enable the code handling for reporting key with coordinate.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE

/*
 * Note.
 * The below flag is used to enable debug mode data log for firmware.
 * Please make sure firmware support debug mode data log firstly, then you can enable this flag.
 * By default, this flag is enabled.
 */
#define CONFIG_ENABLE_FIRMWARE_DATA_LOG (0)   // 1 : Enable, 0 : Disable

/*
 * Note.
 * The below compile option is used to enable segment read debug mode finger touch data for MSG26XXM/MSG28XX only.
 * Since I2C transaction length limitation for some specific MTK BB chip(EX. MT6589/MT6572/...) or QCOM BB chip, the debug mode finger touch data of MSG26XXM/MSG28XX can not be retrieved by one time I2C read operation.  
 * So we need to retrieve the complete finger touch data by segment read.
 * By default, this compile option is enabled.
 */
//#define CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

/*
 * Note.
 * The below compile option is used to enable gesture wakeup.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_GESTURE_WAKEUP

// ------------------- #ifdef CONFIG_ENABLE_GESTURE_WAKEUP ------------------- //
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

/*
 * Note.
 * The below compile option is used to enable device driver to support at most 64 types of gesture wakeup mode.
 * If the below compile option is not enabled, device driver can only support at most 16 types of gesture wakeup mode.
 * By the way, 64 types of gesture wakeup mode is ready for MSG22XX/MSG28XX only.
 * But, 64 types of gesture wakeup mode for MSG21XXA/MSG26XXM is not supported.
 * By default, this compile option is disabled.
 */
//#define CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

/*
 * Note.
 * The below compile option is used to enable gesture debug mode.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_GESTURE_DEBUG_MODE

/*
 * Note.
 * The below compile option is used to enable gesture information mode.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_GESTURE_INFORMATION_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP
// ------------------- #endif CONFIG_ENABLE_GESTURE_WAKEUP ------------------- //


/*
 * Note.
 * The below compile option shall be used when compile option CONFIG_ENABLE_ITO_MP_TEST or CONFIG_UPDATE_FIRMWARE_BY_SW_ID is enabled.
 * One or more than one the below compile option can be enabled based on the touch ic that customer project are used.
 * By default, the below compile option are all disabled.
 */
//#define CONFIG_ENABLE_CHIP_TYPE_MSG21XXA
#define CONFIG_ENABLE_CHIP_TYPE_MSG22XX
//#define CONFIG_ENABLE_CHIP_TYPE_MSG26XXM
//#define CONFIG_ENABLE_CHIP_TYPE_MSG28XX


/*
 * Note.
 * The below compile option is used to enable phone level MP test handling.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC
#define CONFIG_ENABLE_ITO_MP_TEST

// ------------------- #ifdef CONFIG_ENABLE_ITO_MP_TEST ------------------- //
#ifdef CONFIG_ENABLE_ITO_MP_TEST
#define WT_CTP_OPEN_SHORT_TEST

/*
 * Note.
 * The below compile option is used to enable the specific short test item of 2R triangle pattern for self-capacitive touch ic.
 * This compile option is used for MSG21XXA/MSG22XX only.
 * Please enable the compile option if the ITO pattern is 2R triangle pattern for MSG21XXA/MSG22XX.
 * Please disable the compile option if the ITO pattern is H(horizontal) triangle pattern for MSG21XXA/MSG22XX.
 * By default, this compile option is enabled.
 */
#define CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE

#endif //CONFIG_ENABLE_ITO_MP_TEST
// ------------------- #endif CONFIG_ENABLE_ITO_MP_TEST ------------------- //


/*
 * Note.
 * If this compile option is not defined, the SW ID mechanism for updating firmware will be disabled.
 * By default, this compile option is disabled.
 */
#define CONFIG_UPDATE_FIRMWARE_BY_SW_ID

// ------------------- #ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID ------------------- //
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*
 * Note.
 * If this compile option is defined, the update firmware bin file shall be stored in a two dimensional array format.
 * Else, the update firmware bin file shall be stored in an one dimensional array format.
 * Be careful, MSG22XX only support storing update firmware bin file in an one dimensional array format, it does not support two dimensional array format.
 * By default, this compile option is enabled.
 */
//#define CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY 

#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID
// ------------------- #endif CONFIG_UPDATE_FIRMWARE_BY_SW_ID ------------------- //


/*
 * Note.
 * The below compile option is used to enable hotknot.
 * By the way, hotknot is supported for MSG28XX only, MSG21XXA/MSG22XX/MSG26XXM is not supported.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_HOTKNOT


/*
 * Note.
 * The below compile option is used to enable proximity detection.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_PROXIMITY_DETECTION

/*
 * Note.
 * The below compile option is used to enable notifier feedback handling for SPRD/QCOM platform.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_NOTIFIER_FB


/*
 * Note.
 * The below compile option is used to enable report rate calculation.
 * By default, this compile option is enabled.
 */
#define CONFIG_ENABLE_COUNT_REPORT_RATE


/*
 * Note.
 * The below compile option is used to enable glove mode.
 * By the way, glove mode is supported for MSG28XX only, MSG21XXA/MSG22XX/MSG26XXM is not supported.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_GLOVE_MODE


/*
 * Note.
 * The below compile option is used to enable leather sheath mode.
 * By the way, leather sheath mode is supported for MSG28XX only, MSG21XXA/MSG22XX/MSG26XXM is not supported.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_LEATHER_SHEATH_MODE


/*
 * Note.
 * The below compile option is used to enable jni interface.
 * By default, this compile option is enabled.
 */
#define CONFIG_ENABLE_JNI_INTERFACE


/*
 * Note.
 * The below compile option is used to enable charger detection for notifying the charger plug in/plug out status to touch firmware.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_CHARGER_DETECTION


/*
 * Note.
 * The below compile option is used to enable ESD protection.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_ESD_PROTECTION

// ------------------- #ifdef CONFIG_ENABLE_ESD_PROTECTION ------------------- //
#ifdef CONFIG_ENABLE_ESD_PROTECTION

/*
 * Note.
 * There are two ESD protection check method.
 * Method 1. Require the new ESD check command(CmdId:0x55) support from firmware which is currently implemented for MSG22XX only. So default is not supported.
 * Method 2. Use read finger touch data for checking whether I2C connection is still available under ESD testing.
 * By default, this compile option is disabled. It means use Method 2 as default ESD check method.
 */
//#define CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE

#endif //CONFIG_ENABLE_ESD_PROTECTION
// ------------------- #endif CONFIG_ENABLE_ESD_PROTECTION ------------------- //


/*
 * Note.
 * The below compile option is used to enable the debug code for clarifying some issues. For example, to debug the delay time issue for IC hardware reset.
 * By the way, this feature is supported for MSG28XX only.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_CODE_FOR_DEBUG


/*
 * Note.
 * The below compile option is used to enable/disable Type A/Type B multi-touch protocol for reporting touch point/key to Linux input sub-system.
 * If this compile option is defined, Type B protocol is enabled.
 * Else, Type A protocol is enabled.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_TYPE_B_PROTOCOL


/*
 * Note.
 * The below compile option is used to enable force touch for reporting touch point to Linux input sub-system.
 * By the way, force touch is supported for MSG28XX only, but is not supported for MSG26XXM/MSG21XXA/MSG22XX yet.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_FORCE_TOUCH


/*
 * Note. 
 * The below two compile option is used to enable update firmware with 8 byte or 32 byte each time for MSG28XX.
 * If the below two compile option is disabled, then update firmware with 128 byte each time for MSG28XX.
 * By default, the below two compile option is disabled.
 */
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME 
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME


/*
 * Note. 
 * The below compile option is used to enable high speed isp mode for update firmware of MSG28XX.
 * By default, the below compile option is disabled.
 */
//#define CONFIG_ENABLE_HIGH_SPEED_ISP_MODE


/*
 * Note. 
 * The below compile option is used to enable update firmware with I2C data rate 400KHz for MSG22XX.
 * If this compile option is disabled, then update firmware with I2C data rate less than 400KHz for MSG22XX.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K 

// ------------------- #ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K ------------------- //
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

/*
 * Note.
 * There are three methods to update firmware for MSG22XX(with chip revision >= 0x04) when I2C data rate is 400KHz.
 * Method A. Enable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
 * Method B. Enable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
 * Method C. Enable I2C 400KHz non-burst write mode, only one byte can be written each time.
 * By default, the compile option CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A is enabled.
 */
#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_C

#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K
// ------------------- #endif CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K ------------------- //


/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

#define u8   unsigned char
#define u16  unsigned short
#define u32  uint32_t
#define s8   signed char
#define s16  signed short
#define s32  signed int
#define s64  int64_t
#define u64  uint64_t

// Chip Id
#define CHIP_TYPE_MSG21XX   (0x01) // EX. MSG2133
#define CHIP_TYPE_MSG21XXA  (0x02) // EX. MSG2133A/MSG2138A(Besides, use version to distinguish MSG2133A/MSG2138A, you may refer to _DrvFwCtrlUpdateFirmwareCash()) 
#define CHIP_TYPE_MSG26XXM  (0x03) // EX. MSG2633M
#define CHIP_TYPE_MSG22XX   (0x7A) // EX. MSG2238/MSG2256
#define CHIP_TYPE_MSG28XX   (0x85) // EX. MSG2833/MSG2835/MSG2836/MSG2840/MSG2856/MSG5835/MSG5846

// Chip Revision
#define CHIP_TYPE_MSG22XX_REVISION_U05   (0x04) // U05


#define PACKET_TYPE_TOOTH_PATTERN   (0x20)
#define PACKET_TYPE_GESTURE_WAKEUP  (0x50)
#define PACKET_TYPE_GESTURE_DEBUG  		(0x51)
#define PACKET_TYPE_GESTURE_INFORMATION	(0x52)
#define PACKET_TYPE_ESD_CHECK_HW_RESET	(0x60)

#define TOUCH_SCREEN_X_MIN   (0)
#define TOUCH_SCREEN_Y_MIN   (0)
/*
 * Note.
 * Please change the below touch screen resolution according to the touch panel that you are using.
 */
#define TOUCH_SCREEN_X_MAX   (480)  //LCD_WIDTH
#define TOUCH_SCREEN_Y_MAX   (854) //LCD_HEIGHT
/*
 * Note.
 * Please do not change the below setting.
 */
#define TPD_WIDTH   (2048)
#define TPD_HEIGHT  (2048)

#ifdef CONFIG_ARCH_MT6580
#define TPD_I2C_NUMBER		    I2C_CAP_TOUCH_CHANNEL
#define TPD_POWER_SOURCE_CUSTOM	PMIC_APP_CAP_TOUCH_VDD
#define CUST_EINT_TOUCH_PANEL_TYPE CUST_EINT_POLARITY_LOW
#endif

#define BIT0  (1<<0)  // 0x0001
#define BIT1  (1<<1)  // 0x0002
#define BIT2  (1<<2)  // 0x0004
#define BIT3  (1<<3)  // 0x0008
#define BIT4  (1<<4)  // 0x0010
#define BIT5  (1<<5)  // 0x0020
#define BIT6  (1<<6)  // 0x0040
#define BIT7  (1<<7)  // 0x0080
#define BIT8  (1<<8)  // 0x0100
#define BIT9  (1<<9)  // 0x0200
#define BIT10 (1<<10) // 0x0400
#define BIT11 (1<<11) // 0x0800
#define BIT12 (1<<12) // 0x1000
#define BIT13 (1<<13) // 0x2000
#define BIT14 (1<<14) // 0x4000
#define BIT15 (1<<15) // 0x8000


#define MAX_DEBUG_REGISTER_NUM     (10)
#define MAX_DEBUG_COMMAND_ARGUMENT_NUM      (4)

#define MAX_UPDATE_FIRMWARE_BUFFER_SIZE    (130) // 130KB. The size shall be large enough for stored any kind firmware size of MSG21XXA(33KB)/MSG22XX(48.5KB)/MSG26XXM(40KB)/MSG28XX(130KB).

#define MAX_I2C_TRANSACTION_LENGTH_LIMIT      (250) // Please change the value depends on the I2C transaction limitation for the platform that you are using.
#define MAX_TOUCH_IC_REGISTER_BANK_SIZE       (256) // It is a fixed value and shall not be modified.


#define PROCFS_AUTHORITY (0666)


#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#define GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG     0x00000001    //0000 0000 0000 0000   0000 0000 0000 0001
#define GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG        0x00000002    //0000 0000 0000 0000   0000 0000 0000 0010
#define GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG      0x00000004    //0000 0000 0000 0000   0000 0000 0000 0100
#define GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG      0x00000008    //0000 0000 0000 0000   0000 0000 0000 1000
#define GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG     0x00000010    //0000 0000 0000 0000   0000 0000 0001 0000
#define GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG      0x00000020    //0000 0000 0000 0000   0000 0000 0010 0000
#define GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG      0x00000040    //0000 0000 0000 0000   0000 0000 0100 0000
#define GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG      0x00000080    //0000 0000 0000 0000   0000 0000 1000 0000
#define GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG      0x00000100    //0000 0000 0000 0000   0000 0001 0000 0000
#define GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG      0x00000200    //0000 0000 0000 0000   0000 0010 0000 0000
#define GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG      0x00000400    //0000 0000 0000 0000   0000 0100 0000 0000
#define GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG      0x00000800    //0000 0000 0000 0000   0000 1000 0000 0000
#define GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG      0x00001000    //0000 0000 0000 0000   0001 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE1_FLAG         0x00002000    //0000 0000 0000 0000   0010 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE2_FLAG         0x00004000    //0000 0000 0000 0000   0100 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE3_FLAG         0x00008000    //0000 0000 0000 0000   1000 0000 0000 0000

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
#define GESTURE_WAKEUP_MODE_RESERVE4_FLAG         0x00010000    //0000 0000 0000 0001   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE5_FLAG         0x00020000    //0000 0000 0000 0010   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE6_FLAG         0x00040000    //0000 0000 0000 0100   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE7_FLAG         0x00080000    //0000 0000 0000 1000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE8_FLAG         0x00100000    //0000 0000 0001 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE9_FLAG         0x00200000    //0000 0000 0010 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE10_FLAG        0x00400000    //0000 0000 0100 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE11_FLAG        0x00800000    //0000 0000 1000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE12_FLAG        0x01000000    //0000 0001 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE13_FLAG        0x02000000    //0000 0010 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE14_FLAG        0x04000000    //0000 0100 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE15_FLAG        0x08000000    //0000 1000 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE16_FLAG        0x10000000    //0001 0000 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE17_FLAG        0x20000000    //0010 0000 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE18_FLAG        0x40000000    //0100 0000 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE19_FLAG        0x80000000    //1000 0000 0000 0000   0000 0000 0000 0000

#define GESTURE_WAKEUP_MODE_RESERVE20_FLAG        0x00000001    //0000 0000 0000 0000   0000 0000 0000 0001
#define GESTURE_WAKEUP_MODE_RESERVE21_FLAG        0x00000002    //0000 0000 0000 0000   0000 0000 0000 0010
#define GESTURE_WAKEUP_MODE_RESERVE22_FLAG        0x00000004    //0000 0000 0000 0000   0000 0000 0000 0100
#define GESTURE_WAKEUP_MODE_RESERVE23_FLAG        0x00000008    //0000 0000 0000 0000   0000 0000 0000 1000
#define GESTURE_WAKEUP_MODE_RESERVE24_FLAG        0x00000010    //0000 0000 0000 0000   0000 0000 0001 0000
#define GESTURE_WAKEUP_MODE_RESERVE25_FLAG        0x00000020    //0000 0000 0000 0000   0000 0000 0010 0000
#define GESTURE_WAKEUP_MODE_RESERVE26_FLAG        0x00000040    //0000 0000 0000 0000   0000 0000 0100 0000
#define GESTURE_WAKEUP_MODE_RESERVE27_FLAG        0x00000080    //0000 0000 0000 0000   0000 0000 1000 0000
#define GESTURE_WAKEUP_MODE_RESERVE28_FLAG        0x00000100    //0000 0000 0000 0000   0000 0001 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE29_FLAG        0x00000200    //0000 0000 0000 0000   0000 0010 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE30_FLAG        0x00000400    //0000 0000 0000 0000   0000 0100 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE31_FLAG        0x00000800    //0000 0000 0000 0000   0000 1000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE32_FLAG        0x00001000    //0000 0000 0000 0000   0001 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE33_FLAG        0x00002000    //0000 0000 0000 0000   0010 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE34_FLAG        0x00004000    //0000 0000 0000 0000   0100 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE35_FLAG        0x00008000    //0000 0000 0000 0000   1000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE36_FLAG        0x00010000    //0000 0000 0000 0001   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE37_FLAG        0x00020000    //0000 0000 0000 0010   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE38_FLAG        0x00040000    //0000 0000 0000 0100   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE39_FLAG        0x00080000    //0000 0000 0000 1000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE40_FLAG        0x00100000    //0000 0000 0001 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE41_FLAG        0x00200000    //0000 0000 0010 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE42_FLAG        0x00400000    //0000 0000 0100 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE43_FLAG        0x00800000    //0000 0000 1000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE44_FLAG        0x01000000    //0000 0001 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE45_FLAG        0x02000000    //0000 0010 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE46_FLAG        0x04000000    //0000 0100 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE47_FLAG        0x08000000    //0000 1000 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE48_FLAG        0x10000000    //0001 0000 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE49_FLAG        0x20000000    //0010 0000 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE50_FLAG        0x40000000    //0100 0000 0000 0000   0000 0000 0000 0000
#define GESTURE_WAKEUP_MODE_RESERVE51_FLAG        0x80000000    //1000 0000 0000 0000   0000 0000 0000 0000
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#define  KEY_GESTURE_CLICK	KEY_A
#define  KEY_GESTURE_UP		KEY_UP
#define  KEY_GESTURE_DOWN		KEY_DOWN
#define  KEY_GESTURE_LEFT		KEY_LEFT 
#define  KEY_GESTURE_RIGHT		KEY_RIGHT
#define  KEY_GESTURE_O		KEY_O
#define  KEY_GESTURE_E		KEY_E
#define  KEY_GESTURE_M		KEY_M 
#define  KEY_GESTURE_L		KEY_L
#define  KEY_GESTURE_W		KEY_W
#define  KEY_GESTURE_S		KEY_S 
#define  KEY_GESTURE_V		KEY_V
#define  KEY_GESTURE_Z		KEY_Z
#define  KEY_GESTURE_C		KEY_C
#endif

#define GESTURE_WAKEUP_PACKET_LENGTH    (6)

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
#define GESTURE_DEBUG_MODE_PACKET_LENGTH	(128)
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#define GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH	(128)
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#define FEATURE_GESTURE_WAKEUP_MODE         0x0001   
#define FEATURE_GESTURE_DEBUG_MODE          0x0002
#define FEATURE_GESTURE_INFORMATION_MODE    0x0003

#define FEATURE_TOUCH_DRIVER_DEBUG_LOG      0x0010
#define FEATURE_FIRMWARE_DATA_LOG           0x0011
#define FEATURE_FORCE_TO_UPDATE_FIRMWARE    0x0012


#define I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE   (20) // delay 20ms

#define I2C_SMBUS_WRITE_COMMAND_DELAY_FOR_QCOM_PLATFORM   (5) // delay 5ms

#define I2C_SMBUS_READ_COMMAND_DELAY_FOR_SPRD_PLATFORM   (5) // delay 5ms

#define FIRMWARE_FILE_PATH_ON_SD_CARD      "/mnt/sdcard/msctp_update.bin"

#define POWER_SUPPLY_BATTERY_STATUS_PATCH  "/sys/class/power_supply/battery/status"

#define ESD_PROTECT_CHECK_PERIOD   (300) // delay 3s
#define ESD_CHECK_HW_RESET_PACKET_LENGTH    (8)

/*
 * Note.
 * The below flag is used to enable the output log mechanism while touch device driver is running.
 * If the debug log level is set as 0, the function for output log will be disabled.
 * By default, the debug log level is set as 1.
 */
#define CONFIG_TOUCH_DRIVER_DEBUG_LOG_LEVEL (0)   // 1 : Default, 0 : No log. The bigger value, the more detailed log is output.

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

extern u8 TOUCH_DRIVER_DEBUG_LOG_LEVEL;

/*--------------------------------------------------------------------------*/
/* PREPROCESSOR MACRO DEFINITION                                            */
/*--------------------------------------------------------------------------*/


#define DEBUG_LEVEL(level, dev, fmt,arg...) do {\
	                                          /* if (level <= TOUCH_DRIVER_DEBUG_LOG_LEVEL)*/\
	                                         /*printk(fmt , ##arg); */\
                                      } while (0)

#define DBG(dev, fmt, arg...) DEBUG_LEVEL(1, dev, fmt, ##arg) 
/*
#define DEBUG_LEVEL(level, dev, fmt, arg...) do {\
	                                           if (level <= TOUCH_DRIVER_DEBUG_LOG_LEVEL)\
	                                               dev_info(dev, fmt, ##arg);\
                                        } while (0)

#define DBG(dev, fmt, arg...) DEBUG_LEVEL(1, dev, fmt, ##arg)
*/

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef enum
{
    EMEM_ALL = 0,
    EMEM_MAIN,
    EMEM_INFO
} EmemType_e;

typedef enum
{
    ITO_TEST_MODE_OPEN_TEST = 1,
    ITO_TEST_MODE_SHORT_TEST = 2,
    ITO_TEST_MODE_WATERPROOF_TEST = 3
} ItoTestMode_e;

typedef enum
{
    ITO_TEST_LOG_TYPE_GOLDEN_CHANNEL = 1,
    ITO_TEST_LOG_TYPE_RESULT = 2,
    ITO_TEST_LOG_TYPE_RATIO = 3
} ItoTestLogType_e;

typedef enum
{
    ITO_TEST_OK = 0,
    ITO_TEST_FAIL,
    ITO_TEST_GET_TP_TYPE_ERROR,
    ITO_TEST_UNDEFINED_ERROR,
    ITO_TEST_UNDER_TESTING

} ItoTestResult_e;

typedef enum
{
    ADDRESS_MODE_8BIT = 0,
    ADDRESS_MODE_16BIT = 1
} AddressMode_e;

typedef enum
{
    MUTUAL_MODE = 0,
    MUTUAL_SINE,
    SELF,
    WATERPROOF,
    MUTUAL_SINGLE_DRIVE
} ItoTestFwMode_e;

typedef enum
{
	_50p,
	_40p,
	_30p,
	_20p,
	_10p
} ItoTestCfbValue_e;

typedef enum
{
	DISABLE = 0,
	ENABLE,
} ItoTestChargePumpStatus_e;

typedef enum
{
	GND = 0x00,
    POS_PULSE = 0x01,
    NEG_PULSE = 0x02,
    HIGH_IMPEDENCE = 0x03,
} ItoTestSensorPADState_e;

/*--------------------------------------------------------------------------*/
/* GLOBAL VARIABLE DEFINITION                                               */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

extern u8 DrvCommonCalculateCheckSum(u8 *pMsg, u32 nLength);
extern u32 DrvCommonConvertCharToHexDigit(char *pCh, u32 nLength);
extern u32 DrvCommonCrcDoReflect(u32 nRef, s8 nCh);
extern u32 DrvCommonCrcGetValue(u32 nText, u32 nPrevCRC);
extern void DrvCommonCrcInitTable(void);
extern void DrvCommonReadFile(char *pFilePath, u8 *pBuf, u16 nLength);

#endif  /* __MSTAR_DRV_COMMON_H__ */
