// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
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
#include <linux/compat.h>

#include "kd_camera_feature.h"
#include "cam_cal_config.h"
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"

#define EEPROM_DEBUG
#define MOTO_OB_VALUE 64
#define MOTO_WB_VALUE_BASE 64
#define MOT_AWB_RB_MIN_VALUE 200
#define MOT_AWB_G_MIN_VALUE 760
#define MOT_AWB_RBG_MAX_VALUE 880
#define MOT_AWB_GRGB_RATIO_MIN_1000TIMES 970
#define MOT_AWB_GRGB_RATIO_MAX_1000TIMES 1030
#define MOT_AF_ADDR 0x27
#define MOT_AF_DATA_SIZE 24
#define MOT_AWB_ADDR 0x41
#define MOT_AWB_DATA_SIZE 43
#define IDX_MAX_CAM_NUMBER 7 // refer to IHalsensor.h
#define MAX_EEPROM_LIST_NUMBER 32
#define MAX_temp(a,b,c) (a)>(b)?((a)>(c)?(a):(c)):((b)>(c)?(b):(c))
#define ABS(a,b) a>=b?(a-b):(b-a)
#undef E
#define E(__x__) (__x__)
extern struct STRUCT_CAM_CAL_CONFIG_STRUCT CAM_CAL_CONFIG_LIST;
#undef E
#define E(__x__) (&__x__)

#define MAX_TEMP(a,b,c)  (a)>(b)?((a)>(c)?(a):(c)):((b)>(c)?(b):(c))

/****************************************************************
 * Global variable
 ****************************************************************/

static struct STRUCT_CAM_CAL_CONFIG_STRUCT *cam_cal_config_list[] = {CAM_CAL_CONFIG_LIST};
static struct STRUCT_CAM_CAL_CONFIG_STRUCT *cam_cal_config;
static unsigned int last_sensor_id = 0xFFFFFFFF;
static unsigned short cam_cal_index = 0xFFFF;
static unsigned short cam_cal_number =
		sizeof(cam_cal_config_list)/sizeof(struct STRUCT_CAM_CAL_CONFIG_STRUCT *);

static unsigned char *mp_eeprom_preload[IDX_MAX_CAM_NUMBER];
static unsigned char *mp_layout_preload[IDX_MAX_CAM_NUMBER];

unsigned int show_cmd_error_log(enum ENUM_CAMERA_CAM_CAL_TYPE_ENUM cmd)
{
	error_log("Return %s\n", CamCalErrString[cmd]);
	return 0;
}

int get_mtk_format_version(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	int ret = 0;

	if (read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				0xFA3, 1, (unsigned char *) &ret) > 0)
		debug_log("MTK format version = 0x%02x\n", ret);
	else {
		error_log("Read Failed\n");
		ret = -1;
	}

	return ret;
}

int crc_reverse_byte(int data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

int32_t check_crc16(uint8_t  *data, uint32_t size, uint32_t ref_crc)
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

	debug_log("ref_crc 0x%x, crc 0x%x, crc_reverse 0x%x, matches? %d\n",
		ref_crc, crc, crc_reverse, crc_match);

	return crc_match;
}

unsigned int layout_check(struct EEPROM_DRV_FD_DATA *pdata,
				unsigned int sensorID)
{
	unsigned int header_offset = cam_cal_config->layout->header_addr;
	unsigned int check_id = 0x00000000;
	unsigned int result = CAM_CAL_ERR_NO_DEVICE;

	if (cam_cal_config->sensor_id == sensorID)
		debug_log("%s sensor_id matched\n", cam_cal_config->name);
	else {
		debug_log("%s sensor_id not matched\n", cam_cal_config->name);
		return result;
	}

	if (read_data_region(pdata, (u8 *)&check_id, header_offset, 4) != 4) {
		debug_log("header_id read failed\n");
		return result;
	}

	if (check_id == cam_cal_config->layout->header_id) {
		debug_log("header_id matched 0x%08x 0x%08x\n",
			check_id, cam_cal_config->layout->header_id);
		result = CAM_CAL_ERR_NO_ERR;
	} else
		debug_log("header_id not matched 0x%08x 0x%08x\n",
			check_id, cam_cal_config->layout->header_id);

	return result;
}

unsigned int layout_no_ck(struct EEPROM_DRV_FD_DATA *pdata,
				unsigned int sensorID)
{
	unsigned int header_offset = cam_cal_config->layout->header_addr;
	unsigned int check_id = 0x00000000;
	unsigned int result = CAM_CAL_ERR_NO_DEVICE;

	if (cam_cal_config->sensor_id == sensorID)
		debug_log("%s sensor_id matched\n", cam_cal_config->name);
	else {
		debug_log("%s sensor_id not matched\n", cam_cal_config->name);
		return result;
	}

	if (read_data_region(pdata, (u8 *)&check_id, header_offset, 4) != 4) {
		debug_log("header_id read failed 0x%08x 0x%08x (forced in)\n",
			check_id, cam_cal_config->layout->header_id);
		return result;
	}

	debug_log("header_id = 0x%08x\n", check_id);

	if (check_id == cam_cal_config->layout->header_id) {
		debug_log("header_id matched 0x%08x 0x%08x (skipped out)\n",
			check_id, cam_cal_config->layout->header_id);
	} else {
		debug_log("header_id not matched 0x%08x 0x%08x (forced in)\n",
			check_id, cam_cal_config->layout->header_id);
		result = CAM_CAL_ERR_NO_ERR;
	}

	result = CAM_CAL_ERR_NO_ERR;
	return result;
}

unsigned int mot_layout_no_ck(struct EEPROM_DRV_FD_DATA *pdata,
				unsigned int sensorId)
{
	unsigned int result = CAM_CAL_ERR_NO_DEVICE;
	if (cam_cal_config->sensor_id == sensorId) {
		debug_log("%s sensor_id matched\n", cam_cal_config->name);
		result = CAM_CAL_ERR_NO_ERR;
	} else {
		debug_log("%s sensor_id not matched\n", cam_cal_config->name);
		return result;
	}

	return result;
}

int  mot_check_awb_data(unsigned char* awb_data, int  size)
{
	int CalR = 1, CalGr = 1, CalGb = 1, CalB = 1;
	int FacR = 1, FacGr = 1, FacGb = 1, FacB = 1;
	int RGBGratioDeviation, checkSum;
	int rg_ratio_gold = 1, bg_ratio_gold = 1, grgb_ratio_gold = 1;
	int rg_ratio_unit = 1, bg_ratio_unit = 1, grgb_ratio_unit = 1;
	if(size != MOT_AWB_DATA_SIZE+2)
		return NONEXISTENCE;

	checkSum = awb_data[43]<<8 | awb_data[44];
	RGBGratioDeviation = awb_data[6];
	if(check_crc16(awb_data, 43, checkSum)) {
		debug_log("check_crc16 ok");
	} else {
		debug_log("check_crc16 err");
		return CRC_FAILURE;
	}

	//check ratio limt
	rg_ratio_unit = (awb_data[32]<<8 | awb_data[33])*1000/16384;
	bg_ratio_unit = (awb_data[34]<<8 | awb_data[35])*1000/16384;
	grgb_ratio_unit = (awb_data[36]<<8 | awb_data[37])*1000/16384;
	rg_ratio_gold = (awb_data[18]<<8 | awb_data[19])*1000/16384;
	bg_ratio_gold = (awb_data[20]<<8 | awb_data[21])*1000/16384;
	grgb_ratio_gold = (awb_data[22]<<8 | awb_data[23])*1000/16384;

	if(grgb_ratio_unit<MOT_AWB_GRGB_RATIO_MIN_1000TIMES || grgb_ratio_unit>MOT_AWB_GRGB_RATIO_MAX_1000TIMES
		|| grgb_ratio_gold<MOT_AWB_GRGB_RATIO_MIN_1000TIMES || grgb_ratio_gold>MOT_AWB_GRGB_RATIO_MAX_1000TIMES
		|| (ABS(rg_ratio_unit, rg_ratio_gold))>RGBGratioDeviation
		|| (ABS(bg_ratio_unit, bg_ratio_gold))>RGBGratioDeviation) {
		debug_log("ratio check err");
		return LIMIT_FAILURE;
	}

	CalR  = (awb_data[24]<<8 | awb_data[25])/64;
	CalGr = (awb_data[26]<<8 | awb_data[27])/64;
	CalGb = (awb_data[28]<<8 | awb_data[29])/64;
	CalB  = (awb_data[30]<<8 | awb_data[31])/64;

	if(CalR<MOT_AWB_RB_MIN_VALUE || CalR>MOT_AWB_RBG_MAX_VALUE
		|| CalGr<MOT_AWB_G_MIN_VALUE ||CalGr>MOT_AWB_RBG_MAX_VALUE
		|| CalGb<MOT_AWB_G_MIN_VALUE ||CalGb>MOT_AWB_RBG_MAX_VALUE
		|| CalB<MOT_AWB_RB_MIN_VALUE || CalB>MOT_AWB_RBG_MAX_VALUE) {
		debug_log("check unit R Gr Gb B limit error");
		return LIMIT_FAILURE;
	}

	FacR  = (awb_data[10]<<8 | awb_data[11])/64;
	FacGr = (awb_data[12]<<8 | awb_data[13])/64;
	FacGb = (awb_data[14]<<8 | awb_data[15])/64;
	FacB  = (awb_data[16]<<8 | awb_data[17])/64;

	if(FacR<MOT_AWB_RB_MIN_VALUE || FacR>MOT_AWB_RBG_MAX_VALUE
		|| FacGr<MOT_AWB_G_MIN_VALUE ||FacGr>MOT_AWB_RBG_MAX_VALUE
		|| FacGb<MOT_AWB_G_MIN_VALUE ||FacGb>MOT_AWB_RBG_MAX_VALUE
		|| FacB<MOT_AWB_RB_MIN_VALUE || FacB>MOT_AWB_RBG_MAX_VALUE) {
		debug_log("check gold R Gr Gb B limit error");
		return LIMIT_FAILURE;
	}

	return NO_ERRORS;
}

unsigned int mot_lens_id_to_name(uint8_t id,unsigned int block_size,unsigned char * lens_id,unsigned int err)
{
    int i=0,flag=0,ret=0;

    const struct STRUCT_MOT_LENS__ID lens_table[] =
    {
	{0x21,"38134A-400"},
	{0x22,"39411A-400"},
	{0xE0,"ZD0017J1"},
	{0x23,"39292B-400"},
	{0xE1,"XA-0216L-H5085"},
	{0x80,"505265A02"},
	{0x24,"39374A-400"},
	{0x25,"39449A-400"},
	{0x26,"39453A-400"},
	{0x27,"39454A-400"},
	{0x81,"505265C01"},
	{0x2F,"39454A-400"},
	{0x40,"39397A-400"},
	{0xE2,"1630A"},
	{0x41,"39395A-403"},
	{0x42,"39486A-400"},
	{0x43,"39374B-400"},
	{0x44,"39449B-400"},
	{0x45,"39495A-400"},
	{0x2E,"39453D-400"},
	{0x46,"39516A-400"},
	{0xE3,"ZC0018G1"},
	{0x82,"1086261A01"},
	{0x47,"39603A-400"},
	{0x36,"39453D-402"},
	{0x83,"506288A01-100"},
	{0x3E,"39453C-400"},
	{0x60,"39553A-400"},
	{0xE4,"ZD0017P4"},
	{0x2B,"39292C-400"},
	{0x48,"39397B-400"},
    };

    for (i =0;i <  sizeof(lens_table)/sizeof(struct STRUCT_MOT_LENS__ID);i++)
   {
      if(id == lens_table[i].lens_id)
      {
          ret = snprintf(lens_id, MAX_CALIBRATION_STRING, lens_table[i].lens_name);
          flag = 1;
          break;
      }
   }

   if (flag == 0)
   {
       ret = snprintf(lens_id, MAX_CALIBRATION_STRING, "Unknow");
   }

   if (ret < 0 || ret >= block_size)
   {
	debug_log("snprintf of mnf->lens_id failed");
	memset(lens_id, 0,MAX_CALIBRATION_STRING);
	err = CAM_CAL_ERR_NO_PARTNO;
   }
   return err;
}
unsigned int mot_do_manufacture_info(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];
	int read_data_size, checkSum, ret;
	uint8_t  tempBuf[39] = {0};

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size + 2, (unsigned char *)tempBuf);
	if (read_data_size <= 0) {
		err = CAM_CAL_ERR_NO_PARTNO;
		return err;
	}

	checkSum = (tempBuf[37]<<8) | (tempBuf[38]);

	if(check_crc16(tempBuf, 37, checkSum)) {
		debug_log("check_crc16 ok");
		err = CAM_CAL_ERR_NO_ERR;
	} else {
		debug_log("check_crc16 err");
		err = CAM_CAL_ERR_NO_PARTNO;
		return err;
	}
	 //eeprom_table_version
	ret = snprintf(pCamCalData->ManufactureData.eeprom_table_version, MAX_CALIBRATION_STRING, "0x%x", tempBuf[0]);

	if (ret < 0 || ret >= block_size) {
		debug_log("snprintf of mnf->eeprom_table_version failed");
		memset(pCamCalData->ManufactureData.eeprom_table_version, 0,
			sizeof(pCamCalData->ManufactureData.eeprom_table_version));
		err = CAM_CAL_ERR_NO_PARTNO;
	}

	//part_number
	ret = snprintf(pCamCalData->ManufactureData.part_number, MAX_CALIBRATION_STRING, "%c%c%c%c%c%c%c%c",
		tempBuf[3], tempBuf[4], tempBuf[5], tempBuf[6],
		tempBuf[7], tempBuf[8], tempBuf[9], tempBuf[10]);

	if (ret < 0 || ret >= block_size) {
		debug_log("snprintf of mnf->mot_part_number failed");
		memset(pCamCalData->ManufactureData.part_number, 0,
			sizeof(pCamCalData->ManufactureData.part_number));
		err = CAM_CAL_ERR_NO_PARTNO;
	}

	//actuator_id
	ret = snprintf(pCamCalData->ManufactureData.actuator_id, MAX_CALIBRATION_STRING, "0x%x", tempBuf[11]);

	if (ret < 0 || ret >= block_size) {
		debug_log("snprintf of mnf->actuator_id failed");
		memset(pCamCalData->ManufactureData.actuator_id, 0,
			sizeof(pCamCalData->ManufactureData.actuator_id));
		err = CAM_CAL_ERR_NO_PARTNO;
	}
	//lens_id
        err = mot_lens_id_to_name(tempBuf[12],block_size,pCamCalData->ManufactureData.lens_id,err);
	//manufacture id
	if(tempBuf[13] == 'T' && tempBuf[14] == 'S') {
		ret = snprintf(pCamCalData->ManufactureData.manufacturer_id, MAX_CALIBRATION_STRING, "Tianshi");
	} else if(tempBuf[13] == 'S' && tempBuf[14] == 'U') {
		ret = snprintf(pCamCalData->ManufactureData.manufacturer_id, MAX_CALIBRATION_STRING, "Sunny");
	} else if(tempBuf[13] == 'Q' && tempBuf[14] == 'T') {
		ret = snprintf(pCamCalData->ManufactureData.manufacturer_id, MAX_CALIBRATION_STRING, "Qtech");
	} else if(tempBuf[13] == 'S' && tempBuf[14] == 'W') {
		ret = snprintf(pCamCalData->ManufactureData.manufacturer_id, MAX_CALIBRATION_STRING, "Sunwin");
	} else if(tempBuf[13] == 'O' && tempBuf[14] == 'F') {
		ret = snprintf(pCamCalData->ManufactureData.manufacturer_id, MAX_CALIBRATION_STRING, "Ofilm");
	} else {
		ret = snprintf(pCamCalData->ManufactureData.manufacturer_id, MAX_CALIBRATION_STRING, "Unknow");
	}

	if (ret < 0 || ret >= block_size) {
		debug_log("snprintf of mnf->manufacturer_id failed");
		memset(pCamCalData->ManufactureData.manufacturer_id, 0,
			sizeof(pCamCalData->ManufactureData.manufacturer_id));
		err = CAM_CAL_ERR_NO_PARTNO;
	}

	//factory_id
	ret = snprintf(pCamCalData->ManufactureData.factory_id, MAX_CALIBRATION_STRING, "%c%c", tempBuf[15], tempBuf[16]);

	if (ret < 0 || ret >= block_size) {
		debug_log("snprintf of mnf->factory_id failed");
		memset(pCamCalData->ManufactureData.factory_id, 0,
			sizeof(pCamCalData->ManufactureData.factory_id));
		err = CAM_CAL_ERR_NO_PARTNO;
	}

	//manufacture_line
	ret = snprintf(pCamCalData->ManufactureData.manufacture_line, MAX_CALIBRATION_STRING, "0x%x", tempBuf[17]);

	if (ret < 0 || ret >= block_size) {
		debug_log("snprintf of mnf->manufacture_line failed");
		memset(pCamCalData->ManufactureData.manufacture_line, 0,
			sizeof(pCamCalData->ManufactureData.manufacture_line));
		err = CAM_CAL_ERR_NO_PARTNO;
	}

	//manufacture_date
	ret = snprintf(pCamCalData->ManufactureData.manufacture_date, MAX_CALIBRATION_STRING, "20%02u%02u%02u",
		tempBuf[18], tempBuf[19], tempBuf[20]);

	if (ret < 0 || ret >= block_size) {
		debug_log("snprintf of mnf->manufacture_date failed");
		memset(pCamCalData->ManufactureData.manufacture_date, 0,
			sizeof(pCamCalData->ManufactureData.manufacture_date));
		err = CAM_CAL_ERR_NO_PARTNO;
	}

	//serial_number
	ret = snprintf(pCamCalData->ManufactureData.serial_number, MAX_CALIBRATION_STRING,
		"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		tempBuf[21], tempBuf[22], tempBuf[23], tempBuf[24], tempBuf[25], tempBuf[26], tempBuf[27], tempBuf[28],
		tempBuf[29], tempBuf[30], tempBuf[31], tempBuf[32], tempBuf[33], tempBuf[34], tempBuf[35], tempBuf[36]);

	if (ret < 0 || ret >= block_size) {
		debug_log("snprintf of mnf->serial_number failed");
		memset(pCamCalData->ManufactureData.serial_number, 0,
			sizeof(pCamCalData->ManufactureData.serial_number));
		err = CAM_CAL_ERR_NO_PARTNO;
	}
