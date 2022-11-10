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

unsigned int s5kgd2_mot_do_factory_verify(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData);

#define S5KGD2_MOT_EEPROM_ADDR 0x00
#define S5KGD2_MOT_EEPROM_DATA_SIZE 0x1911
#define S5KGD2_MOT_SERIAL_NUMBER_ADDR 0x15
#define S5KGD2_MOT_MNF_ADDR 0x00
#define S5KGD2_MOT_MNF_DATA_SIZE 37
#define S5KGD2_MOT_MNF_CHECKSUM_ADDR 0x25
#define S5KGD2_MOT_AF_ADDR 0x27
#define S5KGD2_MOT_AF_DATA_SIZE 24
#define S5KGD2_MOT_AF_CHECKSUM_ADDR 0x3F
#define S5KGD2_MOT_AWB_ADDR 0x41
#define S5KGD2_MOT_AWB_DATA_SIZE 43
#define S5KGD2_MOT_AWB_CHECKSUM_ADDR 0x6C
#define S5KGD2_MOT_LSC_ADDR 0x0BC8
#define S5KGD2_MOT_LSC_DATA_SIZE 1868
#define S5KGD2_MOT_LSC_CHECKSUM_ADDR 0x1314
#define S5KGD2_MOT_PDAF1_ADDR 0x1316
#define S5KGD2_MOT_PDAF2_ADDR 0x1506
#define S5KGD2_MOT_PDAF_DATA1_SIZE 496
#define S5KGD2_MOT_PDAF_DATA2_SIZE 1004
#define S5KGD2_MOT_PDAF_CHECKSUM_ADDR 0x18F2

#define S5KGD2_MOT_MTK_NECESSARY_DATA_ADDR 0x18F6
#define S5KGD2_MOT_MTK_NECESSARY_DATA_SIZE 19
#define S5KGD2_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR 0x01909

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000003, 0x32443832, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000000, 0x00000000, 0x00000000, do_module_version},
		{0x00000000, 0x00000000, 0x00000000, do_part_number},
		{0x00000001, 0x00000BC8, 0x0000074C, mot_do_single_lsc},
		{0x00000001, 0x00000027, 0x00000045, mot_do_2a_gain},
		{0x00000001, 0x00001316, 0x000005DC, mot_do_pdaf},
		{0x00000000, 0x00000FAE, 0x00000550, do_stereo_data},
		{0x00000000, 0x00000000, 0x00000E25, do_dump_all},
		{0x00000000, 0x00000F80, 0x0000000A, do_lens_id},
		{0x00000001, 0x00000000, 0x00000025, mot_do_manufacture_info},
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT mot_s5kgd2_eeprom = {
	.name = "mot_s5kgd2_eeprom",
	.sensor_type = FRONT_CAMERA,
	.check_layout_function = mot_layout_no_ck,
	.read_function = Common_read_region,
	.mot_do_factory_verify_function = s5kgd2_mot_do_factory_verify,
	.layout = &cal_layout_table,
	.sensor_id = MOT_AION_S5KGD2_SENSOR_ID,
	.i2c_write_id = 0xA2,
	.max_size = 0x8000,
	.serial_number_bit = 16,
	.enable_preload = 1,
	.preload_size = S5KGD2_MOT_EEPROM_DATA_SIZE,
};


