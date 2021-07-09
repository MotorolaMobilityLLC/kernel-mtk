/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define PFX "MOTO_EEPROM"
#include "kd_camera_typedef.h"
#include "custom_eeprom_driver.h"

#define MAX_EEPROM_LAYOUT_NUM 4
#define MY 0
#define CHECK_SNPRINTF_RET(ret, str, msg)           \
    if (ret < 0 || ret >= MAX_CALIBRATION_STRING) { \
        LOG_ERR(msg);                               \
        str[0] = 0;                                 \
    }

static unsigned int g_lastDevID;
extern struct i2c_client *g_pstI2Cclients[I2C_DEV_IDX_MAX];

MOT_EEPROM_CAL CalcheckTbl[MAX_EEPROM_LAYOUT_NUM] =
{
	{
		{
			.sensorID= MOT_COFUL_S5KJN1_QTECH_ID,
			.deviceID = 0x01,
			.dataLength = 0x19CB,
			.sensorVendorid = 0x16020000,
			.vendorByte = {1,2,3,4},
			.dataBuffer = NULL
		},
		{
			{0x00000001, 0x00000000, 0x00000025, mot_check_mnf_data },
			{0x00000001, 0x18F60027, 0x00100018, mot_check_af_data  },//High 16 bit: AF sync data; Low 16 bit: AF inf/macro data.
			{0x00000001, 0x00000041, 0x0000002B, mot_check_awb_data },
			{0x00000001, 0x00000BC8, 0x0000074C, mot_check_lsc_data },
			{0x00000001, 0x13161506, 0x01F003EC, mot_check_pdaf_data},//High 16 bit: pdaf output1 data; Low 16 bit: pdaf output2 data.
			{0x00000001, 0x00000015, 0x00000010, NULL},//dump serial number.
		}
	},
	{
		{
			.sensorID= MOT_CORFU_HI1336_OFILM_ID,
			.deviceID = 0x02,
			.dataLength = 0x0F43,
			.sensorVendorid = 0x16020000,
			.vendorByte = {1,2,3,4},
			.dataBuffer = NULL
		},
		{
			{0x00000001, 0x00000000, 0x00000025, mot_check_mnf_data },
			{0x00000000, 0x00000005, 0x00000002, mot_check_af_data  },
			{0x00000001, 0x00000041, 0x0000002B, mot_check_awb_data },
			{0x00000001, 0x000007E1, 0x0000074C, mot_check_lsc_data },
			{0x00000000, 0x00000763, 0x00000800, mot_check_pdaf_data},
			{0x00000000, 0x00000015, 0x00000010, NULL},//dump serial number.
		}
	},
};

static int EEPROM_get_cmd_info(unsigned int sensorID,
	struct stCAM_CAL_CMD_INFO_STRUCT *cmdInfo)
{
	struct stCAM_CAL_LIST_STRUCT *pCamCalList = NULL;
	int i = 0;

	LOG_INF("%s enter", __func__);

	cam_cal_get_sensor_list(&pCamCalList);
	if (pCamCalList != NULL) {
		LOG_INF("pCamCalList!=NULL && pCamCalFunc!= NULL\n");
		for (i = 0; pCamCalList[i].sensorID != 0; i++) {
			if (pCamCalList[i].sensorID == sensorID) {
				LOG_INF("pCamCalList[%d].sensorID==%x\n", i,
				       pCamCalList[i].sensorID);

				cmdInfo->i2cAddr = pCamCalList[i].slaveID >> 1;
				cmdInfo->readCMDFunc =
					pCamCalList[i].readCamCalData;
				cmdInfo->maxEepromSize =
					pCamCalList[i].maxEepromSize;

				/*
				 * Default 8K for Common_read_region driver
				 * 0 for others
				 */
				if (cmdInfo->readCMDFunc == Common_read_region
				    && cmdInfo->maxEepromSize == 0) {
					cmdInfo->maxEepromSize =
						DEFAULT_MAX_EEPROM_SIZE_8K;
				}

				return 1;
			}
		}
	}
	return 0;

}

static struct stCAM_CAL_CMD_INFO_STRUCT *EEPROM_get_cmd_info_ex
	(unsigned int sensorID, unsigned int deviceID)
{
	int i = 0;

	LOG_INF("%s enter", __func__);
	/* To check device ID */
	for (i = 0; i < IMGSENSOR_SENSOR_IDX_MAX_NUM; i++) {
		if (g_camCalDrvInfo[i].deviceID == deviceID)
			break;
	}
	/* To check cmd from Sensor ID */

	if (i == IMGSENSOR_SENSOR_IDX_MAX_NUM) {
		for (i = 0; i < IMGSENSOR_SENSOR_IDX_MAX_NUM; i++) {
			/* To Set Client */
			if (g_camCalDrvInfo[i].sensorID == 0) {
				LOG_INF("Start get_cmd_info!\n");
				EEPROM_get_cmd_info(sensorID,
					&g_camCalDrvInfo[i]);

				if (g_camCalDrvInfo[i].readCMDFunc != NULL) {
					g_camCalDrvInfo[i].sensorID = sensorID;
					g_camCalDrvInfo[i].deviceID = deviceID;
					LOG_INF("deviceID=%d, SensorID=%x\n",
						deviceID, sensorID);
				}
				break;
			}
		}
	}

	if (i == IMGSENSOR_SENSOR_IDX_MAX_NUM) {/*g_camCalDrvInfo is full */
		return NULL;
	} else {
		return &g_camCalDrvInfo[i];
	}
}

