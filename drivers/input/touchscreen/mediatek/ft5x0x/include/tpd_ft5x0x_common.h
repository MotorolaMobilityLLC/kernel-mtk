/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_WAKEUP_TRIAL	60
#define TPD_WAKEUP_DELAY	100


/*#define CONFIG_TPD_ROTATE_270*//*if use,90/270/180 move to defconfig file*/
/*#define CONFIG_FT_AUTO_UPGRADE_SUPPORT*//*move to defconfig file*/
/*#define FT5X36_UPGADE*//*donot use it*/
/*#define FTS_AUTO_UPGRADE*//*donot use it*/
#define TPD_DELAY		(2*HZ/100)
/*#define CONFIG_CUST_FTS_APK_DEBUG*//*move to defconfig file*/

/*focaltech register*/
#define FT_GESTRUE_MODE_SWITCH_REG 0xD0
#define FT_GESTRUE_GETID_REG 0xD3

#define TPD_RES_X		800
#define TPD_RES_Y		1280

#define CONFIG_TPD_HAVE_CALIBRATION
/* #define TPD_CALIBRATION_MATRIX	{962, 0, 0, 0, 1600, 0, 0, 0}; */

#define TPD_CALIBRATION_MATRIX_ROTATION_NORMAL  {-4096, 0, 800*4096, 0, -4096, 1280*4096, 0, 0}
#define TPD_CALIBRATION_MATRIX_ROTATION_FACTORY {-4096, 0, 800*4096, 0, -4096, 1280*4096, 0, 0}
/* #define TPD_CALIBRATION_MATRIX_ROTATION_NORMAL  {-5328, 0, 800*4096, 0, 4096, 0, 0, 0}; */
/* #define TPD_CALIBRATION_MATRIX_ROTATION_FACTORY  {-5328, 0, 800*4096, 0, 4096, 0, 0, 0}; */


/*
#define FTP_DEBUG_ON                   0
#define FTP_ERROR(fmt,arg...)          dprintf(CRITICAL,"<FTP-ERR>"fmt"\n", ##arg)
#define FTP_INFO(fmt,arg...)           dprintf(CRITICAL,"<FTP-INF>"fmt"\n", ##arg)

#if FTP_DEBUG_ON
#define FTP_DEBUG(fmt,arg...)          do{\
					 dprintf(CRITICAL,"<FTP-DBG>"fmt"\n", ##arg);\
				       }while(0)
#else
#define FTP_DEBUG(fmt,arg...)
#endif
*/

typedef void (*GES_CBFUNC)(u8);
/*****************************************************************************
 * ENUM
 ****************************************************************************/
enum GTP_WORK_STATE {
	GTP_UNKNOWN = 0,
	GTP_NORMAL,
	GTP_DOZE,
	GTP_SLEEP,
};

enum TOUCH_DOZE_T1 {
	DOZE_INPOCKET = 0,
	DOZE_NOT_INPOCKET = 1,
};

enum TOUCH_DOZE_T2 {
	DOZE_DISABLE = 0,
	DOZE_ENABLE = 1,
};

enum TOUCH_WAKE_T {
	TOUCH_WAKE_BY_NONE,
	TOUCH_WAKE_BY_INT,
	TOUCH_WAKE_BY_IPI,
	TOUCH_WAKE_BY_SWITCH
};

/* typedef enum */
/* { */
/* //SCP->AP */
/* IPI_COMMAND_SA_GESTURE_TYPE, */

/* //AP->SCP */
/* IPI_COMMAND_AS_CUST_PARAMETER, */
/* IPI_COMMAND_AS_ENTER_DOZEMODE, */
/* IPI_COMMAND_AS_ENABLE_GESTURE, */
/* IPI_COMMAND_AS_GESTURE_SWITCH, */

/* }TOUCH_IPI_CMD_T; */

/*****************************************************************************
 * STRUCTURE
 ****************************************************************************/
struct Touch_SmartWake_ID {
	u8 id;
	GES_CBFUNC cb;
};

/* typedef struct */
/* { */
/* u32 i2c_num; */
/* u32 int_num; */
/* u32 io_int; */
/* u32 io_rst; */
/* }Touch_Cust_Setting; */

/* typedef struct */
/* { */
/* u32 cmd; */

/* union { */
/* u32 data; */
/* Touch_Cust_Setting tcs; */
/* } param; */
/* }Touch_IPI_Packet; */

/* #define TPD_HAVE_TREMBLE_ELIMINATION */


extern struct tpd_device *tpd;
extern unsigned int tpd_rst_gpio_number;
extern void tpd_button(unsigned int x, unsigned int y, unsigned int down);
#ifdef CONFIG_CUST_FTS_APK_DEBUG
extern int ft_rw_iic_drv_init(struct i2c_client *client);
extern void  ft_rw_iic_drv_exit(void);
#endif

#ifdef CONFIG_FT_AUTO_UPGRADE_SUPPORT
extern u8 *tpd_i2c_dma_va;
extern dma_addr_t tpd_i2c_dma_pa;
extern int tpd_auto_upgrade(struct i2c_client *client);
#endif
#endif /* TOUCHPANEL_H__ */
