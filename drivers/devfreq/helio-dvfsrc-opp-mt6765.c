/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include "helio-dvfsrc-opp.h"

int get_soc_efuse(void)
{
	return get_devinfo_with_index(60);
}

void vcore_volt_init(void)
{
/*
 * ToDo: Build OPP Table
 * ToDo: Build file system for vcore voltage
 */
/*
 *         int ddr_type = get_ddr_type();
 *         int soc_efuse = get_soc_efuse();
 *
 */
}