static int EEPROM_set_i2c_bus(unsigned int deviceID,
			      struct stCAM_CAL_CMD_INFO_STRUCT *cmdInfo)
{
	enum IMGSENSOR_SENSOR_IDX idx;
	enum EEPROM_I2C_DEV_IDX i2c_idx;
	struct i2c_client *client;

	LOG_INF("%s enter", __func__);

	idx = IMGSENSOR_SENSOR_IDX_MAP(deviceID);
	i2c_idx = get_i2c_dev_sel(idx);

	if (idx == IMGSENSOR_SENSOR_IDX_NONE)
		return -EFAULT;

	if (i2c_idx < I2C_DEV_IDX_1 || i2c_idx >= I2C_DEV_IDX_MAX)
		return -EFAULT;

	client = g_pstI2Cclients[i2c_idx];
	LOG_INF("%s end! deviceID=%d index=%u i2c_idx=%u client=%p\n",
		 __func__, deviceID, idx, i2c_idx, client);

	if (client == NULL) {
		pr_err("i2c client is NULL");
		return -EFAULT;
	}

	if (cmdInfo != NULL) {
		client->addr = cmdInfo->i2cAddr;
		cmdInfo->client = client;
	}

	return 0;
}


static unsigned int read_region(struct stCAM_CAL_DATAINFO_STRUCT* ptempbuf)
{
	int i4RetValue = 0;
	struct stCAM_CAL_CMD_INFO_STRUCT *pcmdInf = NULL;

	LOG_INF("SensorID=%x DeviceID=%x\n",
		ptempbuf->sensorID, ptempbuf->deviceID);
	pcmdInf = EEPROM_get_cmd_info_ex(
		ptempbuf->sensorID,
		ptempbuf->deviceID);

	/* Check the max size if specified */
	if (pcmdInf != NULL &&
	    (pcmdInf->maxEepromSize != 0) &&
	    (pcmdInf->maxEepromSize < ptempbuf->dataLength)) {
		LOG_ERR("Error!! not support address >= 0x%x!!\n",
			 pcmdInf->maxEepromSize);
		return -EFAULT;
	}

	if (pcmdInf != NULL && g_lastDevID != ptempbuf->deviceID) {
		if (EEPROM_set_i2c_bus(ptempbuf->deviceID,
				       pcmdInf) != 0) {
			LOG_ERR("deviceID Error!\n");
			return -EFAULT;
		}
		g_lastDevID = ptempbuf->deviceID;
	}

	if (pcmdInf != NULL) {
		if (pcmdInf->readCMDFunc != NULL)
			i4RetValue =
				pcmdInf->readCMDFunc(pcmdInf->client,
						  0x0,
						  ptempbuf->dataBuffer,
						  ptempbuf->dataLength);
		else {
			LOG_ERR("pcmdInf->readCMDFunc == NULL\n");
			return -EFAULT;
		}
	}

	LOG_INF("i4RetValue=%d", i4RetValue);
	return 0;
}

static void eeprom_dump_bin(const char *file_name, uint32_t size, const void *data)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0666);
	if (IS_ERR_OR_NULL(fp))
	{
		ret = PTR_ERR(fp);
		LOG_INF("open file error(%s), error(%d)\n",  file_name, ret);
		goto p_err;
	}

	ret = vfs_write(fp, (const char *)data, size, &fp->f_pos);
	if (ret < 0)
	{
		LOG_INF("file write fail(%s) to EEPROM data(%d)", file_name, ret);
		goto p_err;
	}

	LOG_INF("wirte to file(%s)\n", file_name);
p_err:
	if (!IS_ERR_OR_NULL(fp))
		filp_close(fp, NULL);

	set_fs(old_fs);
	LOG_INF(" end writing file");
}

static uint32_t convert_crc(uint8_t *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
}

static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static uint16_t to_uint16_swap(uint8_t *data)
{
	uint16_t converted;
	memcpy(&converted, data, sizeof(uint16_t));
	return ntohs(converted);
}

