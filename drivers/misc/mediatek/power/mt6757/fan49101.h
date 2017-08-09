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
*   fan49101.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   fan49101 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _fan49101_SW_H_
#define _fan49101_SW_H_

/* Regs */
#define FAN49101_SOFTRESET		0x00
#define FAN49101_VOUT		    0x01
#define FAN49101_CONTROL		0x02
/* IC Type */
#define FAN49101_ID1			0x40
/* DIE ID */
#define FAN49101_ID2			0x41

enum fan49101_vendor {
	FAN49101_VENDOR_FAIRCHILD = 0x83,
};

/* Device ID */
enum {
	FAN49101_DEVICE_ID = 0,
};


extern int is_fan49101_exist(void);
extern void fan49101_dump_register(void);
extern unsigned int fan49101_read_interface(unsigned char RegNum, unsigned char *val,
					    unsigned char MASK, unsigned char SHIFT);
extern unsigned int fan49101_config_interface(unsigned char RegNum, unsigned char val,
					      unsigned char MASK, unsigned char SHIFT);

#endif				/* _fan49101_SW_H_ */
