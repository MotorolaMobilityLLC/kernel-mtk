//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fc8180_ppi.c

	Description : source of EBI2LCD interface

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
#include "fci_oal.h"

#define BBM_BASE_ADDR       0x00

#define PPI_BMODE           0x00
#define PPI_WMODE           0x10
#define PPI_LMODE           0x20
#define PPI_READ            0x40
#define PPI_WRITE           0x00
#define PPI_AINC            0x80

#define FC8180_PPI_REG      (*(volatile u8 *) (BBM_BASE_ADDR))

s32 fc8180_ppi_init(HANDLE handle, u16 param1, u16 param2)
{
	OAL_CREATE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_byteread(HANDLE handle, u16 addr, u8 *data)
{
	u32 length = 1;
	u8 cmd = PPI_READ | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4;
	FC8180_PPI_REG = addr >> 8;
	FC8180_PPI_REG = addr >> 12;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;

	*data = (FC8180_PPI_REG & 0x0f) << 4;
	*data |= (FC8180_PPI_REG & 0x0f);
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_wordread(HANDLE handle, u16 addr, u16 *data)
{
	u32 length = 2;
	u8 cmd = PPI_AINC | PPI_READ | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4;
	FC8180_PPI_REG = addr >> 8;
	FC8180_PPI_REG = addr >> 12;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;

	*data = (FC8180_PPI_REG & 0x0f) << 4;
	*data |= (FC8180_PPI_REG & 0x0f);
	*data |= (FC8180_PPI_REG & 0x0f) << 12;
	*data |= (FC8180_PPI_REG & 0x0f) << 8;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_longread(HANDLE handle, u16 addr, u32 *data)
{
	u32 length = 4;
	u8 cmd = PPI_AINC | PPI_READ | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4;
	FC8180_PPI_REG = addr >> 8;
	FC8180_PPI_REG = addr >> 12;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;
	*data = (FC8180_PPI_REG & 0x0f) << 4;
	*data |= (FC8180_PPI_REG & 0x0f);
	*data |= (FC8180_PPI_REG & 0x0f) << 12;
	*data |= (FC8180_PPI_REG & 0x0f) << 8;
	*data |= (FC8180_PPI_REG & 0x0f) << 20;
	*data |= (FC8180_PPI_REG & 0x0f) << 16;
	*data |= (FC8180_PPI_REG & 0x0f) << 28;
	*data |= (FC8180_PPI_REG & 0x0f) << 24;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 i;
	u8 cmd = PPI_AINC | PPI_READ | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4;
	FC8180_PPI_REG = addr >> 8;
	FC8180_PPI_REG = addr >> 12;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;

	for (i = 0; i < length; i++) {
		data[i] = (FC8180_PPI_REG & 0x0f) << 4;
		data[i] |= (FC8180_PPI_REG & 0x0f);
	}
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_bytewrite(HANDLE handle, u16 addr, u8 data)
{
	u32 length = 1;
	u8 cmd = PPI_WRITE | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4;
	FC8180_PPI_REG = addr >> 8;
	FC8180_PPI_REG = addr >> 12;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;
	FC8180_PPI_REG = (data >> 4) & 0x0f;
	FC8180_PPI_REG = data & 0x0f;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_wordwrite(HANDLE handle, u16 addr, u16 data)
{
	u32 length = 2;
	u8 cmd = PPI_AINC | PPI_WRITE | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4;
	FC8180_PPI_REG = addr >> 8;
	FC8180_PPI_REG = addr >> 12;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;
	FC8180_PPI_REG = (data >> 4) & 0x0f;
	FC8180_PPI_REG = data & 0x0f;
	FC8180_PPI_REG = (data >> 12) & 0x0f;
	FC8180_PPI_REG = (data >> 8) & 0x0f;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_longwrite(HANDLE handle, u16 addr, u32 data)
{
	u32 length = 4;
	u8 cmd = PPI_AINC | PPI_WRITE | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4;
	FC8180_PPI_REG = addr >> 8;
	FC8180_PPI_REG = addr >> 12;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;
	FC8180_PPI_REG = (data >> 4) & 0x0f;
	FC8180_PPI_REG = data & 0x0f;
	FC8180_PPI_REG = (data >> 12) & 0x0f;
	FC8180_PPI_REG = (data >> 8) & 0x0f;
	FC8180_PPI_REG = (data >> 20) & 0x0f;
	FC8180_PPI_REG = (data >> 16) & 0x0f;
	FC8180_PPI_REG = (data >> 28) & 0x0f;
	FC8180_PPI_REG = (data >> 24) & 0x0f;
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 i;
	u8 cmd = PPI_AINC | PPI_WRITE | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4;
	FC8180_PPI_REG = addr >> 8;
	FC8180_PPI_REG = addr >> 12;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;

	for (i = 0; i < length; i++) {
		FC8180_PPI_REG = (data[i] >> 4) & 0x0f;
		FC8180_PPI_REG = data[i] & 0x0f;
	}
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_dataread(HANDLE handle, u16 addr, u8 *data, u32 length)
{
	s32 i;
	u8 cmd = PPI_READ | ((length & 0x0f0000) >> 16);

	OAL_OBTAIN_SEMAPHORE();
	FC8180_PPI_REG = addr;
	FC8180_PPI_REG = addr >> 4 ;
	FC8180_PPI_REG = addr >> 8 ;
	FC8180_PPI_REG = addr >> 12 ;
	FC8180_PPI_REG = cmd >> 4;
	FC8180_PPI_REG = cmd;
	FC8180_PPI_REG = length >> 12;
	FC8180_PPI_REG = length >> 8;
	FC8180_PPI_REG = length >> 4;
	FC8180_PPI_REG = length;

	for (i = 0; i < length; i++) {
		data[i] = (FC8180_PPI_REG & 0x0f) << 4;
		data[i] |= (FC8180_PPI_REG & 0x0f);
	}
	OAL_RELEASE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_ppi_deinit(HANDLE handle)
{
	OAL_DELETE_SEMAPHORE();

	return BBM_OK;
}
//add dtv ---shenyong@wind-mobi.com ---20161119 end 
