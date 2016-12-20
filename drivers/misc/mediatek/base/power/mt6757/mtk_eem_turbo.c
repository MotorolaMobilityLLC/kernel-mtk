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
#define KERNEL44

#ifdef __KERNEL__
#ifdef KERNEL44
	#include "mtk_defptp.h"
#else
	#include "mt_defptp.h"
#endif
#endif
#include "mtk_eem_turbo.h"

#if defined(EEM_ENABLE_VTURBO)

unsigned int tOE1TurboTbl[NUM_ELM_SRAM] = {
0x1F, 0x168, 0x8, 0x0, 0xF, 0x0, 0x48, 0x48
};/* 2340 */

unsigned int tOE2_6355TurboTbl[NUM_ELM_SRAM] = {
0x1F, 0x180, 0x8, 0x0, 0xF, 0x0, 0x55, 0x67
};/* 2496 */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
unsigned int tKBP_6355TurboTbl[NUM_ELM_SRAM] = {
0x1F, 0x190, 0x8, 0x0, 0xF, 0x0, 0x60, 0x72
};/* 2600 */
#endif

unsigned int *tbl;
unsigned int *get_turbo(unsigned int segCode)
{

	switch (segCode) {
	case 0:
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
		/* OE2+6355 */
		tbl = tOE2_6355TurboTbl;
#else
		/* Olympus */
		tbl = tOE1TurboTbl;
#endif
		break;
	case 1:
		/* Kibo */
		tbl = tOE2_6355TurboTbl;
		break;
	case 3:
	case 7:
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
		/* Kibo+ */
		tbl = tKBP_6355TurboTbl;
#else
		/* Kibo */
		tbl = tOE2_6355TurboTbl;
#endif
		break;
	}

	return tbl;
}
#endif
