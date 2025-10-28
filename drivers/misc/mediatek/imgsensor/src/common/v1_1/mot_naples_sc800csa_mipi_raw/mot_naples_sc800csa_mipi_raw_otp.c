/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
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
#include "cam_cal.h"
#include "cam_cal_define.h"
//#include "cam_cal_list.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include "mot_naples_sc800csa_mipi_raw_Sensor.h"
#include "mot_naples_sc800csa_mipi_raw_otp.h"
#define MOT_NAPLES_SC800CSA_I2C_ID     0x6c
#define LOG_INF(format, args...)		pr_err(PFX "[%s] " format, __func__, ##args)

static  struct imgsensor_struct *imgsensor;

static mot_calibration_status_t calibration_status = {NO_ERRORS};
static mot_calibration_mnf_t mnf_info = {0};

struct mot_naples_sc800csa_otp_t mot_naples_sc800csa_otp_info = {0};


bool checksummodule = false;
bool checksumawb = false;
bool checksumlsc = false;
bool checksumall = false;

static u16 read_cmos_sensor(u16 addr)
{
	u16 get_byte=0;
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

	//kdSetI2CSpeed(400);

	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, MOT_NAPLES_SC800CSA_I2C_ID);

	return get_byte;
}

static void write_cmos_sensor8(u16 addr, u16 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, MOT_NAPLES_SC800CSA_I2C_ID);
}


static u8 CompareWriteAndRead(u16 uRegNum, BYTE * pWriteData, BYTE * pReadData, u16 size)
{
    int i;
    for ( i = 0; i < size; i++)
    {
        if (pWriteData[i] != pReadData[i])
        {
            pr_debug(PFX,"pWriteData[i]:0x%x  pReadData[i]:0x%x ", uRegNum + i, pWriteData[i], pReadData[i]);
            return 0;
        }
    }
    return 1;
}

static u16 mot_naples_sc800csa_otp_read_group(u16 page, u16 addr, u8 *data, u16 length)
{
    int i;
    int times;
    size_t loop_time;
    BOOL re = TRUE;
    BYTE def = 0x00, busy_flag = 0x01;

    BYTE pRegData[2][390] = { 0 };
    BYTE threshold[3][3] = { {0x48,0x38,0x41},{0x48,0x18,0x41},{0x48,0x58,0x41} };

    for ( times = 0; times < 2; times++)
    {
        write_cmos_sensor8(0x36b0, threshold[times][0]);
        write_cmos_sensor8(0x36b1, threshold[times][1]);
        write_cmos_sensor8(0x36b2, threshold[times][2]);
        write_cmos_sensor8(0x4408, 0x80 + (page - 1) * 0x02);
        write_cmos_sensor8(0x4409, 0x00);
        write_cmos_sensor8(0x440a, 0x81 + (page - 1) * 0x02);
        write_cmos_sensor8(0x440b, 0xff);
        write_cmos_sensor8(0x4401, 0x13);
        write_cmos_sensor8(0x4412, 0x03 + (page - 2) * 0x02);
        write_cmos_sensor8(0x4407, 0x00);

        write_cmos_sensor8(0x4400, 0x11);
        mdelay(10);

        for ( loop_time = 0; loop_time < 1000; loop_time++)
        {
            mdelay(5);
            def = read_cmos_sensor(0x4420);//[0]busy,0 ok//[1]otp,0 ok
            busy_flag = def & 0x1;
            if (0 == busy_flag) break;
        }


        if (busy_flag)
        {
            re = FALSE;
            goto READ_CLOCK_END;
        }

        for ( i = 0; i < length; i++)
        {
            pRegData[times][i] = read_cmos_sensor(addr+i);
            //pr_debug(PFX,"addr = 0x%x, data = 0x%x\n", addr+i, pRegData[times][i]);
        }
    }

    if (CompareWriteAndRead(addr, pRegData[0], pRegData[1], length))
    {
        //size = 390;
        memcpy(data, pRegData[0], length);
    }
    else
    {
        re = -2;
    }

READ_CLOCK_END:

    if (2 == page)
    {
        write_cmos_sensor8(0x0100, 0x00);
        write_cmos_sensor8(0x4424, 0x01);
        write_cmos_sensor8(0x4408, 0x00);
        write_cmos_sensor8(0x4409, 0x00);
        write_cmos_sensor8(0x440a, 0x01);
        write_cmos_sensor8(0x440b, 0xff);
        write_cmos_sensor8(0x4401, 0x13);
        write_cmos_sensor8(0x4412, 0x01);
        write_cmos_sensor8(0x4407, 0x0e);
        //WriteIIC(0x3106, 0x01);
        write_cmos_sensor8(0x363c, 0x8c);
        write_cmos_sensor8(0x36b0, 0x48);
        write_cmos_sensor8(0x36b1, 0x38);
        write_cmos_sensor8(0x36b2, 0x41);
        write_cmos_sensor8(0x0100, 0x01);
        mdelay(100);
    }

    return re;
}

