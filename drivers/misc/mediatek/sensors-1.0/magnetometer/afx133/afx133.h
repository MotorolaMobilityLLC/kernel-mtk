/* include/linux/afx133.h - AFX133 compass driver
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Definitions for AFX133 compass chip.
 */
#ifndef AFX133_H
#define AFX133_H

#include <linux/ioctl.h>

//#define BYPASS_PCODE_CHECK_FOR_MTK_DRL
//#define SUPPORT_SOFTGYRO_FUNCTION

#define AFX133_BUFSIZE      6

#define AF8133J_PID 		  0x5E
#define AF9133_PID 		    0x60
#define AF5133_PID 		    0x64

#define REG_PCODE       0x00
#define REG_DATA				0x03
#define REG_MEASURE	    0x0A
#define REG_I2C_CHECK		0x10
#define REG_SW_RESET		0x11
#define REG_AVG			    0x13
#define REG_MODE				0x14
#define REG_OSR			    0x22
#define REG_BIST        0x28
#define REG_WAITING			0x32
#define REG_OTP         0x3C
#define REG_OTP1        0x3F

//========================================
#define AFX133_OFFSET_MIN  -10000
#define AFX133_OFFSET_MAX  10000
#define AFX133_SENS_MIN       500
#define AFX133_SENS_MAX      4000
//========================================
#define AFX133_MODE_IDLE    0x00
#define AFX133_MODE_SINGLE  0x01
#define AFX133_MODE_WAKE    0x02


// conversion of magnetic data (for AFX133) to uT units
#define CONVERT_M			        36
#define CONVERT_M_DIV		      1000		


#endif


