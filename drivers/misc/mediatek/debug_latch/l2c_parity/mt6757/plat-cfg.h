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

#ifndef __PLAT_CFG_H__
#define __PLAT_CFG_H__

const struct plat_cfg_l2_parity cfg_l2_parity = {
	.nr_little_cluster = 2,
	.cluster = {
		{
			.mp_id = 0,
			.l2_cache_parity1_rdata = 0x007c,
			.l2_cache_parity2_rdata = 0x0080,
		},
		{
			.mp_id = 1,
			.l2_cache_parity1_rdata = 0x027c,
			.l2_cache_parity2_rdata = 0x0280,
		},
	},
};

#endif
