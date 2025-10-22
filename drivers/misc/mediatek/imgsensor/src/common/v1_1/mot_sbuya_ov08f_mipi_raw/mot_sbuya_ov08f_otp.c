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

#include "kd_imgsensor_define.h"

#include "mot_sbuya_ov08f_otp.h"
#include "mot_sbuya_ov08f_mipi_raw_Sensor.h"

#define LOG_INF(format, args...)    \
	pr_debug(PFX "[%s] " format, __func__, ##args)

static struct i2c_client *g_pstI2CclientG;
static struct imgsensor_struct *spImgSensor;
static int8_t schkSumResult = false;
static mot_calibration_status_t calibration_status = {STATUS_CRC_FAIL};
static mot_calibration_mnf_t mnf_info = {0};

#define OV08F_OTP_DEBUG  1
#define OV08F_OTP_SIZE 3819

unsigned char g_otpMemoryData[OV08F_OTP_SIZE] = {0};

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

static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static uint32_t convert_crc(uint8_t *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
}

static int32_t eeprom_util_check_crc16(uint8_t *data, uint32_t size, uint32_t ref_crc)
{
	int32_t crc_match = 0;
	uint16_t crc = 0x0000;
	uint16_t crc_reverse = 0x0000;
	uint32_t i, j;

	uint32_t tmp;
	uint32_t tmp_reverse;

	/* Calculate both methods of CRC since integrators differ on
	* how CRC should be calculated. */
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

	if (crc == ref_crc || crc_reverse == ref_crc)
		crc_match = 1;

	LOG_INF("REF_CRC 0x%x CALC CRC 0x%x CALC Reverse CRC 0x%x matches? %d\n",
		ref_crc, crc, crc_reverse, crc_match);

	return crc_match;
}

static int8_t ov08f_otp_do_checksum(unsigned char *data)
{
	struct SBUYA_OV08F_otp_t *otp = (struct SBUYA_OV08F_otp_t*)data;
	if (!eeprom_util_check_crc16(otp->flag_of_basic_infomation, BASIC_INFO_SIZE, convert_crc(otp->manufacturing_data_chksum))) {
		LOG_INF("basic info CRC Failed!");
		return -1;
	} else {
		calibration_status.mnf = STATUS_OK;
	}
	if (!eeprom_util_check_crc16(otp->flag_of_awb, AWB_SIZE, convert_crc(otp->awb_chksum))) {
		LOG_INF("Awb CRC Failed!");
		return -1;
	} else {
		calibration_status.awb = STATUS_OK;
	}
	if (!eeprom_util_check_crc16(otp->flag_of_oc, OC_SIZE, convert_crc(otp->oc_chksum))) {
		LOG_INF("OC CRC Failed!");
		return -1;
	}
	if (!eeprom_util_check_crc16(otp->flag_of_sfr1, SFR1_SIZE, convert_crc(otp->sfr1_chksum))) {
		LOG_INF("Sfr1 CRC Failed!");
		return -1;
	}
	if (!eeprom_util_check_crc16(otp->flag_of_sfr2, SFR2_SIZE, convert_crc(otp->sfr2_chksum))) {
		LOG_INF("Sfr2 CRC Failed!");
		return -1;
	}
	if (!eeprom_util_check_crc16(otp->flag_of_qcom_lsc, QCOM_LSC_SIZE, convert_crc(otp->qcom_lsc_chksum))) {
		LOG_INF("Qcom Lsc CRC Failed!");
		return -1;
	}
	if (!eeprom_util_check_crc16(otp->flag_of_mtk_lsc, MTK_LSC_SIZE, convert_crc(otp->mtk_lsc_chksum))) {
		LOG_INF("MTK Lsc CRC Failed!");
		return -1;
	} else {
		calibration_status.lsc = STATUS_OK;
	}
	if (!eeprom_util_check_crc16(otp->flag_of_mtk_necessary_info, MTK_NECESSARY_INFO_SIZE, convert_crc(otp->mtk_necessary_info_chksum))) {
		LOG_INF("MTK_necessary info CRC Failed!");
		return -1;
	}

	LOG_INF("OV08F otp checksum ok!");
	return 0;
}

static void ov08f_sensor_otp_init()
{
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x1d,0x00);
	write_cmos_sensor(0x1c,0x19);
	write_cmos_sensor(0x20,0x0f);
	write_cmos_sensor(0xe7,0x03);
	write_cmos_sensor(0xe7,0x00);
	mdelay(3);

	return;
}

