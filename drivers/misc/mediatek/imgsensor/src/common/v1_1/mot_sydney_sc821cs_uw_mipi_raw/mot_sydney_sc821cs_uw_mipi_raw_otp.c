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
#include "mot_sydney_sc821cs_uw_mipi_raw_Sensor.h"
#include "mot_sydney_sc821cs_uw_mipi_raw_otp.h"
#define LOG_INF(format, args...)		pr_err(PFX "[%s] " format, __func__, ##args)
#define SC821CS_OTP_DEBUG_ON 1

static  struct imgsensor_struct *imgsensor;

static mot_calibration_status_t calibration_status = {NO_ERRORS};
static mot_calibration_mnf_t mnf_info = {0};

struct sc821cs_dd_uw_otp_struct sc821cs_dd_uw_otp = {0};


bool checksummodule = false;
bool checksumawb = false;
bool checksumlsc = false;
bool checksumall = false;

static void SYDNEY_SC821CS_UW_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;
	uint8_t* module_info = data;
    	// lens_id
	struct SYDNEY_SC821CS_UW_eeprom_t eeprom = {
        	.lens_id = module_info[10],
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


mot_calibration_status_t *SYDNEY_SC821CS_UW_eeprom_get_calibration_status(void)
{
	return &calibration_status;
}

mot_calibration_mnf_t *SYDNEY_SC821CS_UW_eeprom_get_mnf_info(void)
{
	return &mnf_info;
}

void SYDNEY_SC821CS_UW_eeprom_format_calibration_data(struct imgsensor_struct *pImgsensor)
{
	imgsensor = pImgsensor;
	SYDNEY_SC821CS_UW_eeprom_get_mnf_data((void *)sc821cs_dd_uw_otp.module_info, &mnf_info);
}

static void SC821CS_Otp_Write_I2C_CAM_CAL_U8(u16 addr, u8 para)
{
	char pu_send_cmd[3] = { (char)(addr >> 8), (char)(addr & 0xFF),
				(char)(para & 0xFF) };

	iWriteRegI2C(pu_send_cmd, 3, OTP_I2C_ADDR);
}

static int SC821CS_Otp_Read_I2C_CAM_CAL(u16 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, OTP_I2C_ADDR);

	return get_byte;
}

static int sc821cs_set_page_and_load_data(int page) //set page
{
	// uint64_t Startaddress = 0;
	// uint64_t EndAddress = 0;
	int first_delay = 1;
	int delay = 0;
	int pag = 0;

	// Startaddress = page * 0x400 + 0x7C00; //set start address in page
	// EndAddress = Startaddress + 0x3ff; //set end address in page
	pag = page * 2 - 1; //change page
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4408, 0x00);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4409, 0x00);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x448a, 0x03);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x448b, 0xff);

	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4401, 0x15); // address set finished
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4412, pag & 0xff); // set page
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4407, (page == 1 ? 0x11 : 0x00));// set page finished
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4402, 0x01);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4403, 0x03);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4404, 0x09);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4405, 0x0b);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x440c, 0x0e);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x440e, 0x0b);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x0100, 0x00);
	SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x4400, 0x11); // manual load begin
	while ((SC821CS_Otp_Read_I2C_CAM_CAL(0x4420) & 0x01) == 0x01) {
		delay++;
		pr_err("set_page waitting, OTP is still busy for loading %d times\n", delay);
		if (delay == 5) {
			pr_err("set_page fail, load timeout!!!\n");

			return SC821CS_OTP_RET_FAIL;
		}
		if (first_delay == 1) {
        	mdelay(15);
			first_delay = 0;
    	} else {
       		mdelay(10);
		}
	}
	pr_err("set_page success\n");

	return SC821CS_OTP_RET_SUCCESS;
}

static int sc821cs_set_threshold(u8 threshold) //set thereshold
{
	int threshold_reg1[3] = { 0x78, 0x78, 0x78 };
	int threshold_reg2[3] = { 0x02, 0x02, 0x02 };
	int threshold_reg3[3] = { 0x42, 0x71, 0x03 };

	if (threshold < 3 && threshold >= 0) {
		SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x36b0, threshold_reg1[threshold]);
		SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x36b1, threshold_reg2[threshold]);
		SC821CS_Otp_Write_I2C_CAM_CAL_U8(0x36b2, threshold_reg3[threshold]);
		pr_err("set_threshold %d\n", threshold);
	} else {
		pr_err("set invalid threshold %d\n", threshold);

		return SC821CS_OTP_RET_FAIL;
	}

	return SC821CS_OTP_RET_SUCCESS;
}

