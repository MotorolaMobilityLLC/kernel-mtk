//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fc8180_i2c.c

	Description : header of I2C interface

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
#ifndef __FC8180_I2C_H__
#define __FC8180_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

extern s32 fc8180_i2c_init(HANDLE handle, u16 param1, u16 param2);
extern s32 fc8180_i2c_byteread(HANDLE handle, u16 addr, u8 *data);
extern s32 fc8180_i2c_wordread(HANDLE handle, u16 addr, u16 *data);
extern s32 fc8180_i2c_longread(HANDLE handle, u16 addr, u32 *data);
extern s32 fc8180_i2c_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 fc8180_i2c_bytewrite(HANDLE handle, u16 addr, u8 data);
extern s32 fc8180_i2c_wordwrite(HANDLE handle, u16 addr, u16 data);
extern s32 fc8180_i2c_longwrite(HANDLE handle, u16 addr, u32 data);
extern s32 fc8180_i2c_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 fc8180_i2c_dataread(HANDLE handle, u16 addr, u8 *data, u32 length);
extern s32 fc8180_i2c_deinit(HANDLE handle);
extern s32 fc8180_bypass_read(HANDLE handle, u8 chip, u8 addr, u8 *data,
							u16 length);
extern s32 fc8180_bypass_write(HANDLE handle, u8 chip, u8 addr, u8 *data,
							u16 length);

#ifdef __cplusplus
}
#endif

#endif /* __FC8180_I2C_H__ */
//add dtv ---shenyong@wind-mobi.com ---20161119 end 