static void ov08f_sensor_otp_set_block(u8 block)
{
	write_cmos_sensor(0xfd,0x03);
	write_cmos_sensor(0x9f,0x20);
	write_cmos_sensor(0x9d,0x10);
	write_cmos_sensor(0xa9,block);
	write_cmos_sensor(0xfd,0x09);

	return;
}

static bool ov08f_otp_verify_group(u8 block, u16 addr)
{
	u16 flag = 0;
	ov08f_sensor_otp_set_block(block);
	flag = read_cmos_sensor(addr);

	if ((flag & 0xc0) >> 6 == 0x01) {
		LOG_INF("group vaild, block is %d", (unsigned int)block);
		return true;
	} else if ((flag & 0xc0) >> 6 == 0x00) {
		LOG_INF("group empty, block is %d", (unsigned int)block);
		return false;
	} else if ((flag & 0xc0) >> 6 == 0x11) {
		LOG_INF("group invaild, block is %d", (unsigned int)block);
		return false;
	} else {
		LOG_INF("UNKOWN OTP ERR");
		return false;
	}

	return false;
}

static u8 ov08f_otp_get_verify_group_num()
{
	bool groupValid = false;

	groupValid = ov08f_otp_verify_group(4, 0x00);
	if (groupValid == true) {
		LOG_INF("group1 vaild");
		return 1;
	}

	groupValid = ov08f_otp_verify_group(34, 0x00);
	if (groupValid == true) {
		LOG_INF("group2 vaild");
		return 2;
	}

	return 0;
}

static u8 ov08f_otp_read_group(u8 block_start, u8 block_end, unsigned char *data)
{
	int i = 0;
	int j = 0;
	int index = 0;

	for (j = block_start; j < block_end; ++j) {
		ov08f_sensor_otp_set_block(j);
		for (i = 0; i < BLOCK_DATA_SIZE; i++) {
			data[index++] = read_cmos_sensor(i);
		}
	}

	ov08f_sensor_otp_set_block(block_end);
	for (i = 0; i < LAST_BLOCK_DATA_SIZE; i++) {
		data[index++] = read_cmos_sensor(i);
	}

	return 0;
}

int ov08f_iReadData(unsigned char *pinputdata)
{
	int i4RetValue = 0;
	u8 groupNum = 0;

	ov08f_sensor_otp_init();

	groupNum = ov08f_otp_get_verify_group_num();

	if (groupNum == 1) {
		LOG_INF("group1 vaild");
		i4RetValue = ov08f_otp_read_group(GROUP1_BLOCK_START_NUM,
			GROUP1_BLOCK_END_NUM, pinputdata);
		if (i4RetValue != 0) {
			LOG_INF("I2C iReadData failed!!\n");
			return -1;
		}
	} else if (groupNum == 2) {
		LOG_INF("group2 vaild");
		i4RetValue = ov08f_otp_read_group(GROUP2_BLOCK_START_NUM,
			GROUP2_BLOCK_END_NUM, pinputdata);
		if (i4RetValue != 0) {
			LOG_INF("I2C iReadData failed!!\n");
			return -1;
		}
	} else {
		LOG_INF("UNKOWN OTP ERR");
		return -1;
	}

	return 0;
}

