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

unsigned int hi1336_mot_do_factory_verify(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData);
unsigned int mot_hi1336_do_2a_gain(struct EEPROM_DRV_FD_DATA *pdata,
		unsigned int start_addr, unsigned int block_size, unsigned int *pGetSensorCalData);

#define MOTO_OB_VALUE 64
#define MOTO_WB_VALUE_BASE 64
#define MOT_AWB_RB_MIN_VALUE 200
#define MOT_AWB_G_MIN_VALUE 760
#define MOT_AWB_RBG_MAX_VALUE 880
#define MOT_AWB_GRGB_RATIO_MIN_1000TIMES 970
#define MOT_AWB_GRGB_RATIO_MAX_1000TIMES 1030
#define MAX_temp(a,b,c) (a)>(b)?((a)>(c)?(a):(c)):((b)>(c)?(b):(c))
#define ABS(a,b) a>=b?(a-b):(b-a)

#define HI1336_MOT_EEPROM_ADDR 0x00
#define HI1336_MOT_EEPROM_DATA_SIZE 0x0E54
#define HI1336_MOT_SERIAL_NUMBER_ADDR 0x15
#define HI1336_MOT_MNF_ADDR 0x00
#define HI1336_MOT_MNF_DATA_SIZE 37
#define HI1336_MOT_MNF_CHECKSUM_ADDR 0x25
#define HI1336_MOT_AF_ADDR 0x27
#define HI1336_MOT_AF_DATA_SIZE 24
#define HI1336_MOT_AF_CHECKSUM_ADDR 0x3F
#define HI1336_MOT_AWB_ADDR 0x41
#define HI1336_MOT_AWB_DATA_SIZE 43
#define HI1336_MOT_AWB_CHECKSUM_ADDR 0x6C
#define HI1336_MOT_LSC_ADDR 0xF7
#define HI1336_MOT_LSC_DATA_SIZE 1868
#define HI1336_MOT_LSC_CHECKSUM_ADDR 0x843
#define HI1336_MOT_PDAF1_ADDR 0x845
#define HI1336_MOT_PDAF2_ADDR 0xA35
#define HI1336_MOT_PDAF_DATA1_SIZE 496
#define HI1336_MOT_PDAF_DATA2_SIZE 1004
#define HI1336_MOT_PDAF_CHECKSUM_ADDR 0xE21
#define HI1336_MOT_AF_SYNC_DATA_ADDR 0x0E25
#define HI1336_MOT_AF_SYNC_DATA_SIZE 24
#define HI1336_MOT_AF_SYNC_DATA_CHECKSUM_ADDR 0x0E3D
#define HI1336_MOT_MTK_NECESSARY_DATA_ADDR 0x0E3F
#define HI1336_MOT_MTK_NECESSARY_DATA_SIZE 19
#define HI1336_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR 0x0E52

static struct STRUCT_CALIBRATION_LAYOUT_STRUCT cal_layout_table = {
	0x00000003, 0x32443832, CAM_CAL_SINGLE_EEPROM_DATA,
	{
		{0x00000000, 0x00000000, 0x00000000, do_module_version},
		{0x00000000, 0x00000000, 0x00000E25, do_part_number},
		{0x00000001, 0x000000F7, 0x0000074C, mot_do_single_lsc},
		{0x00000001, 0x00000027, 0x00000045, mot_hi1336_do_2a_gain},
		{0x00000001, 0x00000845, 0x000005DC, mot_do_pdaf},
		{0x00000000, 0x00000FAE, 0x00000550, do_stereo_data},
		{0x00000000, 0x00000000, 0x00000E25, do_dump_all},
		{0x00000000, 0x00000F80, 0x0000000A, do_lens_id},
		{0x00000001, 0x00000000, 0x00000025, mot_do_manufacture_info},
	}
};

