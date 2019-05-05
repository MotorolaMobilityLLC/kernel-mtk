/*
 * Copyright (C) 2015 MediaTek Inc.
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
#include "s5k4h7yx_sunwin_p161m_mipi_raw_Sensor.h"

#define USHORT             unsigned short
#define BYTE               unsigned char
#define S5K4H7SUB_OTP_LSCFLAG_ADDR	0x0A3D
#define S5K4H7SUB_OTP_FLAGFLAG_ADDR	0x0A04


#define S5K4H7SUB_OTP_AWBFLAG_ADDR	0x0A0D
#define S5K4H7SUB_LSC_PAGE		0
#define S5K4H7SUB_FLAG_PAGE		21
#define S5K4H7SUB_AWB_PAGE		22
//Chenyee camera pangfei 20180608 modify for CSW1802A-9 begin
#define S5K4H7SUB_OTP_FLAGFLAG_ADDR_G2	0x0A24

#define G_G_GOLDEN (unsigned int)(512+0.5) //1024
#define R_G_GOLDEN (unsigned int)(2*88.0/(156+156)*512+0.5)//1600//0x020d*5
#define B_G_GOLDEN (unsigned int)(2*111.0/(156+156)*512+0.5) //0x0297*5
//Chenyee camera pangfei 20180608 modify for CSW1802A-9 end

extern unsigned int sensor_module_id;

typedef struct {
	unsigned short	infoflag;
	unsigned short	lsc_infoflag;
	unsigned short	flag_infoflag;
	unsigned short	flag_module_integrator_id;
	int		awb_offset;
	int		flag_offset;
	int		lsc_offset;
	int 		lsc_group;
	int 		flag_group;
	int 		group;
	unsigned short	frgcur;
	unsigned short	fbgcur;
	unsigned int	nr_gain;
	unsigned int	ng_gain;
	unsigned int	nb_gain;
	unsigned int	ngrcur;
	unsigned int	ngbcur;
	unsigned int	ngcur;
	unsigned int	nrcur;
	unsigned int	nbcur;
	unsigned int	nggolden;
	unsigned int	nrgolden;
	unsigned int	nbgolden;
	unsigned int	ngrgolden;
	unsigned int	ngbgolden;
	unsigned int	frggolden;
	unsigned int	fbggolden;
	unsigned int	awb_flag_sum;
	unsigned int	lsc_sum;
	unsigned int	lsc_check_flag;
}OTP;

OTP otp_data_info = {0};

/**********************************************************
 * get_4h7_page_data
 * get page data
 * return true or false
 * *******************************************************/
void get_4h7_page_data(int pageidx, unsigned char *pdata)
{
	unsigned short get_byte=0;
	unsigned int addr = 0x0A05;
	int i = 0;
	otp_4h7_write_cmos_sensor_8(0x0A02,pageidx);
	otp_4h7_write_cmos_sensor_8(0x0A00,0x01);

	do
	{
		mdelay(1);
		get_byte = otp_4h7_read_cmos_sensor(0x0A01);
	}while((get_byte & 0x01) != 1);

	for(i = 0; i < 20; i++){
		pdata[i] = otp_4h7_read_cmos_sensor(addr);
		addr++;
        printk("OTP  otp_4h7_read_cmos_sensor pdata[%d]=0x%x",i,pdata[i]);
	}

	otp_4h7_write_cmos_sensor_8(0x0A00,0x00);
}

void get_4h7_page_data_lsc(int pageidx, unsigned char *pdata)
{
	unsigned short get_byte=0;
	unsigned int addr = 0x0A04;
	int i = 0;
	otp_4h7_write_cmos_sensor_8(0x0A02,pageidx);
	otp_4h7_write_cmos_sensor_8(0x0A00,0x01);

	do
	{
		mdelay(1);
		get_byte = otp_4h7_read_cmos_sensor(0x0A01);
	}while((get_byte & 0x01) != 1);

	for(i = 0; i < 64; i++){
		pdata[i] = otp_4h7_read_cmos_sensor(addr);
		addr++;
               printk("OTP  otp_4h7_read_cmos_sensor_lsc pdata[%d]=0x%x",i,pdata[i]);
	}

	otp_4h7_write_cmos_sensor_8(0x0A00,0x00);
}