unsigned int s5kgd2_mot_do_factory_verify(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData)
{
	struct STRUCT_MOT_EEPROM_DATA *pCamCalData =
				(struct STRUCT_MOT_EEPROM_DATA *)pGetSensorCalData;
	int read_data_size, checkSum, checkSum1;

	memset(&pCamCalData->CalibrationStatus, NONEXISTENCE, sizeof(pCamCalData->CalibrationStatus));

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			S5KGD2_MOT_EEPROM_ADDR, S5KGD2_MOT_EEPROM_DATA_SIZE, (unsigned char *)pCamCalData->DumpAllEepromData);
	if (read_data_size <= 0) {
		return CAM_CAL_ERR_NO_DEVICE;
	}
	//mnf check
	checkSum = (pCamCalData->DumpAllEepromData[S5KGD2_MOT_MNF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[S5KGD2_MOT_MNF_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData, S5KGD2_MOT_MNF_DATA_SIZE, checkSum)) {
		debug_log("check mnf crc16 ok");
		pCamCalData->CalibrationStatus.mnf_status= NO_ERRORS;
	} else {
		debug_log("check mnf crc16 err");
		pCamCalData->CalibrationStatus.mnf_status = CRC_FAILURE;
	}
	//for dump serial_number
	memcpy(pCamCalData->serial_number, &pCamCalData->DumpAllEepromData[S5KGD2_MOT_SERIAL_NUMBER_ADDR],
		pCamCalData->serial_number_bit);

	//af check
	checkSum = (pCamCalData->DumpAllEepromData[S5KGD2_MOT_AF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[S5KGD2_MOT_AF_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData+S5KGD2_MOT_AF_ADDR, S5KGD2_MOT_AF_DATA_SIZE, checkSum)) {
		debug_log("check af crc16 ok");
		pCamCalData->CalibrationStatus.af_status = NO_ERRORS;
	} else {
		debug_log("check af crc16 err");
		pCamCalData->CalibrationStatus.af_status = CRC_FAILURE;
	}

	//awb check
	pCamCalData->CalibrationStatus.awb_status =
		mot_check_awb_data(pCamCalData->DumpAllEepromData+S5KGD2_MOT_AWB_ADDR, S5KGD2_MOT_AWB_DATA_SIZE + 2);
	if(pCamCalData->CalibrationStatus.awb_status == NO_ERRORS) {
		debug_log("check awb data ok");
	} else {
		debug_log("check awb data err");
	}

	//lsc check
	checkSum = (pCamCalData->DumpAllEepromData[S5KGD2_MOT_LSC_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[S5KGD2_MOT_LSC_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData+S5KGD2_MOT_LSC_ADDR, S5KGD2_MOT_LSC_DATA_SIZE, checkSum)) {
		debug_log("check lsc crc16 ok");
		pCamCalData->CalibrationStatus.lsc_status= NO_ERRORS;
	} else {
		debug_log("check lsc crc16 err");
		pCamCalData->CalibrationStatus.lsc_status = CRC_FAILURE;
	}

	//pdaf check
	checkSum = (pCamCalData->DumpAllEepromData[S5KGD2_MOT_PDAF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[S5KGD2_MOT_PDAF_CHECKSUM_ADDR+1]);
	checkSum1 = (pCamCalData->DumpAllEepromData[S5KGD2_MOT_PDAF_CHECKSUM_ADDR+2])<< 8
		|(pCamCalData->DumpAllEepromData[S5KGD2_MOT_PDAF_CHECKSUM_ADDR+3]);
	debug_log("checkSum  = 0x%x, checkSum1 = 0x%x", checkSum, checkSum1);

	if(check_crc16(pCamCalData->DumpAllEepromData+S5KGD2_MOT_PDAF1_ADDR, S5KGD2_MOT_PDAF_DATA1_SIZE, checkSum)
		&& check_crc16(pCamCalData->DumpAllEepromData+S5KGD2_MOT_PDAF2_ADDR, S5KGD2_MOT_PDAF_DATA2_SIZE, checkSum1)) {
		debug_log("check pdaf crc16 ok");
		pCamCalData->CalibrationStatus.pdaf_status= NO_ERRORS;
	} else {
		debug_log("check pdaf crc16 err");
		pCamCalData->CalibrationStatus.pdaf_status = CRC_FAILURE;
	}

	//mtk necessary data check
	checkSum = (pCamCalData->DumpAllEepromData[S5KGD2_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[S5KGD2_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);
	if(check_crc16(pCamCalData->DumpAllEepromData+S5KGD2_MOT_MTK_NECESSARY_DATA_ADDR, S5KGD2_MOT_MTK_NECESSARY_DATA_SIZE, checkSum)) {
		debug_log("check mtk necessary data crc16 ok");
		pCamCalData->CalibrationStatus.mtk_necessary_status = NO_ERRORS;
	} else {
		debug_log("check mtk necessary data crc16 err");
		pCamCalData->CalibrationStatus.mtk_necessary_status = CRC_FAILURE;
	}
	return CAM_CAL_ERR_NO_ERR;
}