#ifdef EEPROM_DEBUG
	debug_log("eeprom_table_version: %s\n", pCamCalData->ManufactureData.eeprom_table_version);
	debug_log("part_number: %s\n", pCamCalData->ManufactureData.part_number);
	debug_log("actuator_id: %s\n", pCamCalData->ManufactureData.actuator_id);
	debug_log("lens_id: %s\n", pCamCalData->ManufactureData.lens_id);
	debug_log("manufacturer_id: %s\n", pCamCalData->ManufactureData.manufacturer_id);
	debug_log("factory_id: %s\n", pCamCalData->ManufactureData.factory_id);
	debug_log("manufacture_line: %s\n", pCamCalData->ManufactureData.manufacture_line);
	debug_log("manufacture_date: %s\n", pCamCalData->ManufactureData.manufacture_date);
	debug_log("serial_number: %s\n", pCamCalData->ManufactureData.serial_number);
#endif

	return err;
}

/***********************************************************************************
 * Function : To read 2A information. Please put your AWB+AF data function, here.
 ***********************************************************************************/

unsigned int mot_do_2a_gain(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	int read_data_size, checkSum;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];

	unsigned char AWBAFConfig = 0x3; //set af awb enable
	unsigned short AFInf, AFMacro, AFInfDistance, AFMacroDistance;
	int RGBGratioDeviation, tempMax = 0;
	int CalR = 1, CalGr = 1, CalGb = 1, CalG = 1, CalB = 1;
	int FacR = 1, FacGr = 1, FacGb = 1, FacG = 1, FacB = 1;
	int RawCalR = 1, RawCalGr = 1, RawCalGb = 1, RawCalB = 1;
	int RawFacR = 1, RawFacGr = 1, RawFacGb = 1, RawFacB = 1;
	int rg_ratio_gold = 1, bg_ratio_gold = 1, grgb_ratio_gold = 1;
	int rg_ratio_unit = 1, bg_ratio_unit = 1, grgb_ratio_unit = 1;
	uint8_t  af_data[26] = {0};
	uint8_t  awb_data[45] = {0};
	int af_addr = MOT_AF_ADDR;
	int af_size = MOT_AF_DATA_SIZE;
	int awb_addr = MOT_AWB_ADDR;
	int awb_size = MOT_AWB_DATA_SIZE;

	debug_log("block_size=%d sensor_id=%x\n", block_size, pCamCalData->sensorID);

	memset((void *)&pCamCalData->Single2A, 0, sizeof(struct STRUCT_CAM_CAL_SINGLE_2A_STRUCT));

	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}
	if (block_size == 0) {
		error_log("block_size(%d) is not correct\n", block_size);
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}

	pCamCalData->Single2A.S2aVer = 0x01;
	pCamCalData->Single2A.S2aBitEn = (0x03 & AWBAFConfig);
	pCamCalData->Single2A.S2aAfBitflagEn = (0x0c & AWBAFConfig);

	/* AWB Calibration Data*/
	if (0x1 & AWBAFConfig) {
		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				awb_addr, awb_size + 2, (unsigned char *)awb_data);
		if (read_data_size > 0)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}

		checkSum = awb_data[43]<<8 | awb_data[44];
		RGBGratioDeviation = awb_data[6];
		debug_log("RGBGratioDeviation = %d", RGBGratioDeviation);
		if(check_crc16(awb_data, 43, checkSum)) {
			debug_log("check_crc16 ok");
			err = CAM_CAL_ERR_NO_ERR;
		} else {
			debug_log("check_crc16 err");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}
		//check ratio limt
		rg_ratio_unit = (awb_data[32]<<8 | awb_data[33])*1000/16384;
		bg_ratio_unit = (awb_data[34]<<8 | awb_data[35])*1000/16384;
		grgb_ratio_unit = (awb_data[36]<<8 | awb_data[37])*1000/16384;
		rg_ratio_gold = (awb_data[18]<<8 | awb_data[19])*1000/16384;
		bg_ratio_gold = (awb_data[20]<<8 | awb_data[21])*1000/16384;
		grgb_ratio_gold = (awb_data[22]<<8 | awb_data[23])*1000/16384;
		debug_log("ratio*1000, Unit R/G = %d, B/G = %d, Gr/Gb = %d, Gold R/G = %d, B/G = %d, Gr/Gb = %d",
			rg_ratio_unit, bg_ratio_unit, grgb_ratio_unit, rg_ratio_gold, bg_ratio_gold, grgb_ratio_gold);
		if(grgb_ratio_unit<MOT_AWB_GRGB_RATIO_MIN_1000TIMES || grgb_ratio_unit>MOT_AWB_GRGB_RATIO_MAX_1000TIMES
			|| grgb_ratio_gold<MOT_AWB_GRGB_RATIO_MIN_1000TIMES || grgb_ratio_gold>MOT_AWB_GRGB_RATIO_MAX_1000TIMES
			|| (ABS(rg_ratio_unit, rg_ratio_gold))>RGBGratioDeviation
			|| (ABS(bg_ratio_unit, bg_ratio_gold))>RGBGratioDeviation) {
			debug_log("ratio check err");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}

		RawCalR  = (awb_data[24]<<8 | awb_data[25]);
		RawCalGr = (awb_data[26]<<8 | awb_data[27]);
		RawCalGb = (awb_data[28]<<8 | awb_data[29]);
		RawCalB  = (awb_data[30]<<8 | awb_data[31]);

		CalR  = RawCalR / MOTO_WB_VALUE_BASE;
		CalGr = RawCalGr / MOTO_WB_VALUE_BASE;
		CalGb = RawCalGb / MOTO_WB_VALUE_BASE;
		CalB  = RawCalB / MOTO_WB_VALUE_BASE;

		if(CalR<MOT_AWB_RB_MIN_VALUE || CalR>MOT_AWB_RBG_MAX_VALUE
			|| CalGr<MOT_AWB_G_MIN_VALUE ||CalGr>MOT_AWB_RBG_MAX_VALUE
			|| CalGb<MOT_AWB_G_MIN_VALUE ||CalGb>MOT_AWB_RBG_MAX_VALUE
			|| CalB<MOT_AWB_RB_MIN_VALUE || CalB>MOT_AWB_RBG_MAX_VALUE) {
			debug_log("check unit R Gr Gb B limit error");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}

		//Let's use EEPROM programmed values instead actual values to improve accuracy here
#ifdef MOTO_OB_VALUE
		CalR  = RawCalR - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		CalGr = RawCalGr - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		CalGb = RawCalGb - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		CalB  = RawCalB - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
#else
		CalR  = RawCalR;
		CalGr = RawCalGr;
		CalGb = RawCalGb;
		CalB  = RawCalB;
#endif

		CalG = ((CalGr + CalGb) + 1) >> 1;

		debug_log("Unit R = %d, Gr= %d, Gb = %d, B = %d, G = %d", CalR/MOTO_WB_VALUE_BASE,
		          CalGr/MOTO_WB_VALUE_BASE, CalGb/MOTO_WB_VALUE_BASE, CalB/MOTO_WB_VALUE_BASE, CalG/MOTO_WB_VALUE_BASE);

		tempMax = MAX_temp(CalR,CalG,CalB);

		pCamCalData->Single2A.S2aAwb.rUnitGainu4R = (u32)((tempMax*512 + (CalR >> 1))/CalR);
		pCamCalData->Single2A.S2aAwb.rUnitGainu4G = (u32)((tempMax*512 + (CalG >> 1))/CalG);
		pCamCalData->Single2A.S2aAwb.rUnitGainu4B  = (u32)((tempMax*512 + (CalB >> 1))/CalB);

		RawFacR  = (awb_data[10]<<8 | awb_data[11]);
		RawFacGr = (awb_data[12]<<8 | awb_data[13]);
		RawFacGb = (awb_data[14]<<8 | awb_data[15]);
		RawFacB  = (awb_data[16]<<8 | awb_data[17]);

		FacR  = RawFacR / MOTO_WB_VALUE_BASE;
		FacGr = RawFacGr / MOTO_WB_VALUE_BASE;
		FacGb = RawFacGb / MOTO_WB_VALUE_BASE;
		FacB  = RawFacB / MOTO_WB_VALUE_BASE;

		if(FacR<MOT_AWB_RB_MIN_VALUE || FacR>MOT_AWB_RBG_MAX_VALUE
			|| FacGr<MOT_AWB_G_MIN_VALUE ||FacGr>MOT_AWB_RBG_MAX_VALUE
			|| FacGb<MOT_AWB_G_MIN_VALUE ||FacGb>MOT_AWB_RBG_MAX_VALUE
			|| FacB<MOT_AWB_RB_MIN_VALUE || FacB>MOT_AWB_RBG_MAX_VALUE) {
			debug_log("check gold R Gr Gb B limit error");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}

		//Let's use EEPROM programmed values instead actual values to improve accuracy here
#ifdef MOTO_OB_VALUE
		FacR  = RawFacR - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		FacGr = RawFacGr - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		FacGb = RawFacGb - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		FacB  = RawFacB - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
#else
		FacR  = RawFacR;
		FacGr = RawFacGr;
		FacGb = RawFacGb;
		FacB  = RawFacB;
