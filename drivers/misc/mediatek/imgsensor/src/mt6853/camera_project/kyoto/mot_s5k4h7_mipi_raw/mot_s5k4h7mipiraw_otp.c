/*
 * Copyright (C) 2021 lucas (guoqiang8@lenovo.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
  ****************************************************************************/
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
#include "mot_s5k4h7mipiraw_otp.h"

#define PFX "[imgsensor-otp]"
#define LOG_INF(format, args...)     \
	pr_info(PFX "[%s] " format, \
	__func__, ##args)

#define I2C_ADDR 0x5A
#define I2C_SPEED 400
static OTP_INFO  otp_info_map = {
	.mnf_info.page_info.page[0] = 21,
	.mnf_info.page_info.page[1] = 25,
	.mnf_info.page_info.page[2] = 29,
	.mnf_info.page_info.page_offset[0] = {1,0,0},
	.mnf_info.page_info.page_offset[1] = {1,0,0},
	.mnf_info.page_info.page_offset[2] = {1,0,0},
	.mnf_info.page_info.page_addr[0] = 0x0A04,
	.mnf_info.page_info.page_addr[1] = 0x0A04,
	.mnf_info.page_info.page_addr[2] = 0x0A04,
	.mnf_info.size = 33,//include flag

	.awb_info.page_info.page[0] = 21,
	.awb_info.page_info.page[1] = 25,
	.awb_info.page_info.page[2] = 29,
	.awb_info.page_info.page_offset[0] = {2,8,51},
	.awb_info.page_info.page_offset[1] = {2,8,51},
	.awb_info.page_info.page_offset[2] = {2,8,51},
	.awb_info.page_info.page_addr[0] = 0x0A3C,
	.awb_info.page_info.page_addr[1] = 0x0A3C,
	.awb_info.page_info.page_addr[2] = 0x0A3C,
	.awb_info.size = 59,//include flag

	.af_info.page_info.page[0]=121,
	.af_info.page_info.page[1]=122,
	.af_info.page_info.page[2]=123,
	.af_info.page_info.page_offset[0] = {1,0,0},
	.af_info.page_info.page_offset[1] = {1,0,0},
	.af_info.page_info.page_offset[2] = {1,0,0},
	.af_info.page_info.page_addr[0]= 0x0A04,
	.af_info.page_info.page_addr[1]= 0x0A10,
	.af_info.page_info.page_addr[2]= 0x0A1C,
	.af_info.size = 27,//include flag

	.lsc_info.page_info.page[0] = 33,
	.lsc_info.page_info.page[1] = 62,
	.lsc_info.page_info.page[2] = 91,
	.lsc_info.page_info.page_offset[0] = {31,64,15},
	.lsc_info.page_info.page_offset[1] = {31,49,30},
	.lsc_info.page_info.page_offset[2] = {31,34,45},
	.lsc_info.page_info.page_addr[0] = 0x0A04,
	.lsc_info.page_info.page_addr[1] = 0x0A13,
	.lsc_info.page_info.page_addr[2] = 0x0A22,
	.lsc_info.size = 1871,//include flag (lsc + crc +flag) 1868+2+1

};
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2CTiming(pusendcmd, 2, (u8 *)&get_byte, 1, I2C_ADDR, I2C_SPEED);
	return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF),
			(char)(para & 0xFF)};

	iWriteRegI2CTiming(pusendcmd, 3, I2C_ADDR, I2C_SPEED);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {
	(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF) };
	iWriteRegI2CTiming(pusendcmd, 4,I2C_ADDR, I2C_SPEED);

}

