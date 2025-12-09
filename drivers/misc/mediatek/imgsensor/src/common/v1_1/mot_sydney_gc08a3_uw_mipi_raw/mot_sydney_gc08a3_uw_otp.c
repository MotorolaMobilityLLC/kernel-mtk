/*
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
#include "mot_sydney_gc08a3_uw_otp.h"

#define PFX "[MOTO_EEPROM gc08a3_uw-otp CAIHF]"
static int m_mot_camera_debug = 1;
#define LOG_INF(format, args...)        do { if (m_mot_camera_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define LOG_DBG(format, args...)        do { if (m_mot_camera_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)

#define LOG_ERR(format, args...) pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)


#define MOT_SYDNEY_GC08A3_OTP_DEBUG  1

#define I2C_ADDR  0x62

unsigned char uwtempData[UW_ALL_DATA_SIZE] = {0};
unsigned char uwgroupAllData[UW_ALL_DATA_SIZE] = {0};
kal_uint8 uw_checksum_module = 0;
kal_uint8 uw_checksum_awb = 0;
kal_uint8 uw_checksum_lsc = 0;
static mot_calibration_status_t calibration_status = {STATUS_CRC_FAIL};
static mot_calibration_mnf_t mnf_info = {0};
struct mot_sydney_gc08a3_uw_otp_t mot_sydney_gc08a3_uw_otp_info;

EXPORT_SYMBOL(mot_sydney_gc08a3_uw_otp_info);

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

static void mot_sydney_gc08a3_uw_otp_init(void)
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

static void mot_sydney_gc08a3_uw_otp_close(void)
{
	write_cmos_sensor_8bit(0x0316, 0x01);
	write_cmos_sensor_8bit(0x0a67, 0x00);
}

static kal_uint16 mot_sydney_gc08a3_uw_otp_read_group(kal_uint16 addr, kal_uint8 *data, kal_uint16 length)
{
	kal_uint16 i = 0;
	write_cmos_sensor_8bit(0x0313, 0x00);
	write_cmos_sensor_8bit(0x0a69, (addr >> 8) & 0xff);
	write_cmos_sensor_8bit(0x0a6a, addr & 0xff);
	write_cmos_sensor_8bit(0x0313, 0x20);
	write_cmos_sensor_8bit(0x0313, 0x12);

	for (i = 0; i < length; i++) {
		data[i] = read_cmos_sensor(0x0a6c);
		uwtempData[i]=data[i];
#if MOT_SYDNEY_GC08A3_OTP_DEBUG
	LOG_INF("read_group addr = 0x%x, data = 0x%x\n", addr + i * 8, data[i]);
#endif
	}

	return 0;
}

static kal_uint16 mot_sydney_gc08a3_uw_otp_read_byte(kal_uint16 addr)
{
	kal_uint16 val = 0;
	write_cmos_sensor_8bit(0x0313, 0x00);
	write_cmos_sensor_8bit(0x0a69, (addr >> 8) & 0xff);
	write_cmos_sensor_8bit(0x0a6a, addr & 0xff);
	write_cmos_sensor_8bit(0x0313, 0x20);
	val = read_cmos_sensor(0x0a6c);
#if MOT_SYDNEY_GC08A3_OTP_DEBUG
	LOG_INF("test addr = 0x%x, data = 0x%x\n", addr , val);
#endif
	return val;
}

static int mot_sydney_gc08a3_uw_iReadData(unsigned int ui4_offset, unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;
	int i4ResidueDataLength;
	u32 u4CurrentOffset;
	kal_uint8 *pBuff;

	i4ResidueDataLength = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;
	i4RetValue =mot_sydney_gc08a3_uw_otp_read_group((kal_uint16) u4CurrentOffset, pBuff, i4ResidueDataLength);
	if (i4RetValue != 0) {
		pr_debug(PFX,"I2C iReadData failed!!\n");
		return -1;
	}
	return 0;
}

static bool check_sum(kal_uint8 *data, unsigned int size, kal_uint8 chksum)
{
	int32_t check_sum_cal = 0;
	int32_t i = 0;

	for (i =0; i < size; i++) {
		check_sum_cal += data[i];
	}
	check_sum_cal = (check_sum_cal % 255 ) + 1;

	if( chksum == check_sum_cal){
		return true;
	}else{
		LOG_INF("chksum fail size = %d check_sum_cal=%d sum-in-eeprom=%d", size, check_sum_cal, chksum);
		return false;
	}
}

static int8_t  mot_sydney_gc08a3_uw_otp_checksum(void)
{

	// check crc16
	if (check_sum(&mot_sydney_gc08a3_uw_otp_info.module_param[0], UW_MODULE_LENGTH, mot_sydney_gc08a3_uw_otp_info.moduleChksum)) {
		LOG_INF("basic info check success!");
		uw_checksum_module = 1;
		calibration_status.mnf = STATUS_OK;
		//return -1;
	} else {
		LOG_INF("basic info check Failed!");
	}

	//for awb checksum
	if (check_sum(&mot_sydney_gc08a3_uw_otp_info.awb_param[0], UW_AWB_LENGTH, mot_sydney_gc08a3_uw_otp_info.awbChksum))
	{
		uw_checksum_awb = 1;
		LOG_INF("Awb check success!");
		calibration_status.awb = STATUS_OK;
	}else{
		LOG_INF("Awb check fail!");
		return -1;
	}

	//for lsc checksum
	if (check_sum(&mot_sydney_gc08a3_uw_otp_info.lsc_param[0], UW_LSC_LENGTH, mot_sydney_gc08a3_uw_otp_info.lscChksum))
	{
		uw_checksum_lsc = 1;
		LOG_INF("LSC check success!");
		calibration_status.lsc = STATUS_OK;
	}else{
		LOG_INF("LSC check fail!");
		return -1;
	}

	return 0;
}

static void gc08a3_uw_format_mnf_data(mot_calibration_mnf_t *mnf)
{
	int ret = 0;

	ret = snprintf(mnf->table_revision, MAX_CALIBRATION_STRING, "0x%x",
		mot_sydney_gc08a3_uw_otp_info.module_param[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->table_revision failed");
		mnf->table_revision[0] = 0;
	}

	ret = snprintf(mnf->mot_part_number, MAX_CALIBRATION_STRING, "%c%c%c%c%c%c%c%c",
		mot_sydney_gc08a3_uw_otp_info.module_param[3], mot_sydney_gc08a3_uw_otp_info.module_param[4],
		mot_sydney_gc08a3_uw_otp_info.module_param[5], mot_sydney_gc08a3_uw_otp_info.module_param[6],
		mot_sydney_gc08a3_uw_otp_info.module_param[7], mot_sydney_gc08a3_uw_otp_info.module_param[8],
		mot_sydney_gc08a3_uw_otp_info.module_param[9], mot_sydney_gc08a3_uw_otp_info.module_param[10]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->mot_part_number failed");
		mnf->mot_part_number[0] = 0;
	}

	ret = snprintf(mnf->actuator_id, MAX_CALIBRATION_STRING, "0x%x", mot_sydney_gc08a3_uw_otp_info.module_param[11]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->actuator_id failed");
		mnf->actuator_id[0] = 0;
	}

	if (mot_sydney_gc08a3_uw_otp_info.module_param[12] == 0x01){
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
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "SEGA");
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->integrator failed");
		mnf->integrator[0] = 0;
	}

	/*
	ret = snprintf(mnf->factory_id, MAX_CALIBRATION_STRING, "%c%c",
		mot_sydney_gc08a3_uw_otp_info.module_param[15], mot_sydney_gc08a3_uw_otp_info.module_param[16]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->factory_id failed");
		mnf->factory_id[0] = 0;
	}

	ret = snprintf(mnf->manufacture_line, MAX_CALIBRATION_STRING, "%u",
		mot_sydney_gc08a3_uw_otp_info.module_param[17]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->manufacture_line failed");
		mnf->manufacture_line[0] = 0;
	}

	ret = snprintf(mnf->manufacture_date, MAX_CALIBRATION_STRING, "20%u/%u/%u",
		mot_sydney_gc08a3_uw_otp_info.module_param[18], mot_sydney_gc08a3_uw_otp_info.module_param[19],
		mot_sydney_gc08a3_uw_otp_info.module_param[20]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->manufacture_date failed");
		mnf->manufacture_date[0] = 0;
	}

	ret = snprintf(mnf->serial_number, MAX_CALIBRATION_STRING, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		mot_sydney_gc08a3_uw_otp_info.module_param[21], mot_sydney_gc08a3_uw_otp_info.module_param[22],
		mot_sydney_gc08a3_uw_otp_info.module_param[23], mot_sydney_gc08a3_uw_otp_info.module_param[24],
		mot_sydney_gc08a3_uw_otp_info.module_param[25], mot_sydney_gc08a3_uw_otp_info.module_param[26],
		mot_sydney_gc08a3_uw_otp_info.module_param[27], mot_sydney_gc08a3_uw_otp_info.module_param[28],
		mot_sydney_gc08a3_uw_otp_info.module_param[29], mot_sydney_gc08a3_uw_otp_info.module_param[30],
		mot_sydney_gc08a3_uw_otp_info.module_param[31], mot_sydney_gc08a3_uw_otp_info.module_param[32],
		mot_sydney_gc08a3_uw_otp_info.module_param[33], mot_sydney_gc08a3_uw_otp_info.module_param[34],
		mot_sydney_gc08a3_uw_otp_info.module_param[35], mot_sydney_gc08a3_uw_otp_info.module_param[36]);
	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->serial_number failed");
		mnf->serial_number[0] = 0;
	}
	*/
	return;
}

