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
#include "eeprom_i2c_s5k4h7_driver.h"

#define PFX "[MOTO_EEPROM s5k4h7-otp]"
static int m_mot_camera_debug = 1;
#define LOG_INF(format, args...)        do { if (m_mot_camera_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define LOG_DBG(format, args...)        do { if (m_mot_camera_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)

#define LOG_ERR(format, args...) pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)

#define GetBit(x,y)   ((x) >> (y) & 1)

#define CHECK_SNPRINTF_RET(ret, str, msg)           \
    if (ret < 0 || ret >= MAX_CALIBRATION_STRING) { \
        LOG_ERR(msg);                               \
        str[0] = 0;                                 \
    }

#define I2C_ADDR 0x5A
#define I2C_SPEED 400
#define S5K4H7_OTP_SIZE 6720
#define S5K4H7_OTP_DATA_PATH "/data/vendor/camera_dump/mot_gnevan_s5k4h7_otp.bin"
static OTP_INFO  otp_info_map = {
	.mnf_info.page_info.page[0] = 21,
	.mnf_info.page_info.page[1] = 26,
	.mnf_info.page_info.page[2] = 31,
	.mnf_info.page_info.page_offset[0] = {1,0,0},
	.mnf_info.page_info.page_offset[1] = {1,0,0},
	.mnf_info.page_info.page_offset[2] = {1,0,0},
	.mnf_info.page_info.page_addr[0] = 0x0A04,
	.mnf_info.page_info.page_addr[1] = 0x0A04,
	.mnf_info.page_info.page_addr[2] = 0x0A04,
	.mnf_info.size = 33,//include flag

	.awb_info.page_info.page[0] = 21,
	.awb_info.page_info.page[1] = 26,
	.awb_info.page_info.page[2] = 31,
	.awb_info.page_info.page_offset[0] = {2,8,51},
	.awb_info.page_info.page_offset[1] = {2,8,51},
	.awb_info.page_info.page_offset[2] = {2,8,51},
	.awb_info.page_info.page_addr[0] = 0x0A3C,
	.awb_info.page_info.page_addr[1] = 0x0A3C,
	.awb_info.page_info.page_addr[2] = 0x0A3C,
	.awb_info.size = 59,//include flag

	.lsc_info.page_info.page[0] = 36,
	.lsc_info.page_info.page[1] = 65,
	.lsc_info.page_info.page[2] = 94,
	.lsc_info.page_info.page_offset[0] = {31,64,15},
	.lsc_info.page_info.page_offset[1] = {31,49,30},
	.lsc_info.page_info.page_offset[2] = {31,34,45},
	.lsc_info.page_info.page_addr[0] = 0x0A04,
	.lsc_info.page_info.page_addr[1] = 0x0A13,
	.lsc_info.page_info.page_addr[2] = 0x0A22,
	.lsc_info.size = 1871,//include flag (lsc + crc +flag) 1868+2+1

	.light_source_info.page_info.page[0] = 21,
	.light_source_info.page_info.page[1] = 26,
	.light_source_info.page_info.page[2] = 31,
	.light_source_info.page_info.page_offset[0] = {0,0,0},
	.light_source_info.page_info.page_offset[1] = {0,0,0},
	.light_source_info.page_info.page_offset[2] = {0,0,0},
	.light_source_info.page_info.page_addr[0] = 0x0A25,
	.light_source_info.page_info.page_addr[1] = 0x0A25,
	.light_source_info.page_info.page_addr[2] = 0x0A25,
	.light_source_info.size = 23,//include flag

	.optical_center_info.page_info.page[0] = 22,
	.optical_center_info.page_info.page[1] = 27,
	.optical_center_info.page_info.page[2] = 32,
	.optical_center_info.page_info.page_offset[0] = {0,13,8},
	.optical_center_info.page_info.page_offset[1] = {0,13,8},
	.optical_center_info.page_info.page_offset[2] = {0,13,8},
	.optical_center_info.page_info.page_addr[0] = 0x0A37,
	.optical_center_info.page_info.page_addr[1] = 0x0A37,
	.optical_center_info.page_info.page_addr[2] = 0x0A37,
	.optical_center_info.size = 21,//include flag

	.sfr_distance_1_info.page_info.page[0] = 23,
	.sfr_distance_1_info.page_info.page[1] = 28,
	.sfr_distance_1_info.page_info.page[2] = 33,
	.sfr_distance_1_info.page_info.page_offset[0] = {0,0,0},
	.sfr_distance_1_info.page_info.page_offset[1] = {0,0,0},
	.sfr_distance_1_info.page_info.page_offset[2] = {0,0,0},
	.sfr_distance_1_info.page_info.page_addr[0] = 0x0A0C,
	.sfr_distance_1_info.page_info.page_addr[1] = 0x0A0C,
	.sfr_distance_1_info.page_info.page_addr[2] = 0x0A0C,
	.sfr_distance_1_info.size = 42,//include flag

	.sfr_distance_2_info.page_info.page[0] = 23,
	.sfr_distance_2_info.page_info.page[1] = 28,
	.sfr_distance_2_info.page_info.page[2] = 33,
	.sfr_distance_2_info.page_info.page_offset[0] = {0,14,28},
	.sfr_distance_2_info.page_info.page_offset[1] = {0,14,28},
	.sfr_distance_2_info.page_info.page_offset[2] = {0,14,28},
	.sfr_distance_2_info.page_info.page_addr[0] = 0x0A36,
	.sfr_distance_2_info.page_info.page_addr[1] = 0x0A36,
	.sfr_distance_2_info.page_info.page_addr[2] = 0x0A36,
	.sfr_distance_2_info.size = 42,//include flag

	.sfr_distance_3_info.page_info.page[0] = 24,
	.sfr_distance_3_info.page_info.page[1] = 29,
	.sfr_distance_3_info.page_info.page[2] = 34,
	.sfr_distance_3_info.page_info.page_offset[0] = {0,36,6},
	.sfr_distance_3_info.page_info.page_offset[1] = {0,36,6},
	.sfr_distance_3_info.page_info.page_offset[2] = {0,36,6},
	.sfr_distance_3_info.page_info.page_addr[0] = 0x0A20,
	.sfr_distance_3_info.page_info.page_addr[1] = 0x0A20,
	.sfr_distance_3_info.page_info.page_addr[2] = 0x0A20,
	.sfr_distance_3_info.size = 42,//include flag

	.mtk_module_info.page_info.page[0] = 124,
	.mtk_module_info.page_info.page[1] = 124,
	.mtk_module_info.page_info.page[2] = 124,
	.mtk_module_info.page_info.page_offset[0] = {0,0,0},
	.mtk_module_info.page_info.page_offset[1] = {0,0,0},
	.mtk_module_info.page_info.page_offset[2] = {0,20,2},
	.mtk_module_info.page_info.page_addr[0] = 0x0A04,
	.mtk_module_info.page_info.page_addr[1] = 0x0A1A,
	.mtk_module_info.page_info.page_addr[2] = 0x0A30,
	.mtk_module_info.size = 22,//include flag
};

