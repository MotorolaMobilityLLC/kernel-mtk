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

#ifndef __HELIO_DVFSRC_OPP_H
#define __HELIO_DVFSRC_OPP_H

#if defined(CONFIG_MACH_MT6765)
#include <helio-dvfsrc-opp-mt6765.h>
#else
#include <helio-dvfsrc-opp-mt67xx.h>
#endif

struct opp_profile {
	int vcore_uv;
	int ddr_khz;
};

extern int get_cur_vcore_dvfs_opp(void);
extern int get_vcore_uv(int opp);
extern int get_cur_vcore_opp(void);
extern int get_cur_vcore_uv(void);

extern int get_ddr_khz(int opp);
extern int get_cur_ddr_opp(void);
extern int get_cur_ddr_khz(void);

#endif /* __HELIO_DVFSRC_OPP_H */

