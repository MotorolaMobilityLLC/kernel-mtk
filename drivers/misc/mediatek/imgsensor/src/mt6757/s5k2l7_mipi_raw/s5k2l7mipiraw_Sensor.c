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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	   s5k2l7mipi_Sensor.c
 *
 * Project:
 * --------
 *	   ALPS
 *
 * Description:
 * ------------
 *	   Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k2l7mipiraw_Sensor.h"


//#include "s5k2l7_otp.h"
/*Enable PDAF function */
#define ENABLE_S5K2L7_PDAF_RAW
#define SENSOR_M1 // open this define for m1, close this define for m2
#ifdef ENABLE_S5K2L7_PDAF_RAW
    #define MARK_HDR
#endif
//#define TEST_PATTERN_EN
/*WDR auto ration mode*/
//#define ENABLE_WDR_AUTO_RATION
/****************************Modify Following Strings for Debug****************************/
#define PFX "s5k2l7_camera_sensor"
#define LOG_1 LOG_INF("s5k2l7,MIPI 4LANE\n")
/****************************	Modify end	  *******************************************/
#define LOG_INF(fmt, args...)	pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##args)
static kal_uint32 chip_id = 0;

static DEFINE_SPINLOCK(imgsensor_drv_lock);
#define ORIGINAL_VERSION 1
//#define SLOW_MOTION_120FPS
static imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K2L7_SENSOR_ID,		  //record sensor id defined in Kd_imgsensor.h

	.checksum_value = 0xafb5098f,	   //checksum value for Camera Auto Test

	.pre = {

		.pclk = 960000000,				//record different mode's pclk
		.linelength = 10160,				//record different mode's linelength
		.framelength =3149, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2016,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 1512,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 300

	},
	.cap = {
		.pclk = 960000000,				//record different mode's pclk
		.linelength = 10208, 			//record different mode's linelength
		.framelength =3134, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 4032,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 3024,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 300

	},
	.cap1 = {
		.pclk = 960000000,				//record different mode's pclk
		.linelength = 10208, 			//record different mode's linelength
		.framelength =3134, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 4032,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 3024,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 300
	},

	.normal_video = {

		.pclk = 960000000,				//record different mode's pclk
		.linelength = 10208,				//record different mode's linelength
		.framelength =3134, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 4032,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 3024,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 300

	},

	.hs_video = {
		.pclk = 960000000,
		.linelength = 10160,
		.framelength = 1049,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1344,		//record different mode's width of grabwindow
		.grabwindow_height = 756,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 900,
	},

	.slim_video = {
		.pclk = 960000000,
		.linelength = 10160,
		.framelength = 3149,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1344,		//record different mode's width of grabwindow
		.grabwindow_height = 756,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,

	},
	.margin = 16,
	.min_shutter = 1,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num
	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,
	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = { 0x5A, 0x20, 0xFF},
	.i2c_speed = 300,
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x3D0,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.hdr_mode = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x5A,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
/* full_w; full_h; x0_offset; y0_offset; w0_size; h0_size; scale_w; scale_h; x1_offset;  y1_offset;  w1_size;  h1_size;
	 x2_tg_offset;	 y2_tg_offset;	w2_tg_size;  h2_tg_size;*/
{
{ 4032, 3024, 0,   0, 4032, 3024, 2016, 1512, 0, 0, 2016, 1512, 0, 0, 2016, 1512}, // Preview
{ 4032, 3024, 0,   0, 4032, 3024, 4032, 3024, 0, 0, 4032, 3024, 0, 0, 4032, 3024}, // capture
{ 4032, 3024, 0,   0, 4032, 3024, 4032, 3024, 0, 0, 4032, 3024, 0, 0, 4032, 3024}, // normal_video
{ 4032, 3024, 0, 348, 4032, 2328, 1344,  756, 0, 0, 1344,  756, 0, 0, 1344, 756}, // hs_video
{ 4032, 3024, 0, 348, 4032, 2328, 1344,  756, 0, 0, 1336,  756, 0, 0, 1344, 756}, // slim_video
};

//#define USE_OIS
#ifdef USE_OIS
#define OIS_I2C_WRITE_ID 0x48
#define OIS_I2C_READ_ID 0x49

#define RUMBA_OIS_CTRL	 0x0000
#define RUMBA_OIS_STATUS 0x0001
#define RUMBA_OIS_MODE	 0x0002
#define CENTERING_MODE	 0x05
#define RUMBA_OIS_OFF	 0x0030

#define RUMBA_OIS_SETTING_ADD 0x0002
#define RUMBA_OIS_PRE_SETTING 0x02
#define RUMBA_OIS_CAP_SETTING 0x01


#define RUMBA_OIS_PRE	 0
#define RUMBA_OIS_CAP	 1


static void OIS_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	iWriteRegI2C(pusendcmd , 3, OIS_I2C_WRITE_ID);
}
static kal_uint16 OIS_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(pusendcmd , 2, (u8*)&get_byte,1,OIS_I2C_READ_ID);
	return get_byte;
}
static int OIS_on(int mode)
{
	int ret = 0;
	if(mode == RUMBA_OIS_PRE_SETTING){
		OIS_write_cmos_sensor(RUMBA_OIS_SETTING_ADD,RUMBA_OIS_PRE_SETTING);
	}
	if(mode == RUMBA_OIS_CAP_SETTING){
		OIS_write_cmos_sensor(RUMBA_OIS_SETTING_ADD,RUMBA_OIS_CAP_SETTING);
	}
	OIS_write_cmos_sensor(RUMBA_OIS_MODE,CENTERING_MODE);
	ret = OIS_read_cmos_sensor(RUMBA_OIS_MODE);
	LOG_INF("pangfei OIS ret=%d %s %d\n",ret,__func__,__LINE__);

	if(ret != CENTERING_MODE){
		//return -1;
	}
	OIS_write_cmos_sensor(RUMBA_OIS_CTRL,0x01);
	ret = OIS_read_cmos_sensor(RUMBA_OIS_CTRL);
	LOG_INF("pangfei OIS ret=%d %s %d\n",ret,__func__,__LINE__);
	if(ret != 0x01){
		//return -1;
	}

}

static int OIS_off(void)
{
	int ret = 0;
	OIS_write_cmos_sensor(RUMBA_OIS_OFF,0x01);
	ret = OIS_read_cmos_sensor(RUMBA_OIS_OFF);
	LOG_INF("pangfei OIS ret=%d %s %d\n",ret,__func__,__LINE__);
}
#endif
//add for s5k2l7 pdaf
static SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX = 0,
	.i4OffsetY = 0,
	.i4PitchX  = 0,
	.i4PitchY  = 0,
	.i4PairNum	=0,
	.i4SubBlkW	=0,
	.i4SubBlkH	=0,
	.i4PosL = {{0,0}},
	.i4PosR = {{0,0}},
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst = 0,
	.i4Crop = {{0,0},{0,0},{0,0},{0,0},{0,0}, \
			   {0,0},{0,0},{0,0},{0,0},{0,0}},
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	//iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
	iReadRegI2CTiming(pu_send_cmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id,imgsensor_info.i2c_speed);

	return get_byte;
}
static kal_uint16 read_cmos_sensor_twobyte(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char get_word[2];
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	//iReadRegI2C(pu_send_cmd, 2, get_word, 2, imgsensor.i2c_write_id);
	iReadRegI2CTiming(pu_send_cmd, 2, get_word, 2, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
	get_byte = (((int)get_word[0])<<8) | get_word[1];
	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	//iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
	iWriteRegI2CTiming(pu_send_cmd, 3, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
}

static void write_cmos_sensor_twobyte(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8),(char)(para & 0xFF)};
	//LOG_INF("write_cmos_sensor_twobyte is %x,%x,%x,%x\n", pu_send_cmd[0], pu_send_cmd[1], pu_send_cmd[2], pu_send_cmd[3]);
	//iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
	iWriteRegI2CTiming(pu_send_cmd, 4, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
}
static void set_dummy(void)
{
#if 1
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	//write_cmos_sensor(0x0104, 0x01);
	//write_cmos_sensor_twobyte(0x6028,0x4000);
	//write_cmos_sensor_twobyte(0x602A,0xC340 );
	write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);

	//write_cmos_sensor_twobyte(0x602A,0xC342 );
	write_cmos_sensor_twobyte(0x0342, imgsensor.line_length);

	//write_cmos_sensor(0x0104, 0x00);
#endif
#if 0
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);
	write_cmos_sensor_twobyte(0x0342, imgsensor.line_length);
	write_cmos_sensor(0x0104, 0x00);
#endif
}	 /*    set_dummy  */

static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	LOG_INF("framerate = %d, min framelength should enable = %d \n", framerate,min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
		imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		{
			imgsensor.frame_length = imgsensor_info.max_frame_length;
			imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
		}
		if (min_framelength_en)
			imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	 /*    set_max_framerate  */


/*************************************************************************
* FUNCTION
*	 set_shutter
*
* DESCRIPTION
*	 This function set e-shutter of sensor to change exposure time.
*	 The registers 0x3500 ,0x3501 and 0x3502 control exposure of s5k2l7.
*	 The exposure value is in number of Tline, where Tline is the time of sensor one line.
*
*	 Exposure = [reg 0x3500]<<12 + [reg 0x3501]<<4 + [reg 0x3502]>>4;
*	 The maximum exposure value is limited by VTS defined by register 0x380e and 0x380f.
	  Maximum Exposure <= VTS -4
*
* PARAMETERS
*	 iShutter : exposured lines
*
* RETURNS
*	 None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	//LOG_INF("enter xxxx  set_shutter, shutter =%d\n", shutter);

	unsigned long flags;
	//kal_uint16 realtime_fps = 0;
	//kal_uint32 frame_length = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	LOG_INF("set_shutter =%d\n", shutter);
	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
	if(!shutter) shutter = 1; /*avoid 0*/
	spin_lock(&imgsensor_drv_lock);
		if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
			imgsensor.frame_length = shutter + imgsensor_info.margin;
		else
			imgsensor.frame_length = imgsensor.min_frame_length;

		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
			imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);

	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;


// Frame length :4000 C340
	//write_cmos_sensor_twobyte(0x6028,0x4000);
	//write_cmos_sensor_twobyte(0x602A,0xC340 );
	write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);

//Shutter reg : 4000 C202
	//write_cmos_sensor_twobyte(0x6028,0x4000);
	//write_cmos_sensor_twobyte(0x602A,0xC202 );
	write_cmos_sensor_twobyte(0x0202,shutter);
	write_cmos_sensor_twobyte(0x0226,shutter);

}	 /*    set_shutter */

static void hdr_write_shutter(kal_uint16 le, kal_uint16 se)
{
	//LOG_INF("enter xxxx  set_shutter, shutter =%d\n", shutter);
	unsigned int iRation;

	unsigned long flags;
	//kal_uint16 realtime_fps = 0;
	//kal_uint32 frame_length = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = le;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	if(!le) le = 1; /*avoid 0*/

	spin_lock(&imgsensor_drv_lock);
	if (le > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = le + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;

	spin_unlock(&imgsensor_drv_lock);

	le = (le < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : le;
	le = (le > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : le;

	// Frame length :4000 C340
	//write_cmos_sensor_twobyte(0x6028,0x4000);
	//write_cmos_sensor_twobyte(0x602A,0xC340 );
	write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);

	//SET LE/SE ration
	//iRation = (((LE + SE/2)/SE) >> 1 ) << 1 ;
	iRation = ((10 * le / se) + 5) / 10;
	if(iRation < 2)
		iRation = 1;
	else if(iRation < 4)
		iRation = 2;
	else if(iRation < 8)
		iRation = 4;
	else if(iRation < 16)
		iRation = 8;
	else if(iRation < 32)
		iRation = 16;
	else
		iRation = 1;

	/*set ration for auto */
	iRation = 0x100 * iRation;
#if defined(ENABLE_WDR_AUTO_RATION)
	/*LE / SE ration ,	0x218/0x21a =  LE Ration*/
	/*0x218 =0x400, 0x21a=0x100, LE/SE = 4x*/
	write_cmos_sensor_twobyte(0x0218, iRation);
	write_cmos_sensor_twobyte(0x021a, 0x100);
#endif
	/*Short exposure */
	write_cmos_sensor_twobyte(0x0202,se);
	/*Log exposure ratio*/
	write_cmos_sensor_twobyte(0x0226,le);

	LOG_INF("HDR set shutter LE=%d, SE=%d, iRation=0x%x\n", le, se,iRation);

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0;
	reg_gain = gain/2;
	return (kal_uint16)reg_gain;
}

/*************************************************************************
* FUNCTION
*	 set_gain
*
* DESCRIPTION
*	 This function is to set global gain to sensor.
*
* PARAMETERS
*	 iGain : sensor global gain(base: 0x80)
*
* RETURNS
*	 the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{

	  kal_uint16 reg_gain;
	  kal_uint32 sensor_gain1 = 0;
	  kal_uint32 sensor_gain2 = 0;
	/* 0x350A[0:1], 0x350B[0:7] AGC real gain */
	/* [0:3] = N meams N /16 X	*/
	/* [4:9] = M meams M X		 */
	/* Total gain = M + N /16 X   */

	//
	if (gain < BASEGAIN || gain > 32 * BASEGAIN) {
		LOG_INF("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 32 * BASEGAIN)
			gain = 32 * BASEGAIN;
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	//LOG_INF("gain = %d , reg_gain = 0x%x ", gain, reg_gain);

	//Analog gain HW reg : 4000 C204

	write_cmos_sensor_twobyte(0x6028,0x4000);
	write_cmos_sensor_twobyte(0x602A,0x0204 );
	write_cmos_sensor_twobyte(0x6F12,reg_gain);
	write_cmos_sensor_twobyte(0x6F12,reg_gain);


	write_cmos_sensor_twobyte(0x602C,0x4000);
	write_cmos_sensor_twobyte(0x602E, 0x0204);
	sensor_gain1 = read_cmos_sensor_twobyte(0x6F12);

	write_cmos_sensor_twobyte(0x602C,0x4000);
	write_cmos_sensor_twobyte(0x602E, 0x0206);
	sensor_gain2 = read_cmos_sensor_twobyte(0x6F12);
	LOG_INF("imgsensor.gain(0x%x), gain1(0x%x), gain2(0x%x)\n",imgsensor.gain, sensor_gain1, sensor_gain2);

	return gain;

}	 /*    set_gain  */

static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);
#if 1
	/********************************************************
	   *
	   *   0x3820[2] ISP Vertical flip
	   *   0x3820[1] Sensor Vertical flip
	   *
	   *   0x3821[2] ISP Horizontal mirror
	   *   0x3821[1] Sensor Horizontal mirror
	   *
	   *   ISP and Sensor flip or mirror register bit should be the same!!
	   *
	   ********************************************************/
	spin_lock(&imgsensor_drv_lock);
	imgsensor.mirror= image_mirror;
	spin_unlock(&imgsensor_drv_lock);
	switch (image_mirror) {

		case IMAGE_NORMAL:
			write_cmos_sensor(0x0101,0x00);   // Gr
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0101,0x01);
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0101,0x02);
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x0101,0x03);//Gb
			break;
		default:
			LOG_INF("Error image_mirror setting\n");

	}
#endif
}

/*************************************************************************
* FUNCTION
*	 night_mode
*
* DESCRIPTION
*	 This function night mode of sensor.
*
* PARAMETERS
*	 bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	 None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}	 /*    night_mode	 */

// #define	S5K2l7FW


#ifndef MARK_HDR
static void sensor_WDR_zhdr(void)
{
	if(imgsensor.hdr_mode == 9)
	{
		LOG_INF("sensor_WDR_zhdr\n");
		/*it would write 0x21E = 0x1, 0x21F=0x00*/
		/*0x21E=1 , Enable WDR*/
		/*0x21F=0x00, Use Manual mode to set short /long exp */
#if defined(ENABLE_WDR_AUTO_RATION)
		write_cmos_sensor_twobyte(0x021E, 0x0101); /*For WDR auot ration*/
#else
		write_cmos_sensor_twobyte(0x021E, 0x0100); /*For WDR manual ration*/
#endif
		write_cmos_sensor_twobyte(0x0220, 0x0801);
		write_cmos_sensor_twobyte(0x0222, 0x0100);



	}
	else
	{

		write_cmos_sensor_twobyte(0x021E, 0x0000);
		write_cmos_sensor_twobyte(0x0220, 0x0801);

	}
	/*for LE/SE Test*/
	//hdr_write_shutter(3460,800);

}
#endif

static void sensor_init_11_new(void)
{
	/*2l7_global(HQ)_1014.sset*/
	LOG_INF("Enter s5k2l7 sensor_init.\n");
//$MIPI[Width:4032,Height:1512,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:2034,useEmbData:0]
//$MV1[MCLK:24,Width:4032,Height:1512,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:2034,pvi_pclk_inverse:0]


write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0XBBF8);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6010, 0X0001);
mdelay(3);

