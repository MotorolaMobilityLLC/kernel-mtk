//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fci_tun.h

	Description : header of tuner control driver

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#ifndef __FCI_TUN_H__
#define __FCI_TUN_H__

#include "fci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum I2C_TYPE {
	FCI_HPI_TYPE        = 0
};

enum PRODUCT_TYPE {
	FC8180_TUNER        = 8180
};

enum BAND_TYPE {
	ISDBT_1_SEG_TYPE    = 0
};

extern s32 tuner_ctrl_select(HANDLE handle, enum I2C_TYPE type);
extern s32 tuner_ctrl_deselect(HANDLE handle);
extern s32 tuner_select(HANDLE handle, enum PRODUCT_TYPE product,
					enum BAND_TYPE band);
extern s32 tuner_deselect(HANDLE handle);
extern s32 tuner_i2c_read(HANDLE handle, u8 addr, u8 alen, u8 *data, u8 len);
extern s32 tuner_i2c_write(HANDLE handle, u8 addr, u8 alen, u8 *data, u8 len);
extern s32 tuner_set_freq(HANDLE handle, u32 freq);
extern s32 tuner_get_rssi(HANDLE handle, s32 *rssi);

#ifdef __cplusplus
}
#endif

#endif /* __FCI_TUN_H__ */
//add dtv ---shenyong@wind-mobi.com ---20161119 end 