int32_t eeprom_util_check_crc16(uint8_t *data, uint32_t size, uint32_t ref_crc)
{
	int32_t crc_match = 0;
	uint16_t crc = 0x0000;
	uint16_t crc_reverse = 0x0000;
	uint32_t i, j;

	uint32_t tmp;
	uint32_t tmp_reverse;

	/* Calculate both methods of CRC since integrators differ on
	* how CRC should be calculated. */
	for (i = 0; i < size; i++)
	{
		tmp_reverse = crc_reverse_byte(data[i]);
		tmp = data[i] & 0xff;
		for (j = 0; j < 8; j++)
		{
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

static uint8_t mot_eeprom_util_check_awb_limits(awb_t unit, awb_t golden)
{
	uint8_t result = 0;

	if (unit.r < AWB_R_MIN || unit.r > AWB_R_MAX)
	{
		LOG_ERR("unit r out of range! MIN: %d, r: %d, MAX: %d",
			AWB_R_MIN, unit.r, AWB_R_MAX);
		result = 1;
	}
	if (unit.gr < AWB_GR_MIN || unit.gr > AWB_GR_MAX)
	{
		LOG_ERR("unit gr out of range! MIN: %d, gr: %d, MAX: %d",
			AWB_GR_MIN, unit.gr, AWB_GR_MAX);
		result = 1;
	}
	if (unit.gb < AWB_GB_MIN || unit.gb > AWB_GB_MAX)
	{
		LOG_ERR("unit gb out of range! MIN: %d, gb: %d, MAX: %d",
			AWB_GB_MIN, unit.gb, AWB_GB_MAX);
		result = 1;
	}
	if (unit.b < AWB_B_MIN || unit.b > AWB_B_MAX)
	{
		LOG_ERR("unit b out of range! MIN: %d, b: %d, MAX: %d",
			AWB_B_MIN, unit.b, AWB_B_MAX);
		result = 1;
	}

	if (golden.r < AWB_R_MIN || golden.r > AWB_R_MAX)
	{
		LOG_ERR("golden r out of range! MIN: %d, r: %d, MAX: %d",
			AWB_R_MIN, golden.r, AWB_R_MAX);
		result = 1;
	}
	if (golden.gr < AWB_GR_MIN || golden.gr > AWB_GR_MAX)
	{
		LOG_ERR("golden gr out of range! MIN: %d, gr: %d, MAX: %d",
			AWB_GR_MIN, golden.gr, AWB_GR_MAX);
		result = 1;
	}
	if (golden.gb < AWB_GB_MIN || golden.gb > AWB_GB_MAX)
	{
		LOG_ERR("golden gb out of range! MIN: %d, gb: %d, MAX: %d",
			AWB_GB_MIN, golden.gb, AWB_GB_MAX);
		result = 1;
	}
	if (golden.b < AWB_B_MIN || golden.b > AWB_B_MAX)
	{
		LOG_ERR("golden b out of range! MIN: %d, b: %d, MAX: %d",
			AWB_B_MIN, golden.b, AWB_B_MAX);
		result = 1;
	}

	return result;
}

static uint8_t mot_eeprom_util_calculate_awb_factors_limit(awb_t unit,
					awb_t golden, awb_limit_t limit)
{
	uint32_t r_g;
	uint32_t b_g;
	uint32_t golden_rg, golden_bg;
	uint32_t gr_gb;
	uint32_t golden_gr_gb;
	uint32_t r_g_golden_min;
	uint32_t r_g_golden_max;
	uint32_t b_g_golden_min;
	uint32_t b_g_golden_max;

	LOG_INF("unit: r_g = 0x%x, b_g=0x%x, gr_gb=0x%x \n", unit.r_g, unit.b_g, unit.gr_gb);
	LOG_INF("golden: r_g = 0x%x, b_g=0x%x, gr_gb=0x%x \n", golden.r_g, golden.b_g, golden.gr_gb);
	LOG_INF("limit golden  0x%x, 0x%x ,0x%x 0x%x \n",limit.r_g_golden_min,limit.r_g_golden_max,
							limit.b_g_golden_min,limit.b_g_golden_max);

	r_g = unit.r_g*1000;
	b_g = unit.b_g*1000;
	gr_gb = unit.gr_gb*100;

	golden_rg = golden.r_g*1000;
	golden_bg = golden.b_g*1000;
	golden_gr_gb = golden.gr_gb*100;

	r_g_golden_min = limit.r_g_golden_min*16384;
	r_g_golden_max = limit.r_g_golden_max*16384;
	b_g_golden_min = limit.b_g_golden_min*16384;
	b_g_golden_max = limit.b_g_golden_max*16384;

	LOG_INF("r_g=%d, b_g=%d, r_g_golden_min=%d, r_g_golden_max =%d\n", r_g, b_g, r_g_golden_min, r_g_golden_max);
	LOG_INF("golden_rg=%d, golden_bg=%d, b_g_golden_min=%d, b_g_golden_max =%d\n", golden_rg, golden_bg, b_g_golden_min, b_g_golden_max);

	if (r_g < (golden_rg - r_g_golden_min) ||
		r_g > (golden_rg + r_g_golden_max))
	{
		LOG_ERR("Final RG calibration factors out of range!");
		return 1;
	}

	if (b_g < (golden_bg - b_g_golden_min) ||
		b_g > (golden_bg + b_g_golden_max))
	{
		LOG_ERR("Final BG calibration factors out of range!");
		return 1;
	}

	LOG_INF("gr_gb = %d, golden_gr_gb=%d \n", gr_gb, golden_gr_gb);

	if (gr_gb < AWB_GR_GB_MIN || gr_gb > AWB_GR_GB_MAX) {
		LOG_INF("Final gr_gb calibration factors out of range!!!");
		return 1;
	}

	if (golden_gr_gb < AWB_GR_GB_MIN || golden_gr_gb > AWB_GR_GB_MAX) {
		LOG_INF("Final golden_gr_gb calibration factors out of range!!!");
		return 1;
	}

	return 0;
}

static void get_manufacture_data(u8 *data,UINT32 StartAddr,
					UINT32 BlockSize, mot_calibration_mnf_t *pMnfCamID)
{
	int ret = 0;
	manufacture_info *pEeprom = (manufacture_info *)(data + StartAddr);

	memset(pMnfCamID, 0, sizeof(mot_calibration_mnf_t));

	if (BlockSize != sizeof(manufacture_info))
	{
		LOG_ERR("manufacture info blockSize is incorrect: BlockSize %d, expected %d",
			BlockSize, sizeof(manufacture_info));
		return;
	}

	// tableRevision
	ret = snprintf(pMnfCamID->table_revision, MAX_CALIBRATION_STRING, "0x%x", pEeprom->table_revision[0]);
	CHECK_SNPRINTF_RET(ret, pMnfCamID->table_revision, "failed to fill table revision string");

	// moto part number
	ret = snprintf(pMnfCamID->mot_part_number, MAX_CALIBRATION_STRING, "SC%c%c%c%c%c%c%c%c",
			pEeprom->mot_part_number[0], pEeprom->mot_part_number[1],
			pEeprom->mot_part_number[2], pEeprom->mot_part_number[3],
			pEeprom->mot_part_number[4], pEeprom->mot_part_number[5],
			pEeprom->mot_part_number[6], pEeprom->mot_part_number[7]);
	CHECK_SNPRINTF_RET(ret, pMnfCamID->mot_part_number, "failed to fill part number string");

	// actuator ID
	switch (pEeprom->actuator_id[0])
	{
		case 0xFF: ret = snprintf(pMnfCamID->actuator_id, MAX_CALIBRATION_STRING, "N/A");	break;
		case 0x30: ret = snprintf(pMnfCamID->actuator_id, MAX_CALIBRATION_STRING, "Dongwoo");	break;
		default:   ret = snprintf(pMnfCamID->actuator_id, MAX_CALIBRATION_STRING, "Unknown");	break;
	}
	CHECK_SNPRINTF_RET(ret, pMnfCamID->actuator_id, "failed to fill actuator id string");

	// lens ID
	switch (pEeprom->lens_id[0])
	{
		case 0x00: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "XuYe");		break;
		case 0x20: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Sunnys");		break;
		case 0x29: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Sunny 39292A-400");break;
		case 0x40: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Largan");		break;
		case 0x42: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Largan 50281A3");	break;
		case 0x60: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "SEMCO");		break;
		case 0x80: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Genius");		break;
		case 0x84: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "AAC 325174A01");	break;
		case 0xA0: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Sunnys");		break;
		case 0xA1: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Sunnys 39390A-400");break;
		case 0xC0: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "AAC");		break;
		case 0xE0: ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Kolen");		break;
		default:   ret = snprintf(pMnfCamID->lens_id, MAX_CALIBRATION_STRING, "Unknown");	break;
	}
	CHECK_SNPRINTF_RET(ret, pMnfCamID->lens_id, "failed to fill lens id string");

	// manufacturer ID
	if (pEeprom->manufacturer_id[0] == 'S' && pEeprom->manufacturer_id[1] == 'U')
		ret = snprintf(pMnfCamID->integrator, MAX_CALIBRATION_STRING, "Sunny");
	else if (pEeprom->manufacturer_id[0] == 'O' && pEeprom->manufacturer_id[1] == 'F')
		ret = snprintf(pMnfCamID->integrator, MAX_CALIBRATION_STRING, "OFilm");
	else if (pEeprom->manufacturer_id[0] == 'H' && pEeprom->manufacturer_id[1] == 'O')
		ret = snprintf(pMnfCamID->integrator, MAX_CALIBRATION_STRING, "Holitech");
	else if (pEeprom->manufacturer_id[0] == 'T' && pEeprom->manufacturer_id[1] == 'S')
		ret = snprintf(pMnfCamID->integrator, MAX_CALIBRATION_STRING, "Tianshi");
	else if (pEeprom->manufacturer_id[0] == 'S' && pEeprom->manufacturer_id[1] == 'W')
		ret = snprintf(pMnfCamID->integrator, MAX_CALIBRATION_STRING, "Sunwin");
	else if (pEeprom->manufacturer_id[0] == 'S' && pEeprom->manufacturer_id[1] == 'E')
		ret = snprintf(pMnfCamID->integrator, MAX_CALIBRATION_STRING, "Semco");
	else if (pEeprom->manufacturer_id[0] == 'Q' && pEeprom->manufacturer_id[1] == 'T')
		ret = snprintf(pMnfCamID->integrator, MAX_CALIBRATION_STRING, "Qtech");
	else
		ret = snprintf(pMnfCamID->integrator, MAX_CALIBRATION_STRING, "Unknown");

	CHECK_SNPRINTF_RET(ret, pMnfCamID->integrator, "failed to fill integrator string");

	// factory ID
	ret = snprintf(pMnfCamID->factory_id, MAX_CALIBRATION_STRING, "%c%c",
			pEeprom->factory_id[0], pEeprom->factory_id[1]);
	CHECK_SNPRINTF_RET(ret, pMnfCamID->factory_id, "failed to fill factory id string");

	// manufacture line
	ret = snprintf(pMnfCamID->manufacture_line, MAX_CALIBRATION_STRING, "%u", pEeprom->manufacture_line[0]);
	CHECK_SNPRINTF_RET(ret, pMnfCamID->manufacture_line, "failed to fill manufature line string");

	// manufacture date
	ret = snprintf(pMnfCamID->manufacture_date, MAX_CALIBRATION_STRING, "20%u/%u/%u",
			pEeprom->manufacture_date[0], pEeprom->manufacture_date[1], pEeprom->manufacture_date[2]);
	CHECK_SNPRINTF_RET(ret, pMnfCamID->manufacture_date, "failed to fill manufature date");

	// serialNumber
	ret = snprintf(pMnfCamID->serial_number, MAX_CALIBRATION_STRING, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		pEeprom->serial_number[0],  pEeprom->serial_number[1],
		pEeprom->serial_number[2],  pEeprom->serial_number[3],
		pEeprom->serial_number[4],  pEeprom->serial_number[5],
		pEeprom->serial_number[6],  pEeprom->serial_number[7],
		pEeprom->serial_number[8],  pEeprom->serial_number[9],
		pEeprom->serial_number[10], pEeprom->serial_number[11],
		pEeprom->serial_number[12], pEeprom->serial_number[13],
		pEeprom->serial_number[14], pEeprom->serial_number[15]);
	CHECK_SNPRINTF_RET(ret, pMnfCamID->serial_number, "failed to fill serial number");

	LOG_INF("integrator: %s, lens_id: %s, actuator_id: %s, manufacture_date: %s, serial_number: %s",
		pMnfCamID->integrator, pMnfCamID->lens_id, pMnfCamID->actuator_id,
		pMnfCamID->manufacture_date, pMnfCamID->serial_number);
}