//read  sensor otp data
static int moto_read_sensor_otp_data(int page, int start_add, unsigned char *Buff, int size)
{
	unsigned short stram_flag = 0;
	unsigned short val = 0;
	int i = 0;

	if (NULL == Buff) return 0;

	stram_flag = read_cmos_sensor_8(0x0100);
	if (stram_flag == 0) {
		write_cmos_sensor_8(0x0100,0x01);
		mdelay(50);
	}
	write_cmos_sensor_8(0x0a02,page);
	write_cmos_sensor_8(0x3b41,0x01);
	write_cmos_sensor_8(0x3b42,0x03);
	write_cmos_sensor_8(0x3b40,0x01);
	write_cmos_sensor(0x0a00,0x0100); //4 otp enable and read start

	for (i = 0; i <= 100; i++)
	{
		mdelay(1);
		val = read_cmos_sensor_8(0x0A01);
		if (val == 0x01)
			break;
	}

	for ( i = 0; i < size; i++ ) {
		Buff[i] = read_cmos_sensor_8(start_add+i);
	}
	write_cmos_sensor(0x0a00,0x0400);
	write_cmos_sensor(0x0a00,0x0000); //4 //otp enable and read end

	return 0;
}

//get vail page flag
static void mot_select_vail_page(PAGE_INFO *pg_inf)
{
	int i = 0;
	unsigned short page =0;
	unsigned short start_addr =0;
	unsigned char page_flag;

	pg_inf-> page_vail = 0;

	for( i=0; i< DATA_PAGE_NUM; i++)
	{
		page = pg_inf-> page[i];
		start_addr = pg_inf-> page_addr[i];
		moto_read_sensor_otp_data(page,start_addr,&page_flag,1);
		page_flag &= 0xC0;
		if(page_flag)
			break;
	}

	pg_inf-> page_vail = i;
	pg_inf-> flag = (CHECK_PAGE)page_flag;
}

static bool moto_read_awb_data(AWB_INFO *awb_info)
{
	unsigned short page = 0;
	unsigned short page_idx = 0;
	unsigned short start_addr = 0;
	//unsigned int size = awb_info->size;
	unsigned char *buff = awb_info->data;
	PAGE_OFFSETINFO page_offset;
	PAGE_INFO *page_info = &(awb_info->page_info);

	mot_select_vail_page(page_info);
	page_idx = page_info-> page_vail;
	if(DATA_PAGE_NUM > page_idx)
	{
		LOG_INF ("page index = %d flag = 0x%x", page_idx, page_info->flag);
		page = page_info->page[page_idx];
		page_offset = page_info->page_offset[page_idx];
		start_addr = page_info-> page_addr[page_idx];
		moto_read_sensor_otp_data(page,start_addr,buff,page_offset.page_sart_size);
		moto_read_sensor_otp_data(page+1,OTP_PAGE_START_ADDR,
				buff+page_offset.page_sart_size,page_offset.page_end_size);

		return TRUE;
	}

	return FALSE;
}

static bool moto_read_af_data(AF_INFO *af_info)
{
	unsigned short page = 0;
	unsigned short page_idx = 0;
	unsigned short start_addr = 0;
	unsigned int size = af_info->size;
	unsigned char *buff = af_info->data;
	PAGE_OFFSETINFO page_offset;
	PAGE_INFO *page_info = &(af_info->page_info);

	mot_select_vail_page(page_info);
	page_idx = page_info-> page_vail;
	if(DATA_PAGE_NUM > page_idx)
	{
		LOG_INF ("page index = %d flag = 0x%x", page_idx, page_info->flag);
		page = page_info->page[page_idx];
		page_offset = page_info->page_offset[page_idx];
		start_addr = page_info-> page_addr[page_idx];
		moto_read_sensor_otp_data(page,start_addr,buff,size);
		return TRUE;
	}

	return FALSE;
}


