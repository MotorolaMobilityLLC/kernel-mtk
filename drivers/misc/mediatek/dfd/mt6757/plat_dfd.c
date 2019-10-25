/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <mt-plat/mtk_devinfo.h>
#include <mt-plat/mtk_platform_debug.h>
#include "../dfd.h"
#include <linux/printk.h>

static unsigned int is_kibop;

unsigned int check_dfd_support(void)
{
	unsigned int segment;

	is_kibop = 0;
	segment = (get_devinfo_with_index(30) & 0x000000E0) >> 5;
	if (segment == 0x3 || segment == 0x7) {
		is_kibop = 1;
		pr_notice("[dfd] MT6757p enable DFD\n");
		return 1;
	}

	pr_notice("[dfd] Kibo disable DFD,0x%x\n", segment);
	return 0;

}

unsigned int dfd_infra_base(void)
{
	return 0x390;
}

/* DFD_V30_BASE_ADDR_IN_INFRA
 * bit[9:0] : AP address bit[33:24]
 */
unsigned int dfd_ap_addr_offset(void)
{
	return 24;
}