mot_s5k4h7_otp_t mot_s5k4h7_otp = {{0}};

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

UINT16 read_uint16(UINT8 *reg)
{
    return (((UINT16)reg[0] << 8) | ((UINT16)reg[1]));
}


static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
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

mot_s5k4h7_otp_alldata_t mot_s5k4h7_otp_data = {{0}};
void moto_read_otp_all_data(void)
{
	unsigned short page_idx;
	unsigned short start_addr = OTP_PAGE_START_ADDR;
	LOG_INF("X");
	for(page_idx =21;page_idx <126;page_idx++)
	{
		moto_read_sensor_otp_data(page_idx,start_addr,mot_s5k4h7_otp_data.data+(page_idx-21)*64,64);
	}
	LOG_INF("E");
	return;
}
#if 0
void s5k4h7_otp_dump_bin(const void *data, uint32_t size)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(S5K4H7_OTP_DATA_PATH, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0666);
	if (IS_ERR_OR_NULL(fp)) {
		ret = PTR_ERR(fp);
		LOG_ERR("open file error(%s), error(%d)\n",  S5K4H7_OTP_DATA_PATH, ret);
		goto p_err;
	}

	ret = vfs_write(fp, (const char *)data, size, &fp->f_pos);
	if (ret < 0) {
		LOG_ERR("file write fail(%s) to EEPROM data(%d)", S5K4H7_OTP_DATA_PATH, ret);
		goto p_err;
	}

	LOG_INF("wirte to file(%s)\n", S5K4H7_OTP_DATA_PATH);
