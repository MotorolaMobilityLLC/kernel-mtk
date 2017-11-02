//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fc8180_isr.c

	Description : source of interrupt service routine

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

s32 (*fc8180_ac_callback)(ulong userdata, u8 *data, s32 length) = NULL;
s32 (*fc8180_ts_callback)(ulong userdata, u8 *data, s32 length) = NULL;

ulong fc8180_ac_user_data;
ulong fc8180_ts_user_data;

static u8 ts_buf[TS_BUF_SIZE];

#ifndef BBM_I2C_TSIF
static void fc8180_data(HANDLE handle, u8 buf_int_status)
{
	u16 size = 0;
	u32 rc;

	if (buf_int_status & 0x01) { /* TS */
		size = TS_BUF_SIZE / 2;
		rc = bbm_data(handle, BBM_TS_DATA, &ts_buf[0], size);
		if (BBM_OK != rc)
			return;

		if (fc8180_ts_callback)
			(*fc8180_ts_callback)(fc8180_ts_user_data,
						&ts_buf[0], size);
	}
}
#endif

void fc8180_isr(HANDLE handle)
{
#ifdef BBM_AUX_INT
	u8 aux_int_status = 0;
#endif

#ifndef BBM_I2C_TSIF
	u8 buf_int_status = 0;
	u8 int_status = 0;
	bbm_read(handle, BBM_BUF_STATUS, &buf_int_status);
	if (buf_int_status) {
		bbm_write(handle, BBM_BUF_STATUS, buf_int_status);
		fc8180_data(handle, buf_int_status);
	}

	/* Overrun */
	buf_int_status = 0;
	bbm_read(handle, BBM_BUF_STATUS, &buf_int_status);
	if (buf_int_status) {
		bbm_write(handle, BBM_BUF_STATUS, buf_int_status);
		fc8180_data(handle, buf_int_status);
	}

	bbm_read(handle, BBM_INT_STATUS, &int_status);
	if (int_status)
		bbm_write(handle, BBM_INT_STATUS, int_status);

#endif

#ifdef BBM_AUX_INT
	bbm_byte_read(handle, BBM_AUX_STATUS_CLEAR, &aux_int_status);

	if (aux_int_status) {
		bbm_byte_write(handle, BBM_AUX_STATUS_CLEAR, aux_int_status);

		fc8180_aux_int(handle, aux_int_status);
	}
#endif
}
//add dtv ---lenovo@lenovo.com ---20161119 end 
