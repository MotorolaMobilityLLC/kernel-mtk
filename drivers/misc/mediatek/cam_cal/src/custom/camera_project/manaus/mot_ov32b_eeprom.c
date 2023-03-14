// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__

#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "cam_cal_config.h"

unsigned int ov32b_mot_do_factory_verify(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData);

#define OV32B_MOT_EEPROM_ADDR 0x00
#define OV32B_MOT_EEPROM_DATA_SIZE 0x1066
#define OV32B_MOT_SERIAL_NUMBER_ADDR 0x15
#define OV32B_MOT_MNF_ADDR 0x00
#define OV32B_MOT_MNF_DATA_SIZE 37
#define OV32B_MOT_MNF_CHECKSUM_ADDR 0x25
#define OV32B_MOT_AWB_ADDR 0x41
#define OV32B_MOT_AWB_DATA_SIZE 43
#define OV32B_MOT_AWB_CHECKSUM_ADDR 0x6C
#define OV32B_MOT_LSC_ADDR 0x903
#define OV32B_MOT_LSC_DATA_SIZE 1868
#define OV32B_MOT_LSC_CHECKSUM_ADDR 0X104F


static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000003, 0x32443832, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000000, 0x00000000, 0x00000000, do_module_version},
		{0x00000000, 0x00000000, 0x00000000, do_part_number},
		{0x00000001, 0x00000903, 0x0000074C, mot_do_single_lsc},
		{0x00000001, 0x00000027, 0x00000045, mot_do_awb_gain},
		{0x00000000, 0x00000000, 0x00000000, mot_do_pdaf},
		{0x00000000, 0x00000000, 0x00000000, do_stereo_data},
		{0x00000000, 0x00000000, 0x00000000, do_dump_all},
		{0x00000000, 0x00000000, 0x00000000, do_lens_id},
		{0x00000001, 0x00000000, 0x00000025, mot_do_manufacture_info}
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT mot_ov32b_eeprom = {
	.name = "mot_ov32b_eeprom",
	.sensor_type = FRONT_CAMERA,
	.check_layout_function = mot_layout_no_ck,
	.read_function = Common_read_region,
	.mot_do_factory_verify_function = ov32b_mot_do_factory_verify,
	.layout = &cal_layout_table,
	.sensor_id = MOT_MANAUS_OV32B_SENSOR_ID,
	.i2c_write_id = 0xA2,
	.max_size = 0x8000,
	.serial_number_bit = 16,
	.enable_preload = 1,
	.preload_size = OV32B_MOT_EEPROM_DATA_SIZE,
};

unsigned int ov32b_mot_do_factory_verify(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData)
{
	struct STRUCT_MOT_EEPROM_DATA *pCamCalData =
				(struct STRUCT_MOT_EEPROM_DATA *)pGetSensorCalData;
	int read_data_size, checkSum;

	memset(&pCamCalData->CalibrationStatus, NONEXISTENCE, sizeof(pCamCalData->CalibrationStatus));

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			OV32B_MOT_EEPROM_ADDR, OV32B_MOT_EEPROM_DATA_SIZE, (unsigned char *)pCamCalData->DumpAllEepromData);
	if (read_data_size <= 0) {
		return CAM_CAL_ERR_NO_DEVICE;
	}
	//mnf check
	checkSum = (pCamCalData->DumpAllEepromData[OV32B_MOT_MNF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV32B_MOT_MNF_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData, OV32B_MOT_MNF_DATA_SIZE, checkSum)) {
		debug_log("check mnf crc16 ok");
		pCamCalData->CalibrationStatus.mnf_status= NO_ERRORS;
	} else {
		debug_log("check mnf crc16 err");
		pCamCalData->CalibrationStatus.mnf_status = CRC_FAILURE;
	}
	//for dump serial_number
	memcpy(pCamCalData->serial_number, &pCamCalData->DumpAllEepromData[OV32B_MOT_SERIAL_NUMBER_ADDR],
		pCamCalData->serial_number_bit);
	//awb check
	pCamCalData->CalibrationStatus.awb_status =
		mot_check_awb_data(pCamCalData->DumpAllEepromData+OV32B_MOT_AWB_ADDR, OV32B_MOT_AWB_DATA_SIZE + 2);
	if(pCamCalData->CalibrationStatus.awb_status == NO_ERRORS) {
		debug_log("check awb data ok");
	} else {
		debug_log("check awb data err");
	}

	//lsc check
	checkSum = (pCamCalData->DumpAllEepromData[OV32B_MOT_LSC_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV32B_MOT_LSC_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData+OV32B_MOT_LSC_ADDR, OV32B_MOT_LSC_DATA_SIZE, checkSum)) {
		debug_log("check lsc crc16 ok");
		pCamCalData->CalibrationStatus.lsc_status= NO_ERRORS;
	} else {
		debug_log("check lsc crc16 err");
		pCamCalData->CalibrationStatus.lsc_status = CRC_FAILURE;
	}
	return CAM_CAL_ERR_NO_ERR;
}

