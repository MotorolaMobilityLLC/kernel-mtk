/*
 * Copyright (C) 2019 MediaTek Inc.
 * Copyright (C) 2021 lucas <guoqiang8@lenovo.com>
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


MOT_EEPROM_CAL CalcheckTbl[MAX_EEPROM_LAYOUT_NUM] =
{
	{
		{
			.sensorID= MOT_S5KHM2_SENSOR_ID,
			.deviceID = 0x01,
			.dataLength = 0x1638,
			.sensorVendorid = 0x16020000,
			.vendorByte = {1,2,3,4},
			.dataBuffer = NULL
		},
		{
			{0x00000000, 0x00000000, 0x00000000, mot_check_mnf_data },
			{0x00000000, 0x00000005, 0x00000002, mot_check_af_data  },
			{0x00000000, 0x00000017, 0x0000074C, mot_check_awb_data },
			{0x00000000, 0x00000007, 0x0000000E, mot_check_lsc_data },
			{0x00000000, 0x00000763, 0x00000800, mot_check_pdaf_data}
		}
	},
	{
		{
			.sensorID= MOT_OV32B40_SENSOR_ID,
			.deviceID = 0x02,
			.dataLength = 0x0770,
			.sensorVendorid = 0x16020000,
			.vendorByte = {1,2,3,4},
			.dataBuffer = NULL
		},
		{
			{0x00000000, 0x00000000, 0x00000000, mot_check_mnf_data },
			{0x00000000, 0x00000005, 0x00000002, mot_check_af_data  },
			{0x00000000, 0x00000017, 0x0000074C, mot_check_awb_data },
			{0x00000000, 0x00000007, 0x0000000E, mot_check_lsc_data },
			{0x00000000, 0x00000763, 0x00000800, mot_check_pdaf_data}
        }
    },
	{
		{
			.sensorID= MOT_OV02B1B_SENSOR_ID,
			.deviceID = 0x04,
			.dataLength = 0x0770,
			.sensorVendorid = 0x16020000,
			.vendorByte = {1,2,3,4},
			.dataBuffer = NULL
		},
		{
			{0x00000000, 0x00000000, 0x00000000, mot_check_mnf_data },
			{0x00000000, 0x00000005, 0x00000002, mot_check_af_data  },
			{0x00000000, 0x00000017, 0x0000074C, mot_check_awb_data },
			{0x00000000, 0x00000007, 0x0000000E, mot_check_lsc_data },
			{0x00000000, 0x00000763, 0x00000800, mot_check_pdaf_data}
		}
	},
	{
		{
			.sensorID= MOT_S5K4H7_SENSOR_ID,
			.deviceID = 0x10,
			.dataLength = 0x0770,
			.sensorVendorid = 0x16020000,
			.vendorByte = {1,2,3,4},
			.dataBuffer = NULL
		},
		{
			{0x00000000, 0x00000000, 0x00000000, mot_check_mnf_data },
			{0x00000000, 0x00000005, 0x00000002, mot_check_af_data  },
			{0x00000000, 0x00000017, 0x0000074C, mot_check_awb_data },
			{0x00000000, 0x00000007, 0x0000000E, mot_check_lsc_data },
			{0x00000000, 0x00000763, 0x00000800, mot_check_pdaf_data}
		}
     }
};

static struct stCAM_CAL_LIST_STRUCT *get_list(struct CAM_CAL_SENSOR_INFO *sinfo)
{
	struct stCAM_CAL_LIST_STRUCT *plist;

	cam_cal_get_sensor_list(&plist);

	while (plist &&
	       (plist->sensorID != 0) &&
	       (plist->sensorID != sinfo->sensor_id))
		plist++;

	return plist;
}

static unsigned int read_region(struct EEPROM_DRV_FD_DATA *pdata,
				unsigned char *buf,
				unsigned int offset, unsigned int size)
{
	unsigned int ret;
	unsigned short dts_addr;
	struct stCAM_CAL_LIST_STRUCT *plist = get_list(&pdata->sensor_info);
	unsigned int size_limit = (plist && plist->maxEepromSize > 0)
		? plist->maxEepromSize : DEFAULT_MAX_EEPROM_SIZE_8K;

	if (offset + size > size_limit)
	{
		pr_debug("Error! not support address >= 0x%x!!\n", size_limit);
		return 0;
	}

	if (plist && plist->readCamCalData)
	{
		pr_debug("i2c addr 0x%x\n", plist->slaveID);
		mutex_lock(&pdata->pdrv->eeprom_mutex);
		dts_addr = pdata->pdrv->pi2c_client->addr;
		pdata->pdrv->pi2c_client->addr = (plist->slaveID >> 1);
		ret = plist->readCamCalData(pdata->pdrv->pi2c_client,
					    offset, buf, size);
		pdata->pdrv->pi2c_client->addr = dts_addr;
		mutex_unlock(&pdata->pdrv->eeprom_mutex);
	}
	else
	{
		pr_debug("no customized\n");
		mutex_lock(&pdata->pdrv->eeprom_mutex);
		ret = Common_read_region(pdata->pdrv->pi2c_client,
					 offset, buf, size);
		mutex_unlock(&pdata->pdrv->eeprom_mutex);
	}

	return ret;
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
		LOG_INF("unit r out of range! MIN: %d, r: %d, MAX: %d",
			AWB_R_MIN, unit.r, AWB_R_MAX);
		result = 1;
	}
	if (unit.gr < AWB_GR_MIN || unit.gr > AWB_GR_MAX)
	{
		LOG_INF("unit gr out of range! MIN: %d, gr: %d, MAX: %d",
			AWB_GR_MIN, unit.gr, AWB_GR_MAX);
		result = 1;
	}
	if (unit.gb < AWB_GB_MIN || unit.gb > AWB_GB_MAX)
	{
		LOG_INF("unit gb out of range! MIN: %d, gb: %d, MAX: %d",
			AWB_GB_MIN, unit.gb, AWB_GB_MAX);
		result = 1;
	}
	if (unit.b < AWB_B_MIN || unit.b > AWB_B_MAX)
	{
		LOG_INF("unit b out of range! MIN: %d, b: %d, MAX: %d",
			AWB_B_MIN, unit.b, AWB_B_MAX);
		result = 1;
	}

	if (golden.r < AWB_R_MIN || golden.r > AWB_R_MAX)
	{
		LOG_INF("golden r out of range! MIN: %d, r: %d, MAX: %d",
			AWB_R_MIN, golden.r, AWB_R_MAX);
		result = 1;
	}
	if (golden.gr < AWB_GR_MIN || golden.gr > AWB_GR_MAX)
	{
		LOG_INF("golden gr out of range! MIN: %d, gr: %d, MAX: %d",
			AWB_GR_MIN, golden.gr, AWB_GR_MAX);
		result = 1;
	}
	if (golden.gb < AWB_GB_MIN || golden.gb > AWB_GB_MAX)
	{
		LOG_INF("golden gb out of range! MIN: %d, gb: %d, MAX: %d",
			AWB_GB_MIN, golden.gb, AWB_GB_MAX);
		result = 1;
	}
	if (golden.b < AWB_B_MIN || golden.b > AWB_B_MAX)
	{
		LOG_INF("golden b out of range! MIN: %d, b: %d, MAX: %d",
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
	uint32_t r_g_golden_min;
	uint32_t r_g_golden_max;
	uint32_t b_g_golden_min;
	uint32_t b_g_golden_max;

	r_g = unit.r_g * 1000;
	b_g = unit.b_g*1000;

	golden_rg = golden.r_g* 1000;
	golden_bg = golden.b_g* 1000;

	r_g_golden_min = limit.r_g_golden_min*16384;
	r_g_golden_max = limit.r_g_golden_max*16384;
	b_g_golden_min = limit.b_g_golden_min*16384;
	b_g_golden_max = limit.b_g_golden_max*16384;
	LOG_INF("rg = %d, bg=%d,rgmin=%d,bgmax =%d\n",r_g,b_g,
				r_g_golden_min,r_g_golden_max);
	LOG_INF("grg = %d, gbg=%d,bgmin=%d,bgmax =%d\n",golden_rg,
				golden_bg,b_g_golden_min,b_g_golden_max);
	if (r_g < (golden_rg - r_g_golden_min) ||
		r_g > (golden_rg + r_g_golden_max))
	{
		LOG_INF("Final RG calibration factors out of range!");
		return 1;
	}

	if (b_g < (golden_bg - b_g_golden_min) ||
		b_g > (golden_bg + b_g_golden_max))
	{
		LOG_INF("Final BG calibration factors out of range!");
		return 1;
	}
	return 0;
}

static void mot_check_mnf_data(u8 *data,UINT32 StartAddr,
				UINT32 BlockSize,RetStatus *rStatus)
{
	uint8_t manufacture_crc16[2] = {*(data+StartAddr+BlockSize),
					*(data+StartAddr+BlockSize+1)};

	if (!eeprom_util_check_crc16(data+StartAddr, BlockSize,
		convert_crc(manufacture_crc16)))
	{
		LOG_INF("Manufacturing CRC Fails!");
		rStatus->mnf_status = CRC_FAILURE;
	}
	else
	{
		LOG_INF("Manufacturing CRC Pass");
		rStatus->mnf_status = NO_ERRORS;
	}
}

static void mot_check_af_data(u8 *data, UINT32 StartAddr,
				UINT32 BlockSize,RetStatus *rStatus)
{
	uint8_t af_cal_crc16[2] = {*(data+StartAddr+BlockSize),
					*(data+StartAddr+BlockSize+1)};

	if (!eeprom_util_check_crc16(data+StartAddr, BlockSize,
					convert_crc(af_cal_crc16)))
	{
		LOG_INF("Autofocus CRC Fails!");
		rStatus->af_status = CRC_FAILURE;
	}
	else
	{
		LOG_INF("Autofocus CRC Pass");
		rStatus->af_status = NO_ERRORS;
	}
}

static void mot_check_awb_data(u8 *data,UINT32 StartAddr,
				UINT32 BlockSize,RetStatus *rStatus)
{
	uint8_t awb_crc16[2] = {*(data+StartAddr+BlockSize),
				*(data+StartAddr+BlockSize+1)};
	char awb_dat[39] = {0};
	camcal_awb *eeprom = NULL;
	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;
	calibration_status_t mstatus;

	memcpy(awb_dat, data+StartAddr, sizeof(camcal_awb));

	if(!eeprom_util_check_crc16(data+StartAddr,BlockSize,
					convert_crc(awb_crc16)))
	{
		LOG_INF("AWB CRC Fails!");
		mstatus = CRC_FAILURE;
		goto endfun;
	}
	eeprom = (camcal_awb *)awb_dat;
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
		LOG_INF("AWB CRC limit Fails!");
		mstatus = LIMIT_FAILURE;
		goto endfun;
	}

	golden_limit.r_g_golden_min = eeprom->awb_r_g_golden_min_limit[0];
	golden_limit.r_g_golden_max = eeprom->awb_r_g_golden_max_limit[0];
	golden_limit.b_g_golden_min = eeprom->awb_b_g_golden_min_limit[0];
	golden_limit.b_g_golden_max = eeprom->awb_b_g_golden_max_limit[0];

	if (mot_eeprom_util_calculate_awb_factors_limit(unit, golden,golden_limit))
	{
		LOG_INF("AWB CRC factor limit Fails!");
		mstatus = LIMIT_FAILURE;
		goto endfun;
	}
	LOG_INF("AWB CRC Pass");
	mstatus = NO_ERRORS;
endfun:
	rStatus->awb_status = mstatus;
}

static void mot_check_lsc_data( u8 *data, UINT32 StartAddr,
				UINT32 BlockSize, RetStatus *rStatus)
{
	uint8_t lsc_crc16[2] = {*(data+StartAddr+BlockSize),
				*(data+StartAddr+BlockSize+1)};

	if (!eeprom_util_check_crc16(data+StartAddr, BlockSize,
		convert_crc(lsc_crc16)))
	{
		LOG_INF("LSC CRC Fails!");
		rStatus->lsc_status = CRC_FAILURE;
	}
	else
	{
		LOG_INF("LSC CRC Pass");
		rStatus->lsc_status = NO_ERRORS;
	}
}

static void  mot_check_pdaf_data( u8 *data, UINT32 StartAddr,
				UINT32 BlockSize,RetStatus *rStatus)
{
	uint8_t pdaf_output1_crc16[2] = {*(data+StartAddr+BlockSize),
					*(data+StartAddr+BlockSize+1)};


	if (!eeprom_util_check_crc16(data+StartAddr, BlockSize,
		convert_crc(pdaf_output1_crc16)))
	{
		LOG_INF("PDAF OUTPUT1 CRC Fails!");
		rStatus->pdaf_status = CRC_FAILURE;
	}
	else
	{
		LOG_INF("PDAF CRC Pass");
		rStatus->pdaf_status = NO_ERRORS;
	}
#if 0
	u8 pdaf_crc16[2] = {0,0}; //need fix XXXXXXXX
	if (!eeprom_util_check_crc16(data+StartAddr, BlockSize,
		convert_crc(pdaf_crc16))) {
		LOG_INF("PDAF OUTPUT2 CRC Fails!");
		return CRC_FAILURE;
	}
#endif
}

int imgread_cam_cal_data(int sensorid, const char* dump_file ,
						RetStatus *rStatus)
{
	struct EEPROM_DRV_FD_DATA *fd_pdata = NULL;
	struct stCAM_CAL_DATAINFO_STRUCT* pData;
	int i4RetValue = -1;
	//u32 vendorID = 0;
	//u8 tmpBuf[4] = {0};
	struct file *fdata = NULL;
	unsigned int index = 0;
	char device_drv_name[DEV_NAME_STR_LEN_MAX] = { 0 };
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

	LOG_INF("SensorID=%x DeviceID=%x\n",pData->sensorID, pData->deviceID);
	index = IMGSENSOR_SENSOR_IDX_MAP(pData->deviceID);
	if (MAX_EEPROM_NUMBER <= index ) {
		LOG_ERR("node index out of bound\n");
		return -EINVAL;
	}

	i4RetValue = snprintf(device_drv_name, DEV_NAME_STR_LEN_MAX - 1,
		DEV_NAME_FMT_DEV, index);
	LOG_INF("device_drv_name=%s",device_drv_name);
	if (i4RetValue < 0) {
		LOG_ERR("[eeprom]%s error, ret = %d", __func__, i4RetValue);
		return -EFAULT;
	}

	fdata = filp_open(device_drv_name, O_RDWR, 0);

	if (IS_ERR(fdata)){
		LOG_ERR("fail to open %s\n", device_drv_name);
		return -EFAULT;
	}

	fd_pdata = (struct EEPROM_DRV_FD_DATA *) fdata->private_data;
	if(NULL == fd_pdata){
		LOG_ERR("fp_pdata is null %s\n");
		filp_close(fdata,NULL);
		return -EFAULT;
	}
	fd_pdata->sensor_info.sensor_id = pData->sensorID;

#if 0
	i4RetValue = read_region(fd_pdata, &tmpBuf[0], 1, 1);
	if (0 == i4RetValue) {
		LOG_ERR("vendorID read failed 0x%x != 0x%x,i4RetValue=%d\n",tmpBuf[0],
						pData->sensorVendorid >> 24,i4RetValue);
		filp_close(fdata,NULL);
		return -EFAULT;
	}
	vendorID = tmpBuf[0];
	if(vendorID != pData->sensorVendorid >> 24){
		LOG_ERR("vendorID cmp failed 0x%x != 0x%x\n",vendorID,
							pData->sensorVendorid >> 24);
		filp_close(fdata,NULL);
		return -EFAULT;
	}
#endif
	if (pData->dataBuffer == NULL){
		pData->dataBuffer = kmalloc(pData->dataLength, GFP_KERNEL);
		if (pData->dataBuffer == NULL) {
			LOG_ERR("pData->dataBuffer is malloc fail\n");
			filp_close(fdata,NULL);
			return -EFAULT;
		}
	}
	i4RetValue = read_region(fd_pdata, pData->dataBuffer, 0x0, pData->dataLength);
	if (i4RetValue != pData->dataLength)
	{
		kfree(pData->dataBuffer);
		pData->dataBuffer = NULL;
		LOG_ERR("eeprom data read failed\n");
		filp_close(fdata,NULL);
		return -EFAULT;
	}
	else
	{
		eeprom_dump_bin(dump_file, pData->dataLength,pData->dataBuffer);

		for (i =0 ; i < EEPROM_CRC_LIST; i++)
		{
			if ((0 != CalcheckTbl[match_index].CalItemTbl[i].Include)
				&& (CalcheckTbl[match_index].CalItemTbl[i].doCalDataCheck != NULL))
			{
				CalcheckTbl[match_index].CalItemTbl[i].doCalDataCheck(pData->dataBuffer,
					CalcheckTbl[match_index].CalItemTbl[i].StartAddr,
					CalcheckTbl[match_index].CalItemTbl[i].BlockSize,rStatus);
			}
		}
		kfree(pData->dataBuffer);
		pData->dataBuffer = NULL;
		LOG_INF("SensorID=%x DeviceID=%x read success\n",pData->sensorID, pData->deviceID);
	}

	filp_close(fdata,NULL);
	return 0;
}

