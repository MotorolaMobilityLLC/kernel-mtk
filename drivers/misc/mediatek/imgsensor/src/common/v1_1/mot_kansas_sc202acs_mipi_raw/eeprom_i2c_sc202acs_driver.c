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
#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include "cam_cal.h"
#include "cam_cal_define.h"
//#include "cam_cal_list.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#include "eeprom_i2c_sc202acs_driver.h"

static struct i2c_client *g_pstI2CclientG;

#define SC202ACS_OTP_DEBUG  1
#define SC202ACS_I2C_ID     0x6c /*0x20*/

#define SC202ACS_OTP_SIZE 32
#define SC202ACS_OTP_HEAD 4

unsigned char g_otpMemoryData[SC202ACS_OTP_SIZE+SC202ACS_OTP_HEAD] = {0};

static u16 read_cmos_sensor(u16 addr)
{
	u16 get_byte = 0;
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, SC202ACS_I2C_ID);

	return get_byte;
}

static void write_cmos_sensor(u16 addr, u16 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8),
		(char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, SC202ACS_I2C_ID);
}

static int sc202acs_sensor_otp_init(int section){
	int delay=0;

	write_cmos_sensor(0x3106,0x05);
	write_cmos_sensor(0x440d,0x10);
	write_cmos_sensor(0x4409,0x8000);
	write_cmos_sensor(0x440b,0x801f);
	write_cmos_sensor(0x0100,0x01);
	write_cmos_sensor(0x4400,0x11);

	while((read_cmos_sensor(0x4420)&0x01) == 0x01){
		delay++;
		if(delay == 1000) {
			pr_debug("1st section OTP is still busy for reading. R0x4420[0]!=0 \n");
			return 0;
		}
	}
	return 1;
}

static u8 SC202ACS_otp_read_group(u16 addr, u8 *data, u8 length)
{
	u8 i = 0;
	for (i = 0; i < length; i++) {
		data[i] = read_cmos_sensor(addr + i);
#if SC202ACS_OTP_DEBUG
		pr_debug("addr = 0x%x, data = 0x%x\n", addr + i, data[i]);
#endif
	}
	return 0;
}

int sc202acs_iReadData(unsigned int ui4_offset,
	unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;

	pr_debug("ui4_offset = 0x%x, ui4_length = %d \n", ui4_offset, ui4_length);

	sc202acs_sensor_otp_init(1);

	pr_debug("offset = 0x%x, length = %d \n", ui4_offset, ui4_length);
	mdelay(10);

	i4RetValue = SC202ACS_otp_read_group(ui4_offset, pinputdata, ui4_length);
	if (i4RetValue != 0) {
		pr_debug("I2C iReadData failed!!\n");
		return -1;
	}
	return 0;
}

unsigned int sc202acs_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
        pr_debug(" sc202acs_read_region\n");
	g_pstI2CclientG = client;
	if (size > (SC202ACS_OTP_SIZE + SC202ACS_OTP_HEAD))
		size = (SC202ACS_OTP_SIZE + SC202ACS_OTP_HEAD);
	memcpy((void*)data,(void*)g_otpMemoryData,size);
	return size;
}

EXPORT_SYMBOL(sc202acs_read_region);

void sc202acs_read_otp_data(void)
{
	g_otpMemoryData[0] = 0x95;
	g_otpMemoryData[1] = 0x55;
	g_otpMemoryData[2] = 0x96;
	g_otpMemoryData[3] = 0x30;
	sc202acs_iReadData(0x8000, SC202ACS_OTP_SIZE, &g_otpMemoryData[SC202ACS_OTP_HEAD]);
}