unsigned short selective_read_region(int pageidx,unsigned int addr)
{
	unsigned short get_byte = 0;
	otp_4h7_write_cmos_sensor_8(0x0A02,pageidx);
	otp_4h7_write_cmos_sensor_8(0x0A00,0x01);
	do
	{
		mdelay(1);
		get_byte = otp_4h7_read_cmos_sensor(0x0A01);
	}while((get_byte & 0x01) != 1);

	get_byte = otp_4h7_read_cmos_sensor(addr);
	otp_4h7_write_cmos_sensor_8(0x0A00,0x00);

	return get_byte;
}


unsigned short selective_read_region_16(int pageidx,unsigned int addr)
{
	unsigned short get_byte = 0;
	otp_4h7_write_cmos_sensor_8(0x0A02,pageidx);
	otp_4h7_write_cmos_sensor_8(0x0A00,0x01);
	do
	{
		mdelay(1);
		get_byte = otp_4h7_read_cmos_sensor(0x0A01);
	}while((get_byte & 0x01) != 1);

	get_byte = ((otp_4h7_read_cmos_sensor(addr) << 8) | otp_4h7_read_cmos_sensor(addr+1));
	printk("--OTP  selective_read_region_16  addr=0x%x  addr+1=0x%x   get_byte=0x%x  addr=0x%x pageidx=%d" ,otp_4h7_read_cmos_sensor(addr),otp_4h7_read_cmos_sensor(addr+1),get_byte,addr, pageidx);

	otp_4h7_write_cmos_sensor_8(0x0A00,0x00);

	return get_byte;
}

#if 0
unsigned int selective_read_region_16(int pageidx,unsigned int addr)
{
	unsigned int get_byte = 0;
	static int old_pageidx = 0;
	if(pageidx != old_pageidx){
		otp_4h7_write_cmos_sensor_8(0x0A00,0x00);
		otp_4h7_write_cmos_sensor_8(0x0A02,pageidx);
		otp_4h7_write_cmos_sensor_8(0x0A00,0x01);
		do
		{
			mdelay(1);
			get_byte = otp_4h7_read_cmos_sensor(0x0A01);
		}while((get_byte & 0x01) != 1);
	}

	get_byte = ((otp_4h7_read_cmos_sensor(addr) << 8) | otp_4h7_read_cmos_sensor(addr+1));

	printk("------OTP  selective_read_region_16  addr=0x%x  addr+1=0x%x   get_byte=0x%x  addr=0x%x pageidx=%d" ,otp_4h7_read_cmos_sensor(addr),otp_4h7_read_cmos_sensor(addr+1),get_byte,addr, pageidx);

	old_pageidx = pageidx;
	return get_byte;
}

#endif


/*****************************************************
 * cal_rgb_gain
 * **************************************************/
void cal_rgb_gain(int* r_gain, int* g_gain, int* b_gain, unsigned int r_ration, unsigned int b_ration)
{
	int gain_default = 0x0100;
	if(r_ration >= 1){
		if(b_ration >= 1){
			*g_gain = gain_default;
			*r_gain = (int)((gain_default*1000 * r_ration + 500)/1000);
			*b_gain = (int)((gain_default*1000 * b_ration + 500)/1000);
		}
		else{
			*b_gain = gain_default;
			*g_gain = (int)((gain_default * 1000 / b_ration + 500)/1000);
			*r_gain = (int)((gain_default * r_ration *1000 / b_ration + 500)/1000);
		}
	}
	else{
		if(b_ration >= 1){
			*r_gain = gain_default;
			*g_gain = (int)((gain_default * 1000 / r_ration + 500)/1000);
			*b_gain = (int)((gain_default * b_ration*1000 / r_ration + 500) / 1000);
		}
		else{
			if(r_ration >= b_ration){
				*b_gain = gain_default;
				*g_gain = (int)((gain_default * 1000 / b_ration + 500) / 1000);
				*r_gain = (int)((gain_default * r_ration * 1000 / b_ration + 500) / 1000);
			}
			else{
				*r_gain = gain_default;
				*g_gain = (int)((gain_default * 1000 / r_ration + 500)/1000);
				*b_gain = (int)((gain_default * b_ration * 1000 / r_ration + 500) / 1000);
			}
		}
	}
}

/**********************************************************
 * apply_4h7_otp_awb
 * apply otp
 * *******************************************************/
