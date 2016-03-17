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

#ifndef SEC_CFG_H
#define SEC_CFG_H

/* EXPORT TO USERS */
#include "sec_cfg_v1.h"
#include "sec_cfg_v3.h"
#include "sec_cfg_common.h"
#include "sec_cfg_crypto.h"

/**************************************************************************
 *  SECCFG VERSION
 **************************************************************************/
typedef union {
	SECURE_CFG_V1 v1;
	SECURE_CFG_V3 v3;
} SECCFG_U;

typedef union {
	SECURE_IMG_INFO_V1 v1;
	SECURE_IMG_INFO_V3 v3;
} SEC_IMG_U;


/**************************************************************************
 *  EXPORT FUNCTIONS
 **************************************************************************/

#endif