static bool moto_read_lsc_data(LSC_INFO *lsc_info)
{
	unsigned short page = 0;
	unsigned short page_idx = 0;
	unsigned short start_addr = 0;
	unsigned short offset = 0;
	unsigned int size = lsc_info->size;
	unsigned char *buff = lsc_info->data;
	PAGE_OFFSETINFO page_offset;
	PAGE_INFO *page_info = &(lsc_info->page_info);
	unsigned int i = 0;

	mot_select_vail_page(page_info);
	page_idx = page_info-> page_vail;
	if(DATA_PAGE_NUM > page_idx)
	{
		page        = page_info->page[page_idx];
		page_offset = page_info->page_offset[page_idx];
		start_addr  = page_info->page_addr[page_idx];
		LOG_INF ("page index = %d flag = 0x%x page num =%d\n", page_idx, page_info->flag,page_offset.page_num);
		for(i =0; i< page_offset.page_num ; i++)
		{

			if (i == (page_offset.page_num -2))
			{
				offset = page_offset.page_sart_size + (page_offset.page_num-3)*OTP_PAGE_SIZE;
				moto_read_sensor_otp_data(page +i,OTP_PAGE_START_ADDR,buff + offset,page_offset.page_end_size);
				break;
			}
			else if((0 < i) && (i <(page_offset.page_num -2)))
			{

				offset = page_offset.page_sart_size + (i-1)*OTP_PAGE_SIZE;
				moto_read_sensor_otp_data(page +i,OTP_PAGE_START_ADDR,buff + offset,OTP_PAGE_SIZE);
			}
			else if (0 == i)
			{
				moto_read_sensor_otp_data(page +i,start_addr,buff,page_offset.page_sart_size);
			}
		}
		LOG_INF ("size = %d  buff[1868] = %d buff[1869] = %d\n", size,buff[1868],buff[1869]);
		return TRUE;
	}

	return FALSE;
}

static bool moto_read_mnf_data(MNF_INFO *mnf_info)
{
	unsigned short page = 0;
	unsigned short page_idx = 0;
	unsigned short start_addr = 0;
	unsigned int size = mnf_info->size;
	unsigned char *buff = mnf_info->data;
	PAGE_OFFSETINFO page_offset;
	PAGE_INFO *page_info = &(mnf_info->page_info);

	mot_select_vail_page(page_info);
	page_idx = page_info-> page_vail;
	if(DATA_PAGE_NUM > page_idx)
	{
		LOG_INF ("page index = %d flag = 0x%x\n", page_idx, page_info->flag);
		page = page_info->page[page_idx];
		page_offset = page_info->page_offset[page_idx];
		start_addr = page_info-> page_addr[page_idx];
		moto_read_sensor_otp_data(page,start_addr,buff,size);
		return TRUE;
	}

	return FALSE;
}

int s5k4h7_otp_data(void)
{
	MNF_INFO *mnf_info = &(otp_info_map.mnf_info);
	AWB_INFO *awb_info = &(otp_info_map.awb_info);
	AF_INFO  *af_info  = &(otp_info_map.af_info);
	LSC_INFO *lsc_info = &(otp_info_map.lsc_info);
	unsigned int size = 0;
	bool ret = FALSE;

	memset(otp_info_map.data,0,2048);

	mnf_info->data = otp_info_map.data;
	size = mnf_info->size;
	ret = moto_read_mnf_data(mnf_info);
	LOG_INF ("mnf ret %d size = %d\n", ret,size);

	awb_info->data = otp_info_map.data + size;
	size += awb_info->size;
	ret = moto_read_awb_data(awb_info);
	LOG_INF ("awb ret %d size = %d\n", ret,size);

	af_info->data = otp_info_map.data + size;
	size += af_info->size;
	ret = moto_read_af_data(af_info);
	LOG_INF ("af ret %d size = %d\n", ret,size);

	lsc_info->data = otp_info_map.data + size;
	size += lsc_info->size;
	ret = moto_read_lsc_data(lsc_info);
	LOG_INF ("lsc ret %d size = %d\n", ret,size);

	return 0;
}

unsigned int mot_s5k4h7_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
	unsigned char *otp_data = otp_info_map.data;
	LOG_INF ("eeprom read data addr = 0x%x size = %d\n",addr,size);
	memcpy(data, otp_data + addr,size);
	ret = size;
	return ret;
}