p_err:
	if (!IS_ERR_OR_NULL(fp))
		filp_close(fp, NULL);

	set_fs(old_fs);
	LOG_INF(" end writing file");
}
#endif
//get vail page flag
static void mot_select_vail_page(PAGE_INFO *pg_inf)
{
	int i = 0;
	unsigned short page =0;
	unsigned short start_addr =0;
	unsigned char page_flag;

	pg_inf-> page_vail = 0;
	pg_inf-> flag = PAGE_INVAIL;

	for( i=0; i< DATA_PAGE_NUM; i++)
	{
		page = pg_inf-> page[i];
		start_addr = pg_inf-> page_addr[i];
		moto_read_sensor_otp_data(page,start_addr,&page_flag,1);
		LOG_INF("page %d flag %d%d .", page,GetBit(page_flag, 7),GetBit(page_flag, 6));
		if (GetBit(page_flag, 7) == 0 && GetBit(page_flag, 6) == 1)
		{
			pg_inf-> flag = PAGE_VAIL;
			LOG_INF("page %d is valid.", page);
			break;
		}
	}

	pg_inf-> page_vail = i;
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
		start_addr = page_info->page_addr[page_idx];
		moto_read_sensor_otp_data(page,start_addr,buff,page_offset.page_sart_size);
		moto_read_sensor_otp_data(page+1,OTP_PAGE_START_ADDR,
				buff+page_offset.page_sart_size,page_offset.page_end_size);

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
		for(i = 0; i < page_offset.page_num; i++)
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

static bool moto_read_common_one_page_data(COMMON_PAGE_INFO *common_page_info)
{
	unsigned short page = 0;
	unsigned short page_idx = 0;
	unsigned short start_addr = 0;
	unsigned int size = common_page_info->size;
	unsigned char *buff = common_page_info->data;
	PAGE_INFO *page_info = &(common_page_info->page_info);

	mot_select_vail_page(page_info);
	page_idx = page_info-> page_vail;
	if(DATA_PAGE_NUM > page_idx)
	{
		page = page_info->page[page_idx];
		LOG_INF("page index = %d, page = %d, flag = 0x%x\n", page_idx, page, page_info->flag);

		start_addr = page_info-> page_addr[page_idx];
		moto_read_sensor_otp_data(page,start_addr,buff,size);
		return TRUE;
	}

	return FALSE;
}

static bool moto_read_common_two_page_data(COMMON_PAGE_INFO *common_two_page_info)
{
	unsigned short page = 0;
	unsigned short page_idx = 0;
	unsigned short start_addr = 0;
	unsigned char *buff = common_two_page_info->data;
	PAGE_OFFSETINFO page_offset;
	PAGE_INFO *page_info = &(common_two_page_info->page_info);

	mot_select_vail_page(page_info);
	page_idx = page_info-> page_vail;
	if(DATA_PAGE_NUM > page_idx)
	{
		page = page_info->page[page_idx];
		LOG_INF("page index = %d, page = %d, flag = 0x%x\n", page_idx, page, page_info->flag);

		page_offset = page_info->page_offset[page_idx];
		start_addr = page_info-> page_addr[page_idx];

		// read the first page data
		moto_read_sensor_otp_data(page, start_addr, buff, page_offset.page_sart_size);

		// read remain of data in the next page
		moto_read_sensor_otp_data(page+1, OTP_PAGE_START_ADDR, buff+page_offset.page_sart_size, page_offset.page_end_size);

		return TRUE;
	}

	return FALSE;
}

static bool moto_read_one_or_two_page_data(COMMON_PAGE_INFO *common_page_info)
{
	unsigned short page = 0;
	unsigned short page_idx = 0;
	unsigned short start_addr = 0;
	unsigned int size = common_page_info->size;
	unsigned char *buff = common_page_info->data;
	PAGE_OFFSETINFO page_offset;
	PAGE_INFO *page_info = &(common_page_info->page_info);

	mot_select_vail_page(page_info);
	page_idx = page_info-> page_vail;
	if(DATA_PAGE_NUM > page_idx)
	{
		page = page_info->page[page_idx];
		page_offset = page_info->page_offset[page_idx];
		start_addr = page_info-> page_addr[page_idx];
		LOG_INF("page index = %d, page = %d, flag = 0x%x, page_sart_size = %d, page_end_size = %d\n",
			page_idx, page, page_info->flag, page_offset.page_sart_size, page_offset.page_end_size);

		if ( page_offset.page_sart_size != 0 &&
			page_offset.page_end_size != 0)
		{
			LOG_INF("all data are in two different page.");

			// read the first page data
			moto_read_sensor_otp_data(page, start_addr, buff, page_offset.page_sart_size);

			// read remain of data in the next page
			moto_read_sensor_otp_data(page+1, OTP_PAGE_START_ADDR, buff+page_offset.page_sart_size, page_offset.page_end_size);
			return TRUE;
		}
		else
		{
			LOG_INF("all data are in one page.");
			moto_read_sensor_otp_data(page,start_addr,buff,size);
			return TRUE;
		}
	}

	return FALSE;
}

int s5k4h7_read_otp_data(void)
{
	MNF_INFO *mnf_info = &(otp_info_map.mnf_info);
	AWB_INFO *awb_info = &(otp_info_map.awb_info);
	LSC_INFO *lsc_info = &(otp_info_map.lsc_info);

	COMMON_PAGE_INFO *light_source_info   = &(otp_info_map.light_source_info);
	COMMON_PAGE_INFO *optical_center_info = &(otp_info_map.optical_center_info);
	COMMON_PAGE_INFO *sfr_distance_1_info = &(otp_info_map.sfr_distance_1_info);
	COMMON_PAGE_INFO *sfr_distance_2_info = &(otp_info_map.sfr_distance_2_info);
	COMMON_PAGE_INFO *sfr_distance_3_info = &(otp_info_map.sfr_distance_3_info);
	COMMON_PAGE_INFO *mtk_module_info     = &(otp_info_map.mtk_module_info);

	unsigned int size = 0;
	bool ret = FALSE;

	memset(otp_info_map.data,0,S5K4H7_EEPROM_MAX_SIZE);

	mnf_info->data = otp_info_map.data;
	size = mnf_info->size;
	ret = moto_read_mnf_data(mnf_info);
	memcpy(mot_s5k4h7_otp.program_flag,
		mnf_info->data,
		(mot_s5k4h7_otp.light_source_flag - mot_s5k4h7_otp.program_flag)/sizeof(UINT8));
	LOG_INF ("mnf ret %d size = %d\n", ret,size);

	awb_info->data = otp_info_map.data + size;
	size += awb_info->size;
	ret = moto_read_awb_data(awb_info);
	memcpy(mot_s5k4h7_otp.wb_flag,
		awb_info->data,
		(mot_s5k4h7_otp.oc_flag - mot_s5k4h7_otp.wb_flag)/sizeof(UINT8));
	LOG_INF ("awb ret %d size = %d\n", ret,size);

	lsc_info->data = otp_info_map.data + size;
	size += lsc_info->size;
	ret = moto_read_lsc_data(lsc_info);
	memcpy(mot_s5k4h7_otp.lsc_flag,
		lsc_info->data,
		(mot_s5k4h7_otp.mtk_info_flag - mot_s5k4h7_otp.lsc_flag)/sizeof(UINT8));
	LOG_INF ("lsc ret %d size = %d\n", ret,size);

	light_source_info->data = otp_info_map.data + size;
	size += light_source_info->size;
	ret = moto_read_common_one_page_data(light_source_info);
	memcpy(mot_s5k4h7_otp.light_source_flag,
		light_source_info->data,
		(mot_s5k4h7_otp.wb_flag - mot_s5k4h7_otp.light_source_flag)/sizeof(UINT8));
	LOG_INF ("light_source_info ret %d size = %d\n", ret,size);

	optical_center_info->data = otp_info_map.data + size;
	size += optical_center_info->size;
	ret = moto_read_common_two_page_data(optical_center_info);
	memcpy(mot_s5k4h7_otp.oc_flag,
		optical_center_info->data,
		(mot_s5k4h7_otp.sfr_dist1_flag - mot_s5k4h7_otp.oc_flag)/sizeof(UINT8));
	LOG_INF ("optical_center_info ret %d size = %d\n", ret,size);

	sfr_distance_1_info->data = otp_info_map.data + size;
	size += sfr_distance_1_info->size;
	ret = moto_read_common_one_page_data(sfr_distance_1_info);
	memcpy(mot_s5k4h7_otp.sfr_dist1_flag,
		sfr_distance_1_info->data,
		(mot_s5k4h7_otp.sfr_dist2_flag - mot_s5k4h7_otp.sfr_dist1_flag)/sizeof(UINT8));
	LOG_INF ("sfr_distance_1_info ret %d size = %d\n", ret,size);

	sfr_distance_2_info->data = otp_info_map.data + size;
	size += sfr_distance_2_info->size;
	ret = moto_read_common_two_page_data(sfr_distance_2_info);
	memcpy(mot_s5k4h7_otp.sfr_dist2_flag,
		sfr_distance_2_info->data,
		(mot_s5k4h7_otp.sfr_dist3_flag - mot_s5k4h7_otp.sfr_dist2_flag)/sizeof(UINT8));
	LOG_INF ("sfr_distance_2_info ret %d size = %d\n", ret,size);

	sfr_distance_3_info->data = otp_info_map.data + size;
	size += sfr_distance_3_info->size;
	ret = moto_read_common_one_page_data(sfr_distance_3_info);
	memcpy(mot_s5k4h7_otp.sfr_dist3_flag,
		sfr_distance_3_info->data,
		(mot_s5k4h7_otp.lsc_flag - mot_s5k4h7_otp.sfr_dist3_flag)/sizeof(UINT8));
	LOG_INF ("sfr_distance_3_info ret %d size = %d\n", ret,size);

	mtk_module_info->data = otp_info_map.data + size;
	size += mtk_module_info->size;
	ret = moto_read_one_or_two_page_data(mtk_module_info);
	memcpy(mot_s5k4h7_otp.mtk_info_flag,
		mtk_module_info->data,
		(mot_s5k4h7_otp.mtk_info_crc + 2 - mot_s5k4h7_otp.mtk_info_flag)/sizeof(UINT8));
	LOG_INF ("mtk_module_info ret %d size = %d\n", ret,size);
	moto_read_otp_all_data();
	//s5k4h7_otp_dump_bin(mot_s5k4h7_otp_data.data, S5K4H7_OTP_SIZE);
	return 0;
}

unsigned int s5k4h7_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size)
{
	unsigned int ret = 0;
	unsigned int dump_all_size = 6720;
	unsigned char *otp_data = otp_info_map.data;
	unsigned char *otp_alldata = mot_s5k4h7_otp_data.data;
	LOG_INF ("eeprom read data addr = 0x%x size = %d\n",addr,size);
       if (size !=dump_all_size) {
	memcpy(data, otp_data + addr,size);
        } else {
	memcpy(data, otp_alldata,size);
        }
	ret = size;
	return ret;
}
EXPORT_SYMBOL(s5k4h7_read_region);
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

