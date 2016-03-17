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

/******************************************************************************
 *  INCLUDE LIBRARY
 ******************************************************************************/
#include "sec_boot_lib.h"

/******************************************************************************
 * CONSTANT DEFINITIONS
 ******************************************************************************/
#define MOD                         "SEC_CFG_CRYPTO"

/******************************************************************************
 *  GET SECCFG CIPHER LENGTH
 ******************************************************************************/
unsigned int get_seccfg_cipher_len(void)
{
	switch (get_seccfg_ver()) {
	case SECCFG_V1:
	case SECCFG_V1_2:
		return SECURE_IMAGE_COUNT * sizeof(SECURE_IMG_INFO_V1) + 16;
	case SECCFG_V3:
		return SECURE_IMAGE_COUNT_V3 * sizeof(SECURE_IMG_INFO_V3) + 12 + 4 +
			EXT_REGION_BUF_SIZE;
	default:
		/* return the wrong size */
		return 0;
	}
}
