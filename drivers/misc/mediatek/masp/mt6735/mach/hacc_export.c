/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2011. All rights reserved.
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

#include "sec_osal.h"
/*#include <mach/mt_typedefs.h>*/
#include "sec_hal.h"
#include "sec_osal_light.h"
#include "sec_boot_lib.h"
#include "sec_error.h"
#include "hacc_mach.h"



/******************************************************************************
 * This file provide the HACC operation function to secure library
 * All the functions should be general ...
 ******************************************************************************/
#define MOD                         "ASF"

/******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/
bool bHACC_HWWrapKeyInit = FALSE;
bool bHACC_SWKeyInit = FALSE;

/******************************************************************************
 *  INTERNAL VARIABLES
 ******************************************************************************/
static const unsigned int g_HACC_CFG_1[8] = {
	0x9ED40400, 0x00E884A1, 0xE3F083BD, 0x2F4E6D8A,
	0xFF838E5C, 0xE940A0E3, 0x8D4DECC6, 0x45FC0989
};

static const unsigned int g_HACC_CFG_2[8] = {
	0xAA542CDA, 0x55522114, 0xE3F083BD, 0x55522114,
	0xAA542CDA, 0xAA542CDA, 0x55522114, 0xAA542CDA
};

static const unsigned int g_HACC_CFG_3[8] = {
	0x2684B690, 0xEB67A8BE, 0xA113144C, 0x177B1215,
	0x168BEE66, 0x1284B684, 0xDF3BCE3A, 0x217F6FA2
};


/******************************************************************************
 *  INTERNAL ENGINE
 ******************************************************************************/
static unsigned char *sp_hacc_internal(unsigned char *buf, unsigned int size, bool bAC,
				       HACC_USER user, bool bDoLock, AES_OPS aes_type, bool bEn)
{
	unsigned int err = 0;

	/* ---------------------------- */
	/* get hacc lock                 */
	/* ---------------------------- */
	if (TRUE == bDoLock) {
		/* If the semaphore is successfully acquired, this function returns 0. */
		err = osal_hacc_lock();

		if (err) {
			err = ERR_SBOOT_HACC_LOCK_FAIL;
			goto _err;
		}
	}

	/* ---------------------------- */
	/* ciphering and force AC               */
	/* ---------------------------- */
	switch (user) {
	case HACC_USER1:
		/* ---------------------------- */
		/* use smart phone hacc function 1  */
		/* ---------------------------- */
		HACC_V3_Init(bEn, g_HACC_CFG_1);

		HACC_V3_Run((unsigned int *)buf, size, (unsigned int *)buf);

		HACC_V3_Terminate();
		break;

	case HACC_USER2:
		/* ---------------------------- */
		/* use smart phone hacc function 2  */
		/* ---------------------------- */
		HACC_V3_Init(bEn, g_HACC_CFG_2);

		HACC_V3_Run((unsigned int *)buf, size, (unsigned int *)buf);

		HACC_V3_Terminate();
		break;


	case HACC_USER3:
		/* use smart phone hacc function 3 */
		/* ---------------------------- */
		if (FALSE == bHACC_HWWrapKeyInit) {
			err = ERR_HACC_HW_WRAP_KEY_NOT_INIT;
			goto _err;
		}


		err = hacc_set_key(AES_HW_WRAP_KEY, AES_KEY_256);

		if (SEC_OK != err)
			goto _err;

		err = hacc_do_aes(aes_type, buf, buf, AES_BLK_SZ_ALIGN(size));

		if (SEC_OK != err)
			goto _err;
		break;

	case HACC_USER4:
		/* ---------------------------- */
		/* use smart phone hacc function 4  */
		/* ---------------------------- */
		HACC_V3_Init(bEn, g_HACC_CFG_3);

		HACC_V3_Run((unsigned int *)buf, size, (unsigned int *)buf);

		HACC_V3_Terminate();
		break;

	default:
		err = ERR_HACC_UNKNOWN_USER;
		goto _err;
	}

	/* ---------------------------- */
	/* release hacc lock             */
	/* ---------------------------- */
	if (TRUE == bDoLock)
		osal_hacc_unlock();

	return buf;

_err:
	if (TRUE == bDoLock)
		osal_hacc_unlock();

	pr_debug("[%s] HACC Fail (0x%x)\n", MOD, err);

	BUG_ON(!(0));

	return buf;
}

/******************************************************************************
 *  ENCRYPTION
 ******************************************************************************/
unsigned char *masp_hal_sp_hacc_enc(unsigned char *buf, unsigned int size, unsigned char bAC,
				    HACC_USER user, unsigned char bDoLock)
{
	return sp_hacc_internal(buf, size, TRUE, user, bDoLock, AES_ENC, TRUE);
}


/******************************************************************************
 *  DECRYPTION
 ******************************************************************************/
unsigned char *masp_hal_sp_hacc_dec(unsigned char *buf, unsigned int size, unsigned char bAC,
				    HACC_USER user, unsigned char bDoLock)
{
	return sp_hacc_internal(buf, size, TRUE, user, bDoLock, AES_DEC, FALSE);
}

/******************************************************************************
 *  HACC BLK SIZE
 ******************************************************************************/
unsigned int masp_hal_sp_hacc_blk_sz(void)
{
	return AES_BLK_SZ;
}

/******************************************************************************
 *  HACC INITIALIZATION
 ******************************************************************************/
unsigned int masp_hal_sp_hacc_init(unsigned char *sec_seed, unsigned int size)
{
	AES_KEY_SEED keyseed;
	unsigned int i = 0;

	if (_CRYPTO_SEED_LEN != size)
		return ERR_HACC_SEED_LEN_ERROR;

	keyseed.size = HACC_AES_MAX_KEY_SZ;
	for (i = 0; i < HACC_AES_MAX_KEY_SZ / 2; i++) {
		keyseed.seed[i] = sec_seed[i];
		keyseed.seed[HACC_AES_MAX_KEY_SZ - i - 1] = sec_seed[i] + MTK_HACC_SEED;
	}

	pr_debug("0x%x,0x%x,0x%x,0x%x\n", keyseed.seed[0], keyseed.seed[1], keyseed.seed[2],
	     keyseed.seed[3]);

	return hacc_init(&keyseed);
}