#endif

		FacG = ((FacGr + FacGb) + 1) >> 1;

		debug_log("Gold R = %d, Gr= %d, Gb = %d, B = %d, G = %d", FacR/MOTO_WB_VALUE_BASE,
		          FacGr/MOTO_WB_VALUE_BASE, FacGb/MOTO_WB_VALUE_BASE, FacB/MOTO_WB_VALUE_BASE, FacG/MOTO_WB_VALUE_BASE);

		tempMax = MAX_temp(FacR,FacG,FacB);

		pCamCalData->Single2A.S2aAwb.rGoldGainu4R = (u32)((tempMax * 512 + (FacR >> 1)) /FacR);
		pCamCalData->Single2A.S2aAwb.rGoldGainu4G = (u32)((tempMax * 512 + (FacG >> 1)) /FacG);
		pCamCalData->Single2A.S2aAwb.rGoldGainu4B  = (u32)((tempMax * 512 + (FacB >> 1)) /FacB);

		pCamCalData->Single2A.S2aAwb.rValueR   = CalR/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rValueGr  = CalGr/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rValueGb  = CalGb/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rValueB   = CalB/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rGoldenR  = FacR/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rGoldenGr = FacGr/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rGoldenGb = FacGb/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rGoldenB  = FacB/MOTO_WB_VALUE_BASE;
		debug_log("======================AWB CAM_CAL==================\n");
		debug_log("[rCalGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rValueR);
		debug_log("[rCalGain.u4Gr] = %d\n", pCamCalData->Single2A.S2aAwb.rValueGr);
		debug_log("[rCalGain.u4Gb] = %d\n", pCamCalData->Single2A.S2aAwb.rValueGb);
		debug_log("[rCalGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rValueB);
		debug_log("[rFacGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldenR);
		debug_log("[rFacGain.u4Gr] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldenGr);
		debug_log("[rFacGain.u4Gb] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldenGb);
		debug_log("[rFacGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldenB);
		debug_log("[rCalGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4R);
		debug_log("[rCalGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4G);
		debug_log("[rCalGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4B);
		debug_log("[rFacGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4R);
		debug_log("[rFacGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4G);
		debug_log("[rFacGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4B);
		debug_log("======================AWB CAM_CAL==================\n");

	}
	/* AF Calibration Data*/
	if (0x2 & AWBAFConfig) {
		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				af_addr, af_size + 2, (unsigned char *)af_data);
		if (read_data_size > 0)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}

		checkSum = af_data[24]<<8 | af_data[25];
		if(check_crc16(af_data, 24, checkSum)) {
			debug_log("check_crc16 ok");
			err = CAM_CAL_ERR_NO_ERR;
		} else {
			debug_log("check_crc16 err");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}
		AFMacro = (af_data[2]<<8 | af_data[3])/64;
		AFInf = (af_data[6]<<8 | af_data[7])/64;
		AFInfDistance = af_data[4]<<8 | af_data[5];
		AFMacroDistance = af_data[0]<<8 | af_data[1];
		pCamCalData->Single2A.S2aAf[0] = AFInf;
		pCamCalData->Single2A.S2aAf[1] = AFMacro;
		pCamCalData->Single2A.S2aAF_t.AF_infinite_pattern_distance = AFInfDistance;
		pCamCalData->Single2A.S2aAF_t.AF_Macro_pattern_distance = AFMacroDistance;

		debug_log("======================AF CAM_CAL==================\n");
		debug_log("[AFInfDistance] = %dmm\n", AFInfDistance);
		debug_log("[AFMacroDistance] = %dmm\n", AFMacroDistance);
		debug_log("[AFInf] = %d\n", AFInf);
		debug_log("[AFMacro] = %d\n", AFMacro);
		debug_log("======================AF CAM_CAL==================\n");
	}

	if((pCamCalData->sensorID == MOT_DUBAI_OV50A_SENSOR_ID) ||
	   (pCamCalData->sensorID == MOT_AION_OV50A_SENSOR_ID)  ||
	   (pCamCalData->sensorID == MOT_MANAUS_OV50A_SENSOR_ID) ) {
		//af posture calibration data
		unsigned char AF_POSTURE[20];
		unsigned int af_posture_data_offset = 0x2313;
		unsigned int af_inf_posture, af_macro_posture, AF_infinite_calibration_temperature;
		memset(AF_POSTURE, 0, 20);
		debug_log("af_posture_data_offset = 0x%x\n", af_posture_data_offset);

		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				af_posture_data_offset, 20, (unsigned char *) AF_POSTURE);
		if (read_data_size > 0)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}

		checkSum = AF_POSTURE[18]<<8 | AF_POSTURE[19];
		if(check_crc16(AF_POSTURE, 18, checkSum)) {
			debug_log("check_crc16 ok");
			err = CAM_CAL_ERR_NO_ERR;
		} else {
			debug_log("check_crc16 err");
			err = CAM_CAL_NONE_BITEN;
			return err;
		}

		af_inf_posture = AF_POSTURE[10]<<8|AF_POSTURE[9];
		af_macro_posture = AF_POSTURE[12]<<8|AF_POSTURE[11];
		AF_infinite_calibration_temperature = AF_POSTURE[17];
		pCamCalData->Single2A.S2aAF_t.Posture_AF_infinite_calibration = af_inf_posture;
		pCamCalData->Single2A.S2aAF_t.Posture_AF_macro_calibration = af_macro_posture;
		pCamCalData->Single2A.S2aAF_t.AF_infinite_calibration_temperature = AF_infinite_calibration_temperature;

		debug_log("======================AF POSTURE CAM_CAL==================\n");
		debug_log("[AFInfPosture] = 0x%x\n", af_inf_posture);
		debug_log("[AFMacroPosture] = 0x%x\n", af_macro_posture);
		debug_log("[AFInfiniteCalibrationTemperature] = 0x%x\n", AF_infinite_calibration_temperature);
		debug_log("======================AF POSTURE CAM_CAL==================\n");
	}

	return err;
}


/***********************************************************************************
 * Function : To read AWB information. Please put your AWB data function, here.
 ***********************************************************************************/

unsigned int mot_do_awb_gain(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	int read_data_size, checkSum;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];

	unsigned char AWBConfig = 0x1;
	int RGBGratioDeviation, tempMax = 0;
	int CalR = 1, CalGr = 1, CalGb = 1, CalG = 1, CalB = 1;
	int FacR = 1, FacGr = 1, FacGb = 1, FacG = 1, FacB = 1;
	int RawCalR = 1, RawCalGr = 1, RawCalGb = 1, RawCalB = 1;
	int RawFacR = 1, RawFacGr = 1, RawFacGb = 1, RawFacB = 1;
	int rg_ratio_gold = 1, bg_ratio_gold = 1, grgb_ratio_gold = 1;
	int rg_ratio_unit = 1, bg_ratio_unit = 1, grgb_ratio_unit = 1;
	uint8_t  awb_data[45] = {0};
	int awb_addr = MOT_AWB_ADDR;
	int awb_size = MOT_AWB_DATA_SIZE;

	debug_log("block_size=%d sensor_id=%x\n", block_size, pCamCalData->sensorID);

	memset((void *)&pCamCalData->Single2A, 0, sizeof(struct STRUCT_CAM_CAL_SINGLE_2A_STRUCT));

	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}
	if (block_size == 0) {
		error_log("block_size(%d) is not correct\n", block_size);
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}

	pCamCalData->Single2A.S2aVer = 0x01;
	pCamCalData->Single2A.S2aBitEn = (0x01 & AWBConfig);

	/* AWB Calibration Data*/
	if (0x1 & AWBConfig) {
		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				awb_addr, awb_size + 2, (unsigned char *)awb_data);
		if (read_data_size > 0)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}

		checkSum = awb_data[43]<<8 | awb_data[44];
		RGBGratioDeviation = awb_data[6];
		debug_log("RGBGratioDeviation = %d", RGBGratioDeviation);
		if(check_crc16(awb_data, 43, checkSum)) {
			debug_log("check_crc16 ok");
			err = CAM_CAL_ERR_NO_ERR;
		} else {
			debug_log("check_crc16 err");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}
		//check ratio limt
		rg_ratio_unit = (awb_data[32]<<8 | awb_data[33])*1000/16384;
		bg_ratio_unit = (awb_data[34]<<8 | awb_data[35])*1000/16384;
		grgb_ratio_unit = (awb_data[36]<<8 | awb_data[37])*1000/16384;
		rg_ratio_gold = (awb_data[18]<<8 | awb_data[19])*1000/16384;
		bg_ratio_gold = (awb_data[20]<<8 | awb_data[21])*1000/16384;
		grgb_ratio_gold = (awb_data[22]<<8 | awb_data[23])*1000/16384;
		debug_log("ratio*1000, Unit R/G = %d, B/G = %d, Gr/Gb = %d, Gold R/G = %d, B/G = %d, Gr/Gb = %d",
			rg_ratio_unit, bg_ratio_unit, grgb_ratio_unit, rg_ratio_gold, bg_ratio_gold, grgb_ratio_gold);
		if(grgb_ratio_unit<MOT_AWB_GRGB_RATIO_MIN_1000TIMES || grgb_ratio_unit>MOT_AWB_GRGB_RATIO_MAX_1000TIMES
			|| grgb_ratio_gold<MOT_AWB_GRGB_RATIO_MIN_1000TIMES || grgb_ratio_gold>MOT_AWB_GRGB_RATIO_MAX_1000TIMES
			|| (ABS(rg_ratio_unit, rg_ratio_gold))>RGBGratioDeviation
			|| (ABS(bg_ratio_unit, bg_ratio_gold))>RGBGratioDeviation) {
			debug_log("ratio check err");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}

		RawCalR = (awb_data[24]<<8 | awb_data[25]);
		RawCalGr = (awb_data[26]<<8 | awb_data[27]);
		RawCalGb = (awb_data[28]<<8 | awb_data[29]);
		RawCalB = (awb_data[30]<<8 | awb_data[31]);

		CalR  = RawCalR / MOTO_WB_VALUE_BASE;
		CalGr = RawCalGr / MOTO_WB_VALUE_BASE;
		CalGb = RawCalGb / MOTO_WB_VALUE_BASE;
		CalB  = RawCalB / MOTO_WB_VALUE_BASE;

		if(CalR<MOT_AWB_RB_MIN_VALUE || CalR>MOT_AWB_RBG_MAX_VALUE
			|| CalGr<MOT_AWB_G_MIN_VALUE ||CalGr>MOT_AWB_RBG_MAX_VALUE
			|| CalGb<MOT_AWB_G_MIN_VALUE ||CalGb>MOT_AWB_RBG_MAX_VALUE
			|| CalB<MOT_AWB_RB_MIN_VALUE || CalB>MOT_AWB_RBG_MAX_VALUE) {
			debug_log("check unit R Gr Gb B limit error");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}

#ifdef MOTO_OB_VALUE
		CalR  = RawCalR - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		CalGr = RawCalGr - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		CalGb = RawCalGb - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		CalB  = RawCalB - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
#else
		CalR  = RawCalR;
		CalGr = RawCalGr;
		CalGb = RawCalGb;
		CalB  = RawCalB;
#endif

		CalG = ((CalGr + CalGb) + 1) >> 1;

		debug_log("Unit R = %d, Gr= %d, Gb = %d, B = %d, G = %d", CalR/MOTO_WB_VALUE_BASE,
		          CalGr/MOTO_WB_VALUE_BASE, CalGb/MOTO_WB_VALUE_BASE, CalB/MOTO_WB_VALUE_BASE, CalG/MOTO_WB_VALUE_BASE);
		tempMax = MAX_temp(CalR,CalG,CalB);

		pCamCalData->Single2A.S2aAwb.rUnitGainu4R = (u32)((tempMax*512 + (CalR >> 1))/CalR);
		pCamCalData->Single2A.S2aAwb.rUnitGainu4G = (u32)((tempMax*512 + (CalG >> 1))/CalG);
		pCamCalData->Single2A.S2aAwb.rUnitGainu4B  = (u32)((tempMax*512 + (CalB >> 1))/CalB);

		RawFacR  = (awb_data[10]<<8 | awb_data[11]);
		RawFacGr = (awb_data[12]<<8 | awb_data[13]);
		RawFacGb = (awb_data[14]<<8 | awb_data[15]);
		RawFacB  = (awb_data[16]<<8 | awb_data[17]);

		FacR  = RawFacR / MOTO_WB_VALUE_BASE;
		FacGr = RawFacGr / MOTO_WB_VALUE_BASE;
		FacGb = RawFacGb / MOTO_WB_VALUE_BASE;
		FacB  = RawFacB / MOTO_WB_VALUE_BASE;

		if(FacR<MOT_AWB_RB_MIN_VALUE || FacR>MOT_AWB_RBG_MAX_VALUE
			|| FacGr<MOT_AWB_G_MIN_VALUE ||FacGr>MOT_AWB_RBG_MAX_VALUE
			|| FacGb<MOT_AWB_G_MIN_VALUE ||FacGb>MOT_AWB_RBG_MAX_VALUE
			|| FacB<MOT_AWB_RB_MIN_VALUE || FacB>MOT_AWB_RBG_MAX_VALUE) {
			debug_log("check gold R Gr Gb B limit error");
			err = CAM_CAL_ERR_NO_3A_GAIN;
			return err;
		}

#ifdef MOTO_OB_VALUE
		FacR  = RawFacR - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		FacGr = RawFacGr - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		FacGb = RawFacGb - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
		FacB  = RawFacB - MOTO_OB_VALUE * MOTO_WB_VALUE_BASE;
#else
		FacR  = RawFacR;
		FacGr = RawFacGr;
		FacGb = RawFacGb;
		FacB  = RawFacB;
#endif
		FacG = ((FacGr + FacGb) + 1) >> 1;
		debug_log("Gold R = %d, Gr= %d, Gb = %d, B = %d, G = %d", FacR/MOTO_WB_VALUE_BASE,
		          FacGr/MOTO_WB_VALUE_BASE, FacGb/MOTO_WB_VALUE_BASE, FacB/MOTO_WB_VALUE_BASE, FacG/MOTO_WB_VALUE_BASE);
		tempMax = MAX_temp(FacR,FacG,FacB);

		pCamCalData->Single2A.S2aAwb.rGoldGainu4R = (u32)((tempMax * 512 + (FacR >> 1)) /FacR);
		pCamCalData->Single2A.S2aAwb.rGoldGainu4G = (u32)((tempMax * 512 + (FacG >> 1)) /FacG);
		pCamCalData->Single2A.S2aAwb.rGoldGainu4B  = (u32)((tempMax * 512 + (FacB >> 1)) /FacB);

		pCamCalData->Single2A.S2aAwb.rValueR   = CalR/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rValueGr  = CalGr/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rValueGb  = CalGb/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rValueB   = CalB/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rGoldenR  = FacR/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rGoldenGr = FacGr/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rGoldenGb = FacGb/MOTO_WB_VALUE_BASE;
		pCamCalData->Single2A.S2aAwb.rGoldenB  = FacB/MOTO_WB_VALUE_BASE;
		debug_log("======================AWB CAM_CAL==================\n");
		debug_log("[rCalGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rValueR);
		debug_log("[rCalGain.u4Gr] = %d\n", pCamCalData->Single2A.S2aAwb.rValueGr);
		debug_log("[rCalGain.u4Gb] = %d\n", pCamCalData->Single2A.S2aAwb.rValueGb);
		debug_log("[rCalGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rValueB);
		debug_log("[rFacGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldenR);
		debug_log("[rFacGain.u4Gr] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldenGr);
		debug_log("[rFacGain.u4Gb] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldenGb);
		debug_log("[rFacGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldenB);
		debug_log("[rCalGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4R);
		debug_log("[rCalGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4G);
		debug_log("[rCalGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4B);
		debug_log("[rFacGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4R);
		debug_log("[rFacGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4G);
		debug_log("[rFacGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4B);
		debug_log("======================AWB CAM_CAL==================\n");

	}
	return err;
}

/***********************************************************************************
 * Function : To read LSC Table
 ***********************************************************************************/
unsigned int mot_do_single_lsc(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	int read_data_size, checkSum;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];
	uint8_t  tempBuf[1870] = {0};

	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}
	if (block_size != CAM_CAL_SINGLE_LSC_SIZE)
		error_log("block_size(%d) is not match (%d)\n",
				block_size, CAM_CAL_SINGLE_LSC_SIZE);

	pCamCalData->SingleLsc.LscTable.MtkLcsData.MtkLscType = 2;//mtk type
	pCamCalData->SingleLsc.LscTable.MtkLcsData.PixId = 8;

	debug_log("u4Offset=0x%x, u4Length=0x%x", start_addr,  block_size);
	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size + 2, (unsigned char *) tempBuf);

	if (read_data_size <= 0) {
		err = CAM_CAL_ERR_NO_SHADING;
		return err;
	}

	checkSum = (tempBuf[1868]<<8) | (tempBuf[1869]);

	if(check_crc16(tempBuf, 1868, checkSum)) {
		debug_log("check_crc16 ok");
		err = CAM_CAL_ERR_NO_ERR;
	} else {
		debug_log("check_crc16 err");
		err = CAM_CAL_ERR_NO_SHADING;
		return err;
	}

	pCamCalData->SingleLsc.LscTable.MtkLcsData.TableSize = block_size;
	if (block_size > 0) {
		pCamCalData->SingleLsc.TableRotation = 0;
		read_data_size = read_data(pdata,
			pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size, (unsigned char *)
			&pCamCalData->SingleLsc.LscTable.MtkLcsData.SlimLscType);
		if (block_size == read_data_size)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			error_log("Read Failed\n");
			err = CamCalReturnErr[pCamCalData->Command];
			show_cmd_error_log(pCamCalData->Command);
		}
	}

	debug_log("======================SingleLsc Data==================\n");
	debug_log("[1st] = %x, %x, %x, %x\n",
		pCamCalData->SingleLsc.LscTable.Data[0],
		pCamCalData->SingleLsc.LscTable.Data[1],
		pCamCalData->SingleLsc.LscTable.Data[2],
		pCamCalData->SingleLsc.LscTable.Data[3]);
	debug_log("[1st] = SensorLSC(1)?MTKLSC(2)?  %x\n",
		pCamCalData->SingleLsc.LscTable.MtkLcsData.MtkLscType);
	debug_log("CapIspReg =0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[0],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[1],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[2],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[3],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[4]);
	debug_log("RETURN = 0x%x\n", err);
	debug_log("======================SingleLsc Data==================\n");

	return err;
}

unsigned int mot_do_pdaf(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	int read_data_size, checkSum1, checkSum2;
	int err =  CamCalReturnErr[pCamCalData->Command];
	uint8_t  tempBuf[1504] = {0};

	pCamCalData->PDAF.Size_of_PDAF = block_size;
	debug_log("PDAF start_addr =%x table_size=%d\n", start_addr, block_size);

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size + 4, (unsigned char *)tempBuf);
	if (read_data_size <= 0) {
		err = CAM_CAL_ERR_NO_PDAF;
		return err;
	}
	checkSum1 = tempBuf[1500] << 8 | tempBuf[1501];
	checkSum2 = tempBuf[1502] << 8 | tempBuf[1503];
	debug_log("checkSum1  = 0x%x, checkSum2 = 0x%x", checkSum1, checkSum2);

	if(check_crc16(tempBuf, 496, checkSum1) && check_crc16(tempBuf +496, 1004, checkSum2)) {
		debug_log("check_crc16 ok");
		err = CAM_CAL_ERR_NO_ERR;
	} else {
		debug_log("check_crc16 err");
		err = CAM_CAL_ERR_NO_PDAF;
		return err;
	}

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size, (unsigned char *)&pCamCalData->PDAF.Data[0]);
	if (read_data_size <= 0) {
		err = CAM_CAL_ERR_NO_PDAF;
		return err;
	}

	debug_log("======================PDAF Data==================\n");
	debug_log("First five %x, %x, %x, %x, %x\n",
		pCamCalData->PDAF.Data[0],
		pCamCalData->PDAF.Data[1],
		pCamCalData->PDAF.Data[2],
		pCamCalData->PDAF.Data[3],
		pCamCalData->PDAF.Data[4]);
	debug_log("RETURN = 0x%x\n", err);
	debug_log("======================PDAF Data==================\n");

	return err;

}

unsigned int do_module_version(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	(void) pdata;
	(void) start_addr;
	(void) block_size;
	(void) pGetSensorCalData;

	return CAM_CAL_ERR_NO_ERR;
}

unsigned int do_part_number(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];
	unsigned int size_limit = sizeof(pCamCalData->PartNumber);

	memset(&pCamCalData->PartNumber[0], 0, size_limit);

	if (block_size > size_limit) {
		error_log("part number size can't larger than %u\n", size_limit);
		return err;
	}

	if (read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size, (unsigned char *)&pCamCalData->PartNumber[0]) > 0)
		err = CAM_CAL_ERR_NO_ERR;
	else {
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
	}

	debug_log("======================Part Number==================\n");
	debug_log("[Part Number] = %x %x %x %x\n",
			pCamCalData->PartNumber[0], pCamCalData->PartNumber[1],
			pCamCalData->PartNumber[2], pCamCalData->PartNumber[3]);
	debug_log("[Part Number] = %x %x %x %x\n",
			pCamCalData->PartNumber[4], pCamCalData->PartNumber[5],
			pCamCalData->PartNumber[6], pCamCalData->PartNumber[7]);
	debug_log("[Part Number] = %x %x %x %x\n",
			pCamCalData->PartNumber[8], pCamCalData->PartNumber[9],
			pCamCalData->PartNumber[10], pCamCalData->PartNumber[11]);
	debug_log("======================Part Number==================\n");

	return err;
}


/***********************************************************************************
 * Function : To read 2A information. Please put your AWB+AF data function, here.
 ***********************************************************************************/

unsigned int do_2a_gain(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	int read_data_size;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];

	unsigned int CalGain = 0, FacGain = 0;
	unsigned char AWBAFConfig = 0;

	unsigned short AFInf = 0, AFMacro = 0;
	int tempMax = 0;
	int CalR = 1, CalGr = 1, CalGb = 1, CalG = 1, CalB = 1;
	int FacR = 1, FacGr = 1, FacGb = 1, FacG = 1, FacB = 1;

	debug_log("block_size=%d sensor_id=%x\n", block_size, pCamCalData->sensorID);
	memset((void *)&pCamCalData->Single2A, 0, sizeof(struct STRUCT_CAM_CAL_SINGLE_2A_STRUCT));
	/* Check rule */
	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}
	if (block_size != 14) {
		error_log("block_size(%d) is not correct (%d)\n", block_size, 14);
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}
	/* Check AWB & AF enable bit */
	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr + 1, 1, (unsigned char *)&AWBAFConfig);
	if (read_data_size > 0)
		err = CAM_CAL_ERR_NO_ERR;
	else {
		pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
	}
	pCamCalData->Single2A.S2aVer = 0x01;
	pCamCalData->Single2A.S2aBitEn = (0x03 & AWBAFConfig);
	debug_log("S2aBitEn=0x%02x", pCamCalData->Single2A.S2aBitEn);
	if (get_mtk_format_version(pdata, pGetSensorCalData) >= 0x18)
		if (0x2 & AWBAFConfig)
			pCamCalData->Single2A.S2aAfBitflagEn = 0x0C;
		else
			pCamCalData->Single2A.S2aAfBitflagEn = 0x00;
	else
		pCamCalData->Single2A.S2aAfBitflagEn = (0x0C & AWBAFConfig);
	/* AWB Calibration Data*/
	if (0x1 & AWBAFConfig) {
		/* AWB Unit Gain (5100K) */
		debug_log("5100K AWB\n");
		pCamCalData->Single2A.S2aAwb.rGainSetNum = 0;
		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				start_addr + 2, 4, (unsigned char *)&CalGain);
		if (read_data_size > 0)	{
			debug_log("Read CalGain OK %x\n", read_data_size);
			CalR  = CalGain & 0xFF;
			CalGr = (CalGain >> 8) & 0xFF;
			CalGb = (CalGain >> 16) & 0xFF;
			CalG  = ((CalGr + CalGb) + 1) >> 1;
			CalB  = (CalGain >> 24) & 0xFF;
			if (CalR > CalG)
				/* R > G */
				if (CalR > CalB)
					tempMax = CalR;
				else
					tempMax = CalB;
			else
				/* G > R */
				if (CalG > CalB)
					tempMax = CalG;
				else
					tempMax = CalB;
			debug_log(
				"UnitR:%d, UnitG:%d, UnitB:%d, New Unit Max=%d",
				CalR, CalG, CalB, tempMax);
			err = CAM_CAL_ERR_NO_ERR;
		} else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read CalGain Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}
		if (CalGain != 0x00000000 &&
			CalGain != 0xFFFFFFFF &&
			CalR    != 0x00000000 &&
			CalG    != 0x00000000 &&
			CalB    != 0x00000000) {
			pCamCalData->Single2A.S2aAwb.rGainSetNum++;
			pCamCalData->Single2A.S2aAwb.rUnitGainu4R =
					(unsigned int)((tempMax * 512 + (CalR >> 1)) / CalR);
			pCamCalData->Single2A.S2aAwb.rUnitGainu4G =
					(unsigned int)((tempMax * 512 + (CalG >> 1)) / CalG);
			pCamCalData->Single2A.S2aAwb.rUnitGainu4B =
					(unsigned int)((tempMax * 512 + (CalB >> 1)) / CalB);
		} else
			error_log(
			"There are something wrong on EEPROM, plz contact module vendor!!\n");
		/* AWB Golden Gain (5100K) */
		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				start_addr + 6, 4, (unsigned char *)&FacGain);
		if (read_data_size > 0)	{
			debug_log("Read FacGain OK\n");
			FacR  = FacGain & 0xFF;
			FacGr = (FacGain >> 8) & 0xFF;
			FacGb = (FacGain >> 16) & 0xFF;
			FacG  = ((FacGr + FacGb) + 1) >> 1;
			FacB  = (FacGain >> 24) & 0xFF;
			if (FacR > FacG)
				if (FacR > FacB)
					tempMax = FacR;
				else
					tempMax = FacB;
			else
				if (FacG > FacB)
					tempMax = FacG;
				else
					tempMax = FacB;
			debug_log(
				"GoldenR:%d, GoldenG:%d, GoldenB:%d, New Golden Max=%d",
				FacR, FacG, FacB, tempMax);
			err = CAM_CAL_ERR_NO_ERR;
		} else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read FacGain Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}
		if (FacGain != 0x00000000 &&
			FacGain != 0xFFFFFFFF &&
			FacR    != 0x00000000 &&
			FacG    != 0x00000000 &&
			FacB    != 0x00000000)	{
			pCamCalData->Single2A.S2aAwb.rGoldGainu4R =
					(unsigned int)((tempMax * 512 + (FacR >> 1)) / FacR);
			pCamCalData->Single2A.S2aAwb.rGoldGainu4G =
					(unsigned int)((tempMax * 512 + (FacG >> 1)) / FacG);
			pCamCalData->Single2A.S2aAwb.rGoldGainu4B =
					(unsigned int)((tempMax * 512 + (FacB >> 1)) / FacB);
		} else
			error_log(
			"There are something wrong on EEPROM, plz contact module vendor!!\n");
		/* Set AWB to 3A Layer */
		pCamCalData->Single2A.S2aAwb.rValueR   = CalR;
		pCamCalData->Single2A.S2aAwb.rValueGr  = CalGr;
		pCamCalData->Single2A.S2aAwb.rValueGb  = CalGb;
		pCamCalData->Single2A.S2aAwb.rValueB   = CalB;
		pCamCalData->Single2A.S2aAwb.rGoldenR  = FacR;
		pCamCalData->Single2A.S2aAwb.rGoldenGr = FacGr;
		pCamCalData->Single2A.S2aAwb.rGoldenGb = FacGb;
		pCamCalData->Single2A.S2aAwb.rGoldenB  = FacB;

		debug_log("======================AWB CAM_CAL==================\n");
		debug_log("[CalGain] = 0x%x\n", CalGain);
		debug_log("[FacGain] = 0x%x\n", FacGain);
		debug_log("[rCalGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4R);
		debug_log("[rCalGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4G);
		debug_log("[rCalGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4B);
		debug_log("[rFacGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4R);
		debug_log("[rFacGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4G);
		debug_log("[rFacGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4B);
		debug_log("======================AWB CAM_CAL==================\n");

		if (get_mtk_format_version(pdata, pGetSensorCalData) >= 0x22) {
			/* AWB Unit Gain (3100K) */
			debug_log("3100K AWB\n");
			CalR = CalGr = CalGb = CalG = CalB = 0;
			tempMax = 0;
			read_data_size = read_data(pdata,
					pCamCalData->sensorID, pCamCalData->deviceID,
					0x14FE, 4, (unsigned char *)&CalGain);
			if (read_data_size > 0)	{
				debug_log("Read CalGain OK %x\n", read_data_size);
				CalR  = CalGain & 0xFF;
				CalGr = (CalGain >> 8) & 0xFF;
				CalGb = (CalGain >> 16) & 0xFF;
				CalG  = ((CalGr + CalGb) + 1) >> 1;
				CalB  = (CalGain >> 24) & 0xFF;
				if (CalR > CalG)
					/* R > G */
					if (CalR > CalB)
						tempMax = CalR;
					else
						tempMax = CalB;
				else
					/* G > R */
					if (CalG > CalB)
						tempMax = CalG;
					else
						tempMax = CalB;
				debug_log(
					"UnitR:%d, UnitG:%d, UnitB:%d, New Unit Max=%d",
					CalR, CalG, CalB, tempMax);
				err = CAM_CAL_ERR_NO_ERR;
			} else {
				pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
				error_log("Read CalGain Failed\n");
				show_cmd_error_log(pCamCalData->Command);
			}
			if (CalGain != 0x00000000 &&
				CalGain != 0xFFFFFFFF &&
				CalR    != 0x00000000 &&
				CalG    != 0x00000000 &&
				CalB    != 0x00000000) {
				pCamCalData->Single2A.S2aAwb.rGainSetNum++;
				pCamCalData->Single2A.S2aAwb.rUnitGainu4R_low =
					(unsigned int)((tempMax * 512 + (CalR >> 1)) / CalR);
				pCamCalData->Single2A.S2aAwb.rUnitGainu4G_low =
					(unsigned int)((tempMax * 512 + (CalG >> 1)) / CalG);
				pCamCalData->Single2A.S2aAwb.rUnitGainu4B_low =
					(unsigned int)((tempMax * 512 + (CalB >> 1)) / CalB);
			} else
				error_log(
			"There are something wrong on EEPROM, plz contact module vendor!!\n");
			/* AWB Golden Gain (3100K) */
			FacR = FacGr = FacGb = FacG = FacB = 0;
			tempMax = 0;
			read_data_size = read_data(pdata,
					pCamCalData->sensorID, pCamCalData->deviceID,
					0x1502, 4, (unsigned char *)&FacGain);
			if (read_data_size > 0)	{
				debug_log("Read FacGain OK\n");
				FacR  = FacGain & 0xFF;
				FacGr = (FacGain >> 8) & 0xFF;
				FacGb = (FacGain >> 16) & 0xFF;
				FacG  = ((FacGr + FacGb) + 1) >> 1;
				FacB  = (FacGain >> 24) & 0xFF;
				if (FacR > FacG)
					if (FacR > FacB)
						tempMax = FacR;
					else
						tempMax = FacB;
				else
					if (FacG > FacB)
						tempMax = FacG;
					else
						tempMax = FacB;
				debug_log(
					"GoldenR:%d, GoldenG:%d, GoldenB:%d, New Golden Max=%d",
					FacR, FacG, FacB, tempMax);
				err = CAM_CAL_ERR_NO_ERR;
			} else {
				pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
				error_log("Read FacGain Failed\n");
				show_cmd_error_log(pCamCalData->Command);
			}
			if (FacGain != 0x00000000 &&
				FacGain != 0xFFFFFFFF &&
				FacR    != 0x00000000 &&
				FacG    != 0x00000000 &&
				FacB    != 0x00000000)	{
				pCamCalData->Single2A.S2aAwb.rGoldGainu4R_low =
					(unsigned int)((tempMax * 512 + (FacR >> 1)) / FacR);
				pCamCalData->Single2A.S2aAwb.rGoldGainu4G_low =
					(unsigned int)((tempMax * 512 + (FacG >> 1)) / FacG);
				pCamCalData->Single2A.S2aAwb.rGoldGainu4B_low =
					(unsigned int)((tempMax * 512 + (FacB >> 1)) / FacB);
			} else
				error_log(
			"There are something wrong on EEPROM, plz contact module vendor!!\n");
			/* AWB Unit Gain (4000K) */
			debug_log("4000K AWB\n");
			CalR = CalGr = CalGb = CalG = CalB = 0;
			tempMax = 0;
			read_data_size = read_data(pdata,
					pCamCalData->sensorID, pCamCalData->deviceID,
					0x1506, 4, (unsigned char *)&CalGain);
			if (read_data_size > 0)	{
				debug_log("Read CalGain OK %x\n", read_data_size);
				CalR  = CalGain & 0xFF;
				CalGr = (CalGain >> 8) & 0xFF;
				CalGb = (CalGain >> 16) & 0xFF;
				CalG  = ((CalGr + CalGb) + 1) >> 1;
				CalB  = (CalGain >> 24) & 0xFF;
				if (CalR > CalG)
					/* R > G */
					if (CalR > CalB)
						tempMax = CalR;
					else
						tempMax = CalB;
				else
					/* G > R */
					if (CalG > CalB)
						tempMax = CalG;
					else
						tempMax = CalB;
				debug_log(
					"UnitR:%d, UnitG:%d, UnitB:%d, New Unit Max=%d",
					CalR, CalG, CalB, tempMax);
				err = CAM_CAL_ERR_NO_ERR;
			} else {
				pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
				error_log("Read CalGain Failed\n");
				show_cmd_error_log(pCamCalData->Command);
			}
			if (CalGain != 0x00000000 &&
				CalGain != 0xFFFFFFFF &&
				CalR    != 0x00000000 &&
				CalG    != 0x00000000 &&
				CalB    != 0x00000000) {
				pCamCalData->Single2A.S2aAwb.rGainSetNum++;
				pCamCalData->Single2A.S2aAwb.rUnitGainu4R_mid =
					(unsigned int)((tempMax * 512 + (CalR >> 1)) / CalR);
				pCamCalData->Single2A.S2aAwb.rUnitGainu4G_mid =
					(unsigned int)((tempMax * 512 + (CalG >> 1)) / CalG);
				pCamCalData->Single2A.S2aAwb.rUnitGainu4B_mid =
					(unsigned int)((tempMax * 512 + (CalB >> 1)) / CalB);
			} else
				error_log(
			"There are something wrong on EEPROM, plz contact module vendor!!\n");
			/* AWB Golden Gain (4000K) */
			FacR = FacGr = FacGb = FacG = FacB = 0;
			tempMax = 0;
			read_data_size = read_data(pdata,
					pCamCalData->sensorID, pCamCalData->deviceID,
					0x150A, 4, (unsigned char *)&FacGain);
			if (read_data_size > 0)	{
				debug_log("Read FacGain OK\n");
				FacR  = FacGain & 0xFF;
				FacGr = (FacGain >> 8) & 0xFF;
				FacGb = (FacGain >> 16) & 0xFF;
				FacG  = ((FacGr + FacGb) + 1) >> 1;
				FacB  = (FacGain >> 24) & 0xFF;
				if (FacR > FacG)
					if (FacR > FacB)
						tempMax = FacR;
					else
						tempMax = FacB;
				else
					if (FacG > FacB)
						tempMax = FacG;
					else
						tempMax = FacB;
				debug_log(
					"GoldenR:%d, GoldenG:%d, GoldenB:%d, New Golden Max=%d",
					FacR, FacG, FacB, tempMax);
				err = CAM_CAL_ERR_NO_ERR;
			} else {
				pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
				error_log("Read FacGain Failed\n");
				show_cmd_error_log(pCamCalData->Command);
			}
			if (FacGain != 0x00000000 &&
				FacGain != 0xFFFFFFFF &&
				FacR    != 0x00000000 &&
				FacG    != 0x00000000 &&
				FacB    != 0x00000000)	{
				pCamCalData->Single2A.S2aAwb.rGoldGainu4R_mid =
					(unsigned int)((tempMax * 512 + (FacR >> 1)) / FacR);
				pCamCalData->Single2A.S2aAwb.rGoldGainu4G_mid =
					(unsigned int)((tempMax * 512 + (FacG >> 1)) / FacG);
				pCamCalData->Single2A.S2aAwb.rGoldGainu4B_mid =
					(unsigned int)((tempMax * 512 + (FacB >> 1)) / FacB);
			} else
				error_log(
			"There are something wrong on EEPROM, plz contact module vendor!!\n");
		}
	}
	/* AF Calibration Data*/
	if (0x2 & AWBAFConfig) {
		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				start_addr + 10, 2, (unsigned char *)&AFInf);
		if (read_data_size > 0)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}

		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				start_addr + 12, 2, (unsigned char *)&AFMacro);
		if (read_data_size > 0)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}

		pCamCalData->Single2A.S2aAf[0] = AFInf;
		pCamCalData->Single2A.S2aAf[1] = AFMacro;

		////Only AF Gathering <////
		debug_log("======================AF CAM_CAL==================\n");
		debug_log("[AFInf] = %d\n", AFInf);
		debug_log("[AFMacro] = %d\n", AFMacro);
		debug_log("======================AF CAM_CAL==================\n");
	}
	/* AF Closed-Loop Calibration Data*/
	if ((get_mtk_format_version(pdata, pGetSensorCalData) < 0x18)
		? (0x4 & AWBAFConfig) : (0x2 & AWBAFConfig)) {
		//load AF addition info
		unsigned char AF_INFO[64];
		unsigned int af_info_offset = 0xf63;

		memset(AF_INFO, 0, 64);
		debug_log("af_info_offset = %d\n", af_info_offset);

		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				af_info_offset, 64, (unsigned char *) AF_INFO);
		if (read_data_size > 0)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}
		debug_log("AF Test = %x %x %x %x\n",
				AF_INFO[6], AF_INFO[7], AF_INFO[8], AF_INFO[9]);

		pCamCalData->Single2A.S2aAF_t.Close_Loop_AF_Min_Position =
			AF_INFO[0] | (AF_INFO[1] << 8);
		pCamCalData->Single2A.S2aAF_t.Close_Loop_AF_Max_Position =
			AF_INFO[2] | (AF_INFO[3] << 8);
		pCamCalData->Single2A.S2aAF_t.Close_Loop_AF_Hall_AMP_Offset =
			AF_INFO[4];
		pCamCalData->Single2A.S2aAF_t.Close_Loop_AF_Hall_AMP_Gain =
			AF_INFO[5];
		pCamCalData->Single2A.S2aAF_t.AF_infinite_pattern_distance =
			AF_INFO[6] | (AF_INFO[7] << 8);
		pCamCalData->Single2A.S2aAF_t.AF_Macro_pattern_distance =
			AF_INFO[8] | (AF_INFO[9] << 8);
		pCamCalData->Single2A.S2aAF_t.AF_infinite_calibration_temperature =
			AF_INFO[10];

		if (get_mtk_format_version(pdata, pGetSensorCalData) >= 0x22) {
			pCamCalData->Single2A.S2aAF_t.AF_macro_calibration_temperature =
				0;
			pCamCalData->Single2A.S2aAF_t.AF_dac_code_bit_depth =
				AF_INFO[11];
			pCamCalData->Single2A.S2aAF_t.AF_Middle_calibration_temperature =
				0;
		} else {
			pCamCalData->Single2A.S2aAF_t.AF_macro_calibration_temperature =
				AF_INFO[11];
			pCamCalData->Single2A.S2aAF_t.AF_dac_code_bit_depth =
				0;
			pCamCalData->Single2A.S2aAF_t.AF_Middle_calibration_temperature =
				AF_INFO[20];
		}

		if (get_mtk_format_version(pdata, pGetSensorCalData) >= 0x18) {
			pCamCalData->Single2A.S2aAF_t.Posture_AF_infinite_calibration =
				AF_INFO[12] | (AF_INFO[13] << 8);
			pCamCalData->Single2A.S2aAF_t.Posture_AF_macro_calibration =
				AF_INFO[14] | (AF_INFO[15] << 8);
		}

		pCamCalData->Single2A.S2aAF_t.AF_Middle_calibration =
			AF_INFO[18] | (AF_INFO[19] << 8);

		if (get_mtk_format_version(pdata, pGetSensorCalData) >= 0x22) {
			memset(AF_INFO, 0, 64);
			read_data_size = read_data(pdata,
					pCamCalData->sensorID, pCamCalData->deviceID,
					0x154F, 41, (unsigned char *) AF_INFO);
			if (read_data_size >= 0) {
				pCamCalData->Single2A.S2aAF_t.Optical_zoom_cali_num = AF_INFO[0];
				memcpy(pCamCalData->Single2A.S2aAF_t.Optical_zoom_AF_cali,
					&AF_INFO[1], 40);
			}
		}

		debug_log("======================AF addition CAM_CAL==================\n");
		debug_log("[AF_infinite_pattern_distance] = %dmm\n",
				pCamCalData->Single2A.S2aAF_t.AF_infinite_pattern_distance);
		debug_log("[AF_Macro_pattern_distance] = %dmm\n",
				pCamCalData->Single2A.S2aAF_t.AF_Macro_pattern_distance);
		debug_log("[AF_Middle_calibration] = %d\n",
				pCamCalData->Single2A.S2aAF_t.AF_Middle_calibration);
		debug_log("======================AF addition CAM_CAL==================\n");
	}
	return err;
}


