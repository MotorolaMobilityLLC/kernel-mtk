//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : bbm.h

	Description : API header of 1-SEG baseband module

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
#ifndef __BBM_H__
#define __BBM_H__

#include "fci_types.h"
#include "fc8180_regs.h"
#include "fci_tun.h"
#include "fci_oal.h"

#define DRIVER_VER "S1.10.0"

#ifdef __cplusplus
extern "C" {
#endif

extern s32 bbm_com_reset(HANDLE handle);
extern s32 bbm_com_probe(HANDLE handle);
extern s32 bbm_com_init(HANDLE handle);
extern s32 bbm_com_deinit(HANDLE handle);
extern s32 bbm_com_read(HANDLE handle, u16 addr, u8 *data);
extern s32 bbm_com_byte_read(HANDLE handle, u16 addr, u8 *data);
extern s32 bbm_com_word_read(HANDLE handle, u16 addr, u16 *data);
extern s32 bbm_com_long_read(HANDLE handle, u16 addr, u32 *data);
extern s32 bbm_com_bulk_read(HANDLE handle, u16 addr, u8 *data, u16 size);
extern s32 bbm_com_data(HANDLE handle, u16 addr, u8 *data, u32 size);
extern s32 bbm_com_write(HANDLE handle, u16 addr, u8 data);
extern s32 bbm_com_byte_write(HANDLE handle, u16 addr, u8 data);
extern s32 bbm_com_word_write(HANDLE handle, u16 addr, u16 data);
extern s32 bbm_com_long_write(HANDLE handle, u16 addr, u32 data);
extern s32 bbm_com_bulk_write(HANDLE handle, u16 addr, u8 *data, u16 size);
extern s32 bbm_com_i2c_init(HANDLE handle, u32 type);
extern s32 bbm_com_i2c_deinit(HANDLE handle);
extern s32 bbm_com_tuner_select(HANDLE handle, u32 product, u32 band);
extern s32 bbm_com_tuner_deselect(HANDLE handle);
extern s32 bbm_com_tuner_read(HANDLE handle, u8 addr, u8 alen, u8 *buffer,
								u8 len);
extern s32 bbm_com_tuner_write(HANDLE handle, u8 addr, u8 alen, u8 *buffer,
								u8 len);
extern s32 bbm_com_tuner_set_freq(HANDLE handle, u32 freq);
extern s32 bbm_com_tuner_get_rssi(HANDLE handle, s32 *rssi);
extern s32 bbm_com_scan_status(HANDLE handle);
extern s32 bbm_com_hostif_select(HANDLE handle, u32 hostif);
extern s32 bbm_com_hostif_deselect(HANDLE handle);
extern s32 bbm_com_ts_callback_register(ulong userdata, s32 (*callback)(
				ulong userdata, u8 *data, s32 length));
extern s32 bbm_com_ts_callback_deregister(void);
extern s32 bbm_com_ac_callback_register(ulong userdata, s32 (*callback)(
				ulong userdata, u8 *data, s32 length));
extern s32 bbm_com_ac_callback_deregister(void);
extern void bbm_com_isr(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /* __BBM_H__ */
//add dtv ---lenovo@lenovo.com ---20161119 end 