void apply_4h7_otp_awb(void)
{
	char r_gain_h, r_gain_l, g_gain_h, g_gain_l, b_gain_h, b_gain_l;
	unsigned int r_ratio, b_ratio;
//Chenyee camera pangfei 20180608 modify for CSW1802A-9 begin
#ifdef ORIGINAL
	otp_data_info.ngcur = (unsigned int)((otp_data_info.ngrcur + otp_data_info.ngbcur)*1000/2 + 500);

	otp_data_info.frgcur = (unsigned int)(otp_data_info.nrcur*1000 / otp_data_info.ngcur + 500);
	otp_data_info.fbgcur = (unsigned int)(otp_data_info.nbcur*1000 / otp_data_info.ngcur + 500);

	otp_data_info.nggolden = G_G_GOLDEN;//(unsigned int)((otp_data_info.ngrgolden + otp_data_info.ngbgolden)*1000 / 2 + 500);

	otp_data_info.frggolden = R_G_GOLDEN;//(unsigned int)(otp_data_info.nrgolden*1000/ otp_data_info.nggolden +500);
	otp_data_info.fbggolden = B_G_GOLDEN;//(unsigned int)(otp_data_info.nbgolden*1000/ otp_data_info.nggolden +500);
#else
	//otp_data_info.ngcur = (unsigned int)((otp_data_info.ngrcur + otp_data_info.ngbcur)*1000/2 + 500);

	//otp_data_info.frgcur = (unsigned int)(otp_data_info.nrcur*1000 / otp_data_info.ngcur + 500);
	//otp_data_info.fbgcur = (unsigned int)(otp_data_info.nbcur*1000 / otp_data_info.ngcur + 500);

	otp_data_info.nggolden = G_G_GOLDEN;//(unsigned int)((otp_data_info.ngrgolden + otp_data_info.ngbgolden)*1000 / 2 + 500);

	otp_data_info.frggolden = R_G_GOLDEN;//(unsigned int)(otp_data_info.nrgolden*1000/ otp_data_info.nggolden +500);
	otp_data_info.fbggolden = B_G_GOLDEN;//(unsigned int)(otp_data_info.nbgolden*1000/ otp_data_info.nggolden +500);
#endif
//Chenyee camera pangfei 20180608 modify for CSW1802A-9 end

	r_ratio = (unsigned int)((otp_data_info.frggolden * 1000 / otp_data_info.frgcur + 500)/1000);
	b_ratio = (unsigned int)((otp_data_info.fbggolden * 1000 / otp_data_info.fbgcur + 500)/1000);

        printk("OTP apply_4h7_otp_awb  otp_data_info.frggolden=0x%x  tp_data_info.fbggolden=0x%x  tp_data_info.frgcur=0x%x  otp_data_info.fbgcur=0x%x  r_ratio=0x%x  b_ratio=0x%x \n",otp_data_info.frggolden,otp_data_info.fbggolden,otp_data_info.frgcur,otp_data_info.fbgcur,r_ratio,b_ratio);

	cal_rgb_gain(&otp_data_info.nr_gain, &otp_data_info.ng_gain, &otp_data_info.nb_gain, r_ratio, b_ratio);

	r_gain_h = (otp_data_info.nr_gain >> 8) & 0xff;
	r_gain_l = (otp_data_info.nr_gain >> 0) & 0xff;

	g_gain_h = (otp_data_info.ng_gain >> 8) & 0xff;
	g_gain_l = (otp_data_info.ng_gain >> 0) & 0xff;

	b_gain_h = (otp_data_info.nb_gain >> 8) & 0xff;
	b_gain_l = (otp_data_info.nb_gain >> 0) & 0xff;
	
	otp_4h7_write_cmos_sensor_8(0x3C0F,0x00);

	otp_4h7_write_cmos_sensor_8(0x0210,r_gain_h);
	otp_4h7_write_cmos_sensor_8(0x0211,r_gain_l);

	otp_4h7_write_cmos_sensor_8(0x020E,g_gain_h);
	otp_4h7_write_cmos_sensor_8(0x020F,g_gain_l);

	otp_4h7_write_cmos_sensor_8(0x0214,g_gain_h);
	otp_4h7_write_cmos_sensor_8(0x0215,g_gain_l);

	otp_4h7_write_cmos_sensor_8(0x0212,b_gain_h);
	otp_4h7_write_cmos_sensor_8(0x0213,b_gain_l);
	printk("OTP    apply_4h7_otp_awb  r_gain_h=0x%x  r_gain_l=0x%x  g_gain_h=0x%x  g_gain_l=0x%x  b_gain_h=0x%x  b_gain_l=0x%x \n",r_gain_h,r_gain_l,g_gain_h,g_gain_l,b_gain_h,b_gain_l);
}