static int sc821cs_sensor_otp_read_data(unsigned int ui4_offset,
					unsigned int ui4_length, unsigned char *pinputdata)
{
	int i;
	unsigned int checksum_cal = 0;
	unsigned int checksum = 0;

	switch (ui4_offset) {
	case SC821CS_OTP_MODULE_GROUP1_STARTADDR:
	case SC821CS_OTP_MODULE_GROUP2_STARTADDR:
	case SC821CS_OTP_MODULE_GROUP3_STARTADDR:
		pinputdata[0] = sc821cs_dd_uw_otp.ModuleFlag;
		break;
	case SC821CS_OTP_SN_GROUP1_STARTADDR:
	case SC821CS_OTP_SN_GROUP2_STARTADDR:
	case SC821CS_OTP_SN_GROUP3_STARTADDR:
		pinputdata[0] = sc821cs_dd_uw_otp.SNFlag;
		break;
	case SC821CS_OTP_AWB_GROUP1_STARTADDR:
	case SC821CS_OTP_AWB_GROUP2_STARTADDR:
	case SC821CS_OTP_AWB_GROUP3_STARTADDR:
		pinputdata[0] = sc821cs_dd_uw_otp.WBFlag;
		break;
	default:
		break;
	}

	for (i = 0; i < ui4_length; i++) {
		pinputdata[i] = SC821CS_Otp_Read_I2C_CAM_CAL(ui4_offset + i);
		checksum_cal += pinputdata[i];
#if SC821CS_OTP_DEBUG_ON
		pr_err("addr=0x%x, data=0x%x\n", ui4_offset + i - 1, pinputdata[i]);
#endif
	}

	checksum = pinputdata[i - 1];
	checksum_cal -= checksum;
	checksum_cal = checksum_cal % 255 + 1;
	if (checksum_cal != checksum) {
		pr_err("checksum fail, checksum_cal=0x%x, checksum=0x%x\n",
				checksum_cal, checksum);

		return SC821CS_OTP_RET_FAIL;
	}
	pr_err("checksum success, checksum_cal=0x%x, checksum=0x%x, addr :0x%x\n",
		checksum_cal, checksum, i - 1);

	return SC821CS_OTP_RET_SUCCESS;
}

static int sc821cs_sensor_otp_read_module_info(unsigned char *pinputdata)
{
	int ret = SC821CS_OTP_RET_FAIL;

	sc821cs_dd_uw_otp.ModuleFlag =
		SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_MODULE_FLAGADDR);
	pr_err("Read ModuleFlag addr :0x%x, data:0x%x\n",
			SC821CS_OTP_MODULE_FLAGADDR,
			sc821cs_dd_uw_otp.ModuleFlag);

	if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP1_FLAG) {
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_MODULE_GROUP1_STARTADDR,
			SC821CS_DD_UW_OTP_MODULE_LENS, pinputdata);
		pr_err("group1 ret = %d!\n", ret);
	} else if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP2_FLAG) {
		sc821cs_set_page_and_load_data(3);
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_MODULE_GROUP2_STARTADDR,
			SC821CS_DD_UW_OTP_MODULE_LENS, pinputdata);
		pr_err("group2 ret = %d!\n", ret);
	} else if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP3_FLAG) {
		sc821cs_set_page_and_load_data(5);
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_MODULE_GROUP3_STARTADDR,
			SC821CS_DD_UW_OTP_MODULE_LENS, pinputdata);
		pr_err("group3 ret = %d!\n", ret);
	} else {
		pr_err("invalid flag :0x%x\n",
				sc821cs_dd_uw_otp.ModuleFlag);
	}

	return ret;
}

