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

/*****************************************************************************
*
* Filename:
* ---------
*   fan53555.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   fan53555 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _fan53555_SW_H_
#define _fan53555_SW_H_

#define fan53555_REG_NUM 6

extern int is_fan53555_exist(void);
extern int is_fan53555_sw_ready(void);
extern void fan53555_dump_register(void);
extern unsigned int fan53555_read_interface(unsigned char RegNum, unsigned char *val,
					    unsigned char MASK, unsigned char SHIFT);
extern unsigned int fan53555_config_interface(unsigned char RegNum, unsigned char val,
					      unsigned char MASK, unsigned char SHIFT);
extern int fan53555_vosel(unsigned long val);
extern unsigned int fan53555_read_byte(unsigned char cmd, unsigned char *returnData);
extern int is_fan53555_sw_ready(void);
extern int get_fan53555_i2c_ch_num(void);

#endif				/* _fan53555_SW_H_ */
