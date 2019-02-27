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

#include <helio-dvfsrc.h>
#include <helio-dvfsrc-opp.h>

static struct opp_profile opp_table[VCORE_DVFS_OPP_NUM];
static int vcore_dvfs_opp_to_vcore_opp[VCORE_DVFS_OPP_NUM];
static int vcore_dvfs_opp_to_ddr_opp[VCORE_DVFS_OPP_NUM];


int get_cur_vcore_dvfs_opp(void)
{
	return VCORE_DVFS_OPP_NUM - __builtin_ffs(get_vcore_dvfs_level());
}

int get_vcore_uv(int opp)
{
	return opp_table[opp].vcore_uv;
}

int get_cur_vcore_opp(void)
{
	return vcore_dvfs_opp_to_vcore_opp[get_cur_vcore_dvfs_opp()];
}

int get_cur_vcore_uv(void)
{
	return opp_table[get_cur_vcore_dvfs_opp()].vcore_uv;
}

int get_ddr_khz(int opp)
{
	return opp_table[opp].ddr_khz;
}

int get_cur_ddr_opp(void)
{
	return vcore_dvfs_opp_to_ddr_opp[get_cur_vcore_dvfs_opp()];
}

int get_cur_ddr_khz(void)
{
	return opp_table[get_cur_vcore_dvfs_opp()].ddr_khz;
}