static int sc821cs_sensor_otp_read_awb_info(unsigned char *pinputdata)
{
	int ret = SC821CS_OTP_RET_FAIL;

	if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP1_FLAG) {
		sc821cs_dd_uw_otp.WBFlag =
		SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_AWB_GROUP1_FLAGADDR);
		pr_err("Read WBFlag addr :0x%x, data:0x%x\n",
		SC821CS_OTP_AWB_GROUP1_FLAGADDR, sc821cs_dd_uw_otp.WBFlag);
		if(sc821cs_dd_uw_otp.WBFlag != 0x01){
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.WBFlag);
			return ret;
		}
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_AWB_GROUP1_STARTADDR,
			SC821CS_DD_UW_OTP_AWB_LENS, pinputdata);
		pr_err("group1 ret = %d!\n", ret);
	} else if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP2_FLAG) {
		sc821cs_set_page_and_load_data(3);
		sc821cs_dd_uw_otp.WBFlag =
		SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_AWB_GROUP2_FLAGADDR);
		pr_err("Read WBFlag addr :0x%x, data:0x%x\n",
		SC821CS_OTP_AWB_GROUP2_FLAGADDR, sc821cs_dd_uw_otp.WBFlag);
		if(sc821cs_dd_uw_otp.WBFlag != 0x01){
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.WBFlag);
			return ret;
		}
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_AWB_GROUP2_STARTADDR,
			SC821CS_DD_UW_OTP_AWB_LENS, pinputdata);
		pr_err("group2 ret = %d!\n", ret);
	} else if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP3_FLAG) {
		sc821cs_set_page_and_load_data(5);
		sc821cs_dd_uw_otp.WBFlag =
		SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_AWB_GROUP3_FLAGADDR);
		pr_err("Read WBFlag addr :0x%x, data:0x%x\n",
		SC821CS_OTP_AWB_GROUP3_FLAGADDR, sc821cs_dd_uw_otp.WBFlag);
		if(sc821cs_dd_uw_otp.WBFlag != 0x01){
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.WBFlag);
			return ret;
		}
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_AWB_GROUP3_STARTADDR,
			SC821CS_DD_UW_OTP_AWB_LENS, pinputdata);
		pr_err("group3 ret = %d!\n", ret);
	} else
		pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.WBFlag);

	return ret;
}

static unsigned int sc821cs_sensor_otp_read_lsc_data(int page, unsigned int lscpage3_offset,
							unsigned int ui4_length, unsigned char *pinputdata)
{
	int i;
	unsigned int checksum_cal = 0;
	unsigned int ui4_lsc_offset = 0;
	unsigned int ui4_lsc_length = 0;

	ui4_lsc_offset = lscpage3_offset;
	ui4_lsc_length = ui4_length;

	for (i = 0; i < ui4_lsc_length; i++) {
		pinputdata[i] =
			SC821CS_Otp_Read_I2C_CAM_CAL(ui4_lsc_offset + i);
		checksum_cal += pinputdata[i];
#if SC821CS_OTP_DEBUG_ON
		pr_err("addr=0x%x, data=0x%x\n", ui4_lsc_offset + i, pinputdata[i]);
#endif
	}
	//checksum_cal = checksum_cal % 255;
	pr_err("read otp page:%d lsc_data success,checksum_cal:0x%x\n",
			page, checksum_cal);

	return checksum_cal;
}