static void NAPLES_SC800CSA_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;
	uint8_t* module_param = data;
    	// lens_id
	struct NAPLES_SC800CSA_eeprom_t eeprom = {
        	.lens_id = module_param[12],
    	};

	LOG_INF("eeprom.lens_id:0x%x", eeprom.lens_id);
	if (eeprom.lens_id == 0x01){
		ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "HX-M0846A");
	} else {
		ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "Unknown");
		LOG_INF("unknown lens_id");
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		pr_err("snprintf of mnf->serial_number failed");
		mnf->serial_number[0] = 0;
	}
}


mot_calibration_status_t *NAPLES_SC800CSA_eeprom_get_calibration_status(void)
{
	return &calibration_status;
}

mot_calibration_mnf_t *NAPLES_SC800CSA_eeprom_get_mnf_info(void)
{
	return &mnf_info;
}

void NAPLES_SC800CSA_eeprom_format_calibration_data(struct imgsensor_struct *pImgsensor)
{
	imgsensor = pImgsensor;
	NAPLES_SC800CSA_eeprom_get_mnf_data((void *)mot_naples_sc800csa_otp_info.module_param, &mnf_info);
}

static int mot_naples_sc800csa_iReadData(u16 page, unsigned int ui4_offset, unsigned int ui4_length, unsigned char *pinputdata)
{
    int i4RetValue = 0;
    int i4ResidueDataLength;
    u32 u4CurrentOffset;
    u8 *pBuff;

    pr_debug(PFX,"ui4_offset = 0x%x, ui4_length = %d \n", ui4_offset, ui4_length);

    i4ResidueDataLength = (int)ui4_length;
    u4CurrentOffset = ui4_offset;
    pBuff = pinputdata;

    i4RetValue = mot_naples_sc800csa_otp_read_group(page, (u16) u4CurrentOffset, pBuff, i4ResidueDataLength);
    if (i4RetValue != 1) {
        pr_debug(PFX,"I2C iReadData failed!!\n");
        return -1;
    }

    return 0;
}

static bool mot_naples_sc800csa_param_checksum(u8 *buf, unsigned int size, u8 checksum)
{
    int i, sum = 0;

    for (i = 0; i < size; i++)
    {
        sum += buf[i];
        //pr_debug(PFX,"buf[%d] = 0x%x %d", i, buf[i], buf[i]);
    }

    if ((sum % 255 + 1) != checksum)
    {
        LOG_INF("checksum fail size = %d sum=%d sum-in-eeprom=%d", size, sum % 255 + 1, checksum);
        return false;
    }
    LOG_INF("checksum success size = %d sum=%d sum-in-eeprom=%d", size, sum % 255 + 1, checksum);
    return true;
}

static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}
static uint32_t convert_crc(uint8_t *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
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

	LOG_INF("REF_CRC 0x%x CALC CRC 0x%x CALC Reverse CRC 0x%x matches? %d\n",
		ref_crc, crc, crc_reverse, crc_match);

	return crc_match;
}

static bool mot_naples_sc800csa_all_data_checksum(kal_uint8 *data, unsigned int size, kal_uint8 chksum)
{
	int32_t check_sum_cal = 0;
	int32_t i = 0;

	for (i =0; i < size; i++) {
		check_sum_cal += data[i];
	}
	check_sum_cal = (check_sum_cal % 255 ) + 1;

	if( chksum == check_sum_cal){
		return true;
	} else {
		LOG_INF("chksum fail size = %d check_sum_cal=%d sum-in-eeprom=%d", size, check_sum_cal, chksum);
		return false;
	}
}