write_cmos_sensor_twobyte(0X6214, 0X7970);
write_cmos_sensor_twobyte(0X6218, 0X7150);
//TnP
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X87AC);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X0449);
write_cmos_sensor_twobyte(0X6F12, 0X0348);
write_cmos_sensor_twobyte(0X6F12, 0X044A);
write_cmos_sensor_twobyte(0X6F12, 0X4860);
write_cmos_sensor_twobyte(0X6F12, 0X101A);
write_cmos_sensor_twobyte(0X6F12, 0X0881);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XDFB8);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X8A68);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X5C20);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0XA3D4);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X30B5);
write_cmos_sensor_twobyte(0X6F12, 0X7E4C);
write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
write_cmos_sensor_twobyte(0X6F12, 0XE232);
write_cmos_sensor_twobyte(0X6F12, 0X608C);
write_cmos_sensor_twobyte(0X6F12, 0X081A);
write_cmos_sensor_twobyte(0X6F12, 0XA18C);
write_cmos_sensor_twobyte(0X6F12, 0X401E);
write_cmos_sensor_twobyte(0X6F12, 0X1944);
write_cmos_sensor_twobyte(0X6F12, 0X8142);
write_cmos_sensor_twobyte(0X6F12, 0X01D9);
write_cmos_sensor_twobyte(0X6F12, 0X0020);
write_cmos_sensor_twobyte(0X6F12, 0X0346);
write_cmos_sensor_twobyte(0X6F12, 0X7949);
write_cmos_sensor_twobyte(0X6F12, 0X9340);
write_cmos_sensor_twobyte(0X6F12, 0X4880);
write_cmos_sensor_twobyte(0X6F12, 0X9040);
write_cmos_sensor_twobyte(0X6F12, 0X784A);
write_cmos_sensor_twobyte(0X6F12, 0X99B2);
write_cmos_sensor_twobyte(0X6F12, 0XD181);
write_cmos_sensor_twobyte(0X6F12, 0X1082);
write_cmos_sensor_twobyte(0X6F12, 0X774D);
write_cmos_sensor_twobyte(0X6F12, 0XAD79);
write_cmos_sensor_twobyte(0X6F12, 0X002D);
write_cmos_sensor_twobyte(0X6F12, 0X06D1);
write_cmos_sensor_twobyte(0X6F12, 0X33B1);
write_cmos_sensor_twobyte(0X6F12, 0X94F8);
write_cmos_sensor_twobyte(0X6F12, 0XB730);
write_cmos_sensor_twobyte(0X6F12, 0X1BB1);
write_cmos_sensor_twobyte(0X6F12, 0X5181);
write_cmos_sensor_twobyte(0X6F12, 0X401E);
write_cmos_sensor_twobyte(0X6F12, 0X9081);
write_cmos_sensor_twobyte(0X6F12, 0X30BD);
write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
write_cmos_sensor_twobyte(0X6F12, 0XFF70);
write_cmos_sensor_twobyte(0X6F12, 0X5081);
write_cmos_sensor_twobyte(0X6F12, 0XF9E7);
write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
write_cmos_sensor_twobyte(0X6F12, 0XF041);
write_cmos_sensor_twobyte(0X6F12, 0X0646);
write_cmos_sensor_twobyte(0X6F12, 0X6B48);
write_cmos_sensor_twobyte(0X6F12, 0X0D46);
write_cmos_sensor_twobyte(0X6F12, 0X8268);
write_cmos_sensor_twobyte(0X6F12, 0X140C);
write_cmos_sensor_twobyte(0X6F12, 0X97B2);
write_cmos_sensor_twobyte(0X6F12, 0X0022);
write_cmos_sensor_twobyte(0X6F12, 0X3946);
write_cmos_sensor_twobyte(0X6F12, 0X2046);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XEEF8);
write_cmos_sensor_twobyte(0X6F12, 0X2946);
write_cmos_sensor_twobyte(0X6F12, 0X3046);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XEFF8);
write_cmos_sensor_twobyte(0X6F12, 0X6448);
write_cmos_sensor_twobyte(0X6F12, 0X6749);
write_cmos_sensor_twobyte(0X6F12, 0X0122);
write_cmos_sensor_twobyte(0X6F12, 0X4088);
write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
write_cmos_sensor_twobyte(0X6F12, 0XBC07);
write_cmos_sensor_twobyte(0X6F12, 0X3946);
write_cmos_sensor_twobyte(0X6F12, 0X2046);
write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
write_cmos_sensor_twobyte(0X6F12, 0XF041);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XDEB8);
write_cmos_sensor_twobyte(0X6F12, 0X70B5);
write_cmos_sensor_twobyte(0X6F12, 0X0646);
write_cmos_sensor_twobyte(0X6F12, 0X5D48);
write_cmos_sensor_twobyte(0X6F12, 0X0022);
write_cmos_sensor_twobyte(0X6F12, 0XC168);
write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
write_cmos_sensor_twobyte(0X6F12, 0X2946);
write_cmos_sensor_twobyte(0X6F12, 0X2046);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XD3F8);
write_cmos_sensor_twobyte(0X6F12, 0X3046);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XDAF8);
write_cmos_sensor_twobyte(0X6F12, 0X5B49);
write_cmos_sensor_twobyte(0X6F12, 0X96F8);
write_cmos_sensor_twobyte(0X6F12, 0XB100);
write_cmos_sensor_twobyte(0X6F12, 0X0880);
write_cmos_sensor_twobyte(0X6F12, 0X2946);
write_cmos_sensor_twobyte(0X6F12, 0X2046);
write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
write_cmos_sensor_twobyte(0X6F12, 0X7040);
write_cmos_sensor_twobyte(0X6F12, 0X0122);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XC5B8);
write_cmos_sensor_twobyte(0X6F12, 0X70B5);
write_cmos_sensor_twobyte(0X6F12, 0X0646);
write_cmos_sensor_twobyte(0X6F12, 0X5148);
write_cmos_sensor_twobyte(0X6F12, 0X0022);
write_cmos_sensor_twobyte(0X6F12, 0X0169);
write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
write_cmos_sensor_twobyte(0X6F12, 0X2946);
write_cmos_sensor_twobyte(0X6F12, 0X2046);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XBAF8);
write_cmos_sensor_twobyte(0X6F12, 0X3046);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0XC6F8);
write_cmos_sensor_twobyte(0X6F12, 0X26B3);
write_cmos_sensor_twobyte(0X6F12, 0X4D48);
write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
write_cmos_sensor_twobyte(0X6F12, 0XA007);
write_cmos_sensor_twobyte(0X6F12, 0X00B3);
write_cmos_sensor_twobyte(0X6F12, 0X4B48);
write_cmos_sensor_twobyte(0X6F12, 0X6521);
write_cmos_sensor_twobyte(0X6F12, 0X90F8);
write_cmos_sensor_twobyte(0X6F12, 0XE407);
write_cmos_sensor_twobyte(0X6F12, 0X4843);
write_cmos_sensor_twobyte(0X6F12, 0X4B49);
write_cmos_sensor_twobyte(0X6F12, 0X01EB);
write_cmos_sensor_twobyte(0X6F12, 0X8003);
write_cmos_sensor_twobyte(0X6F12, 0X01F6);
write_cmos_sensor_twobyte(0X6F12, 0X9C02);
write_cmos_sensor_twobyte(0X6F12, 0X4949);
write_cmos_sensor_twobyte(0X6F12, 0XB3F8);
write_cmos_sensor_twobyte(0X6F12, 0X3202);
write_cmos_sensor_twobyte(0X6F12, 0X91F8);
write_cmos_sensor_twobyte(0X6F12, 0XF719);
write_cmos_sensor_twobyte(0X6F12, 0X32F8);
write_cmos_sensor_twobyte(0X6F12, 0X1110);
write_cmos_sensor_twobyte(0X6F12, 0X0844);
write_cmos_sensor_twobyte(0X6F12, 0X40F4);
write_cmos_sensor_twobyte(0X6F12, 0X8051);
write_cmos_sensor_twobyte(0X6F12, 0X4548);
write_cmos_sensor_twobyte(0X6F12, 0X0180);
write_cmos_sensor_twobyte(0X6F12, 0XB3F8);
write_cmos_sensor_twobyte(0X6F12, 0X3612);
write_cmos_sensor_twobyte(0X6F12, 0X424B);
write_cmos_sensor_twobyte(0X6F12, 0X93F8);
write_cmos_sensor_twobyte(0X6F12, 0XF939);
write_cmos_sensor_twobyte(0X6F12, 0X32F8);
write_cmos_sensor_twobyte(0X6F12, 0X1320);
write_cmos_sensor_twobyte(0X6F12, 0X1144);
write_cmos_sensor_twobyte(0X6F12, 0X41F4);
write_cmos_sensor_twobyte(0X6F12, 0X8051);
write_cmos_sensor_twobyte(0X6F12, 0X8180);
write_cmos_sensor_twobyte(0X6F12, 0X2946);
write_cmos_sensor_twobyte(0X6F12, 0X2046);
write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
write_cmos_sensor_twobyte(0X6F12, 0X7040);
write_cmos_sensor_twobyte(0X6F12, 0X0122);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0X8AB8);
write_cmos_sensor_twobyte(0X6F12, 0X70B5);
write_cmos_sensor_twobyte(0X6F12, 0X3C4D);
write_cmos_sensor_twobyte(0X6F12, 0X41F2);
write_cmos_sensor_twobyte(0X6F12, 0XF046);
write_cmos_sensor_twobyte(0X6F12, 0X1544);
write_cmos_sensor_twobyte(0X6F12, 0X049C);
write_cmos_sensor_twobyte(0X6F12, 0XAD5D);
write_cmos_sensor_twobyte(0X6F12, 0X0DB9);
write_cmos_sensor_twobyte(0X6F12, 0X1B78);
write_cmos_sensor_twobyte(0X6F12, 0X03B1);
write_cmos_sensor_twobyte(0X6F12, 0X0123);
write_cmos_sensor_twobyte(0X6F12, 0X2370);
write_cmos_sensor_twobyte(0X6F12, 0X374B);
write_cmos_sensor_twobyte(0X6F12, 0X03EB);
write_cmos_sensor_twobyte(0X6F12, 0XC203);
write_cmos_sensor_twobyte(0X6F12, 0XB3F8);
write_cmos_sensor_twobyte(0X6F12, 0XD630);
write_cmos_sensor_twobyte(0X6F12, 0X2381);
write_cmos_sensor_twobyte(0X6F12, 0X90F8);
write_cmos_sensor_twobyte(0X6F12, 0XCC50);
write_cmos_sensor_twobyte(0X6F12, 0X1346);
write_cmos_sensor_twobyte(0X6F12, 0X05B1);
write_cmos_sensor_twobyte(0X6F12, 0X0223);
write_cmos_sensor_twobyte(0X6F12, 0X334D);
write_cmos_sensor_twobyte(0X6F12, 0X03EB);
write_cmos_sensor_twobyte(0X6F12, 0XC303);
write_cmos_sensor_twobyte(0X6F12, 0X05EB);
write_cmos_sensor_twobyte(0X6F12, 0X8303);
write_cmos_sensor_twobyte(0X6F12, 0X1B79);
write_cmos_sensor_twobyte(0X6F12, 0X53B1);
write_cmos_sensor_twobyte(0X6F12, 0X284B);
write_cmos_sensor_twobyte(0X6F12, 0XB3F8);
write_cmos_sensor_twobyte(0X6F12, 0XB855);
write_cmos_sensor_twobyte(0X6F12, 0X244B);
write_cmos_sensor_twobyte(0X6F12, 0X1B78);
write_cmos_sensor_twobyte(0X6F12, 0X03F5);
write_cmos_sensor_twobyte(0X6F12, 0XFC63);
write_cmos_sensor_twobyte(0X6F12, 0X9D42);
write_cmos_sensor_twobyte(0X6F12, 0X01D9);
write_cmos_sensor_twobyte(0X6F12, 0X0123);
write_cmos_sensor_twobyte(0X6F12, 0X2370);
write_cmos_sensor_twobyte(0X6F12, 0X90F8);
write_cmos_sensor_twobyte(0X6F12, 0X2830);
write_cmos_sensor_twobyte(0X6F12, 0X03B1);
write_cmos_sensor_twobyte(0X6F12, 0X0123);
write_cmos_sensor_twobyte(0X6F12, 0X6370);
write_cmos_sensor_twobyte(0X6F12, 0X90F8);
write_cmos_sensor_twobyte(0X6F12, 0X1833);
write_cmos_sensor_twobyte(0X6F12, 0XE370);
write_cmos_sensor_twobyte(0X6F12, 0X90F8);
write_cmos_sensor_twobyte(0X6F12, 0X1703);
write_cmos_sensor_twobyte(0X6F12, 0XA070);
write_cmos_sensor_twobyte(0X6F12, 0X01EB);
write_cmos_sensor_twobyte(0X6F12, 0X8200);
write_cmos_sensor_twobyte(0X6F12, 0XD0F8);
write_cmos_sensor_twobyte(0X6F12, 0X2C11);
write_cmos_sensor_twobyte(0X6F12, 0XC1F3);
write_cmos_sensor_twobyte(0X6F12, 0X8F11);
write_cmos_sensor_twobyte(0X6F12, 0XA180);
write_cmos_sensor_twobyte(0X6F12, 0XD0F8);
write_cmos_sensor_twobyte(0X6F12, 0X3401);
write_cmos_sensor_twobyte(0X6F12, 0XC0F3);
write_cmos_sensor_twobyte(0X6F12, 0X8F10);
write_cmos_sensor_twobyte(0X6F12, 0XE080);
write_cmos_sensor_twobyte(0X6F12, 0X70BD);
write_cmos_sensor_twobyte(0X6F12, 0X10B5);
write_cmos_sensor_twobyte(0X6F12, 0X0022);
write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
write_cmos_sensor_twobyte(0X6F12, 0XB311);
write_cmos_sensor_twobyte(0X6F12, 0X1D48);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0X56F8);
write_cmos_sensor_twobyte(0X6F12, 0X104C);
write_cmos_sensor_twobyte(0X6F12, 0X0122);
write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
write_cmos_sensor_twobyte(0X6F12, 0X7911);
write_cmos_sensor_twobyte(0X6F12, 0X6060);
write_cmos_sensor_twobyte(0X6F12, 0X1A48);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0X4EF8);
write_cmos_sensor_twobyte(0X6F12, 0X0022);
write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
write_cmos_sensor_twobyte(0X6F12, 0X4B11);
write_cmos_sensor_twobyte(0X6F12, 0XA060);
write_cmos_sensor_twobyte(0X6F12, 0X1748);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0X47F8);
write_cmos_sensor_twobyte(0X6F12, 0X0022);
write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
write_cmos_sensor_twobyte(0X6F12, 0X2911);
write_cmos_sensor_twobyte(0X6F12, 0XE060);
write_cmos_sensor_twobyte(0X6F12, 0X1548);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0X40F8);
write_cmos_sensor_twobyte(0X6F12, 0X0022);
write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
write_cmos_sensor_twobyte(0X6F12, 0XBF01);
write_cmos_sensor_twobyte(0X6F12, 0X2061);
write_cmos_sensor_twobyte(0X6F12, 0X1248);
write_cmos_sensor_twobyte(0X6F12, 0X00F0);
write_cmos_sensor_twobyte(0X6F12, 0X39F8);
write_cmos_sensor_twobyte(0X6F12, 0X6061);
write_cmos_sensor_twobyte(0X6F12, 0X10BD);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X0AE0);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X8A50);
write_cmos_sensor_twobyte(0X6F12, 0X4000);
write_cmos_sensor_twobyte(0X6F12, 0XF000);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X0A40);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X5D00);
write_cmos_sensor_twobyte(0X6F12, 0X4000);
write_cmos_sensor_twobyte(0X6F12, 0X6B56);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X68F0);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X0BB0);
write_cmos_sensor_twobyte(0X6F12, 0X4000);
write_cmos_sensor_twobyte(0X6F12, 0XF66E);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X7220);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X5900);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X6F12, 0X8720);
write_cmos_sensor_twobyte(0X6F12, 0X0001);
write_cmos_sensor_twobyte(0X6F12, 0X14E7);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X5059);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X512F);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0XFB2B);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0XD02D);
write_cmos_sensor_twobyte(0X6F12, 0X45F6);
write_cmos_sensor_twobyte(0X6F12, 0X576C);
write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X6F12, 0X6047);
write_cmos_sensor_twobyte(0X6F12, 0X45F2);
write_cmos_sensor_twobyte(0X6F12, 0X590C);
write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X6F12, 0X6047);
write_cmos_sensor_twobyte(0X6F12, 0X45F2);
write_cmos_sensor_twobyte(0X6F12, 0X2F1C);
write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X6F12, 0X6047);
write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
write_cmos_sensor_twobyte(0X6F12, 0X2B3C);
write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X6F12, 0X6047);
write_cmos_sensor_twobyte(0X6F12, 0X43F2);
write_cmos_sensor_twobyte(0X6F12, 0X7B5C);
write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X6F12, 0X6047);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
//Global
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0XF466, 0X000C);
write_cmos_sensor_twobyte(0XF468, 0X000D);
write_cmos_sensor_twobyte(0XF488, 0X0008);
write_cmos_sensor_twobyte(0X0200, 0X0100);
write_cmos_sensor_twobyte(0X0202, 0X0100);
write_cmos_sensor_twobyte(0X0224, 0X0100);
write_cmos_sensor_twobyte(0X0226, 0X0100);
write_cmos_sensor_twobyte(0XF414, 0X0007);
write_cmos_sensor_twobyte(0XF416, 0X0004);
write_cmos_sensor_twobyte(0X30C0, 0X0001);
write_cmos_sensor_twobyte(0X30C6, 0X0100);
write_cmos_sensor_twobyte(0X30CA, 0X0300);
write_cmos_sensor_twobyte(0X30C8, 0X05DC);
write_cmos_sensor_twobyte(0X6B36, 0X5200);
write_cmos_sensor_twobyte(0X6B38, 0X0000);
write_cmos_sensor_twobyte(0X0B04, 0X0101);
write_cmos_sensor_twobyte(0X3094, 0X2800);
write_cmos_sensor_twobyte(0X3096, 0X5400);
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X16B0);
write_cmos_sensor_twobyte(0X6F12, 0X0040);
write_cmos_sensor_twobyte(0X6F12, 0X0040);
write_cmos_sensor_twobyte(0X602A, 0X167E);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X602A, 0X1682);
write_cmos_sensor_twobyte(0X6F12, 0X0010);
write_cmos_sensor_twobyte(0X602A, 0X16A0);
write_cmos_sensor_twobyte(0X6F12, 0X2101);
write_cmos_sensor_twobyte(0X602A, 0X1664);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X2A98);
write_cmos_sensor_twobyte(0X6F12, 0X0001);
write_cmos_sensor_twobyte(0X602A, 0X2A9A);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X03FF);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X6F12, 0X0FC0);
write_cmos_sensor_twobyte(0X602A, 0X2AC2);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X03FF);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X6F12, 0X0FC0);
write_cmos_sensor_twobyte(0X602A, 0X2AEA);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X03FF);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X6F12, 0X0FC0);
write_cmos_sensor_twobyte(0X602A, 0X2B12);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X03FF);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X6F12, 0X0FC0);
write_cmos_sensor_twobyte(0X602A, 0X2B3A);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X03FF);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X6F12, 0X0FC0);
write_cmos_sensor_twobyte(0X602A, 0X2B62);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X03FF);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X6F12, 0X0FC0);
write_cmos_sensor_twobyte(0X602A, 0X2B8A);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X03FF);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X6F12, 0X0FC0);
write_cmos_sensor_twobyte(0X602A, 0X2BB2);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X03FF);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X6F12, 0X0FC0);
write_cmos_sensor_twobyte(0X602A, 0X3568);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X402C);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X4AF0);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X35C4);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X35F6);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X35BA);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X35EC);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X35B0);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X35E2);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X35A6);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X35D8);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X4088);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X40BA);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X407E);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X40B0);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X4074);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X40A6);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X406A);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X409C);
write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
write_cmos_sensor_twobyte(0X602A, 0X4B4C);
write_cmos_sensor_twobyte(0X6F12, 0XFFCD);
write_cmos_sensor_twobyte(0X602A, 0X4B7E);
write_cmos_sensor_twobyte(0X6F12, 0XFFCD);
write_cmos_sensor_twobyte(0X602A, 0X4B42);
write_cmos_sensor_twobyte(0X6F12, 0XFFCD);
write_cmos_sensor_twobyte(0X602A, 0X4B74);
write_cmos_sensor_twobyte(0X6F12, 0XFFCD);
write_cmos_sensor_twobyte(0X602A, 0X4B38);
write_cmos_sensor_twobyte(0X6F12, 0XFFCD);
write_cmos_sensor_twobyte(0X602A, 0X4B6A);
write_cmos_sensor_twobyte(0X6F12, 0XFFCD);
write_cmos_sensor_twobyte(0X602A, 0X4B2E);
write_cmos_sensor_twobyte(0X6F12, 0XFFCD);
write_cmos_sensor_twobyte(0X602A, 0X4B60);
write_cmos_sensor_twobyte(0X6F12, 0XFFCD);
write_cmos_sensor_twobyte(0X602A, 0X3684);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X4148);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X4C0C);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X3688);
write_cmos_sensor_twobyte(0X6F12, 0X000E);
write_cmos_sensor_twobyte(0X6F12, 0X000E);
write_cmos_sensor_twobyte(0X602A, 0X368E);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X414C);
write_cmos_sensor_twobyte(0X6F12, 0X000E);
write_cmos_sensor_twobyte(0X6F12, 0X000E);
write_cmos_sensor_twobyte(0X602A, 0X4152);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X4C10);
write_cmos_sensor_twobyte(0X6F12, 0X000E);
write_cmos_sensor_twobyte(0X6F12, 0X000E);
write_cmos_sensor_twobyte(0X6F12, 0X0002);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0B52);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0B54);
write_cmos_sensor_twobyte(0X6F12, 0X0001);
write_cmos_sensor_twobyte(0X602A, 0X1686);
write_cmos_sensor_twobyte(0X6F12, 0X0328);
write_cmos_sensor_twobyte(0X602A, 0X0B02);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X4C54);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X4C58);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X4C5C);
write_cmos_sensor_twobyte(0X6F12, 0X010F);
write_cmos_sensor_twobyte(0X602A, 0X09A0);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X602A, 0X09A2);
write_cmos_sensor_twobyte(0X6F12, 0X0080);
write_cmos_sensor_twobyte(0X602A, 0X09A6);
write_cmos_sensor_twobyte(0X6F12, 0X008C);
write_cmos_sensor_twobyte(0X602A, 0X09AA);
write_cmos_sensor_twobyte(0X6F12, 0X1E7F);
write_cmos_sensor_twobyte(0X6F12, 0X1E7F);
write_cmos_sensor_twobyte(0X6F12, 0XF408);
write_cmos_sensor_twobyte(0X602A, 0X09A4);
write_cmos_sensor_twobyte(0X6F12, 0X007F);
write_cmos_sensor_twobyte(0X602A, 0X09A8);
write_cmos_sensor_twobyte(0X6F12, 0X007F);
write_cmos_sensor_twobyte(0X602A, 0X09DA);
write_cmos_sensor_twobyte(0X6F12, 0X0009);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X6F12, 0XF482);
write_cmos_sensor_twobyte(0X6F12, 0X0009);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X6F12, 0XF484);
write_cmos_sensor_twobyte(0X602A, 0X1666);
write_cmos_sensor_twobyte(0X6F12, 0X054C);
write_cmos_sensor_twobyte(0X6F12, 0X0183);
write_cmos_sensor_twobyte(0X6F12, 0X07FF);
write_cmos_sensor_twobyte(0X602A, 0X16AA);
write_cmos_sensor_twobyte(0X6F12, 0X2608);
write_cmos_sensor_twobyte(0X602A, 0X16AE);
write_cmos_sensor_twobyte(0X6F12, 0X2608);
write_cmos_sensor_twobyte(0X602A, 0X1620);
write_cmos_sensor_twobyte(0X6F12, 0X0408);
write_cmos_sensor_twobyte(0X602A, 0X1622);
write_cmos_sensor_twobyte(0X6F12, 0X0808);
write_cmos_sensor_twobyte(0X602A, 0X1628);
write_cmos_sensor_twobyte(0X6F12, 0X0408);
write_cmos_sensor_twobyte(0X602A, 0X162A);
write_cmos_sensor_twobyte(0X6F12, 0X0808);
write_cmos_sensor_twobyte(0X602A, 0X168A);
write_cmos_sensor_twobyte(0X6F12, 0X001B);
write_cmos_sensor_twobyte(0X602A, 0X1640);
write_cmos_sensor_twobyte(0X6F12, 0X0505);
write_cmos_sensor_twobyte(0X602A, 0X1642);
write_cmos_sensor_twobyte(0X6F12, 0X070E);
write_cmos_sensor_twobyte(0X602A, 0X1648);
write_cmos_sensor_twobyte(0X6F12, 0X0505);
write_cmos_sensor_twobyte(0X602A, 0X164A);
write_cmos_sensor_twobyte(0X6F12, 0X070E);
write_cmos_sensor_twobyte(0X602A, 0X1650);
write_cmos_sensor_twobyte(0X6F12, 0X0A0A);
write_cmos_sensor_twobyte(0X602A, 0X1652);
write_cmos_sensor_twobyte(0X6F12, 0X040A);
write_cmos_sensor_twobyte(0X602A, 0X1658);
write_cmos_sensor_twobyte(0X6F12, 0X0A0A);
write_cmos_sensor_twobyte(0X602A, 0X165A);
write_cmos_sensor_twobyte(0X6F12, 0X040A);
write_cmos_sensor_twobyte(0X602A, 0X15F4);
write_cmos_sensor_twobyte(0X6F12, 0X0004);
write_cmos_sensor_twobyte(0X602A, 0X15E4);
write_cmos_sensor_twobyte(0X6F12, 0X0001);
write_cmos_sensor_twobyte(0X602A, 0X15E6);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X602A, 0X15EA);
write_cmos_sensor_twobyte(0X6F12, 0X0302);
write_cmos_sensor_twobyte(0X602A, 0X0BB8);
write_cmos_sensor_twobyte(0X6F12, 0X01A0);
write_cmos_sensor_twobyte(0X6F12, 0X01A0);
write_cmos_sensor_twobyte(0X602A, 0X0BBE);
write_cmos_sensor_twobyte(0X6F12, 0X01A0);
write_cmos_sensor_twobyte(0X602A, 0X0BC8);
write_cmos_sensor_twobyte(0X6F12, 0X06E0);
write_cmos_sensor_twobyte(0X6F12, 0X0420);
write_cmos_sensor_twobyte(0X602A, 0X0BCE);
write_cmos_sensor_twobyte(0X6F12, 0X0420);
write_cmos_sensor_twobyte(0X602A, 0X0BD8);
write_cmos_sensor_twobyte(0X6F12, 0X000A);
write_cmos_sensor_twobyte(0X6F12, 0X0006);
write_cmos_sensor_twobyte(0X602A, 0X0BDE);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X602A, 0X0A10);
write_cmos_sensor_twobyte(0X6F12, 0X0064);
write_cmos_sensor_twobyte(0X6F12, 0X0520);
write_cmos_sensor_twobyte(0X6F12, 0X06E0);
write_cmos_sensor_twobyte(0X6F12, 0X06DE);
write_cmos_sensor_twobyte(0X602A, 0X15FE);
write_cmos_sensor_twobyte(0X6F12, 0X0001);
write_cmos_sensor_twobyte(0X602A, 0X0C10);
write_cmos_sensor_twobyte(0X6F12, 0X0361);
write_cmos_sensor_twobyte(0X6F12, 0X028A);
write_cmos_sensor_twobyte(0X602A, 0X0C16);
write_cmos_sensor_twobyte(0X6F12, 0X01D8);
write_cmos_sensor_twobyte(0X602A, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X019D);
write_cmos_sensor_twobyte(0X6F12, 0X0114);
write_cmos_sensor_twobyte(0X602A, 0X0C2E);
write_cmos_sensor_twobyte(0X6F12, 0X00BF);
write_cmos_sensor_twobyte(0X6F12, 0X01D5);
write_cmos_sensor_twobyte(0X6F12, 0X014C);
write_cmos_sensor_twobyte(0X602A, 0X0C36);
write_cmos_sensor_twobyte(0X6F12, 0X00F7);
write_cmos_sensor_twobyte(0X6F12, 0X019C);
write_cmos_sensor_twobyte(0X6F12, 0X0113);
write_cmos_sensor_twobyte(0X602A, 0X0C3E);
write_cmos_sensor_twobyte(0X6F12, 0X00BE);
write_cmos_sensor_twobyte(0X6F12, 0X01D6);
write_cmos_sensor_twobyte(0X6F12, 0X014D);
write_cmos_sensor_twobyte(0X602A, 0X0C46);
write_cmos_sensor_twobyte(0X6F12, 0X00F8);
write_cmos_sensor_twobyte(0X6F12, 0X0094);
write_cmos_sensor_twobyte(0X6F12, 0X0072);
write_cmos_sensor_twobyte(0X602A, 0X0C4E);
write_cmos_sensor_twobyte(0X6F12, 0X010F);
write_cmos_sensor_twobyte(0X6F12, 0X007F);
write_cmos_sensor_twobyte(0X6F12, 0X005D);
write_cmos_sensor_twobyte(0X602A, 0X0C56);
write_cmos_sensor_twobyte(0X6F12, 0X00FA);
write_cmos_sensor_twobyte(0X6F12, 0X0069);
write_cmos_sensor_twobyte(0X6F12, 0X005B);
write_cmos_sensor_twobyte(0X602A, 0X0C5E);
write_cmos_sensor_twobyte(0X6F12, 0X005B);
write_cmos_sensor_twobyte(0X602A, 0X0C68);
write_cmos_sensor_twobyte(0X6F12, 0X008D);
write_cmos_sensor_twobyte(0X6F12, 0X006B);
write_cmos_sensor_twobyte(0X602A, 0X0C6E);
write_cmos_sensor_twobyte(0X6F12, 0X0108);
write_cmos_sensor_twobyte(0X6F12, 0X0086);
write_cmos_sensor_twobyte(0X6F12, 0X0064);
write_cmos_sensor_twobyte(0X602A, 0X0C76);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X6F12, 0X0062);
write_cmos_sensor_twobyte(0X6F12, 0X0054);
write_cmos_sensor_twobyte(0X602A, 0X0C7E);
write_cmos_sensor_twobyte(0X6F12, 0X0054);
write_cmos_sensor_twobyte(0X602A, 0X0C88);
write_cmos_sensor_twobyte(0X6F12, 0X0063);
write_cmos_sensor_twobyte(0X6F12, 0X0055);
write_cmos_sensor_twobyte(0X602A, 0X0C8E);
write_cmos_sensor_twobyte(0X6F12, 0X0055);
write_cmos_sensor_twobyte(0X602A, 0X0C98);
write_cmos_sensor_twobyte(0X6F12, 0X008D);
write_cmos_sensor_twobyte(0X6F12, 0X006B);
write_cmos_sensor_twobyte(0X602A, 0X0C9E);
write_cmos_sensor_twobyte(0X6F12, 0X0108);
write_cmos_sensor_twobyte(0X6F12, 0X0086);
write_cmos_sensor_twobyte(0X6F12, 0X0064);
write_cmos_sensor_twobyte(0X602A, 0X0CA6);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X6F12, 0X0062);
write_cmos_sensor_twobyte(0X6F12, 0X0054);
write_cmos_sensor_twobyte(0X602A, 0X0CAE);
write_cmos_sensor_twobyte(0X6F12, 0X0054);
write_cmos_sensor_twobyte(0X602A, 0X0CC0);
write_cmos_sensor_twobyte(0X6F12, 0X019B);
write_cmos_sensor_twobyte(0X6F12, 0X0112);
write_cmos_sensor_twobyte(0X602A, 0X0CC6);
write_cmos_sensor_twobyte(0X6F12, 0X00BD);
write_cmos_sensor_twobyte(0X6F12, 0X0355);
write_cmos_sensor_twobyte(0X6F12, 0X0282);
write_cmos_sensor_twobyte(0X602A, 0X0CCE);
write_cmos_sensor_twobyte(0X6F12, 0X01D5);
write_cmos_sensor_twobyte(0X6F12, 0X019D);
write_cmos_sensor_twobyte(0X6F12, 0X0114);
write_cmos_sensor_twobyte(0X602A, 0X0CD6);
write_cmos_sensor_twobyte(0X6F12, 0X00BF);
write_cmos_sensor_twobyte(0X6F12, 0X0353);
write_cmos_sensor_twobyte(0X6F12, 0X0280);
write_cmos_sensor_twobyte(0X602A, 0X0CDE);
write_cmos_sensor_twobyte(0X6F12, 0X01D3);
write_cmos_sensor_twobyte(0X6F12, 0X019B);
write_cmos_sensor_twobyte(0X6F12, 0X0112);
write_cmos_sensor_twobyte(0X602A, 0X0CE6);
write_cmos_sensor_twobyte(0X6F12, 0X00BD);
write_cmos_sensor_twobyte(0X6F12, 0X01F3);
write_cmos_sensor_twobyte(0X6F12, 0X016A);
write_cmos_sensor_twobyte(0X602A, 0X0CEE);
write_cmos_sensor_twobyte(0X6F12, 0X0113);
write_cmos_sensor_twobyte(0X602A, 0X0CF8);
write_cmos_sensor_twobyte(0X6F12, 0X00DF);
write_cmos_sensor_twobyte(0X6F12, 0X005A);
write_cmos_sensor_twobyte(0X602A, 0X0D08);
write_cmos_sensor_twobyte(0X6F12, 0X00D6);
write_cmos_sensor_twobyte(0X6F12, 0X0051);
write_cmos_sensor_twobyte(0X602A, 0X0D18);
write_cmos_sensor_twobyte(0X6F12, 0X00DD);
write_cmos_sensor_twobyte(0X6F12, 0X0058);
write_cmos_sensor_twobyte(0X602A, 0X0D20);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X602A, 0X0D26);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X6F12, 0X0193);
write_cmos_sensor_twobyte(0X6F12, 0X010A);
write_cmos_sensor_twobyte(0X602A, 0X0D2E);
write_cmos_sensor_twobyte(0X6F12, 0X00BA);
write_cmos_sensor_twobyte(0X602A, 0X0D38);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X602A, 0X0D3E);
write_cmos_sensor_twobyte(0X6F12, 0X0010);
write_cmos_sensor_twobyte(0X6F12, 0X01D5);
write_cmos_sensor_twobyte(0X6F12, 0X014C);
write_cmos_sensor_twobyte(0X602A, 0X0D46);
write_cmos_sensor_twobyte(0X6F12, 0X00F7);
write_cmos_sensor_twobyte(0X6F12, 0X01EB);
write_cmos_sensor_twobyte(0X6F12, 0X0162);
write_cmos_sensor_twobyte(0X602A, 0X0D4E);
write_cmos_sensor_twobyte(0X6F12, 0X010B);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0D56);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0D5E);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0D68);
write_cmos_sensor_twobyte(0X6F12, 0X0361);
write_cmos_sensor_twobyte(0X6F12, 0X028A);
write_cmos_sensor_twobyte(0X602A, 0X0D6E);
write_cmos_sensor_twobyte(0X6F12, 0X01D8);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X602A, 0X0D76);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X602A, 0X0D88);
write_cmos_sensor_twobyte(0X6F12, 0X0361);
write_cmos_sensor_twobyte(0X6F12, 0X028A);
write_cmos_sensor_twobyte(0X602A, 0X0D8E);
write_cmos_sensor_twobyte(0X6F12, 0X01D8);
write_cmos_sensor_twobyte(0X602A, 0X0D98);
write_cmos_sensor_twobyte(0X6F12, 0X0361);
write_cmos_sensor_twobyte(0X6F12, 0X028A);
write_cmos_sensor_twobyte(0X602A, 0X0D9E);
write_cmos_sensor_twobyte(0X6F12, 0X01D8);
write_cmos_sensor_twobyte(0X602A, 0X0DA8);
write_cmos_sensor_twobyte(0X6F12, 0X0355);
write_cmos_sensor_twobyte(0X6F12, 0X0282);
write_cmos_sensor_twobyte(0X602A, 0X0DAE);
write_cmos_sensor_twobyte(0X6F12, 0X01D5);
write_cmos_sensor_twobyte(0X602A, 0X0DB8);
write_cmos_sensor_twobyte(0X6F12, 0X0355);
write_cmos_sensor_twobyte(0X6F12, 0X0282);
write_cmos_sensor_twobyte(0X602A, 0X0DBE);
write_cmos_sensor_twobyte(0X6F12, 0X01D5);
write_cmos_sensor_twobyte(0X6F12, 0X015F);
write_cmos_sensor_twobyte(0X6F12, 0X00DA);
write_cmos_sensor_twobyte(0X602A, 0X0DC6);
write_cmos_sensor_twobyte(0X6F12, 0X008D);
write_cmos_sensor_twobyte(0X6F12, 0X0193);
write_cmos_sensor_twobyte(0X6F12, 0X010A);
write_cmos_sensor_twobyte(0X602A, 0X0DCE);
write_cmos_sensor_twobyte(0X6F12, 0X00BA);
write_cmos_sensor_twobyte(0X6F12, 0X026D);
write_cmos_sensor_twobyte(0X6F12, 0X01E4);
write_cmos_sensor_twobyte(0X602A, 0X0DD6);
write_cmos_sensor_twobyte(0X6F12, 0X013A);
write_cmos_sensor_twobyte(0X6F12, 0X0351);
write_cmos_sensor_twobyte(0X6F12, 0X027E);
write_cmos_sensor_twobyte(0X602A, 0X0DDE);
write_cmos_sensor_twobyte(0X6F12, 0X01D1);
write_cmos_sensor_twobyte(0X6F12, 0X019B);
write_cmos_sensor_twobyte(0X6F12, 0X0112);
write_cmos_sensor_twobyte(0X602A, 0X0DE6);
write_cmos_sensor_twobyte(0X6F12, 0X00C2);
write_cmos_sensor_twobyte(0X6F12, 0X01AD);
write_cmos_sensor_twobyte(0X6F12, 0X0124);
write_cmos_sensor_twobyte(0X602A, 0X0DEE);
write_cmos_sensor_twobyte(0X6F12, 0X00D4);
write_cmos_sensor_twobyte(0X6F12, 0X01A0);
write_cmos_sensor_twobyte(0X6F12, 0X0117);
write_cmos_sensor_twobyte(0X602A, 0X0DF6);
write_cmos_sensor_twobyte(0X6F12, 0X00C7);
write_cmos_sensor_twobyte(0X6F12, 0X01B2);
write_cmos_sensor_twobyte(0X6F12, 0X0129);
write_cmos_sensor_twobyte(0X602A, 0X0DFE);
write_cmos_sensor_twobyte(0X6F12, 0X00D9);
write_cmos_sensor_twobyte(0X6F12, 0X01A5);
write_cmos_sensor_twobyte(0X6F12, 0X011C);
write_cmos_sensor_twobyte(0X602A, 0X0E06);
write_cmos_sensor_twobyte(0X6F12, 0X00CC);
write_cmos_sensor_twobyte(0X6F12, 0X01B2);
write_cmos_sensor_twobyte(0X6F12, 0X0129);
write_cmos_sensor_twobyte(0X602A, 0X0E0E);
write_cmos_sensor_twobyte(0X6F12, 0X00D9);
write_cmos_sensor_twobyte(0X6F12, 0X019B);
write_cmos_sensor_twobyte(0X6F12, 0X0112);
write_cmos_sensor_twobyte(0X602A, 0X0E16);
write_cmos_sensor_twobyte(0X6F12, 0X00C2);
write_cmos_sensor_twobyte(0X6F12, 0X019E);
write_cmos_sensor_twobyte(0X6F12, 0X0115);
write_cmos_sensor_twobyte(0X602A, 0X0E1E);
write_cmos_sensor_twobyte(0X6F12, 0X00C5);
write_cmos_sensor_twobyte(0X602A, 0X0E30);
write_cmos_sensor_twobyte(0X6F12, 0X01A3);
write_cmos_sensor_twobyte(0X6F12, 0X011A);
write_cmos_sensor_twobyte(0X602A, 0X0E36);
write_cmos_sensor_twobyte(0X6F12, 0X00CA);
write_cmos_sensor_twobyte(0X6F12, 0X01B2);
write_cmos_sensor_twobyte(0X6F12, 0X0129);
write_cmos_sensor_twobyte(0X602A, 0X0E3E);
write_cmos_sensor_twobyte(0X6F12, 0X00D9);
write_cmos_sensor_twobyte(0X602A, 0X0E50);
write_cmos_sensor_twobyte(0X6F12, 0X019B);
write_cmos_sensor_twobyte(0X6F12, 0X0112);
write_cmos_sensor_twobyte(0X602A, 0X0E56);
write_cmos_sensor_twobyte(0X6F12, 0X00C2);
write_cmos_sensor_twobyte(0X6F12, 0X019E);
write_cmos_sensor_twobyte(0X6F12, 0X0115);
write_cmos_sensor_twobyte(0X602A, 0X0E5E);
write_cmos_sensor_twobyte(0X6F12, 0X00C5);
write_cmos_sensor_twobyte(0X6F12, 0X00FC);
write_cmos_sensor_twobyte(0X6F12, 0X0077);
write_cmos_sensor_twobyte(0X602A, 0X0E66);
write_cmos_sensor_twobyte(0X6F12, 0X0074);
write_cmos_sensor_twobyte(0X6F12, 0X0196);
write_cmos_sensor_twobyte(0X6F12, 0X010D);
write_cmos_sensor_twobyte(0X602A, 0X0E6E);
write_cmos_sensor_twobyte(0X6F12, 0X00BD);
write_cmos_sensor_twobyte(0X6F12, 0X026B);
write_cmos_sensor_twobyte(0X6F12, 0X01E2);
write_cmos_sensor_twobyte(0X602A, 0X0E76);
write_cmos_sensor_twobyte(0X6F12, 0X0138);
write_cmos_sensor_twobyte(0X6F12, 0X0353);
write_cmos_sensor_twobyte(0X6F12, 0X0280);
write_cmos_sensor_twobyte(0X602A, 0X0E7E);
write_cmos_sensor_twobyte(0X6F12, 0X01D2);
write_cmos_sensor_twobyte(0X602A, 0X0E90);
write_cmos_sensor_twobyte(0X6F12, 0X019A);
write_cmos_sensor_twobyte(0X6F12, 0X0111);
write_cmos_sensor_twobyte(0X602A, 0X0E96);
write_cmos_sensor_twobyte(0X6F12, 0X00C1);
write_cmos_sensor_twobyte(0X6F12, 0X01B0);
write_cmos_sensor_twobyte(0X6F12, 0X0127);
write_cmos_sensor_twobyte(0X602A, 0X0E9E);
write_cmos_sensor_twobyte(0X6F12, 0X00D6);
write_cmos_sensor_twobyte(0X6F12, 0X01B2);
write_cmos_sensor_twobyte(0X6F12, 0X0129);
write_cmos_sensor_twobyte(0X602A, 0X0EA6);
write_cmos_sensor_twobyte(0X6F12, 0X00D8);
write_cmos_sensor_twobyte(0X6F12, 0X01C8);
write_cmos_sensor_twobyte(0X6F12, 0X013F);
write_cmos_sensor_twobyte(0X602A, 0X0EAE);
write_cmos_sensor_twobyte(0X6F12, 0X00ED);
write_cmos_sensor_twobyte(0X6F12, 0X01CA);
write_cmos_sensor_twobyte(0X6F12, 0X0141);
write_cmos_sensor_twobyte(0X602A, 0X0EB6);
write_cmos_sensor_twobyte(0X6F12, 0X00EF);
write_cmos_sensor_twobyte(0X6F12, 0X01E0);
write_cmos_sensor_twobyte(0X6F12, 0X0157);
write_cmos_sensor_twobyte(0X602A, 0X0EBE);
write_cmos_sensor_twobyte(0X6F12, 0X0104);
write_cmos_sensor_twobyte(0X6F12, 0X01E3);
write_cmos_sensor_twobyte(0X6F12, 0X015A);
write_cmos_sensor_twobyte(0X602A, 0X0EC6);
write_cmos_sensor_twobyte(0X6F12, 0X0107);
write_cmos_sensor_twobyte(0X6F12, 0X01F9);
write_cmos_sensor_twobyte(0X6F12, 0X0170);
write_cmos_sensor_twobyte(0X602A, 0X0ECE);
write_cmos_sensor_twobyte(0X6F12, 0X011C);
write_cmos_sensor_twobyte(0X6F12, 0X01FB);
write_cmos_sensor_twobyte(0X6F12, 0X0172);
write_cmos_sensor_twobyte(0X602A, 0X0ED6);
write_cmos_sensor_twobyte(0X6F12, 0X011E);
write_cmos_sensor_twobyte(0X6F12, 0X0211);
write_cmos_sensor_twobyte(0X6F12, 0X0188);
write_cmos_sensor_twobyte(0X602A, 0X0EDE);
write_cmos_sensor_twobyte(0X6F12, 0X0133);
write_cmos_sensor_twobyte(0X6F12, 0X0213);
write_cmos_sensor_twobyte(0X6F12, 0X018A);
write_cmos_sensor_twobyte(0X602A, 0X0EE6);
write_cmos_sensor_twobyte(0X6F12, 0X0135);
write_cmos_sensor_twobyte(0X6F12, 0X0229);
write_cmos_sensor_twobyte(0X6F12, 0X01A0);
write_cmos_sensor_twobyte(0X602A, 0X0EEE);
write_cmos_sensor_twobyte(0X6F12, 0X014A);
write_cmos_sensor_twobyte(0X6F12, 0X022B);
write_cmos_sensor_twobyte(0X6F12, 0X01A2);
write_cmos_sensor_twobyte(0X602A, 0X0EF6);
write_cmos_sensor_twobyte(0X6F12, 0X014C);
write_cmos_sensor_twobyte(0X6F12, 0X0241);
write_cmos_sensor_twobyte(0X6F12, 0X01B8);
write_cmos_sensor_twobyte(0X602A, 0X0EFE);
write_cmos_sensor_twobyte(0X6F12, 0X0161);
write_cmos_sensor_twobyte(0X6F12, 0X0243);
write_cmos_sensor_twobyte(0X6F12, 0X01BA);
write_cmos_sensor_twobyte(0X602A, 0X0F06);
write_cmos_sensor_twobyte(0X6F12, 0X0163);
write_cmos_sensor_twobyte(0X6F12, 0X0259);
write_cmos_sensor_twobyte(0X6F12, 0X01D0);
write_cmos_sensor_twobyte(0X602A, 0X0F0E);
write_cmos_sensor_twobyte(0X6F12, 0X0178);
write_cmos_sensor_twobyte(0X6F12, 0X030E);
write_cmos_sensor_twobyte(0X6F12, 0X0237);
write_cmos_sensor_twobyte(0X602A, 0X0F16);
write_cmos_sensor_twobyte(0X6F12, 0X018B);
write_cmos_sensor_twobyte(0X6F12, 0X0324);
write_cmos_sensor_twobyte(0X6F12, 0X024D);
write_cmos_sensor_twobyte(0X602A, 0X0F1E);
write_cmos_sensor_twobyte(0X6F12, 0X01A0);
write_cmos_sensor_twobyte(0X6F12, 0X0326);
write_cmos_sensor_twobyte(0X6F12, 0X024F);
write_cmos_sensor_twobyte(0X602A, 0X0F26);
write_cmos_sensor_twobyte(0X6F12, 0X01A2);
write_cmos_sensor_twobyte(0X6F12, 0X033C);
write_cmos_sensor_twobyte(0X6F12, 0X0265);
write_cmos_sensor_twobyte(0X602A, 0X0F2E);
write_cmos_sensor_twobyte(0X6F12, 0X01B7);
write_cmos_sensor_twobyte(0X6F12, 0X033E);
write_cmos_sensor_twobyte(0X6F12, 0X0267);
write_cmos_sensor_twobyte(0X602A, 0X0F36);
write_cmos_sensor_twobyte(0X6F12, 0X01B9);
write_cmos_sensor_twobyte(0X6F12, 0X0354);
write_cmos_sensor_twobyte(0X6F12, 0X027D);
write_cmos_sensor_twobyte(0X602A, 0X0F3E);
write_cmos_sensor_twobyte(0X6F12, 0X01CE);
write_cmos_sensor_twobyte(0X602A, 0X0FB0);
write_cmos_sensor_twobyte(0X6F12, 0X019B);
write_cmos_sensor_twobyte(0X6F12, 0X0112);
write_cmos_sensor_twobyte(0X602A, 0X0FB6);
write_cmos_sensor_twobyte(0X6F12, 0X00C2);
write_cmos_sensor_twobyte(0X6F12, 0X01B2);
write_cmos_sensor_twobyte(0X6F12, 0X0129);
write_cmos_sensor_twobyte(0X602A, 0X0FBE);
write_cmos_sensor_twobyte(0X6F12, 0X00D9);
write_cmos_sensor_twobyte(0X602A, 0X0FD0);
write_cmos_sensor_twobyte(0X6F12, 0X019B);
write_cmos_sensor_twobyte(0X6F12, 0X0112);
write_cmos_sensor_twobyte(0X602A, 0X0FD6);
write_cmos_sensor_twobyte(0X6F12, 0X00C2);
write_cmos_sensor_twobyte(0X6F12, 0X019E);
write_cmos_sensor_twobyte(0X6F12, 0X0115);
write_cmos_sensor_twobyte(0X602A, 0X0FDE);
write_cmos_sensor_twobyte(0X6F12, 0X00C5);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X602A, 0X0FE6);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X6F12, 0X0357);
write_cmos_sensor_twobyte(0X6F12, 0X0284);
write_cmos_sensor_twobyte(0X602A, 0X0FEE);
write_cmos_sensor_twobyte(0X6F12, 0X01D7);
write_cmos_sensor_twobyte(0X6F12, 0X00F7);
write_cmos_sensor_twobyte(0X6F12, 0X0072);
write_cmos_sensor_twobyte(0X602A, 0X0FF6);
write_cmos_sensor_twobyte(0X6F12, 0X006F);
write_cmos_sensor_twobyte(0X602A, 0X1008);
write_cmos_sensor_twobyte(0X6F12, 0X00FA);
write_cmos_sensor_twobyte(0X6F12, 0X0075);
write_cmos_sensor_twobyte(0X602A, 0X100E);
write_cmos_sensor_twobyte(0X6F12, 0X0072);
write_cmos_sensor_twobyte(0X6F12, 0X019A);
write_cmos_sensor_twobyte(0X6F12, 0X0111);
write_cmos_sensor_twobyte(0X602A, 0X1016);
write_cmos_sensor_twobyte(0X6F12, 0X00C1);
write_cmos_sensor_twobyte(0X6F12, 0X019D);
write_cmos_sensor_twobyte(0X6F12, 0X0114);
write_cmos_sensor_twobyte(0X602A, 0X101E);
write_cmos_sensor_twobyte(0X6F12, 0X00C4);
write_cmos_sensor_twobyte(0X6F12, 0X0352);
write_cmos_sensor_twobyte(0X6F12, 0X027F);
write_cmos_sensor_twobyte(0X602A, 0X1026);
write_cmos_sensor_twobyte(0X6F12, 0X01D2);
write_cmos_sensor_twobyte(0X6F12, 0X0355);
write_cmos_sensor_twobyte(0X6F12, 0X0282);
write_cmos_sensor_twobyte(0X602A, 0X102E);
write_cmos_sensor_twobyte(0X6F12, 0X01D5);
write_cmos_sensor_twobyte(0X6F12, 0X019A);
write_cmos_sensor_twobyte(0X6F12, 0X0111);
write_cmos_sensor_twobyte(0X602A, 0X1036);
write_cmos_sensor_twobyte(0X6F12, 0X00C1);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X602A, 0X103E);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X6F12, 0X019F);
write_cmos_sensor_twobyte(0X6F12, 0X0116);
write_cmos_sensor_twobyte(0X602A, 0X1046);
write_cmos_sensor_twobyte(0X6F12, 0X00C6);
write_cmos_sensor_twobyte(0X602A, 0X1058);
write_cmos_sensor_twobyte(0X6F12, 0X0013);
write_cmos_sensor_twobyte(0X6F12, 0X0013);
write_cmos_sensor_twobyte(0X602A, 0X105E);
write_cmos_sensor_twobyte(0X6F12, 0X0013);
write_cmos_sensor_twobyte(0X6F12, 0X0353);
write_cmos_sensor_twobyte(0X6F12, 0X0280);
write_cmos_sensor_twobyte(0X602A, 0X1066);
write_cmos_sensor_twobyte(0X6F12, 0X01D3);
write_cmos_sensor_twobyte(0X602A, 0X1098);
write_cmos_sensor_twobyte(0X6F12, 0X01BC);
write_cmos_sensor_twobyte(0X6F12, 0X0133);
write_cmos_sensor_twobyte(0X602A, 0X109E);
write_cmos_sensor_twobyte(0X6F12, 0X00E3);
write_cmos_sensor_twobyte(0X6F12, 0X01C6);
write_cmos_sensor_twobyte(0X6F12, 0X013D);
write_cmos_sensor_twobyte(0X602A, 0X10A6);
write_cmos_sensor_twobyte(0X6F12, 0X00ED);
write_cmos_sensor_twobyte(0X602A, 0X10D8);
write_cmos_sensor_twobyte(0X6F12, 0X019B);
write_cmos_sensor_twobyte(0X6F12, 0X0112);
write_cmos_sensor_twobyte(0X602A, 0X10DE);
write_cmos_sensor_twobyte(0X6F12, 0X00C2);
write_cmos_sensor_twobyte(0X6F12, 0X01AD);
write_cmos_sensor_twobyte(0X6F12, 0X0124);
write_cmos_sensor_twobyte(0X602A, 0X10E6);
write_cmos_sensor_twobyte(0X6F12, 0X00D4);
write_cmos_sensor_twobyte(0X602A, 0X1118);
write_cmos_sensor_twobyte(0X6F12, 0X01D3);
write_cmos_sensor_twobyte(0X6F12, 0X014A);
write_cmos_sensor_twobyte(0X602A, 0X111E);
write_cmos_sensor_twobyte(0X6F12, 0X00F5);
write_cmos_sensor_twobyte(0X602A, 0X1158);
write_cmos_sensor_twobyte(0X6F12, 0X0352);
write_cmos_sensor_twobyte(0X6F12, 0X027F);
write_cmos_sensor_twobyte(0X602A, 0X115E);
write_cmos_sensor_twobyte(0X6F12, 0X01D2);
write_cmos_sensor_twobyte(0X602A, 0X1198);
write_cmos_sensor_twobyte(0X6F12, 0X01A0);
write_cmos_sensor_twobyte(0X6F12, 0X0117);
write_cmos_sensor_twobyte(0X602A, 0X119E);
write_cmos_sensor_twobyte(0X6F12, 0X00C7);
write_cmos_sensor_twobyte(0X6F12, 0X01B2);
write_cmos_sensor_twobyte(0X6F12, 0X0129);
write_cmos_sensor_twobyte(0X602A, 0X11A6);
write_cmos_sensor_twobyte(0X6F12, 0X00D9);
write_cmos_sensor_twobyte(0X602A, 0X11B8);
write_cmos_sensor_twobyte(0X6F12, 0X0014);
write_cmos_sensor_twobyte(0X6F12, 0X0014);
write_cmos_sensor_twobyte(0X602A, 0X11BE);
write_cmos_sensor_twobyte(0X6F12, 0X0014);
write_cmos_sensor_twobyte(0X6F12, 0X0198);
write_cmos_sensor_twobyte(0X6F12, 0X010F);
write_cmos_sensor_twobyte(0X602A, 0X11C6);
write_cmos_sensor_twobyte(0X6F12, 0X00BF);
write_cmos_sensor_twobyte(0X6F12, 0X0275);
write_cmos_sensor_twobyte(0X6F12, 0X01EC);
write_cmos_sensor_twobyte(0X602A, 0X11CE);
write_cmos_sensor_twobyte(0X6F12, 0X0142);
write_cmos_sensor_twobyte(0X6F12, 0X02C1);
write_cmos_sensor_twobyte(0X6F12, 0X0258);
write_cmos_sensor_twobyte(0X602A, 0X11D6);
write_cmos_sensor_twobyte(0X6F12, 0X018A);
write_cmos_sensor_twobyte(0X602A, 0X1258);
write_cmos_sensor_twobyte(0X6F12, 0X000B);
write_cmos_sensor_twobyte(0X6F12, 0X000B);
write_cmos_sensor_twobyte(0X602A, 0X125E);
write_cmos_sensor_twobyte(0X6F12, 0X000B);
write_cmos_sensor_twobyte(0X602A, 0X1268);
write_cmos_sensor_twobyte(0X6F12, 0X01B4);
write_cmos_sensor_twobyte(0X6F12, 0X01B4);
write_cmos_sensor_twobyte(0X602A, 0X126E);
write_cmos_sensor_twobyte(0X6F12, 0X00C1);
write_cmos_sensor_twobyte(0X6F12, 0X01BB);
write_cmos_sensor_twobyte(0X6F12, 0X01BB);
write_cmos_sensor_twobyte(0X602A, 0X1276);
write_cmos_sensor_twobyte(0X6F12, 0X00CB);
write_cmos_sensor_twobyte(0X6F12, 0X01C5);
write_cmos_sensor_twobyte(0X6F12, 0X01C5);
write_cmos_sensor_twobyte(0X602A, 0X127E);
write_cmos_sensor_twobyte(0X6F12, 0X00D5);
write_cmos_sensor_twobyte(0X6F12, 0X01CC);
write_cmos_sensor_twobyte(0X6F12, 0X01CC);
write_cmos_sensor_twobyte(0X602A, 0X1286);
write_cmos_sensor_twobyte(0X6F12, 0X00DF);
write_cmos_sensor_twobyte(0X6F12, 0X01D6);
write_cmos_sensor_twobyte(0X6F12, 0X01D6);
write_cmos_sensor_twobyte(0X602A, 0X128E);
write_cmos_sensor_twobyte(0X6F12, 0X00E9);
write_cmos_sensor_twobyte(0X6F12, 0X01DD);
write_cmos_sensor_twobyte(0X6F12, 0X01DD);
write_cmos_sensor_twobyte(0X602A, 0X1296);
write_cmos_sensor_twobyte(0X6F12, 0X00F3);
write_cmos_sensor_twobyte(0X6F12, 0X01E7);
write_cmos_sensor_twobyte(0X6F12, 0X01E7);
write_cmos_sensor_twobyte(0X602A, 0X129E);
write_cmos_sensor_twobyte(0X6F12, 0X00FD);
write_cmos_sensor_twobyte(0X6F12, 0X01EE);
write_cmos_sensor_twobyte(0X6F12, 0X01EE);
write_cmos_sensor_twobyte(0X602A, 0X12A6);
write_cmos_sensor_twobyte(0X6F12, 0X0107);
write_cmos_sensor_twobyte(0X6F12, 0X01F8);
write_cmos_sensor_twobyte(0X6F12, 0X01F8);
write_cmos_sensor_twobyte(0X602A, 0X12AE);
write_cmos_sensor_twobyte(0X6F12, 0X0111);
write_cmos_sensor_twobyte(0X6F12, 0X01FF);
write_cmos_sensor_twobyte(0X6F12, 0X01FF);
write_cmos_sensor_twobyte(0X602A, 0X12B6);
write_cmos_sensor_twobyte(0X6F12, 0X011B);
write_cmos_sensor_twobyte(0X6F12, 0X0209);
write_cmos_sensor_twobyte(0X6F12, 0X0209);
write_cmos_sensor_twobyte(0X602A, 0X12BE);
write_cmos_sensor_twobyte(0X6F12, 0X0125);
write_cmos_sensor_twobyte(0X6F12, 0X0210);
write_cmos_sensor_twobyte(0X6F12, 0X0210);
write_cmos_sensor_twobyte(0X602A, 0X12C6);
write_cmos_sensor_twobyte(0X6F12, 0X012F);
write_cmos_sensor_twobyte(0X6F12, 0X021A);
write_cmos_sensor_twobyte(0X6F12, 0X021A);
write_cmos_sensor_twobyte(0X602A, 0X12CE);
write_cmos_sensor_twobyte(0X6F12, 0X0139);
write_cmos_sensor_twobyte(0X6F12, 0X0221);
write_cmos_sensor_twobyte(0X6F12, 0X0221);
write_cmos_sensor_twobyte(0X602A, 0X12D6);
write_cmos_sensor_twobyte(0X6F12, 0X0143);
write_cmos_sensor_twobyte(0X6F12, 0X022B);
write_cmos_sensor_twobyte(0X6F12, 0X022B);
write_cmos_sensor_twobyte(0X602A, 0X12DE);
write_cmos_sensor_twobyte(0X6F12, 0X014D);
write_cmos_sensor_twobyte(0X6F12, 0X0232);
write_cmos_sensor_twobyte(0X6F12, 0X0232);
write_cmos_sensor_twobyte(0X602A, 0X12E6);
write_cmos_sensor_twobyte(0X6F12, 0X0157);
write_cmos_sensor_twobyte(0X6F12, 0X023C);
write_cmos_sensor_twobyte(0X6F12, 0X023C);
write_cmos_sensor_twobyte(0X602A, 0X12EE);
write_cmos_sensor_twobyte(0X6F12, 0X0161);
write_cmos_sensor_twobyte(0X6F12, 0X0243);
write_cmos_sensor_twobyte(0X6F12, 0X0243);
write_cmos_sensor_twobyte(0X602A, 0X12F6);
write_cmos_sensor_twobyte(0X6F12, 0X016B);
write_cmos_sensor_twobyte(0X6F12, 0X024D);
write_cmos_sensor_twobyte(0X6F12, 0X024D);
write_cmos_sensor_twobyte(0X602A, 0X12FE);
write_cmos_sensor_twobyte(0X6F12, 0X0175);
write_cmos_sensor_twobyte(0X6F12, 0X0254);
write_cmos_sensor_twobyte(0X6F12, 0X0254);
write_cmos_sensor_twobyte(0X602A, 0X1306);
write_cmos_sensor_twobyte(0X6F12, 0X017F);
write_cmos_sensor_twobyte(0X6F12, 0X025E);
write_cmos_sensor_twobyte(0X6F12, 0X025E);
write_cmos_sensor_twobyte(0X602A, 0X130E);
write_cmos_sensor_twobyte(0X6F12, 0X0189);
write_cmos_sensor_twobyte(0X6F12, 0X0265);
write_cmos_sensor_twobyte(0X6F12, 0X0265);
write_cmos_sensor_twobyte(0X602A, 0X1316);
write_cmos_sensor_twobyte(0X6F12, 0X0193);
write_cmos_sensor_twobyte(0X6F12, 0X026F);
write_cmos_sensor_twobyte(0X6F12, 0X026F);
write_cmos_sensor_twobyte(0X602A, 0X131E);
write_cmos_sensor_twobyte(0X6F12, 0X019D);
write_cmos_sensor_twobyte(0X6F12, 0X0276);
write_cmos_sensor_twobyte(0X6F12, 0X0276);
write_cmos_sensor_twobyte(0X602A, 0X1326);
write_cmos_sensor_twobyte(0X6F12, 0X01A7);
write_cmos_sensor_twobyte(0X6F12, 0X0280);
write_cmos_sensor_twobyte(0X6F12, 0X0280);
write_cmos_sensor_twobyte(0X602A, 0X132E);
write_cmos_sensor_twobyte(0X6F12, 0X01B1);
write_cmos_sensor_twobyte(0X6F12, 0X0287);
write_cmos_sensor_twobyte(0X6F12, 0X0287);
write_cmos_sensor_twobyte(0X602A, 0X1336);
write_cmos_sensor_twobyte(0X6F12, 0X01BB);
write_cmos_sensor_twobyte(0X6F12, 0X01C0);
write_cmos_sensor_twobyte(0X6F12, 0X01C0);
write_cmos_sensor_twobyte(0X602A, 0X133E);
write_cmos_sensor_twobyte(0X6F12, 0X00D0);
write_cmos_sensor_twobyte(0X6F12, 0X01D1);
write_cmos_sensor_twobyte(0X6F12, 0X01D1);
write_cmos_sensor_twobyte(0X602A, 0X1346);
write_cmos_sensor_twobyte(0X6F12, 0X00E4);
write_cmos_sensor_twobyte(0X6F12, 0X01E2);
write_cmos_sensor_twobyte(0X6F12, 0X01E2);
write_cmos_sensor_twobyte(0X602A, 0X134E);
write_cmos_sensor_twobyte(0X6F12, 0X00F8);
write_cmos_sensor_twobyte(0X6F12, 0X01F3);
write_cmos_sensor_twobyte(0X6F12, 0X01F3);
write_cmos_sensor_twobyte(0X602A, 0X1356);
write_cmos_sensor_twobyte(0X6F12, 0X010C);
write_cmos_sensor_twobyte(0X6F12, 0X0204);
write_cmos_sensor_twobyte(0X6F12, 0X0204);
write_cmos_sensor_twobyte(0X602A, 0X135E);
write_cmos_sensor_twobyte(0X6F12, 0X0120);
write_cmos_sensor_twobyte(0X6F12, 0X0215);
write_cmos_sensor_twobyte(0X6F12, 0X0215);
write_cmos_sensor_twobyte(0X602A, 0X1366);
write_cmos_sensor_twobyte(0X6F12, 0X0134);
write_cmos_sensor_twobyte(0X6F12, 0X0226);
write_cmos_sensor_twobyte(0X6F12, 0X0226);
write_cmos_sensor_twobyte(0X602A, 0X136E);
write_cmos_sensor_twobyte(0X6F12, 0X0148);
write_cmos_sensor_twobyte(0X6F12, 0X0237);
write_cmos_sensor_twobyte(0X6F12, 0X0237);
write_cmos_sensor_twobyte(0X602A, 0X1376);
write_cmos_sensor_twobyte(0X6F12, 0X015C);
write_cmos_sensor_twobyte(0X6F12, 0X0248);
write_cmos_sensor_twobyte(0X6F12, 0X0248);
write_cmos_sensor_twobyte(0X602A, 0X137E);
write_cmos_sensor_twobyte(0X6F12, 0X0170);
write_cmos_sensor_twobyte(0X6F12, 0X0259);
write_cmos_sensor_twobyte(0X6F12, 0X0259);
write_cmos_sensor_twobyte(0X602A, 0X1386);
write_cmos_sensor_twobyte(0X6F12, 0X0184);
write_cmos_sensor_twobyte(0X6F12, 0X026A);
write_cmos_sensor_twobyte(0X6F12, 0X026A);
write_cmos_sensor_twobyte(0X602A, 0X138E);
write_cmos_sensor_twobyte(0X6F12, 0X0198);
write_cmos_sensor_twobyte(0X6F12, 0X027B);
write_cmos_sensor_twobyte(0X6F12, 0X027B);
write_cmos_sensor_twobyte(0X602A, 0X1396);
write_cmos_sensor_twobyte(0X6F12, 0X01AC);
write_cmos_sensor_twobyte(0X6F12, 0X028C);
write_cmos_sensor_twobyte(0X6F12, 0X028C);
write_cmos_sensor_twobyte(0X602A, 0X139E);
write_cmos_sensor_twobyte(0X6F12, 0X01C0);
write_cmos_sensor_twobyte(0X602A, 0X14B8);
write_cmos_sensor_twobyte(0X6F12, 0X00BE);
write_cmos_sensor_twobyte(0X6F12, 0X0096);
write_cmos_sensor_twobyte(0X602A, 0X14BE);
write_cmos_sensor_twobyte(0X6F12, 0X0105);
write_cmos_sensor_twobyte(0X6F12, 0X0024);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X602A, 0X14C6);
write_cmos_sensor_twobyte(0X6F12, 0X0012);
write_cmos_sensor_twobyte(0X602A, 0X14D0);
write_cmos_sensor_twobyte(0X6F12, 0X019A);
write_cmos_sensor_twobyte(0X6F12, 0X0111);
write_cmos_sensor_twobyte(0X602A, 0X14D6);
write_cmos_sensor_twobyte(0X6F12, 0X00C1);
write_cmos_sensor_twobyte(0X6F12, 0X0087);
write_cmos_sensor_twobyte(0X6F12, 0X0065);
write_cmos_sensor_twobyte(0X602A, 0X14DE);
write_cmos_sensor_twobyte(0X6F12, 0X0102);
write_cmos_sensor_twobyte(0X6F12, 0X0362);
write_cmos_sensor_twobyte(0X6F12, 0X028B);
write_cmos_sensor_twobyte(0X602A, 0X14E6);
write_cmos_sensor_twobyte(0X6F12, 0X01D9);
write_cmos_sensor_twobyte(0X6F12, 0X001A);
write_cmos_sensor_twobyte(0X6F12, 0X001A);
write_cmos_sensor_twobyte(0X602A, 0X14EE);
write_cmos_sensor_twobyte(0X6F12, 0X001A);
write_cmos_sensor_twobyte(0X602A, 0X1508);
write_cmos_sensor_twobyte(0X6F12, 0X0603);
write_cmos_sensor_twobyte(0X602A, 0X1550);
write_cmos_sensor_twobyte(0X6F12, 0X0306);
write_cmos_sensor_twobyte(0X602A, 0X1552);
write_cmos_sensor_twobyte(0X6F12, 0X0606);
write_cmos_sensor_twobyte(0X602A, 0X1554);
write_cmos_sensor_twobyte(0X6F12, 0X0606);
write_cmos_sensor_twobyte(0X602A, 0X1556);
write_cmos_sensor_twobyte(0X6F12, 0X0603);
write_cmos_sensor_twobyte(0X602A, 0X15DC);
write_cmos_sensor_twobyte(0X6F12, 0X0001);
write_cmos_sensor_twobyte(0X602A, 0X15DE);
write_cmos_sensor_twobyte(0X6F12, 0X0107);
write_cmos_sensor_twobyte(0X602A, 0X0A96);
write_cmos_sensor_twobyte(0X6F12, 0X6E33);
write_cmos_sensor_twobyte(0X602A, 0X2800);
write_cmos_sensor_twobyte(0X6F12, 0X0245);
write_cmos_sensor_twobyte(0X6F12, 0X0105);
write_cmos_sensor_twobyte(0X602A, 0X2816);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X6F12, 0X0180);
write_cmos_sensor_twobyte(0X602A, 0X2814);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X2824);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0998);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X55BE);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X55C0);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X51D0);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X51E0);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X51F0);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X2A12);
write_cmos_sensor_twobyte(0X6F12, 0X0102);
write_cmos_sensor_twobyte(0X602A, 0X2C68);
write_cmos_sensor_twobyte(0X6F12, 0X0005);
write_cmos_sensor_twobyte(0X6F12, 0X0005);
write_cmos_sensor_twobyte(0X6F12, 0X0005);
write_cmos_sensor_twobyte(0X6F12, 0X0005);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X6F12, 0X0005);
write_cmos_sensor_twobyte(0X6F12, 0X0005);
write_cmos_sensor_twobyte(0X6F12, 0X0005);
write_cmos_sensor_twobyte(0X6F12, 0X0005);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X6F12, 0X0003);
write_cmos_sensor_twobyte(0X602A, 0X372C);
write_cmos_sensor_twobyte(0X6F12, 0X000F);
write_cmos_sensor_twobyte(0X6F12, 0X000D);
write_cmos_sensor_twobyte(0X6F12, 0X000F);
write_cmos_sensor_twobyte(0X6F12, 0X000D);
write_cmos_sensor_twobyte(0X6F12, 0X000B);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X6F12, 0X000B);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X6F12, 0X000F);
write_cmos_sensor_twobyte(0X6F12, 0X000D);
write_cmos_sensor_twobyte(0X6F12, 0X000F);
write_cmos_sensor_twobyte(0X6F12, 0X000D);
write_cmos_sensor_twobyte(0X6F12, 0X000B);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X6F12, 0X000B);
write_cmos_sensor_twobyte(0X6F12, 0X000C);
write_cmos_sensor_twobyte(0X602A, 0X41F0);
write_cmos_sensor_twobyte(0X6F12, 0X001B);
write_cmos_sensor_twobyte(0X6F12, 0X001A);
write_cmos_sensor_twobyte(0X6F12, 0X001B);
write_cmos_sensor_twobyte(0X6F12, 0X001A);
write_cmos_sensor_twobyte(0X6F12, 0X0017);
write_cmos_sensor_twobyte(0X6F12, 0X0014);
write_cmos_sensor_twobyte(0X6F12, 0X0017);
write_cmos_sensor_twobyte(0X6F12, 0X0014);
write_cmos_sensor_twobyte(0X6F12, 0X001B);
write_cmos_sensor_twobyte(0X6F12, 0X001A);
write_cmos_sensor_twobyte(0X6F12, 0X001B);
write_cmos_sensor_twobyte(0X6F12, 0X001A);
write_cmos_sensor_twobyte(0X6F12, 0X0017);
write_cmos_sensor_twobyte(0X6F12, 0X0014);
write_cmos_sensor_twobyte(0X6F12, 0X0017);
write_cmos_sensor_twobyte(0X6F12, 0X0014);
write_cmos_sensor_twobyte(0X602A, 0X8A50);
write_cmos_sensor_twobyte(0X6F12, 0X00E8);
write_cmos_sensor_twobyte(0X602A, 0X0A80);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X3110);
write_cmos_sensor_twobyte(0X602A, 0X51D2);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X51E2);
write_cmos_sensor_twobyte(0X6F12, 0X0800);
write_cmos_sensor_twobyte(0X602A, 0X51F2);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X602A, 0X5840);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0A50);
write_cmos_sensor_twobyte(0X6F12, 0X2100);
write_cmos_sensor_twobyte(0X6F12, 0X2100);
//Split
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X7970);
write_cmos_sensor_twobyte(0X6218, 0X7150);
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X0C10);
write_cmos_sensor_twobyte(0X6F12, 0X0361);
write_cmos_sensor_twobyte(0X602A, 0X0CC8);
write_cmos_sensor_twobyte(0X6F12, 0X0355);
write_cmos_sensor_twobyte(0X602A, 0X0CD8);
write_cmos_sensor_twobyte(0X6F12, 0X0353);
write_cmos_sensor_twobyte(0X602A, 0X0D68);
write_cmos_sensor_twobyte(0X6F12, 0X0361);
write_cmos_sensor_twobyte(0X602A, 0X0D88);
write_cmos_sensor_twobyte(0X6F12, 0X0361);
write_cmos_sensor_twobyte(0X602A, 0X0D98);
write_cmos_sensor_twobyte(0X6F12, 0X0361);
write_cmos_sensor_twobyte(0X602A, 0X0DA8);
write_cmos_sensor_twobyte(0X6F12, 0X0355);
write_cmos_sensor_twobyte(0X602A, 0X0DB8);
write_cmos_sensor_twobyte(0X6F12, 0X0355);
write_cmos_sensor_twobyte(0X602A, 0X0DD8);
write_cmos_sensor_twobyte(0X6F12, 0X0351);
write_cmos_sensor_twobyte(0X602A, 0X0E78);
write_cmos_sensor_twobyte(0X6F12, 0X0353);
write_cmos_sensor_twobyte(0X602A, 0X0F10);
write_cmos_sensor_twobyte(0X6F12, 0X030E);
write_cmos_sensor_twobyte(0X602A, 0X0F18);
write_cmos_sensor_twobyte(0X6F12, 0X0324);
write_cmos_sensor_twobyte(0X602A, 0X0F20);
write_cmos_sensor_twobyte(0X6F12, 0X0326);
write_cmos_sensor_twobyte(0X602A, 0X0F28);
write_cmos_sensor_twobyte(0X6F12, 0X033C);
write_cmos_sensor_twobyte(0X602A, 0X0F30);
write_cmos_sensor_twobyte(0X6F12, 0X033E);
write_cmos_sensor_twobyte(0X602A, 0X0F38);
write_cmos_sensor_twobyte(0X6F12, 0X0354);
write_cmos_sensor_twobyte(0X602A, 0X0FE8);
write_cmos_sensor_twobyte(0X6F12, 0X0357);
write_cmos_sensor_twobyte(0X602A, 0X1020);
write_cmos_sensor_twobyte(0X6F12, 0X0352);
write_cmos_sensor_twobyte(0X602A, 0X1028);
write_cmos_sensor_twobyte(0X6F12, 0X0355);
write_cmos_sensor_twobyte(0X602A, 0X1060);
write_cmos_sensor_twobyte(0X6F12, 0X0353);
write_cmos_sensor_twobyte(0X602A, 0X1158);
write_cmos_sensor_twobyte(0X6F12, 0X0352);
write_cmos_sensor_twobyte(0X602A, 0X14B8);
write_cmos_sensor_twobyte(0X6F12, 0X00BE);
write_cmos_sensor_twobyte(0X602A, 0X14E0);
write_cmos_sensor_twobyte(0X6F12, 0X0362);




