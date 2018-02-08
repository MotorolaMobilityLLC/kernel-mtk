//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : bbm.c

	Description : API source of ISDB-T baseband module

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
#include "fci_types.h"
#include "fci_tun.h"
#include "fci_hal.h"
#include "fc8180_bb.h"
#include "fc8180_isr.h"

s32 bbm_com_reset(HANDLE handle)
{
	s32 res;

	res = fc8180_reset(handle);

	return res;
}

s32 bbm_com_probe(HANDLE handle)
{
	s32 res;

	res = fc8180_probe(handle);

	return res;
}

s32 bbm_com_init(HANDLE handle)
{
	s32 res;

	res = fc8180_init(handle);

	return res;
}

s32 bbm_com_deinit(HANDLE handle)
{
	s32 res;

	res = fc8180_deinit(handle);

	return res;
}

s32 bbm_com_read(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;

	res = bbm_read(handle, addr, data);

	return res;
}

s32 bbm_com_byte_read(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;

	res = bbm_byte_read(handle, addr, data);

	return res;
}

s32 bbm_com_word_read(HANDLE handle, u16 addr, u16 *data)
{
	s32 res;

	res = bbm_word_read(handle, addr, data);

	return res;
}

s32 bbm_com_long_read(HANDLE handle, u16 addr, u32 *data)
{
	s32 res;

	res = bbm_long_read(handle, addr, data);

	return res;
}

s32 bbm_com_bulk_read(HANDLE handle, u16 addr, u8 *data, u16 size)
{
	s32 res;

	res = bbm_bulk_read(handle, addr, data, size);

	return res;
}

s32 bbm_com_data(HANDLE handle, u16 addr, u8 *data, u32 size)
{
	s32 res;

	res = bbm_data(handle, addr, data, size);

	return res;
}

s32 bbm_com_write(HANDLE handle, u16 addr, u8 data)
{
	s32 res;

	res = bbm_write(handle, addr, data);

	return res;
}

s32 bbm_com_byte_write(HANDLE handle, u16 addr, u8 data)
{
	s32 res;

	res = bbm_byte_write(handle, addr, data);

	return res;
}

s32 bbm_com_word_write(HANDLE handle, u16 addr, u16 data)
{
	s32 res;

	res = bbm_word_write(handle, addr, data);

	return res;
}

s32 bbm_com_long_write(HANDLE handle, u16 addr, u32 data)
{
	s32 res;

	res = bbm_long_write(handle, addr, data);

	return res;
}

s32 bbm_com_bulk_write(HANDLE handle, u16 addr, u8 *data, u16 size)
{
	s32 res;

	res = bbm_bulk_write(handle, addr, data, size);

	return res;
}

s32 bbm_com_i2c_init(HANDLE handle, u32 type)
{
	s32 res;

	res = tuner_ctrl_select(handle, (enum I2C_TYPE) type);

	return res;
}

s32 bbm_com_i2c_deinit(HANDLE handle)
{
	s32 res;

	res = tuner_ctrl_deselect(handle);

	return res;
}

s32 bbm_com_tuner_read(HANDLE handle, u8 addr, u8 alen, u8 *buffer, u8 len)
{
	s32 res;

	res = tuner_i2c_read(handle, addr, alen, buffer, len);

	return res;
}

s32 bbm_com_tuner_write(HANDLE handle, u8 addr, u8 alen, u8 *buffer, u8 len)
{
	s32 res;

	res = tuner_i2c_write(handle, addr, alen, buffer, len);

	return res;
}

s32 bbm_com_tuner_set_freq(HANDLE handle, u32 freq)
{
	s32 res;

	res = tuner_set_freq(handle, freq);

	return res;
}

s32 bbm_com_tuner_select(HANDLE handle, u32 product, u32 band)
{
	s32 res;

	res = tuner_select(handle, (enum PRODUCT_TYPE) product,
					(enum BAND_TYPE) band);

	return res;
}

s32 bbm_com_tuner_deselect(HANDLE handle)
{
	s32 res;

	res = tuner_deselect(handle);

	return res;
}

s32 bbm_com_tuner_get_rssi(HANDLE handle, s32 *rssi)
{
	s32 res;

	res = tuner_get_rssi(handle, rssi);

	return res;
}

s32 bbm_com_scan_status(HANDLE handle)
{
	s32 res;

	res = fc8180_scan_status(handle);

	return res;
}

void bbm_com_isr(HANDLE handle)
{
	fc8180_isr(handle);
}

s32 bbm_com_hostif_select(HANDLE handle, u32 hostif)
{
	s32 res;

	res = bbm_hostif_select(handle, hostif);

	return res;
}

s32 bbm_com_hostif_deselect(HANDLE handle)
{
	s32 res;

	res = bbm_hostif_deselect(handle);

	return res;
}

s32 bbm_com_ts_callback_register(ulong userdata, s32 (*callback)(ulong userdata,
						u8 *data, s32 length))
{
	fc8180_ts_user_data = userdata;
	fc8180_ts_callback = callback;

	return BBM_OK;
}

s32 bbm_com_ts_callback_deregister(void)
{
	fc8180_ts_user_data = 0;
	fc8180_ts_callback = NULL;

	return BBM_OK;
}

s32 bbm_com_ac_callback_register(ulong userdata, s32 (*callback)(ulong userdata,
						u8 *data, s32 length))
{
	fc8180_ac_user_data = userdata;
	fc8180_ac_callback = callback;

	return BBM_OK;
}

s32 bbm_com_ac_callback_deregister(void)
{
	fc8180_ac_user_data = 0;
	fc8180_ac_callback = NULL;

	return BBM_OK;
}
//add dtv ---shenyong@wind-mobi.com ---20161119 end 
