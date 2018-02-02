//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fci_tun.c

	Description : source of tuner control driver

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
#include "fci_hal.h"
#include "fci_tun.h"
#include "fci_hpi.h"
#include "fc8180_bb.h"
#include "fc8180_tun.h"

#define FC8180_TUNER_ADDR       0xaa

struct I2C_DRV {
	s32 (*init)(HANDLE handle, s32 speed, s32 slaveaddr);
	s32 (*read)(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data,
							u8 len);
	s32 (*write)(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data,
							u8 len);
	s32 (*deinit)(HANDLE handle);
};

static struct I2C_DRV fcihpi = {
	&fci_hpi_init,
	&fci_hpi_read,
	&fci_hpi_write,
	&fci_hpi_deinit
};

struct TUNER_DRV {
	s32 (*init)(HANDLE handle, enum BAND_TYPE band);
	s32 (*set_freq)(HANDLE handle, u32 freq);
	s32 (*get_rssi)(HANDLE handle, s32 *rssi);
	s32 (*deinit)(HANDLE handle);
};

static struct TUNER_DRV fc8180_tuner = {
	&fc8180_tuner_init,
	&fc8180_set_freq,
	&fc8180_get_rssi,
	&fc8180_tuner_deinit
};

static u8 tuner_addr = FC8180_TUNER_ADDR;
static enum BAND_TYPE tuner_band = ISDBT_1_SEG_TYPE;
static enum I2C_TYPE tuner_i2c = FCI_HPI_TYPE;
static struct I2C_DRV *tuner_ctrl = &fcihpi;
static struct TUNER_DRV *tuner = &fc8180_tuner;

s32 tuner_ctrl_select(HANDLE handle, enum I2C_TYPE type)
{
	switch (type) {
	case FCI_HPI_TYPE:
		tuner_ctrl = &fcihpi;
		break;
	default:
		return BBM_E_TN_CTRL_SELECT;
	}

	if (tuner_ctrl->init(handle, 400, 0))
		return BBM_E_TN_CTRL_INIT;

	tuner_i2c = type;

	return BBM_OK;
}

s32 tuner_ctrl_deselect(HANDLE handle)
{
	if (tuner_ctrl == NULL)
		return BBM_E_TN_CTRL_SELECT;

	tuner_ctrl->deinit(handle);

	tuner_i2c = FCI_HPI_TYPE;
	tuner_ctrl = &fcihpi;

	return BBM_OK;
}

s32 tuner_i2c_read(HANDLE handle, u8 addr, u8 alen, u8 *data, u8 len)
{
	if (tuner_ctrl == NULL)
		return BBM_E_TN_CTRL_SELECT;

	if (tuner_ctrl->read(handle, tuner_addr, addr, alen, data, len))
		return BBM_E_TN_READ;

	return BBM_OK;
}

s32 tuner_i2c_write(HANDLE handle, u8 addr, u8 alen, u8 *data, u8 len)
{
	if (tuner_ctrl == NULL)
		return BBM_E_TN_CTRL_SELECT;

	if (tuner_ctrl->write(handle, tuner_addr, addr, alen, data, len))
		return BBM_E_TN_WRITE;

	return BBM_OK;
}

s32 tuner_set_freq(HANDLE handle, u32 freq)
{
#ifdef BBM_I2C_TSIF
	u8 tsif_en = 0;
	u8 buf_en = 0;
#endif
	if (tuner == NULL)
		return BBM_E_TN_SELECT;

#if (BBM_BAND_WIDTH == 8)
	freq -= 460;
#else
	freq -= 380;
#endif

#ifdef BBM_I2C_TSIF
	bbm_byte_read(handle, BBM_TS_SEL, &tsif_en);
	bbm_byte_write(handle, BBM_TS_SEL, tsif_en & 0x7f);

	bbm_byte_read(handle, BBM_BUF_ENABLE, &buf_en);
	bbm_byte_write(handle, BBM_BUF_ENABLE, 0x00);
#endif

	if (tuner->set_freq(handle, freq))
		return BBM_E_TN_SET_FREQ;

	fc8180_reset(handle);

#ifdef BBM_I2C_TSIF
	bbm_byte_write(handle, BBM_TS_SEL, tsif_en);
	bbm_byte_write(handle, BBM_BUF_ENABLE, buf_en);
#endif

	return BBM_OK;
}

s32 tuner_select(HANDLE handle, enum PRODUCT_TYPE product, enum BAND_TYPE band)
{
	switch (product) {
	case FC8180_TUNER:
		tuner = &fc8180_tuner;
		tuner_addr = FC8180_TUNER_ADDR;
		tuner_band = band;
		break;
	default:
		return BBM_E_TN_SELECT;
	}

	if (tuner == NULL)
		return BBM_E_TN_SELECT;

	if (tuner->init(handle, tuner_band))
		return BBM_E_TN_INIT;

	return BBM_OK;
}

s32 tuner_deselect(HANDLE handle)
{
	if (tuner == NULL)
		return BBM_E_TN_SELECT;

	if (tuner->deinit(handle))
		return BBM_NOK;

	return BBM_OK;
}

s32 tuner_get_rssi(HANDLE handle, s32 *rssi)
{
	if (tuner == NULL)
		return BBM_E_TN_SELECT;

	if (tuner->get_rssi(handle, rssi))
		return BBM_E_TN_RSSI;

	return BBM_OK;
}
//add dtv ---lenovo@lenovo.com ---20161119 end 
