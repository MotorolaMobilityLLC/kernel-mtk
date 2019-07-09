/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Johnson Yeh
 * Maintain: Luca Hsu, Tigers Huang, Dicky Chiang
 */

/**
 *
 * @file    ilitek_drv_common.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __ILITEK_DRV_COMMON_H__
#define __ILITEK_DRV_COMMON_H__

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
#include <linux/kobject.h>
#include <linux/version.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
//#include <linux/uaccess.h> // TODO : For use Android 6.0 on MTK platform
#include <asm/irq.h>
#include <asm/io.h>

/*--------------------------------------------------------------------------*/
/* TOUCH DEVICE DRIVER RELEASE VERSION                                      */
/*--------------------------------------------------------------------------*/

#define DEVICE_DRIVER_RELEASE_VERSION   ("8.0.1.0")

/*--------------------------------------------------------------------------*/
/* COMPILE OPTION DEFINITION                                                */
/*--------------------------------------------------------------------------*/

/*
 * Note.
 * The below compile option is used to enable the specific device driver code handling for distinct smart phone developer platform.
 * For running on Spreadtrum platform, please define the compile option CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM.
 * For running on Qualcomm platform, please define the compile option CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM.
 * For running on MediaTek platform, please define the compile option CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM.
 * For running on Rockchip platform , please define the compile option ROCKCHIP_PLATFORM (it should be defined with QCOM platform compile option)
 * For running on Allwinner platform , please define the compile option ALLWINNER_PLATFORM (it should be defined with QCOM platform compile option)
 * For running on Amlogic platform , please define the compile option AMLOGIC_PLATFORM (it should be defined with QCOM platform compile option)
 */
//#define CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM
//#define CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM
#define CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#define CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM
#define ROCKCHIP_PLATFORM
//#define ALLWINNER_PLATFORM
//#define AMLOGIC_PLATFORM
#endif
/*
 * Note.
 * The below compile option is used to enable code handling for specific MTK platform which use Android 6.0 upward.
 * This compile option is used for MTK platform only.
 * By default, this compile option is disabled.
 */
//#define CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD


/*
 * Note.
 * One or more than one the below compile option can be enabled based on the touch ic that customer project are used.
 * By default, the below compile option are all disabled.
 */
//#define CONFIG_ENABLE_CHIP_TYPE_MSG22XX
//#define CONFIG_ENABLE_CHIP_TYPE_MSG28XX       // This compile option can be used for MSG28XX/MSG58XX/MSG58XXA/ILI2117A/ILI2118A


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
#define CONFIG_ENABLE_TOUCH_PIN_CONTROL

/*
 * Note.
 * The below compile option is used to distinguish different workqueue scheduling mechanism when firmware report finger touch to device driver by IRQ interrupt.
 * For MTK platform, there are two type of workqueue scheduling mechanism available.
 * Please refer to the related code which is enclosed by compile option CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM in ilitek_drv_main.c for more detail.
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
//#define CONFIG_ENABLE_DMA_IIC


/*
 * Note.
 * The below compile option is used to enable the specific device driver code handling when touch panel support virtual key(EX. Menu, Home, Back, Search).
 * If this compile option is not defined, the function for virtual key handling will be disabled.
 * By default, this compile option is enabled.
 */
//#define CONFIG_TP_HAVE_KEY

/*
 * Note.
 * Since specific MTK BB chip report virtual key touch by using coordinate instead of key code, the below compile option is used to enable the code handling for reporting key with coordinate.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE

/*
 * Note.
 * The below flag is used to enable debug mode data log for firmware.
 * Please make sure firmware support debug mode data log firstly, then you can enable this flag.
 * By default, this flag is enabled.
 */
#define CONFIG_ENABLE_FIRMWARE_DATA_LOG (1)   // 1 : Enable, 0 : Disable

/*
 * Note.
 * The below compile option is used to enable segment read debug mode finger touch data for MSG28XX/MSG58XX/MSG58XXA only.
 * Since I2C transaction length limitation for some specific MTK BB chip(EX. MT6589/MT6572/...) or QCOM BB chip, the debug mode finger touch data of MSG28XX/MSG58XX/MSG58XXA can not be retrieved by one time I2C read operation.  
 * So we need to retrieve the complete finger touch data by segment read.
 * By default, this compile option is enabled.
 */
#define CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

/*
 * Note.
 * The below flag is used to enable output specific debug value for firmware by MTPTool APK/TP Test Studio Tool.
 * Please make sure firmware support ENABLE_APK_PRINT_FW_LOG_DEBUG function firstly, then you can use this feature.
 * By default, this flag is enabled.
 */