/***********************************************************************************
 * Function : To read LSC Table
 ***********************************************************************************/
unsigned int do_single_lsc(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	int read_data_size;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];
	unsigned short table_size = 0;

	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}
	if (block_size != CAM_CAL_SINGLE_LSC_SIZE)
		error_log("block_size(%d) is not match (%d)\n",
				block_size, CAM_CAL_SINGLE_LSC_SIZE);

	pCamCalData->SingleLsc.LscTable.MtkLcsData.MtkLscType = 2;//mtk type
	pCamCalData->SingleLsc.LscTable.MtkLcsData.PixId = 8;

	debug_log("u4Offset=%d u4Length=%lu", start_addr - 2, sizeof(table_size));
	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr - 2, sizeof(table_size), (unsigned char *)&table_size);
	if (read_data_size <= 0)
		err = CAM_CAL_ERR_NO_SHADING;

	debug_log("lsc table_size %d\n", table_size);
	pCamCalData->SingleLsc.LscTable.MtkLcsData.TableSize = table_size;
	if (table_size > 0) {
		pCamCalData->SingleLsc.TableRotation = 0;
		debug_log("u4Offset=%d u4Length=%d", start_addr, table_size);
		read_data_size = read_data(pdata,
			pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, table_size, (unsigned char *)
			&pCamCalData->SingleLsc.LscTable.MtkLcsData.SlimLscType);
		if (table_size == read_data_size)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			error_log("Read Failed\n");
			err = CamCalReturnErr[pCamCalData->Command];
			show_cmd_error_log(pCamCalData->Command);
		}
	}

	debug_log("======================SingleLsc Data==================\n");
	debug_log("[1st] = %x, %x, %x, %x\n",
		pCamCalData->SingleLsc.LscTable.Data[0],
		pCamCalData->SingleLsc.LscTable.Data[1],
		pCamCalData->SingleLsc.LscTable.Data[2],
		pCamCalData->SingleLsc.LscTable.Data[3]);
	debug_log("[1st] = SensorLSC(1)?MTKLSC(2)?  %x\n",
		pCamCalData->SingleLsc.LscTable.MtkLcsData.MtkLscType);
	debug_log("CapIspReg =0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[0],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[1],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[2],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[3],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[4]);
	debug_log("RETURN = 0x%x\n", err);
	debug_log("======================SingleLsc Data==================\n");

	return err;
}

