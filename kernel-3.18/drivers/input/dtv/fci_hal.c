//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fci_hal.c

	Description : source of host interface

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
#include "fci_hal.h"
#include "fc8180_spi.h"
#include "fc8180_ppi.h"
#include "fc8180_i2c.h"
#include "fc8180_spib.h"

struct IF_PORT {
	s32 (*init)(HANDLE handle, u16 param1, u16 param2);
	s32 (*byteread)(HANDLE handle, u16 addr, u8  *data);
	s32 (*wordread)(HANDLE handle, u16 addr, u16 *data);
	s32 (*longread)(HANDLE handle, u16 addr, u32 *data);
	s32 (*bulkread)(HANDLE handle, u16 addr, u8  *data, u16 length);
	s32 (*bytewrite)(HANDLE handle, u16 addr, u8  data);
	s32 (*wordwrite)(HANDLE handle, u16 addr, u16 data);
	s32 (*longwrite)(HANDLE handle, u16 addr, u32 data);
	s32 (*bulkwrite)(HANDLE handle, u16 addr, u8 *data, u16 length);
	s32 (*dataread)(HANDLE handle, u16 addr, u8 *data, u32 length);
	s32 (*deinit)(HANDLE handle);
};

static struct IF_PORT spiif = {
	&fc8180_spi_init,
	&fc8180_spi_byteread,
	&fc8180_spi_wordread,
	&fc8180_spi_longread,
	&fc8180_spi_bulkread,
	&fc8180_spi_bytewrite,
	&fc8180_spi_wordwrite,
	&fc8180_spi_longwrite,
	&fc8180_spi_bulkwrite,
	&fc8180_spi_dataread,
	&fc8180_spi_deinit
};

static struct IF_PORT ppiif = {
	&fc8180_ppi_init,
	&fc8180_ppi_byteread,
	&fc8180_ppi_wordread,
	&fc8180_ppi_longread,
	&fc8180_ppi_bulkread,
	&fc8180_ppi_bytewrite,
	&fc8180_ppi_wordwrite,
	&fc8180_ppi_longwrite,
	&fc8180_ppi_bulkwrite,
	&fc8180_ppi_dataread,
	&fc8180_ppi_deinit
};

static struct IF_PORT i2cif = {
	&fc8180_i2c_init,
	&fc8180_i2c_byteread,
	&fc8180_i2c_wordread,
	&fc8180_i2c_longread,
	&fc8180_i2c_bulkread,
	&fc8180_i2c_bytewrite,
	&fc8180_i2c_wordwrite,
	&fc8180_i2c_longwrite,
	&fc8180_i2c_bulkwrite,
	&fc8180_i2c_dataread,
	&fc8180_i2c_deinit
};

static struct IF_PORT spibif = {
	&fc8180_spib_init,
	&fc8180_spib_byteread,
	&fc8180_spib_wordread,
	&fc8180_spib_longread,
	&fc8180_spib_bulkread,
	&fc8180_spib_bytewrite,
	&fc8180_spib_wordwrite,
	&fc8180_spib_longwrite,
	&fc8180_spib_bulkwrite,
	&fc8180_spib_dataread,
	&fc8180_spib_deinit
};

static struct IF_PORT *ifport = &i2cif;
static u8 hostif_type = BBM_I2C;

s32 bbm_hostif_select(HANDLE handle, u32 hostif)
{
	hostif_type = hostif;

	switch (hostif) {
	case BBM_SPI:
		ifport = &spiif;
		break;
	case BBM_I2C:
		ifport = &i2cif;
		break;
	case BBM_PPI:
		ifport = &ppiif;
		break;
	case BBM_SPIB:
		ifport = &spibif;
		break;
	default:
		return BBM_E_HOSTIF_SELECT;
	}

	if (ifport->init(handle, 0, 0))
		return BBM_E_HOSTIF_INIT;

	return BBM_OK;
}

s32 bbm_hostif_deselect(HANDLE handle)
{
	if (ifport->deinit(handle))
		return BBM_NOK;

	ifport = &spiif;
	hostif_type = BBM_SPI;

	return BBM_OK;
}

s32 bbm_read(HANDLE handle, u16 addr, u8 *data)
{
	if (ifport->byteread(handle, addr, data))
		return BBM_E_BB_READ;
	return BBM_OK;
}

s32 bbm_byte_read(HANDLE handle, u16 addr, u8 *data)
{
	if (ifport->byteread(handle, addr, data))
		return BBM_E_BB_READ;
	return BBM_OK;
}

s32 bbm_word_read(HANDLE handle, u16 addr, u16 *data)
{
	if (ifport->wordread(handle, addr, data))
		return BBM_E_BB_READ;
	return BBM_OK;
}

s32 bbm_long_read(HANDLE handle, u16 addr, u32 *data)
{
	if (ifport->longread(handle, addr, data))
		return BBM_E_BB_READ;
	return BBM_OK;
}

s32 bbm_bulk_read(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	if (ifport->bulkread(handle, addr, data, length))
		return BBM_E_BB_READ;
	return BBM_OK;
}

s32 bbm_write(HANDLE handle, u16 addr, u8 data)
{
	if (ifport->bytewrite(handle, addr, data))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

s32 bbm_byte_write(HANDLE handle, u16 addr, u8 data)
{
	if (ifport->bytewrite(handle, addr, data))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

s32 bbm_word_write(HANDLE handle, u16 addr, u16 data)
{
	if (ifport->wordwrite(handle, addr, data))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

s32 bbm_long_write(HANDLE handle, u16 addr, u32 data)
{
	if (ifport->longwrite(handle, addr, data))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

s32 bbm_bulk_write(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	if (ifport->bulkwrite(handle, addr, data, length))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}

s32 bbm_data(HANDLE handle, u16 addr, u8 *data, u32 length)
{
	if (ifport->dataread(handle, addr, data, length))
		return BBM_E_BB_WRITE;
	return BBM_OK;
}
//add dtv ---lenovo@lenovo.com ---20161119 end 