static int sc821cs_sensor_otp_read_lsc_info(unsigned char *pinputdata)
{
    int ret = SC821CS_OTP_RET_FAIL;
    int page = SC821CS_OTP_PAGE1;
    unsigned int checksum_cal = 0;
    unsigned int checksum = 0;
    unsigned int checksum_addr = 0;
    char *group_name = "";
    int start_page1, start_page2, start_page3;
    unsigned int lscpage1_offset = 0, lscpage2_offset = 0, lscpage3_offset = 0;
	unsigned int page_lensize1 = 0, page_lensize2 = 0, page_lensize3 = 0;

    if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP1_FLAG) {
        start_page1 = 1;
        start_page2 = 2;
		start_page3 = 3;

		page_lensize1 = 0x83FB - 0x82B8 + 1;
		page_lensize2 = 0x87FB - 0x8468 + 1;
		page_lensize3 = 0x8ADB - 0x8868 + 1;

        lscpage1_offset = 0x82B8;
		lscpage2_offset = 0x8468;
		lscpage3_offset = 0x8868;

		sc821cs_dd_uw_otp.LscFlag = SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_LSC_GROUP1_FLAGADDR);
		pinputdata[0] = sc821cs_dd_uw_otp.LscFlag;
		pr_err("Read LscFlag addr :0x%x, data:0x%x\n",
	            SC821CS_OTP_LSC_GROUP1_FLAGADDR, sc821cs_dd_uw_otp.LscFlag);
		if (sc821cs_dd_uw_otp.LscFlag != 0x01) {
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.LscFlag);
			return ret;
		}

        checksum_addr = SC821CS_OTP_LSC_GROUP1_CHECKSUMADDR;
        group_name = "group1";
    } else if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP2_FLAG) {
        start_page1 = 3;
        start_page2 = 4;
		start_page3 = 5;

		page_lensize1 = 0x8BFB - 0x8B09 + 1;
		page_lensize2 = 0x8FFB - 0x8C68 + 1;
		page_lensize3 = 0x932C - 0x9068 + 1;

		lscpage1_offset = 0x8B09;
		lscpage2_offset = 0x8C68;
		lscpage3_offset = 0x9068;

		sc821cs_set_page_and_load_data(start_page1);
		sc821cs_dd_uw_otp.LscFlag = SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_LSC_GROUP2_FLAGADDR);
		pinputdata[0] = sc821cs_dd_uw_otp.LscFlag;
		pr_err("Read LscFlag addr :0x%x, data:0x%x\n",
	            SC821CS_OTP_LSC_GROUP2_FLAGADDR, sc821cs_dd_uw_otp.LscFlag);
		if (sc821cs_dd_uw_otp.LscFlag != 0x01) {
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.LscFlag);
			return ret;
		}

        checksum_addr = SC821CS_OTP_LSC_GROUP2_CHECKSUMADDR;
        group_name = "group2";
    } else if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP3_FLAG) {
        start_page1 = 5;
        start_page2 = 6;
		start_page3 = 7;

		page_lensize1 = 0x93FB - 0x935A + 1;
		page_lensize2 = 0x97FB - 0x9468 + 1;
		page_lensize3 = 0x9B7D - 0x9868 + 1;

        lscpage1_offset = 0x935A;
		lscpage2_offset = 0x9468;
		lscpage3_offset = 0x9868;

		sc821cs_set_page_and_load_data(start_page1);
		sc821cs_dd_uw_otp.LscFlag = SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_LSC_GROUP3_FLAGADDR);
		pinputdata[0] = sc821cs_dd_uw_otp.LscFlag;
		pr_err("Read LscFlag addr :0x%x, data:0x%x\n",
	            SC821CS_OTP_LSC_GROUP3_FLAGADDR, sc821cs_dd_uw_otp.LscFlag);
		if (sc821cs_dd_uw_otp.LscFlag != 0x01) {
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.LscFlag);
			return ret;
		}

        checksum_addr = SC821CS_OTP_LSC_GROUP3_CHECKSUMADDR;
        group_name = "group3";
    } else {
        pr_err("lsc_info invalid flag 0x%x!\n", sc821cs_dd_uw_otp.LscFlag);
        return ret;
    }

    sc821cs_set_page_and_load_data(start_page1);
    checksum_cal += sc821cs_sensor_otp_read_lsc_data(
        page, lscpage1_offset, page_lensize1,
        pinputdata + 1);
    sc821cs_set_page_and_load_data(start_page2);
    checksum_cal += sc821cs_sensor_otp_read_lsc_data(
        page, lscpage2_offset, page_lensize2,
        pinputdata + 1 + page_lensize1);
    sc821cs_set_page_and_load_data(start_page3);
    checksum_cal += sc821cs_sensor_otp_read_lsc_data(
        page, lscpage3_offset, page_lensize3,
        pinputdata + 1 + page_lensize1 + page_lensize2);
    checksum_cal = checksum_cal % 255 + 1;
    checksum = SC821CS_Otp_Read_I2C_CAM_CAL(checksum_addr);

    if (checksum_cal == checksum) {
        pr_err("%s checksum pass! checksum_cal = 0x%x, checksum = 0x%x, addr: 0x%x\n",
                   group_name, checksum_cal, checksum, checksum_addr);
        ret = SC821CS_OTP_RET_SUCCESS;
    } else {
        pr_err("%s checksum fail, checksum_cal:0x%x, checksum:0x%x!\n",
                   group_name, checksum_cal, checksum);
        ret = SC821CS_OTP_RET_FAIL;
    }

    return ret;
}

