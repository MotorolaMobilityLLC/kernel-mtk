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

#include "mot_milan_gc02m1mipiraw_Sensor.h"
#include "mot_milan_gc02m1_otp.h"


/******************Modify Following Strings for Debug*******************/
#define PFX "mot_milan_gc02m1_otp"
#define LOG_1 SENSORDB("GC02M1,MIPI CAM\n")
#define LOG_INF(format, args...) \
	pr_err(PFX "[%s] " format, __func__, ##args)
/*********************   Modify end    *********************************/

#define USHORT        unsigned short
#define BYTE          unsigned char
#define I2C_ID          0x6e

gc02m1_otp_data_t gc02m1_otp_data;

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

static void eeprom_dump_bin(const char *file_name, uint32_t size, const void *data)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0666);
	if (IS_ERR_OR_NULL(fp))
	{
		ret = PTR_ERR(fp);
		LOG_INF("open file error(%s), error(%d)\n",  file_name, ret);
		goto p_err;
	}

	ret = vfs_write(fp, (const char *)data, size, &fp->f_pos);
	if (ret < 0)
	{
		LOG_INF("file write fail(%s) to EEPROM data(%d)", file_name, ret);
		goto p_err;
	}

	LOG_INF("wirte to file(%s)\n", file_name);
p_err:
	if (!IS_ERR_OR_NULL(fp))
		filp_close(fp, NULL);

	set_fs(old_fs);
	LOG_INF(" end writing file");
}

unsigned int gc02m1_read_all_data(mot_calibration_info_t * pOtpCalInfo)
{
	unsigned int i = 0;
	unsigned int addr = 0;

	u8 *otp_data = (u8 *)&gc02m1_otp_data;

	pOtpCalInfo->mnf_status = STATUS_OK;
	LOG_INF("LOG_INF gc02m1_read_all_data\n");

	write_cmos_sensor_8(0xf3, 0x30);
	write_cmos_sensor_8(0xfe, 0x02);
	write_cmos_sensor_8(0x17, 0x78);
	write_cmos_sensor_8(0xf3, 0x34);
	otp_data[0] = read_cmos_sensor_8(0x019);
	LOG_INF("G-Dg read_cmos_sensor_8(0x78) = 0x%x 0x%x \n", otp_data[0], gc02m1_otp_data.program_flag);
	if((otp_data[0]&0xC0)>>6 == 0x01)//1
		addr = 0x80;
	else if((otp_data[0]&0x30)>>6 == 0x01)//2
		addr = 0xC0;
	else {
		pOtpCalInfo->mnf_status=STATUS_CRC_FAIL;
		return -1;
	}

	for (i = 0; i < 8; i++) {
		write_cmos_sensor_8(0xfe, 0x02);
		write_cmos_sensor_8(0x17, addr+i*8);
		write_cmos_sensor_8(0xf3, 0x34);
		otp_data[i+1]= read_cmos_sensor_8(0x019);
		LOG_INF("G-Dg otp_data[%d] = 0x%x\n",i+1,otp_data[i+1]);
	}

	eeprom_dump_bin(EEPROM_DATA_PATH, sizeof(gc02m1_otp_data_t), &gc02m1_otp_data.program_flag);
	eeprom_dump_bin(SERIAL_DATA_PATH, DUMP_DEPTH_SERIAL_NUMBER_SIZE, gc02m1_otp_data.serial_number);

	return 0;
}

