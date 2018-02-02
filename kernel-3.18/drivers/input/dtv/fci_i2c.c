//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fci_i2c.c

	Description : source of internal i2c driver

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
#include "fci_oal.h"
#include "fc8180_regs.h"
#include "fci_hal.h"

#define I2CSTAT_TIP         0x02 /* Tip bit */
#define I2CSTAT_NACK        0x80 /* Nack bit */

#define I2C_TIMEOUT         100

#define I2C_CR_STA          0x80
#define I2C_CR_STO          0x40
#define I2C_CR_RD           0x20
#define I2C_CR_WR           0x10
#define I2C_CR_NACK         0x08
#define I2C_CR_IACK         0x01

#define I2C_WRITE           0
#define I2C_READ            1

#define I2C_OK              0
#define I2C_NOK             1
#define I2C_NACK            2
#define I2C_NOK_LA          3 /* Lost arbitration */
#define I2C_NOK_TOUT        4 /* time out */

#define  FC8180_FREQ_XTAL  BBM_XTAL_FREQ

/*static OAL_SEMAPHORE hBbmMutex;*/

static s32 WaitForXfer(HANDLE handle)
{
	s32 i;
	s32 res = I2C_OK;
	u8 status;

	i = I2C_TIMEOUT * 20000;
	/* wait for transfer complete */
	do {
		bbm_read(handle, BBM_I2C_SR, &status);
		i--;
	} while ((i > 0) && (status & I2CSTAT_TIP));

	/* check time out or nack */
	if (status & I2CSTAT_TIP) {
		res = I2C_NOK_TOUT;
	} else {
		bbm_read(handle, BBM_I2C_SR, &status);
		if (status & I2CSTAT_NACK)
			res = I2C_NACK;
		else
			res = I2C_OK;
	}

	return res;
}

static s32 fci_i2c_transfer(HANDLE handle, u8 cmd_type, u8 chip, u8 addr[],
					u8 addr_len, u8 data[], u8 data_len)
{
	s32 i;
	s32 result = I2C_OK;

	switch (cmd_type) {
	case I2C_WRITE:
		bbm_write(handle, BBM_I2C_TXR, chip | cmd_type);
		bbm_write(handle, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR /*0x90*/);
		result = WaitForXfer(handle);
		if (result != I2C_OK)
			return result;

		if (addr && addr_len) {
			i = 0;
			while ((i < addr_len) && (result == I2C_OK)) {
				bbm_write(handle, BBM_I2C_TXR, addr[i]);
				bbm_write(handle, BBM_I2C_CR, I2C_CR_WR);
				result = WaitForXfer(handle);
				if (result != I2C_OK)
					return result;
				i++;
			}
		}

		i = 0;
		while ((i < data_len) && (result == I2C_OK)) {
			bbm_write(handle, BBM_I2C_TXR, data[i]);
			bbm_write(handle, BBM_I2C_CR, I2C_CR_WR /*0x10*/);
			result = WaitForXfer(handle);
			if (result != I2C_OK)
				return result;
			i++;
		}

		bbm_write(handle, BBM_I2C_CR, I2C_CR_STO /*0x40*/);
		result = WaitForXfer(handle);
		if (result != I2C_OK)
			return result;
		break;
	case I2C_READ:
		if (addr && addr_len) {
			bbm_write(handle, BBM_I2C_TXR, chip | I2C_WRITE);
			/* send start */
			bbm_write(handle, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR);
			result = WaitForXfer(handle);
			if (result != I2C_OK)
				return result;

			i = 0;
			while ((i < addr_len) && (result == I2C_OK)) {
				bbm_write(handle, BBM_I2C_TXR, addr[i]);
				bbm_write(handle, BBM_I2C_CR, I2C_CR_WR);
				result = WaitForXfer(handle);
				if (result != I2C_OK)
					return result;
				i++;
			}
		}

		bbm_write(handle, BBM_I2C_TXR, chip | I2C_READ);
		/* resend start */
		bbm_write(handle, BBM_I2C_CR, I2C_CR_STA | I2C_CR_WR);
		result = WaitForXfer(handle);
		if (result != I2C_OK)
			return result;

		i = 0;
		while ((i < data_len) && (result == I2C_OK)) {
			if (i == data_len - 1) {
				/* No Ack Read */
				bbm_write(handle, BBM_I2C_CR, I2C_CR_RD |
						I2C_CR_NACK);
				result = WaitForXfer(handle);
				if ((result != I2C_NACK) && (result !=
							I2C_OK)) {
					print_log(handle, "NACK4-0[%02x]\n",
								result);
					return result;
				}
			} else {
				/* Ack Read */
				bbm_write(handle, BBM_I2C_CR, I2C_CR_RD);
				result = WaitForXfer(handle);
				if (result != I2C_OK) {
					print_log(handle, "NACK4-1[%02x]\n",
								result);
					return result;
				}
			}
			bbm_read(handle, BBM_I2C_RXR, &data[i]);
			i++;
		}

		/* send stop */
		bbm_write(handle, BBM_I2C_CR, I2C_CR_STO /*0x40*/);
		result = WaitForXfer(handle);
		if ((result != I2C_NACK) && (result != I2C_OK)) {
			print_log(handle, "NACK5[%02X]\n", result);
			return result;
		}
		break;
	default:
		return I2C_NOK;
	}

	return I2C_OK;
}

s32 fci_i2c_init(HANDLE handle, s32 speed, s32 slaveaddr)
{
	u16 r = FC8180_FREQ_XTAL % (5 * speed);
	u16 pr = (FC8180_FREQ_XTAL - r) / (5 * speed) - 1;

	if (((5 * speed) >> 1) <= r)
		pr++;

	/*OAL_CREATE_SEMAPHORE(&hBbmMutex, "int_i2c", 1, OAL_FIFO);*/

	bbm_word_write(handle, BBM_I2C_PR_L, pr);
	bbm_write(handle, BBM_I2C_CTR, 0xc0);

	print_log(handle, "Internal I2C Pre-scale: 0x%02x\n", pr);

	return BBM_OK;
}

s32 fci_i2c_read(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	s32 ret;

	/*OAL_OBTAIN_SEMAPHORE(&hBbmMutex, OAL_SUSPEND);*/
	ret = fci_i2c_transfer(handle, I2C_READ, chip << 1, &addr, alen, data,
									len);
	/*OAL_RELEASE_SEMAPHORE(&hBbmMutex);*/

	if (ret != I2C_OK) {
		print_log(handle, "fci_i2c_read() result=%d, addr = %x, "
					"data=%x\n", ret, addr, *data);
		return ret;
	}

	return ret;
}

s32 fci_i2c_write(HANDLE handle, u8 chip, u8 addr, u8 alen, u8 *data, u8 len)
{
	s32 ret;
	u8 *paddr = &addr;

	/*OAL_OBTAIN_SEMAPHORE(&hBbmMutex, OAL_SUSPEND);*/
	ret = fci_i2c_transfer(handle, I2C_WRITE, chip << 1, paddr, alen, data,
									len);
	/*OAL_RELEASE_SEMAPHORE(&hBbmMutex);*/

	if (ret != I2C_OK) {
		print_log(handle, "fci_i2c_write() result=%d, addr= %x, "
					"data=%x\n", ret, addr, *data);
	}

	return ret;
}

s32 fci_i2c_deinit(HANDLE handle)
{
	bbm_write(handle, BBM_I2C_CTR, 0x00);
	/*OAL_DELETE_SEMAPHORE(&hBbmMutex);*/
	return BBM_OK;
}
//add dtv ---lenovo@lenovo.com ---20161119 end 
