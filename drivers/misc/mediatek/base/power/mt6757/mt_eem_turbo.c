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

#include <linux/kernel.h>
#include <linux/module.h>
#include "mt_eem_turbo.h"
#if defined(EEM_ENABLE_VTURBO)
unsigned int tFyTbl[8] =    {0x1F, 0x168, 0x8, 0x0, 0xF, 0x0, 0x48, 0x44};/* 2340 */
unsigned int tTurboTbl[8] = {0x1F, 0x180, 0x8, 0x0, 0xF, 0x0, 0x48, 0x48};/* 2496 */

unsigned int *tbl;
unsigned int *get_turbo(unsigned int binLevel, unsigned int binLevelEng)
{
	if ((1 == binLevel) || (3 == binLevel)) {
		tbl = tFyTbl;
	} else if ((2 == binLevel) || (4 == binLevel)) {
		tbl = tTurboTbl;
	} else {
		if ((2 == ((binLevelEng >> 4) & 0x07)) || (2 == ((binLevelEng >> 10) & 0x07)))
			tbl = tFyTbl;
		else
			tbl = tTurboTbl;
	}
	return tbl;
}
#endif