static void s5k4h7_check_lsc_data(mot_calibration_info_t * pOtpCalInfo)
{
	mot_s5k4h7_otp_t *pOTP  = &mot_s5k4h7_otp;

	pOtpCalInfo->lsc_status = STATUS_OK;

	if (!eeprom_util_check_crc16(pOTP->lsc_flag,
					(mot_s5k4h7_otp.lsc_crc - mot_s5k4h7_otp.lsc_flag)/sizeof(UINT8),
					read_uint16(pOTP->lsc_crc)))
	{
		LOG_ERR("LSC CRC Fails!");
		pOtpCalInfo->lsc_status = STATUS_CRC_FAIL;
		return;
	}
	else
		LOG_INF("LSC CRC Pass");
}

static void s5k4h7_check_awb_data(mot_calibration_info_t * pOtpCalInfo)
{
	mot_s5k4h7_otp_t *pOTP  = &mot_s5k4h7_otp;

	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;

	pOtpCalInfo->awb_status = STATUS_OK;

	if (!eeprom_util_check_crc16(pOTP->light_source_flag,
					(mot_s5k4h7_otp.light_source_crc - mot_s5k4h7_otp.light_source_flag)/sizeof(UINT8),
					read_uint16(pOTP->light_source_crc)))
	{
		LOG_ERR("Light Source CRC Fails!");
		pOtpCalInfo->awb_status = STATUS_CRC_FAIL;
		return;
	}
	else
		LOG_INF("Light Source CRC Pass");

	if (!eeprom_util_check_crc16(pOTP->wb_flag,
					(mot_s5k4h7_otp.wb_crc - mot_s5k4h7_otp.wb_flag)/sizeof(UINT8),
					read_uint16(pOTP->wb_crc)))
	{
		LOG_ERR("AWB CRC Fails!");
		pOtpCalInfo->awb_status = STATUS_CRC_FAIL;
		return;
	}
	else
		LOG_INF("AWB CRC Pass");

	unit.r     = read_uint16(pOTP->wb_unit_r);
	unit.gr    = read_uint16(pOTP->wb_unit_gr);
	unit.gb    = read_uint16(pOTP->wb_unit_gb);
	unit.b     = read_uint16(pOTP->wb_unit_b);
	unit.r_g   = read_uint16(pOTP->wb_unit_r_g_ratio);
	unit.b_g   = read_uint16(pOTP->wb_unit_b_g_ratio);
	unit.gr_gb = read_uint16(pOTP->wb_unit_gr_gb_ratio);

	golden.r     = read_uint16(pOTP->wb_golden_r);
	golden.gr    = read_uint16(pOTP->wb_golden_gr);
	golden.gb    = read_uint16(pOTP->wb_golden_gb);
	golden.b     = read_uint16(pOTP->wb_golden_b);
	golden.r_g   = read_uint16(pOTP->wb_golden_r_g_ratio);
	golden.b_g   = read_uint16(pOTP->wb_golden_b_g_ratio);
	golden.gr_gb = read_uint16(pOTP->wb_golden_gr_gb_ratio);

	if (mot_eeprom_util_check_awb_limits(unit, golden))
	{
		LOG_ERR("AWB CRC limit Fails!");
		pOtpCalInfo->awb_status = STATUS_LIMIT_FAIL;
		return;
	}

	golden_limit.r_g_golden_min = pOTP->wb_limit_range_rg_lower[0];
	golden_limit.r_g_golden_max = pOTP->wb_limit_range_rg_upper[0];
	golden_limit.b_g_golden_min = pOTP->wb_limit_range_bg_lower[0];
	golden_limit.b_g_golden_max = pOTP->wb_limit_range_bg_upper[0];

	if (mot_eeprom_util_calculate_awb_factors_limit(unit, golden, golden_limit))
	{
		LOG_ERR("AWB CRC factor limit Fails!");
		pOtpCalInfo->awb_status = STATUS_LIMIT_FAIL;
		return;
	}

	LOG_INF("AWB Limit Pass");
}

