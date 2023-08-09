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

unsigned int ov50a_mot_do_factory_verify(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData);

#define OV50A_MOT_EEPROM_ADDR 0x00
//BEGIN OF IKSWT-185759
#define OV50A_MOT_EEPROM_DATA_SIZE 0x23E9
//END OF IKSWT-185759
#define OV50A_MOT_SERIAL_NUMBER_ADDR 0x15
#define OV50A_MOT_MNF_ADDR 0x00
#define OV50A_MOT_MNF_DATA_SIZE 37
#define OV50A_MOT_MNF_CHECKSUM_ADDR 0x25
#define OV50A_MOT_AF_ADDR 0x27
#define OV50A_MOT_AF_DATA_SIZE 24
#define OV50A_MOT_AF_CHECKSUM_ADDR 0x3F
#define OV50A_MOT_AWB_ADDR 0x41
#define OV50A_MOT_AWB_DATA_SIZE 43
#define OV50A_MOT_AWB_CHECKSUM_ADDR 0x6C
#define OV50A_MOT_LSC_ADDR 0x7E1
#define OV50A_MOT_LSC_DATA_SIZE 1868
#define OV50A_MOT_LSC_CHECKSUM_ADDR 0XF2D
#define OV50A_MOT_PDAF1_ADDR 0xF2F
#define OV50A_MOT_PDAF2_ADDR 0x111F
#define OV50A_MOT_PDAF_DATA1_SIZE 496
#define OV50A_MOT_PDAF_DATA2_SIZE 1004
#define OV50A_MOT_PDAF_CHECKSUM_ADDR 0x150B
#define OV50A_MOT_XTALK_ADDR 0x150F
#define OV50A_MOT_XTALK_DATA_SIZE 3568
#define OV50A_MOT_XTALK_CHECKSUM_ADDR 0x22FF
#define OV50A_MOT_AF_SYNC_DATA_ADDR 0x2301
#define OV50A_MOT_AF_SYNC_DATA_SIZE 16
#define OV50A_MOT_AF_SYNC_DATA_CHECKSUM_ADDR 0x2311
#define OV50A_MOT_MTK_NECESSARY_DATA_ADDR 0x2313
#define OV50A_MOT_MTK_NECESSARY_DATA_SIZE 18
#define OV50A_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR 0x2325

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000003, 0x32443832, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000000, 0x00000000, 0x00000000, do_module_version},
		{0x00000000, 0x00000000, 0x00000025, do_part_number},
		{0x00000001, 0x000007E1, 0x0000074C, mot_do_single_lsc},
		{0x00000001, 0x00000027, 0x00000045, mot_do_2a_gain},
		{0x00000001, 0x00000F2F, 0x000005DC, mot_do_pdaf},
		{0x00000000, 0x00000FAE, 0x00000550, do_stereo_data},
		{0x00000000, 0x00000000, 0x00000E25, do_dump_all},
		{0x00000000, 0x00000F80, 0x0000000A, do_lens_id},
		{0x00000001, 0x00000000, 0x00000025, mot_do_manufacture_info}
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT mot_ov50a_eeprom = {
	.name = "mot_ov50a_eeprom",
	.sensor_type = MAIN_CAMERA,
	.check_layout_function = mot_layout_no_ck,
	.read_function = Common_read_region,
	.mot_do_factory_verify_function = ov50a_mot_do_factory_verify,
	.layout = &cal_layout_table,
	.sensor_id = MOT_MANAUS_OV50A_SENSOR_ID,
	.i2c_write_id = 0xA0,
	.max_size = 0x8000,
	.serial_number_bit = 16,
	.enable_preload = 1,
	.preload_size = OV50A_MOT_EEPROM_DATA_SIZE,
};

