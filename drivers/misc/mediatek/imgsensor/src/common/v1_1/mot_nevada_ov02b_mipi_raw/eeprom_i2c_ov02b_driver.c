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
#include "mot_nevada_ov02bmipiraw_Sensor.h"
#include "eeprom_i2c_ov02b_driver.h"

static struct imgsensor_struct *spImgSensor;
static struct i2c_client *g_pstI2CclientG;

#define OV02B_OTP_DEBUG  1
#define OV02B_I2C_ID     0x79 /*0x20*/

#define OV02B_OTP_SIZE 32
#define OV02B_OTP_HEAD 4

static unsigned char g_otpMemoryData[OV02B_OTP_SIZE+OV02B_OTP_HEAD] = {0};

static struct IMGSENSOR_I2C_CFG *get_i2c_cfg(void)
{
        return &(((struct IMGSENSOR_SENSOR_INST *)
                  (spImgSensor->psensor_func->psensor_inst))->i2c_cfg);
}


static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
        kal_uint16 get_byte = 0;
        char pusendcmd[1] = {(char)(addr & 0xFF) };

        imgsensor_i2c_read(
                get_i2c_cfg(),
                pusendcmd,
                1,
                (u8 *)&get_byte,
                1,
                spImgSensor->i2c_write_id,
                IMGSENSOR_I2C_SPEED);
        return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
        char pusendcmd[2] = {(char)(addr & 0xFF), (char)(para & 0xFF)};
        imgsensor_i2c_write(
                get_i2c_cfg(),
                pusendcmd,
                2,
                2,
                spImgSensor->i2c_write_id,
                IMGSENSOR_I2C_SPEED);
}


static u8 OV02B_otp_read_group(u16 addr, u8 *data, u8 length)
{
	u8 i = 0;
	for (i = 0; i < length; i++) {
		data[i] = read_cmos_sensor(addr + i);
#if OV02B_OTP_DEBUG
		pr_debug("addr = 0x%x, data = 0x%x\n", addr + i, data[i]);
#endif
	}
	return 0;
}

static void ov02b_sensor_otp_init()
{
       write_cmos_sensor(0xfd, 0x06);
       write_cmos_sensor(0x21, 0x00);
       write_cmos_sensor(0x2f, 0x01);

       return;
}

int ov02b_iReadData(unsigned int ui4_offset,
	unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;

	pr_debug("ui4_offset = 0x%x, ui4_length = %d \n", ui4_offset, ui4_length);

	ov02b_sensor_otp_init();

	pr_debug("offset = 0x%x, length = %d \n", ui4_offset, ui4_length);
	mdelay(10);

	i4RetValue = OV02B_otp_read_group(ui4_offset, pinputdata, ui4_length);
	if (i4RetValue != 0) {
		pr_debug("I2C iReadData failed!!\n");
		return -1;
	}
	return 0;
}

unsigned int ov02b_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
        pr_debug(" ov02b_read_region addr = 0x%x, size = %d\n", addr, size);
	g_pstI2CclientG = client;
	if (size > (OV02B_OTP_SIZE + OV02B_OTP_HEAD))
		size = (OV02B_OTP_SIZE + OV02B_OTP_HEAD);
	memcpy((void*)data,(void*)g_otpMemoryData,size);
	return size;
}

EXPORT_SYMBOL(ov02b_read_region);

void ov02b_read_otp_data(struct imgsensor_struct *pImgSensor)
{
	spImgSensor = pImgSensor;

	g_otpMemoryData[0] = 0x95;
	g_otpMemoryData[1] = 0x55;
	g_otpMemoryData[2] = 0x96;
	g_otpMemoryData[3] = 0x30;

	ov02b_iReadData(0x00, OV02B_OTP_SIZE, &g_otpMemoryData[OV02B_OTP_HEAD]);
}
