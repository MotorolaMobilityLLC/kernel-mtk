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

#include "eeprom_i2c_sc202cs_driver.h"
#include "kd_imgsensor_define.h"

static struct i2c_client *g_pstI2CclientG;

#define SC501CS_OTP_DEBUG  0
#define SC202CS_I2C_ID     0x6c /*0x20*/

#define SC202CS_OTP_SIZE 32
#define SC202CS_OTP_HEAD 4

#define SC202CS_OTP_AWB_FLAG_ADDR (0x08)
#define SC202CS_OTP_AWB_GROUP1_CHECKSUM_H (0x14) //0x08~0x13
#define SC202CS_OTP_AWB_GROUP1_CHECKSUM_L (0x15)
#define SC202CS_OTP_GROUP1 0x40
#define SC202CS_GROUP1_CHECKSUM_LENGTH 12

#define SC202CS_OTP_AWB_GROUP2_GOLDEN_RG_L_ADDR (0x16)
#define SC202CS_OTP_AWB_GROUP2_CHECKSUM_H (0x1D) //0x16~0x1C
#define SC202CS_OTP_AWB_GROUP2_CHECKSUM_L (0x1F)
#define SC202CS_OTP_GROUP2 0xD0
#define SC202CS_GROUP2_CHECKSUM_LENGTH 7
#define SC202CS_OTP_DATA_PATH "/data/vendor/camera_dump/mot_gnevan_sc202cs_otp.bin"

unsigned char g_otpMemoryData[SC202CS_OTP_SIZE+SC202CS_OTP_HEAD] = {0};

static u8 crc_reverse_byte(u32 data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
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

	pr_debug("REF_CRC 0x%x CALC CRC 0x%x CALC Reverse CRC 0x%x\n",
		ref_crc, crc, crc_reverse);

	return (crc == ref_crc || crc_reverse == ref_crc);
}

static u16 read_cmos_sensor(u16 addr)
{
	u16 get_byte = 0;
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, SC202CS_I2C_ID);

	return get_byte;
}

void sc202cs_otp_dump_bin(const void *data, uint32_t size)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(SC202CS_OTP_DATA_PATH, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0666);
	if (IS_ERR_OR_NULL(fp)) {
		ret = PTR_ERR(fp);
		pr_err("open file error(%s), error(%d)\n",  SC202CS_OTP_DATA_PATH, ret);
		goto p_err;
	}

	ret = vfs_write(fp, (const char *)data, size, &fp->f_pos);
	if (ret < 0) {
		pr_err("file write fail(%s) to EEPROM data(%d)", SC202CS_OTP_DATA_PATH, ret);
		goto p_err;
	}

	pr_debug("wirte to file(%s)\n", SC202CS_OTP_DATA_PATH);
p_err:
	if (!IS_ERR_OR_NULL(fp))
		filp_close(fp, NULL);

	set_fs(old_fs);
	pr_debug(" end writing file");
}

static void write_cmos_sensor(u16 addr, u16 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8),
		(char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, SC202CS_I2C_ID);
}

static int sc202cs_sensor_otp_init(int section){
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

static u8 SC202CS_otp_read_group(u16 addr, u8 *data, u8 length)
{
	u8 i = 0;
	for (i = 0; i < length; i++) {
		data[i] = read_cmos_sensor(addr + i);
#if SC501CS_OTP_DEBUG
		pr_debug("addr = 0x%x, data = 0x%x\n", addr + i, data[i]);
#endif
	}
	return 0;
}

int sc202cs_iReadData(unsigned int ui4_offset,
	unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;

	pr_debug("ui4_offset = 0x%x, ui4_length = %d \n", ui4_offset, ui4_length);

	sc202cs_sensor_otp_init(1);

	pr_debug("offset = 0x%x, length = %d \n", ui4_offset, ui4_length);
	mdelay(10);

	i4RetValue = SC202CS_otp_read_group(ui4_offset, pinputdata, ui4_length);
	if (i4RetValue != 0) {
		pr_debug("I2C iReadData failed!!\n");
		return -1;
	}
	return 0;
}

unsigned int sc202_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
        pr_debug("hfx sc202_read_region\n");
	g_pstI2CclientG = client;
	if (size > (SC202CS_OTP_SIZE + SC202CS_OTP_HEAD))
		size = (SC202CS_OTP_SIZE + SC202CS_OTP_HEAD);
	memcpy((void*)data,(void*)g_otpMemoryData,size);
	return size;
}

void sc202_read_otp_data(mot_calibration_info_t * pOtpCalInfo)
{
	u8 awbGroupFlag = 0;
	u16 checkSumRes = 0;
	u16 checkSumValue = 0;
	pOtpCalInfo->awb_status = STATUS_CRC_FAIL;
	g_otpMemoryData[0] = 0x95;
	g_otpMemoryData[1] = 0x55;
	g_otpMemoryData[2] = 0x96;
	g_otpMemoryData[3] = 0x30;
	sc202cs_iReadData(0x8000, SC202CS_OTP_SIZE, &g_otpMemoryData[SC202CS_OTP_HEAD]);
	awbGroupFlag = g_otpMemoryData[SC202CS_OTP_AWB_FLAG_ADDR + SC202CS_OTP_HEAD];
	if (SC202CS_OTP_GROUP2 == awbGroupFlag) {
		checkSumValue = (g_otpMemoryData[SC202CS_OTP_AWB_GROUP2_CHECKSUM_H + SC202CS_OTP_HEAD] << 8) | g_otpMemoryData[SC202CS_OTP_AWB_GROUP2_CHECKSUM_L + SC202CS_OTP_HEAD];
		checkSumRes = otp_check_crc16((u8*) (&g_otpMemoryData[SC202CS_OTP_AWB_GROUP2_GOLDEN_RG_L_ADDR + SC202CS_OTP_HEAD]), SC202CS_GROUP2_CHECKSUM_LENGTH, checkSumValue);
		pr_debug("sc202cs awbGroupFlag = 0x%x checkSumValue = 0x%x checkSumRes = 0x%x \n", awbGroupFlag, checkSumValue, checkSumRes);
	}
	else if (SC202CS_OTP_GROUP1 == awbGroupFlag) {
		checkSumValue = (g_otpMemoryData[SC202CS_OTP_AWB_GROUP1_CHECKSUM_H + SC202CS_OTP_HEAD] << 8) | g_otpMemoryData[SC202CS_OTP_AWB_GROUP1_CHECKSUM_L + SC202CS_OTP_HEAD];
		checkSumRes = otp_check_crc16((u8*) (&g_otpMemoryData[SC202CS_OTP_AWB_FLAG_ADDR + SC202CS_OTP_HEAD]), SC202CS_GROUP1_CHECKSUM_LENGTH, checkSumValue);
		pr_debug("sc202cs awbGroupFlag = 0x%x checkSumValue = 0x%x checkSumRes = 0x%x \n", awbGroupFlag, checkSumValue, checkSumRes);
	}
	else {
		pr_err("sc202cs awbGroupFlag = 0x%x invalid \n", awbGroupFlag);
		return;
	}

	if (checkSumRes) {
		pOtpCalInfo->awb_status = STATUS_OK;
		pr_debug("sc202cs awb checksum pass");
	}
	else {
		pr_err("sc202cs awb checksum fail");
	}
	sc202cs_otp_dump_bin(g_otpMemoryData+SC202CS_OTP_HEAD, SC202CS_OTP_SIZE);
}


