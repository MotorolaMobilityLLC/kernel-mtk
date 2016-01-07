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
#include "hacc_mach.h"
/*#include "sec_log.h"*/
#include "sec_error.h"
#include "sec_typedef.h"

/******************************************************************************
 * Crypto Engine Test Driver Debug Control
 ******************************************************************************/
#define MOD                         "CE"

/******************************************************************************
 * Seed Definition
 ******************************************************************************/
#define _CRYPTO_SEED_LEN            (16)

/******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************/
/* return the result of hwEnableClock ( )
   - TRUE  (1) means crypto engine init success
   - FALSE (0) means crypto engine init fail    */
unsigned char masp_hal_secure_algo_init(void)
{
	bool ret = TRUE;

	return ret;
}

/* return the result of hwDisableClock ( )
   - TRUE  (1) means crypto engine de-init success
   - FALSE (0) means crypto engine de-init fail    */
unsigned char masp_hal_secure_algo_deinit(void)
{
	bool ret = TRUE;

	return ret;
}

/******************************************************************************
 * CRYPTO ENGINE EXPORTED APIs
 ******************************************************************************/
/* perform crypto operation
   @ Direction   : TRUE  (1) means encrypt
		   FALSE (0) means decrypt
   @ ContentAddr : input source address
   @ ContentLen  : input source length
   @ CustomSeed  : customization seed for crypto engine
   @ ResText     : output destination address */
void masp_hal_secure_algo(unsigned char Direction, unsigned char *ContentAddr,
			  unsigned int ContentLen, unsigned char *CustomSeed,
			  unsigned char *ResText)
{
	unsigned int err;
	unsigned char *src, *dst, *seed;
	unsigned int i = 0;

	/* try to get hacc lock */
	do {
		/* If the semaphore is successfully acquired, this function returns 0. */
		err = osal_hacc_lock();
	} while (0 != err);

	/* initialize hacc crypto configuration */
	seed = (unsigned char *)CustomSeed;
	err = masp_hal_sp_hacc_init(seed, _CRYPTO_SEED_LEN);

	if (SEC_OK != err)
		goto _error;

	/* initialize source and destination address */
	src = (unsigned char *)ContentAddr;
	dst = (unsigned char *)ResText;

	/* according to input parameter to encrypt or decrypt */
	switch (Direction) {
	case TRUE:
		/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
		/* ! CCCI driver already got HACC lock ! */
		/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
		dst =
		    masp_hal_sp_hacc_enc((unsigned char *)src, ContentLen, TRUE, HACC_USER3, FALSE);
		break;

	case FALSE:
		/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
		/* ! CCCI driver already got HACC lock ! */
		/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
		dst =
		    masp_hal_sp_hacc_dec((unsigned char *)src, ContentLen, TRUE, HACC_USER3, FALSE);
		break;

	default:
		err = ERR_KER_CRYPTO_INVALID_MODE;
		goto _error;
	}


	/* copy result */
	for (i = 0; i < ContentLen; i++)
		*(ResText + i) = *(dst + i);

	/* try to release hacc lock */
	osal_hacc_unlock();

	return;

_error:
	/* try to release hacc lock */
	osal_hacc_unlock();

	pr_err("[%s] masp_hal_secure_algo error (0x%x)\n", MOD, err);
	BUG_ON(!(0));
}
