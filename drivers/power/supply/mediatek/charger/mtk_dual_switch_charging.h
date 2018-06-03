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

#ifndef _MTK_DUAL_SWITCH_CHARGER_H
#define _MTK_DUAL_SWITCH_CHARGER_H

#define CHR_CC		(0x0001)
#define CHR_TUNING	(0x0002)
#define CHR_POSTCC	(0x0003)
#define CHR_BATFULL	(0x0004)
#define CHR_ERROR	(0x0005)

struct dual_switch_charging_alg_data {
	int state;
	bool disable_charging;
	struct mutex ichg_aicr_access_mutex;

	unsigned int total_charging_time;
	unsigned int pre_cc_charging_time;
	unsigned int cc_charging_time;
	unsigned int cv_charging_time;
	unsigned int full_charging_time;
};

#endif /* End of _MTK_DUAL_SWITCH_CHARGER_H */