static void s5k4h7_check_mnf_data(mot_calibration_info_t * pOtpCalInfo)
{
	int ret = 0;
	mot_s5k4h7_otp_t *pOTP  = &mot_s5k4h7_otp;
	mot_calibration_mnf_t * pMnfData = &pOtpCalInfo->mnf_cal_data;

	pOtpCalInfo->mnf_status = STATUS_OK;

	//eeprom_dump_bin(EEPROM_DATA_PATH, sizeof(mot_s5k4h7_otp_t), pOTP->program_flag);
	//eeprom_dump_bin(SERIAL_DATA_PATH, DUMP_WIDE_SERIAL_NUMBER_SIZE, pOTP->serial_number);

	if (!eeprom_util_check_crc16(pOTP->program_flag,
					(mot_s5k4h7_otp.mnf_info_crc - mot_s5k4h7_otp.program_flag)/sizeof(UINT8),
					read_uint16(pOTP->mnf_info_crc)))
	{
		LOG_ERR("Manufacturing CRC Fails!");
		pOtpCalInfo->mnf_status = STATUS_CRC_FAIL;
		return;
	}
	else
		LOG_INF("Manufacturing CRC Pass");

	// tableRevision
	ret = snprintf(pMnfData->table_revision, MAX_CALIBRATION_STRING, "0x%x", pOTP->table_revision[0]);
	CHECK_SNPRINTF_RET(ret, pMnfData->table_revision, "failed to fill table revision string");

	// moto part number
	ret = snprintf(pMnfData->mot_part_number, MAX_CALIBRATION_STRING, "SC%c%c%c%c%c%c%c%c",
			pOTP->mot_part_number[0], pOTP->mot_part_number[1],
			pOTP->mot_part_number[2], pOTP->mot_part_number[3],
			pOTP->mot_part_number[4], pOTP->mot_part_number[5],
			pOTP->mot_part_number[6], pOTP->mot_part_number[7]);
	CHECK_SNPRINTF_RET(ret, pMnfData->mot_part_number, "failed to fill part number string");

	// actuator ID
	switch (pOTP->actuator_id[0])
	{
		case 0xFF: ret = snprintf(pMnfData->actuator_id, MAX_CALIBRATION_STRING, "N/A");	break;
		case 0x30: ret = snprintf(pMnfData->actuator_id, MAX_CALIBRATION_STRING, "Dongwoo");	break;
		default:   ret = snprintf(pMnfData->actuator_id, MAX_CALIBRATION_STRING, "Unknown");	break;
	}
	CHECK_SNPRINTF_RET(ret, pMnfData->actuator_id, "failed to fill actuator id string");

	// lens ID
	switch (pOTP->lens_id[0])
	{
		case 0x00: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "XuYe");		break;
		case 0x07: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Sunny 38127A-400");break;
		case 0x20: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Sunnys");		break;
		case 0x29: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Sunny 39292A-400");break;
		case 0x40: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Largan");		break;
		case 0x42: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Largan 50281A3");	break;
		case 0x60: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "SEMCO");		break;
		case 0x80: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Genius");		break;
		case 0x84: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "AAC 325174A01");	break;
		case 0xA0: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Sunnys");		break;
		case 0xA1: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Sunnys 39390A-400");break;
		case 0xC0: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "AAC");		break;
		case 0xE0: ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Kolen");		break;
		default:   ret = snprintf(pMnfData->lens_id, MAX_CALIBRATION_STRING, "Unknown");	break;
	}
	CHECK_SNPRINTF_RET(ret, pMnfData->lens_id, "failed to fill lens id string");

	// manufacturer ID
	if (pOTP->manufacturer_id[0] == 'S' && pOTP->manufacturer_id[1] == 'U')
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Sunny");
	else if (pOTP->manufacturer_id[0] == 'O' && pOTP->manufacturer_id[1] == 'F')
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "OFilm");
	else if (pOTP->manufacturer_id[0] == 'H' && pOTP->manufacturer_id[1] == 'O')
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Holitech");
	else if (pOTP->manufacturer_id[0] == 'T' && pOTP->manufacturer_id[1] == 'S')
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Tianshi");
	else if (pOTP->manufacturer_id[0] == 'S' && pOTP->manufacturer_id[1] == 'W')
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Sunwin");
	else if (pOTP->manufacturer_id[0] == 'S' && pOTP->manufacturer_id[1] == 'E')
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Semco");
	else if (pOTP->manufacturer_id[0] == 'Q' && pOTP->manufacturer_id[1] == 'T')
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Qtech");
	else
		ret = snprintf(pMnfData->integrator, MAX_CALIBRATION_STRING, "Unknown");

	CHECK_SNPRINTF_RET(ret, pMnfData->integrator, "failed to fill integrator string");

	// factory ID
	ret = snprintf(pMnfData->factory_id, MAX_CALIBRATION_STRING, "%c%c",
			pOTP->factory_id[0], pOTP->factory_id[1]);
	CHECK_SNPRINTF_RET(ret, pMnfData->factory_id, "failed to fill factory id string");

	// manufacture line
	ret = snprintf(pMnfData->manufacture_line, MAX_CALIBRATION_STRING, "%u", pOTP->manufacture_line[0]);
	CHECK_SNPRINTF_RET(ret, pMnfData->manufacture_line, "failed to fill manufature line string");

	// manufacture date
	ret = snprintf(pMnfData->manufacture_date, MAX_CALIBRATION_STRING, "20%u/%u/%u",
			pOTP->manufacture_date[0], pOTP->manufacture_date[1], pOTP->manufacture_date[2]);
	CHECK_SNPRINTF_RET(ret, pMnfData->manufacture_date, "failed to fill manufature date");

	// serialNumber
	ret = snprintf(pMnfData->serial_number, MAX_CALIBRATION_STRING, "%02x%02x%02x%02x%02x%02x%02x%02x",
		pOTP->serial_number[0],  pOTP->serial_number[1],
		pOTP->serial_number[2],  pOTP->serial_number[3],
		pOTP->serial_number[4],  pOTP->serial_number[5],
		pOTP->serial_number[6],  pOTP->serial_number[7]);
	CHECK_SNPRINTF_RET(ret, pMnfData->serial_number, "failed to fill serial number");

	LOG_INF("integrator: %s, lens_id: %s, actuator_id: %s, manufacture_date: %s, serial_number: %s",
		pMnfData->integrator, pMnfData->lens_id, pMnfData->actuator_id,
		pMnfData->manufacture_date, pMnfData->serial_number);
}

int s5k4h7_get_cal_info(mot_calibration_info_t * pOtpCalInfo)
{
	s5k4h7_check_mnf_data(pOtpCalInfo);
	s5k4h7_check_awb_data(pOtpCalInfo);
	s5k4h7_check_lsc_data(pOtpCalInfo);

	LOG_INF("status mnf:%d, awb:%d, lsc:%d", pOtpCalInfo->mnf_status, pOtpCalInfo->awb_status, pOtpCalInfo->lsc_status);
	return 0;
}
