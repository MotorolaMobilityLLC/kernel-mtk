//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fci_bypass.c

	Description : source of internal i2c bypass

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
#include "fc8180_regs.h"
#include "fc8180_i2c.h"

s32 fci_bypass_init(HANDLE handle, s32 speed, s32 slaveaddr)
{
	return BBM_OK;
}

s32 fci_bypass_read(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	s32 res;

	res = fc8180_bypass_read(handle, chip, addr, data, len);

	return res;
}

s32 fci_bypass_write(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	s32 res;

	res = fc8180_bypass_write(handle, chip, addr, data, len);

	return res;
}

s32 fci_bypass_deinit(HANDLE handle)
{
	return BBM_OK;
}
//add dtv ---lenovo@lenovo.com ---20161119 end 