write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X0990);
write_cmos_sensor_twobyte(0X6F12, 0X0020); // out pedestal 32




}

static void preview_setting_11_new(void)
{
	LOG_INF("preview_setting_11_new");

	// Stream Off
	write_cmos_sensor(0x0100,0x00);

    while(1)
    {
        if( read_cmos_sensor(0x0005)==0xFF) break;

    }


	//no bin
#ifdef SENSOR_M1
	// M1 setting
//$MIPI[Width:4032,Height:1512,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:2034,useEmbData:0]
//$MV1[MCLK:24,Width:4032,Height:1512,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:2034,pvi_pclk_inverse:0]
write_cmos_sensor_twobyte(0X6028, 0X4000);


write_cmos_sensor_twobyte(0X6214, 0X7970); // Clk on
write_cmos_sensor_twobyte(0X6218, 0X7150);
write_cmos_sensor_twobyte(0X3064, 0X0020);

//Mode
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X3064, 0X0020);
write_cmos_sensor_twobyte(0X0344, 0X0000);
write_cmos_sensor_twobyte(0X0346, 0X0000);
write_cmos_sensor_twobyte(0X0348, 0X1F7F);
write_cmos_sensor_twobyte(0X034A, 0X0BD7);
write_cmos_sensor_twobyte(0X034C, 0X0FC0);
write_cmos_sensor_twobyte(0X034E, 0X05E8);
write_cmos_sensor_twobyte(0X0408, 0X0000);
write_cmos_sensor_twobyte(0X040A, 0X0004);
write_cmos_sensor_twobyte(0X0900, 0X0122);
write_cmos_sensor_twobyte(0X0380, 0X0001);
write_cmos_sensor_twobyte(0X0382, 0X0003);
write_cmos_sensor_twobyte(0X0384, 0X0001);
write_cmos_sensor_twobyte(0X0386, 0X0003);
write_cmos_sensor_twobyte(0X0400, 0X0000);
write_cmos_sensor_twobyte(0X0404, 0X0010);
write_cmos_sensor_twobyte(0X3060, 0X0100);
write_cmos_sensor_twobyte(0X0114, 0X0300);
write_cmos_sensor_twobyte(0X0110, 0X1002);
write_cmos_sensor_twobyte(0X0136, 0X1800);
write_cmos_sensor_twobyte(0X0304, 0X0006);
write_cmos_sensor_twobyte(0X0306, 0X01E0);
write_cmos_sensor_twobyte(0X0302, 0X0001);
write_cmos_sensor_twobyte(0X0300, 0X0004);
write_cmos_sensor_twobyte(0X030C, 0X0001);
write_cmos_sensor_twobyte(0X030E, 0X0004);
write_cmos_sensor_twobyte(0X0310, 0X00A9);// for PV only
write_cmos_sensor_twobyte(0X0312, 0X0000);
write_cmos_sensor_twobyte(0X030A, 0X0001);
write_cmos_sensor_twobyte(0X0308, 0X0008);
write_cmos_sensor_twobyte(0X0342, 0X27B0);
write_cmos_sensor_twobyte(0X0340, 0X0C4D);
write_cmos_sensor_twobyte(0X021E, 0X0000);
write_cmos_sensor_twobyte(0X3098, 0X0400);
write_cmos_sensor_twobyte(0X309A, 0X0002);
write_cmos_sensor_twobyte(0X30BC, 0X0031);
write_cmos_sensor_twobyte(0XF41E, 0X2180);
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X0990);
write_cmos_sensor_twobyte(0X6F12, 0X0020);
write_cmos_sensor_twobyte(0X602A, 0X0B16);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X16B4);
write_cmos_sensor_twobyte(0X6F12, 0X54C2);
write_cmos_sensor_twobyte(0X6F12, 0X12AF);
write_cmos_sensor_twobyte(0X6F12, 0X0328);
write_cmos_sensor_twobyte(0X602A, 0X1688);
write_cmos_sensor_twobyte(0X6F12, 0X00A0);
write_cmos_sensor_twobyte(0X602A, 0X168C);
write_cmos_sensor_twobyte(0X6F12, 0X0028);
write_cmos_sensor_twobyte(0X6F12, 0X0030);
write_cmos_sensor_twobyte(0X6F12, 0X0C18);
write_cmos_sensor_twobyte(0X6F12, 0X0C18);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C20);
write_cmos_sensor_twobyte(0X6F12, 0X0C20);
write_cmos_sensor_twobyte(0X6F12, 0X0C30);
write_cmos_sensor_twobyte(0X6F12, 0X0C30);
write_cmos_sensor_twobyte(0X602A, 0X0A94);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X602A, 0X5840);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X16BC);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X06C0);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X602A, 0X27A8);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0B62);
write_cmos_sensor_twobyte(0X6F12, 0X0146);
write_cmos_sensor_twobyte(0X602A, 0X0B4A);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X602A, 0X0AEC);
write_cmos_sensor_twobyte(0X6F12, 0X0207);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29D8);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29E2);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2958);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2998);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2962);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29A2);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X79F0);
write_cmos_sensor_twobyte(0X6218, 0X79F0);




