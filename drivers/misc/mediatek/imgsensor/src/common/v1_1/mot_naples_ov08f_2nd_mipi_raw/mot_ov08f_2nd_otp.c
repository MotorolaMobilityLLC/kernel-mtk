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

#include "mot_ov08f_2nd_otp.h"
#include "mot_naples_ov08f_2nd_mipi_raw_Sensor.h"

#define LOG_INF(format, args...)    \
	pr_debug(PFX "[%s] " format, __func__, ##args)

static struct i2c_client *g_pstI2CclientG;
static struct imgsensor_struct *spImgSensor;
static int8_t schkSumResult = false;
static mot_calibration_status_t calibration_status = {STATUS_CRC_FAIL};
static mot_calibration_mnf_t mnf_info = {0};

#define OV08F_OTP_DEBUG  1
#define OV08F_OTP_SIZE 1928

unsigned char g_otpMemoryData_2nd[OV08F_OTP_SIZE] = {0};

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

static int32_t eeprom_util_check_sum(uint8_t *data, uint32_t size,uint32_t index)
{
	int32_t check_sum = 0,check_sum_cal = 0;
	int32_t check_sum_match = 0;

	for (; index < size; index++) {
		check_sum_cal += data[index];
	}
	check_sum_cal = (check_sum_cal % 255 ) + 1;
	check_sum = data[size];
	if( check_sum == check_sum_cal){
          check_sum_match = 1;
		  LOG_INF("eeprom_util_check_sum check success");
	}

	return check_sum_match;
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
	struct NAPLES_OV08F_otp_t *otp = (struct NAPLES_OV08F_otp_t*)data;

	if (!eeprom_util_check_crc16(otp->eeprom_table_revision, BASIC_INFO_SIZE, convert_crc(otp->manufacturing_data_chksum))) {
		LOG_INF("basic info CRC Failed!");
		return -1;
	} else {
		LOG_INF("basic info CRC success!");
		calibration_status.mnf = STATUS_OK;
	}

	if (!eeprom_util_check_sum(otp->flag_of_awb, AWB_SIZE,0)){
		LOG_INF("Awb chksum Failed!");
		return -1;
	} else {
		LOG_INF("Awb check success!");
		calibration_status.awb = STATUS_OK;
	}

	if (!eeprom_util_check_sum(otp->flag_of_mtk_lsc, MTK_LSC_SIZE,0)) {
		LOG_INF("MTK Lsc chksum Failed!");
		return -1;
	} else {
		LOG_INF("LSC check success!");
		calibration_status.lsc = STATUS_OK;
	}

	// if (!eeprom_util_check_sum(otp->flag_of_basic_infomation, ALL_DATA_SIZE,1)) {
	// 	LOG_INF("all data chksum Failed!");
	// 	return -1;
	// } else {
	// 	LOG_INF("all data chksum success!");
	// }

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

static u8 ov08f_otp_get_verify_group_num()
{
	u16 flag = 0;
	ov08f_sensor_otp_set_block(GROUP1_BLOCK_START_NUM);

	flag = read_cmos_sensor(0x00);

	if((flag & 0x3f)  == 0x3f){
		return 3;
	}else if((flag & 0x07)  == 0x07){
		return 2;
	}else if ((flag & 0x01)  == 0x01) {
		return 1;
	}

	return 0;
}

static u8 ov08f_otp_read_group(u8 block_start, u8 block_end, unsigned char *data)
{
	int i = 0;
	int j = block_start + 1;
	int index = 0;

	ov08f_sensor_otp_set_block(block_start);
	if (block_start == GROUP1_BLOCK_START_NUM) {
		for (i = 1; i < BLOCK_DATA_SIZE; i++) {
			data[index++] = read_cmos_sensor(i);
		}
	} else if (block_start == GROUP2_BLOCK_START_NUM) {
		for (i = 81; i < BLOCK_DATA_SIZE; i++) {
			data[index++] = read_cmos_sensor(i);
		}
	}  else if (block_start == GROUP3_BLOCK_START_NUM) {
		for (i = 33; i < BLOCK_DATA_SIZE; i++) {
			data[index++] = read_cmos_sensor(i);
		}
	}

	for ( ; j < block_end; ++j) {
		ov08f_sensor_otp_set_block(j);
		for (i = 0; i < BLOCK_DATA_SIZE; i++) {
			data[index++] = read_cmos_sensor(i);
		}
	}

	ov08f_sensor_otp_set_block(block_end);
	if (block_start == GROUP1_BLOCK_START_NUM) {
		for (i = 0; i < GROUP1_LAST_BLOCK_DATA_SIZE; i++) {
			data[index++] = read_cmos_sensor(i);
		}
	} else if (block_start == GROUP2_BLOCK_START_NUM) {
		for (i = 0; i < GROUP2_LAST_BLOCK_DATA_SIZE; i++) {
			data[index++] = read_cmos_sensor(i);
		}
	}  else if (block_start == GROUP3_BLOCK_START_NUM) {
		for (i = 0; i < GROUP3_LAST_BLOCK_DATA_SIZE; i++) {
			data[index++] = read_cmos_sensor(i);
		}
	}
	return 0;
}

int ov08f_iReadData_2nd(unsigned char *pinputdata)
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
	} else if (groupNum == 3) {
		LOG_INF("group3 vaild");
		i4RetValue = ov08f_otp_read_group(GROUP3_BLOCK_START_NUM,
			GROUP3_BLOCK_END_NUM, pinputdata);
		if (i4RetValue != 0) {
			LOG_INF("I2C iReadData failed!!\n");
			return -1;
		}
	}else {
		LOG_INF("UNKOWN OTP ERR");
		return -1;
	}

	return 0;
}

