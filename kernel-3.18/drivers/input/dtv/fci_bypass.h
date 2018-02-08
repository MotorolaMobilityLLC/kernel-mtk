//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fci_bypass.h

	Description : header of internal i2c bypass

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
#ifndef __FCI_BYPASS_H__
#define __FCI_BYPASS_H__

#include "fci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern s32 fci_bypass_init(HANDLE handle, s32 speed, s32 slaveaddr);
extern s32 fci_bypass_read(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data,
								u8 len);
extern s32 fci_bypass_write(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data,
								u8 len);
extern s32 fci_bypass_deinit(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /* __FCI_BYPASS_H__ */
//add dtv ---shenyong@wind-mobi.com ---20161119 end 