static bool mot_naples_sc800csa_read_module_info(u8 moduleflag)
{
    bool ret = false;

    pr_debug(PFX,"--------------mot_naples_sc800csa module info read begin------------\n");
    if (moduleflag == 1) {
        mot_naples_sc800csa_iReadData(MODULE_GROUP_INFO_PAGE, MODULE_GROUP1_INFO_ADDR, MODULE_INFO_LENGTH, &mot_naples_sc800csa_otp_info.module_param[0]);
        mot_naples_sc800csa_iReadData(MODULE_GROUP_INFO_PAGE, MODULE_GROUP1_CHECKSUM, 2, &mot_naples_sc800csa_otp_info.moduleChksum[0]);
    } else if (moduleflag  == 2) {
        mot_naples_sc800csa_iReadData(MODULE_GROUP2_INFO_PAGE, MODULE_GROUP2_INFO_ADDR, MODULE_INFO_LENGTH, &mot_naples_sc800csa_otp_info.module_param[0]);
        mot_naples_sc800csa_iReadData(MODULE_GROUP2_INFO_PAGE, MODULE_GROUP2_CHECKSUM, 2, &mot_naples_sc800csa_otp_info.moduleChksum[0]);
    } else {
        pr_debug(PFX,"--------------mot_naples_sc800csa module info read failed------------\n");
    }
    pr_debug(PFX,"--------------mot_naples_sc800csa module info read end------------\n");
    ret = eeprom_util_check_crc16(&mot_naples_sc800csa_otp_info.module_param[0], MODULE_INFO_LENGTH, convert_crc(&mot_naples_sc800csa_otp_info.moduleChksum[0]));
    if (ret) {
        LOG_INF("--------------mot_naples_sc800csa module info checksum success------------\n");
    }
    return ret;
}

static bool mot_naples_sc800csa_read_awb_info(u8 moduleflag)
{
    bool ret = false;
    pr_debug(PFX,"--------------mot_naples_sc800csa awb info read begin------------\n");
    if (moduleflag  == 1) {
        mot_naples_sc800csa_iReadData(AWB_GROUP1_INFO_PAGE, OTP_AWB_GROUP1_FLAG, 1, &mot_naples_sc800csa_otp_info.awb_flag);
        mot_naples_sc800csa_iReadData(AWB_GROUP1_INFO_PAGE, AWB_GROUP1_INFO_ADDR, AWB_INFO_LENGTH, &mot_naples_sc800csa_otp_info.awb_param[0]);
        mot_naples_sc800csa_iReadData(AWB_GROUP1_INFO_PAGE, AWB_GROUP1_CHECKSUM, 1, &mot_naples_sc800csa_otp_info.awbChksum);
    } else if (moduleflag == 2) {
        mot_naples_sc800csa_iReadData(AWB_GROUP2_INFO_PAGE, OTP_AWB_GROUP2_FLAG, 1, &mot_naples_sc800csa_otp_info.awb_flag);
        mot_naples_sc800csa_iReadData(AWB_GROUP2_INFO_PAGE, AWB_GROUP2_INFO_ADDR, AWB_INFO_LENGTH, &mot_naples_sc800csa_otp_info.awb_param[0]);
        mot_naples_sc800csa_iReadData(AWB_GROUP2_INFO_PAGE, AWB_GROUP2_CHECKSUM, 1, &mot_naples_sc800csa_otp_info.awbChksum);
    } else {
        pr_debug(PFX,"--------------mot_naples_sc800csa awb info read failed------------\n");
    }
    pr_debug(PFX,"--------------mot_naples_sc800csa awb info read end------------\n");
    ret = mot_naples_sc800csa_param_checksum(&mot_naples_sc800csa_otp_info.awb_flag, AWB_INFO_LENGTH + 1, mot_naples_sc800csa_otp_info.awbChksum);
    if (ret) {
        LOG_INF("--------------mot_naples_sc800csa awb info checksum success------------\n");
    }
    return ret;
}