#else
	// M2 setting




#endif

	// Stream On
	write_cmos_sensor_twobyte(0x0100,0x0100);

	mDELAY(10);


}

#ifndef MARK_HDR
static void capture_setting_WDR(kal_uint16 currefps)
{


	write_cmos_sensor_twobyte(0x0100, 0x0000);
	LOG_INF("Capture WDR(fps = %d)",currefps);
    while(1)
    {
        if( read_cmos_sensor(0x0005)==0xFF) break;

    }

	#ifdef SENSOR_M1
//$MIPI[Width:8064,Height:3024,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:2034,useEmbData:0]
//$MV1[MCLK:24,Width:8064,Height:3024,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:2034,pvi_pclk_inverse:0]
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X7970); // Clk on
write_cmos_sensor_twobyte(0X6218, 0X7150);
write_cmos_sensor_twobyte(0X3064, 0X0020);
//Mode
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X3064, 0X0020);
write_cmos_sensor_twobyte(0X0344, 0X0000);
write_cmos_sensor_twobyte(0X0346, 0X0000);
write_cmos_sensor_twobyte(0X0348, 0X1F7F);
write_cmos_sensor_twobyte(0X034A, 0X0BD7);
write_cmos_sensor_twobyte(0X034C, 0X1F80);
write_cmos_sensor_twobyte(0X034E, 0X0BD0);
write_cmos_sensor_twobyte(0X0408, 0X0000);
write_cmos_sensor_twobyte(0X040A, 0X0008);
write_cmos_sensor_twobyte(0X0900, 0X0011);
write_cmos_sensor_twobyte(0X0380, 0X0001);
write_cmos_sensor_twobyte(0X0382, 0X0001);
write_cmos_sensor_twobyte(0X0384, 0X0001);
write_cmos_sensor_twobyte(0X0386, 0X0001);
write_cmos_sensor_twobyte(0X0400, 0X0000);
write_cmos_sensor_twobyte(0X0404, 0X0010);
write_cmos_sensor_twobyte(0X3060, 0X0100);
write_cmos_sensor_twobyte(0X0114, 0X0300);
write_cmos_sensor_twobyte(0X0110, 0X1002);
write_cmos_sensor_twobyte(0X0136, 0X1800);
write_cmos_sensor_twobyte(0X0304, 0X0006);
write_cmos_sensor_twobyte(0X0306, 0X01E0);
write_cmos_sensor_twobyte(0X0302, 0X0001);
write_cmos_sensor_twobyte(0X0300, 0X0004);
write_cmos_sensor_twobyte(0X030C, 0X0001);
write_cmos_sensor_twobyte(0X030E, 0X0004);
write_cmos_sensor_twobyte(0X0310, 0X0153);
write_cmos_sensor_twobyte(0X0312, 0X0000);
write_cmos_sensor_twobyte(0X030A, 0X0001);
write_cmos_sensor_twobyte(0X0308, 0X0008);
write_cmos_sensor_twobyte(0X0342, 0X27E0);
write_cmos_sensor_twobyte(0X0340, 0X0C3E);
write_cmos_sensor_twobyte(0X021E, 0X0000);
write_cmos_sensor_twobyte(0X3098, 0X0400);
write_cmos_sensor_twobyte(0X309A, 0X0002);
write_cmos_sensor_twobyte(0X30BC, 0X0031);
write_cmos_sensor_twobyte(0XF41E, 0X2180);
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X0990);
write_cmos_sensor_twobyte(0X6F12, 0X0020);
write_cmos_sensor_twobyte(0X602A, 0X0B16);
write_cmos_sensor_twobyte(0X6F12, 0X0200);
write_cmos_sensor_twobyte(0X602A, 0X16B4);
write_cmos_sensor_twobyte(0X6F12, 0X54C2);
write_cmos_sensor_twobyte(0X6F12, 0X122F);
write_cmos_sensor_twobyte(0X6F12, 0X4328);
write_cmos_sensor_twobyte(0X602A, 0X1688);
write_cmos_sensor_twobyte(0X6F12, 0X00A2);
write_cmos_sensor_twobyte(0X602A, 0X168C);
write_cmos_sensor_twobyte(0X6F12, 0X0028);
write_cmos_sensor_twobyte(0X6F12, 0X0030);
write_cmos_sensor_twobyte(0X6F12, 0X0C18);
write_cmos_sensor_twobyte(0X6F12, 0X0C18);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C20);
write_cmos_sensor_twobyte(0X6F12, 0X0C20);
write_cmos_sensor_twobyte(0X6F12, 0X0C30);
write_cmos_sensor_twobyte(0X6F12, 0X0C30);
write_cmos_sensor_twobyte(0X602A, 0X0A94);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X5840);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X16BC);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X06C0);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X602A, 0X27A8);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X0B62);
write_cmos_sensor_twobyte(0X6F12, 0X0146);
write_cmos_sensor_twobyte(0X602A, 0X0B4A);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X602A, 0X0AEC);
write_cmos_sensor_twobyte(0X6F12, 0X0207);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29D8);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29E2);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2958);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2998);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2962);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29A2);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X79F0);
write_cmos_sensor_twobyte(0X6218, 0X79F0);



