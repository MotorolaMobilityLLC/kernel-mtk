/*
 * Copyright (C) 2024 tinno81 (tinno81@motorola.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
  ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "mot_bogota_gc08a8mipiraw_otp.h"

#define PFX "[MOTO_EEPROM gc08a8-otp]"
static int m_mot_camera_debug = 1;
#define LOG_INF(format, args...)        do { if (m_mot_camera_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define LOG_DBG(format, args...)        do { if (m_mot_camera_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)

#define LOG_ERR(format, args...) pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)


#define MOT_BOGOTA_GC08A8_OTP_DEBUG  0

#define I2C_ADDR 0x62

//begin 20220402 add for otp check
struct mot_bogota_gc08a8_otp_t mot_bogota_gc08a8_otp_info;
EXPORT_SYMBOL(mot_bogota_gc08a8_otp_info);

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = {
		(char)((addr >> 8) & 0xff),
		(char)(addr & 0xff)
	};

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, I2C_ADDR);

	return get_byte;
}

static void write_cmos_sensor_8bit(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {
		(char)((addr >> 8) & 0xff),
		(char)(addr & 0xff),
		(char)(para & 0xff)
	};

	iWriteRegI2C(pu_send_cmd, 3, I2C_ADDR);
}

static void mot_bogota_gc08a8_otp_init(void)
{
	write_cmos_sensor_8bit(0x031c, 0x60);
	write_cmos_sensor_8bit(0x0315, 0x80);

	write_cmos_sensor_8bit(0x0324, 0x44);
	write_cmos_sensor_8bit(0x0316, 0x09);
	write_cmos_sensor_8bit(0x0a67, 0x80);
	write_cmos_sensor_8bit(0x0313, 0x00);
	write_cmos_sensor_8bit(0x0a53, 0x0e);//04
	write_cmos_sensor_8bit(0x0a65, 0x17);
	write_cmos_sensor_8bit(0x0a68, 0xA1);
	write_cmos_sensor_8bit(0x0a47, 0x00);
	write_cmos_sensor_8bit(0x0a58, 0x00);
	write_cmos_sensor_8bit(0x0ace, 0x0c);
	mdelay(10);//add
}

static void mot_bogota_gc08a8_otp_close(void)
{
	write_cmos_sensor_8bit(0x0316, 0x01);
	write_cmos_sensor_8bit(0x0a67, 0x00);
}

static kal_uint16 mot_bogota_gc08a8_otp_read_group(kal_uint16 addr, kal_uint8 *data, kal_uint16 length)
{
	kal_uint16 i = 0;

	write_cmos_sensor_8bit(0x0313, 0x00);
	write_cmos_sensor_8bit(0x0a69, (addr >> 8) & 0xff);
	write_cmos_sensor_8bit(0x0a6a, addr & 0xff);
	write_cmos_sensor_8bit(0x0313, 0x20);
	write_cmos_sensor_8bit(0x0313, 0x12);

	for (i = 0; i < length; i++) {
		data[i] = read_cmos_sensor(0x0a6c);
#if MOT_BOGOTA_GC08A8_OTP_DEBUG
	pr_debug(PFX,"addr = 0x%x, data = 0x%x\n", addr + i * 8, data[i]);
#endif
	}
	return 0;
}

static kal_uint16 mot_bogota_gc08a8_otp_read_byte(kal_uint16 addr)
{
	kal_uint16 val = 0;
	write_cmos_sensor_8bit(0x0313, 0x00);
	write_cmos_sensor_8bit(0x0a69, (addr >> 8) & 0xff);
	write_cmos_sensor_8bit(0x0a6a, addr & 0xff);
	write_cmos_sensor_8bit(0x0313, 0x20);
	//write_cmos_sensor_8bit(0x0313, 0x12);//??
	val = read_cmos_sensor(0x0a6c);
#if MOT_BOGOTA_GC08A8_OTP_DEBUG
	pr_debug(PFX,"addr = 0x%x, data = 0x%x\n", addr , val);
#endif
	return val;
}

static int mot_bogota_gc08a8_iReadData(unsigned int ui4_offset, unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;
	int i4ResidueDataLength;
	u32 u4CurrentOffset;
	kal_uint8 *pBuff;

	pr_debug(PFX,"ui4_offset = 0x%x, ui4_length = %d \n", ui4_offset, ui4_length);
	i4ResidueDataLength = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;

	i4RetValue =mot_bogota_gc08a8_otp_read_group((kal_uint16) u4CurrentOffset, pBuff, i4ResidueDataLength);
	if (i4RetValue != 0) {
		pr_debug(PFX,"I2C iReadData failed!!\n");
		return -1;
	}
	return 0;
}

static bool check_sum(kal_uint8 *buf, unsigned int size, kal_uint8 chksum)
{
	int i, sum = 0;

	for (i = 0; i < size; i++)
	{
		sum += buf[i];
		//pr_debug(PFX,"buf[%d] = 0x%x %d", i, buf[i], buf[i]);
	}

	if (((sum % 255) + 1) != chksum)
	{
		LOG_INF("chksum fail size = %d sum=%d sum-in-eeprom=%d", size, ((sum % 255) + 1), chksum);
		return false;
	}
	return true;
}

bool check_mot_bogota_gc08a8_otp(void)
{
	kal_uint8 checksum_module = 0;
	kal_uint8 checksum_awb = 0;
	kal_uint8 checksum_lsc = 0;

	mot_bogota_gc08a8_otp_init();

	mot_bogota_gc08a8_otp_info.module_flag = mot_bogota_gc08a8_otp_read_byte(MODULE_GROUP_FLAG);
	mot_bogota_gc08a8_otp_info.awb_flag = mot_bogota_gc08a8_otp_read_byte(AWB_GROUP_FLAG);
	mot_bogota_gc08a8_otp_info.lsc_flag = mot_bogota_gc08a8_otp_read_byte(LSC_GROUP_FLAG);
	LOG_INF("mot_bogota_gc08a8_otp_info.module_flag, = 0x%x", mot_bogota_gc08a8_otp_info.module_flag);
	LOG_INF("mot_bogota_gc08a8_otp_info.awb_flag, = 0x%x", mot_bogota_gc08a8_otp_info.awb_flag);
	LOG_INF("mot_bogota_gc08a8_otp_info.lsc_flag, = 0x%x", mot_bogota_gc08a8_otp_info.lsc_flag);

	//for module info otp read
    if (mot_bogota_gc08a8_otp_info.module_flag == 0x01) {
        LOG_INF("group1_module, size %d", MODULE_LENGTH);
        mot_bogota_gc08a8_iReadData(GROUP1_MODULE_INFO_FLAG, MODULE_LENGTH, &mot_bogota_gc08a8_otp_info.module_param[0]);
		mot_bogota_gc08a8_iReadData(GROUP1_MODULE_INFO_FLAG + (MODULE_LENGTH - 1) * 8, 1, &mot_bogota_gc08a8_otp_info.moduleChksum);
    } else if (mot_bogota_gc08a8_otp_info.module_flag == 0x07) {
        LOG_INF("group2_module, size %d", MODULE_LENGTH);
        mot_bogota_gc08a8_iReadData(GROUP2_MODULE_INFO_FLAG, MODULE_LENGTH, &mot_bogota_gc08a8_otp_info.module_param[0]);
		mot_bogota_gc08a8_iReadData(GROUP2_MODULE_INFO_FLAG + (MODULE_LENGTH - 1) * 8, 1, &mot_bogota_gc08a8_otp_info.moduleChksum);
    } else if ((mot_bogota_gc08a8_otp_info.module_flag & 0x0f) == 0x00) {
        LOG_INF("module info is empty");
    } else {
        LOG_INF("invalid block module flag 0x%x", mot_bogota_gc08a8_otp_info.module_flag);
    }

	//for muduleinfo checksum
	if (check_sum(&mot_bogota_gc08a8_otp_info.module_param[0], MODULE_LENGTH - 1, mot_bogota_gc08a8_otp_info.moduleChksum))
	{
		LOG_INF("[yy]mot_bogota_gc08a8OTP:module flag chksum pass");
		checksum_module = 1;

		LOG_INF("module id = 0x%x", mot_bogota_gc08a8_otp_info.module_param[0]);
		pr_debug(PFX,"Year = 0x%x", mot_bogota_gc08a8_otp_info.module_param[1]);
		pr_debug(PFX,"Month = 0x%x", mot_bogota_gc08a8_otp_info.module_param[2]);
		pr_debug(PFX,"Day = 0x%x", mot_bogota_gc08a8_otp_info.module_param[3]);
		pr_debug(PFX,"LENSID = 0x%x", mot_bogota_gc08a8_otp_info.module_param[4]);
		pr_debug(PFX,"VCMID = 0x%x", mot_bogota_gc08a8_otp_info.module_param[5]);
		pr_debug(PFX,"DriverICID = 0x%x", mot_bogota_gc08a8_otp_info.module_param[6]);

	}

	//for awb otp read
    if (mot_bogota_gc08a8_otp_info.awb_flag == 0x01) {
        LOG_INF("group1_awb, size %d", AWB_LENGTH);
        mot_bogota_gc08a8_iReadData(GROUP1_AWB_INFO_FLAG, AWB_LENGTH, &mot_bogota_gc08a8_otp_info.awb_param[0]);
		mot_bogota_gc08a8_iReadData(GROUP1_AWB_INFO_FLAG + (AWB_LENGTH - 1) * 8, 1, &mot_bogota_gc08a8_otp_info.awbChksum);
    } else if (mot_bogota_gc08a8_otp_info.awb_flag == 0x07) {
        LOG_INF("group2_awb, size %d", AWB_LENGTH);
        mot_bogota_gc08a8_iReadData(GROUP2_AWB_INFO_FLAG, AWB_LENGTH, &mot_bogota_gc08a8_otp_info.awb_param[0]);
		mot_bogota_gc08a8_iReadData(GROUP2_AWB_INFO_FLAG + (AWB_LENGTH - 1) * 8, 1, &mot_bogota_gc08a8_otp_info.awbChksum);
    } else if ((mot_bogota_gc08a8_otp_info.awb_flag & 0x0f) == 0x00) {
        LOG_INF("awb info is empty");
    } else {
        LOG_INF("invalid block awb flag 0x%x", mot_bogota_gc08a8_otp_info.awb_flag);
    }
	//for awb checksum
	if (check_sum(&mot_bogota_gc08a8_otp_info.awb_param[0], AWB_LENGTH - 1, mot_bogota_gc08a8_otp_info.awbChksum))
	{
		checksum_awb = 1;
		LOG_INF("[yy]mot_bogota_gc08a8OTP:awb flag chksum pass");
	}
	else
	{
		int i;
		for (i = 0; i < AWB_LENGTH-1; i++)
		     pr_debug(PFX,"[yy]mot_bogota_gc08a8OTP:awb[%d]=0x%x  %d\n", i, mot_bogota_gc08a8_otp_info.awb_param[i], mot_bogota_gc08a8_otp_info.awb_param[i]);
	}

	//for lsc otp read
    if (mot_bogota_gc08a8_otp_info.lsc_flag == 0x01) {
        LOG_INF("group1_lsc, size %d", LSC_LENGTH);
		mot_bogota_gc08a8_otp_info.lsc_flag = 0x01;
        mot_bogota_gc08a8_iReadData(GROUP1_LSC_INFO_FLAG, LSC_LENGTH, &mot_bogota_gc08a8_otp_info.lsc_param[0]);
		mot_bogota_gc08a8_iReadData(GROUP1_LSC_INFO_FLAG + (LSC_LENGTH - 1) * 8, 1, &mot_bogota_gc08a8_otp_info.lscChksum);
    } else if (mot_bogota_gc08a8_otp_info.lsc_flag == 0x07) {
        LOG_INF("group2_lsc, size %d", LSC_LENGTH);
        mot_bogota_gc08a8_otp_info.lsc_flag = 0x04;
        mot_bogota_gc08a8_iReadData(GROUP2_LSC_INFO_FLAG, LSC_LENGTH, &mot_bogota_gc08a8_otp_info.lsc_param[0]);
		mot_bogota_gc08a8_iReadData(GROUP2_LSC_INFO_FLAG + (LSC_LENGTH - 1) * 8, 1, &mot_bogota_gc08a8_otp_info.lscChksum);
    } else if ((mot_bogota_gc08a8_otp_info.lsc_flag & 0x0f) == 0x00) {
        LOG_INF("lsc info is empty");
    } else {
        LOG_INF("invalid block lsc flag 0x%x", mot_bogota_gc08a8_otp_info.lsc_flag);
    }

	//for lsc checksum
	if (check_sum(&mot_bogota_gc08a8_otp_info.lsc_param[0], LSC_LENGTH - 1, mot_bogota_gc08a8_otp_info.lscChksum))
	{
		checksum_lsc = 1;
		LOG_INF("[yy]mot_bogota_gc08a8OTP:lsc flag chksum pass");
	}

    mot_bogota_gc08a8_otp_close();

	if (1 == (checksum_module & checksum_awb & checksum_lsc))
	{
		return true;
	}
	else
	{
		LOG_INF("otp check fail");
		return false;
	}
}

unsigned int mot_bogota_gc08a8_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size)
{
	unsigned char *dataTmp = data;

    pr_err("<%s>%d:otp region addr = 0x%x, size = %d\n", __func__, __LINE__, addr, size);
#if 1
    if (addr == 0x0 && size == 1) {//0xff
        *(u32 *)data = mot_bogota_gc08a8_otp_info.module_flag;
    } else if (addr == 0x0 && size == 1904) {
        unsigned int totalSize = sizeof(mot_bogota_gc08a8_otp_info.module_flag) +
                                 sizeof(mot_bogota_gc08a8_otp_info.module_param) +
                                 sizeof(mot_bogota_gc08a8_otp_info.moduleChksum) +
                                 sizeof(mot_bogota_gc08a8_otp_info.awb_flag) +
                                 sizeof(mot_bogota_gc08a8_otp_info.awb_param) +
                                 sizeof(mot_bogota_gc08a8_otp_info.awbChksum) +
                                 sizeof(mot_bogota_gc08a8_otp_info.lsc_flag) +
                                 sizeof(mot_bogota_gc08a8_otp_info.lsc_param) +
                                 sizeof(mot_bogota_gc08a8_otp_info.lscChksum);
        pr_err("<%s>%d: otp region addr = 0x%x, size = %d  totalSize=%d \n", __func__, __LINE__, addr, size, totalSize);
        if (size == totalSize) {
            data[0] = mot_bogota_gc08a8_otp_info.module_flag;
            dataTmp += sizeof(mot_bogota_gc08a8_otp_info.module_flag);

            memcpy(dataTmp, mot_bogota_gc08a8_otp_info.module_param, sizeof(mot_bogota_gc08a8_otp_info.module_param));
            dataTmp += sizeof(mot_bogota_gc08a8_otp_info.module_param);

            data[19] = mot_bogota_gc08a8_otp_info.moduleChksum;
            dataTmp += sizeof(mot_bogota_gc08a8_otp_info.moduleChksum);

            data[20] = mot_bogota_gc08a8_otp_info.awb_flag;
            dataTmp += sizeof(mot_bogota_gc08a8_otp_info.awb_flag);

            memcpy(dataTmp, mot_bogota_gc08a8_otp_info.awb_param, sizeof(mot_bogota_gc08a8_otp_info.awb_param));
            dataTmp += sizeof(mot_bogota_gc08a8_otp_info.awb_param);

            data[33] = mot_bogota_gc08a8_otp_info.awbChksum;
            dataTmp += sizeof(mot_bogota_gc08a8_otp_info.awbChksum);

            data[34] = mot_bogota_gc08a8_otp_info.lsc_flag;
            dataTmp += sizeof(mot_bogota_gc08a8_otp_info.lsc_flag);

            memcpy(dataTmp, mot_bogota_gc08a8_otp_info.lsc_param, sizeof(mot_bogota_gc08a8_otp_info.lsc_param));
            dataTmp += sizeof(mot_bogota_gc08a8_otp_info.lsc_param);

            data[totalSize - 1] = mot_bogota_gc08a8_otp_info.lscChksum;
        } else {
            pr_err("<%s>%d: gc08a8 otp size != totalSize", __func__, __LINE__);
            size = totalSize;
        }
    } else if (addr == 0x01 && size == 18) {
        memcpy(data, (mot_bogota_gc08a8_otp_info.module_param), size);
        pr_err("<%s>%d: addr = 0x%x, read module\n", __func__, __LINE__, addr);
    } else if (addr == 0x13 && size == 1) {
        *(u32 *)data = mot_bogota_gc08a8_otp_info.moduleChksum;
        pr_err("<%s>%d: addr = 0x%x, read module checksum\n", __func__, __LINE__, addr);
    } else if (addr == 0x15 && size == 12) {
        memcpy(data, (mot_bogota_gc08a8_otp_info.awb_param), size);
        pr_err("<%s>%d: addr = 0x%x, read awb\n", addr);
    } else if (addr == 0x21 && size == 1) {
        *(u32 *)data = mot_bogota_gc08a8_otp_info.awbChksum;
        pr_err("<%s>%d: addr = 0x%x, read awb checksum 0x%x\n", __func__, __LINE__, addr, *(u32 *)data);
    } else if (addr == 0x23 && size == 1868) {
        memcpy(data, mot_bogota_gc08a8_otp_info.lsc_param, size);
        pr_err("<%s>%d: addr = 0x%x, read lsc\n", __func__, __LINE__, addr);
    } else if (addr == 0x076f && size == 1) {
        *(u32 *)data = mot_bogota_gc08a8_otp_info.lscChksum;
        pr_err("<%s>%d: addr = 0x%x, read lscChksum = %x\n", __func__, __LINE__, addr, *(u32 *)data);
    } else{
        pr_err("<%s>%d: otp addr = 0x%x, size = %d ,read error !!!\n", __func__, __LINE__, addr, size);
    }
#endif
    return size;
}

EXPORT_SYMBOL(mot_bogota_gc08a8_read_region);
//end 20220402 add for otp check
