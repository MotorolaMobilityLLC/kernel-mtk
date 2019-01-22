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

#ifndef __MPU_H__
#define __MPU_H__

#include <mt_emi.h>

#define EMI_MPU_TEST		0

#define EMI_MPU_MAX_CMD_LEN	128
#define EMI_MPU_MAX_TOKEN	4

#define NO_PROTECTION	0
#define SEC_RW		1
#define SEC_RW_NSEC_R	2
#define SEC_RW_NSEC_W	3
#define SEC_R_NSEC_R	4
#define FORBIDDEN	5
#define SEC_R_NSEC_RW	6

#define UNLOCK		0
#define LOCK		1

struct emi_region_info_t {
	unsigned long long start;
	unsigned long long end;
	unsigned int region;
	unsigned int apc[EMI_MPU_DGROUP_NUM];
};

extern int is_md_master(unsigned int master_id);
extern void set_ap_region_permission(unsigned int apc[EMI_MPU_DGROUP_NUM]);

#endif /* __MPU_H__ */