static void ov08f_format_mnf_data(void *data, mot_calibration_mnf_t *mnf)
{
	int ret = 0;
	struct SBUYA_OV08F_otp_t *otp = (struct SBUYA_OV08F_otp_t *)data;

	ret = snprintf(mnf->table_revision, MAX_CALIBRATION_STRING, "0x%x",
		otp->eeprom_table_revision[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->table_revision failed");
		mnf->table_revision[0] = 0;
	}

	ret = snprintf(mnf->mot_part_number, MAX_CALIBRATION_STRING, "%c%c%c%c%c%c%c%c",
		otp->moto_part_num[0], otp->moto_part_num[1], otp->moto_part_num[2], otp->moto_part_num[3],
		otp->moto_part_num[4], otp->moto_part_num[5], otp->moto_part_num[6], otp->moto_part_num[7]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->mot_part_number failed");
		mnf->mot_part_number[0] = 0;
	}

	ret = snprintf(mnf->actuator_id, MAX_CALIBRATION_STRING, "0x%x", otp->actuator_id[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->actuator_id failed");
		mnf->actuator_id[0] = 0;
	}

	if (otp->lens_id[0] == 0xC2){
		ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "AAC 084195A01");
	} else {
		ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "Unknown");
		LOG_INF("unknown lens_id");
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->lens_id failed");
		mnf->lens_id[0] = 0;
	}

	if (otp->manufacturer_id[0] == 'Q' && otp->manufacturer_id[1] == 'T') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Qtech");
	} else if (otp->manufacturer_id[0] == 'S' && otp->manufacturer_id[1] == 'W') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "SunWing");
	} else {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Unknown");
		LOG_INF("unknown manufacturer_id");
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->integrator failed");
		mnf->integrator[0] = 0;
	}

	ret = snprintf(mnf->factory_id, MAX_CALIBRATION_STRING, "%c%c",
		otp->factory_id[0], otp->factory_id[1]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->factory_id failed");
		mnf->factory_id[0] = 0;
	}

	ret = snprintf(mnf->manufacture_line, MAX_CALIBRATION_STRING, "%u",
		otp->manufacture_line[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->manufacture_line failed");
		mnf->manufacture_line[0] = 0;
	}

	ret = snprintf(mnf->manufacture_date, MAX_CALIBRATION_STRING, "20%u/%u/%u",
		otp->manufacture_date[0], otp->manufacture_date[1], otp->manufacture_date[2]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->manufacture_date failed");
		mnf->manufacture_date[0] = 0;
	}

	ret = snprintf(mnf->serial_number, MAX_CALIBRATION_STRING, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		otp->serial_number[0], otp->serial_number[1],
		otp->serial_number[2], otp->serial_number[3],
		otp->serial_number[4], otp->serial_number[5],
		otp->serial_number[6], otp->serial_number[7],
		otp->serial_number[8], otp->serial_number[9],
		otp->serial_number[10], otp->serial_number[11],
		otp->serial_number[12], otp->serial_number[13],
		otp->serial_number[14], otp->serial_number[15]);
	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->serial_number failed");
		mnf->serial_number[0] = 0;
	}
	return;
}

void ov08f_read_otp_data(struct imgsensor_struct *pImgSensor)
{
	spImgSensor = pImgSensor;
	ov08f_iReadData(g_otpMemoryData);

	schkSumResult = ov08f_otp_do_checksum(g_otpMemoryData);

	ov08f_format_mnf_data(g_otpMemoryData, &mnf_info);
}

mot_calibration_status_t *SBUYA_OV08F_otp_get_calibration_status(void)
{
	return &calibration_status;
}

mot_calibration_mnf_t *SBUYA_OV08F_otp_get_mnf_info(void)
{
	return &mnf_info;
}

unsigned int ov08f_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	struct SBUYA_OV08F_otp_t *otp = (struct SBUYA_OV08F_otp_t*)g_otpMemoryData;
	g_pstI2CclientG = client;

	LOG_INF("addr = 0x%04x, size = %d", addr, size);
	if (size > (OV08F_OTP_SIZE)) {
		size = (OV08F_OTP_SIZE);
	}
	if (addr == 0x0 && size == 3819) {
		memcpy((void*)data, (void*)otp, size);
	} else if (addr == 0xeed && size == 1) {
		memcpy((void*)data, (void*)&calibration_status.lsc, size);
	} else if (addr == 0xeec && size == 1) {
		memcpy((void*)data, (void*)&schkSumResult, size);
	} else {
		LOG_INF("addr = 0x%04x, size = %d, wrong addr or size, read failed", addr, size);
	}
	return size;
}

EXPORT_SYMBOL(ov08f_read_region);