static void mot_check_mnf_data(u8 *data,UINT32 StartAddr,
				UINT32 BlockSize, mot_calibration_info_t *mot_cal_info)
{
	uint8_t manufacture_crc16[2] = {*(data+StartAddr+BlockSize),
					*(data+StartAddr+BlockSize+1)};
	int i = 0;

	for (i = 0; i < 8; i++) {
		LOG_INF("G-Dg part_num[%d] = 0x%x", i, *(data+StartAddr+3+i));
	}

	if (!eeprom_util_check_crc16(data+StartAddr, BlockSize,
		convert_crc(manufacture_crc16)))
	{
		LOG_INF("Manufacturing CRC Fails!");
		mot_cal_info->mnf_status = STATUS_CRC_FAIL;
	}
	else
	{
		LOG_INF("Manufacturing CRC Pass");
		mot_cal_info->mnf_status = STATUS_OK;
	}
}

static void mot_check_af_data(u8 *data, UINT32 StartAddr,
				UINT32 BlockSize, mot_calibration_info_t *mot_cal_info)
{
	// High 16 bit: AF sync data
	UINT16 StartAddr1 = 0xFFFF & (StartAddr>>16);
	UINT16 BlockSize1 = 0xFFFF & (BlockSize>>16);
	UINT8 af_crc1[2] = {*(data+StartAddr1+BlockSize1), *(data+StartAddr1+BlockSize1+1)};

	// Low 16 bit: AF inf/macro data.
	UINT16 StartAddr2 = 0xFFFF & StartAddr;
	UINT16 BlockSize2 = 0xFFFF & BlockSize;
	UINT8 af_crc2[2] = {*(data+StartAddr2+BlockSize2), *(data+StartAddr2+BlockSize2+1)};

	mot_cal_info->af_status = STATUS_OK;

	LOG_INF("StartAddr = 0x%x, BlockSize = 0x%x", StartAddr, BlockSize);
	LOG_INF("StartAddr1 = 0x%x, BlockSize1 = 0x%x", StartAddr1, BlockSize1);
	LOG_INF("StartAddr2 = 0x%x, BlockSize2 = 0x%x", StartAddr2, BlockSize2);

	if (BlockSize1 != 0)
	{
		if (!eeprom_util_check_crc16(data+StartAddr1, BlockSize1, convert_crc(af_crc1)))
		{
			LOG_INF("AF sync data CRC Fails!");
			mot_cal_info->af_status = STATUS_CRC_FAIL;
		}
		else
		{
			LOG_INF("AF sync data CRC Pass");
		}
	}

	if (BlockSize2 != 0)
	{
		if (!eeprom_util_check_crc16(data+StartAddr2, BlockSize2, convert_crc(af_crc2)))
		{
			LOG_INF("AF inf/macro data CRC Fails!");
			mot_cal_info->af_status = STATUS_CRC_FAIL;
		}
		else
		{
			LOG_INF("AF inf/macro data CRC Pass");
		}
	}
}

