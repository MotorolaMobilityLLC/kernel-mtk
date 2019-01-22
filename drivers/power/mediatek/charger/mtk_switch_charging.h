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

#ifndef _MTK_SWITCH_CHARGER_H
#define _MTK_SWITCH_CHARGER_H

/*****************************************************************************
 *  Pulse Charging State
 ****************************************************************************/
#define  CHR_PRE                        (0x1000)
#define  CHR_CC                         (0x1001)
#define  CHR_TOP_OFF                    (0x1002)
#define  CHR_POST_FULL                  (0x1003)
#define  CHR_BATFULL                    (0x1004)
#define  CHR_ERROR                      (0x1005)
#define  CHR_HOLD						(0x1006)
#define  CHR_PEP30						(0x1007)


struct switch_charging_alg_data {
	int state;

	unsigned int total_charging_time;
	unsigned int pre_cc_charging_time;
	unsigned int cc_charging_time;
	unsigned int cv_charging_time;
	unsigned int full_charging_time;
};





#endif /* End of _MTK_SWITCH_CHARGER_H */