unsigned int do_pdaf(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	int read_data_size;
	int err =  CamCalReturnErr[pCamCalData->Command];

	pCamCalData->PDAF.Size_of_PDAF = block_size;
	debug_log("PDAF start_addr =%x table_size=%d\n", start_addr, block_size);

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size, (unsigned char *)&pCamCalData->PDAF.Data[0]);
	if (read_data_size > 0)
		err = CAM_CAL_ERR_NO_ERR;

	debug_log("======================PDAF Data==================\n");
	debug_log("First five %x, %x, %x, %x, %x\n",
		pCamCalData->PDAF.Data[0],
		pCamCalData->PDAF.Data[1],
		pCamCalData->PDAF.Data[2],
		pCamCalData->PDAF.Data[3],
		pCamCalData->PDAF.Data[4]);
	debug_log("RETURN = 0x%x\n", err);
	debug_log("======================PDAF Data==================\n");

	return err;

}

/******************************************************************************
 * This function will add after sensor support FOV data
 ******************************************************************************/
unsigned int do_stereo_data(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	int read_data_size;
	unsigned int err =  0;
	char *Stereo_Data = NULL;

	debug_log("DoCamCal_Stereo_Data sensorID = %x\n", pCamCalData->sensorID);
	Stereo_Data = kmalloc(1360, GFP_KERNEL);
	if (!Stereo_Data) {
		must_log("kmalloc return NULL failed\n", __func__, __LINE__);
		return CamCalReturnErr[pCamCalData->Command];
	}
	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size, (unsigned char *)Stereo_Data);
	if (read_data_size > 0)
		err = CAM_CAL_ERR_NO_ERR;
	else {
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
	}
	kfree(Stereo_Data);

	debug_log("======================DoCamCal_Stereo_Data==================\n");
	debug_log("======================DoCamCal_Stereo_Data==================\n");

	return err;
}