#define CONFIG_ENABLE_APK_PRINT_FIRMWARE_SPECIFIC_LOG (1)   // 1 : Enable, 0 : Disable

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
 * By the way, 64 types of gesture wakeup mode is ready for MSG22XX/MSG28XX/MSG58XX/MSG58XXA only.
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
 * The below compile option is used to enable phone level MP test handling.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_ITO_MP_TEST

// ------------------- #ifdef CONFIG_ENABLE_ITO_MP_TEST ------------------- //
#ifdef CONFIG_ENABLE_ITO_MP_TEST

/*
 * Note.
 * The below compile option is used to enable the specific short test item of 2R triangle pattern for self-capacitive touch ic.
 * This compile option is used for MSG22XX only.
 * Please enable the compile option if the ITO pattern is 2R triangle pattern for MSG22XX.
 * Please disable the compile option if the ITO pattern is H(horizontal) triangle pattern for MSG22XX.
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
//#define CONFIG_UPDATE_FIRMWARE_BY_SW_ID

// ------------------- #ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID ------------------- //
#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID

/*
 * Note.
 * If this compile option is defined, the update firmware bin file shall be stored in a two dimensional array format.
 * Else, the update firmware bin file shall be stored in an one dimensional array format.
 * Be careful, MSG22XX only support storing update firmware bin file in an one dimensional array format, it does not support two dimensional array format.
 * By default, this compile option is enabled.
 */
#define CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY 

#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID
// ------------------- #endif CONFIG_UPDATE_FIRMWARE_BY_SW_ID ------------------- //

/*
 * Note.
 * The below compile option is used to enable proximity detection.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_PROXIMITY_DETECTION


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
//#define CONFIG_ENABLE_COUNT_REPORT_RATE


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
 * Method 2. Use I2C write command for checking whether I2C connection is still available under ESD testing.
 * By default, this compile option is disabled. It means use Method 2 as default ESD check method.
 */
//#define CONFIG_ENABLE_ESD_CHECK_COMMAND_BY_FIRMWARE

#endif //CONFIG_ENABLE_ESD_PROTECTION
// ------------------- #endif CONFIG_ENABLE_ESD_PROTECTION ------------------- //


/*
 * Note.
 * The below compile option is used to enable the debug code for clarifying some issues. For example, to debug the delay time issue for IC hardware reset.
 * By the way, this feature is supported for MSG28XX/MSG58XX/MSG58XXA only.
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
 * The below compile option is used to retrieve a change pressure value from TP firmware while report touch point to Linux input sub-system.
 * By the way, this compile option is used for ILI21XX only.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_REPORT_PRESSURE


/*
 * Note. 
 * The below two compile option is used to enable update firmware with 8 byte or 32 byte each time for MSG28XX/MSG58XX/MSG58XXA.
 * If the below two compile option is disabled, then update firmware with 128 byte each time for MSG28XX/MSG58XX/MSG58XXA.
 * By default, the below two compile option is disabled.
 */
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_8_BYTE_EACH_TIME 
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_32_BYTE_EACH_TIME


/*
 * Note. 
 * The below compile option is used to enable high speed isp mode for update firmware of MSG28XX/MSG58XX/MSG58XXA.
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


/* The below 3 define are used for MSG22xx/ILI21xx */
//#define CONFIG_SWAP_X_Y

//#define CONFIG_REVERSE_X
//#define CONFIG_REVERSE_Y


/*--------------------------------------------------------------------------*/
/* PREPROCESSOR CONSTANT DEFINITION                                         */
/*--------------------------------------------------------------------------*/

/*
 * Note.
 * Please change the below GPIO pin setting to follow the platform that you are using(EX. MediaTek, Spreadtrum, Qualcomm).
 */
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)

#include <mach/board.h>
#include <mach/gpio.h>

//#include <soc/sprd/board.h>
//#include <soc/sprd/gpio.h>
//#include <soc/sprd/i2c-sprd.h>

#include <linux/of_gpio.h>

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
#include <mach/regulator.h>
//#include <soc/sprd/regulator.h>
#include <linux/regulator/consumer.h>
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

#ifndef CONFIG_ENABLE_TOUCH_PIN_CONTROL
// TODO : Please FAE colleague to confirm with customer device driver engineer about the value of RST and INT GPIO setting
#define MS_TS_MSG_IC_GPIO_RST   GPIO_TOUCH_RESET //53 //35 
#define MS_TS_MSG_IC_GPIO_INT   GPIO_TOUCH_IRQ   //52 //37
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
#include <linux/pinctrl/consumer.h>