/*********************************************************
 *apply_4h7_otp_lsc
 * ******************************************************/

void apply_4h7_otp_enb_lsc(void)
{
	printk("OTP enable lsc\n");
	otp_4h7_write_cmos_sensor_8(0x0B00,0x01);
	otp_4h7_write_cmos_sensor_8(0x3400,0x00);
}

/*********************************************************
 * otp_group_info_4h7
 * *****************************************************/
//Chenyee camera pangfei 20180608 modify for CSW1802A-9 begin
#ifdef ORIGINAL
int otp_group_info_4h7(void)
{
	memset(&otp_data_info,0,sizeof(OTP));

	otp_data_info.lsc_infoflag =
		selective_read_region(S5K4H7SUB_LSC_PAGE,S5K4H7SUB_OTP_LSCFLAG_ADDR);

	if( otp_data_info.lsc_infoflag == 0x01 ){
		otp_data_info.lsc_offset = 0;
		otp_data_info.lsc_group = 1;
		otp_data_info.lsc_sum = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A32);
                printk("4H7 OTP otp_data_info.lsc_group = 1;\n");
	}
	else if ( otp_data_info.lsc_infoflag == 0x03 ){
		otp_data_info.lsc_offset = 1;
		otp_data_info.lsc_group = 2;
		otp_data_info.lsc_sum = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A34);
                printk("otp_data_info.lsc_group = 2;\n");
	}
	else{
		printk("4H7 OTP read data fail lsc empty!!!\n");
		//goto error;
	}

	otp_data_info.flag_infoflag =
		selective_read_region(S5K4H7SUB_FLAG_PAGE,S5K4H7SUB_OTP_FLAGFLAG_ADDR);

	if( (otp_data_info.flag_infoflag>>4 & 0x0c) == 0x04 ){
		otp_data_info.flag_offset = 0;
		otp_data_info.flag_group = 1;
	}
	else if ( (otp_data_info.flag_infoflag>>4 & 0x03) == 0x01 ){
		otp_data_info.flag_offset = 4;
		otp_data_info.flag_group = 2;
	}
	else{
		printk("4h7 OTP read data fail flag empty!!!\n");
		//goto error;
	}

	otp_data_info.flag_module_integrator_id =
		otp_4h7_read_cmos_sensor(S5K4H7SUB_OTP_FLAGFLAG_ADDR + otp_data_info.flag_offset + 1);

	otp_data_info.infoflag = selective_read_region(S5K4H7SUB_AWB_PAGE,S5K4H7SUB_OTP_AWBFLAG_ADDR);

	otp_data_info.nrcur = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A0E);
	otp_data_info.nbcur = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A10);
	otp_data_info.ngrcur = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A12);
	otp_data_info.ngbcur = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A14);

	otp_data_info.nrgolden = 0x020d;//selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A16);
	otp_data_info.nbgolden = 0x0297;//selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A18);
	otp_data_info.ngrgolden = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A1A);
	otp_data_info.ngbgolden = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A1c);

	if( (otp_data_info.infoflag>>4 & 0x0c) == 0x04 ){
		otp_data_info.awb_offset = 0;
		otp_data_info.group = 1;
		otp_data_info.awb_flag_sum = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A2E);
	}
	else if ( (otp_data_info.infoflag>>4 & 0x03) == 0x01 ){
		otp_data_info.awb_offset = 0X10;
		otp_data_info.group = 2;
		otp_data_info.awb_flag_sum = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A30);
	}
	else{
		printk("4h7 OTP read data fail otp_data_info.empty!!!\n");
		//goto error;
	}

	return  0;
