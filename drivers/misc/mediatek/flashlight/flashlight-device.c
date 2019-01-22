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

#include "flashlight.h"

#if defined(mt6757)
	#if defined(evb6757p_dm_64) || defined(k57pv1_dm_64) || defined(k57pv1_64_baymo)
	const struct flashlight_device_id flashlight_id[] = {
		/* {"NAME", TYPE, CT, PART} */
		{"flashlights-rt5081", 0, 0, 0},
		{"flashlights-rt5081", 0, 1, 0},
		{"flashlights-none", 1, 0, 0},
		{"flashlights-none", 1, 1, 0},
		{"flashlights-none", 0, 0, 1},
		{"flashlights-none", 0, 1, 1},
		{"flashlights-none", 1, 0, 1},
		{"flashlights-none", 1, 1, 1},
	};
	#else
	const struct flashlight_device_id flashlight_id[] = {
		/* {"NAME", TYPE, CT, PART} */
		{"flashlights-lm3643", 0, 0, 0},
		{"flashlights-lm3643", 0, 1, 0},
		{"flashlights-none", 1, 0, 0},
		{"flashlights-none", 1, 1, 0},
		{"flashlights-none", 0, 0, 1},
		{"flashlights-none", 0, 1, 1},
		{"flashlights-none", 1, 0, 1},
		{"flashlights-none", 1, 1, 1},
	};
	#endif
#elif defined(mt6799)
const struct flashlight_device_id flashlight_id[] = {
	/* {"NAME", TYPE, CT, PART} */
	{"flashlights-mt6336", 0, 0, 0},
	{"flashlights-mt6336", 0, 1, 0},
	{"flashlights-none", 1, 0, 0},
	{"flashlights-none", 1, 1, 0},
	{"flashlights-none", 0, 0, 1},
	{"flashlights-none", 0, 1, 1},
	{"flashlights-none", 1, 0, 1},
	{"flashlights-none", 1, 1, 1},
};
#else
const struct flashlight_device_id flashlight_id[] = {
	/* {"NAME", TYPE, CT, PART} */
	{"flashlights-none", 0, 0, 0},
	{"flashlights-none", 0, 1, 0},
	{"flashlights-none", 1, 0, 0},
	{"flashlights-none", 1, 1, 0},
	{"flashlights-none", 0, 0, 1},
	{"flashlights-none", 0, 1, 1},
	{"flashlights-none", 1, 0, 1},
	{"flashlights-none", 1, 1, 1},
};
#endif