#define PINCTRL_STATE_ACTIVE	"pmx_ts_active"
#define PINCTRL_STATE_SUSPEND	"pmx_ts_suspend"
#define PINCTRL_STATE_RELEASE	"pmx_ts_release"
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
//#include <linux/input/vir_ps.h> 

#define GTP_ADDR_LENGTH       (2)
#define GTP_IOCTL_MAGIC 			(0x1C)
#define LTR_IOCTL_GET_PFLAG  	_IOR(GTP_IOCTL_MAGIC, 1, int)
#define LTR_IOCTL_GET_LFLAG  	_IOR(GTP_IOCTL_MAGIC, 2, int)
#define LTR_IOCTL_SET_PFLAG  	_IOW(GTP_IOCTL_MAGIC, 3, int)
#define LTR_IOCTL_SET_LFLAG  	_IOW(GTP_IOCTL_MAGIC, 4, int)
#define LTR_IOCTL_GET_DATA  	_IOW(GTP_IOCTL_MAGIC, 5, unsigned char)
#define GTP_IOCTL_PROX_ON 		_IO(GTP_IOCTL_MAGIC, 7)
#define GTP_IOCTL_PROX_OFF		_IO(GTP_IOCTL_MAGIC, 8)
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_TP_HAVE_KEY
#define TOUCH_KEY_MENU (139) //229
#define TOUCH_KEY_HOME (172) //102
#define TOUCH_KEY_BACK (158)
#define TOUCH_KEY_SEARCH (217)

#define MAX_KEY_NUM (4)
#endif //CONFIG_TP_HAVE_KEY

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)

#ifdef ALLWINNER_PLATFORM

#include <linux/irq.h>
#include <linux/init-input.h>
#include <linux/pm.h>
//#include <mach/sys_config.h>
#include <linux/gpio.h>
#include <linux/pm_runtime.h>
//#include <linux/earlysuspend.h>

extern struct ctp_config_info config_info;

#define CTP_IRQ_MODE			(IRQF_TRIGGER_FALLING)
#define CTP_IRQ_NUMBER          (config_info.int_number)

#elif defined(AMLOGIC_PLATFORM)

#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/amlogic/sd.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/amlogic/gpio-amlogic.h>
#include <linux/of_irq.h>

#endif 
#include <linux/of_gpio.h>

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
#include <linux/regulator/consumer.h>
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

#ifdef CONFIG_ENABLE_NOTIFIER_FB
#include <linux/notifier.h>
#include <linux/fb.h>
#endif //CONFIG_ENABLE_NOTIFIER_FB

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
//#include <linux/input/vir_ps.h> 
#include <linux/sensors.h>
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifndef CONFIG_ENABLE_TOUCH_PIN_CONTROL
// TODO : Please FAE colleague to confirm with customer device driver engineer about the value of RST and INT GPIO setting
#define MS_TS_MSG_IC_GPIO_RST   0
#define MS_TS_MSG_IC_GPIO_INT   1
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
#include <linux/pinctrl/consumer.h>

#define PINCTRL_STATE_ACTIVE	"pmx_ts_active"
#define PINCTRL_STATE_SUSPEND	"pmx_ts_suspend"
#define PINCTRL_STATE_RELEASE	"pmx_ts_release"
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL

#ifdef CONFIG_TP_HAVE_KEY
#define TOUCH_KEY_MENU (139) //229
#define TOUCH_KEY_HOME (172) //102
#define TOUCH_KEY_BACK (158)
#define TOUCH_KEY_SEARCH (217)

#define MAX_KEY_NUM (4)
#endif //CONFIG_TP_HAVE_KEY

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

#include <linux/sched.h>
#include <linux/kthread.h>
//#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>

#include <linux/namei.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
//#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION
/*
#ifdef CONFIG_ENABLE_TOUCH_PIN_CONTROL
#include <linux/pinctrl/consumer.h>
#endif //CONFIG_ENABLE_TOUCH_PIN_CONTROL
*/
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>

#ifdef TIMER_DEBUG
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#endif //TIMER_DEBUG

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>
#endif //CONFIG_MTK_SENSOR_HUB_SUPPORT

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
#include <linux/regulator/consumer.h>
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

#ifdef CONFIG_MTK_BOOT
#include "mtk_boot_common.h"
#endif //CONFIG_MTK_BOOT

#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_gpio.h>

#include <cust_eint.h>
#include <pmic_drv.h>

