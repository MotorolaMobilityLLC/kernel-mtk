//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fc8180_i2c.c

	Description : source of I2C interface

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
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#include "fci_types.h"
#include "fc8180_regs.h"
#include "fci_oal.h"
#include "fci_hal.h"
#include "fc8180_spi.h"

#define I2C_READ   0x01 /* read command */
#define I2C_WRITE  0x02 /* write command */
#define I2C_AINC   0x04 /* address increment */

#define CHIP_ADDR           0x58

#define I2C_M_FCIRD 1
#define I2C_M_FCIWR 0
#define I2C_MAX_SEND_LENGTH 256

struct i2c_ts_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
};

struct i2c_client *fc8180_i2c;

struct i2c_driver fc8180_i2c_driver;
static DEFINE_MUTEX(fci_i2c_lock);

static int fc8180_i2c_probe(struct i2c_client *i2c_client,
	const struct i2c_device_id *id)
{
	fc8180_i2c = i2c_client;
	i2c_set_clientdata(i2c_client, NULL);
	return 0;
}

static int fc8180_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id fc8180_id[] = {
	{ "isdbt_i2c", 0 },
	{ },
};

struct i2c_driver fc8180_i2c_driver = {
	.driver = {
			.name = "isdbt_i2c",
			.owner = THIS_MODULE,
		},
	.probe    = fc8180_i2c_probe,
	.remove   = fc8180_remove,
	.id_table = fc8180_id,
};

static s32 i2c_bulkread(HANDLE handle, u8 chip, u8 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg rmsg[2];
	unsigned char i2c_data[2];

	rmsg[0].addr = chip;
	rmsg[0].flags = I2C_M_FCIWR;
	rmsg[0].len = 2;
	rmsg[0].buf = i2c_data;
	i2c_data[0] = (addr >> 8) & 0xff;
	i2c_data[1] = addr & 0xff;

	rmsg[1].addr = chip;
	rmsg[1].flags = I2C_M_FCIRD;
	rmsg[1].len = length;
	rmsg[1].buf = data;
	res = i2c_transfer(fc8180_i2c->adapter, &rmsg[0], 2);

	return BBM_OK;
}

static s32 i2c_bulkwrite(HANDLE handle, u8 chip, u8 addr, u8 *data, u16 length)
{
	int res;
	struct i2c_msg wmsg;
	unsigned char i2c_data[I2C_MAX_SEND_LENGTH];

	if ((length + 1) > I2C_MAX_SEND_LENGTH)
		return -ENODEV;

	wmsg.addr = chip;
	wmsg.flags = I2C_M_FCIWR;
	wmsg.len = length + 2;
	wmsg.buf = i2c_data;

	i2c_data[0] = (addr >> 8) & 0xff;
	i2c_data[1] = addr & 0xff;
	memcpy(&i2c_data[2], data, length);

	res = i2c_transfer(fc8180_i2c->adapter, &wmsg, 1);

	return BBM_OK;
}


static s32 i2c_dataread(HANDLE handle, u8 chip, u8 addr, u8 *data, u32 length)
{
	return i2c_bulkread(handle, chip, addr, data, length);
}

s32 fc8180_bypass_read(HANDLE handle, u8 chip, u8 addr, u8 *data, u16 length)
{
	s32 res;
	u8 bypass_addr = 0x03;
	u8 bypass_data = 1;
	u8 bypass_len  = 1;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, bypass_addr, &bypass_data,
							bypass_len);
	res |= i2c_bulkread(handle, chip, addr, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_bypass_write(HANDLE handle, u8 chip, u8 addr, u8 *data, u16 length)
{
	s32 res;
	u8 bypass_addr = 0x03;
	u8 bypass_data = 1;
	u8 bypass_len  = 1;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, bypass_addr, &bypass_data,
							bypass_len);
	res |= i2c_bulkwrite(handle, chip, addr, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_init(HANDLE handle, u16 param1, u16 param2)
{
#ifdef BBM_I2C_SPI
	fc8180_spi_init(handle, 0, 0);
#else
	/* ts_initialize(); */
#endif

	return BBM_OK;
}

s32 fc8180_i2c_byteread(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;
	u8 command = I2C_READ;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(handle, CHIP_ADDR, BBM_DATA_REG, data, 1);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_wordread(HANDLE handle, u16 addr, u16 *data)
{
	s32 res;
	u8 command = I2C_READ | I2C_AINC;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(handle, CHIP_ADDR, BBM_DATA_REG, (u8 *)data, 2);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_longread(HANDLE handle, u16 addr, u32 *data)
{
	s32 res;
	u8 command = I2C_READ | I2C_AINC;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(handle, CHIP_ADDR, BBM_DATA_REG, (u8 *)data, 4);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 res;
	u8 command = I2C_READ | I2C_AINC;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkread(handle, CHIP_ADDR, BBM_DATA_REG, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_bytewrite(HANDLE handle, u16 addr, u8 data)
{
	s32 res;
	u8 command = I2C_WRITE;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 1);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_wordwrite(HANDLE handle, u16 addr, u16 data)
{
	s32 res;
	u8 command = I2C_WRITE | I2C_AINC;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 2);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_longwrite(HANDLE handle, u16 addr, u32 data)
{
	s32 res;
	u8 command = I2C_WRITE | I2C_AINC;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_DATA_REG, (u8 *)&data, 4);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 res;
	u8 command = I2C_WRITE | I2C_AINC;

	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_DATA_REG, data, length);
	mutex_unlock(&fci_i2c_lock);

	return res;
}

s32 fc8180_i2c_dataread(HANDLE handle, u16 addr, u8 *data, u32 length)
{
	s32 res;
	u8 command = I2C_READ;

#ifdef BBM_I2C_SPI
	res = fc8180_spi_dataread(handle, addr, data, length);
#else
	mutex_lock(&fci_i2c_lock);
	res = i2c_bulkwrite(handle, CHIP_ADDR, BBM_ADDRESS_REG,
						(u8 *) &addr, 2);
	res |= i2c_bulkwrite(handle, CHIP_ADDR, BBM_COMMAND_REG, &command, 1);
	res |= i2c_dataread(handle, CHIP_ADDR, BBM_DATA_REG, data, length);
	mutex_unlock(&fci_i2c_lock);
#endif

	return res;
}

s32 fc8180_i2c_deinit(HANDLE handle)
{
#ifdef BBM_I2C_SPI
	fc8180_spi_deinit(handle);
#else
	bbm_write(handle, BBM_TS_SEL, 0x00);
	/* ts_receiver_disable(); */
#endif

	return BBM_OK;
}
//add dtv ---lenovo@lenovo.com ---20161119 end 