static int sc821cs_sensor_otp_read_sn_info(unsigned char *pinputdata)
{
	int ret = SC821CS_OTP_RET_FAIL;

	if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP1_FLAG) {
		sc821cs_set_page_and_load_data(3);
		sc821cs_dd_uw_otp.SNFlag = SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_SN_GROUP1_FLAGADDR);
		pr_err("Read SNFlag addr :0x%x, data:0x%x\n",SC821CS_OTP_SN_GROUP1_FLAGADDR, sc821cs_dd_uw_otp.SNFlag);
		if(sc821cs_dd_uw_otp.SNFlag != 0x01){
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.SNFlag);
			return ret;
		}
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_SN_GROUP1_STARTADDR,
			SC821CS_DD_UW_OTP_SN_LENS, pinputdata);
		pr_err("group1 ret = %d!\n", ret);
	} else if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP2_FLAG) {
		sc821cs_set_page_and_load_data(5);
		sc821cs_dd_uw_otp.SNFlag = SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_SN_GROUP2_FLAGADDR);
		pr_err("Read SNFlag addr :0x%x, data:0x%x\n",SC821CS_OTP_SN_GROUP2_FLAGADDR, sc821cs_dd_uw_otp.SNFlag);
		if(sc821cs_dd_uw_otp.SNFlag != 0x01){
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.SNFlag);
			return ret;
		}
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_SN_GROUP2_STARTADDR,
			SC821CS_DD_UW_OTP_SN_LENS, pinputdata);
		pr_err("group2 ret = %d!\n", ret);
	} else if (sc821cs_dd_uw_otp.ModuleFlag == SC821CS_GROUP3_FLAG) {
		sc821cs_set_page_and_load_data(7);
		sc821cs_dd_uw_otp.SNFlag = SC821CS_Otp_Read_I2C_CAM_CAL(SC821CS_OTP_SN_GROUP3_FLAGADDR);
		pr_err("Read SNFlag addr :0x%x, data:0x%x\n",SC821CS_OTP_SN_GROUP3_FLAGADDR, sc821cs_dd_uw_otp.SNFlag);
		if(sc821cs_dd_uw_otp.SNFlag != 0x01){
			pr_err("invalid flag = 0x%x!\n", sc821cs_dd_uw_otp.SNFlag);
			return ret;
		}
		ret = sc821cs_sensor_otp_read_data(
			SC821CS_OTP_SN_GROUP3_STARTADDR,
			SC821CS_DD_UW_OTP_SN_LENS, pinputdata);
		pr_err("group3 ret = %d!\n", ret);
	} else
		pr_err("sn_info invalid flag 0x%x!\n", sc821cs_dd_uw_otp.SNFlag);

	return ret;
}

