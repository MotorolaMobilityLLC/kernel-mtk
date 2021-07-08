/*
 * Copyright (C) 2019 MediaTek Inc.
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
/*
 * NOTE:
 * The modification is appended to initialization of image sensor.
 * After sensor initialization, use the function
 * bool otp_update_wb(unsigned char golden_rg, unsigned char golden_bg)
 * and
 * bool otp_update_lenc(void)
 * and then the calibration of AWB & LSC & BLC will be applied.
 * After finishing the OTP written, we will provide you the typical
 * value of golden sample.
 */

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>
#include <linux/slab.h>

//#ifndef VENDOR_EDIT
//#include "kd_camera_hw.h"
/*Caohua.Lin@Camera.Drv, 20180126 remove to adapt with mt6771*/
//#endif
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_typedef.h"

#include "mot_cofud_gc02m1mipiraw_Sensor.h"
#include "gc02m1tspotp.h"


/******************Modify Following Strings for Debug*******************/
#define PFX "GC02M1TSPOTP"
#define LOG_1 SENSORDB("GC02M1TSP,MIPI CAM\n")
#define LOG_INF(format, args...) \
	pr_err(PFX "[%s] " format, __func__, ##args)
/*********************   Modify end    *********************************/

#define USHORT        unsigned short
#define BYTE          unsigned char
#define I2C_ID          0x6e

int gc02m1_wb_idx = -1;
gc02m1_otp_t gc02m1_otp_data;

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[1] = { (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 1, (u8 *) &get_byte, 1, I2C_ID);
	return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[2] = {
		(char)(addr & 0xFF),
		(char)(para & 0xFF) };

	iWriteRegI2C(pusendcmd, 2, I2C_ID);
}

static u8 crc_reverse_byte(u32 data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static u32 convert_crc(u8 *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
}

static bool otp_check_crc16(u8 *data, u32 size, u32 ref_crc)
{
	u16 crc = 0x0000;
	u16 crc_reverse = 0x0000;
	u32 i, j;

	u32 tmp;
	u32 tmp_reverse;

//Calculate both methods of CRC since integrators differ on
// how CRC should be calculated.
	for (i = 0; i < size; i++) {
		tmp_reverse = crc_reverse_byte(data[i]);
		tmp = data[i] & 0xff;
		for (j = 0; j < 8; j++) {
			if (((crc & 0x8000) >> 8) ^ (tmp & 0x80))
				crc = (crc << 1) ^ 0x8005;
			else
				crc = crc << 1;
			tmp <<= 1;

			if (((crc_reverse & 0x8000) >> 8) ^ (tmp_reverse & 0x80))
				crc_reverse = (crc_reverse << 1) ^ 0x8005;
			else
				crc_reverse = crc_reverse << 1;

			tmp_reverse <<= 1;
		}
	}

	crc_reverse = (crc_reverse_byte(crc_reverse) << 8) |
		crc_reverse_byte(crc_reverse >> 8);

	LOG_INF("REF_CRC 0x%x CALC CRC 0x%x CALC Reverse CRC 0x%x\n",
		ref_crc, crc, crc_reverse);

	return (crc == ref_crc || crc_reverse == ref_crc);
}

unsigned int gc02m1_read_all_data(mot_calibration_info_t * pOtpCalInfo)
{
	unsigned int i = 0;
	unsigned int wb_flag = 0;

	u8 *otp_data = (u8 *)&gc02m1_otp_data;

	LOG_INF("LOG_INF gc02m1_read_all_data\n");

	write_cmos_sensor_8(0xf3, 0x30);
	write_cmos_sensor_8(0xfe, 0x02);
	write_cmos_sensor_8(0x17, 0x78);
	write_cmos_sensor_8(0xf3, 0x34);
	wb_flag =read_cmos_sensor_8(0x019);
	LOG_INF("group2 debugxxx =read_cmos_sensor_8(0x78) = 0x%x \n",wb_flag);
	pOtpCalInfo->awb_status=STATUS_CRC_FAIL;
	if(wb_flag == 0x40)//1
	{
		for (i = 0; i < 9; i++) {
		write_cmos_sensor_8(0xfe, 0x02);
		write_cmos_sensor_8(0x17, 0x78+i*8);
		write_cmos_sensor_8(0xf3, 0x34);
		otp_data[i]= read_cmos_sensor_8(0x019);
		LOG_INF("group1 debugxxx G-Dg otp_data[%d] = 0x%x\n",i,otp_data[i]);
		}
		if ((wb_flag & 0xC0) >> 6 == 0x01) {
			if (otp_check_crc16(&(gc02m1_otp_data.group_data[0].wb_flag),
				sizeof(gc02m1_otp_data.group_data[0].wb_data)+1,
						convert_crc(&(gc02m1_otp_data.group_data[0].wb_checksum[0])))) {
                                pOtpCalInfo->awb_status = STATUS_OK;
                                LOG_INF("pOtpCalInfo->awb_status=%d\n",pOtpCalInfo->awb_status);
				LOG_INF("debugxxx CRC eeprom WB checksum success\n");
				gc02m1_wb_idx = 3;
			}
		}
	}else if(wb_flag == 0x10)//2
	{
	     for (i = 0; i < 8; i++) {
              write_cmos_sensor_8(0xfe, 0x02);
	      write_cmos_sensor_8(0x17, 0xC0+i*8);
	      write_cmos_sensor_8(0xf3, 0x34);
	      otp_data[i+1]= read_cmos_sensor_8(0x019);
	      LOG_INF("group2 debugxxx G-Dg otp_data[%d] = 0x%x\n",i,otp_data[i]);
        }
        if ((wb_flag & 0x30) >> 4 == 0x01) {
		if (otp_check_crc16((gc02m1_otp_data.group_data[0].wb_data),
			sizeof(gc02m1_otp_data.group_data[0].wb_data),
						convert_crc(&(gc02m1_otp_data.group_data[0].wb_checksum[0])))) {
                                pOtpCalInfo->awb_status = STATUS_OK;
                                LOG_INF("pOtpCalInfo->awb_status=%d\n",pOtpCalInfo->awb_status);
				LOG_INF("debugxxx CRC eeprom WB checksum success\n");
				gc02m1_wb_idx = 3;
			}
		}
	}

	return 0;
}

unsigned int GC02M1_OTP_Read_Data(u32 addr, u8 *data, u32 size)
{

	LOG_INF("G-Dg GC02M1_OTP_Read_Data, size=%d\n", size);

	if (gc02m1_wb_idx == -1) {
		pr_err("G-Dg otp data err!!!");
		return -1;
	}

	if (size == 2) { //read flag
		memcpy(data, &gc02m1_wb_idx, size);
		LOG_INF("G-Dg addr = 0x%x\n", addr);
	} else if (size == 6) { //read single awb data
		memcpy(data, gc02m1_otp_data.group_data[0].wb_data, size);
		LOG_INF("G-Dg addr = 0x%x, read wb\n", addr);
	}
	return 0;
}
