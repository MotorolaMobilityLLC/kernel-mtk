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

#ifndef __CUST_CLK_BUF_H__
#define __CUST_CLK_BUF_H__
typedef enum{
	CLOCK_BUFFER_DISABLE,
	CLOCK_BUFFER_SW_CONTROL,
	CLOCK_BUFFER_HW_CONTROL
} MTK_CLK_BUF_STATUS;

#define CLK_BUF1_STATUS	CLOCK_BUFFER_HW_CONTROL
#define CLK_BUF2_STATUS	CLOCK_BUFFER_HW_CONTROL
#define CLK_BUF3_STATUS	CLOCK_BUFFER_SW_CONTROL
#define CLK_BUF4_STATUS	CLOCK_BUFFER_SW_CONTROL



#endif