static void mot_check_awb_data(u8 *data,UINT32 StartAddr,
				UINT32 BlockSize, mot_calibration_info_t *mot_cal_info)
{
	uint8_t awb_crc16[2] = {*(data+StartAddr+BlockSize),
				*(data+StartAddr+BlockSize+1)};
	camcal_awb awb_data = {0};
	camcal_awb *eeprom = &awb_data;
	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;
	MotCalibrationStatus mstatus;

	memcpy(eeprom, data+StartAddr, sizeof(camcal_awb));

	if(!eeprom_util_check_crc16(data+StartAddr,BlockSize,
					convert_crc(awb_crc16)))
	{
		LOG_ERR("AWB CRC Fails!");
		mstatus = STATUS_CRC_FAIL;
		goto endfun;
	}
	else
		LOG_INF("AWB CRC Pass");

	unit.r = to_uint16_swap(eeprom->awb_src_1_r);
	unit.gr = to_uint16_swap(eeprom->awb_src_1_gr);
	unit.gb = to_uint16_swap(eeprom->awb_src_1_gb);
	unit.b = to_uint16_swap(eeprom->awb_src_1_b);
	unit.r_g = to_uint16_swap(eeprom->awb_src_1_rg_ratio);
	unit.b_g = to_uint16_swap(eeprom->awb_src_1_bg_ratio);
	unit.gr_gb = to_uint16_swap(eeprom->awb_src_1_gr_gb_ratio);

	golden.r = to_uint16_swap(eeprom->awb_src_1_golden_r);
	golden.gr = to_uint16_swap(eeprom->awb_src_1_golden_gr);
	golden.gb = to_uint16_swap(eeprom->awb_src_1_golden_gb);
	golden.b = to_uint16_swap(eeprom->awb_src_1_golden_b);
	golden.r_g = to_uint16_swap(eeprom->awb_src_1_golden_rg_ratio);
	golden.b_g = to_uint16_swap(eeprom->awb_src_1_golden_bg_ratio);
	golden.gr_gb = to_uint16_swap(eeprom->awb_src_1_golden_gr_gb_ratio);

	if (mot_eeprom_util_check_awb_limits(unit, golden)) {
		LOG_ERR("AWB CRC limit Fails!");
		mstatus = STATUS_LIMIT_FAIL;
		goto endfun;
	}

	golden_limit.r_g_golden_min = eeprom->awb_r_g_golden_min_limit[0];
	golden_limit.r_g_golden_max = eeprom->awb_r_g_golden_max_limit[0];
	golden_limit.b_g_golden_min = eeprom->awb_b_g_golden_min_limit[0];
	golden_limit.b_g_golden_max = eeprom->awb_b_g_golden_max_limit[0];

	if (mot_eeprom_util_calculate_awb_factors_limit(unit, golden,golden_limit))
	{
		LOG_ERR("AWB CRC factor limit Fails!");
		mstatus = STATUS_LIMIT_FAIL;
		goto endfun;
	}

	LOG_INF("AWB Limit Pass");
	mstatus = STATUS_OK;
endfun:
	mot_cal_info->awb_status = mstatus;
}

