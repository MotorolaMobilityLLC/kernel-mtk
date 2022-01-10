// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__

#include <linux/kernel.h>
#include <linux/string.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "cam_cal_config.h"

#define GC02M1B_MOT_EEPROM_DATA_SIZE 0x11
#define OTP_VAILD_DATA_SIZE 0x08
#define GC02M1B_MOT_EEPROM_DATA_SIZE_16 0x23
#define OTP_VAILD_DATA_SIZE_16 0x11

extern unsigned char gc02m1b_valid_data[OTP_VAILD_DATA_SIZE];
extern unsigned char gc02m1b_data_eeprom[GC02M1B_MOT_EEPROM_DATA_SIZE]; //add flag and data

unsigned int gc02m1b_mot_do_data_copy(struct EEPROM_DRV_FD_DATA *pdata,
				unsigned int *pGetSensorCalData);

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000010, 0x19050000, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000000, 0x00000000, 0x00000000, do_module_version},
		{0x00000000, 0x00000000, 0x00000E25, do_part_number},
		{0x00000000, 0x000000F7, 0x0000074C, mot_do_single_lsc},
		{0x00000000, 0x00000027, 0x00000045, mot_do_2a_gain},
		{0x00000000, 0x00000845, 0x000005DC, mot_do_pdaf},
		{0x00000000, 0x00000FAE, 0x00000550, do_stereo_data},
		{0x00000000, 0x00000000, 0x00000E25, do_dump_all},
		{0x00000000, 0x00000F80, 0x0000000A, do_lens_id},
		{0x00000000, 0x00000000, 0x00000025, mot_do_manufacture_info},
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT mot_gc02m1b_eeprom = {
	.name = "mot_gc02m1b_eeprom",
	.sensor_type = DEPTH_CAMERA,
	.check_layout_function = mot_layout_no_ck,
	.read_function = Common_read_region,
	.mot_do_factory_verify_function = gc02m1b_mot_do_data_copy,
	.layout = &cal_layout_table,
	.sensor_id = MOT_DUBAI_GC02M1B_SENSOR_ID,
	.i2c_write_id = 0x6E,
	.max_size = 0x8000,
	.serial_number_bit = 8,
	.enable_preload = 1,
	.preload_size = GC02M1B_MOT_EEPROM_DATA_SIZE,
};

unsigned int gc02m1b_mot_do_data_copy(struct EEPROM_DRV_FD_DATA *pdata,
				unsigned int *pGetSensorCalData){
	struct STRUCT_MOT_EEPROM_DATA *pCamCalData =
				(struct STRUCT_MOT_EEPROM_DATA *)pGetSensorCalData;

	int ret, err;
	unsigned char TempBuf[GC02M1B_MOT_EEPROM_DATA_SIZE_16] = {0};

	err = CAM_CAL_ERR_NO_ERR;

	debug_log("Otp Data dump:");

	//dump all
	ret = snprintf(TempBuf, GC02M1B_MOT_EEPROM_DATA_SIZE_16,
		"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		gc02m1b_data_eeprom[0], gc02m1b_data_eeprom[1],
		gc02m1b_data_eeprom[2], gc02m1b_data_eeprom[3],
		gc02m1b_data_eeprom[4], gc02m1b_data_eeprom[5],
		gc02m1b_data_eeprom[6], gc02m1b_data_eeprom[7],
		gc02m1b_data_eeprom[8], gc02m1b_data_eeprom[9],
		gc02m1b_data_eeprom[10], gc02m1b_data_eeprom[11],
		gc02m1b_data_eeprom[12], gc02m1b_data_eeprom[13],
		gc02m1b_data_eeprom[14], gc02m1b_data_eeprom[15],
		gc02m1b_data_eeprom[16]);

	if (ret < 0) {
		debug_log("snprintf of DumpAllEepromData failed");
		memset(pCamCalData->DumpAllEepromData, 0,
			sizeof(pCamCalData->DumpAllEepromData));
		err = CAM_CAL_ERR_DUMP_FAILED;
	} else {
		memcpy(pCamCalData->DumpAllEepromData, gc02m1b_data_eeprom, sizeof(gc02m1b_data_eeprom));
		debug_log("DumpAllEepromData: %s\n", TempBuf);
	}

	memset(TempBuf, 0, sizeof(TempBuf));
	//dump vaild
	ret = snprintf(TempBuf, OTP_VAILD_DATA_SIZE_16,
		"%02x%02x%02x%02x%02x%02x%02x%02x",
		gc02m1b_valid_data[0], gc02m1b_valid_data[1],
		gc02m1b_valid_data[2], gc02m1b_valid_data[3],
		gc02m1b_valid_data[4], gc02m1b_valid_data[5],
		gc02m1b_valid_data[6], gc02m1b_valid_data[7]);

	if (ret < 0) {
		debug_log("snprintf of serial_number failed");
		memset(pCamCalData->serial_number, 0,
			sizeof(pCamCalData->serial_number));
		err = CAM_CAL_ERR_DUMP_FAILED;
	} else {
		memcpy(pCamCalData->serial_number, gc02m1b_valid_data, sizeof(gc02m1b_valid_data));
		debug_log("serial_number: %s\n", TempBuf);
	}

	return err;
}