void read_mot_sydney_sc821cs_uw_otp_data(void)
{
    int threshold = 0;
	int ret = SC821CS_OTP_RET_FAIL;

    pr_err(PFX,"wangliang mot_sydney_sc821cs_uw moduleflag read begin");
    /*group valid flag check*/
    for (threshold = 0; threshold < 3; threshold++) {
		sc821cs_set_threshold(threshold);
		sc821cs_set_page_and_load_data(SC821CS_OTP_PAGE1);
		ret = sc821cs_sensor_otp_read_module_info(
			sc821cs_dd_uw_otp.module_info);
		if (ret == SC821CS_OTP_RET_FAIL) {
			sc821cs_dd_uw_otp.ModuleFlag = SC821CS_INVALID_FLAG;
			pr_err("read module info in threshold R%d fail\n", threshold);
			//continue;
		}

        ret = sc821cs_sensor_otp_read_awb_info(
			sc821cs_dd_uw_otp.wb_data);
		if (ret == SC821CS_OTP_RET_FAIL) {
			sc821cs_dd_uw_otp.WBFlag = SC821CS_INVALID_FLAG;
			pr_err("read awb info  in threshold R%d fail\n", threshold);
			continue;
		}

        ret = sc821cs_sensor_otp_read_lsc_info(
			sc821cs_dd_uw_otp.lsc_data);
		if (ret == SC821CS_OTP_RET_FAIL) {
			sc821cs_dd_uw_otp.LscFlag = SC821CS_INVALID_FLAG;
			pr_err("read lsc info  in threshold R%d fail\n", threshold);
			continue;
		}

		ret = sc821cs_sensor_otp_read_sn_info(
			sc821cs_dd_uw_otp.sn_data);
		if (ret == SC821CS_OTP_RET_FAIL) {
			sc821cs_dd_uw_otp.SNFlag = SC821CS_INVALID_FLAG;
			pr_err("read sn_data in threshold R%d fail\n", threshold);
			continue;
		}

        pr_err("read all otp data in threshold R%d success\n", threshold);
		break;
    }
    if (ret == SC821CS_OTP_RET_FAIL) {
		pr_err("read otp data  in threshold R1 R2 R3 all failed!\n");
	}
}

unsigned int sc821cs_uw_read_region(struct i2c_client *client, unsigned int addr,
                                unsigned char *data, unsigned int size)
{
    unsigned char *dataTmp = data;

    pr_err("mot_sydney_sc821cs_uw otp region addr = 0x%x, size = %d\n", addr, size);
    if (addr == 0x1 && size == 1) {//0xff
        *(u8 *)data = 0x00000006;
    } else {
        unsigned int totalSize = sizeof(sc821cs_dd_uw_otp.ModuleFlag) +
                                 sizeof(sc821cs_dd_uw_otp.SNFlag) +
                                 sizeof(sc821cs_dd_uw_otp.WBFlag) +
                                 sizeof(sc821cs_dd_uw_otp.LscFlag) +
                                 sizeof(sc821cs_dd_uw_otp.module_info) +
                                 sizeof(sc821cs_dd_uw_otp.sn_data) +
                                 sizeof(sc821cs_dd_uw_otp.wb_data) +
                                 sizeof(sc821cs_dd_uw_otp.lsc_data);
        pr_err("mot_sydney_sc821cs_uw otp region addr = 0x%x, size = %d  totalSize=%d \n", addr, size, totalSize);
        if (1/*size == totalSize*/) {

            data[0] = sc821cs_dd_uw_otp.ModuleFlag;

            memcpy(dataTmp, sc821cs_dd_uw_otp.module_info, sizeof(sc821cs_dd_uw_otp.module_info));
            dataTmp += sizeof(sc821cs_dd_uw_otp.module_info);

			data[9] = sc821cs_dd_uw_otp.WBFlag;
            dataTmp += sizeof(sc821cs_dd_uw_otp.WBFlag);

            memcpy(dataTmp, sc821cs_dd_uw_otp.wb_data, sizeof(sc821cs_dd_uw_otp.wb_data));
            dataTmp += sizeof(sc821cs_dd_uw_otp.wb_data);

			data[27] = sc821cs_dd_uw_otp.LscFlag;
            dataTmp += sizeof(sc821cs_dd_uw_otp.LscFlag);

            memcpy(dataTmp, sc821cs_dd_uw_otp.lsc_data, sizeof(sc821cs_dd_uw_otp.lsc_data));
            dataTmp += sizeof(sc821cs_dd_uw_otp.lsc_data);

            data[1897] = sc821cs_dd_uw_otp.SNFlag;
            dataTmp += sizeof(sc821cs_dd_uw_otp.SNFlag);

            memcpy(dataTmp, sc821cs_dd_uw_otp.sn_data, sizeof(sc821cs_dd_uw_otp.sn_data));
            dataTmp += sizeof(sc821cs_dd_uw_otp.sn_data);


        } else {
            pr_err("mot_sydney_sc821cs_uw otp size != totalSize");
            size = totalSize;
        }
    }
    return size;
}
EXPORT_SYMBOL(sc821cs_uw_read_region);