//error:
//	return  -1;
}
#else 
int otp_group_info_4h7(void)
{
	unsigned short AWB_Group1_flag = 0;
	unsigned short AWB_Group2_flag = 0;
	
	memset(&otp_data_info,0,sizeof(OTP));

	otp_data_info.lsc_infoflag =
		selective_read_region(S5K4H7SUB_LSC_PAGE,S5K4H7SUB_OTP_LSCFLAG_ADDR);

	if( otp_data_info.lsc_infoflag == 0x01 ){
		otp_data_info.lsc_offset = 0;
		otp_data_info.lsc_group = 1;
		otp_data_info.lsc_sum = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A32);
	}
	else if ( otp_data_info.lsc_infoflag == 0x03 ){
		otp_data_info.lsc_offset = 1;
		otp_data_info.lsc_group = 2;
		otp_data_info.lsc_sum = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A34);
	}
	else{
		printk("4H7 OTP read data fail lsc empty!!!\n");
		//goto error;
	}

	otp_data_info.flag_infoflag =
		selective_read_region(S5K4H7SUB_FLAG_PAGE,S5K4H7SUB_OTP_FLAGFLAG_ADDR);

	if( (otp_data_info.flag_infoflag>>4 & 0x0c) == 0x04 ){
		otp_data_info.flag_offset = 0;
		otp_data_info.flag_group = 1;
	}
	else if ( (otp_data_info.flag_infoflag>>4 & 0x03) == 0x01 ){
		otp_data_info.flag_offset = 4;
		otp_data_info.flag_group = 2;
	}
	else{
		printk("4h7 OTP read data fail flag empty!!!\n");
		//goto error;
	}

	otp_data_info.flag_module_integrator_id =
		otp_4h7_read_cmos_sensor(S5K4H7SUB_OTP_FLAGFLAG_ADDR + otp_data_info.flag_offset + 1);

    sensor_module_id = otp_data_info.flag_module_integrator_id;

	if( (otp_data_info.flag_group ==1) || (otp_data_info.flag_group == 2)){
		if(otp_data_info.flag_group ==1){
			otp_data_info.flag_offset = 0;
			otp_data_info.flag_group = 1;
			otp_data_info.flag_infoflag = 1;
			otp_data_info.group = 1;
			
			otp_data_info.frgcur = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A05);//0x11f;
			otp_data_info.fbgcur = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A07);//0x179;

			otp_data_info.awb_flag_sum = selective_read_region(S5K4H7SUB_AWB_PAGE,0x0A19);
			printk("4h7 OTP read group 1 frgcur=0x%x fbgcur=0x%x awb_flag_sum=0x%x!\n", otp_data_info.frgcur,otp_data_info.fbgcur,otp_data_info.awb_flag_sum);
		}
		else if(otp_data_info.flag_group == 2){
			otp_data_info.flag_offset = 4;
			otp_data_info.flag_group = 2;
			otp_data_info.flag_infoflag = 1;
			otp_data_info.group = 2;
			
			otp_data_info.frgcur = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A1A);
			otp_data_info.fbgcur = selective_read_region_16(S5K4H7SUB_AWB_PAGE,0x0A1C);

			otp_data_info.awb_flag_sum = selective_read_region(S5K4H7SUB_AWB_PAGE,0x0A2E);
			printk("4h7 OTP read group 2 frgcur=0x%x fbgcur=0x%x awb_flag_sum=0x%x!\n", otp_data_info.frgcur,otp_data_info.fbgcur,otp_data_info.awb_flag_sum);
		}
	}
	else{
		printk("4h7 OTP read AWB_Group1_flag 0x%x  AWB_Group2_flag 0x%x data fail flag empty!!!\n", AWB_Group1_flag, AWB_Group2_flag);
	}

	return  0;
}
#endif
//Chenyee camera pangfei 20180608 modify for CSW1802A-9 end

/*********************************************************
 * read_4h7_page
 * read_Page1~Page21 of data
 * return true or false
 ********************************************************/
bool read_4h7_page(int page_start,int page_end,unsigned char *pdata)
{
	bool bresult = true;
	int st_page_start = page_start;
	if (page_start <= 0 || page_end > 22){
		bresult = false;
		printk(" OTP page_end is large!");
		return bresult;
	}
	for(; st_page_start <= page_end; st_page_start++){
                printk("OTP st_page_start=%d!",st_page_start);
		get_4h7_page_data(st_page_start, pdata);
	}
	return bresult;
}




bool read_4h7_page_lsc(int page_start,int page_end,unsigned char *pdata)
{
	bool bresult = true;
	int st_page_start = page_start;
	if (page_start <= 0 || page_end > 21){
		bresult = false;
		printk(" OTP page_end is large!");
		return bresult;
	}
	for(; st_page_start <= page_end; st_page_start++){
                printk("OTP st_page_start=%d page_end=%d ",st_page_start,page_end);
		get_4h7_page_data_lsc(st_page_start, pdata);
	}
	return bresult;
}

