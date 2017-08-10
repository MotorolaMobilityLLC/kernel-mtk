/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <linux/xlog.h>
//#include <asm/system.h>

#include <linux/proc_fs.h>


#include <linux/dma-mapping.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#define PFX "OV16880_pdafotp"
#define LOG_INF(fmt, args...)   pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##args)

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);

#define USHORT             	unsigned short
#define BYTE                unsigned char
#define Sleep(ms)           mdelay(ms)

#define EEPROM              GT24C64A
#define EEPROM_READ_ID    	(0xA0)
#define I2C_SPEED           (400)  //CAT24C512 can support 1Mhz

#define MAX_OFFSET          (0xffff)
#define DATA_SIZE           (4096)

BYTE eeprom_data[DATA_SIZE] = {0};
static bool get_done = false;
static int last_size = 0;
static int last_offset = 0;

bool selective_read_eeprom(kal_uint16 addr, BYTE* data)
{
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	if(addr > MAX_OFFSET)
		return false;

	if(iReadRegI2C(pu_send_cmd, 2, (u8*)data, 1, EEPROM_READ_ID)<0)
		return false;
	return true;
}

static bool _read_eeprom(kal_uint16 addr, BYTE* data, kal_uint32 size )
{
	int i = 0;
	int offset = addr;
	for(i = 0; i < size; i++) 
	{
		if(!selective_read_eeprom(offset, &data[i]))
		{
			return false;
		}
		LOG_INF("read_eeprom 0x%x 0x%x\n",offset, data[i]);
		offset++;
	}
	get_done = true;
	last_size = size;
	last_offset = addr;
	return true;
}
bool read_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size)
{
	addr = 0x0801;
	size = 496+876;
	LOG_INF("read ov16880 eeprom, addr = %d; size = %d\n", addr, size);
	if(!get_done || last_size != size || last_offset != addr) 
	{
		if(!_read_eeprom(addr, eeprom_data, size))
		{
			get_done = 0;
			last_size = 0;
			last_offset = 0;
			return false;
		}else{
			LOG_INF("Eeprom read completed \n");
		}
	}else{
		LOG_INF("ERROR: read failed in entry 1\n");
	}
	memcpy(data, eeprom_data, size);
  return true;
}






