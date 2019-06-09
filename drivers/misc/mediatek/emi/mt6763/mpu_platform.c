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

#include <linux/platform_device.h>

#include "mt_emi.h"

enum { MASTER_APMCU_CH1 = 0,
	   MASTER_APMCU_CH2 = 1,
	   MASTER_MM_CH1 = 2,
	   MASTER_MDMCU = 3,
	   MASTER_MDDMA = 4,
	   MASTER_MM_CH2 = 5,
	   MASTER_MFG = 6,
	   MASTER_PERI = 7,
	   MASTER_ALL = 8 };

int is_md_master(unsigned int master_id, unsigned int domain_id)
{
	if (((master_id & 0x7) == MASTER_MDMCU) ||
	    ((master_id & 0x7) == MASTER_MDDMA))
		return 1;

	return 0;
}

void set_ap_region_permission(unsigned int apc[EMI_MPU_DGROUP_NUM])
{
	SET_ACCESS_PERMISSION(apc, LOCK, FORBIDDEN, FORBIDDEN, FORBIDDEN,
			      FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION,
			      NO_PROTECTION, FORBIDDEN, SEC_R_NSEC_RW,
			      FORBIDDEN, NO_PROTECTION, FORBIDDEN, FORBIDDEN,
			      FORBIDDEN, NO_PROTECTION);
}