#include "cust_gpio_usage.h"
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#define CONFIG_MT_RT_MONITOR
#ifdef CONFIG_MT_RT_MONITOR
#define MT_ALLOW_RT_PRIO_BIT 0x10000000
#else
#define MT_ALLOW_RT_PRIO_BIT 0x0
#endif //CONFIG_MT_RT_MONITOR
#define REG_RT_PRIO(x) ((x) | MT_ALLOW_RT_PRIO_BIT)

#define RTPM_PRIO_TPD  REG_RT_PRIO(4)

#include "tpd.h"

#ifndef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#define MS_TS_MSG_IC_GPIO_RST   (GPIO_CTP_RST_PIN)
#define MS_TS_MSG_IC_GPIO_INT   (GPIO_CTP_EINT_PIN)
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD

#ifdef CONFIG_TP_HAVE_KEY
#define TOUCH_KEY_MENU    KEY_MENU 
#define TOUCH_KEY_HOME    KEY_HOMEPAGE 
#define TOUCH_KEY_BACK    KEY_BACK
#define TOUCH_KEY_SEARCH  KEY_SEARCH

#define MAX_KEY_NUM (4)
#endif //CONFIG_TP_HAVE_KEY

#endif

//------------------------------------------------------------------------------//

#define u8   unsigned char
#define u16  unsigned short
#define u32  unsigned int
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
#define CHIP_TYPE_MSG28XX   (0x85) // EX. MSG2833/MSG2835/MSG2836/MSG2840/MSG2856/MSG5846
#define CHIP_TYPE_MSG58XXA  (0xBF) // EX. MSG5846A
#define CHIP_TYPE_MSG5846A  (0x5846) 
#define CHIP_TYPE_MSG5856A  (0x5856) 
#define CHIP_TYPE_MSG2836A  (0x2836) 
#define CHIP_TYPE_MSG2846A  (0x2846) 
#define CHIP_TYPE_ILI2117A  (0x2117) // EX. ILI2117A
#define CHIP_TYPE_ILI2118A  (0x2118) // EX. ILI2118A

// Chip Revision
#define CHIP_TYPE_MSG22XX_REVISION_U05   (0x04) // U05

#define PACKET_TYPE_SELF_FREQ_SCAN  (0x02)
#define PACKET_TYPE_TOOTH_PATTERN   (0x20)
#define PACKET_TYPE_CSUB_PATTERN   (0x30)
#define PACKET_TYPE_FOUT_PATTERN   (0x40)
#define PACKET_TYPE_GESTURE_WAKEUP  (0x50)
#define PACKET_TYPE_FREQ_PATTERN	(0x60)
#define PACKET_TYPE_GESTURE_DEBUG  		(0x51)
#define PACKET_TYPE_GESTURE_INFORMATION	(0x52)
#define PACKET_TYPE_ESD_CHECK_HW_RESET	(0x60)

#define TOUCH_SCREEN_X_MIN   (0)
#define TOUCH_SCREEN_Y_MIN   (0)
/*
 * Note.
 * Please change the below touch screen resolution according to the touch panel that you are using.
 */
#define TOUCH_SCREEN_X_MAX   (1080)  //LCD_WIDTH
#define TOUCH_SCREEN_Y_MAX   (1920) //LCD_HEIGHT
/*
 * Note.
 * Please do not change the below setting.
 */
#define TPD_WIDTH   (2048)
#define TPD_HEIGHT  (2048)


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

#define MAX_UPDATE_FIRMWARE_BUFFER_SIZE    (130) // 130KB. The size shall be large enough for stored any kind firmware size of MSG22XX(48.5KB)/MSG28XX(130KB)/MSG58XX(130KB)/MSG58XXA(130KB).

#define MAX_I2C_TRANSACTION_LENGTH_LIMIT      (250) //(128) // Please change the value depends on the I2C transaction limitation for the platform that you are using.
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
#define FEATURE_DISABLE_ESD_PROTECTION_CHECK    0x0013
#define FEATURE_APK_PRINT_FIRMWARE_SPECIFIC_LOG    0x0014
#define FEATURE_SELF_FREQ_SCAN    0x0015

#define I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE   (20) // delay 20ms
#define I2C_SMBUS_WRITE_COMMAND_DELAY_FOR_PLATFORM   (5) // delay 5ms
#define I2C_SMBUS_READ_COMMAND_DELAY_FOR_PLATFORM   (5) // delay 5ms

#define FIRMWARE_FILE_PATH_ON_SD_CARD      "/mnt/sdcard/msctp_update.bin"