#else
// M2_full_size_setting
//TBD
#endif

	/*Streaming  output */
	write_cmos_sensor_twobyte(0x0100, 0x0100);

}
#endif

static void capture_setting(void)
{

	LOG_INF("capture_setting");
	// Stream Off
	write_cmos_sensor(0x0100,0x00);
    while(1)
    {
        if( read_cmos_sensor(0x0005)==0xFF) break;

    }
//no bin
#ifdef SENSOR_M1
	// M1_fullsize_setting
//$MIPI[Width:8064,Height:3024,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:2034,useEmbData:0]
//$MV1[MCLK:24,Width:8064,Height:3024,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:2034,pvi_pclk_inverse:0]
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X7970); // Clk on
write_cmos_sensor_twobyte(0X6218, 0X7150);
write_cmos_sensor_twobyte(0X3064, 0X0020);
//Mode
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X3064, 0X0020);
write_cmos_sensor_twobyte(0X0344, 0X0000);
write_cmos_sensor_twobyte(0X0346, 0X0000);
write_cmos_sensor_twobyte(0X0348, 0X1F7F);
write_cmos_sensor_twobyte(0X034A, 0X0BD7);
write_cmos_sensor_twobyte(0X034C, 0X1F80);
write_cmos_sensor_twobyte(0X034E, 0X0BD0);
write_cmos_sensor_twobyte(0X0408, 0X0000);
write_cmos_sensor_twobyte(0X040A, 0X0008);
write_cmos_sensor_twobyte(0X0900, 0X0011);
write_cmos_sensor_twobyte(0X0380, 0X0001);
write_cmos_sensor_twobyte(0X0382, 0X0001);
write_cmos_sensor_twobyte(0X0384, 0X0001);
write_cmos_sensor_twobyte(0X0386, 0X0001);
write_cmos_sensor_twobyte(0X0400, 0X0000);
write_cmos_sensor_twobyte(0X0404, 0X0010);
write_cmos_sensor_twobyte(0X3060, 0X0100);
write_cmos_sensor_twobyte(0X0114, 0X0300);
write_cmos_sensor_twobyte(0X0110, 0X1002);
write_cmos_sensor_twobyte(0X0136, 0X1800);
write_cmos_sensor_twobyte(0X0304, 0X0006);
write_cmos_sensor_twobyte(0X0306, 0X01E0);
write_cmos_sensor_twobyte(0X0302, 0X0001);
write_cmos_sensor_twobyte(0X0300, 0X0004);
write_cmos_sensor_twobyte(0X030C, 0X0001);
write_cmos_sensor_twobyte(0X030E, 0X0004);
write_cmos_sensor_twobyte(0X0310, 0X0153);
write_cmos_sensor_twobyte(0X0312, 0X0000);
write_cmos_sensor_twobyte(0X030A, 0X0001);
write_cmos_sensor_twobyte(0X0308, 0X0008);
write_cmos_sensor_twobyte(0X0342, 0X27E0);
write_cmos_sensor_twobyte(0X0340, 0X0C3E);
write_cmos_sensor_twobyte(0X021E, 0X0000);
write_cmos_sensor_twobyte(0X3098, 0X0400);
write_cmos_sensor_twobyte(0X309A, 0X0002);
write_cmos_sensor_twobyte(0X30BC, 0X0031);
write_cmos_sensor_twobyte(0XF41E, 0X2180);
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X0990);
write_cmos_sensor_twobyte(0X6F12, 0X0020);
write_cmos_sensor_twobyte(0X602A, 0X0B16);
write_cmos_sensor_twobyte(0X6F12, 0X0200);
write_cmos_sensor_twobyte(0X602A, 0X16B4);
write_cmos_sensor_twobyte(0X6F12, 0X54C2);
write_cmos_sensor_twobyte(0X6F12, 0X122F);
write_cmos_sensor_twobyte(0X6F12, 0X4328);
write_cmos_sensor_twobyte(0X602A, 0X1688);
write_cmos_sensor_twobyte(0X6F12, 0X00A2);
write_cmos_sensor_twobyte(0X602A, 0X168C);
write_cmos_sensor_twobyte(0X6F12, 0X0028);
write_cmos_sensor_twobyte(0X6F12, 0X0030);
write_cmos_sensor_twobyte(0X6F12, 0X0C18);
write_cmos_sensor_twobyte(0X6F12, 0X0C18);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C20);
write_cmos_sensor_twobyte(0X6F12, 0X0C20);
write_cmos_sensor_twobyte(0X6F12, 0X0C30);
write_cmos_sensor_twobyte(0X6F12, 0X0C30);
write_cmos_sensor_twobyte(0X602A, 0X0A94);
write_cmos_sensor_twobyte(0X6F12, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X5840);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X16BC);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X06C0);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X602A, 0X27A8);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X0B62);
write_cmos_sensor_twobyte(0X6F12, 0X0146);
write_cmos_sensor_twobyte(0X602A, 0X0B4A);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X602A, 0X0AEC);
write_cmos_sensor_twobyte(0X6F12, 0X0207);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29D8);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29E2);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2958);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2998);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2962);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29A2);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X79F0);
write_cmos_sensor_twobyte(0X6218, 0X79F0);


