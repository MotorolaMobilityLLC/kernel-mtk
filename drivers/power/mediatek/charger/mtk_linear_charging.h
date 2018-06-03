/*
* Copyright (C) 2017 MediaTek Inc.
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

#ifndef _MTK_LINEAR_CHARGER_H
#define _MTK_LINEAR_CHARGER_H

/*****************************************************************************
 *  Linear Charging State
 ****************************************************************************/
#define CHR_CC (0x0001)
#define CHR_CV (0x0002)
#define CHR_BATFULL (0x0003)
#define CHR_ERROR (0x0004)

#define MAX_CV_CHARGING_TIME (3 * 60 * 60) /* 3 hours */

#define RECHARGE_OFFSET 150000 /* uV */
#define TOPOFF_VOLTAGE 4200000 /* uV */
#define CHG_FULL_CURRENT 150000 /* uA */

struct linear_charging_alg_data {
	int state;
	bool disable_charging;
	struct mutex ichg_access_mutex;

	unsigned int total_charging_time;
	unsigned int pre_cc_charging_time;
	unsigned int cc_charging_time;
	unsigned int cv_charging_time;
	unsigned int full_charging_time;

	int recharge_offset; /* uv */
	int topoff_voltage; /* uv */
	int chg_full_current; /* uA */
};

#endif /* End of _MTK_LINEAR_CHARGER_H */