#define POWER_SUPPLY_BATTERY_STATUS_PATCH  "/sys/class/power_supply/battery/status"
#define ILITEK_TRIMCODE_INITIAL_PATH_ON_SD_CARD      "/mnt/sdcard/trimcode.txt"
#define CHARGER_DETECT_CHECK_PERIOD   (100) // delay 1s

#define ESD_PROTECT_CHECK_PERIOD   (300) // delay 3s
#define ESD_CHECK_HW_RESET_PACKET_LENGTH    (8)

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_JNI_INTERFACE
#define MSGTOOL_MAGIC_NUMBER               96
#define MSGTOOL_IOCTL_RUN_CMD              _IO(MSGTOOL_MAGIC_NUMBER, 1)


#define MSGTOOL_RESETHW           0x01 
#define MSGTOOL_REGGETXBYTEVALUE  0x02 
#define MSGTOOL_HOTKNOTSTATUS     0x03 
#define MSGTOOL_FINGERTOUCH       0x04
#define MSGTOOL_BYPASSHOTKNOT     0x05
#define MSGTOOL_DEVICEPOWEROFF    0x06
#define MSGTOOL_GETSMDBBUS        0x07
#define MSGTOOL_SETIICDATARATE    0x08
#define MSGTOOL_ERASE_FLASH		  0x09
#endif //CONFIG_ENABLE_JNI_INTERFACE

//------------------------------------------------------------------------------//

#define PROC_NODE_CLASS                       "class"
#define PROC_NODE_MS_TOUCHSCREEN_MSG20XX      "ms-touchscreen-msg20xx"
#define PROC_NODE_DEVICE                      "device"
#define PROC_NODE_CHIP_TYPE                   "chip_type"
#define PROC_NODE_FIRMWARE_DATA               "data"
#define PROC_NODE_FIRMWARE_UPDATE             "update"
#define PROC_NODE_CUSTOMER_FIRMWARE_VERSION   "version"
#define PROC_NODE_PLATFORM_FIRMWARE_VERSION   "platform_version"
#define PROC_NODE_DEVICE_DRIVER_VERSION       "driver_version"
#define PROC_NODE_SD_CARD_FIRMWARE_UPDATE     "sdcard_update"
#define PROC_NODE_FIRMWARE_DEBUG              "debug"
#define PROC_NODE_FIRMWARE_SET_DEBUG_VALUE    "set_debug_value"
#define PROC_NODE_FIRMWARE_SMBUS_DEBUG        "smbus_debug"

#define PROC_NODE_MP_TEST_CUSTOMISED          "mp_test_customised"

#define PROC_NODE_FIRMWARE_SET_DQMEM_VALUE    "set_dqmem_value"

#ifdef CONFIG_ENABLE_ITO_MP_TEST
#define PROC_NODE_MP_TEST                     "test"
#define PROC_NODE_MP_TEST_LOG                 "test_log"
#define PROC_NODE_MP_TEST_FAIL_CHANNEL        "test_fail_channel"
#define PROC_NODE_MP_TEST_SCOPE               "test_scope"
#define PROC_NODE_MP_TEST_LOG_ALL             "test_log_all"
#endif //CONFIG_ENABLE_ITO_MP_TEST

#define PROC_NODE_FIRMWARE_MODE               "mode"
#define PROC_NODE_FIRMWARE_SENSOR             "sensor"
#define PROC_NODE_FIRMWARE_PACKET_HEADER      "header"

#define PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS   "query_feature_support_status"
#define PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS  "change_feature_support_status"

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#define PROC_NODE_GESTURE_WAKEUP_MODE         "gesture_wakeup_mode"
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
#define PROC_NODE_GESTURE_DEBUG_MODE          "gesture_debug"
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#define PROC_NODE_GESTURE_INFORMATION_MODE    "gesture_infor"
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_COUNT_REPORT_RATE
#define PROC_NODE_REPORT_RATE                 "report_rate"
#endif //CONFIG_ENABLE_COUNT_REPORT_RATE

#define PROC_NODE_GLOVE_MODE                  "glove_mode"
#define PROC_NODE_OPEN_GLOVE_MODE             "open_glove_mode"
#define PROC_NODE_CLOSE_GLOVE_MODE            "close_glove_mode"

#define PROC_NODE_LEATHER_SHEATH_MODE         "leather_sheath_mode"

#ifdef CONFIG_ENABLE_JNI_INTERFACE
#define PROC_NODE_JNI_NODE                    "msgtool"
#endif //CONFIG_ENABLE_JNI_INTERFACE