static void ov08f_format_mnf_data(void *data, mot_calibration_mnf_t *mnf)
{
	int ret = 0;
	struct NAPLES_OV08F_otp_t *otp = (struct NAPLES_OV08F_otp_t *)data;

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

	if (otp->lens_id[0] == 0x01){
		ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "S08101A");
	} else {
		ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "Unknown");
		LOG_INF("unknown lens_id");
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->lens_id failed");
		mnf->lens_id[0] = 0;
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Unknown");
		LOG_INF("unknown manufacturer_id");
	}else {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "TrulyOpto");
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

void ov08f_read_otp_data_2nd(struct imgsensor_struct *pImgSensor)
{
	spImgSensor = pImgSensor;

	ov08f_iReadData_2nd(g_otpMemoryData_2nd);

	schkSumResult = ov08f_otp_do_checksum(g_otpMemoryData_2nd);

	ov08f_format_mnf_data(g_otpMemoryData_2nd, &mnf_info);
}

mot_calibration_status_t *NAPLES_OV08F_otp_get_calibration_status_2nd(void)
{
	return &calibration_status;
}

mot_calibration_mnf_t *NAPLES_OV08F_otp_get_mnf_info_2nd(void)
{
	return &mnf_info;
}

unsigned int ov08f_2nd_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	struct NAPLES_OV08F_otp_t *otp = (struct NAPLES_OV08F_otp_t*)g_otpMemoryData_2nd;
	g_pstI2CclientG = client;

	LOG_INF("addr = 0x%04x, size = %d", addr, size);
	if (size > (OV08F_OTP_SIZE)) {
		size = (OV08F_OTP_SIZE);
	}
	if (addr == 0x0 && size == 1928) {
		memcpy((void*)data, (void*)otp, size);
	} else if (addr == 0x78B && size == 1) {
		memcpy((void*)data, (void*)&calibration_status.lsc, size);
	} else if (addr == 0x78A && size == 1) {
		memcpy((void*)data, (void*)&schkSumResult, size);
	} else {
		LOG_INF("addr = 0x%04x, size = %d, wrong addr or size, read failed", addr, size);
	}
	return size;
}

EXPORT_SYMBOL(ov08f_2nd_read_region);