unsigned int sum_awb_flag(unsigned int sum_start, unsigned int sum_end, unsigned char *pdata)
{
	int i = 0;
	unsigned int start;
	unsigned int re_sum = 0;
	for(start = 0x0A05; i < 20; i++, start++){
		if((start >= sum_start) && (start <= sum_end)){
			re_sum += pdata[i];
		}
	}
	return  re_sum;
}

unsigned int sum_awb_flag_lsc(unsigned int sum_start, unsigned int sum_end, unsigned char *pdata)
{
	int i = 0;
	unsigned int start;
	unsigned int re_sum = 0;
	for(start = 0x0A04; i < 64; i++, start++){
		if((start >= sum_start) && (start <= sum_end)){
			re_sum += pdata[i];
		}
	}
	return  re_sum;
}

bool check_sum_flag_awb(void)
{

	int page_start = 22,page_end = 22;

	unsigned char data_p[22][20] = {};
	bool bresult = 0;
	unsigned int  sum_awbfg = 0;
        unsigned int  checksum = 0;

	bresult &= read_4h7_page(page_start, page_end, data_p[page_start-1]);
        //printk("OTP 4h7 bresult = %d ");
	if(otp_data_info.group == 1){

		sum_awbfg = sum_awb_flag(0x0A05, 0X0A18, data_p[page_start-1]);
		//sum_awbfg += sum_awb_flag(0x0A0E, 0X0A1D, data_p[page_start-1]);
                checksum=(sum_awbfg%0xff)+1;
                printk("OTP 4h7 check awb flag sum_awbfg=0x%x  checksum = 0x%x !!!",sum_awbfg,checksum);
	}
	else if (otp_data_info.group == 2){
		sum_awbfg = sum_awb_flag(0x0A09, 0X0A0C, data_p[page_start-1]);
		//sum_awbfg += sum_awb_flag(0x0A1E, 0X0A2D, data_p[page_start-1]);
	}
	if(checksum == otp_data_info.awb_flag_sum){
                printk("OTP 4h7 checksum awb ok\n");
		//apply_4h7_otp_awb();
	}
	else{
		printk("OTP 4h7 check awb flag sum fail!!!");
		bresult &= 0;
	}

	return  bresult;

}

bool  check_sum_flag_lsc(void)
{
	int page_start = 21,page_end = 21;

	unsigned char data_p[21][64] = {};
	bool bresult = true;
	unsigned int  sum_slc = 0;

	if(otp_data_info.lsc_group == 1){
		for(page_start = 1, page_end = 6; page_start <= page_end; page_start++){
			bresult &= read_4h7_page_lsc(page_start, page_start, data_p[page_start-1]);
			if(page_start == 6){
				sum_slc += sum_awb_flag_lsc(0x0A04, 0x0A2B, data_p[page_start-1]);
				continue;
			}
			sum_slc += sum_awb_flag_lsc(0x0A04, 0X0A43,data_p[page_start-1]);
		}
	}
	else if(otp_data_info.lsc_group == 2){
		for(page_start = 6, page_end = 12; page_start <= page_end; page_start++){
			bresult &= read_4h7_page(page_start, page_start, data_p[page_start-1]);
			if(page_start == 6){
				sum_slc += sum_awb_flag_lsc(0x0A2C, 0x0A43, data_p[page_start-1]);
				continue;
			}
			else if(page_start <12){
				sum_slc += sum_awb_flag_lsc(0x0A04, 0X0A43, data_p[page_start-1]);
			}
			else{
				sum_slc += sum_awb_flag_lsc(0x0A04, 0X0A13, data_p[page_start-1]);
			}
		}
	}

	#if 0
	if(sum_slc == otp_data_info.lsc_sum){
                printk("OTP 4h7 check lsc sum ok !!!");
		apply_4h7_otp_enb_lsc();
		otp_data_info.lsc_check_flag = 1;
	}
	else{
		printk("OTP 4h7 check lsc sum fail!!!");
		bresult &= 0;
		otp_data_info.lsc_check_flag = 0;
	}
        #endif

        //apply_4h7_otp_enb_lsc();
	return  bresult;
}

bool update_otp(void)
{
	int result = 1;
	if(otp_group_info_4h7() == -1){
		printk("OTP read data fail  empty!!!\n");
		result &= 0;
	}
	else{
		if(check_sum_flag_awb() == 0 && otp_data_info.lsc_check_flag){

			printk("OTP 4h7 check sum fail!!!\n");
			result &= 0;
		}
		else{
			printk("OTP 4h7 check ok\n");
		}
                check_sum_flag_lsc();
	}
	return  result;
}