bool check_mot_sydney_gc08a3_uw_otp(void)
{
	kal_uint8  groupflag;
	mot_sydney_gc08a3_uw_otp_init();

	groupflag = mot_sydney_gc08a3_uw_otp_read_byte(UW_MODULE_GROUP_FLAG);
	LOG_INF("mot_sydney_gc08a3_uw_otp_info.module_flag, = 0x%x", groupflag);

	//for module info otp read
	if ((groupflag & 0x37) == 0x37) {
		LOG_INF("group3_module, size %d", UW_MODULE_LENGTH);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP3_MODULE_INFO, UW_ALL_DATA_SIZE, &uwtempData[0]);
		memcpy(uwgroupAllData, uwtempData,sizeof(uwtempData));
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP3_MODULE_INFO, UW_MODULE_LENGTH, &mot_sydney_gc08a3_uw_otp_info.module_param[0]);
		mot_sydney_gc08a3_uw_otp_info.moduleChksum = mot_sydney_gc08a3_uw_otp_read_byte(UW_GROUP3_MODULE_CRC);
		mot_sydney_gc08a3_uw_otp_info.awb_flag = mot_sydney_gc08a3_uw_otp_read_byte(UW_GROUP3_AWB_FLAG);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP3_AWB_INFO, UW_AWB_LENGTH, &mot_sydney_gc08a3_uw_otp_info.awb_param[0]);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP3_AWB_INFO + UW_AWB_LENGTH * 8, 1, &mot_sydney_gc08a3_uw_otp_info.awbChksum);
		mot_sydney_gc08a3_uw_otp_info.lsc_flag = mot_sydney_gc08a3_uw_otp_read_byte(UW_GROUP3_LSC_FLAG);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP3_LSC_INFO, UW_LSC_LENGTH, &mot_sydney_gc08a3_uw_otp_info.lsc_param[0]);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP3_LSC_INFO + UW_LSC_LENGTH * 8, 1, &mot_sydney_gc08a3_uw_otp_info.lscChksum);
	} else if ((groupflag & 0x13) == 0x13) {
		LOG_INF("group2_module, size %d", UW_MODULE_LENGTH);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP2_MODULE_INFO, UW_ALL_DATA_SIZE, &uwtempData[0]);
		memcpy(uwgroupAllData, uwtempData,sizeof(uwtempData));
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP2_MODULE_INFO, UW_MODULE_LENGTH, &mot_sydney_gc08a3_uw_otp_info.module_param[0]);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP2_MODULE_CRC, 1, &mot_sydney_gc08a3_uw_otp_info.moduleChksum);
		mot_sydney_gc08a3_uw_otp_info.awb_flag = mot_sydney_gc08a3_uw_otp_read_byte(UW_GROUP2_AWB_FLAG);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP2_AWB_INFO, UW_AWB_LENGTH, &mot_sydney_gc08a3_uw_otp_info.awb_param[0]);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP2_AWB_INFO + UW_AWB_LENGTH * 8, 1, &mot_sydney_gc08a3_uw_otp_info.awbChksum);
		mot_sydney_gc08a3_uw_otp_info.lsc_flag = mot_sydney_gc08a3_uw_otp_read_byte(UW_GROUP2_LSC_FLAG);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP2_LSC_INFO, UW_LSC_LENGTH, &mot_sydney_gc08a3_uw_otp_info.lsc_param[0]);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP2_LSC_INFO + UW_LSC_LENGTH * 8, 1, &mot_sydney_gc08a3_uw_otp_info.lscChksum);
	} else if ((groupflag & 0x01) == 0x01) {
		LOG_INF("group1_module, size %d", UW_MODULE_LENGTH);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP1_MODULE_INFO, UW_ALL_DATA_SIZE, &uwtempData[0]);
		memcpy(uwgroupAllData, uwtempData,sizeof(uwtempData));
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP1_MODULE_INFO, UW_MODULE_LENGTH, &mot_sydney_gc08a3_uw_otp_info.module_param[0]);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP1_MODULE_CRC, 1, &mot_sydney_gc08a3_uw_otp_info.moduleChksum);
		mot_sydney_gc08a3_uw_otp_info.awb_flag = mot_sydney_gc08a3_uw_otp_read_byte(UW_GROUP1_AWB_FLAG);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP1_AWB_INFO, UW_AWB_LENGTH, &mot_sydney_gc08a3_uw_otp_info.awb_param[0]);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP1_AWB_INFO + UW_AWB_LENGTH * 8, 1, &mot_sydney_gc08a3_uw_otp_info.awbChksum);
		mot_sydney_gc08a3_uw_otp_info.lsc_flag = mot_sydney_gc08a3_uw_otp_read_byte(UW_GROUP1_LSC_FLAG);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP1_LSC_INFO, UW_LSC_LENGTH, &mot_sydney_gc08a3_uw_otp_info.lsc_param[0]);
		mot_sydney_gc08a3_uw_iReadData(UW_GROUP1_LSC_INFO + UW_LSC_LENGTH * 8, 1, &mot_sydney_gc08a3_uw_otp_info.lscChksum);
	} else if ((groupflag & 0x0f) == 0x00) {
		LOG_INF("module info is empty");
	} else {
		LOG_INF("invalid block module flag 0x%x", groupflag);
	}

	mot_sydney_gc08a3_uw_otp_checksum();

	gc08a3_uw_format_mnf_data(&mnf_info);

	mot_sydney_gc08a3_uw_otp_close();

	if (1 == (uw_checksum_module & uw_checksum_awb & uw_checksum_lsc))
	{
		return true;
	}
	else
	{
		LOG_INF("otp check fail");
		return false;
	}
}

mot_calibration_status_t *SYDNEY_GC08A3_uw_otp_get_calibration_status(void)
{
	return &calibration_status;
}

mot_calibration_mnf_t *SYDNEY_GC08A3_uw_otp_get_mnf_info(void)
{
	return &mnf_info;
}

unsigned int GC08A3_uw_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size)
{
    pr_err("<%s>%d:otp region addr = 0x%x, size = %d\n", __func__, __LINE__, addr, size);

	if(size > UW_ALL_DATA_SIZE){
		size = UW_ALL_DATA_SIZE;
	}

	if (1 == (uw_checksum_module & uw_checksum_awb & uw_checksum_lsc)){
		if (addr == 0x0 && size == UW_ALL_DATA_SIZE) {
			memcpy(data, uwgroupAllData,sizeof(uwgroupAllData));
		}
	}else{
		LOG_INF("otp gc08a3_read_region read fail ");
		return 0;
	}

    return size;
}

EXPORT_SYMBOL(GC08A3_uw_read_region);
//end 20220402 add for otp check