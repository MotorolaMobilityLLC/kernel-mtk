/*
 * Copyright (C) 2020 MediaTek Inc.
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

#ifndef __MDP_BASE_H__
#define __MDP_BASE_H__

#define MDP_HW_CHECK

static u32 mdp_base[] = {
	0x14000000,
	0x14001000,
	0x14002000,
	0x14003000,
	0x14004000,
	0x14005000,
	0x14020000,
	0x1400d000,
	0x14007000,
	0x1400e000,
	0x1401b000,
	0x10000000,
	0x1401c000,
	0x14016000,
	0x15010000,
	0x15020000,
	0x15021000,
	0x15022000,
	0x15011000,
};

static u32 mdp_sub_base[] = {
	0x0001,
	0x1001,
	0x2001,
	0x3001,
	0x4001,
	0x5001,
	0x0003,
	0xd001,
	0x7001,
	0xe001,
	0xb002,
	0x000a,
	0xc002,
	0x6002,
	0x0004,
	0x1004,
	0x2004,
};

#endif