#define PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE     "selinux_limit_update"
#define PROC_NODE_FORCE_FIRMWARE_UPDATE             "force_fw_update"
#define PROC_NODE_TRIM_CODE		             "trimcode"
#define PROC_NODE_CONTROL_FILM_MODE		            "film"
//------------------------------------------------------------------------------//

#define MUTUAL_DEMO_MODE_PACKET_LENGTH    (43) // for MSG28xx
#define SELF_DEMO_MODE_PACKET_LENGTH    (8) // for MSG22xx

#define MUTUAL_MAX_TOUCH_NUM           (10) // for MSG28xx    
#define SELF_MAX_TOUCH_NUM           (2) // for MSG22xx     

#define MUTUAL_DEBUG_MODE_PACKET_LENGTH    (1280) // for MSG28xx. It is a predefined maximum packet length, not the actual packet length which queried from firmware.
#define SELF_DEBUG_MODE_PACKET_LENGTH    (128) //  for MSG22xx
#define SELF_FREQ_SCAN_PACKET_LENGTH    (306) // TODO : 

#define MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE (48)  //48K
#define MSG22XX_FIRMWARE_INFO_BLOCK_SIZE (512) //512Byte

//#define MSG22XX_FIRMWARE_MODE_UNKNOWN_MODE    (0xFF)
#define MSG22XX_FIRMWARE_MODE_DEMO_MODE       (0x00)
#define MSG22XX_FIRMWARE_MODE_DEBUG_MODE      (0x01)
#define MSG22XX_FIRMWARE_MODE_RAW_DATA_MODE   (0x02)


#define MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE (128) //128K
#define MSG28XX_FIRMWARE_INFO_BLOCK_SIZE (2) //2K
#define MSG28XX_FIRMWARE_WHOLE_SIZE (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+MSG28XX_FIRMWARE_INFO_BLOCK_SIZE) //130K
//#define MSG28XX_FIRMWARE_WHOLE_SIZE (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+MSG22XX_FIRMWARE_INFO_BLOCK_SIZE) //48K
#define MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE  (128)
#define MSG28XX_EMEM_SIZE_BYTES_ONE_WORD  (4)

#define MSG28XX_EMEM_MAIN_MAX_ADDR  (0x3FFF) //0~0x3FFF = 0x4000 = 16384 = 65536/4
#define MSG28XX_EMEM_INFO_MAX_ADDR  (0x1FF) //0~0x1FF = 0x200 = 512 = 2048/4

#define MSG28XX_FIRMWARE_MODE_UNKNOWN_MODE (0xFF)
#define MSG28XX_FIRMWARE_MODE_DEMO_MODE    (0x00)
#define MSG28XX_FIRMWARE_MODE_DEBUG_MODE   (0x01)

#define EMEM_SIZE_MSG28XX (1024*130)
#define EMEM_SIZE_MSG22XX ((1024*48) + 512 )
#define EMEM_TYPE_ALL_BLOCKS 	0x00
#define EMEM_TYPE_MAIN_BLOCK 	0x01
#define EMEM_TYPE_INFO_BLOCK 	0x02

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
#define UPDATE_FIRMWARE_RETRY_COUNT (2)
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#define FIRMWARE_GESTURE_INFORMATION_MODE_A	(0x00)
#define FIRMWARE_GESTURE_INFORMATION_MODE_B	(0x01)
#define FIRMWARE_GESTURE_INFORMATION_MODE_C	(0x02)
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#define FEATURE_FILM_MODE_LOW_STRENGTH     	    0x01 
#define FEATURE_FILM_MODE_HIGH_STRENGTH         0x02 
#define FEATURE_FILM_MODE_DEFAULT		        0x00 

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

#define DEBUG_LEVEL(level, dev, fmt, arg...) do {\
	                                           if (level <= TOUCH_DRIVER_DEBUG_LOG_LEVEL)\
	                                               printk("ILITEK %s, lind = %d,"fmt, __func__, __LINE__, ##arg);\
                                        } while (0)

#define DBG(dev, fmt, arg...) DEBUG_LEVEL(1, dev, fmt, ##arg) 
/*
#define DEBUG_LEVEL(level, dev, fmt, arg...) do {\
	                                           if (level <= TOUCH_DRIVER_DEBUG_LOG_LEVEL)\
	                                               dev_info(dev, fmt, ##arg);\
                                        } while (0)

#define DBG(dev, fmt, arg...) DEBUG_LEVEL(1, dev, fmt, ##arg)
*/

#define MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))