#else

// M2_full_size_setting
//TBD


#endif

	// Stream On
	write_cmos_sensor_twobyte(0x0100,0x0100);

	mDELAY(10);
}

#if 0
static void normal_video_setting_11_new(kal_uint16 currefps)
{
	LOG_INF("Dual PD capture");
	// Stream Off
	write_cmos_sensor(0x0100,0x00);
#ifdef SENSOR_M1
	// M1_bining
//$MIPI[Width:4032,Height:1512,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:2034,useEmbData:0]
//$MV1[MCLK:24,Width:4032,Height:1512,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:2034,pvi_pclk_inverse:0]
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X7970); // Clk on
write_cmos_sensor_twobyte(0X6218, 0X7150);
write_cmos_sensor_twobyte(0X3064, 0X0020);

//Mode
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X3064, 0X0020);
write_cmos_sensor_twobyte(0X0344, 0X0000);
write_cmos_sensor_twobyte(0X0346, 0X0000);
write_cmos_sensor_twobyte(0X0348, 0X1F7F);
write_cmos_sensor_twobyte(0X034A, 0X0BD7);
write_cmos_sensor_twobyte(0X034C, 0X0FC0);
write_cmos_sensor_twobyte(0X034E, 0X05E8);
write_cmos_sensor_twobyte(0X0408, 0X0000);
write_cmos_sensor_twobyte(0X040A, 0X0004);
write_cmos_sensor_twobyte(0X0900, 0X0122);
write_cmos_sensor_twobyte(0X0380, 0X0001);
write_cmos_sensor_twobyte(0X0382, 0X0003);
write_cmos_sensor_twobyte(0X0384, 0X0001);
write_cmos_sensor_twobyte(0X0386, 0X0003);
write_cmos_sensor_twobyte(0X0400, 0X0000);
write_cmos_sensor_twobyte(0X0404, 0X0010);
write_cmos_sensor_twobyte(0X3060, 0X0100);
write_cmos_sensor_twobyte(0X0114, 0X0300);
write_cmos_sensor_twobyte(0X0110, 0X1002);
write_cmos_sensor_twobyte(0X0136, 0X1800);
write_cmos_sensor_twobyte(0X0304, 0X0006);
write_cmos_sensor_twobyte(0X0306, 0X01E0);
write_cmos_sensor_twobyte(0X0302, 0X0001);
write_cmos_sensor_twobyte(0X0300, 0X0004);
write_cmos_sensor_twobyte(0X030C, 0X0001);
write_cmos_sensor_twobyte(0X030E, 0X0004);
write_cmos_sensor_twobyte(0X0310, 0X0153);
write_cmos_sensor_twobyte(0X0312, 0X0000);
write_cmos_sensor_twobyte(0X030A, 0X0001);
write_cmos_sensor_twobyte(0X0308, 0X0008);
write_cmos_sensor_twobyte(0X0342, 0X27B0);
write_cmos_sensor_twobyte(0X0340, 0X0C4D);
write_cmos_sensor_twobyte(0X021E, 0X0000);
write_cmos_sensor_twobyte(0X3098, 0X0400);
write_cmos_sensor_twobyte(0X309A, 0X0002);
write_cmos_sensor_twobyte(0X30BC, 0X0031);
write_cmos_sensor_twobyte(0XF41E, 0X2180);
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X0990);
write_cmos_sensor_twobyte(0X6F12, 0X0020);
write_cmos_sensor_twobyte(0X602A, 0X0B16);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X16B4);
write_cmos_sensor_twobyte(0X6F12, 0X54C2);
write_cmos_sensor_twobyte(0X6F12, 0X12AF);
write_cmos_sensor_twobyte(0X6F12, 0X0328);
write_cmos_sensor_twobyte(0X602A, 0X1688);
write_cmos_sensor_twobyte(0X6F12, 0X00A0);
write_cmos_sensor_twobyte(0X602A, 0X168C);
write_cmos_sensor_twobyte(0X6F12, 0X0028);
write_cmos_sensor_twobyte(0X6F12, 0X0030);
write_cmos_sensor_twobyte(0X6F12, 0X0C18);
write_cmos_sensor_twobyte(0X6F12, 0X0C18);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C20);
write_cmos_sensor_twobyte(0X6F12, 0X0C20);
write_cmos_sensor_twobyte(0X6F12, 0X0C30);
write_cmos_sensor_twobyte(0X6F12, 0X0C30);
write_cmos_sensor_twobyte(0X602A, 0X0A94);
write_cmos_sensor_twobyte(0X6F12, 0X1000);
write_cmos_sensor_twobyte(0X602A, 0X5840);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X16BC);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X06C0);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X602A, 0X27A8);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0B62);
write_cmos_sensor_twobyte(0X6F12, 0X0146);
write_cmos_sensor_twobyte(0X602A, 0X0B4A);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X602A, 0X0AEC);
write_cmos_sensor_twobyte(0X6F12, 0X0207);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29D8);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29E2);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2958);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2998);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X2962);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X602A, 0X29A2);
write_cmos_sensor_twobyte(0X6F12, 0X0BE0);
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X79F0);
write_cmos_sensor_twobyte(0X6218, 0X79F0);


