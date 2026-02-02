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

#include "eeprom_i2c_gc02m1_driver.h"

static struct i2c_client *g_pstI2CclientG;

#define GC02M1_OTP_DEBUG  1
#define GC02M1_I2C_ID     0x6e /*0x20*/

#define GC02M1_OTP_SIZE 8


u16 addrs1[] = {0x80,0x88,0x90,0x98,0xA0,0xA8,0xB0,0xB8};
u16 addrs2[] = {0xC0,0xC8,0xD0,0xD8,0xE0,0xE8,0xF0,0xF8};


unsigned char g_otpMemoryData_gc02m1[GC02M1_OTP_SIZE] = {0};

static u16 read_cmos_sensor(u16 addr)
{
	u16 get_byte = 1;
	u16 ret = 0;

	char pu_send_cmd[1] = {(char)(addr & 0xFF) };

	ret = iReadRegI2C(pu_send_cmd, 1, (u8 *)&get_byte, 1, GC02M1_I2C_ID);

	return get_byte;
}

static void write_cmos_sensor(u16 addr, u16 para)
{
	u16 ret = 0;

	char pu_send_cmd[2] = {(char)(addr & 0xFF), (char)(para & 0xFF)};

	ret = iWriteRegI2C(pu_send_cmd, 2, GC02M1_I2C_ID);
}

static int gc02m1_sensor_otp_init(int section){
	write_cmos_sensor(0xf3, 0x30);
	pr_debug("otp init");
	return 1;
}

static void gc02m1_sensor_otp_close(){
	write_cmos_sensor(0xfe, 0x00);
	pr_debug("otp close");
}

static u16 mot_sydney_gc02m1_otp_read_byte(u16 addr)
{
	u16 val = 0;
	write_cmos_sensor(0xfe, 0x02);
	write_cmos_sensor(0x17, addr&0xff);
	write_cmos_sensor(0xf3, 0x34);
	val = read_cmos_sensor(0x19);
#if GC02M1_OTP_DEBUG
	pr_debug("test addr = 0x%x, data = 0x%x\n", addr , val);
#endif
	return val;
}

static u8 GC02M1_otp_read_group(u16 *addr, u8 *data, u8 length)
{
	u8 i = 0;
	for (i = 0; i < length; i++) {
		data[i] = mot_sydney_gc02m1_otp_read_byte(*(addr+i));
#if GC02M1_OTP_DEBUG
		pr_debug("addr = 0x%x, data = 0x%x\n", *(addr+i), data[i]);
#endif
	}
	return 0;
}

int gc02m1_iReadData(unsigned int ui4_offset,
	unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;
	u8 flagData = 0;

	pr_debug("ui4_offset = 0x%x, ui4_length = %d \n", ui4_offset, ui4_length);

	gc02m1_sensor_otp_init(1);

	pr_debug("offset = 0x%x, length = %d \n", ui4_offset, ui4_length);
	mdelay(10);

	flagData = mot_sydney_gc02m1_otp_read_byte(ui4_offset);

	pr_debug("flagData = 0x%x",flagData);

	if(((flagData&0x30)>>4) == 1)
	{
		pr_debug("read group2");
		i4RetValue = GC02M1_otp_read_group(addrs2, pinputdata, ui4_length);
	}
	else if(((flagData&0xc0)>>6) == 1)
	{
		pr_debug("read group1");
		i4RetValue = GC02M1_otp_read_group(addrs1, pinputdata, ui4_length);
	}
	else {
		pr_debug("group invalid");
		gc02m1_sensor_otp_close();
		return -1;
	}

	gc02m1_sensor_otp_close();

	if (i4RetValue != 0) {
		pr_debug("I2C iReadData failed!!\n");
		return -1;
	}
	return 0;
}

unsigned int gc02m1_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
    pr_debug(" gc02m1_read_region\n");
	g_pstI2CclientG = client;
	if (size > (GC02M1_OTP_SIZE))
		size = (GC02M1_OTP_SIZE);
	memcpy((void*)data,(void*)g_otpMemoryData_gc02m1,size);
	return size;
}

EXPORT_SYMBOL(gc02m1_read_region);

void gc02m1_read_otp_data(void)
{
	gc02m1_iReadData(0x78, GC02M1_OTP_SIZE, &g_otpMemoryData_gc02m1[0]);
}
