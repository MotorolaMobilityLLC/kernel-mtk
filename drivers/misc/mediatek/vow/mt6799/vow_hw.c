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


/*****************************************************************************
 * Header Files
*****************************************************************************/
#include "vow_hw.h"
#include "scp_ipi.h"

/*****************************************************************************
* Function
****************************************************************************/
unsigned int vow_check_scp_status(void)
{
	return is_scp_ready(SCP_B_ID);
}

unsigned int vow_query_eint_num(unsigned int eint_gpio)
{
	unsigned int eint_num;

	eint_num = 0xFF;
	switch (eint_gpio) {
	case VOW_EINT_NUM_0:
		eint_num = 0;
		break;
	case VOW_EINT_NUM_1:
		eint_num = 1;
		break;
	case VOW_EINT_NUM_2:
		eint_num = 2;
		break;
	case VOW_EINT_NUM_3:
		eint_num = 3;
		break;
	case VOW_EINT_NUM_4:
		eint_num = 4;
		break;
	case VOW_EINT_NUM_5:
		eint_num = 5;
		break;
	case VOW_EINT_NUM_6:
		eint_num = 6;
		break;
	case VOW_EINT_NUM_7:
		eint_num = 7;
		break;
	case VOW_EINT_NUM_8:
		eint_num = 8;
		break;
	case VOW_EINT_NUM_9:
		eint_num = 9;
		break;
	case VOW_EINT_NUM_10:
		eint_num = 10;
		break;
	case VOW_EINT_NUM_11:
		eint_num = 11;
		break;
	case VOW_EINT_NUM_12:
		eint_num = 12;
		break;
	case VOW_EINT_NUM_13:
		eint_num = 13;
		break;
	case VOW_EINT_NUM_14:
		eint_num = 14;
		break;
	case VOW_EINT_NUM_15:
		eint_num = 15;
		break;
	default:
		eint_num = 0xFF;
		break;
	}
	return eint_num;
}