static bool mot_naples_sc800csa_read_lsc_info(u8 moduleflag)
{
    bool ret = false;
    int idex = 0;

    u8 *pBuff;
    pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read begin------------\n");
    if (moduleflag  == 1) {
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part1 idex %d------------\n", idex);
        /*get lsc flag*/
        mot_naples_sc800csa_iReadData(LSC_GROUP1_PART1_INFO_PAGE, OTP_LSC_GROUP1_FLAG, 1, &mot_naples_sc800csa_otp_info.lsc_flag);
        /*get lsc para*/
        pBuff = &mot_naples_sc800csa_otp_info.lsc_param[idex];
        mot_naples_sc800csa_iReadData(LSC_GROUP1_PART1_INFO_PAGE, LSC_GROUP1_PART1_INFO_ADDR, LSC_GROUP1_PART1_INFO_LENGTH, pBuff);

        idex = idex + LSC_GROUP1_PART1_INFO_LENGTH;
        pBuff = &mot_naples_sc800csa_otp_info.lsc_param[idex];
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part2 idex %d ------------\n", idex);
        mot_naples_sc800csa_iReadData(LSC_GROUP1_PART2_INFO_PAGE, LSC_GROUP1_PART2_INFO_ADDR, LSC_GROUP1_PART2_INFO_LENGTH,  pBuff/*&mot_naples_sc800csa_otp_info.lsc_param[idex]*/);

        idex = idex + LSC_GROUP1_PART2_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part3 idex %d ------------\n", idex);
        pBuff = &mot_naples_sc800csa_otp_info.lsc_param[idex];
        mot_naples_sc800csa_iReadData(LSC_GROUP1_PART3_INFO_PAGE, LSC_GROUP1_PART3_INFO_ADDR, LSC_GROUP1_PART3_INFO_LENGTH, pBuff/*&mot_naples_sc800csa_otp_info.lsc_param[idex]*/);

        idex = idex + LSC_GROUP1_PART3_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part4 idex %d ------------\n", idex);
        pBuff = &mot_naples_sc800csa_otp_info.lsc_param[idex];
        mot_naples_sc800csa_iReadData(LSC_GROUP1_PART4_INFO_PAGE, LSC_GROUP1_PART4_INFO_ADDR, LSC_GROUP1_PART4_INFO_LENGTH, pBuff/*&mot_naples_sc800csa_otp_info.lsc_param[idex]*/);

        idex = idex + LSC_GROUP1_PART4_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part5 idex %d ------------\n", idex);
        pBuff = &mot_naples_sc800csa_otp_info.lsc_param[idex];
        mot_naples_sc800csa_iReadData(LSC_GROUP1_PART5_INFO_PAGE, LSC_GROUP1_PART5_INFO_ADDR, LSC_GROUP1_PART5_INFO_LENGTH, pBuff/*&mot_naples_sc800csa_otp_info.lsc_param[idex]*/);

        /*for (int i = 0; i < LSC_INFO_LENGTH; i++)
        {
            LOG_INF("summation index %d , data = 0x%x\n", i, mot_naples_sc800csa_otp_info.lsc_param[i]);
        }*/

        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read checksum ------------\n");
        mot_naples_sc800csa_iReadData(LSC_GROUP1_PART5_INFO_PAGE, LSC_GROUP1_CHECKSUM, 1, &mot_naples_sc800csa_otp_info.lscChksum);
    } else if (moduleflag  == 2) {
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part1 ------------\n");
        /*get lsc flag*/
        mot_naples_sc800csa_iReadData(LSC_GROUP2_PART1_INFO_PAGE, OTP_LSC_GROUP2_FLAG, 1, &mot_naples_sc800csa_otp_info.lsc_flag);
        /*get lsc para*/
        mot_naples_sc800csa_iReadData(LSC_GROUP2_PART1_INFO_PAGE, LSC_GROUP2_PART1_INFO_ADDR, LSC_GROUP2_PART1_INFO_LENGTH, &mot_naples_sc800csa_otp_info.lsc_param[idex]);

        idex += LSC_GROUP2_PART1_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part2 ------------\n");
        mot_naples_sc800csa_iReadData(LSC_GROUP2_PART2_INFO_PAGE, LSC_GROUP2_PART2_INFO_ADDR, LSC_GROUP2_PART2_INFO_LENGTH, &mot_naples_sc800csa_otp_info.lsc_param[idex]);

        idex += LSC_GROUP2_PART2_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part3 ------------\n");
        mot_naples_sc800csa_iReadData(LSC_GROUP2_PART3_INFO_PAGE, LSC_GROUP2_PART3_INFO_ADDR, LSC_GROUP2_PART3_INFO_LENGTH, &mot_naples_sc800csa_otp_info.lsc_param[idex]);

        idex += LSC_GROUP2_PART3_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part4 ------------\n");
        mot_naples_sc800csa_iReadData(LSC_GROUP2_PART4_INFO_PAGE, LSC_GROUP2_PART4_INFO_ADDR, LSC_GROUP2_PART4_INFO_LENGTH, &mot_naples_sc800csa_otp_info.lsc_param[idex]);

        idex += LSC_GROUP2_PART4_INFO_LENGTH;
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read part5 ------------\n");
        mot_naples_sc800csa_iReadData(LSC_GROUP2_PART5_INFO_PAGE, LSC_GROUP2_PART5_INFO_ADDR, LSC_GROUP2_PART5_INFO_LENGTH, &mot_naples_sc800csa_otp_info.lsc_param[idex]);

        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read checksum ------------\n");
        mot_naples_sc800csa_iReadData(LSC_GROUP2_PART5_INFO_PAGE, LSC_GROUP2_CHECKSUM, 1, &mot_naples_sc800csa_otp_info.lscChksum);
    } else {
        pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read failed------------\n");
    }
    pr_debug(PFX,"--------------mot_naples_sc800csa lsc info read end------------\n");
    ret = mot_naples_sc800csa_param_checksum(&mot_naples_sc800csa_otp_info.lsc_flag, LSC_INFO_LENGTH + 1, mot_naples_sc800csa_otp_info.lscChksum);
    if (ret) {
        LOG_INF("--------------mot_naples_sc800csa lsc info checksum success------------\n");
    }
    return ret;
}