#define BK_REG8_WL(addr,val)    ( RegSetLByteValue( addr, val ) )
#define BK_REG8_WH(addr,val)    ( RegSetHByteValue( addr, val ) )
#define BK_REG16_W(addr,val)    ( RegSet16BitValue( addr, val ) )
#define BK_REG8_RL(addr)        ( RegGetLByteValue( addr ) )
#define BK_REG8_RH(addr)        ( RegGetHByteValue( addr ) )
#define BK_REG16_R(addr)        ( RegGet16BitValue( addr ) )

#define PRINTF_EMERG(fmt, ...)  printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_ALERT(fmt, ...)  printk(KERN_ALERT pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_CRIT(fmt, ...)   printk(KERN_CRIT pr_fmt(fmt), ##__VA_ARGS__) 
#define PRINTF_ERR(fmt, ...)    printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_WARN(fmt, ...)   printk(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_NOTICE(fmt, ...) printk(KERN_NOTICE pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_INFO(fmt, ...)   printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#define PRINTF_DEBUG(fmt, ...)  printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__) 

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
    MUTUAL_MODE = 0x5705,
    MUTUAL_SINE = 0x5706,
    MUTUAL_KEY = 0x6734,         // latter FW v1007.
    MUTUAL_SINE_KEY = 0x6733,    // latter FW v1007.
    SELF = 0x6278,
    WATERPROOF = 0x7992,
    MUTUAL_SINGLE_DRIVE = 0x0158,
    SINE_PHASE = 0xECFA,
    SET_PHASE,
    DEEP_STANDBY = 0x6179,
    GET_BG_SUM = 0x7912,
} ItoTestMsg28xxFwMode_e;

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

typedef struct
{
    u16 nX;
    u16 nY;
} SelfTouchPoint_t;

typedef struct
{
    u8 nTouchKeyMode;
    u8 nTouchKeyCode;
    u8 nFingerNum;
    SelfTouchPoint_t tPoint[2];
} SelfTouchInfo_t;

typedef struct
{
    u8 nFirmwareMode;
    u8 nLogModePacketHeader;
    u16 nLogModePacketLength;
    u8 nIsCanChangeFirmwareMode;
} SelfFirmwareInfo_t;

typedef struct
{
    u16 nId;
    u16 nX;
    u16 nY;
    u16 nP;
} MutualTouchPoint_t;

/// max 80+1+1 = 82 bytes
typedef struct
{
    u8 nCount;
    u8 nKeyCode;
    MutualTouchPoint_t tPoint[10];
} MutualTouchInfo_t;

typedef struct
{
    u16 nFirmwareMode;
    u8 nType;
    u8 nLogModePacketHeader;
    u8 nMy;
    u8 nMx;
    u8 nSd;
    u8 nSs;
    u16 nLogModePacketLength;
} MutualFirmwareInfo_t;

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
typedef struct {
    u16 nSwId;

#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
    u8 (*pUpdateBin)[1024];
#else // ONE DIMENSIONAL ARRAY
    u8 *pUpdateBin;
#endif //CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
    
} SwIdData_t;


/*
 * Note.
 * The following is sw id enum definition for MSG22XX.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG22XX_SW_ID_XXXX = 0x0001,
    MSG22XX_SW_ID_YYYY = 0x0002,  
    MSG22XX_SW_ID_UNDEFINED = 0xFFFF
} Msg22xxSwId_e;

/*
 * Note.
 * The following is sw id enum definition for MSG28XX.
 * 0x0000 and 0xFFFF are not allowed to be defined as SW ID.
 * SW_ID_UNDEFINED is a reserved enum value, do not delete it or modify it.
 * Please modify the SW ID of the below enum value depends on the TP vendor that you are using.
 */
typedef enum {
    MSG28XX_SW_ID_XXXX = 0x0001,
    MSG28XX_SW_ID_YYYY = 0x0002,
    MSG28XX_SW_ID_UNDEFINED = 0xFFFF
} Msg28xxSwId_e;
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID

#ifdef CONFIG_ENABLE_JNI_INTERFACE
typedef struct
{
    u64 nCmdId;
    u64 nSndCmdDataPtr;      //send data to fw  
    u64 nSndCmdLen;
    u64 nRtnCmdDataPtr;      //receive data from fw
    u64 nRtnCmdLen;
} MsgToolDrvCmd_t;
#endif //CONFIG_ENABLE_JNI_INTERFACE

