////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Motorola Mobility, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Motorola Mobility, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
#include "mot_ov02b1bmipiraw_otp.h"

#define PFX "[MOTO_EEPROM OV02B1B]"

#define LOG_INF(format, args...) pr_info(PFX "[%s %d] " format, __func__, __LINE__, ##args)
#define LOG_DBG(format, args...) pr_debug(PFX "[%s %d] " format, __func__, __LINE__, ##args)
#define LOG_ERR(format, args...) pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)

#define I2C_ADDR 0x7a

#define OTP_SIZE 32
uint8_t raw_data[OTP_SIZE] = {0};

#define CHECK_SNPRINTF_RET(ret, str, msg)           \
    if (ret < 0 || ret >= MAX_CALIBRATION_STRING) { \
        LOG_ERR(msg);                         \
        str[0] = 0;                                 \
    }

static int gPageSelDone = 0;
static int gCurPage = PAGE_0;

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[1] = {(char)(addr & 0xFF)};
	iReadRegI2C(pusendcmd, 1, (u8 *)&get_byte, 1, I2C_ADDR);
	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint16 para)
{
	char pusendcmd[2] = { (char)(addr & 0xFF), (char)(para & 0xFF) };
	iWriteRegI2C(pusendcmd, 2, I2C_ADDR);
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

int ov02b1b_select_page(mnf_info_group *pMnfGroup)
{
	int pgmFlag = 0;

	if (!gPageSelDone)
	{
		for (gCurPage = PAGE_0; gCurPage < PAGE_MAX; gCurPage++)
		{
			pgmFlag = ((pMnfGroup[gCurPage].valid_flag[0] == PGM_FLAG_VALID1) ||
					(pMnfGroup[gCurPage].valid_flag[0] == PGM_FLAG_VALID2));
			if (pgmFlag)
				break;
		}
		gPageSelDone = 1;
	}
	return gCurPage;
}

void mot_ov02b1b_format_mnf_data(UINT8* rawdata, mot_calibration_info_t *pCalInfo)
{
	int ret = 0;
	mot_ov02b1b_otp * pOTP = (mot_ov02b1b_otp *)rawdata;
	mot_calibration_mnf_t *pMnfData = &pCalInfo->mnf_cal_data;

	eeprom_dump_bin(EEPROM_DATA_PATH, OTP_SIZE, rawdata);
	eeprom_dump_bin(SERIAL_DATA_PATH, DUMP_DEPTH_SERIAL_NUMBER_SIZE, pOTP->serial_number);

	if (ov02b1b_select_page(pOTP->mnf_info) >= PAGE_MAX) {
		LOG_ERR("All otp pages are invalid!");
		pCalInfo->mnf_status = STATUS_CRC_FAIL;
		return;
	}

	// tableRevision
	ret = snprintf(pMnfData->table_revision, MAX_CALIBRATION_STRING, "0x%x",
			pOTP->mnf_info[gCurPage].table_revision[0]);
	CHECK_SNPRINTF_RET(ret, pMnfData->table_revision, "failed to fill table revision string");

	// manufacturer_id
	if (pOTP->mnf_info[gCurPage].manufacturer_id[0] == 'S' &&
		pOTP->mnf_info[gCurPage].manufacturer_id[1] == 'U')
			ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Sunny");
	else if (pOTP->mnf_info[gCurPage].manufacturer_id[0] == 'O' &&
		pOTP->mnf_info[gCurPage].manufacturer_id[1] == 'F')
			ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "OFilm");
	else if (pOTP->mnf_info[gCurPage].manufacturer_id[0] == 'H' &&
		pOTP->mnf_info[gCurPage].manufacturer_id[1] == 'O')
			ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Holitech");
	else if (pOTP->mnf_info[gCurPage].manufacturer_id[0] == 'T' &&
		pOTP->mnf_info[gCurPage].manufacturer_id[1] == 'S')
			ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Tianshi");
	else if (pOTP->mnf_info[gCurPage].manufacturer_id[0] == 'S' &&
		pOTP->mnf_info[gCurPage].manufacturer_id[1] == 'W')
			ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Sunwin");
	else
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Unknown");

	CHECK_SNPRINTF_RET(ret, pMnfData->integrator, "failed to fill integrator string");

	// manufacture_line
	ret = snprintf(pMnfData->manufacture_line, MAX_CALIBRATION_STRING, "%u",
			pOTP->mnf_info[gCurPage].manufacture_line[0]);
	CHECK_SNPRINTF_RET(ret, pMnfData->manufacture_line, "failed to fill manufature line string");

	// manufacture_date
	ret = snprintf(pMnfData->manufacture_date, MAX_CALIBRATION_STRING, "20%u/%u/%u",
			pOTP->mnf_info[gCurPage].manufacture_date[0],
			pOTP->mnf_info[gCurPage].manufacture_date[1],
			pOTP->mnf_info[gCurPage].manufacture_date[2]);
	CHECK_SNPRINTF_RET(ret, pMnfData->manufacture_date, "failed to fill manufature date");

	// serialNumber
	ret = snprintf(pMnfData->serial_number, MAX_CALIBRATION_STRING, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			pOTP->serial_number[0],  pOTP->serial_number[1],
			pOTP->serial_number[2],  pOTP->serial_number[3],
			pOTP->serial_number[4],  pOTP->serial_number[5],
			pOTP->serial_number[6],  pOTP->serial_number[7],
			pOTP->serial_number[8],  pOTP->serial_number[9],
			pOTP->serial_number[10], pOTP->serial_number[11],
			pOTP->serial_number[12], pOTP->serial_number[13],
			pOTP->serial_number[14], pOTP->serial_number[15]);
	CHECK_SNPRINTF_RET(ret, pMnfData->serial_number, "failed to fill serial number");

	LOG_INF("integrator: %s, manufacture_date: %s, serial_number: %s",
		pMnfData->integrator, pMnfData->manufacture_date, pMnfData->serial_number);
}

int ov02b1b_format_otp(mot_calibration_info_t * pOtpCalInfo)
{
	int i;

	LOG_INF("Start to format ov02b1b otp");

	write_cmos_sensor(0xfd, 0x06);
	write_cmos_sensor(0x21, 0x00);
	write_cmos_sensor(0x2f, 0x01);

	for( i = 0; i < OTP_SIZE; i++)
	{
		raw_data[i] = read_cmos_sensor(i);
		LOG_INF("buff[%d]=0x%02x\n", i, raw_data[i]);
	}

	mot_ov02b1b_format_mnf_data(raw_data, pOtpCalInfo);

	LOG_INF("finish format ov02b1b otp");

	return 0;
}
