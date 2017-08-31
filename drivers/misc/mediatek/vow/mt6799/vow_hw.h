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

#ifndef __VOW_HW_H__
#define __VOW_HW_H__

/* which the GPIO NUM is corresponed to EINT0~15  */
enum VOW_EINT_NUM {
	VOW_EINT_NUM_0 = 0,
	VOW_EINT_NUM_1 = 1,
	VOW_EINT_NUM_2 = 2,
	VOW_EINT_NUM_3 = 3,
	VOW_EINT_NUM_4 = 4,
	VOW_EINT_NUM_5 = 5,
	VOW_EINT_NUM_6 = 6,
	VOW_EINT_NUM_7 = 7,
	VOW_EINT_NUM_8 = 8,
	VOW_EINT_NUM_9 = 9,
	VOW_EINT_NUM_10 = 10,
	VOW_EINT_NUM_11 = 11,
	VOW_EINT_NUM_12 = 12,
	VOW_EINT_NUM_13 = 13,
	VOW_EINT_NUM_14 = 14,
	VOW_EINT_NUM_15 = 15,
	VOW_EINT_SUM = 16
};

unsigned int vow_check_scp_status(void);
unsigned int vow_query_eint_num(unsigned int eint_gpio);

#endif /*__VOW_HW_H__ */