unsigned int ov50a_mot_do_factory_verify(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData)
{
	struct STRUCT_MOT_EEPROM_DATA *pCamCalData =
				(struct STRUCT_MOT_EEPROM_DATA *)pGetSensorCalData;
	int read_data_size, checkSum, checkSum1;

	memset(&pCamCalData->CalibrationStatus, NO_ERRORS, sizeof(pCamCalData->CalibrationStatus));

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			OV50A_MOT_EEPROM_ADDR, OV50A_MOT_EEPROM_DATA_SIZE, (unsigned char *)pCamCalData->DumpAllEepromData);
	if (read_data_size <= 0) {
		return CAM_CAL_ERR_NO_DEVICE;
	}
	//mnf check
	checkSum = (pCamCalData->DumpAllEepromData[OV50A_MOT_MNF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV50A_MOT_MNF_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData, OV50A_MOT_MNF_DATA_SIZE, checkSum)) {
		debug_log("check mnf crc16 ok");
		pCamCalData->CalibrationStatus.mnf_status= NO_ERRORS;
	} else {
		debug_log("check mnf crc16 err");
		pCamCalData->CalibrationStatus.mnf_status = CRC_FAILURE;
	}
	//for dump serial_number
	memcpy(pCamCalData->serial_number, &pCamCalData->DumpAllEepromData[OV50A_MOT_SERIAL_NUMBER_ADDR],
		pCamCalData->serial_number_bit);

	//af check
	checkSum = (pCamCalData->DumpAllEepromData[OV50A_MOT_AF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV50A_MOT_AF_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData+OV50A_MOT_AF_ADDR, OV50A_MOT_AF_DATA_SIZE, checkSum)) {
		debug_log("check af crc16 ok");
		pCamCalData->CalibrationStatus.af_status = NO_ERRORS;
	} else {
		debug_log("check af crc16 err");
		pCamCalData->CalibrationStatus.af_status = CRC_FAILURE;
	}

	//awb check
	pCamCalData->CalibrationStatus.awb_status =
		mot_check_awb_data(pCamCalData->DumpAllEepromData+OV50A_MOT_AWB_ADDR, OV50A_MOT_AWB_DATA_SIZE + 2);
	if(pCamCalData->CalibrationStatus.awb_status == NO_ERRORS) {
		debug_log("check awb data ok");
	} else {
		debug_log("check awb data err");
	}

	//lsc check
	checkSum = (pCamCalData->DumpAllEepromData[OV50A_MOT_LSC_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV50A_MOT_LSC_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData+OV50A_MOT_LSC_ADDR, OV50A_MOT_LSC_DATA_SIZE, checkSum)) {
		debug_log("check lsc crc16 ok");
		pCamCalData->CalibrationStatus.lsc_status= NO_ERRORS;
	} else {
		debug_log("check lsc crc16 err");
		pCamCalData->CalibrationStatus.lsc_status = CRC_FAILURE;
	}

	//pdaf check
	checkSum = (pCamCalData->DumpAllEepromData[OV50A_MOT_PDAF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV50A_MOT_PDAF_CHECKSUM_ADDR+1]);
	checkSum1 = (pCamCalData->DumpAllEepromData[OV50A_MOT_PDAF_CHECKSUM_ADDR+2])<< 8
		|(pCamCalData->DumpAllEepromData[OV50A_MOT_PDAF_CHECKSUM_ADDR+3]);
	debug_log("checkSum  = 0x%x, checkSum1 = 0x%x", checkSum, checkSum1);

	if(check_crc16(pCamCalData->DumpAllEepromData+OV50A_MOT_PDAF1_ADDR, OV50A_MOT_PDAF_DATA1_SIZE, checkSum)
		&& check_crc16(pCamCalData->DumpAllEepromData+OV50A_MOT_PDAF2_ADDR, OV50A_MOT_PDAF_DATA2_SIZE, checkSum1)) {
		debug_log("check pdaf crc16 ok");
		pCamCalData->CalibrationStatus.pdaf_status= NO_ERRORS;
	} else {
		debug_log("check pdaf crc16 err");
		pCamCalData->CalibrationStatus.pdaf_status = CRC_FAILURE;
	}

	//xtalk check
	checkSum = (pCamCalData->DumpAllEepromData[OV50A_MOT_XTALK_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV50A_MOT_XTALK_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData+OV50A_MOT_XTALK_ADDR, OV50A_MOT_XTALK_DATA_SIZE, checkSum)) {
		debug_log("check xtalk data crc16 ok");
		pCamCalData->CalibrationStatus.xtalk_status = NO_ERRORS;
	} else {
		debug_log("check xtalk data crc16 err");
		pCamCalData->CalibrationStatus.xtalk_status = CRC_FAILURE;
	}

	//af sync data check
	checkSum = (pCamCalData->DumpAllEepromData[OV50A_MOT_AF_SYNC_DATA_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV50A_MOT_AF_SYNC_DATA_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);
	if(check_crc16(pCamCalData->DumpAllEepromData+OV50A_MOT_AF_SYNC_DATA_ADDR, OV50A_MOT_AF_SYNC_DATA_SIZE, checkSum)) {
		debug_log("check af sync data crc16 ok");
		pCamCalData->CalibrationStatus.af_sync_status = NO_ERRORS;
	} else {
		debug_log("check af sync data crc16 err");
		pCamCalData->CalibrationStatus.af_sync_status = CRC_FAILURE;
	}

	//mtk necessary data check
	checkSum = (pCamCalData->DumpAllEepromData[OV50A_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[OV50A_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);
	if(check_crc16(pCamCalData->DumpAllEepromData+OV50A_MOT_MTK_NECESSARY_DATA_ADDR, OV50A_MOT_MTK_NECESSARY_DATA_SIZE, checkSum)) {
		debug_log("check necessary data crc16 ok");
		pCamCalData->CalibrationStatus.mtk_necessary_status = NO_ERRORS;
	} else {
		debug_log("check necessary data crc16 err");
		pCamCalData->CalibrationStatus.mtk_necessary_status = CRC_FAILURE;
	}
	return CAM_CAL_ERR_NO_ERR;
}