static void mot_check_lsc_data( u8 *data, UINT32 StartAddr,
				UINT32 BlockSize, mot_calibration_info_t *mot_cal_info)
{
	uint8_t lsc_crc16[2] = {*(data+StartAddr+BlockSize),
				*(data+StartAddr+BlockSize+1)};

	if (!eeprom_util_check_crc16(data+StartAddr, BlockSize,
		convert_crc(lsc_crc16)))
	{
		LOG_INF("LSC CRC Fails!");
		mot_cal_info->lsc_status = STATUS_CRC_FAIL;
	}
	else
	{
		LOG_INF("LSC CRC Pass");
		mot_cal_info->lsc_status = STATUS_OK;
	}
}

static void  mot_check_pdaf_data( u8 *data, UINT32 StartAddr,
				UINT32 BlockSize, mot_calibration_info_t *mot_cal_info)
{
	// High 16 bit: pdaf output1 data;
	UINT16 StartAddr1 = 0xFFFF & (StartAddr>>16);
	UINT16 BlockSize1 = 0xFFFF & (BlockSize>>16);

	// Low 16 bit: pdaf output2 data
	UINT16 StartAddr2 = 0xFFFF & StartAddr;
	UINT16 BlockSize2 = 0xFFFF & BlockSize;

	/*** 	PDAF eeprom map:
	*	MTK PDAF calibration output1 data
	*	MTK PDAF calibration output2 data
	*	MTK PDAF calibration output1 data CRC
	*	MTK PDAF calibration output2 data CRC
	***/
	// pdaf output1 data crc
	UINT8 pdaf_crc1[2] = {*(data+StartAddr1+BlockSize1+BlockSize2), *(data+StartAddr1+BlockSize1+BlockSize2+1)};

	// pdaf output2 data crc
	UINT8 pdaf_crc2[2] = {*(data+StartAddr2+BlockSize2+2), *(data+StartAddr2+BlockSize2+2+1)};

	mot_cal_info->pdaf_status = STATUS_OK;

	LOG_INF("StartAddr = 0x%x, BlockSize = 0x%x", StartAddr, BlockSize);
	LOG_INF("StartAddr1 = 0x%x, BlockSize1 = 0x%x", StartAddr1, BlockSize1);
	LOG_INF("StartAddr2 = 0x%x, BlockSize2 = 0x%x", StartAddr2, BlockSize2);

	if (BlockSize1 != 0)
	{
		if (!eeprom_util_check_crc16(data+StartAddr1, BlockSize1, convert_crc(pdaf_crc1)))
		{
			LOG_INF("PDAF OUTPUT1 CRC Fails!");
			mot_cal_info->pdaf_status = STATUS_CRC_FAIL;
		}
		else
		{
			LOG_INF("PDAF OUTPUT1 CRC Pass");
		}
	}

	if (BlockSize2 != 0)
	{
		if (!eeprom_util_check_crc16(data+StartAddr2, BlockSize2, convert_crc(pdaf_crc2)))
		{
			LOG_INF("PDAF OUTPUT2 CRC Fails!");
			mot_cal_info->pdaf_status = STATUS_CRC_FAIL;
		}
		else
		{
			LOG_INF("PDAF OUTPUT2 CRC Pass");
		}
	}
}