unsigned int do_dump_all(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	(void) pdata;
	(void) start_addr;
	(void) block_size;
	(void) pGetSensorCalData;

	return CAM_CAL_ERR_NO_ERR;
}

unsigned int do_lens_id_base(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	int read_data_size;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];
	unsigned int size_limit = sizeof(pCamCalData->LensDrvId);

	memset(&pCamCalData->LensDrvId[0], 0, size_limit);

	if (block_size > size_limit) {
		error_log("lens id size can't larger than %u\n", size_limit);
		return err;
	}

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			start_addr, block_size, (unsigned char *)&pCamCalData->LensDrvId[0]);
	if (read_data_size > 0)
		err = CAM_CAL_ERR_NO_ERR;
	else {
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
	}

	debug_log("======================Lens Id==================\n");
	debug_log("[Lens Id] = %x %x %x %x %x\n",
			pCamCalData->LensDrvId[0], pCamCalData->LensDrvId[1],
			pCamCalData->LensDrvId[2], pCamCalData->LensDrvId[3],
			pCamCalData->LensDrvId[4]);
	debug_log("[Lens Id] = %x %x %x %x %x\n",
			pCamCalData->LensDrvId[5], pCamCalData->LensDrvId[6],
			pCamCalData->LensDrvId[7], pCamCalData->LensDrvId[8],
			pCamCalData->LensDrvId[9]);
	debug_log("======================Lens Id==================\n");

	return err;
}

unsigned int do_lens_id(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	unsigned int err = CamCalReturnErr[pCamCalData->Command];

	if (get_mtk_format_version(pdata, pGetSensorCalData) >= 0x18) {
		debug_log("No lens id data\n");
		return err;
	}

	return do_lens_id_base(pdata, start_addr, block_size, pGetSensorCalData);
}

unsigned int get_is_need_power_on(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetNeedPowerOn)
{
	struct STRUCT_CAM_CAL_NEED_POWER_ON *pCamCalNeedPowerOn =
				(struct STRUCT_CAM_CAL_NEED_POWER_ON *)pGetNeedPowerOn;

	enum ENUM_CAMERA_CAM_CAL_TYPE_ENUM lsCommand = pCamCalNeedPowerOn->Command;
	unsigned int uint_lsCommand = (unsigned int)lsCommand;
	unsigned int result = CAM_CAL_ERR_NO_DEVICE;
	int preloadLayoutIndex = IMGSENSOR_SENSOR_DUAL2IDX(pCamCalNeedPowerOn->deviceID);

	if (lsCommand >= CAMERA_CAM_CAL_DATA_LIST) {
		error_log("Invalid Command = 0x%x\n", lsCommand);
		return CAM_CAL_ERR_NO_CMD;
	}

	if (preloadLayoutIndex < 0 || preloadLayoutIndex >= IDX_MAX_CAM_NUMBER) {
		error_log("Invalid DeviceID: 0x%x", pCamCalNeedPowerOn->deviceID);
		return result;
	}

	if (last_sensor_id != pCamCalNeedPowerOn->sensorID) {
		last_sensor_id = pCamCalNeedPowerOn->sensorID;
		if (mp_layout_preload[preloadLayoutIndex] == NULL) {
			debug_log("Preloading layout type");
			debug_log("search %u layouts", cam_cal_number);
			for (cam_cal_index = 0; cam_cal_index < cam_cal_number; cam_cal_index++) {
				cam_cal_config = cam_cal_config_list[cam_cal_index];
				if ((cam_cal_config->check_layout_function != NULL) &&
				(cam_cal_config->check_layout_function(pdata,
				pCamCalNeedPowerOn->sensorID) == CAM_CAL_ERR_NO_ERR))
					break;
			}
			if (cam_cal_index < cam_cal_number) {
				mp_layout_preload[preloadLayoutIndex] = kmalloc(2, GFP_KERNEL);
				memcpy(mp_layout_preload[preloadLayoutIndex], &cam_cal_index, 2);
			}
		} else {
			debug_log("Read layout type from memory[%d]", preloadLayoutIndex);
			memcpy(&cam_cal_index, mp_layout_preload[preloadLayoutIndex], 2);
		}
	}

	if (cam_cal_index < cam_cal_number) {
		cam_cal_config = cam_cal_config_list[cam_cal_index];
		must_log(
		"device_id = %u last_sensor_id = 0x%x current_sensor_id = 0x%x layout type %s found",
		pCamCalNeedPowerOn->deviceID, last_sensor_id, pCamCalNeedPowerOn->sensorID,
		cam_cal_config->name);
		pCamCalNeedPowerOn->needPowerOn = cam_cal_config->has_stored_data &&
			cam_cal_config->layout->cal_layout_tbl[uint_lsCommand].Include;
		result = CAM_CAL_ERR_NO_ERR;
		return result;
	}
	must_log(
		"device_id = %u last_sensor_id = 0x%x current_sensor_id = 0x%x layout type not found",
		pCamCalNeedPowerOn->deviceID, last_sensor_id, pCamCalNeedPowerOn->sensorID);

	result = CamCalReturnErr[uint_lsCommand];
	show_cmd_error_log(lsCommand);
	return result;
}

unsigned int mot_get_cal_factory_data(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData)
{
	struct STRUCT_MOT_EEPROM_DATA *pCamCalData =
				(struct STRUCT_MOT_EEPROM_DATA *)pGetSensorCalData;

	enum ENUM_MOT_CAMERA_CAM_CAL_TYPE_ENUM lsCommand = pCamCalData->Command;
	unsigned int result = CAM_CAL_ERR_NO_DEVICE;

	must_log("device_id = %d, lsCommand = %d\n", pCamCalData->deviceID, lsCommand);

	if (lsCommand != CAMERA_CAM_CAL_DATA_FACTORY_VERIFY) {
		error_log("Invalid Command = 0x%x\n", lsCommand);
		return CAM_CAL_ERR_NO_CMD;
	}

	must_log("last_sensor_id = 0x%x current_sensor_id = 0x%x",
				last_sensor_id, pCamCalData->sensorID);
	if (last_sensor_id != pCamCalData->sensorID) {
		last_sensor_id = pCamCalData->sensorID;
		debug_log("search %u layouts", cam_cal_number);
		for (cam_cal_index = 0; cam_cal_index < cam_cal_number; cam_cal_index++) {
			cam_cal_config = cam_cal_config_list[cam_cal_index];
			if (cam_cal_config->sensor_id == pCamCalData->sensorID) {
				break;
			}
		}
	}

	if ((cam_cal_index < cam_cal_number) && (cam_cal_index >= 0)) {
		strcpy(pCamCalData->SensorName, cam_cal_config->name);
		pCamCalData->sensor_type = (sensor_type_t)cam_cal_config->sensor_type;
		pCamCalData->data_size = (unsigned int)cam_cal_config->preload_size;
		pCamCalData->serial_number_bit= (unsigned int)cam_cal_config->serial_number_bit;
		debug_log("current sensor name : %s, sensor_type = %d, data_size = 0x%x",
			pCamCalData->SensorName, pCamCalData->sensor_type, pCamCalData->data_size);
		if (cam_cal_config->mot_do_factory_verify_function != NULL) {
			result = cam_cal_config->mot_do_factory_verify_function(pdata, pGetSensorCalData);
			return result;
		}
	} else
		must_log("layout type not found");

	return result;
}