#else
	// M2_full_size_setting
//TBD


#endif

		// Stream On
	write_cmos_sensor_twobyte(0x0100,0x0100);

	mDELAY(10);

}
#endif

static void hs_video_setting_11(void)
{
	LOG_INF("hs_video_setting_11");

// Stream Off
write_cmos_sensor(0x0100,0x00);
while(1)
{
    if( read_cmos_sensor(0x0005)==0xFF) break;

}

//$MIPI[Width:2688,Height:756,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:2034,useEmbData:0]
//$MV1[MCLK:24,Width:2688,Height:756,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:2034,pvi_pclk_inverse:0]
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X6214, 0X7970); // Clk on
write_cmos_sensor_twobyte(0X6218, 0X7150);
write_cmos_sensor_twobyte(0X3064, 0X0020);
//Mode
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X3064, 0X0020);
write_cmos_sensor_twobyte(0X0344, 0X0000);
write_cmos_sensor_twobyte(0X0346, 0X015C);
write_cmos_sensor_twobyte(0X0348, 0X1F7F);
write_cmos_sensor_twobyte(0X034A, 0X0A43);
write_cmos_sensor_twobyte(0X034C, 0X0A80);
write_cmos_sensor_twobyte(0X034E, 0X02F4);
write_cmos_sensor_twobyte(0X0408, 0X0000);
write_cmos_sensor_twobyte(0X040A, 0X0004);
write_cmos_sensor_twobyte(0X0900, 0X0113);
write_cmos_sensor_twobyte(0X0380, 0X0001);
write_cmos_sensor_twobyte(0X0382, 0X0001);
write_cmos_sensor_twobyte(0X0384, 0X0001);
write_cmos_sensor_twobyte(0X0386, 0X0005);
write_cmos_sensor_twobyte(0X0400, 0X0000);
write_cmos_sensor_twobyte(0X0404, 0X0010);
write_cmos_sensor_twobyte(0X3060, 0X0103);
write_cmos_sensor_twobyte(0X0114, 0X0300);
write_cmos_sensor_twobyte(0X0110, 0X1002);
write_cmos_sensor_twobyte(0X0136, 0X1800);
write_cmos_sensor_twobyte(0X0304, 0X0006);
write_cmos_sensor_twobyte(0X0306, 0X01E0);
write_cmos_sensor_twobyte(0X0302, 0X0001);
write_cmos_sensor_twobyte(0X0300, 0X0004);
write_cmos_sensor_twobyte(0X030C, 0X0001);
write_cmos_sensor_twobyte(0X030E, 0X0004);
write_cmos_sensor_twobyte(0X0310, 0X0153);
write_cmos_sensor_twobyte(0X0312, 0X0000);
write_cmos_sensor_twobyte(0X030A, 0X0001);
write_cmos_sensor_twobyte(0X0308, 0X0008);
write_cmos_sensor_twobyte(0X0342, 0X27B0);
write_cmos_sensor_twobyte(0X0340, 0X0419);
write_cmos_sensor_twobyte(0X021E, 0X0000);
write_cmos_sensor_twobyte(0X3098, 0X0400);
write_cmos_sensor_twobyte(0X309A, 0X0002);
write_cmos_sensor_twobyte(0X30BC, 0X0031);
write_cmos_sensor_twobyte(0XF41E, 0X2180);
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X0990);
write_cmos_sensor_twobyte(0X6F12, 0X0020);
write_cmos_sensor_twobyte(0X602A, 0X0B16);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X16B4);
write_cmos_sensor_twobyte(0X6F12, 0X54C2);
write_cmos_sensor_twobyte(0X6F12, 0X122F);
write_cmos_sensor_twobyte(0X6F12, 0X0328);
write_cmos_sensor_twobyte(0X602A, 0X1688);
write_cmos_sensor_twobyte(0X6F12, 0X00A0);
write_cmos_sensor_twobyte(0X602A, 0X168C);
write_cmos_sensor_twobyte(0X6F12, 0X0028);
write_cmos_sensor_twobyte(0X6F12, 0X0034);
write_cmos_sensor_twobyte(0X6F12, 0X0C16);
write_cmos_sensor_twobyte(0X6F12, 0X0C16);
write_cmos_sensor_twobyte(0X6F12, 0X0C1C);
write_cmos_sensor_twobyte(0X6F12, 0X0C1C);
write_cmos_sensor_twobyte(0X6F12, 0X0C22);
write_cmos_sensor_twobyte(0X6F12, 0X0C22);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X602A, 0X0A94);
write_cmos_sensor_twobyte(0X6F12, 0X0800);
write_cmos_sensor_twobyte(0X602A, 0X5840);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X16BC);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X06C0);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X602A, 0X27A8);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0B62);
write_cmos_sensor_twobyte(0X6F12, 0X0120);
write_cmos_sensor_twobyte(0X602A, 0X0B4A);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X602A, 0X0AEC);
write_cmos_sensor_twobyte(0X6F12, 0X0207);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X29D8);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X29E2);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X2958);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X2998);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X2962);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X29A2);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X79F0);
write_cmos_sensor_twobyte(0X6218, 0X79F0);

// Stream On
write_cmos_sensor(0x0100,0x01);


mDELAY(10);

}