int imgread_cam_cal_data(int sensorid, const char **dump_file, mot_calibration_info_t *mot_cal_info)
{
	struct stCAM_CAL_DATAINFO_STRUCT* pData;
	int i4RetValue = -1;
	unsigned int match_index = 0;
	unsigned int i = 0;

	for ( i = 0; i < MAX_EEPROM_LAYOUT_NUM; i++)
	{
		if ( sensorid == CalcheckTbl[i].camCalData.sensorID)
		{
			pData = &(CalcheckTbl[i].camCalData);
			match_index = i;
			break;
		}
	}

	if (MAX_EEPROM_LAYOUT_NUM == i)
	{
		LOG_ERR("not found eeprom table!\n");
		return -EFAULT;
	}
	if(pData == NULL){
		LOG_ERR("pData not inited!\n");
		return -EFAULT;
	}

	if (pData->dataBuffer == NULL){
		pData->dataBuffer = kmalloc(pData->dataLength, GFP_KERNEL);
		if (pData->dataBuffer == NULL) {
			LOG_ERR("pData->dataBuffer is malloc fail\n");
			return -EFAULT;
		}
	}

	i4RetValue = read_region(pData);
	if (i4RetValue != 0)
	{
		kfree(pData->dataBuffer);
		pData->dataBuffer = NULL;
		LOG_ERR("eeprom data read failed\n");
		return -EFAULT;
	}
	else
	{
		if (dump_file != NULL && dump_file[0] != NULL)
			eeprom_dump_bin(dump_file[0], pData->dataLength, pData->dataBuffer);

		for (i =0 ; i < EEPROM_CRC_LIST; i++)
		{
			if ((0 != CalcheckTbl[match_index].CalItemTbl[i].Include))
			{
				if ( i == EEPROM_DUMP_SERIAL_NUMBER &&
					dump_file != NULL &&
					dump_file[1] != NULL)
				{
					eeprom_dump_bin(dump_file[1],
							CalcheckTbl[match_index].CalItemTbl[i].BlockSize,
							pData->dataBuffer + CalcheckTbl[match_index].CalItemTbl[i].StartAddr);
					continue;
				}

				if (CalcheckTbl[match_index].CalItemTbl[i].doCalDataCheck != NULL)
				{
					CalcheckTbl[match_index].CalItemTbl[i].doCalDataCheck(pData->dataBuffer,
						CalcheckTbl[match_index].CalItemTbl[i].StartAddr,
						CalcheckTbl[match_index].CalItemTbl[i].BlockSize,
						mot_cal_info);
				}

				if ( i == EEPROM_CRC_MANUFACTURING)
				{
					get_manufacture_data(pData->dataBuffer,
						CalcheckTbl[match_index].CalItemTbl[i].StartAddr,
						CalcheckTbl[match_index].CalItemTbl[i].BlockSize,
						&mot_cal_info->mnf_cal_data);
				}
			}
		}
		kfree(pData->dataBuffer);
		pData->dataBuffer = NULL;
		LOG_INF("Finish Read: SensorID=0x%x DeviceID=0x%x read success\n",pData->sensorID, pData->deviceID);
	}

	return 0;
}
#if MY
int get_ov_cross_talk_data(uint8_t eeprom_i2c_addr, uint32_t cross_talk_data_start_addr,
				uint32_t cross_talk_data_size, ov_cross_talk_cal_t *p_cross_talk_data)
{
	int i;
	uint8_t cross_talk_data[OV_CROSS_TALK_GROUP_SIZE] = {0};

	memset(p_cross_talk_data, 0, sizeof(ov_cross_talk_cal_t));

	for (i = 0; i < cross_talk_data_size; i++)
	{
		cross_talk_data[i]			   = read_cmos_sensor(eeprom_i2c_addr, cross_talk_data_start_addr + i);
		p_cross_talk_data->cross_talk_data[i].addr = 0x5A40 + i;
		p_cross_talk_data->cross_talk_data[i].data = cross_talk_data[i];
	}

	if (i == OV_CROSS_TALK_GROUP_SIZE)
	{
		p_cross_talk_data->cross_talk_crc[0] = read_cmos_sensor(eeprom_i2c_addr, cross_talk_data_start_addr + i);
		p_cross_talk_data->cross_talk_crc[1] = read_cmos_sensor(eeprom_i2c_addr, cross_talk_data_start_addr + i + 1);
	}

	p_cross_talk_data->height[0].addr 		= 0x5A1A; //H 6528
	p_cross_talk_data->height[0].data 		= 0x19;
	p_cross_talk_data->height[1].addr 		= 0x5A1B;
	p_cross_talk_data->height[1].data 		= 0x80;

	p_cross_talk_data->width[0].addr  		= 0x5A1C; //W 4896
	p_cross_talk_data->width[0].data  		= 0x13;
	p_cross_talk_data->width[1].addr  		= 0x5A1D;
	p_cross_talk_data->width[1].data  		= 0x20;

	p_cross_talk_data->cross_talk_enable[0].addr  	= 0x5000; //bit[3] = 1
	p_cross_talk_data->cross_talk_enable[0].data  	= 0x5d;
	p_cross_talk_data->cross_talk_enable[1].addr  	= 0x5a12; //bt[0]  = 1
	p_cross_talk_data->cross_talk_enable[1].data  	= 0x0f;

	if (!eeprom_util_check_crc16(cross_talk_data, cross_talk_data_size,
		convert_crc(p_cross_talk_data->cross_talk_crc)))
	{
		LOG_INF("Cross Talk Data CRC Fail!");
		p_cross_talk_data->is_valid = 0;
	}
	else
	{
		LOG_INF("Cross Talk Data CRC Pass");
		p_cross_talk_data->is_valid = 1;
	}

#if 0 //for debug
	LOG_INF("0x%04x: 0x%02x", p_cross_talk_data->height[0].addr, 			p_cross_talk_data->height[0].data);
	LOG_INF("0x%04x: 0x%02x", p_cross_talk_data->height[1].addr, 			p_cross_talk_data->height[1].data);
	LOG_INF("0x%04x: 0x%02x", p_cross_talk_data->width[0].addr,  			p_cross_talk_data->width[0].data);
	LOG_INF("0x%04x: 0x%02x", p_cross_talk_data->width[1].addr,  			p_cross_talk_data->width[1].data);
	LOG_INF("0x%04x: 0x%02x", p_cross_talk_data->cross_talk_enable[0].addr, 	p_cross_talk_data->cross_talk_enable[0].data);
	LOG_INF("0x%04x: 0x%02x", p_cross_talk_data->cross_talk_enable[1].addr, 	p_cross_talk_data->cross_talk_enable[1].data);

	LOG_INF("******* cross talk data *******");
	for (i = 0; i < cross_talk_data_size; i++)
		LOG_INF("0x%04x: 0x%02x", p_cross_talk_data->cross_talk_data[i].addr, p_cross_talk_data->cross_talk_data[i].data);
	LOG_INF("******* cross talk data *******");

	LOG_INF("CRC: 0x%04x", convert_crc(p_cross_talk_data->cross_talk_crc));
#endif

	return 1;
}
#endif

