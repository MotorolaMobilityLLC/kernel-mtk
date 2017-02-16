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
#include <mach/mtk_secure_api.h>
#include <mt-plat/mtk_platform_debug.h>
#include "../dfd.h"

static unsigned int is_kibop;

unsigned int check_dfd_support(void)
{
	unsigned int segment;

	is_kibop = 0;
	segment = (get_devinfo_with_index(30) & 0x000000E0) >> 5;
	if (segment == 0x3 || segment == 0x7) {
		is_kibop = 1;
		pr_err("[dfd] Kibo+ enable DFD\n");
		return 1;
	}

	pr_err("[dfd] Kibo disable DFD,0x%x\n", segment);
	return 0;

}

void dfd30_SW_reset_workaround(void)
{
	int ret;

	if (is_kibop != 1)
		return;

	pr_err("applying %s\n", __func__);
	ret = mt_secure_call(MTK_SIP_KERNEL_DFD, PLAT_MTK_DFD_WRITE_MAGIC,
			0x0, 0x0);
	if (ret < 0)
		pr_err("[dfd] Disable DFD failed");

	ret = set_sram_flag_dfd_invalid();
	if (ret < 0)
		pr_err("[dfd] clear DFD flag failed");

	return;
}