void read_mot_naples_sc800csa_otp_data(void)
{
    u8 moduleflag =0;
    u8 value =0;
    u8 awb_flag = 0;
    u8 lsc_flag = 0;
    u8 all_Data_Chksum = 0;

    pr_debug(PFX,"mot_naples_sc800csa moduleflag read begin");
    /*group valid flag check*/
    mot_naples_sc800csa_iReadData(MODULE_GROUP_INFO_PAGE, OTP_MODULE_FLAG, 1, &value);
    if (value == 0x01) {
        moduleflag = 1;
        /*awb flag check*/
        mot_naples_sc800csa_iReadData(AWB_GROUP1_INFO_PAGE, OTP_AWB_GROUP1_FLAG, 1, &awb_flag);
        /*lsc flag check*/
        mot_naples_sc800csa_iReadData(LSC_GROUP1_PART1_INFO_PAGE, OTP_LSC_GROUP1_FLAG, 1, &lsc_flag);
        /*all checksum*/
        mot_naples_sc800csa_iReadData(TOTAL_GROUP1_INFO_PAGE, TOTAL_GROUP1_CHECKSUM, 1, &all_Data_Chksum);
    } else if (value == 0x03) {
        pr_debug(PFX,"mot_naples_sc800csa group flag = 0x%x", value);
        moduleflag = 2;
        /*awb flag check*/
        mot_naples_sc800csa_iReadData(AWB_GROUP2_INFO_PAGE, OTP_AWB_GROUP2_FLAG, 1, &awb_flag);
        /*lsc flag check*/
        mot_naples_sc800csa_iReadData(LSC_GROUP2_PART1_INFO_PAGE, OTP_LSC_GROUP2_FLAG, 1, &lsc_flag);
        /*all checksum*/
        mot_naples_sc800csa_iReadData(TOTAL_GROUP2_INFO_PAGE, TOTAL_GROUP2_CHECKSUM, 1, &all_Data_Chksum);
    }

    pr_debug(PFX,"mot_naples_sc800csa moduleflag = 0x%x end", moduleflag);
    if (moduleflag != 1 && moduleflag != 2) {
        pr_debug(PFX,"mot_naples_sc800csa invalid moduleflag = 0x%x", moduleflag);
        return;
    }
    if (awb_flag != 0x55 && lsc_flag != 0x55) {
        pr_debug(PFX,"mot_naples_sc800csa invalid data flag, awb_flag = 0x%x, lsc_flag = 0x%x", awb_flag, lsc_flag);
        return;
    }

    checksummodule = mot_naples_sc800csa_read_module_info(moduleflag);
    checksumawb = mot_naples_sc800csa_read_awb_info(moduleflag);
    checksumlsc = mot_naples_sc800csa_read_lsc_info(moduleflag);
    checksumall = mot_naples_sc800csa_all_data_checksum(&mot_naples_sc800csa_otp_info.module_param[0], ALL_DATA_SIZE - 1, all_Data_Chksum);

    if (true == (checksummodule & checksumawb & checksumlsc & checksumall))
    {
        LOG_INF("----------------mot_naples_sc800csa otp info check success----------------");
    }
    else
    {
        LOG_INF("----------------mot_naples_sc800csa otp info check fail-------------------");
    }
}

