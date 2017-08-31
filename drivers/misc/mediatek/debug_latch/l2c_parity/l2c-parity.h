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

#ifndef __L2C_PARITY_H__
#define __L2C_PARITY_H__

struct cfg_l2_parity_little_cluster {
	unsigned int mp_id;
	unsigned int l2_cache_parity1_rdata;
	unsigned int l2_cache_parity2_rdata;
};

struct plat_cfg_l2_parity {
	unsigned int nr_little_cluster;
	struct cfg_l2_parity_little_cluster cluster[];
};

#endif
