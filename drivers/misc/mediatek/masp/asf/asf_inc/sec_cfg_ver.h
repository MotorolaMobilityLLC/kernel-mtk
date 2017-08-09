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

#ifndef SEC_CFG_VER_H
#define SEC_CFG_VER_H

#include "sec_cfg.h"
#include "sec_cfg_common.h"
#include "sec_cfg_v1.h"
#include "sec_cfg_v3.h"

typedef enum {
	SECCFG_V1 = 1,
	SECCFG_V1_2 = 2,
	SECCFG_V3 = 3,
	SECCFG_UNSET = 4
} SECCFG_VER;

/**************************************************************************
 *  EXPORT FUNCTIONS
 **************************************************************************/
extern void set_seccfg_ver(SECCFG_VER val);
extern SECCFG_VER get_seccfg_ver(void);
extern SECCFG_STATUS get_seccfg_status(SECCFG_U *p_seccfg);
extern void set_seccfg_status(SECCFG_U *p_seccfg, SECCFG_STATUS val);
extern unsigned int get_seccfg_siu(SECCFG_U *p_seccfg);
extern void set_seccfg_siu(SECCFG_U *p_seccfg, unsigned int val);
extern unsigned int get_seccfg_img_cnt(void);
extern int seccfg_ver_detect(void);
extern int seccfg_ver_correct(void);
extern int seccfg_ver_verify(void);

#endif