unsigned int sc800csa_read_region(struct i2c_client *client, unsigned int addr,
                                unsigned char *data, unsigned int size)
{
    unsigned char *dataTmp = data;

    pr_err("mot_naples_sc800csa otp region addr = 0x%x, size = %d\n", addr, size);
    if (addr == 0x1 && size == 1) {//0xff
        *(u8 *)data = 0x00000006;
    } else if (addr == 0x0 && size == 1928) {
        unsigned int totalSize = sizeof(mot_naples_sc800csa_otp_info.module_param) +
                                 sizeof(mot_naples_sc800csa_otp_info.moduleChksum) +
                                 sizeof(mot_naples_sc800csa_otp_info.awb_flag) +
                                 sizeof(mot_naples_sc800csa_otp_info.awb_param) +
                                 sizeof(mot_naples_sc800csa_otp_info.awbChksum) +
                                 sizeof(mot_naples_sc800csa_otp_info.lsc_flag) +
                                 sizeof(mot_naples_sc800csa_otp_info.lsc_param) +
                                 sizeof(mot_naples_sc800csa_otp_info.lscChksum) +
                                 sizeof(mot_naples_sc800csa_otp_info.allDataChksum);
        pr_err("mot_naples_sc800csa otp region addr = 0x%x, size = %d  totalSize=%d \n", addr, size, totalSize);
        if (size == totalSize) {

            memcpy(dataTmp, mot_naples_sc800csa_otp_info.module_param, sizeof(mot_naples_sc800csa_otp_info.module_param));
            dataTmp += sizeof(mot_naples_sc800csa_otp_info.module_param);

            data[37] = mot_naples_sc800csa_otp_info.moduleChksum[0];
            data[38] = mot_naples_sc800csa_otp_info.moduleChksum[1];
            dataTmp += sizeof(mot_naples_sc800csa_otp_info.moduleChksum);

            data[39] = mot_naples_sc800csa_otp_info.awb_flag;
            dataTmp += sizeof(mot_naples_sc800csa_otp_info.awb_flag);

            memcpy(dataTmp, mot_naples_sc800csa_otp_info.awb_param, sizeof(mot_naples_sc800csa_otp_info.awb_param));
            dataTmp += sizeof(mot_naples_sc800csa_otp_info.awb_param);

            data[56] = mot_naples_sc800csa_otp_info.awbChksum;
            dataTmp += sizeof(mot_naples_sc800csa_otp_info.awbChksum);

            data[57] = mot_naples_sc800csa_otp_info.lsc_flag;
            dataTmp += sizeof(mot_naples_sc800csa_otp_info.lsc_flag);

            memcpy(dataTmp, mot_naples_sc800csa_otp_info.lsc_param, sizeof(mot_naples_sc800csa_otp_info.lsc_param));
            dataTmp += sizeof(mot_naples_sc800csa_otp_info.lsc_param);

            data[1926] = mot_naples_sc800csa_otp_info.lscChksum;
            dataTmp += sizeof(mot_naples_sc800csa_otp_info.lscChksum);

            data[totalSize - 1] = mot_naples_sc800csa_otp_info.allDataChksum;
        } else {
            pr_err("mot_naples_sc800csa otp size != totalSize");
            size = totalSize;
        }
    } else{
        pr_err("mot_naples_sc800csa otp add = 0x%x, size = %d ,read error !!!\n",addr,size);
    }
    return size;
}
EXPORT_SYMBOL(sc800csa_read_region);