static void slim_video_setting(void)
{

LOG_INF("slim_video_setting");

// Stream Off
write_cmos_sensor(0x0100,0x00);
while(1)
{
    if( read_cmos_sensor(0x0005)==0xFF) break;

}


//$MIPI[Width:2688,Height:756,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:2034,useEmbData:0]
//$MV1[MCLK:24,Width:2688,Height:756,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:2034,pvi_pclk_inverse:0]
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X7970); // Clk on
write_cmos_sensor_twobyte(0X6218, 0X7150);
write_cmos_sensor_twobyte(0X3064, 0X0020);

//Mode
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X3064, 0X0020);
write_cmos_sensor_twobyte(0X0344, 0X0000);
write_cmos_sensor_twobyte(0X0346, 0X015C);
write_cmos_sensor_twobyte(0X0348, 0X1F7F);
write_cmos_sensor_twobyte(0X034A, 0X0A43);
write_cmos_sensor_twobyte(0X034C, 0X0A80);
write_cmos_sensor_twobyte(0X034E, 0X02F4);
write_cmos_sensor_twobyte(0X0408, 0X0000);
write_cmos_sensor_twobyte(0X040A, 0X0004);
write_cmos_sensor_twobyte(0X0900, 0X0113);
write_cmos_sensor_twobyte(0X0380, 0X0001);
write_cmos_sensor_twobyte(0X0382, 0X0001);
write_cmos_sensor_twobyte(0X0384, 0X0001);
write_cmos_sensor_twobyte(0X0386, 0X0005);
write_cmos_sensor_twobyte(0X0400, 0X0000);
write_cmos_sensor_twobyte(0X0404, 0X0010);
write_cmos_sensor_twobyte(0X3060, 0X0103);
write_cmos_sensor_twobyte(0X0114, 0X0300);
write_cmos_sensor_twobyte(0X0110, 0X1002);
write_cmos_sensor_twobyte(0X0136, 0X1800);
write_cmos_sensor_twobyte(0X0304, 0X0006);
write_cmos_sensor_twobyte(0X0306, 0X01E0);
write_cmos_sensor_twobyte(0X0302, 0X0001);
write_cmos_sensor_twobyte(0X0300, 0X0004);
write_cmos_sensor_twobyte(0X030C, 0X0001);
write_cmos_sensor_twobyte(0X030E, 0X0004);
write_cmos_sensor_twobyte(0X0310, 0X0153);
write_cmos_sensor_twobyte(0X0312, 0X0000);
write_cmos_sensor_twobyte(0X030A, 0X0001);
write_cmos_sensor_twobyte(0X0308, 0X0008);
write_cmos_sensor_twobyte(0X0342, 0X27B0);
write_cmos_sensor_twobyte(0X0340, 0X0C4D);
write_cmos_sensor_twobyte(0X021E, 0X0000);
write_cmos_sensor_twobyte(0X3098, 0X0400);
write_cmos_sensor_twobyte(0X309A, 0X0002);
write_cmos_sensor_twobyte(0X30BC, 0X0031);
write_cmos_sensor_twobyte(0XF41E, 0X2180);
write_cmos_sensor_twobyte(0X6028, 0X2000);
write_cmos_sensor_twobyte(0X602A, 0X0990);
write_cmos_sensor_twobyte(0X6F12, 0X0020);
write_cmos_sensor_twobyte(0X602A, 0X0B16);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X602A, 0X16B4);
write_cmos_sensor_twobyte(0X6F12, 0X54C2);
write_cmos_sensor_twobyte(0X6F12, 0X122F);
write_cmos_sensor_twobyte(0X6F12, 0X0328);
write_cmos_sensor_twobyte(0X602A, 0X1688);
write_cmos_sensor_twobyte(0X6F12, 0X00A0);
write_cmos_sensor_twobyte(0X602A, 0X168C);
write_cmos_sensor_twobyte(0X6F12, 0X0028);
write_cmos_sensor_twobyte(0X6F12, 0X0034);
write_cmos_sensor_twobyte(0X6F12, 0X0C16);
write_cmos_sensor_twobyte(0X6F12, 0X0C16);
write_cmos_sensor_twobyte(0X6F12, 0X0C1C);
write_cmos_sensor_twobyte(0X6F12, 0X0C1C);
write_cmos_sensor_twobyte(0X6F12, 0X0C22);
write_cmos_sensor_twobyte(0X6F12, 0X0C22);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X6F12, 0X0C28);
write_cmos_sensor_twobyte(0X602A, 0X0A94);
write_cmos_sensor_twobyte(0X6F12, 0X0800);
write_cmos_sensor_twobyte(0X602A, 0X5840);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X16BC);
write_cmos_sensor_twobyte(0X6F12, 0X0100);
write_cmos_sensor_twobyte(0X6F12, 0X06C0);
write_cmos_sensor_twobyte(0X6F12, 0X0101);
write_cmos_sensor_twobyte(0X602A, 0X27A8);
write_cmos_sensor_twobyte(0X6F12, 0X0000);
write_cmos_sensor_twobyte(0X602A, 0X0B62);
write_cmos_sensor_twobyte(0X6F12, 0X0120);
write_cmos_sensor_twobyte(0X602A, 0X0B4A);
write_cmos_sensor_twobyte(0X6F12, 0X0018);
write_cmos_sensor_twobyte(0X602A, 0X0AEC);
write_cmos_sensor_twobyte(0X6F12, 0X0207);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X29D8);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X29E2);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X0AE6);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X2958);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X2998);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X2962);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X602A, 0X29A2);
write_cmos_sensor_twobyte(0X6F12, 0X0BD0);
write_cmos_sensor_twobyte(0X6028, 0X4000);
write_cmos_sensor_twobyte(0X6214, 0X79F0);
write_cmos_sensor_twobyte(0X6218, 0X79F0);

// Stream On
write_cmos_sensor(0x0100,0x01);


mDELAY(10);

}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	/********************************************************

	*0x5040[7]: 1 enable,  0 disable
	*0x5040[3:2]; color bar style 00 standard color bar
	*0x5040[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
	********************************************************/


	if (enable)
	{
		write_cmos_sensor(0x0600, 0x000C); // Grayscale
	}
	else
	{
		write_cmos_sensor(0x0600, 0x0000); // Off
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	 get_imgsensor_id
*
* DESCRIPTION
*	 This function get the sensor ID
*
* PARAMETERS
*	 *sensorID : return the sensor ID
*
* RETURNS
*	 None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 5;

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {

			 *sensor_id = read_cmos_sensor_twobyte(0x0000);
			   if (*sensor_id == imgsensor_info.sensor_id || *sensor_id == 0x20C1) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);

				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id && *sensor_id != 0x20C1) {
		// if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	 open
*
* DESCRIPTION
*	 This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	 None
*
* RETURNS
*	 None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 5;
	kal_uint32 sensor_id = 0;
	//kal_uint32 chip_id = 0;
	LOG_1;
	//LOG_2;
	#if 1
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			write_cmos_sensor_twobyte(0x602C,0x4000);
			write_cmos_sensor_twobyte(0x602E,0x0000);
			sensor_id = read_cmos_sensor_twobyte(0x6F12);
			//LOG_INF("JEFF get_imgsensor_id-read sensor ID (0x%x)\n", sensor_id );

			write_cmos_sensor_twobyte(0x602C,0x4000);
			write_cmos_sensor_twobyte(0x602E,0x001A);
			chip_id = read_cmos_sensor_twobyte(0x6F12);
			//chip_id = read_cmos_sensor_twobyte(0x001A);
			//LOG_INF("get_imgsensor_id-read chip_id (0x%x)\n", chip_id );

			if (sensor_id == imgsensor_info.sensor_id  || sensor_id == 0x20C1) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x, chip_id (0x%x)\n", imgsensor.i2c_write_id,sensor_id, chip_id);
				break;
			}
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id || sensor_id == 0x20C1)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id && 0x20C1 != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;
#endif

					write_cmos_sensor_twobyte(0x602C,0x4000);
			write_cmos_sensor_twobyte(0x602E,0x001A);
			chip_id = read_cmos_sensor_twobyte(0x6F12);
	//chip_id = read_cmos_sensor_twobyte(0x001A);
	LOG_INF("JEFF get_imgsensor_id-read chip_id (0x%x)\n", chip_id );
	/* initail sequence write in  */
	//chip_id == 0x022C
	sensor_init_11_new();

#ifdef	USE_OIS
	//OIS_on(RUMBA_OIS_CAP_SETTING);//pangfei OIS
	LOG_INF("pangfei capture OIS setting\n");
	OIS_write_cmos_sensor(0x0002,0x05);
	OIS_write_cmos_sensor(0x0002,0x00);
	OIS_write_cmos_sensor(0x0000,0x01);
#endif

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.hdr_mode = KAL_FALSE;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}	 /*    open  */



/*************************************************************************
* FUNCTION
*	 close
*
* DESCRIPTION
*
*
* PARAMETERS
*	 None
*
* RETURNS
*	 None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/

	return ERROR_NONE;
}	 /*    close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*	 This function start the sensor preview.
*
* PARAMETERS
*	 *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	 None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
		imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
		imgsensor.pclk = imgsensor_info.pre.pclk;
		//imgsensor.video_mode = KAL_FALSE;
		imgsensor.line_length = imgsensor_info.pre.linelength;
		imgsensor.frame_length = imgsensor_info.pre.framelength;
		imgsensor.min_frame_length = imgsensor_info.pre.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	//if(chip_id == 0x022C)
	//{
		preview_setting_11_new();
	//}

	//set_mirror_flip(IMAGE_NORMAL);


#ifdef USE_OIS
	//OIS_on(RUMBA_OIS_PRE_SETTING);	//pangfei OIS
	LOG_INF("pangfei preview OIS setting\n");
	OIS_write_cmos_sensor(0x0002,0x05);
	OIS_write_cmos_sensor(0x0002,0x00);
	OIS_write_cmos_sensor(0x0000,0x01);
#endif
	return ERROR_NONE;
}	 /*    preview	 */

/*************************************************************************
* FUNCTION
*	 capture
*
* DESCRIPTION
*	 This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	 None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;

    /* Mark PIP case for dual pd */
	if (0) // (imgsensor.current_fps == imgsensor_info.cap1.max_framerate)
	{//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else
	{
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
		LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);


    /* Mark HDR setting mode for dual pd sensor */
#ifndef MARK_HDR
    if(imgsensor.hdr_mode == 9)
		capture_setting_WDR(imgsensor.current_fps);
	else
#endif
		capture_setting();


	//set_mirror_flip(IMAGE_NORMAL);
#ifdef	USE_OIS
	//OIS_on(RUMBA_OIS_CAP_SETTING);//pangfei OIS
	LOG_INF("pangfei capture OIS setting\n");
	OIS_write_cmos_sensor(0x0002,0x05);
	OIS_write_cmos_sensor(0x0002,0x00);
	OIS_write_cmos_sensor(0x0000,0x01);
#endif
	return ERROR_NONE;
	}	 /* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
		imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
		imgsensor.pclk = imgsensor_info.normal_video.pclk;
		imgsensor.line_length = imgsensor_info.normal_video.linelength;
		imgsensor.frame_length = imgsensor_info.normal_video.framelength;
		imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
		//imgsensor.current_fps = 300;
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	//if(chip_id == 0x022C)
	//{
		//normal_video_setting_11_new(imgsensor.current_fps);
		capture_setting();
	//}

	//set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}	 /*    normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
		imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
		imgsensor.pclk = imgsensor_info.hs_video.pclk;
		//imgsensor.video_mode = KAL_TRUE;
		imgsensor.line_length = imgsensor_info.hs_video.linelength;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength;
		imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
		imgsensor.dummy_line = 0;
		imgsensor.dummy_pixel = 0;
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	if(chip_id == 0x022C)
	{
		hs_video_setting_11();
	}
	else
	{
		hs_video_setting_11();
	}

	set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}	 /*    hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
		imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
		imgsensor.pclk = imgsensor_info.slim_video.pclk;
		imgsensor.line_length = imgsensor_info.slim_video.linelength;
		imgsensor.frame_length = imgsensor_info.slim_video.framelength;
		imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
		imgsensor.dummy_line = 0;
		imgsensor.dummy_pixel = 0;
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	set_mirror_flip(IMAGE_NORMAL);

	return ERROR_NONE;
}	 /*    slim_video	  */

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	  = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth 	= imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;

	return ERROR_NONE;
}	 /*    get_resolution	 */

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);


	//sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; /* not use */
	//sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; /* not use */
	//imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate; /* not use */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
#if defined(ENABLE_S5K2L7_PDAF_RAW)
		sensor_info->PDAF_Support = 4; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode(Full), 3:PDAF VC mode(Binning), 4: PDAF DualPD Raw Data mode, 5: PDAF DualPD VC mode*/
#else
		sensor_info->PDAF_Support = 0; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode(Full), 3:PDAF VC mode(Binning), 4: PDAF DualPD Raw Data mode, 5: PDAF DualPD VC mode*/
#endif
	sensor_info->HDR_Support = 0; /*0: NO HDR, 1: iHDR, 2:mvHDR, 3:zHDR*/

	/*0: no support, 1: G0,R0.B0, 2: G0,R0.B1, 3: G0,R1.B0, 4: G0,R1.B1*/
	/*					  5: G1,R0.B0, 6: G1,R0.B1, 7: G1,R1.B0, 8: G1,R1.B1*/
	sensor_info->ZHDR_Mode = 0;

	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;	 // 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

			sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

			break;
		default:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}	 /*    get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			capture(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			hs_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			slim_video(image_window, sensor_config_data);
			break;
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	 /* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{//This Function not used after ROME
	LOG_INF("framerate = %d\n ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps,1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) //enable auto flicker
		imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
				frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
				spin_lock(&imgsensor_drv_lock);
					imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
					imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
					imgsensor.min_frame_length = imgsensor.frame_length;
					spin_unlock(&imgsensor_drv_lock);
			} else {
					if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
					LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
				frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
				spin_lock(&imgsensor_drv_lock);
					imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
					imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
					imgsensor.min_frame_length = imgsensor.frame_length;
					spin_unlock(&imgsensor_drv_lock);
			}
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*framerate = imgsensor_info.pre.max_framerate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*framerate = imgsensor_info.normal_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*framerate = imgsensor_info.cap.max_framerate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*framerate = imgsensor_info.hs_video.max_framerate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*framerate = imgsensor_info.slim_video.max_framerate;
			break;
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
							 UINT8 *feature_para,UINT32 *feature_para_len)
{
    UINT32 sensor_id;
	UINT16 *feature_return_para_16=(UINT16 *) feature_para;
	UINT16 *feature_data_16=(UINT16 *) feature_para;
	UINT32 *feature_return_para_32=(UINT32 *) feature_para;
	UINT32 *feature_data_32=(UINT32 *) feature_para;
	unsigned long long *feature_data=(unsigned long long *) feature_para;
	//unsigned long long *feature_return_para=(unsigned long long *) feature_para;

	SET_PD_BLOCK_INFO_T *PDAFinfo;
	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/*feature_id = SENSOR_FEATURE_SET_ESHUTTER(0x3004)&SENSOR_FEATURE_SET_GAIN(0x3006)*/
	if((feature_id != 0x3004) || (feature_id != 0x3006))
		LOG_INF("feature_id = %d\n", feature_id);

	switch (feature_id) {
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk,imgsensor.current_fps);
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			set_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			night_mode((BOOL) *feature_data);
			break;
		case SENSOR_FEATURE_SET_GAIN:
			set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
			set_video_mode(*feature_data);
			break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			get_imgsensor_id(feature_return_para_32);
			break;
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_GET_PDAF_DATA:
            get_imgsensor_id(&sensor_id);
			//add for s5k2l7 pdaf
			LOG_INF("s5k2l7_read_otp_pdaf_data %x\n",sensor_id);
			s5k2l7_read_otp_pdaf_data((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)), sensor_id);
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_FRAMERATE:
			spin_lock(&imgsensor_drv_lock);
			imgsensor.current_fps = *feature_data_32;
			spin_unlock(&imgsensor_drv_lock);
			LOG_INF("current fps :%d\n", imgsensor.current_fps);
			break;
		case SENSOR_FEATURE_SET_HDR:
			LOG_INF("hdr mode :%d\n", (BOOL)*feature_data);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.hdr_mode = (BOOL)*feature_data;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:/*0x3080*/
			//LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%lld\n", *feature_data);

			wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
			}
			break;

		//add for s5k2l7 pdaf
		case SENSOR_FEATURE_GET_PDAF_INFO:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld\n", *feature_data);
			PDAFinfo= (SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(SET_PD_BLOCK_INFO_T));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					break;
			}
			break;

		case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:

			LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n", *feature_data);
			//PDAF capacity enable or not, s5k2l7 only full size support PDAF
			switch (*feature_data) {
#if defined(ENABLE_S5K2L7_PDAF_RAW)
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
#else
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
#endif
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
#if defined(ENABLE_S5K2L7_PDAF_RAW)
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1; ///preview support
#else
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
#endif
					break;
				default:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
			}
			break;
		case SENSOR_FEATURE_SET_HDR_SHUTTER:
			LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1));
			hdr_write_shutter((UINT16)*feature_data,(UINT16)*(feature_data+1));
			break;
		default:
			break;
	}

	return ERROR_NONE;
}	/*	feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K2L7_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	 /*    s5k2l7_MIPI_RAW_SensorInit	 */
