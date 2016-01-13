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

#ifndef TOUCHPANEL_UPGRADE_H__
#define TOUCHPANEL_UPGRADE_H__



/*#define CTP_DETECT_SUPPLIER_THROUGH_GPIO  1*/

#ifdef TPD_ALWAYS_UPGRADE_FW
#undef TPD_ALWAYS_UPGRADE_FW
#endif

/*#define GPIO_CTP_COMPAT_PIN1	GPIO28
#define GPIO_CTP_COMPAT_PIN2	GPIO29*/

#define TPD_SUPPLIER_0		0
#define TPD_SUPPLIER_1		1
#define TPD_SUPPLIER_2		2
#define TPD_SUPPLIER_3		3

#define TPD_FW_VER_0		0x00
#define TPD_FW_VER_1		0x11  /* ht tp   0   1 (choice 0) */
#define TPD_FW_VER_2            0x00
#define TPD_FW_VER_3            0x10  /* zxv tp  0   0 */

/*static unsigned char TPD_FW0[] = {
#include "FT5436IFT5x36i_Ref_20141230_app.i"
};

static unsigned char TPD_FW1[] = {
 // #include "ft5x0x_fw_6127_ht.i"
#include "FT5436IFT5x36i_Ref_20141230_app.i"
};

static unsigned char TPD_FW2[] = {
#include "FT5436IFT5x36i_Ref_20141230_app.i"
};

static unsigned char TPD_FW3[] = {
// #include "ft5x0x_fw_6127_zxv.i"
#include "FT5436IFT5x36i_Ref_20141230_app.i"
};
*/

static unsigned char TPD_FW[] = {
#include "HQ_AW875_FT3427_DJ_A.08.02_V1.0_V02_D01_20151204_app.i"
};
#ifdef CTP_DETECT_SUPPLIER_THROUGH_GPIO
static unsigned char TPD_FW0[] = {
#include "HQ_AW875_FT3427_DJ_A.08.02_V1.0_V02_D01_20151204_app.i"
};

static unsigned char TPD_FW1[] = {
 /* #include "ft5x0x_fw_6127_ht.i" */
#include "HQ_AW875_FT3427_DJ_A.08.02_V1.0_V02_D01_20151204_app.i"
};

static unsigned char TPD_FW2[] = {
#include "HQ_AW875_FT3427_DJ_A.08.02_V1.0_V02_D01_20151204_app.i"
};

static unsigned char TPD_FW3[] = {
#include "HQ_AW875_FT3427_DJ_A.08.02_V1.0_V02_D01_20151204_app.i"
};
#endif /* CTP_DETECT_SUPPLIER_THROUGH_GPIO */
#endif /* TOUCHPANEL_UPGRADE_H__ */