/*--------------------------------------------------------------------------*/
/* GLOBAL FUNCTION DECLARATION                                              */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_DMA_IIC
extern void DmaAlloc(void);
extern void DmaReset(void);
extern void DmaFree(void);
#endif //CONFIG_ENABLE_DMA_IIC

extern u16  RegGet16BitValue(u16 nAddr);
extern u8   RegGetLByteValue(u16 nAddr);
extern u8   RegGetHByteValue(u16 nAddr);
extern void RegGetXBitValue(u16 nAddr, u8 * pRxData, u16 nLength, u16 nMaxI2cLengthLimit);
extern s32  RegSet16BitValue(u16 nAddr, u16 nData);
extern s32  RetryRegSet16BitValue(u16 nAddr, u16 nData);
extern void RegSetLByteValue(u16 nAddr, u8 nData);
extern void RegSetHByteValue(u16 nAddr, u8 nData);
extern void RegSet16BitValueOn(u16 nAddr, u16 nData);
extern void RegSet16BitValueOff(u16 nAddr, u16 nData);
extern u16  RegGet16BitValueByAddressMode(u16 nAddr, AddressMode_e eAddressMode);
extern void RegSet16BitValueByAddressMode(u16 nAddr, u16 nData, AddressMode_e eAddressMode);
extern void RegMask16BitValue(u16 nAddr, u16 nMask, u16 nData, AddressMode_e eAddressMode);
extern void RegGetXBitWrite4ByteValue(u16 nAddr, u8 * pRxData, u16 nLength, u16 nMaxI2cLengthLimit);
extern s32  DbBusEnterSerialDebugMode(void);
extern void DbBusExitSerialDebugMode(void);
extern void DbBusIICUseBus(void);
extern void DbBusIICNotUseBus(void);
extern void DbBusIICReshape(void);
extern void DbBusStopMCU(void);
extern void DbBusNotStopMCU(void);
extern void DbBusResetSlave(void);
extern void DbBusWaitMCU(void);
extern void SetCfb(u8 Cfb);
extern s32 IicWriteData(u8 nSlaveId, u8* pBuf, u16 nSize);
extern s32 IicReadData(u8 nSlaveId, u8* pBuf, u16 nSize);
extern s32 IicSegmentReadDataByDbBus(u8 nRegBank, u8 nRegAddr, u8* pBuf, u16 nSize, u16 nMaxI2cLengthLimit);
extern s32 IicSegmentReadDataBySmBus(u16 nAddr, u8* pBuf, u16 nSize, u16 nMaxI2cLengthLimit);

extern void DrvTouchDeviceHwReset(void);
extern void DrvEnableFingerTouchReport(void);
extern void DrvDisableFingerTouchReport(void);
extern void DrvSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate);
extern void DrvGetCustomerFirmwareVersion(u16 *pMajor, u16 *pMinor, u8 **ppVersion);
extern void DrvGetCustomerFirmwareVersionByDbBus(EmemType_e eEmemType, u16 *pMajor, u16 *pMinor, u8 **ppVersion);

extern void DrvMsg28xxReadEFlashStart(u16 nStartAddr, EmemType_e eEmemType);
extern void DrvMsg28xxReadEFlashDoRead(u16 nReadAddr, u8 *pReadData);
extern void DrvMsg28xxReadEFlashEnd(void);

extern s32 /*__devinit*/ MsDrvInterfaceTouchDeviceProbe(struct i2c_client *pClient, const struct i2c_device_id *pDeviceId);
extern s32 /*__devexit*/ MsDrvInterfaceTouchDeviceRemove(struct i2c_client *pClient);
extern void DrvDBBusI2cResponseAck(void);
#ifdef CONFIG_ENABLE_NOTIFIER_FB
extern int MsDrvInterfaceTouchDeviceFbNotifierCallback(struct notifier_block *pSelf, unsigned long nEvent, void *pData);
#else
#ifdef CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
extern void MsDrvInterfaceTouchDeviceResume(struct device *pDevice);
extern void MsDrvInterfaceTouchDeviceSuspend(struct device *pDevice);
#else
#ifdef CONFIG_HAS_EARLYSUSPEND
extern void MsDrvInterfaceTouchDeviceResume(struct early_suspend *pSuspend);        
extern void MsDrvInterfaceTouchDeviceSuspend(struct early_suspend *pSuspend);
#endif
#endif //CONFIG_PLATFORM_USE_ANDROID_SDK_6_UPWARD
#endif //CONFIG_ENABLE_NOTIFIER_FB
extern void MsDrvInterfaceTouchDeviceSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate);

#endif  /* __ILITEK_DRV_COMMON_H__ */