unsigned int get_cal_data(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				(struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	enum ENUM_CAMERA_CAM_CAL_TYPE_ENUM lsCommand = pCamCalData->Command;
	unsigned int uint_lsCommand = (unsigned int)lsCommand;
	unsigned int result = CAM_CAL_ERR_NO_DEVICE;
	int preloadLayoutIndex = IMGSENSOR_SENSOR_DUAL2IDX(pCamCalData->deviceID);

	if (lsCommand >= CAMERA_CAM_CAL_DATA_LIST) {
		error_log("Invalid Command = 0x%x\n", lsCommand);
		return CAM_CAL_ERR_NO_CMD;
	}

	if (preloadLayoutIndex < 0 || preloadLayoutIndex >= IDX_MAX_CAM_NUMBER) {
		error_log("Invalid DeviceID: 0x%x", pCamCalData->deviceID);
		return result;
	}

	if (last_sensor_id != pCamCalData->sensorID
			|| pCamCalData->DataVer == CAM_CAL_TYPE_NUM
			|| cam_cal_index == cam_cal_number) {
		last_sensor_id = pCamCalData->sensorID;
		if (mp_layout_preload[preloadLayoutIndex] == NULL) {
			debug_log("Preloading layout type");
			debug_log("search %u layouts", cam_cal_number);
			for (cam_cal_index = 0; cam_cal_index < cam_cal_number; cam_cal_index++) {
				cam_cal_config = cam_cal_config_list[cam_cal_index];
				if ((cam_cal_config->check_layout_function != NULL) &&
				(cam_cal_config->check_layout_function(pdata,
				pCamCalData->sensorID) == CAM_CAL_ERR_NO_ERR))
				{
					pCamCalData->SensorName = (unsigned char *)cam_cal_config->name;
					debug_log("current sensor name : %s", pCamCalData->SensorName);
					break;
				}
			}
			if (cam_cal_index < cam_cal_number) {
				mp_layout_preload[preloadLayoutIndex] = kmalloc(2, GFP_KERNEL);
				memcpy(mp_layout_preload[preloadLayoutIndex], &cam_cal_index, 2);
			}
		} else {
			debug_log("Read layout type from memory[%d]", preloadLayoutIndex);
			memcpy(&cam_cal_index, mp_layout_preload[preloadLayoutIndex], 2);
		}
	}

	if (cam_cal_index < cam_cal_number) {
		cam_cal_config = cam_cal_config_list[cam_cal_index];
		must_log(
		"device_id = %u last_sensor_id = 0x%x current_sensor_id = 0x%x layout type %s found",
		pCamCalData->deviceID, last_sensor_id, pCamCalData->sensorID, cam_cal_config->name);
		pCamCalData->DataVer =
			(enum ENUM_CAM_CAL_DATA_VER_ENUM)cam_cal_config->layout->data_ver;
		if ((cam_cal_config->layout->cal_layout_tbl[uint_lsCommand].Include != 0) &&
			(cam_cal_config->layout->cal_layout_tbl[uint_lsCommand].GetCalDataProcess
			!= NULL)) {
			result =
			cam_cal_config->layout->cal_layout_tbl[uint_lsCommand].GetCalDataProcess(
			pdata,
			cam_cal_config->layout->cal_layout_tbl[uint_lsCommand].start_addr,
			cam_cal_config->layout->cal_layout_tbl[uint_lsCommand].block_size,
			pGetSensorCalData);
			return result;
		}
	} else
		must_log(
		"device_id = %u last_sensor_id = 0x%x current_sensor_id = 0x%x layout type not found",
		pCamCalData->deviceID, last_sensor_id, pCamCalData->sensorID);

	result = CamCalReturnErr[uint_lsCommand];
	show_cmd_error_log(lsCommand);
	return result;
}

int read_data(struct EEPROM_DRV_FD_DATA *pdata, unsigned int sensor_id, unsigned int device_id,
		unsigned int offset, unsigned int length, unsigned char *data)
{
	int preloadIndex = IMGSENSOR_SENSOR_DUAL2IDX(device_id);
	unsigned int bufSize = (cam_cal_config->preload_size > cam_cal_config->max_size)
		? cam_cal_config->max_size : cam_cal_config->preload_size;

	(void) sensor_id;

	if (preloadIndex < 0 || preloadIndex >= IDX_MAX_CAM_NUMBER) {
		error_log("Invalid DeviceID: 0x%x", device_id);
		return -1;
	}
	if (cam_cal_config->enable_preload && bufSize > 0) {
		// Preloading to memory and read from memory
		if (mp_eeprom_preload[preloadIndex] == NULL) {
			mp_eeprom_preload[preloadIndex] = kmalloc(bufSize, GFP_KERNEL);
			must_log("Preloading data %u bytes", bufSize);
			if (read_data_region(pdata, mp_eeprom_preload[preloadIndex], 0, bufSize)
					!= bufSize) {
				error_log("Preload data failed");
				kfree(mp_eeprom_preload[preloadIndex]);
				mp_eeprom_preload[preloadIndex] = NULL;
			}
		}
		if ((mp_eeprom_preload[preloadIndex] != NULL) &&
				(offset + length <= bufSize)) {
			debug_log("Read data from memory[%d]", preloadIndex);
			memcpy(data, mp_eeprom_preload[preloadIndex] + offset, length);
			return length;
		}
	}
	// Read data from EEPROM
	must_log("Read data from EEPROM");
	return read_data_region(pdata, data, offset, length);
}

unsigned int read_data_region(struct EEPROM_DRV_FD_DATA *pdata,
			unsigned char *buf,
			unsigned int offset, unsigned int size)
{
	unsigned int ret;
	unsigned short dts_addr;
	unsigned int size_limit = (cam_cal_config->max_size > 0)
		? cam_cal_config->max_size : DEFAULT_MAX_EEPROM_SIZE_8K;

	if (offset + size > size_limit) {
		error_log("Error! Not support address >= 0x%x!!\n", size_limit);
		return 0;
	}
	if (cam_cal_config->read_function) {
		debug_log("i2c read 0x%02x %d %d\n",
					cam_cal_config->i2c_write_id, offset, size);
		mutex_lock(&pdata->pdrv->eeprom_mutex);
		dts_addr = pdata->pdrv->pi2c_client->addr;
		pdata->pdrv->pi2c_client->addr = (cam_cal_config->i2c_write_id >> 1);
		ret = cam_cal_config->read_function(pdata->pdrv->pi2c_client,
					offset, buf, size);
		pdata->pdrv->pi2c_client->addr = dts_addr;
		mutex_unlock(&pdata->pdrv->eeprom_mutex);
	} else {
		debug_log("no customized\n");
		debug_log("i2c read 0x%02x %d %d\n",
				(pdata->pdrv->pi2c_client->addr << 1),
				offset, size);
		mutex_lock(&pdata->pdrv->eeprom_mutex);
		ret = Common_read_region(pdata->pdrv->pi2c_client,
					offset, buf, size);
		mutex_unlock(&pdata->pdrv->eeprom_mutex);
	}
	return ret;
}


/*****************************  custom add  ********************************/
unsigned short CRC16_IBMMSB(unsigned char *data, unsigned int length)
{
	unsigned int i = 0;
	int x = 0;
	unsigned short crc = 0;
	unsigned short ucData = { 0 };

	if (data == NULL)
	{
		must_log("data null");
		return 1;
	}

	for (x = 0; x < length; x++)
	{
		ucData = ((unsigned short)data[x] << 8);
		//printf("ucData = %x\n", ucData);
		crc ^= ucData;

		for (i = 0; i < 8; ++i)
		{
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x8005;
			else
				crc = (crc << 1);
		}
	}
	return crc;
}

unsigned int custom_do_module_info(struct EEPROM_DRV_FD_DATA *pdata,
		 unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
   struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				 (struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	//unsigned int err = CamCalReturnErr[pCamCalData->Command];
	struct STRUCT_CAM_CAL_MODULE_INFO  Module_info;
	int read_data_size;
	int i;

#ifdef ENABLE_CHECK_SUM
	unsigned int checkSum;
#endif
	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
					start_addr,
					sizeof(struct STRUCT_CAM_CAL_MODULE_INFO),
					(unsigned char *)&Module_info);
	if(read_data_size <= 0) {
		error_log("%s: %d, Read Failed\n",__func__,__LINE__);
		show_cmd_error_log(pCamCalData->Command);
		return CamCalReturnErr[pCamCalData->Command];
	}

	if(Module_info.flag != 0x40) {
		must_log("[MODULE_INFO] flag is Invalid\n",Module_info.flag);
		return CamCalReturnErr[pCamCalData->Command];
	}

#ifdef ENABLE_CHECK_SUM
	checkSum = Module_info.check_sum_h << 8 | Module_info.check_sum_l;
	if(checkSum != CRC16_IBMMSB((unsigned char *)&Module_info,sizeof(struct STRUCT_CAM_CAL_MODULE_INFO)-2)) {
		must_log("[MODULE_INFO]checkSum failed, checksum: %d, %d\n",
				  checkSum,
				  CRC16_IBMMSB((unsigned char *)&Module_info,sizeof(struct STRUCT_CAM_CAL_MODULE_INFO)-2));
		return CamCalReturnErr[pCamCalData->Command];
	}

	must_log("[MODULE_INFO]checkSum succeed\n");
#endif
	debug_log("======================MODULE_INFO==================\n");
	debug_log("[MODULE_INFO] flag: 0x%x", Module_info.flag);
	debug_log("[MODULE_INFO]PartNum = 0x%x 0x%x 0x%x 0x%x 0x%x\n 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			   Module_info.part_number[0],
			   Module_info.part_number[1],
			   Module_info.part_number[2],
			   Module_info.part_number[3],
			   Module_info.part_number[4],
			   Module_info.part_number[5],
			   Module_info.part_number[6],
			   Module_info.part_number[7],
			   Module_info.part_number[8],
			   Module_info.part_number[9]);
	debug_log("[MODULE_INFO]supplier_id:0x%x,sensor_id:0x%x,lens_id:0x%x,vcm_id:0x%x,"
			  "driverIC_id:0x%x,phase:0x%x,mirror_flip_status:%x, date:%d-%d-%d",
			   Module_info.supplier_id,
			   Module_info.sensor_id,
			   Module_info.lens_id,
			   Module_info.vcm_id,
			   Module_info.driverIC_id,
			   Module_info.phase,
			   Module_info.mirror_flip_status,
			   Module_info.year,
			   Module_info.month,
			   Module_info.day);

	for(i = 0;i < CUSTOM_SN_SIZE; i++) {
			debug_log("[MODULE INFO]sn: 0x%x", Module_info.SN[i]);
	}
	debug_log("======================MODULE_INFO==================\n");
	return CAM_CAL_ERR_NO_DEVICE;
}

unsigned int custom_do_2a_gain(struct EEPROM_DRV_FD_DATA *pdata,
		 unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
						  (struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];

	unsigned short tempMax = 0;
	int read_data_size;

	struct STRUCT_CAM_CAL_AWB_INFO CalAwbGain;
	unsigned char AWBAFConfig = 0x3;

	unsigned short CalR = 1, CalGr = 1, CalGb = 1, CalG = 1, CalB = 1;
	unsigned short FacR = 1, FacGr = 1, FacGb = 1, FacG = 1, FacB = 1;

#ifdef ENABLE_CHECK_SUM
	unsigned int checkSum;
#endif

	debug_log("block_size=%d sensor_id=%x\n", block_size, pCamCalData->sensorID);
	memset((void *)&pCamCalData->Single2A, 0, sizeof(struct STRUCT_CAM_CAL_SINGLE_2A_STRUCT));

	/* Check rule */
	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}

	pCamCalData->Single2A.S2aVer = 0x01;
	pCamCalData->Single2A.S2aBitEn = (0x03 & AWBAFConfig);
	pCamCalData->Single2A.S2aAfBitflagEn = (0x0C & AWBAFConfig);

	if (0x1 & AWBAFConfig) {
		 /* AWB Unit Gain (5100K) */
		pCamCalData->Single2A.S2aAwb.rGainSetNum = 0;
		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
									start_addr + CUSTOM_AF_DATA_SIZE,
									sizeof(struct STRUCT_CAM_CAL_AWB_INFO),
									(unsigned char *)&CalAwbGain);
		if (read_data_size <= 0) {
				pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
				error_log("Read CalGain Failed\n");
				show_cmd_error_log(pCamCalData->Command);
				return CamCalReturnErr[pCamCalData->Command];
		}

		if (CalAwbGain.flag != 0x40){
			must_log("[AWB_INFO]flag is invalid %x\n", CalAwbGain.flag);
			return CamCalReturnErr[pCamCalData->Command];
		}

#ifdef ENABLE_CHECK_SUM
		checkSum = CalAwbGain.check_sum_h << 8 | CalAwbGain.check_sum_l;
		if(checkSum != CRC16_IBMMSB((unsigned char *)&CalAwbGain,(sizeof(struct STRUCT_CAM_CAL_AWB_INFO)-2))) {
			must_log("[AWB_INFO]checkSum failed, checksum: %d, %d\n",
					  checkSum,
					  CRC16_IBMMSB((unsigned char *)&CalAwbGain,(sizeof(struct STRUCT_CAM_CAL_AWB_INFO)-2)));
			return CamCalReturnErr[pCamCalData->Command];
		}
		must_log("[AWB_INFO]checkSum succeed\n");
#endif
		debug_log("Read CalGain OK %x\n", read_data_size);
		CalR  = ( CalAwbGain.awb_r_h << 8)	| CalAwbGain.awb_r_l;
		CalGr = ( CalAwbGain.awb_gr_h << 8) | CalAwbGain.awb_gr_l;
		CalGb = ( CalAwbGain.awb_gb_h << 8) |  CalAwbGain.awb_gb_l;
		CalG  = (CalGr + CalGb + 1) >> 1;
		CalB  =( CalAwbGain.awb_b_h << 8) |	 CalAwbGain.awb_b_l;
		tempMax = MAX_TEMP(CalR,CalG,CalB);
		debug_log("[AWB_INFO]UnitR:%d, UnitG:%d, UnitB:%d, New Unit Max=%d",
						CalR, CalG, CalB, tempMax);
		err = CAM_CAL_ERR_NO_ERR;

		if (CalR == 0 || CalG == 0 || CalB == 0) {
			error_log("There are something wrong on EEPROM, plz contact module vendor!!\n");
			return CamCalReturnErr[pCamCalData->Command];
		}

		pCamCalData->Single2A.S2aAwb.rGainSetNum++;
		pCamCalData->Single2A.S2aAwb.rUnitGainu4R =
					(unsigned int)((tempMax * 512 + (CalR >> 1)) / CalR);
		pCamCalData->Single2A.S2aAwb.rUnitGainu4G =
					(unsigned int)((tempMax * 512 + (CalG >> 1)) / CalG);
		pCamCalData->Single2A.S2aAwb.rUnitGainu4B =
					(unsigned int)((tempMax * 512 + (CalB >> 1)) / CalB);

		/* AWB Golden Gain (5100K) */
		FacR  = (CalAwbGain.golden_awb_r_h << 8) | CalAwbGain.golden_awb_r_l;
		FacGr = (CalAwbGain.golden_awb_gr_h << 8) | CalAwbGain.golden_awb_gr_l;
		FacGb = (CalAwbGain.golden_awb_gb_h << 8) | CalAwbGain.golden_awb_gb_l;
		FacG  = ((FacGr + FacGb) + 1) >> 1;
		FacB  = (CalAwbGain.golden_awb_b_h << 8) | CalAwbGain.golden_awb_b_l;

		tempMax = MAX_TEMP(FacR,FacB,FacG);
		debug_log("[AWB_INFO]GoldenR:%d, GoldenG:%d, GoldenB:%d, New Golden Max=%d",FacR, FacG, FacB, tempMax);

		if (FacR == 0 || FacG == 0 || FacB == 0) {
			 error_log(
			 "There are something wrong on EEPROM, plz contact module vendor!!\n");
			 return CamCalReturnErr[pCamCalData->Command];
		}

		pCamCalData->Single2A.S2aAwb.rGoldGainu4R =
				(unsigned int)((tempMax * 512 + (FacR >> 1)) / FacR);
		pCamCalData->Single2A.S2aAwb.rGoldGainu4G =
				(unsigned int)((tempMax * 512 + (FacG >> 1)) / FacG);
		pCamCalData->Single2A.S2aAwb.rGoldGainu4B =
				(unsigned int)((tempMax * 512 + (FacB >> 1)) / FacB);

		 /* Set AWB to 3A Layer */
		pCamCalData->Single2A.S2aAwb.rValueR	= CalR;
		pCamCalData->Single2A.S2aAwb.rValueGr	= CalGr;
		pCamCalData->Single2A.S2aAwb.rValueGb	= CalGb;
		pCamCalData->Single2A.S2aAwb.rValueB	= CalB;
		pCamCalData->Single2A.S2aAwb.rGoldenR	= FacR;
		pCamCalData->Single2A.S2aAwb.rGoldenGr	= FacGr;
		pCamCalData->Single2A.S2aAwb.rGoldenGb	= FacGb;
		pCamCalData->Single2A.S2aAwb.rGoldenB	= FacB;

		debug_log("======================AWB CAM_CAL==================\n");
		must_log("[rCalGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4R);
		must_log("[rCalGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4G);
		must_log("[rCalGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4B);

		must_log("[rFacGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4R);
		must_log("[rFacGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4G);
		must_log("[rFacGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4B);
		debug_log("======================AWB CAM_CAL==================\n");
	 }

	/* AF calibration*/
	if (0x2 & AWBAFConfig) {
		struct STRUCT_CAM_CAL_AF_INFO CalAfData;
		unsigned short AFInf, AFMacro;

		memset(&CalAfData, 0, sizeof(struct STRUCT_CAM_CAL_AF_INFO));

		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
								start_addr,
								sizeof(struct STRUCT_CAM_CAL_AF_INFO),
								(unsigned char *)&CalAfData);
		if (read_data_size <= 0) {
				pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
				error_log("[AF_INFO]Read Failed\n");
				show_cmd_error_log(pCamCalData->Command);
		}

		err = CAM_CAL_ERR_NO_ERR;

		if (CalAfData.flag != 0x40){
			must_log("[AF_INFO]flag is invalid %x\n", CalAfData.flag);
			//return CamCalReturnErr[pCamCalData->Command]; //front camera have no af
		}

#ifdef ENABLE_CHECK_SUM
		checkSum = CalAfData.check_sum_h << 8 | CalAfData.check_sum_l;
		if(checkSum != CRC16_IBMMSB((unsigned char *)&CalAfData, sizeof(struct STRUCT_CAM_CAL_AF_INFO)-2)) {
			must_log("[AF_INFO]checkSum failed, checksum: %d, %d\n",
					  checkSum,
					  CRC16_IBMMSB((unsigned char *)&CalAfData, sizeof(struct STRUCT_CAM_CAL_AF_INFO)-2));
			//return CamCalReturnErr[pCamCalData->Command]; //front camera have no af
		}
		debug_log("[AF_INFO]checkSum succeed\n");
#endif

		AFInf = (CalAfData.af_hor_inf_h << 8 | CalAfData.af_hor_inf_l)/64;
		AFMacro = ((CalAfData.af_hor_macro_h << 8 ) | CalAfData.af_hor_macro_l)/64;

		debug_log("======================AF CAM_CAL==================\n");
		must_log("[AF_INFO]AFInf: %d, AFMacro: %d\n", AFInf,AFMacro);
		pCamCalData->Single2A.S2aAf[0] = AFInf;
		pCamCalData->Single2A.S2aAf[1] = AFMacro;
	}

	return err;
}

unsigned int custom_do_single_lsc(struct EEPROM_DRV_FD_DATA *pdata,
		 unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				 (struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	int read_data_size;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];

#ifdef ENABLE_CHECK_SUM
	struct STRUCT_CAM_CAL_LSC_INFO *CalLscData = NULL;
	unsigned int checkSum;
#endif

	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
#ifdef ENABLE_CHECK_SUM
		kfree(CalLscData);
#endif
		return err;
	}

	pCamCalData->SingleLsc.LscTable.MtkLcsData.MtkLscType = 2;//mtk type
	pCamCalData->SingleLsc.LscTable.MtkLcsData.PixId = 8;

#ifdef ENABLE_CHECK_SUM
	CalLscData = kmalloc(sizeof(struct STRUCT_CAM_CAL_LSC_INFO), GFP_KERNEL);
	if (!CalLscData) {
		must_log("kmalloc return NULL failed\n", __func__, __LINE__);
		return CamCalReturnErr[pCamCalData->Command];
	}

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
							   start_addr,
							   sizeof(struct STRUCT_CAM_CAL_LSC_INFO),
							   (unsigned char *)CalLscData);

	if (read_data_size <= 0) {
		must_log("[LSC_INFO] %s,%d: read data failed\n",__func__,__LINE__);
		kfree(CalLscData);
		return CamCalReturnErr[pCamCalData->Command];
	}

	if (CalLscData->flag != 0x40) {
		must_log("flag of lsc is invalid %x\n", CalLscData->flag);
		kfree(CalLscData);
		return CamCalReturnErr[pCamCalData->Command];
	}

	err = CAM_CAL_ERR_NO_ERR;

	checkSum = CalLscData->check_sum_h << 8 | CalLscData->check_sum_l;
	if (checkSum != CRC16_IBMMSB((unsigned char *)CalLscData,
		sizeof(struct STRUCT_CAM_CAL_LSC_INFO)-2)) {
		must_log("[LSC_INFO] checkSum failed, checksum: %d, %d\n",
				  checkSum,
				  CRC16_IBMMSB((unsigned char *)CalLscData,
								sizeof(struct STRUCT_CAM_CAL_LSC_INFO)-2));
		kfree(CalLscData);
		return CamCalReturnErr[pCamCalData->Command];
	}

	must_log("[LSC_INFO] checkSum succeed\n");
#endif

	debug_log("[LSC_INFO]u4Offset=0x%x lsc table_size=%lu", start_addr + 1, CUSTOM_LSC_DATA_SIZE);

	pCamCalData->SingleLsc.LscTable.MtkLcsData.TableSize = CUSTOM_LSC_DATA_SIZE;

	pCamCalData->SingleLsc.TableRotation = 0;
	read_data_size = read_data(pdata,pCamCalData->sensorID, pCamCalData->deviceID,
							   start_addr + 1,
							   CUSTOM_LSC_DATA_SIZE,
							   (unsigned char *)&pCamCalData->SingleLsc.LscTable.MtkLcsData.SlimLscType);
	if (CUSTOM_LSC_DATA_SIZE == read_data_size)
		err = CAM_CAL_ERR_NO_ERR;
	else {
		error_log("Read Failed\n");
		err = CamCalReturnErr[pCamCalData->Command];
		show_cmd_error_log(pCamCalData->Command);
	}

	debug_log("======================SingleLsc Data==================\n");
	debug_log("[1st] = %x, %x, %x, %x\n",
		pCamCalData->SingleLsc.LscTable.Data[0],
		pCamCalData->SingleLsc.LscTable.Data[1],
		pCamCalData->SingleLsc.LscTable.Data[2],
		pCamCalData->SingleLsc.LscTable.Data[3]);
	debug_log("[1st] = SensorLSC(1)?MTKLSC(2)?	%x\n",
		pCamCalData->SingleLsc.LscTable.MtkLcsData.MtkLscType);
	debug_log("CapIspReg =0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[0],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[1],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[2],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[3],
		pCamCalData->SingleLsc.LscTable.MtkLcsData.CapIspReg[4]);
	debug_log("RETURN = 0x%x\n", err);
	debug_log("======================SingleLsc Data==================\n");
#ifdef ENABLE_CHECK_SUM
	kfree(CalLscData);
#endif
	return err;
}

unsigned int custom_do_pdaf(struct EEPROM_DRV_FD_DATA *pdata,
		 unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
				 (struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;

	int read_data_size;
	int err =  CamCalReturnErr[pCamCalData->Command];

#ifdef ENABLE_CHECK_SUM
	struct STRUCT_CAM_CAL_PDAF_INFO *CalPdafData = NULL;
	unsigned int checkSum;

	CalPdafData = kmalloc(sizeof(struct STRUCT_CAM_CAL_PDAF_INFO), GFP_KERNEL);
	if (!CalPdafData) {
		must_log("kmalloc return NULL failed\n", __func__, __LINE__);
		return CamCalReturnErr[pCamCalData->Command];
	}

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
							   start_addr,
							   sizeof(struct STRUCT_CAM_CAL_PDAF_INFO),
							   (unsigned char *)CalPdafData);

	if (read_data_size <= 0) {
		must_log("[PDAF_INFO] %s,%d: read data failed\n",__func__,__LINE__);
		kfree(CalPdafData);
		return CamCalReturnErr[pCamCalData->Command];
	}

	if (CalPdafData->flag != 0x40) {
		must_log("flag of pdaf is invalid %x\n", CalPdafData->flag);
		kfree(CalPdafData);
		return CamCalReturnErr[pCamCalData->Command];
	}

	checkSum = CalPdafData->check_proc1_sum_h << 8 | CalPdafData->check_proc1_sum_l;
	if (checkSum != CRC16_IBMMSB((unsigned char *)CalPdafData,
		CUSTOM_PDAF_PROC1_SIZE + 1)){
		must_log("[PDAF_INFO] PROC 1 checkSum failed, checksum: %d, %d\n",
						checkSum,
						CRC16_IBMMSB((unsigned char *)CalPdafData,
						CUSTOM_PDAF_PROC1_SIZE + 1));
		kfree(CalPdafData);
		return CamCalReturnErr[pCamCalData->Command];
	}

	must_log("[PDAF_INFO] PROC 1 checkSum succeed\n");

	checkSum = CalPdafData->check_proc2_sum_h << 8 | CalPdafData->check_proc2_sum_l;
	if (checkSum != CRC16_IBMMSB((unsigned char *)&(CalPdafData->pdaf_proc2[0]),
		CUSTOM_PDAF_PROC2_SIZE)) {
		must_log("[PDAF_INFO] PROC 2 checkSum failed, checksum: %d, %d\n",
			checkSum,
			CRC16_IBMMSB((unsigned char *)&(CalPdafData->pdaf_proc2[0]),
			CUSTOM_PDAF_PROC2_SIZE));
		kfree(CalPdafData);
		return CamCalReturnErr[pCamCalData->Command];
	}

	must_log("[PDAF_INFO] PROC 2 checkSum succeed\n");
#endif

	block_size = CUSTOM_PDAF_DATA_SIZE;
	pCamCalData->PDAF.Size_of_PDAF = block_size;
	debug_log("PDAF start_addr =%x table_size=%d\n", (start_addr + 1), block_size);

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
							(start_addr + 1),
							block_size,
							(unsigned char *)&pCamCalData->PDAF.Data[0]);
	if (read_data_size <= 0) {
		must_log("[PDAF_INFO] %s,%d: read data failed\n",__func__,__LINE__);
#ifdef ENABLE_CHECK_SUM
		kfree(CalPdafData);
#endif
		return CamCalReturnErr[pCamCalData->Command];
	}

	err = CAM_CAL_ERR_NO_ERR;
	debug_log("======================PDAF Data==================\n");
	debug_log("First five %x, %x, %x, %x, %x\n",
					pCamCalData->PDAF.Data[0],
					pCamCalData->PDAF.Data[1],
					pCamCalData->PDAF.Data[2],
					pCamCalData->PDAF.Data[3],
					pCamCalData->PDAF.Data[4]);
	debug_log("RETURN = 0x%x\n", err);
	debug_log("======================PDAF Data==================\n");
#ifdef ENABLE_CHECK_SUM
	kfree(CalPdafData);
#endif
	return err;
}

unsigned int custom_layout_check(struct EEPROM_DRV_FD_DATA *pdata,
					  unsigned int sensorID)
{
	unsigned int result = CAM_CAL_ERR_NO_DEVICE;

	must_log("%s ,%d Enter\n", __func__,__LINE__);

	if (cam_cal_config->sensor_id == sensorID) {
		must_log("%s,%d: sensor_id matched\n",cam_cal_config->name);
		result = CAM_CAL_ERR_NO_ERR;
	} else {
		must_log("%s,%d: sensor_id not matched\n",cam_cal_config->name);
		return result;
	}

	return result;
}


unsigned int custom_do_awb_gain(struct EEPROM_DRV_FD_DATA *pdata,
		 unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData)
{
	struct STRUCT_CAM_CAL_DATA_STRUCT *pCamCalData =
	                  (struct STRUCT_CAM_CAL_DATA_STRUCT *)pGetSensorCalData;
	unsigned int err = CamCalReturnErr[pCamCalData->Command];

	unsigned short tempMax = 0;
	int read_data_size;

	struct STRUCT_CAM_CAL_AWB_INFO CalAwbGain;
	unsigned char AWBAFConfig = 0x1;

	unsigned short CalR = 1, CalGr = 1, CalGb = 1, CalG = 1, CalB = 1;
	unsigned short FacR = 1, FacGr = 1, FacGb = 1, FacG = 1, FacB = 1;

#ifdef ENABLE_CHECK_SUM
	unsigned int checkSum;
#endif

	debug_log("block_size=%d sensor_id=%x\n", block_size, pCamCalData->sensorID);
	memset((void *)&pCamCalData->Single2A, 0, sizeof(struct STRUCT_CAM_CAL_SINGLE_2A_STRUCT));

	/* Check rule */
	if (pCamCalData->DataVer >= CAM_CAL_TYPE_NUM) {
		err = CAM_CAL_ERR_NO_DEVICE;
		error_log("Read Failed\n");
		show_cmd_error_log(pCamCalData->Command);
		return err;
	}

	pCamCalData->Single2A.S2aVer = 0x01;
	pCamCalData->Single2A.S2aBitEn = (0x03 & AWBAFConfig);
	pCamCalData->Single2A.S2aAfBitflagEn = (0x0C & AWBAFConfig);

	if (0x1 & AWBAFConfig) {
		 /* AWB Unit Gain (5100K) */
		pCamCalData->Single2A.S2aAwb.rGainSetNum = 0;
		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
		                                                   start_addr,
		                                                   sizeof(struct STRUCT_CAM_CAL_AWB_INFO),
		                                                   (unsigned char *)&CalAwbGain);
		if (read_data_size <= 0) {
				pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
				error_log("Read CalGain Failed\n");
				show_cmd_error_log(pCamCalData->Command);
				return CamCalReturnErr[pCamCalData->Command];
		}

		if (CalAwbGain.flag != 0x40){
			must_log("[AWB_INFO]flag is invalid: 0x%x,start_addr:0x%x\n", CalAwbGain.flag,start_addr);
 			must_log("[AWB_INFO] golden_awb_r_h: 0x%x,golden_awb_r_l:0x%x,golden_awb_gr_h:0x%x, golden_awb_gr_l:0x%x\n",
			                CalAwbGain.golden_awb_r_h,
			                CalAwbGain.golden_awb_r_l,
			                CalAwbGain.golden_awb_gr_h,
			                CalAwbGain.golden_awb_gr_l);

			return CamCalReturnErr[pCamCalData->Command];
		}

#ifdef ENABLE_CHECK_SUM
		checkSum = CalAwbGain.check_sum_h << 8 | CalAwbGain.check_sum_l;
		if (checkSum != CRC16_IBMMSB((unsigned char *)&CalAwbGain,
			(sizeof(struct STRUCT_CAM_CAL_AWB_INFO)-2))) {
			must_log("[AWB_INFO]checkSum failed, checksum: %d, %d\n",
			                   checkSum,
			                   CRC16_IBMMSB((unsigned char *)&CalAwbGain,(sizeof(struct STRUCT_CAM_CAL_AWB_INFO)-2)));
			return CamCalReturnErr[pCamCalData->Command];
		}
		must_log("[AWB_INFO]checkSum succeed\n");
#endif
		debug_log("Read CalGain OK %x\n", read_data_size);
		CalR  = ( CalAwbGain.awb_r_h << 8)	| CalAwbGain.awb_r_l;
		CalGr = ( CalAwbGain.awb_gr_h << 8) | CalAwbGain.awb_gr_l;
		CalGb = ( CalAwbGain.awb_gb_h << 8) |  CalAwbGain.awb_gb_l;
		CalG  = (CalGr + CalGb + 1) >> 1;
		CalB  =( CalAwbGain.awb_b_h << 8) |	 CalAwbGain.awb_b_l;
		tempMax = MAX_TEMP(CalR,CalG,CalB);
		debug_log("[AWB_INFO]UnitR:%d, UnitG:%d, UnitB:%d, New Unit Max=%d",
		                     CalR, CalG, CalB, tempMax);
		err = CAM_CAL_ERR_NO_ERR;

		if (CalR == 0 || CalG == 0 || CalB == 0) {
			error_log("There are something wrong on EEPROM, plz contact module vendor!!\n");
			return CamCalReturnErr[pCamCalData->Command];
		}

		pCamCalData->Single2A.S2aAwb.rGainSetNum++;
		pCamCalData->Single2A.S2aAwb.rUnitGainu4R =
		              (unsigned int)((tempMax * 512 + (CalR >> 1)) / CalR);
		pCamCalData->Single2A.S2aAwb.rUnitGainu4G =
		             (unsigned int)((tempMax * 512 + (CalG >> 1)) / CalG);
		pCamCalData->Single2A.S2aAwb.rUnitGainu4B =
		             (unsigned int)((tempMax * 512 + (CalB >> 1)) / CalB);

		/* AWB Golden Gain (5100K) */
		FacR  = (CalAwbGain.golden_awb_r_h << 8) | CalAwbGain.golden_awb_r_l;
		FacGr = (CalAwbGain.golden_awb_gr_h << 8) | CalAwbGain.golden_awb_gr_l;
		FacGb = (CalAwbGain.golden_awb_gb_h << 8) | CalAwbGain.golden_awb_gb_l;
		FacG  = ((FacGr + FacGb) + 1) >> 1;
		FacB  = (CalAwbGain.golden_awb_b_h << 8) | CalAwbGain.golden_awb_b_l;

		tempMax = MAX_TEMP(FacR,FacB,FacG);
		debug_log("[AWB_INFO]GoldenR:%d, GoldenG:%d, GoldenB:%d, New Golden Max=%d",FacR, FacG, FacB, tempMax);

		if (FacR == 0 || FacG == 0 || FacB == 0) {
			 error_log(
			 "There are something wrong on EEPROM, plz contact module vendor!!\n");
			 return CamCalReturnErr[pCamCalData->Command];
		}

		pCamCalData->Single2A.S2aAwb.rGoldGainu4R =
				(unsigned int)((tempMax * 512 + (FacR >> 1)) / FacR);
		pCamCalData->Single2A.S2aAwb.rGoldGainu4G =
				(unsigned int)((tempMax * 512 + (FacG >> 1)) / FacG);
		pCamCalData->Single2A.S2aAwb.rGoldGainu4B =
				(unsigned int)((tempMax * 512 + (FacB >> 1)) / FacB);

		 /* Set AWB to 3A Layer */
		pCamCalData->Single2A.S2aAwb.rValueR	= CalR;
		pCamCalData->Single2A.S2aAwb.rValueGr	= CalGr;
		pCamCalData->Single2A.S2aAwb.rValueGb	= CalGb;
		pCamCalData->Single2A.S2aAwb.rValueB	= CalB;
		pCamCalData->Single2A.S2aAwb.rGoldenR	= FacR;
		pCamCalData->Single2A.S2aAwb.rGoldenGr	= FacGr;
		pCamCalData->Single2A.S2aAwb.rGoldenGb	= FacGb;
		pCamCalData->Single2A.S2aAwb.rGoldenB	= FacB;

		debug_log("======================AWB CAM_CAL==================\n");
		must_log("[rCalGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4R);
		must_log("[rCalGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4G);
		must_log("[rCalGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rUnitGainu4B);

		must_log("[rFacGain.u4R] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4R);
		must_log("[rFacGain.u4G] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4G);
		must_log("[rFacGain.u4B] = %d\n", pCamCalData->Single2A.S2aAwb.rGoldGainu4B);
		debug_log("======================AWB CAM_CAL==================\n");
	 }

	return err;
}