struct STRUCT_CAM_CAL_CONFIG_STRUCT mot_hi1336_eeprom = {
	.name = "mot_hi1336_eeprom",
	.sensor_type = UW_CAMERA,
	.check_layout_function = mot_layout_no_ck,
	.read_function = Common_read_region,
	.mot_do_factory_verify_function = hi1336_mot_do_factory_verify,
	.layout = &cal_layout_table,
	.sensor_id = MOT_AION_HI1336_SENSOR_ID,
	.i2c_write_id = 0xA0,
	.max_size = 0x8000,
	.serial_number_bit = 16,
	.enable_preload = 1,
	.preload_size = HI1336_MOT_EEPROM_DATA_SIZE,
};


unsigned int hi1336_mot_do_factory_verify(struct EEPROM_DRV_FD_DATA *pdata, unsigned int *pGetSensorCalData)
{
	struct STRUCT_MOT_EEPROM_DATA *pCamCalData =
				(struct STRUCT_MOT_EEPROM_DATA *)pGetSensorCalData;
	int read_data_size, checkSum, checkSum1;

	memset(&pCamCalData->CalibrationStatus, NO_ERRORS, sizeof(pCamCalData->CalibrationStatus));

	read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
			HI1336_MOT_EEPROM_ADDR, HI1336_MOT_EEPROM_DATA_SIZE, (unsigned char *)pCamCalData->DumpAllEepromData);
	if (read_data_size <= 0) {
		return CAM_CAL_ERR_NO_DEVICE;
	}
	//mnf check
	checkSum = (pCamCalData->DumpAllEepromData[HI1336_MOT_MNF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[HI1336_MOT_MNF_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData, HI1336_MOT_MNF_DATA_SIZE, checkSum)) {
		debug_log("check mnf crc16 ok");
		pCamCalData->CalibrationStatus.mnf_status= NO_ERRORS;
	} else {
		debug_log("check mnf crc16 err");
		pCamCalData->CalibrationStatus.mnf_status = CRC_FAILURE;
	}
	//for dump serial_number
	memcpy(pCamCalData->serial_number, &pCamCalData->DumpAllEepromData[HI1336_MOT_SERIAL_NUMBER_ADDR],
		pCamCalData->serial_number_bit);

	//af check
	checkSum = (pCamCalData->DumpAllEepromData[HI1336_MOT_AF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[HI1336_MOT_AF_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData+HI1336_MOT_AF_ADDR, HI1336_MOT_AF_DATA_SIZE, checkSum)) {
		debug_log("check af crc16 ok");
		pCamCalData->CalibrationStatus.af_status = NO_ERRORS;
	} else {
		debug_log("check af crc16 err");
		pCamCalData->CalibrationStatus.af_status = CRC_FAILURE;
	}

	//awb check
	pCamCalData->CalibrationStatus.awb_status =
		mot_check_awb_data(pCamCalData->DumpAllEepromData+HI1336_MOT_AWB_ADDR, HI1336_MOT_AWB_DATA_SIZE + 2);
	if(pCamCalData->CalibrationStatus.awb_status == NO_ERRORS) {
		debug_log("check awb data ok");
	} else {
		debug_log("check awb data err");
	}

	//lsc check
	checkSum = (pCamCalData->DumpAllEepromData[HI1336_MOT_LSC_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[HI1336_MOT_LSC_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);

	if(check_crc16(pCamCalData->DumpAllEepromData+HI1336_MOT_LSC_ADDR, HI1336_MOT_LSC_DATA_SIZE, checkSum)) {
		debug_log("check lsc crc16 ok");
		pCamCalData->CalibrationStatus.lsc_status= NO_ERRORS;
	} else {
		debug_log("check lsc crc16 err");
		pCamCalData->CalibrationStatus.lsc_status = CRC_FAILURE;
	}

	//pdaf check
	checkSum = (pCamCalData->DumpAllEepromData[HI1336_MOT_PDAF_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[HI1336_MOT_PDAF_CHECKSUM_ADDR+1]);
	checkSum1 = (pCamCalData->DumpAllEepromData[HI1336_MOT_PDAF_CHECKSUM_ADDR+2])<< 8
		|(pCamCalData->DumpAllEepromData[HI1336_MOT_PDAF_CHECKSUM_ADDR+3]);
	debug_log("checkSum  = 0x%x, checkSum1 = 0x%x", checkSum, checkSum1);

	if(check_crc16(pCamCalData->DumpAllEepromData+HI1336_MOT_PDAF1_ADDR, HI1336_MOT_PDAF_DATA1_SIZE, checkSum)
		&& check_crc16(pCamCalData->DumpAllEepromData+HI1336_MOT_PDAF2_ADDR, HI1336_MOT_PDAF_DATA2_SIZE, checkSum1)) {
		debug_log("check pdaf crc16 ok");
		pCamCalData->CalibrationStatus.pdaf_status= NO_ERRORS;
	} else {
		debug_log("check pdaf crc16 err");
		pCamCalData->CalibrationStatus.pdaf_status = CRC_FAILURE;
	}

	//af sync data check
	checkSum = (pCamCalData->DumpAllEepromData[HI1336_MOT_AF_SYNC_DATA_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[HI1336_MOT_AF_SYNC_DATA_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);
	if(check_crc16(pCamCalData->DumpAllEepromData+HI1336_MOT_AF_SYNC_DATA_ADDR, HI1336_MOT_AF_SYNC_DATA_SIZE, checkSum)) {
		debug_log("check af sync data crc16 ok");
		pCamCalData->CalibrationStatus.af_sync_status = NO_ERRORS;
	} else {
		debug_log("check af sync data crc16 err");
		pCamCalData->CalibrationStatus.af_sync_status = CRC_FAILURE;
	}
	//mtk necessary data check
	checkSum = (pCamCalData->DumpAllEepromData[HI1336_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR])<< 8
		|(pCamCalData->DumpAllEepromData[HI1336_MOT_MTK_NECESSARY_DATA_CHECKSUM_ADDR+1]);
	debug_log("checkSum  = 0x%x", checkSum);
	if(check_crc16(pCamCalData->DumpAllEepromData+HI1336_MOT_MTK_NECESSARY_DATA_ADDR, HI1336_MOT_MTK_NECESSARY_DATA_SIZE, checkSum)) {
		debug_log("check mtk necessary data crc16 ok");
		pCamCalData->CalibrationStatus.mtk_necessary_status = NO_ERRORS;
	} else {
		debug_log("check mtk necessary data crc16 err");
		pCamCalData->CalibrationStatus.mtk_necessary_status = CRC_FAILURE;
	}
	return CAM_CAL_ERR_NO_ERR;
}

unsigned int mot_hi1336_do_2a_gain(struct EEPROM_DRV_FD_DATA *pdata,
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
	int af_addr = HI1336_MOT_AF_ADDR;
	int af_size = HI1336_MOT_AF_DATA_SIZE;
	int awb_addr = HI1336_MOT_AWB_ADDR;
	int awb_size = HI1336_MOT_AWB_DATA_SIZE;

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

	if(pCamCalData->sensorID == MOT_AION_HI1336_SENSOR_ID) {
		//af posture calibration data
		unsigned char AF_POSTURE[21];
		unsigned int af_posture_data_offset = 0x0e3f;
		unsigned int af_inf_posture, af_macro_posture, AF_infinite_calibration_temperature;
		unsigned int MiddleDac;
		memset(AF_POSTURE, 0, 21);
		debug_log("af_posture_data_offset = 0x%x\n", af_posture_data_offset);

		read_data_size = read_data(pdata, pCamCalData->sensorID, pCamCalData->deviceID,
				af_posture_data_offset, 21, (unsigned char *) AF_POSTURE);
		if (read_data_size > 0)
			err = CAM_CAL_ERR_NO_ERR;
		else {
			pCamCalData->Single2A.S2aBitEn = CAM_CAL_NONE_BITEN;
			error_log("Read Failed\n");
			show_cmd_error_log(pCamCalData->Command);
		}

		checkSum = AF_POSTURE[19]<<8 | AF_POSTURE[20];
		if(check_crc16(AF_POSTURE, 19, checkSum)) {
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
		MiddleDac = AFInf+(AFMacro-AFInf)*47/199;  //refer to af lensShift excel
		debug_log("MiddleDac = %d", MiddleDac);
		pCamCalData->Single2A.S2aAF_t.AF_Middle_calibration = MiddleDac;
	}

	return err;
}

