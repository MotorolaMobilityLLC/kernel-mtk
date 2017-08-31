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
#define NEW_2L7_SETTING // open this define for 2l7 setting, close this define for 2L1 setting & M2 setting
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
		/*Mipi datarate 1.99 Gbps*/
#ifdef NEW_2L7_SETTING
		.pclk = 799856640,				//record different mode's pclk
		.linelength = 8448,				//record different mode's linelength
		.framelength =3156, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2008,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 1504,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 300
#else
		.pclk = 799983360,				//record different mode's pclk
		.linelength = 20768,				//record different mode's linelength
		.framelength =3210, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 4016,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 3008,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 120,
#endif
	},
	.cap = {
#ifdef NEW_2L7_SETTING
		.pclk = 799856640,				//record different mode's pclk
		.linelength = 8448, 			//record different mode's linelength
		.framelength =3156, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 4016,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 3008,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 300
#else
		.pclk = 799983360,				//record different mode's pclk
		.linelength = 20768,				//record different mode's linelength
		.framelength =3210, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 4016,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 3008,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 120,
#endif
	},
	.cap1 = {
		/*Mipi datarate 1.99 Gbps*/
		.pclk = 799856640,				//record different mode's pclk
		.linelength = 8448, 			//record different mode's linelength
		.framelength =3156, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 5632,		//record different mode's width of grabwindow
		.grabwindow_height = 4224,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 240,
	},

	.normal_video = {
#ifdef NEW_2L7_SETTING
		.pclk = 799856640,				//record different mode's pclk
		.linelength = 8448,				//record different mode's linelength
		.framelength =3156, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2008,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 1504,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 300
#else
		.pclk = 799983360,				//record different mode's pclk
		.linelength = 20768,				//record different mode's linelength
		.framelength =3210, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 4016,		//Dual PD: need to tg grab width / 2, p1 drv will * 2 itself
		.grabwindow_height = 3008,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 120,
#endif
	},

	.hs_video = {
		.pclk = 720000000,
		.linelength = 6864,
		.framelength = 1716,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,		//record different mode's width of grabwindow
		.grabwindow_height = 1584,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 600,
	},

	.slim_video = {
		.pclk = 720000000,
		.linelength = 9200,
		.framelength = 872,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1408,		//record different mode's width of grabwindow
		.grabwindow_height = 792,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 890,
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
	.i2c_addr_table = { 0x20, 0xff},
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
	.i2c_write_id = 0x20,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
/* full_w; full_h; x0_offset; y0_offset; w0_size; h0_size; scale_w; scale_h; x1_offset;  y1_offset;  w1_size;  h1_size;
	 x2_tg_offset;	 y2_tg_offset;	w2_tg_size;  h2_tg_size;*/
{
#ifdef NEW_2L7_SETTING
{ 2008, 1504, 16, 16,  2008, 1504, 2008, 1504, 0, 0, 2008, 1504, 0, 0, 2008, 1504}, // Preview
{ 4016, 3008, 16, 16,  4016, 3008, 4016, 3008, 0, 0, 4016, 3008, 0, 0, 4016, 3008}, // capture
{ 2008, 1504, 16, 16,  2008, 1504, 2008, 1504, 0, 0, 2008, 1504, 0, 0, 2008, 1504}, // normal_video
#else
 { 4016, 3008, 16, 16,	4016, 3008, 4016, 3008, 0, 0, 4016, 3008, 0, 0, 4016, 3008}, // Preview
 { 4016, 3008, 16, 16,	4016, 3008, 4016, 3008, 0, 0, 4016, 3008, 0, 0, 4016, 3008}, // capture
 { 4016, 3008, 16, 16,	4016, 3008, 4016, 3008, 0, 0, 4016, 3008, 0, 0, 4016, 3008}, // normal_video
#endif
 { 4032, 3024, 16, 544, 4032, 3024, 2816, 1584, 0, 0, 2816, 1584, 0, 0, 2816, 1584}, // hs_video
 { 4032, 3024, 16, 544, 4032, 3024, 1408,  792, 0, 0, 1408,  792, 0, 0, 1408,  792}, // slim_video
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



#ifdef S5K2L7FW
extern int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId);
#define S5K2L7XXMIPI_WRITE_ID 0x5A
#define I2C_BUFFER_LEN	254
static kal_uint16 S5K2L7S_burst_download_cmos_sensor(kal_uint16* para,	kal_uint32 len)
{
		 unsigned char puSendCmd[I2C_BUFFER_LEN];
		 kal_uint32 tosend = 0;
		 kal_uint32 IDX = 0;
		 kal_uint32 totalCount = 0;
		 kal_uint32 addr, data;
		 kal_uint32 old_page=0;
		 kal_uint32 burst_count = 0;
		 kal_uint32 page_count = 0;

		 /**set page 0x0000*/
		 write_cmos_sensor_twobyte(0x6028,0x0000);
		 LOG_INF("JEFF:burst  file para[0]	(0x%x)\n", para[0]);

		 while(IDX*2 < len)
		 {
				   if(tosend == 0)
				   {
							addr= IDX*2;
							write_cmos_sensor_twobyte(0x602A,addr&0xFFFF);

							puSendCmd[tosend++] = 0x6F;
							//LOG_INF("JEFF:burst  file , puSendCmd(0x%x), [tosend ](%d)\n",puSendCmd[tosend - 1], tosend - 1);

							puSendCmd[tosend++] = 0x12;
							//LOG_INF("JEFF:burst  file , puSendCmd(0x%x), [tosend ](%d)\n",puSendCmd[tosend - 1], tosend - 1);
				   }
				   data = para[IDX++];

				   //puSendCmd[tosend++] = (u8)(data >> 8);//big endian
				   puSendCmd[tosend++] = (u8)(data & 0xFF);
				  //LOG_INF("JEFF:burst  file , puSendCmd(0x%x),[tosend ](%d)\n",puSendCmd[tosend - 1], tosend - 1);

				   //puSendCmd[tosend++] = (u8)(data & 0xFF);//big endian
				   puSendCmd[tosend++] = (u8)(data >> 8);
				   //LOG_INF("JEFF:burst  file , puSendCmd(0x%x),[tosend ](%d)\n",puSendCmd[tosend - 1], tosend - 1);

				   if ((tosend	>= I2C_BUFFER_LEN) ||( IDX*2 >= len) || (old_page!=((IDX)>>15)))
				   {
							LOG_INF("JEFF:burst  file start, tosend(%d), IDX(%d), old_page(%d)\n",tosend,IDX,old_page);

							iBurstWriteReg(puSendCmd , tosend, S5K2L7XXMIPI_WRITE_ID);
							totalCount += tosend;
							tosend = 0;

							if(old_page!=((IDX)>>15))
							{
									 write_cmos_sensor_twobyte(0x6028,((IDX)>>15));  //15 pointer offset
									 old_page=((IDX)>>15);
									 ++page_count;
							}
							++burst_count;
				   }
		 }
		 LOG_INF("JEFF:burst  file to sensor totalCount(%d), IDX(%d), old_page(%d)\n", totalCount, IDX, old_page);
		 LOG_INF("JEFF:burst  file to sensor page_count(%d), burst_count(%d), old_page(%d)\n", page_count, burst_count);


		 return totalCount;
}

static void Sensor_FW_read_downlaod(char*  filename)
{
		 /**getting binary size for buffer allocation **/
		 static kal_uint8* s5k2l7_SensorFirmware;
		 loff_t lof = 0;
		 kal_uint32  file_size = 0;
		 kal_uint32 ret;
		 struct file *filp;
		 mm_segment_t oldfs;
		 oldfs = get_fs();
		 set_fs(KERNEL_DS);

		 LOG_INF("Sensor_FW_read_downlaod.\n");
		// filp = filp_open(filename, O_RDWR, S_IRUSR | S_IWUSR);
		 filp = filp_open(filename, O_RDONLY, 0);
		 if (IS_ERR(filp))
		 {
			 LOG_INF(KERN_INFO "Unable to load '%s'.\n", filename);
			 return;
		 }
		 else
		 {
			 LOG_INF("JEFF:open s5k2l7 bin file sucess \n");
		 }

		 lof = vfs_llseek(filp, lof , SEEK_END);
		 file_size = lof;
		 LOG_INF("JEFF:The bin file length (%d)\n", file_size);
		 if (file_size <= 0 )
		 {
				   LOG_INF(KERN_INFO "Invalid firmware '%s'\n",filename);
				   filp_close(filp,0);
				   return ;
		 }

		 s5k2l7_SensorFirmware = kmalloc(file_size, GFP_KERNEL);
		 if(s5k2l7_SensorFirmware == 0)
		  {
				LOG_INF("JEFF:kmalloc failed \n");
				   filp_close(filp,0);
				 return ;
		   }
		  memset(s5k2l7_SensorFirmware, 0, file_size);

		 filp->f_pos = 0;
		 ret = vfs_read(filp, s5k2l7_SensorFirmware, file_size , &(filp->f_pos));
		 if (file_size!= ret)
		 {
				   LOG_INF(KERN_INFO "Failed to read '%s'.\n", filename);
				 kfree(s5k2l7_SensorFirmware);
				   filp_close(filp, NULL);
				   return ;
		 }

/*******************************************************************************/
		 /**FW_download*/
		 write_cmos_sensor_twobyte(0x6028, 0x4000);

		 //Reset & Remap
		 write_cmos_sensor_twobyte(0x6042, 0x0001);

		 //Auto increment enable
		 write_cmos_sensor_twobyte(0x6004, 0x0001);

		 // Download Sensor FW
		 write_cmos_sensor_twobyte(0x6028,0x0000);
		 S5K2l7S_burst_download_cmos_sensor ((kal_uint16*)s5k2l7_SensorFirmware, file_size);

		 write_cmos_sensor_twobyte(0x6028,0x4000);
		 //Reset & Start FW
		 write_cmos_sensor_twobyte(0x6040, 0x0001);

		 // FW Init time
		 mdelay(6);
		 set_fs(oldfs);
		 kfree(s5k2l7_SensorFirmware);
		 filp_close(filp, NULL);
}


#endif

#ifndef MARK_HDR
static void sensor_WDR_zhdr(void)
{
	if(imgsensor.hdr_mode == 9)
	{
		LOG_INF("sensor_WDR_zhdr\n");
		/*it would write 0x216 = 0x1, 0x217=0x00*/
		/*0x216=1 , Enable WDR*/
		/*0x217=0x00, Use Manual mode to set short /long exp */
#if defined(ENABLE_WDR_AUTO_RATION)
		write_cmos_sensor_twobyte(0x0216, 0x0101); /*For WDR auot ration*/
#else
		write_cmos_sensor_twobyte(0x0216, 0x0100); /*For WDR manual ration*/
#endif
		write_cmos_sensor_twobyte(0x0218, 0x0801);
		write_cmos_sensor_twobyte(0x021A, 0x0100);

		write_cmos_sensor_twobyte(0x6028, 0x2000);
		write_cmos_sensor_twobyte(0x602A, 0x6944);
		write_cmos_sensor_twobyte(0x6F12, 0x0000);
	}
	else
	{
		write_cmos_sensor_twobyte(0x0216, 0x0000);
		write_cmos_sensor_twobyte(0x0218, 0x0801);

		write_cmos_sensor_twobyte(0x6028, 0x2000);
		write_cmos_sensor_twobyte(0x602A, 0x6944);
		write_cmos_sensor_twobyte(0x6F12, 0x0000);// Normal case also should turn off the Recon Block.
	}
	/*for LE/SE Test*/
	//hdr_write_shutter(3460,800);

}
#endif 

static void sensor_init_11_new(void)
{
	/*2l7_global(HQ)_1014.sset*/
	LOG_INF("Enter s5k2l7 sensor_init.\n");
#if 0
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0x6214, 0xFFFF);
	write_cmos_sensor_twobyte(0x6216, 0xFFFF);
	write_cmos_sensor_twobyte(0x6218, 0x0000);
	write_cmos_sensor_twobyte(0x621A, 0x0000);
	write_cmos_sensor_twobyte(0x6028, 0x2001);
	write_cmos_sensor_twobyte(0x602A, 0x4DC0);
	write_cmos_sensor_twobyte(0x6F12, 0x0449);
	write_cmos_sensor_twobyte(0x6F12, 0x0348);
	write_cmos_sensor_twobyte(0x6F12, 0x044A);
	write_cmos_sensor_twobyte(0x6F12, 0x0860);
	write_cmos_sensor_twobyte(0x6F12, 0x101A);
	write_cmos_sensor_twobyte(0x6F12, 0x4860);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xFFBA);
	write_cmos_sensor_twobyte(0x6F12, 0x2001);
	write_cmos_sensor_twobyte(0x6F12, 0x54E8);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0xBE60);
	write_cmos_sensor_twobyte(0x6F12, 0x2001);
	write_cmos_sensor_twobyte(0x6F12, 0xAE00);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x10B5);
	write_cmos_sensor_twobyte(0x6F12, 0x0228);
	write_cmos_sensor_twobyte(0x6F12, 0x03D8);
	write_cmos_sensor_twobyte(0x6F12, 0x0229);
	write_cmos_sensor_twobyte(0x6F12, 0x01D8);
	write_cmos_sensor_twobyte(0x6F12, 0x0124);
	write_cmos_sensor_twobyte(0x6F12, 0x00E0);
	write_cmos_sensor_twobyte(0x6F12, 0x0024);
	write_cmos_sensor_twobyte(0x6F12, 0xFB48);
	write_cmos_sensor_twobyte(0x6F12, 0x0078);
	write_cmos_sensor_twobyte(0x6F12, 0x08B1);
	write_cmos_sensor_twobyte(0x6F12, 0x0124);
	write_cmos_sensor_twobyte(0x6F12, 0x04E0);
	write_cmos_sensor_twobyte(0x6F12, 0x1CB9);
	write_cmos_sensor_twobyte(0x6F12, 0x0021);
	write_cmos_sensor_twobyte(0x6F12, 0x5020);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x2AFB);
	write_cmos_sensor_twobyte(0x6F12, 0x2046);
	write_cmos_sensor_twobyte(0x6F12, 0x10BD);
	write_cmos_sensor_twobyte(0x6F12, 0x2DE9);
	write_cmos_sensor_twobyte(0x6F12, 0xF047);
	write_cmos_sensor_twobyte(0x6F12, 0x0546);
	write_cmos_sensor_twobyte(0x6F12, 0xDDE9);
	write_cmos_sensor_twobyte(0x6F12, 0x0897);
	write_cmos_sensor_twobyte(0x6F12, 0x8846);
	write_cmos_sensor_twobyte(0x6F12, 0x1646);
	write_cmos_sensor_twobyte(0x6F12, 0x1C46);
	write_cmos_sensor_twobyte(0x6F12, 0x022B);
	write_cmos_sensor_twobyte(0x6F12, 0x15D3);
	write_cmos_sensor_twobyte(0x6F12, 0xB542);
	write_cmos_sensor_twobyte(0x6F12, 0x03D2);
	write_cmos_sensor_twobyte(0x6F12, 0x0146);
	write_cmos_sensor_twobyte(0x6F12, 0x5120);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x18FB);
	write_cmos_sensor_twobyte(0x6F12, 0xA81B);
	write_cmos_sensor_twobyte(0x6F12, 0xA8EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0601);
	write_cmos_sensor_twobyte(0x6F12, 0x022C);
	write_cmos_sensor_twobyte(0x6F12, 0x12D0);
	write_cmos_sensor_twobyte(0x6F12, 0x80EA);
	write_cmos_sensor_twobyte(0x6F12, 0x0402);
	write_cmos_sensor_twobyte(0x6F12, 0x0244);
	write_cmos_sensor_twobyte(0x6F12, 0x6000);
	write_cmos_sensor_twobyte(0x6F12, 0xB2FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF0F5);
	write_cmos_sensor_twobyte(0x6F12, 0x81EA);
	write_cmos_sensor_twobyte(0x6F12, 0x0402);
	write_cmos_sensor_twobyte(0x6F12, 0x1144);
	write_cmos_sensor_twobyte(0x6F12, 0xB1FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF0F8);
	write_cmos_sensor_twobyte(0x6F12, 0x0020);
	write_cmos_sensor_twobyte(0x6F12, 0x4545);
	write_cmos_sensor_twobyte(0x6F12, 0x0ED2);
	write_cmos_sensor_twobyte(0x6F12, 0xA8EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0501);
	write_cmos_sensor_twobyte(0x6F12, 0xC9F8);
	write_cmos_sensor_twobyte(0x6F12, 0x0010);
	write_cmos_sensor_twobyte(0x6F12, 0x10E0);
	write_cmos_sensor_twobyte(0x6F12, 0x80F0);
	write_cmos_sensor_twobyte(0x6F12, 0x0202);
	write_cmos_sensor_twobyte(0x6F12, 0x1044);
	write_cmos_sensor_twobyte(0x6F12, 0x8508);
	write_cmos_sensor_twobyte(0x6F12, 0x81F0);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x0844);
	write_cmos_sensor_twobyte(0x6F12, 0x4FEA);
	write_cmos_sensor_twobyte(0x6F12, 0x9008);
	write_cmos_sensor_twobyte(0x6F12, 0xEDE7);
	write_cmos_sensor_twobyte(0x6F12, 0xC9F8);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x022C);
	write_cmos_sensor_twobyte(0x6F12, 0xA5EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x00D1);
	write_cmos_sensor_twobyte(0x6F12, 0x4000);
	write_cmos_sensor_twobyte(0x6F12, 0x3860);
	write_cmos_sensor_twobyte(0x6F12, 0xBDE8);
	write_cmos_sensor_twobyte(0x6F12, 0xF087);
	write_cmos_sensor_twobyte(0x6F12, 0x2DE9);
	write_cmos_sensor_twobyte(0x6F12, 0xF041);
	write_cmos_sensor_twobyte(0x6F12, 0xD64A);
	write_cmos_sensor_twobyte(0x6F12, 0xDFF8);
	write_cmos_sensor_twobyte(0x6F12, 0x5C83);
	write_cmos_sensor_twobyte(0x6F12, 0x0446);
	write_cmos_sensor_twobyte(0x6F12, 0x0D46);
	write_cmos_sensor_twobyte(0x6F12, 0x1178);
	write_cmos_sensor_twobyte(0x6F12, 0x98F8);
	write_cmos_sensor_twobyte(0x6F12, 0x1800);
	write_cmos_sensor_twobyte(0x6F12, 0x8AB0);
	write_cmos_sensor_twobyte(0x6F12, 0x21B9);
	write_cmos_sensor_twobyte(0x6F12, 0x94F8);
	write_cmos_sensor_twobyte(0x6F12, 0x2510);
	write_cmos_sensor_twobyte(0x6F12, 0x01B1);
	write_cmos_sensor_twobyte(0x6F12, 0x00B1);
	write_cmos_sensor_twobyte(0x6F12, 0x0120);
	write_cmos_sensor_twobyte(0x6F12, 0x2870);
	write_cmos_sensor_twobyte(0x6F12, 0x94F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7400);
	write_cmos_sensor_twobyte(0x6F12, 0x0228);
	write_cmos_sensor_twobyte(0x6F12, 0x4BD0);
	write_cmos_sensor_twobyte(0x6F12, 0x0020);
	write_cmos_sensor_twobyte(0x6F12, 0xA874);
	write_cmos_sensor_twobyte(0x6F12, 0x94F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7610);
	write_cmos_sensor_twobyte(0x6F12, 0x0229);
	write_cmos_sensor_twobyte(0x6F12, 0x47D0);
	write_cmos_sensor_twobyte(0x6F12, 0x0021);
	write_cmos_sensor_twobyte(0x6F12, 0xE974);
	write_cmos_sensor_twobyte(0x6F12, 0x00B1);
	write_cmos_sensor_twobyte(0x6F12, 0x0220);
	write_cmos_sensor_twobyte(0x6F12, 0x1178);
	write_cmos_sensor_twobyte(0x6F12, 0x0026);
	write_cmos_sensor_twobyte(0x6F12, 0x01B1);
	write_cmos_sensor_twobyte(0x6F12, 0x361F);
	write_cmos_sensor_twobyte(0x6F12, 0x09AA);
	write_cmos_sensor_twobyte(0x6F12, 0x05A9);
	write_cmos_sensor_twobyte(0x6F12, 0xCDE9);
	write_cmos_sensor_twobyte(0x6F12, 0x0012);
	write_cmos_sensor_twobyte(0x6F12, 0xC54F);
	write_cmos_sensor_twobyte(0x6F12, 0x04F1);
	write_cmos_sensor_twobyte(0x6F12, 0x5A04);
	write_cmos_sensor_twobyte(0x6F12, 0xF988);
	write_cmos_sensor_twobyte(0x6F12, 0xA37E);
	write_cmos_sensor_twobyte(0x6F12, 0x01EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0002);
	write_cmos_sensor_twobyte(0x6F12, 0x3889);
	write_cmos_sensor_twobyte(0x6F12, 0x00EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0601);
	write_cmos_sensor_twobyte(0x6F12, 0xE088);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xBCFA);
	write_cmos_sensor_twobyte(0x6F12, 0x08A9);
	write_cmos_sensor_twobyte(0x6F12, 0x04A8);
	write_cmos_sensor_twobyte(0x6F12, 0xCDE9);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x6F12, 0x6089);
	write_cmos_sensor_twobyte(0x6F12, 0x14F8);
	write_cmos_sensor_twobyte(0x6F12, 0x3E1C);
	write_cmos_sensor_twobyte(0x6F12, 0xA37E);
	write_cmos_sensor_twobyte(0x6F12, 0x0144);
	write_cmos_sensor_twobyte(0x6F12, 0xB889);
	write_cmos_sensor_twobyte(0x6F12, 0x491E);
	write_cmos_sensor_twobyte(0x6F12, 0xFA88);
	write_cmos_sensor_twobyte(0x6F12, 0x3044);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xADFA);
	write_cmos_sensor_twobyte(0x6F12, 0x07A9);
	write_cmos_sensor_twobyte(0x6F12, 0x03A8);
	write_cmos_sensor_twobyte(0x6F12, 0xCDE9);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x6F12, 0x237F);
	write_cmos_sensor_twobyte(0x6F12, 0x3A88);
	write_cmos_sensor_twobyte(0x6F12, 0x7989);
	write_cmos_sensor_twobyte(0x6F12, 0x2089);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xA3FA);
	write_cmos_sensor_twobyte(0x6F12, 0x06A9);
	write_cmos_sensor_twobyte(0x6F12, 0x02A8);
	write_cmos_sensor_twobyte(0x6F12, 0xCDE9);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x6F12, 0xA089);
	write_cmos_sensor_twobyte(0x6F12, 0x14F8);
	write_cmos_sensor_twobyte(0x6F12, 0x3C1C);
	write_cmos_sensor_twobyte(0x6F12, 0x237F);
	write_cmos_sensor_twobyte(0x6F12, 0x0144);
	write_cmos_sensor_twobyte(0x6F12, 0x491E);
	write_cmos_sensor_twobyte(0x6F12, 0x3A88);
	write_cmos_sensor_twobyte(0x6F12, 0xF889);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x95FA);
	write_cmos_sensor_twobyte(0x6F12, 0xA07F);
	write_cmos_sensor_twobyte(0x6F12, 0xA4F1);
	write_cmos_sensor_twobyte(0x6F12, 0x5A04);
	write_cmos_sensor_twobyte(0x6F12, 0x28B1);
	write_cmos_sensor_twobyte(0x6F12, 0x0498);
	write_cmos_sensor_twobyte(0x6F12, 0x04E0);
	write_cmos_sensor_twobyte(0x6F12, 0x0120);
	write_cmos_sensor_twobyte(0x6F12, 0xB2E7);
	write_cmos_sensor_twobyte(0x6F12, 0x0121);
	write_cmos_sensor_twobyte(0x6F12, 0xB6E7);
	write_cmos_sensor_twobyte(0x6F12, 0x0598);
	write_cmos_sensor_twobyte(0x6F12, 0x6880);
	write_cmos_sensor_twobyte(0x6F12, 0x94F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7900);
	write_cmos_sensor_twobyte(0x6F12, 0x08B1);
	write_cmos_sensor_twobyte(0x6F12, 0x0298);
	write_cmos_sensor_twobyte(0x6F12, 0x00E0);
	write_cmos_sensor_twobyte(0x6F12, 0x0398);
	write_cmos_sensor_twobyte(0x6F12, 0xA880);
	write_cmos_sensor_twobyte(0x6F12, 0xB4F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6C00);
	write_cmos_sensor_twobyte(0x6F12, 0x0599);
	write_cmos_sensor_twobyte(0x6F12, 0x401A);
	write_cmos_sensor_twobyte(0x6F12, 0x0499);
	write_cmos_sensor_twobyte(0x6F12, 0x401A);
	write_cmos_sensor_twobyte(0x6F12, 0x0028);
	write_cmos_sensor_twobyte(0x6F12, 0x00DC);
	write_cmos_sensor_twobyte(0x6F12, 0x0020);
	write_cmos_sensor_twobyte(0x6F12, 0xE881);
	write_cmos_sensor_twobyte(0x6F12, 0xB4F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6E00);
	write_cmos_sensor_twobyte(0x6F12, 0x0399);
	write_cmos_sensor_twobyte(0x6F12, 0x401A);
	write_cmos_sensor_twobyte(0x6F12, 0x0299);
	write_cmos_sensor_twobyte(0x6F12, 0x401A);
	write_cmos_sensor_twobyte(0x6F12, 0x0028);
	write_cmos_sensor_twobyte(0x6F12, 0x00DC);
	write_cmos_sensor_twobyte(0x6F12, 0x0020);
	write_cmos_sensor_twobyte(0x6F12, 0x2882);
	write_cmos_sensor_twobyte(0x6F12, 0x94F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7800);
	write_cmos_sensor_twobyte(0x6F12, 0x08B1);
	write_cmos_sensor_twobyte(0x6F12, 0x0899);
	write_cmos_sensor_twobyte(0x6F12, 0x00E0);
	write_cmos_sensor_twobyte(0x6F12, 0x0999);
	write_cmos_sensor_twobyte(0x6F12, 0x98F8);
	write_cmos_sensor_twobyte(0x6F12, 0x1B20);
	write_cmos_sensor_twobyte(0x6F12, 0x98F8);
	write_cmos_sensor_twobyte(0x6F12, 0x1930);
	write_cmos_sensor_twobyte(0x6F12, 0x4046);
	write_cmos_sensor_twobyte(0x6F12, 0x12FB);
	write_cmos_sensor_twobyte(0x6F12, 0x03F2);
	write_cmos_sensor_twobyte(0x6F12, 0x91FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF2F6);
	write_cmos_sensor_twobyte(0x6F12, 0x02FB);
	write_cmos_sensor_twobyte(0x6F12, 0x1611);
	write_cmos_sensor_twobyte(0x6F12, 0x94F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7920);
	write_cmos_sensor_twobyte(0x6F12, 0x0AB1);
	write_cmos_sensor_twobyte(0x6F12, 0x069A);
	write_cmos_sensor_twobyte(0x6F12, 0x00E0);
	write_cmos_sensor_twobyte(0x6F12, 0x079A);
	write_cmos_sensor_twobyte(0x6F12, 0x047F);
	write_cmos_sensor_twobyte(0x6F12, 0x867E);
	write_cmos_sensor_twobyte(0x6F12, 0x14FB);
	write_cmos_sensor_twobyte(0x6F12, 0x06F4);
	write_cmos_sensor_twobyte(0x6F12, 0x91FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF3F3);
	write_cmos_sensor_twobyte(0x6F12, 0x92FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF4F6);
	write_cmos_sensor_twobyte(0x6F12, 0xEB80);
	write_cmos_sensor_twobyte(0x6F12, 0x04FB);
	write_cmos_sensor_twobyte(0x6F12, 0x1622);
	write_cmos_sensor_twobyte(0x6F12, 0x437E);
	write_cmos_sensor_twobyte(0x6F12, 0x91FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF3F4);
	write_cmos_sensor_twobyte(0x6F12, 0x03FB);
	write_cmos_sensor_twobyte(0x6F12, 0x1411);
	write_cmos_sensor_twobyte(0x6F12, 0x6981);
	write_cmos_sensor_twobyte(0x6F12, 0x817E);
	write_cmos_sensor_twobyte(0x6F12, 0x92FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF1F1);
	write_cmos_sensor_twobyte(0x6F12, 0x2981);
	write_cmos_sensor_twobyte(0x6F12, 0x98F8);
	write_cmos_sensor_twobyte(0x6F12, 0x1A00);
	write_cmos_sensor_twobyte(0x6F12, 0x92FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF0F1);
	write_cmos_sensor_twobyte(0x6F12, 0x00FB);
	write_cmos_sensor_twobyte(0x6F12, 0x1120);
	write_cmos_sensor_twobyte(0x6F12, 0xA881);
	write_cmos_sensor_twobyte(0x6F12, 0x0AB0);
	write_cmos_sensor_twobyte(0x6F12, 0xBDE8);
	write_cmos_sensor_twobyte(0x6F12, 0xF081);
	write_cmos_sensor_twobyte(0x6F12, 0x2DE9);
	write_cmos_sensor_twobyte(0x6F12, 0xF34F);
	write_cmos_sensor_twobyte(0x6F12, 0x7D4E);
	write_cmos_sensor_twobyte(0x6F12, 0x7D4B);
	write_cmos_sensor_twobyte(0x6F12, 0x81B0);
	write_cmos_sensor_twobyte(0x6F12, 0x0021);
	write_cmos_sensor_twobyte(0x6F12, 0x8A5B);
	write_cmos_sensor_twobyte(0x6F12, 0x03EB);
	write_cmos_sensor_twobyte(0x6F12, 0x4104);
	write_cmos_sensor_twobyte(0x6F12, 0xD5B2);
	write_cmos_sensor_twobyte(0x6F12, 0x120A);
	write_cmos_sensor_twobyte(0x6F12, 0xA4F8);
	write_cmos_sensor_twobyte(0x6F12, 0x8451);
	write_cmos_sensor_twobyte(0x6F12, 0x891C);
	write_cmos_sensor_twobyte(0x6F12, 0xA4F8);
	write_cmos_sensor_twobyte(0x6F12, 0x8621);
	write_cmos_sensor_twobyte(0x6F12, 0x0829);
	write_cmos_sensor_twobyte(0x6F12, 0xF3D3);
	write_cmos_sensor_twobyte(0x6F12, 0x4FF4);
	write_cmos_sensor_twobyte(0x6F12, 0x8011);
	write_cmos_sensor_twobyte(0x6F12, 0xB1FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF0FE);
	write_cmos_sensor_twobyte(0x6F12, 0x7149);
	write_cmos_sensor_twobyte(0x6F12, 0x0022);
	write_cmos_sensor_twobyte(0x6F12, 0xAEF5);
	write_cmos_sensor_twobyte(0x6F12, 0x807B);
	write_cmos_sensor_twobyte(0x6F12, 0xB1F8);
	write_cmos_sensor_twobyte(0x6F12, 0x8812);
	write_cmos_sensor_twobyte(0x6F12, 0xA0F5);
	write_cmos_sensor_twobyte(0x6F12, 0x8078);
	write_cmos_sensor_twobyte(0x6F12, 0x0091);
	write_cmos_sensor_twobyte(0x6F12, 0x6F4D);
	write_cmos_sensor_twobyte(0x6F12, 0x0024);
	write_cmos_sensor_twobyte(0x6F12, 0x05EB);
	write_cmos_sensor_twobyte(0x6F12, 0x8203);
	write_cmos_sensor_twobyte(0x6F12, 0x05EB);
	write_cmos_sensor_twobyte(0x6F12, 0x420A);
	write_cmos_sensor_twobyte(0x6F12, 0xC3F8);
	write_cmos_sensor_twobyte(0x6F12, 0xA441);
	write_cmos_sensor_twobyte(0x6F12, 0xAAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x9441);
	write_cmos_sensor_twobyte(0x6F12, 0x674C);
	write_cmos_sensor_twobyte(0x6F12, 0x4FF0);
	write_cmos_sensor_twobyte(0x6F12, 0x0F01);
	write_cmos_sensor_twobyte(0x6F12, 0x04EB);
	write_cmos_sensor_twobyte(0x6F12, 0x4205);
	write_cmos_sensor_twobyte(0x6F12, 0x05F5);
	write_cmos_sensor_twobyte(0x6F12, 0x2775);
	write_cmos_sensor_twobyte(0x6F12, 0x2F8C);
	write_cmos_sensor_twobyte(0x6F12, 0x2C88);
	write_cmos_sensor_twobyte(0x6F12, 0xA7EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0406);
	write_cmos_sensor_twobyte(0x6F12, 0x06FB);
	write_cmos_sensor_twobyte(0x6F12, 0x08F6);
	write_cmos_sensor_twobyte(0x6F12, 0x96FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF1F6);
	write_cmos_sensor_twobyte(0x6F12, 0x04EB);
	write_cmos_sensor_twobyte(0x6F12, 0x2624);
	write_cmos_sensor_twobyte(0x6F12, 0xC3F8);
	write_cmos_sensor_twobyte(0x6F12, 0x2441);
	write_cmos_sensor_twobyte(0x6F12, 0x2D8A);
	write_cmos_sensor_twobyte(0x6F12, 0xA7EB);
	write_cmos_sensor_twobyte(0x6F12, 0x050C);
	write_cmos_sensor_twobyte(0x6F12, 0x0BFB);
	write_cmos_sensor_twobyte(0x6F12, 0x0CF6);
	write_cmos_sensor_twobyte(0x6F12, 0x96FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF1F6);
	write_cmos_sensor_twobyte(0x6F12, 0xA7EB);
	write_cmos_sensor_twobyte(0x6F12, 0x2629);
	write_cmos_sensor_twobyte(0x6F12, 0x0CFB);
	write_cmos_sensor_twobyte(0x6F12, 0x08FC);
	write_cmos_sensor_twobyte(0x6F12, 0x9CFB);
	write_cmos_sensor_twobyte(0x6F12, 0xF1F6);
	write_cmos_sensor_twobyte(0x6F12, 0x05EB);
	write_cmos_sensor_twobyte(0x6F12, 0x2625);
	write_cmos_sensor_twobyte(0x6F12, 0x4FF0);
	write_cmos_sensor_twobyte(0x6F12, 0x1006);
	write_cmos_sensor_twobyte(0x6F12, 0xC3F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6491);
	write_cmos_sensor_twobyte(0x6F12, 0xC3F8);
	write_cmos_sensor_twobyte(0x6F12, 0x4451);
	write_cmos_sensor_twobyte(0x6F12, 0xB6EB);
	write_cmos_sensor_twobyte(0x6F12, 0x102F);
	write_cmos_sensor_twobyte(0x6F12, 0x03D8);
	write_cmos_sensor_twobyte(0x6F12, 0x3C46);
	write_cmos_sensor_twobyte(0x6F12, 0xC3F8);
	write_cmos_sensor_twobyte(0x6F12, 0xA471);
	write_cmos_sensor_twobyte(0x6F12, 0x19E0);
	write_cmos_sensor_twobyte(0x6F12, 0x042A);
	write_cmos_sensor_twobyte(0x6F12, 0x0BD2);
	write_cmos_sensor_twobyte(0x6F12, 0x0299);
	write_cmos_sensor_twobyte(0x6F12, 0x4FF4);
	write_cmos_sensor_twobyte(0x6F12, 0x8016);
	write_cmos_sensor_twobyte(0x6F12, 0xB6FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF1F6);
	write_cmos_sensor_twobyte(0x6F12, 0xA6EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0E06);
	write_cmos_sensor_twobyte(0x6F12, 0x651B);
	write_cmos_sensor_twobyte(0x6F12, 0x6E43);
	write_cmos_sensor_twobyte(0x6F12, 0x96FB);
	write_cmos_sensor_twobyte(0x6F12, 0xFBF5);
	write_cmos_sensor_twobyte(0x6F12, 0x08E0);
	write_cmos_sensor_twobyte(0x6F12, 0x0299);
	write_cmos_sensor_twobyte(0x6F12, 0xA9EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0405);
	write_cmos_sensor_twobyte(0x6F12, 0x0E1A);
	write_cmos_sensor_twobyte(0x6F12, 0x7543);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F5);
	write_cmos_sensor_twobyte(0x6F12, 0x8056);
	write_cmos_sensor_twobyte(0x6F12, 0x95FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF6F5);
	write_cmos_sensor_twobyte(0x6F12, 0x2C44);
	write_cmos_sensor_twobyte(0x6F12, 0xC3F8);
	write_cmos_sensor_twobyte(0x6F12, 0xA441);
	write_cmos_sensor_twobyte(0x6F12, 0x0099);
	write_cmos_sensor_twobyte(0x6F12, 0x521C);
	write_cmos_sensor_twobyte(0x6F12, 0xA4EB);
	write_cmos_sensor_twobyte(0x6F12, 0x0124);
	write_cmos_sensor_twobyte(0x6F12, 0xC3F8);
	write_cmos_sensor_twobyte(0x6F12, 0xA441);
	write_cmos_sensor_twobyte(0x6F12, 0xBAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x8431);
	write_cmos_sensor_twobyte(0x6F12, 0x6343);
	write_cmos_sensor_twobyte(0x6F12, 0xC4EB);
	write_cmos_sensor_twobyte(0x6F12, 0xE311);
	write_cmos_sensor_twobyte(0x6F12, 0xAAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x9411);
	write_cmos_sensor_twobyte(0x6F12, 0x082A);
	write_cmos_sensor_twobyte(0x6F12, 0x9DD3);
	write_cmos_sensor_twobyte(0x6F12, 0xBDE8);
	write_cmos_sensor_twobyte(0x6F12, 0xFE8F);
	write_cmos_sensor_twobyte(0x6F12, 0x0180);
	write_cmos_sensor_twobyte(0x6F12, 0x7047);
	write_cmos_sensor_twobyte(0x6F12, 0x0160);
	write_cmos_sensor_twobyte(0x6F12, 0x7047);
	write_cmos_sensor_twobyte(0x6F12, 0x2DE9);
	write_cmos_sensor_twobyte(0x6F12, 0xF05F);
	write_cmos_sensor_twobyte(0x6F12, 0xDFF8);
	write_cmos_sensor_twobyte(0x6F12, 0xEC90);
	write_cmos_sensor_twobyte(0x6F12, 0x0446);
	write_cmos_sensor_twobyte(0x6F12, 0x0E46);
	write_cmos_sensor_twobyte(0x6F12, 0x1546);
	write_cmos_sensor_twobyte(0x6F12, 0x4888);
	write_cmos_sensor_twobyte(0x6F12, 0x99F8);
	write_cmos_sensor_twobyte(0x6F12, 0x8B20);
	write_cmos_sensor_twobyte(0x6F12, 0xDFF8);
	write_cmos_sensor_twobyte(0x6F12, 0xE0A0);
	write_cmos_sensor_twobyte(0x6F12, 0xDFF8);
	write_cmos_sensor_twobyte(0x6F12, 0xC4B0);
	write_cmos_sensor_twobyte(0x6F12, 0x0988);
	write_cmos_sensor_twobyte(0x6F12, 0x1F46);
	write_cmos_sensor_twobyte(0x6F12, 0xF2B3);
	write_cmos_sensor_twobyte(0x6F12, 0x6143);
	write_cmos_sensor_twobyte(0x6F12, 0x01EB);
	write_cmos_sensor_twobyte(0x6F12, 0x9003);
	write_cmos_sensor_twobyte(0x6F12, 0xDAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x7400);
	write_cmos_sensor_twobyte(0x6F12, 0xD046);
	write_cmos_sensor_twobyte(0x6F12, 0x9842);
	write_cmos_sensor_twobyte(0x6F12, 0x0BD9);
	write_cmos_sensor_twobyte(0x6F12, 0xB0FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF4F0);
	write_cmos_sensor_twobyte(0x6F12, 0x80B2);
	write_cmos_sensor_twobyte(0x6F12, 0x3080);
	write_cmos_sensor_twobyte(0x6F12, 0xD8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7410);
	write_cmos_sensor_twobyte(0x6F12, 0x00FB);
	write_cmos_sensor_twobyte(0x6F12, 0x1410);
	write_cmos_sensor_twobyte(0x6F12, 0x8000);
	write_cmos_sensor_twobyte(0x6F12, 0x7080);
	write_cmos_sensor_twobyte(0x6F12, 0xD8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7430);
	write_cmos_sensor_twobyte(0x6F12, 0xB8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7800);
	write_cmos_sensor_twobyte(0x6F12, 0x7A88);
	write_cmos_sensor_twobyte(0x6F12, 0x181A);
	write_cmos_sensor_twobyte(0x6F12, 0xB0FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF4F1);
	write_cmos_sensor_twobyte(0x6F12, 0x01FB);
	write_cmos_sensor_twobyte(0x6F12, 0x1400);
	write_cmos_sensor_twobyte(0x6F12, 0x2AB3);
	write_cmos_sensor_twobyte(0x6F12, 0x8242);
	write_cmos_sensor_twobyte(0x6F12, 0x00D9);
	write_cmos_sensor_twobyte(0x6F12, 0x491E);
	write_cmos_sensor_twobyte(0x6F12, 0x1046);
	write_cmos_sensor_twobyte(0x6F12, 0x9BF8);
	write_cmos_sensor_twobyte(0x6F12, 0x0120);
	write_cmos_sensor_twobyte(0x6F12, 0x5E46);
	write_cmos_sensor_twobyte(0x6F12, 0x72B1);
	write_cmos_sensor_twobyte(0x6F12, 0xB6F8);
	write_cmos_sensor_twobyte(0x6F12, 0x04C0);
	write_cmos_sensor_twobyte(0x6F12, 0x8C45);
	write_cmos_sensor_twobyte(0x6F12, 0x0AD8);
	write_cmos_sensor_twobyte(0x6F12, 0xB6F8);
	write_cmos_sensor_twobyte(0x6F12, 0x06C0);
	write_cmos_sensor_twobyte(0x6F12, 0x8C45);
	write_cmos_sensor_twobyte(0x6F12, 0x06D3);
	write_cmos_sensor_twobyte(0x6F12, 0x9BF8);
	write_cmos_sensor_twobyte(0x6F12, 0x0260);
	write_cmos_sensor_twobyte(0x6F12, 0x891B);
	write_cmos_sensor_twobyte(0x6F12, 0xB1FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF2F1);
	write_cmos_sensor_twobyte(0x6F12, 0x01FB);
	write_cmos_sensor_twobyte(0x6F12, 0x0261);
	write_cmos_sensor_twobyte(0x6F12, 0x3980);
	write_cmos_sensor_twobyte(0x6F12, 0xBAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x7820);
	write_cmos_sensor_twobyte(0x6F12, 0x01FB);
	write_cmos_sensor_twobyte(0x6F12, 0x0421);
	write_cmos_sensor_twobyte(0x6F12, 0x0144);
	write_cmos_sensor_twobyte(0x6F12, 0x2960);
	write_cmos_sensor_twobyte(0x6F12, 0x99F8);
	write_cmos_sensor_twobyte(0x6F12, 0x2800);
	write_cmos_sensor_twobyte(0x6F12, 0xF8B1);
	write_cmos_sensor_twobyte(0x6F12, 0x00E0);
	write_cmos_sensor_twobyte(0x6F12, 0x34E0);
	write_cmos_sensor_twobyte(0x6F12, 0x0C22);
	write_cmos_sensor_twobyte(0x6F12, 0x1846);
	write_cmos_sensor_twobyte(0x6F12, 0x72E0);
	write_cmos_sensor_twobyte(0x6F12, 0xB8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x7020);
	write_cmos_sensor_twobyte(0x6F12, 0x8242);
	write_cmos_sensor_twobyte(0x6F12, 0x01D9);
	write_cmos_sensor_twobyte(0x6F12, 0x491E);
	write_cmos_sensor_twobyte(0x6F12, 0x601E);
	write_cmos_sensor_twobyte(0x6F12, 0x98F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6E20);
	write_cmos_sensor_twobyte(0x6F12, 0x02B3);
	write_cmos_sensor_twobyte(0x6F12, 0xB8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6820);
	write_cmos_sensor_twobyte(0x6F12, 0x8242);
	write_cmos_sensor_twobyte(0x6F12, 0x0BD2);
	write_cmos_sensor_twobyte(0x6F12, 0x98F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6460);
	write_cmos_sensor_twobyte(0x6F12, 0x3EB1);
	write_cmos_sensor_twobyte(0x6F12, 0xB8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6A60);
	write_cmos_sensor_twobyte(0x6F12, 0x8642);
	write_cmos_sensor_twobyte(0x6F12, 0x03D8);
	write_cmos_sensor_twobyte(0x6F12, 0xB8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6C20);
	write_cmos_sensor_twobyte(0x6F12, 0x8242);
	write_cmos_sensor_twobyte(0x6F12, 0x00D2);
	write_cmos_sensor_twobyte(0x6F12, 0x1046);
	write_cmos_sensor_twobyte(0x6F12, 0x7880);
	write_cmos_sensor_twobyte(0x6F12, 0xC2E7);
	write_cmos_sensor_twobyte(0x6F12, 0x10E0);
	write_cmos_sensor_twobyte(0x6F12, 0x2001);
	write_cmos_sensor_twobyte(0x6F12, 0xAB00);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0x14B0);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0xC1BE);
	write_cmos_sensor_twobyte(0x6F12, 0x4000);
	write_cmos_sensor_twobyte(0x6F12, 0x9526);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0xCC70);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0x0670);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0xC5C0);
	write_cmos_sensor_twobyte(0x6F12, 0xB8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6220);
	write_cmos_sensor_twobyte(0x6F12, 0xE8E7);
	write_cmos_sensor_twobyte(0x6F12, 0x4FF4);
	write_cmos_sensor_twobyte(0x6F12, 0x8050);
	write_cmos_sensor_twobyte(0x6F12, 0x6860);
	write_cmos_sensor_twobyte(0x6F12, 0xBDE8);
	write_cmos_sensor_twobyte(0x6F12, 0xF09F);
	write_cmos_sensor_twobyte(0x6F12, 0x99F8);
	write_cmos_sensor_twobyte(0x6F12, 0x8C20);
	write_cmos_sensor_twobyte(0x6F12, 0xB0FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF2F0);
	write_cmos_sensor_twobyte(0x6F12, 0x01FB);
	write_cmos_sensor_twobyte(0x6F12, 0x0408);
	write_cmos_sensor_twobyte(0x6F12, 0xB9F8);
	write_cmos_sensor_twobyte(0x6F12, 0x2400);
	write_cmos_sensor_twobyte(0x6F12, 0x401C);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x25F9);
	write_cmos_sensor_twobyte(0x6F12, 0x7288);
	write_cmos_sensor_twobyte(0x6F12, 0x99F8);
	write_cmos_sensor_twobyte(0x6F12, 0x8C10);
	write_cmos_sensor_twobyte(0x6F12, 0xB2FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF1F1);
	write_cmos_sensor_twobyte(0x6F12, 0x99F8);
	write_cmos_sensor_twobyte(0x6F12, 0x8D20);
	write_cmos_sensor_twobyte(0x6F12, 0x32B1);
	write_cmos_sensor_twobyte(0x6F12, 0xB9F8);
	write_cmos_sensor_twobyte(0x6F12, 0x2420);
	write_cmos_sensor_twobyte(0x6F12, 0x3388);
	write_cmos_sensor_twobyte(0x6F12, 0x521C);
	write_cmos_sensor_twobyte(0x6F12, 0x9342);
	write_cmos_sensor_twobyte(0x6F12, 0x00D2);
	write_cmos_sensor_twobyte(0x6F12, 0x0021);
	write_cmos_sensor_twobyte(0x6F12, 0x9BF8);
	write_cmos_sensor_twobyte(0x6F12, 0x0120);
	write_cmos_sensor_twobyte(0x6F12, 0x5B46);
	write_cmos_sensor_twobyte(0x6F12, 0x5AB1);
	write_cmos_sensor_twobyte(0x6F12, 0x9E88);
	write_cmos_sensor_twobyte(0x6F12, 0x8642);
	write_cmos_sensor_twobyte(0x6F12, 0x08D8);
	write_cmos_sensor_twobyte(0x6F12, 0xDE88);
	write_cmos_sensor_twobyte(0x6F12, 0x8642);
	write_cmos_sensor_twobyte(0x6F12, 0x05D3);
	write_cmos_sensor_twobyte(0x6F12, 0x9E78);
	write_cmos_sensor_twobyte(0x6F12, 0x801B);
	write_cmos_sensor_twobyte(0x6F12, 0xB0FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF2F0);
	write_cmos_sensor_twobyte(0x6F12, 0x00FB);
	write_cmos_sensor_twobyte(0x6F12, 0x0260);
	write_cmos_sensor_twobyte(0x6F12, 0x3880);
	write_cmos_sensor_twobyte(0x6F12, 0x7980);
	write_cmos_sensor_twobyte(0x6F12, 0xBAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x7860);
	write_cmos_sensor_twobyte(0x6F12, 0x00FB);
	write_cmos_sensor_twobyte(0x6F12, 0x0460);
	write_cmos_sensor_twobyte(0x6F12, 0x0144);
	write_cmos_sensor_twobyte(0x6F12, 0x4FF4);
	write_cmos_sensor_twobyte(0x6F12, 0x8050);
	write_cmos_sensor_twobyte(0x6F12, 0xC5E9);
	write_cmos_sensor_twobyte(0x6F12, 0x0010);
	write_cmos_sensor_twobyte(0x6F12, 0x5878);
	write_cmos_sensor_twobyte(0x6F12, 0x0028);
	write_cmos_sensor_twobyte(0x6F12, 0xC5D0);
	write_cmos_sensor_twobyte(0x6F12, 0x9BF8);
	write_cmos_sensor_twobyte(0x6F12, 0x0300);
	write_cmos_sensor_twobyte(0x6F12, 0x0128);
	write_cmos_sensor_twobyte(0x6F12, 0x02D0);
	write_cmos_sensor_twobyte(0x6F12, 0xBAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x7800);
	write_cmos_sensor_twobyte(0x6F12, 0x091A);
	write_cmos_sensor_twobyte(0x6F12, 0x0C22);
	write_cmos_sensor_twobyte(0x6F12, 0x4046);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xF2F8);
	write_cmos_sensor_twobyte(0x6F12, 0xB8E7);
	write_cmos_sensor_twobyte(0x6F12, 0x38B5);
	write_cmos_sensor_twobyte(0x6F12, 0x0446);
	write_cmos_sensor_twobyte(0x6F12, 0x0122);
	write_cmos_sensor_twobyte(0x6F12, 0x6946);
	write_cmos_sensor_twobyte(0x6F12, 0x4FF6);
	write_cmos_sensor_twobyte(0x6F12, 0xFD70);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xEEF8);
	write_cmos_sensor_twobyte(0x6F12, 0x9DF8);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x18B9);
	write_cmos_sensor_twobyte(0x6F12, 0x6148);
	write_cmos_sensor_twobyte(0x6F12, 0x007A);
	write_cmos_sensor_twobyte(0x6F12, 0x0128);
	write_cmos_sensor_twobyte(0x6F12, 0x0AD0);
	write_cmos_sensor_twobyte(0x6F12, 0x604D);
	write_cmos_sensor_twobyte(0x6F12, 0x2888);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xE8F8);
	write_cmos_sensor_twobyte(0x6F12, 0x2046);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xEAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x2888);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xECF8);
	write_cmos_sensor_twobyte(0x6F12, 0x38BD);
	write_cmos_sensor_twobyte(0x6F12, 0x5B49);
	write_cmos_sensor_twobyte(0x6F12, 0xFA20);
	write_cmos_sensor_twobyte(0x6F12, 0x0968);
	write_cmos_sensor_twobyte(0x6F12, 0x0880);
	write_cmos_sensor_twobyte(0x6F12, 0xFEE7);
	write_cmos_sensor_twobyte(0x6F12, 0x70B5);
	write_cmos_sensor_twobyte(0x6F12, 0x0446);
	write_cmos_sensor_twobyte(0x6F12, 0xB0FA);
	write_cmos_sensor_twobyte(0x6F12, 0x80F0);
	write_cmos_sensor_twobyte(0x6F12, 0x0D46);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F1);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F1);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0x0821);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0xDFF8);
	write_cmos_sensor_twobyte(0x6F12, 0x8440);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F1);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0xC540);
	write_cmos_sensor_twobyte(0x6F12, 0xB4FB);
	write_cmos_sensor_twobyte(0x6F12, 0xF5F0);
	write_cmos_sensor_twobyte(0x6F12, 0x05FB);
	write_cmos_sensor_twobyte(0x6F12, 0x1041);
	write_cmos_sensor_twobyte(0x6F12, 0xB5EB);
	write_cmos_sensor_twobyte(0x6F12, 0x410F);
	write_cmos_sensor_twobyte(0x6F12, 0x00D2);
	write_cmos_sensor_twobyte(0x6F12, 0x401C);
	write_cmos_sensor_twobyte(0x6F12, 0x70BD);
	write_cmos_sensor_twobyte(0x6F12, 0x2DE9);
	write_cmos_sensor_twobyte(0x6F12, 0xFE4F);
	write_cmos_sensor_twobyte(0x6F12, 0x0746);
	write_cmos_sensor_twobyte(0x6F12, 0x0020);
	write_cmos_sensor_twobyte(0x6F12, 0x0290);
	write_cmos_sensor_twobyte(0x6F12, 0x0C46);
	write_cmos_sensor_twobyte(0x6F12, 0x081D);
	write_cmos_sensor_twobyte(0x6F12, 0xC030);
	write_cmos_sensor_twobyte(0x6F12, 0x00BF);
	write_cmos_sensor_twobyte(0x6F12, 0x8246);
	write_cmos_sensor_twobyte(0x6F12, 0x201D);
	write_cmos_sensor_twobyte(0x6F12, 0xEC30);
	write_cmos_sensor_twobyte(0x6F12, 0x00BF);
	write_cmos_sensor_twobyte(0x6F12, 0xD0E9);
	write_cmos_sensor_twobyte(0x6F12, 0x0013);
	write_cmos_sensor_twobyte(0x6F12, 0x5943);
	write_cmos_sensor_twobyte(0x6F12, 0xD0E9);
	write_cmos_sensor_twobyte(0x6F12, 0x0236);
	write_cmos_sensor_twobyte(0x6F12, 0x090A);
	write_cmos_sensor_twobyte(0x6F12, 0x5943);
	write_cmos_sensor_twobyte(0x6F12, 0x4FEA);
	write_cmos_sensor_twobyte(0x6F12, 0x1129);
	write_cmos_sensor_twobyte(0x6F12, 0xD0E9);
	write_cmos_sensor_twobyte(0x6F12, 0x0413);
	write_cmos_sensor_twobyte(0x6F12, 0x1830);
	write_cmos_sensor_twobyte(0x6F12, 0x5943);
	write_cmos_sensor_twobyte(0x6F12, 0x21C8);
	write_cmos_sensor_twobyte(0x6F12, 0x090A);
	write_cmos_sensor_twobyte(0x6F12, 0x4143);
	write_cmos_sensor_twobyte(0x6F12, 0x4FEA);
	write_cmos_sensor_twobyte(0x6F12, 0x1128);
	write_cmos_sensor_twobyte(0x6F12, 0x04F5);
	write_cmos_sensor_twobyte(0x6F12, 0xF66B);
	write_cmos_sensor_twobyte(0x6F12, 0x4946);
	write_cmos_sensor_twobyte(0x6F12, 0x4046);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x8FF8);
	write_cmos_sensor_twobyte(0x6F12, 0xB0FA);
	write_cmos_sensor_twobyte(0x6F12, 0x80F0);
	write_cmos_sensor_twobyte(0x6F12, 0xB5FA);
	write_cmos_sensor_twobyte(0x6F12, 0x85F1);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F1);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0xC1F1);
	write_cmos_sensor_twobyte(0x6F12, 0x2001);
	write_cmos_sensor_twobyte(0x6F12, 0x0844);
	write_cmos_sensor_twobyte(0x6F12, 0x0021);
	write_cmos_sensor_twobyte(0x6F12, 0x2038);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x82F8);
	write_cmos_sensor_twobyte(0x6F12, 0x0828);
	write_cmos_sensor_twobyte(0x6F12, 0x26FA);
	write_cmos_sensor_twobyte(0x6F12, 0x00F6);
	write_cmos_sensor_twobyte(0x6F12, 0x06FB);
	write_cmos_sensor_twobyte(0x6F12, 0x09F6);
	write_cmos_sensor_twobyte(0x6F12, 0x07DC);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F1);
	write_cmos_sensor_twobyte(0x6F12, 0x0801);
	write_cmos_sensor_twobyte(0x6F12, 0xCE40);
	write_cmos_sensor_twobyte(0x6F12, 0xC540);
	write_cmos_sensor_twobyte(0x6F12, 0x05FB);
	write_cmos_sensor_twobyte(0x6F12, 0x08F5);
	write_cmos_sensor_twobyte(0x6F12, 0xCD40);
	write_cmos_sensor_twobyte(0x6F12, 0x06E0);
	write_cmos_sensor_twobyte(0x6F12, 0xA0F1);
	write_cmos_sensor_twobyte(0x6F12, 0x0801);
	write_cmos_sensor_twobyte(0x6F12, 0x8E40);
	write_cmos_sensor_twobyte(0x6F12, 0xC540);
	write_cmos_sensor_twobyte(0x6F12, 0x05FB);
	write_cmos_sensor_twobyte(0x6F12, 0x08F5);
	write_cmos_sensor_twobyte(0x6F12, 0x8D40);
	write_cmos_sensor_twobyte(0x6F12, 0xCDE9);
	write_cmos_sensor_twobyte(0x6F12, 0x0065);
	write_cmos_sensor_twobyte(0x6F12, 0xF868);
	write_cmos_sensor_twobyte(0x6F12, 0x5A46);
	write_cmos_sensor_twobyte(0x6F12, 0x0168);
	write_cmos_sensor_twobyte(0x6F12, 0x8B68);
	write_cmos_sensor_twobyte(0x6F12, 0x6946);
	write_cmos_sensor_twobyte(0x6F12, 0x9847);
	write_cmos_sensor_twobyte(0x6F12, 0x04F2);
	write_cmos_sensor_twobyte(0x6F12, 0xA474);
	write_cmos_sensor_twobyte(0x6F12, 0xDAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x1810);
	write_cmos_sensor_twobyte(0x6F12, 0xA069);
	write_cmos_sensor_twobyte(0x6F12, 0xFFF7);
	write_cmos_sensor_twobyte(0x6F12, 0x91FF);
	write_cmos_sensor_twobyte(0x6F12, 0x2060);
	write_cmos_sensor_twobyte(0x6F12, 0xE069);
	write_cmos_sensor_twobyte(0x6F12, 0xDAF8);
	write_cmos_sensor_twobyte(0x6F12, 0x1810);
	write_cmos_sensor_twobyte(0x6F12, 0xFFF7);
	write_cmos_sensor_twobyte(0x6F12, 0x8BFF);
	write_cmos_sensor_twobyte(0x6F12, 0x6060);
	write_cmos_sensor_twobyte(0x6F12, 0xD7F8);
	write_cmos_sensor_twobyte(0x6F12, 0x3801);
	write_cmos_sensor_twobyte(0x6F12, 0x40F4);
	write_cmos_sensor_twobyte(0x6F12, 0x0050);
	write_cmos_sensor_twobyte(0x6F12, 0xC7F8);
	write_cmos_sensor_twobyte(0x6F12, 0x3801);
	write_cmos_sensor_twobyte(0x6F12, 0x0298);
	write_cmos_sensor_twobyte(0x6F12, 0x92E6);
	write_cmos_sensor_twobyte(0x6F12, 0x10B5);
	write_cmos_sensor_twobyte(0x6F12, 0x0022);
	write_cmos_sensor_twobyte(0x6F12, 0xAFF2);
	write_cmos_sensor_twobyte(0x6F12, 0xF351);
	write_cmos_sensor_twobyte(0x6F12, 0x1948);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x6BF8);
	write_cmos_sensor_twobyte(0x6F12, 0x0022);
	write_cmos_sensor_twobyte(0x6F12, 0xAFF2);
	write_cmos_sensor_twobyte(0x6F12, 0xD751);
	write_cmos_sensor_twobyte(0x6F12, 0x1748);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x65F8);
	write_cmos_sensor_twobyte(0x6F12, 0x0022);
	write_cmos_sensor_twobyte(0x6F12, 0xAFF2);
	write_cmos_sensor_twobyte(0x6F12, 0x6B51);
	write_cmos_sensor_twobyte(0x6F12, 0x1548);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x5FF8);
	write_cmos_sensor_twobyte(0x6F12, 0x0022);
	write_cmos_sensor_twobyte(0x6F12, 0xAFF2);
	write_cmos_sensor_twobyte(0x6F12, 0x0541);
	write_cmos_sensor_twobyte(0x6F12, 0x1348);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x59F8);
	write_cmos_sensor_twobyte(0x6F12, 0x0022);
	write_cmos_sensor_twobyte(0x6F12, 0xAFF2);
	write_cmos_sensor_twobyte(0x6F12, 0x0331);
	write_cmos_sensor_twobyte(0x6F12, 0x1148);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x53F8);
	write_cmos_sensor_twobyte(0x6F12, 0x0022);
	write_cmos_sensor_twobyte(0x6F12, 0xAFF2);
	write_cmos_sensor_twobyte(0x6F12, 0x7B11);
	write_cmos_sensor_twobyte(0x6F12, 0x0F48);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x4DF8);
	write_cmos_sensor_twobyte(0x6F12, 0x064C);
	write_cmos_sensor_twobyte(0x6F12, 0x0122);
	write_cmos_sensor_twobyte(0x6F12, 0x2080);
	write_cmos_sensor_twobyte(0x6F12, 0x0D48);
	write_cmos_sensor_twobyte(0x6F12, 0x0068);
	write_cmos_sensor_twobyte(0x6F12, 0xAFF2);
	write_cmos_sensor_twobyte(0x6F12, 0x1F11);
	write_cmos_sensor_twobyte(0x6F12, 0x00F0);
	write_cmos_sensor_twobyte(0x6F12, 0x44F8);
	write_cmos_sensor_twobyte(0x6F12, 0x6060);
	write_cmos_sensor_twobyte(0x6F12, 0x10BD);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x2001);
	write_cmos_sensor_twobyte(0x6F12, 0xAB00);
	write_cmos_sensor_twobyte(0x6F12, 0x2001);
	write_cmos_sensor_twobyte(0x6F12, 0x54E0);
	write_cmos_sensor_twobyte(0x6F12, 0x2000);
	write_cmos_sensor_twobyte(0x6F12, 0x05D0);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x6F0F);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0xC54D);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0xC59D);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0xB5AD);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x72AD);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x3909);
	write_cmos_sensor_twobyte(0x6F12, 0x2001);
	write_cmos_sensor_twobyte(0x6F12, 0x54C0);
	write_cmos_sensor_twobyte(0x6F12, 0x40F2);
	write_cmos_sensor_twobyte(0x6F12, 0x754C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x4CF2);
	write_cmos_sensor_twobyte(0x6F12, 0x4D5C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x41F6);
	write_cmos_sensor_twobyte(0x6F12, 0x8F7C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x41F6);
	write_cmos_sensor_twobyte(0x6F12, 0x497C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x41F2);
	write_cmos_sensor_twobyte(0x6F12, 0x757C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x40F2);
	write_cmos_sensor_twobyte(0x6F12, 0xE12C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x43F6);
	write_cmos_sensor_twobyte(0x6F12, 0x091C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x40F2);
	write_cmos_sensor_twobyte(0x6F12, 0xF12C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x41F6);
	write_cmos_sensor_twobyte(0x6F12, 0x877C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x40F2);
	write_cmos_sensor_twobyte(0x6F12, 0x013C);
	write_cmos_sensor_twobyte(0x6F12, 0xC0F2);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x6F12, 0x6047);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x6F12, 0x058F);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x2188);
	write_cmos_sensor_twobyte(0x6F12, 0x07D6);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x005F);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x177C);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x19FA);
	write_cmos_sensor_twobyte(0x6F12, 0x0101);
	write_cmos_sensor_twobyte(0x602A, 0x19FC);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x1718);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0xF4FC, 0x4D17);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x0E24);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x0E64);
	write_cmos_sensor_twobyte(0x6F12, 0x004A);
	write_cmos_sensor_twobyte(0x602A, 0x0EB0);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x602A, 0x0EB2);
	write_cmos_sensor_twobyte(0x6F12, 0x0020);
	write_cmos_sensor_twobyte(0x6F12, 0x0020);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0xF442);
	write_cmos_sensor_twobyte(0x602A, 0x0E8A);
	write_cmos_sensor_twobyte(0x6F12, 0xFDE9);
	write_cmos_sensor_twobyte(0x602A, 0x0E88);
	write_cmos_sensor_twobyte(0x6F12, 0x3EB8);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0xF440, 0x0000);
	write_cmos_sensor_twobyte(0xF4AA, 0x0040);
	write_cmos_sensor_twobyte(0xF486, 0x0000);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0xF442, 0x0000);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x14FE);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0E36);
	write_cmos_sensor_twobyte(0x6F12, 0x0006);
	write_cmos_sensor_twobyte(0x602A, 0x0E38);
	write_cmos_sensor_twobyte(0x6F12, 0x0606);
	write_cmos_sensor_twobyte(0x602A, 0x0E3A);
	write_cmos_sensor_twobyte(0x6F12, 0x060F);
	write_cmos_sensor_twobyte(0x602A, 0x0E3C);
	write_cmos_sensor_twobyte(0x6F12, 0x0F0F);
	write_cmos_sensor_twobyte(0x602A, 0x0E3E);
	write_cmos_sensor_twobyte(0x6F12, 0x0F30);
	write_cmos_sensor_twobyte(0x602A, 0x0E40);
	write_cmos_sensor_twobyte(0x6F12, 0x3030);
	write_cmos_sensor_twobyte(0x602A, 0x0E42);
	write_cmos_sensor_twobyte(0x6F12, 0x3003);
	write_cmos_sensor_twobyte(0x602A, 0x0E46);
	write_cmos_sensor_twobyte(0x6F12, 0x0301);
	write_cmos_sensor_twobyte(0x602A, 0x0706);
	write_cmos_sensor_twobyte(0x6F12, 0x01C0);
	write_cmos_sensor_twobyte(0x6F12, 0x01C0);
	write_cmos_sensor_twobyte(0x6F12, 0x01C0);
	write_cmos_sensor_twobyte(0x6F12, 0x01C0);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0xF4AC, 0x0062);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x0CF4);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0D26);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0D28);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0E80);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6028, 0x2001);
	write_cmos_sensor_twobyte(0x602A, 0xAB00);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0xAB02);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0xAB04);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0x3092, 0x7E50);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x1F1A);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x1D5E);
	write_cmos_sensor_twobyte(0x6F12, 0x0358);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x06AC);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x602A, 0x06A6);
	write_cmos_sensor_twobyte(0x6F12, 0x0108);
	write_cmos_sensor_twobyte(0x602A, 0x06A8);
	write_cmos_sensor_twobyte(0x6F12, 0x0C01);
	write_cmos_sensor_twobyte(0x602A, 0x06FA);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0x021E, 0x0400);
	write_cmos_sensor_twobyte(0x021C, 0x0001);
	write_cmos_sensor_twobyte(0x020E, 0x0100);
	write_cmos_sensor_twobyte(0x3074, 0x0100);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x0EFA);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0x0404, 0x0010);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0xB4D6);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x6878);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x58C0);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0x30E6, 0x0000);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x7500);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x78D0);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x6840);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x5522);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x7CA0);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x6A20);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x6C70);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x6EC0);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x8F30);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x1006);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x100E);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x0EFE);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x69A0);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0xA8D8);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0x3176, 0x0000);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x9010);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x9018);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x9020);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x9028);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x90B0);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x58A0);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x6880);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x93B0);
	write_cmos_sensor_twobyte(0x6F12, 0x0101);
	write_cmos_sensor_twobyte(0x602A, 0x91A0);
	write_cmos_sensor_twobyte(0x6F12, 0x0003);
	write_cmos_sensor_twobyte(0x602A, 0x51E4);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0687);
	write_cmos_sensor_twobyte(0x6F12, 0x0005);
	write_cmos_sensor_twobyte(0x602A, 0x072E);
	write_cmos_sensor_twobyte(0x6F12, 0x004D);
	write_cmos_sensor_twobyte(0x602A, 0x0732);
	write_cmos_sensor_twobyte(0x6F12, 0x004A);
	write_cmos_sensor_twobyte(0x602A, 0x0736);
	write_cmos_sensor_twobyte(0x6F12, 0x002D);
	write_cmos_sensor_twobyte(0x602A, 0x073A);
	write_cmos_sensor_twobyte(0x6F12, 0x000A);
	write_cmos_sensor_twobyte(0x602A, 0x0746);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x074A);
	write_cmos_sensor_twobyte(0x6F12, 0x0012);
	write_cmos_sensor_twobyte(0x602A, 0x073E);
	write_cmos_sensor_twobyte(0x6F12, 0x0024);
	write_cmos_sensor_twobyte(0x602A, 0x0742);
	write_cmos_sensor_twobyte(0x6F12, 0x001E);
	write_cmos_sensor_twobyte(0x602A, 0x074E);
	write_cmos_sensor_twobyte(0x6F12, 0x0276);
	write_cmos_sensor_twobyte(0x602A, 0x0752);
	write_cmos_sensor_twobyte(0x6F12, 0x030C);
	write_cmos_sensor_twobyte(0x602A, 0x0756);
	write_cmos_sensor_twobyte(0x6F12, 0x0026);
	write_cmos_sensor_twobyte(0x602A, 0x075A);
	write_cmos_sensor_twobyte(0x6F12, 0x003A);
	write_cmos_sensor_twobyte(0x602A, 0x075E);
	write_cmos_sensor_twobyte(0x6F12, 0x002E);
	write_cmos_sensor_twobyte(0x602A, 0x0762);
	write_cmos_sensor_twobyte(0x6F12, 0x0035);
	write_cmos_sensor_twobyte(0x602A, 0x0766);
	write_cmos_sensor_twobyte(0x6F12, 0x0009);
	write_cmos_sensor_twobyte(0x602A, 0x076A);
	write_cmos_sensor_twobyte(0x6F12, 0x0002);
	write_cmos_sensor_twobyte(0x602A, 0x076E);
	write_cmos_sensor_twobyte(0x6F12, 0x0002);
	write_cmos_sensor_twobyte(0x602A, 0x0772);
	write_cmos_sensor_twobyte(0x6F12, 0x0009);
	write_cmos_sensor_twobyte(0x602A, 0x0776);
	write_cmos_sensor_twobyte(0x6F12, 0x0025);
	write_cmos_sensor_twobyte(0x602A, 0x077A);
	write_cmos_sensor_twobyte(0x6F12, 0x002C);
	write_cmos_sensor_twobyte(0x602A, 0x077E);
	write_cmos_sensor_twobyte(0x6F12, 0x03DA);
	write_cmos_sensor_twobyte(0x602A, 0x0782);
	write_cmos_sensor_twobyte(0x6F12, 0x03D3);
	write_cmos_sensor_twobyte(0x602A, 0x0786);
	write_cmos_sensor_twobyte(0x6F12, 0x0363);
	write_cmos_sensor_twobyte(0x602A, 0x078A);
	write_cmos_sensor_twobyte(0x6F12, 0x035C);
	write_cmos_sensor_twobyte(0x602A, 0x078E);
	write_cmos_sensor_twobyte(0x6F12, 0x00C3);
	write_cmos_sensor_twobyte(0x602A, 0x0792);
	write_cmos_sensor_twobyte(0x6F12, 0x00BC);
	write_cmos_sensor_twobyte(0x602A, 0x0796);
	write_cmos_sensor_twobyte(0x6F12, 0x00C3);
	write_cmos_sensor_twobyte(0x602A, 0x079A);
	write_cmos_sensor_twobyte(0x6F12, 0x00BC);
	write_cmos_sensor_twobyte(0x602A, 0x079E);
	write_cmos_sensor_twobyte(0x6F12, 0x0015);
	write_cmos_sensor_twobyte(0x602A, 0x07A2);
	write_cmos_sensor_twobyte(0x6F12, 0x000E);
	write_cmos_sensor_twobyte(0x602A, 0x07A6);
	write_cmos_sensor_twobyte(0x6F12, 0x0015);
	write_cmos_sensor_twobyte(0x602A, 0x07AA);
	write_cmos_sensor_twobyte(0x6F12, 0x000E);
	write_cmos_sensor_twobyte(0x602A, 0x07AE);
	write_cmos_sensor_twobyte(0x6F12, 0x0031);
	write_cmos_sensor_twobyte(0x602A, 0x07B2);
	write_cmos_sensor_twobyte(0x6F12, 0x002A);
	write_cmos_sensor_twobyte(0x602A, 0x07B6);
	write_cmos_sensor_twobyte(0x6F12, 0x0025);
	write_cmos_sensor_twobyte(0x602A, 0x07BA);
	write_cmos_sensor_twobyte(0x6F12, 0x001E);
	write_cmos_sensor_twobyte(0x602A, 0x07BE);
	write_cmos_sensor_twobyte(0x6F12, 0x0277);
	write_cmos_sensor_twobyte(0x602A, 0x07C2);
	write_cmos_sensor_twobyte(0x6F12, 0x027E);
	write_cmos_sensor_twobyte(0x602A, 0x07C6);
	write_cmos_sensor_twobyte(0x6F12, 0x030D);
	write_cmos_sensor_twobyte(0x602A, 0x07CA);
	write_cmos_sensor_twobyte(0x6F12, 0x0314);
	write_cmos_sensor_twobyte(0x602A, 0x07CE);
	write_cmos_sensor_twobyte(0x6F12, 0x03CA);
	write_cmos_sensor_twobyte(0x602A, 0x07D2);
	write_cmos_sensor_twobyte(0x6F12, 0x03C3);
	write_cmos_sensor_twobyte(0x602A, 0x07D6);
	write_cmos_sensor_twobyte(0x6F12, 0x037F);
	write_cmos_sensor_twobyte(0x602A, 0x07DA);
	write_cmos_sensor_twobyte(0x6F12, 0x0378);
	write_cmos_sensor_twobyte(0x602A, 0x07DE);
	write_cmos_sensor_twobyte(0x6F12, 0x00B3);
	write_cmos_sensor_twobyte(0x602A, 0x07E2);
	write_cmos_sensor_twobyte(0x6F12, 0x00AC);
	write_cmos_sensor_twobyte(0x602A, 0x07E6);
	write_cmos_sensor_twobyte(0x6F12, 0x001D);
	write_cmos_sensor_twobyte(0x602A, 0x07EA);
	write_cmos_sensor_twobyte(0x6F12, 0x0016);
	write_cmos_sensor_twobyte(0x602A, 0x07EE);
	write_cmos_sensor_twobyte(0x6F12, 0x0277);
	write_cmos_sensor_twobyte(0x602A, 0x07F2);
	write_cmos_sensor_twobyte(0x6F12, 0x027E);
	write_cmos_sensor_twobyte(0x602A, 0x07F6);
	write_cmos_sensor_twobyte(0x6F12, 0x030D);
	write_cmos_sensor_twobyte(0x602A, 0x07FA);
	write_cmos_sensor_twobyte(0x6F12, 0x0314);
	write_cmos_sensor_twobyte(0x602A, 0x07FE);
	write_cmos_sensor_twobyte(0x6F12, 0x03BE);
	write_cmos_sensor_twobyte(0x602A, 0x0802);
	write_cmos_sensor_twobyte(0x6F12, 0x03B7);
	write_cmos_sensor_twobyte(0x602A, 0x0806);
	write_cmos_sensor_twobyte(0x6F12, 0x0373);
	write_cmos_sensor_twobyte(0x602A, 0x080A);
	write_cmos_sensor_twobyte(0x6F12, 0x036C);
	write_cmos_sensor_twobyte(0x602A, 0x080E);
	write_cmos_sensor_twobyte(0x6F12, 0x00A7);
	write_cmos_sensor_twobyte(0x602A, 0x0812);
	write_cmos_sensor_twobyte(0x6F12, 0x00A0);
	write_cmos_sensor_twobyte(0x602A, 0x0816);
	write_cmos_sensor_twobyte(0x6F12, 0x0011);
	write_cmos_sensor_twobyte(0x602A, 0x081A);
	write_cmos_sensor_twobyte(0x6F12, 0x000A);
	write_cmos_sensor_twobyte(0x602A, 0x081E);
	write_cmos_sensor_twobyte(0x6F12, 0x0027);
	write_cmos_sensor_twobyte(0x602A, 0x0822);
	write_cmos_sensor_twobyte(0x6F12, 0x002E);
	write_cmos_sensor_twobyte(0x602A, 0x0826);
	write_cmos_sensor_twobyte(0x6F12, 0x003B);
	write_cmos_sensor_twobyte(0x602A, 0x082A);
	write_cmos_sensor_twobyte(0x6F12, 0x0042);
	write_cmos_sensor_twobyte(0x602A, 0x082E);
	write_cmos_sensor_twobyte(0x6F12, 0x00CB);
	write_cmos_sensor_twobyte(0x602A, 0x0832);
	write_cmos_sensor_twobyte(0x6F12, 0x00C4);
	write_cmos_sensor_twobyte(0x602A, 0x0836);
	write_cmos_sensor_twobyte(0x6F12, 0x0019);
	write_cmos_sensor_twobyte(0x602A, 0x083A);
	write_cmos_sensor_twobyte(0x6F12, 0x0012);
	write_cmos_sensor_twobyte(0x602A, 0x0B9A);
	write_cmos_sensor_twobyte(0x6F12, 0x03DB);
	write_cmos_sensor_twobyte(0x602A, 0x0B9E);
	write_cmos_sensor_twobyte(0x6F12, 0x0364);
	write_cmos_sensor_twobyte(0x602A, 0x0BA2);
	write_cmos_sensor_twobyte(0x6F12, 0x00C4);
	write_cmos_sensor_twobyte(0x602A, 0x0BA6);
	write_cmos_sensor_twobyte(0x6F12, 0x0002);
	write_cmos_sensor_twobyte(0x602A, 0x0BAA);
	write_cmos_sensor_twobyte(0x6F12, 0x03CB);
	write_cmos_sensor_twobyte(0x602A, 0x0BAE);
	write_cmos_sensor_twobyte(0x6F12, 0x0380);
	write_cmos_sensor_twobyte(0x602A, 0x0BB2);
	write_cmos_sensor_twobyte(0x6F12, 0x00B4);
	write_cmos_sensor_twobyte(0x602A, 0x0BB6);
	write_cmos_sensor_twobyte(0x6F12, 0x001E);
	write_cmos_sensor_twobyte(0x602A, 0x0BCA);
	write_cmos_sensor_twobyte(0x6F12, 0x03BF);
	write_cmos_sensor_twobyte(0x602A, 0x0BCE);
	write_cmos_sensor_twobyte(0x6F12, 0x0374);
	write_cmos_sensor_twobyte(0x602A, 0x0BD2);
	write_cmos_sensor_twobyte(0x6F12, 0x00A8);
	write_cmos_sensor_twobyte(0x602A, 0x0BD6);
	write_cmos_sensor_twobyte(0x6F12, 0x0012);
	write_cmos_sensor_twobyte(0x602A, 0x0BEA);
	write_cmos_sensor_twobyte(0x6F12, 0x00CC);
	write_cmos_sensor_twobyte(0x602A, 0x0BEE);
	write_cmos_sensor_twobyte(0x6F12, 0x0006);
	write_cmos_sensor_twobyte(0x602A, 0x0BBA);
	write_cmos_sensor_twobyte(0x6F12, 0x03CB);
	write_cmos_sensor_twobyte(0x602A, 0x0BBE);
	write_cmos_sensor_twobyte(0x6F12, 0x0380);
	write_cmos_sensor_twobyte(0x602A, 0x0BC2);
	write_cmos_sensor_twobyte(0x6F12, 0x00B4);
	write_cmos_sensor_twobyte(0x602A, 0x0BC6);
	write_cmos_sensor_twobyte(0x6F12, 0x001E);
	write_cmos_sensor_twobyte(0x602A, 0x0BDA);
	write_cmos_sensor_twobyte(0x6F12, 0x03BF);
	write_cmos_sensor_twobyte(0x602A, 0x0BDE);
	write_cmos_sensor_twobyte(0x6F12, 0x0374);
	write_cmos_sensor_twobyte(0x602A, 0x0BE2);
	write_cmos_sensor_twobyte(0x6F12, 0x00A8);
	write_cmos_sensor_twobyte(0x602A, 0x0BE6);
	write_cmos_sensor_twobyte(0x6F12, 0x0012);
	write_cmos_sensor_twobyte(0x602A, 0x0BF2);
	write_cmos_sensor_twobyte(0x6F12, 0x001E);
	write_cmos_sensor_twobyte(0x602A, 0x083E);
	write_cmos_sensor_twobyte(0x6F12, 0x0262);
	write_cmos_sensor_twobyte(0x602A, 0x0842);
	write_cmos_sensor_twobyte(0x6F12, 0x059F);
	write_cmos_sensor_twobyte(0x602A, 0x0846);
	write_cmos_sensor_twobyte(0x6F12, 0x0276);
	write_cmos_sensor_twobyte(0x602A, 0x084A);
	write_cmos_sensor_twobyte(0x6F12, 0x059D);
	write_cmos_sensor_twobyte(0x602A, 0x084E);
	write_cmos_sensor_twobyte(0x6F12, 0x0262);
	write_cmos_sensor_twobyte(0x602A, 0x0852);
	write_cmos_sensor_twobyte(0x6F12, 0x0324);
	write_cmos_sensor_twobyte(0x602A, 0x0856);
	write_cmos_sensor_twobyte(0x6F12, 0x001A);
	write_cmos_sensor_twobyte(0x602A, 0x085A);
	write_cmos_sensor_twobyte(0x6F12, 0x0114);
	write_cmos_sensor_twobyte(0x602A, 0x085E);
	write_cmos_sensor_twobyte(0x6F12, 0x001A);
	write_cmos_sensor_twobyte(0x602A, 0x0862);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x602A, 0x0866);
	write_cmos_sensor_twobyte(0x6F12, 0x001A);
	write_cmos_sensor_twobyte(0x602A, 0x086A);
	write_cmos_sensor_twobyte(0x6F12, 0x010C);
	write_cmos_sensor_twobyte(0x602A, 0x086E);
	write_cmos_sensor_twobyte(0x6F12, 0x001A);
	write_cmos_sensor_twobyte(0x602A, 0x0872);
	write_cmos_sensor_twobyte(0x6F12, 0x0262);
	write_cmos_sensor_twobyte(0x602A, 0x0876);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x087A);
	write_cmos_sensor_twobyte(0x6F12, 0x004C);
	write_cmos_sensor_twobyte(0x602A, 0x087E);
	write_cmos_sensor_twobyte(0x6F12, 0x030C);
	write_cmos_sensor_twobyte(0x602A, 0x0882);
	write_cmos_sensor_twobyte(0x6F12, 0x0324);
	write_cmos_sensor_twobyte(0x602A, 0x0886);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x088A);
	write_cmos_sensor_twobyte(0x6F12, 0x004C);
	write_cmos_sensor_twobyte(0x602A, 0x088E);
	write_cmos_sensor_twobyte(0x6F12, 0x030C);
	write_cmos_sensor_twobyte(0x602A, 0x0892);
	write_cmos_sensor_twobyte(0x6F12, 0x0314);
	write_cmos_sensor_twobyte(0x602A, 0x08AE);
	write_cmos_sensor_twobyte(0x6F12, 0x0008);
	write_cmos_sensor_twobyte(0x602A, 0x08B2);
	write_cmos_sensor_twobyte(0x6F12, 0x0005);
	write_cmos_sensor_twobyte(0x602A, 0x08B6);
	write_cmos_sensor_twobyte(0x6F12, 0x0042);
	write_cmos_sensor_twobyte(0x602A, 0x08BA);
	write_cmos_sensor_twobyte(0x6F12, 0x05A5);
	write_cmos_sensor_twobyte(0x602A, 0x08BE);
	write_cmos_sensor_twobyte(0x6F12, 0x0204);
	write_cmos_sensor_twobyte(0x602A, 0x08C2);
	write_cmos_sensor_twobyte(0x6F12, 0x0262);
	write_cmos_sensor_twobyte(0x602A, 0x08C6);
	write_cmos_sensor_twobyte(0x6F12, 0x046A);
	write_cmos_sensor_twobyte(0x602A, 0x08CA);
	write_cmos_sensor_twobyte(0x6F12, 0x059B);
	write_cmos_sensor_twobyte(0x602A, 0x08CE);
	write_cmos_sensor_twobyte(0x6F12, 0x0267);
	write_cmos_sensor_twobyte(0x602A, 0x08D2);
	write_cmos_sensor_twobyte(0x6F12, 0x027E);
	write_cmos_sensor_twobyte(0x602A, 0x08D6);
	write_cmos_sensor_twobyte(0x6F12, 0x026E);
	write_cmos_sensor_twobyte(0x602A, 0x08DA);
	write_cmos_sensor_twobyte(0x6F12, 0x0286);
	write_cmos_sensor_twobyte(0x602A, 0x08DE);
	write_cmos_sensor_twobyte(0x6F12, 0x0276);
	write_cmos_sensor_twobyte(0x602A, 0x08E2);
	write_cmos_sensor_twobyte(0x6F12, 0x0286);
	write_cmos_sensor_twobyte(0x602A, 0x08E6);
	write_cmos_sensor_twobyte(0x6F12, 0x0267);
	write_cmos_sensor_twobyte(0x602A, 0x08EA);
	write_cmos_sensor_twobyte(0x6F12, 0x026C);
	write_cmos_sensor_twobyte(0x602A, 0x08EE);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x08F2);
	write_cmos_sensor_twobyte(0x6F12, 0x0008);
	write_cmos_sensor_twobyte(0x602A, 0x08F6);
	write_cmos_sensor_twobyte(0x6F12, 0x026E);
	write_cmos_sensor_twobyte(0x602A, 0x08FA);
	write_cmos_sensor_twobyte(0x6F12, 0x0286);
	write_cmos_sensor_twobyte(0x602A, 0x08FE);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x0902);
	write_cmos_sensor_twobyte(0x6F12, 0x0008);
	write_cmos_sensor_twobyte(0x602A, 0x0906);
	write_cmos_sensor_twobyte(0x6F12, 0x0267);
	write_cmos_sensor_twobyte(0x602A, 0x090A);
	write_cmos_sensor_twobyte(0x6F12, 0x026C);
	write_cmos_sensor_twobyte(0x602A, 0x090E);
	write_cmos_sensor_twobyte(0x6F12, 0x0204);
	write_cmos_sensor_twobyte(0x602A, 0x0912);
	write_cmos_sensor_twobyte(0x6F12, 0x022A);
	write_cmos_sensor_twobyte(0x602A, 0x0916);
	write_cmos_sensor_twobyte(0x6F12, 0x023A);
	write_cmos_sensor_twobyte(0x602A, 0x091A);
	write_cmos_sensor_twobyte(0x6F12, 0x0262);
	write_cmos_sensor_twobyte(0x602A, 0x091E);
	write_cmos_sensor_twobyte(0x6F12, 0x046A);
	write_cmos_sensor_twobyte(0x602A, 0x0922);
	write_cmos_sensor_twobyte(0x6F12, 0x04FA);
	write_cmos_sensor_twobyte(0x602A, 0x092E);
	write_cmos_sensor_twobyte(0x6F12, 0x050A);
	write_cmos_sensor_twobyte(0x602A, 0x0932);
	write_cmos_sensor_twobyte(0x6F12, 0x059B);
	write_cmos_sensor_twobyte(0x602A, 0x0936);
	write_cmos_sensor_twobyte(0x6F12, 0x022C);
	write_cmos_sensor_twobyte(0x602A, 0x093A);
	write_cmos_sensor_twobyte(0x6F12, 0x0231);
	write_cmos_sensor_twobyte(0x602A, 0x093E);
	write_cmos_sensor_twobyte(0x6F12, 0x0268);
	write_cmos_sensor_twobyte(0x602A, 0x0942);
	write_cmos_sensor_twobyte(0x6F12, 0x026E);
	write_cmos_sensor_twobyte(0x602A, 0x0946);
	write_cmos_sensor_twobyte(0x6F12, 0x0279);
	write_cmos_sensor_twobyte(0x602A, 0x094A);
	write_cmos_sensor_twobyte(0x6F12, 0x027F);
	write_cmos_sensor_twobyte(0x602A, 0x094E);
	write_cmos_sensor_twobyte(0x6F12, 0x04FC);
	write_cmos_sensor_twobyte(0x602A, 0x0952);
	write_cmos_sensor_twobyte(0x6F12, 0x0501);
	write_cmos_sensor_twobyte(0x602A, 0x0956);
	write_cmos_sensor_twobyte(0x6F12, 0x059D);
	write_cmos_sensor_twobyte(0x602A, 0x095A);
	write_cmos_sensor_twobyte(0x6F12, 0x05A2);
	write_cmos_sensor_twobyte(0x602A, 0x095E);
	write_cmos_sensor_twobyte(0x6F12, 0x022E);
	write_cmos_sensor_twobyte(0x602A, 0x0962);
	write_cmos_sensor_twobyte(0x6F12, 0x0232);
	write_cmos_sensor_twobyte(0x602A, 0x0966);
	write_cmos_sensor_twobyte(0x6F12, 0x026A);
	write_cmos_sensor_twobyte(0x602A, 0x096A);
	write_cmos_sensor_twobyte(0x6F12, 0x026F);
	write_cmos_sensor_twobyte(0x602A, 0x096E);
	write_cmos_sensor_twobyte(0x6F12, 0x027B);
	write_cmos_sensor_twobyte(0x602A, 0x0972);
	write_cmos_sensor_twobyte(0x6F12, 0x0280);
	write_cmos_sensor_twobyte(0x602A, 0x0976);
	write_cmos_sensor_twobyte(0x6F12, 0x04FE);
	write_cmos_sensor_twobyte(0x602A, 0x097A);
	write_cmos_sensor_twobyte(0x6F12, 0x0502);
	write_cmos_sensor_twobyte(0x602A, 0x097E);
	write_cmos_sensor_twobyte(0x6F12, 0x059F);
	write_cmos_sensor_twobyte(0x602A, 0x0982);
	write_cmos_sensor_twobyte(0x6F12, 0x05A3);
	write_cmos_sensor_twobyte(0x602A, 0x0986);
	write_cmos_sensor_twobyte(0x6F12, 0x022F);
	write_cmos_sensor_twobyte(0x602A, 0x098A);
	write_cmos_sensor_twobyte(0x6F12, 0x0232);
	write_cmos_sensor_twobyte(0x602A, 0x098E);
	write_cmos_sensor_twobyte(0x6F12, 0x026B);
	write_cmos_sensor_twobyte(0x602A, 0x0992);
	write_cmos_sensor_twobyte(0x6F12, 0x026F);
	write_cmos_sensor_twobyte(0x602A, 0x0996);
	write_cmos_sensor_twobyte(0x6F12, 0x027C);
	write_cmos_sensor_twobyte(0x602A, 0x099A);
	write_cmos_sensor_twobyte(0x6F12, 0x0280);
	write_cmos_sensor_twobyte(0x602A, 0x099E);
	write_cmos_sensor_twobyte(0x6F12, 0x04FF);
	write_cmos_sensor_twobyte(0x602A, 0x09A2);
	write_cmos_sensor_twobyte(0x6F12, 0x0502);
	write_cmos_sensor_twobyte(0x602A, 0x09A6);
	write_cmos_sensor_twobyte(0x6F12, 0x05A0);
	write_cmos_sensor_twobyte(0x602A, 0x09AA);
	write_cmos_sensor_twobyte(0x6F12, 0x05A3);
	write_cmos_sensor_twobyte(0x602A, 0x09AE);
	write_cmos_sensor_twobyte(0x6F12, 0x022D);
	write_cmos_sensor_twobyte(0x602A, 0x09B2);
	write_cmos_sensor_twobyte(0x6F12, 0x022F);
	write_cmos_sensor_twobyte(0x602A, 0x09B6);
	write_cmos_sensor_twobyte(0x6F12, 0x0265);
	write_cmos_sensor_twobyte(0x602A, 0x09BA);
	write_cmos_sensor_twobyte(0x6F12, 0x0268);
	write_cmos_sensor_twobyte(0x602A, 0x09BE);
	write_cmos_sensor_twobyte(0x6F12, 0x0276);
	write_cmos_sensor_twobyte(0x602A, 0x09C2);
	write_cmos_sensor_twobyte(0x6F12, 0x0279);
	write_cmos_sensor_twobyte(0x602A, 0x09C6);
	write_cmos_sensor_twobyte(0x6F12, 0x04FC);
	write_cmos_sensor_twobyte(0x602A, 0x09CA);
	write_cmos_sensor_twobyte(0x6F12, 0x04FF);
	write_cmos_sensor_twobyte(0x602A, 0x09CE);
	write_cmos_sensor_twobyte(0x6F12, 0x059D);
	write_cmos_sensor_twobyte(0x602A, 0x09D2);
	write_cmos_sensor_twobyte(0x6F12, 0x05A0);
	write_cmos_sensor_twobyte(0x602A, 0x09D6);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x09DA);
	write_cmos_sensor_twobyte(0x6F12, 0x0008);
	write_cmos_sensor_twobyte(0x602A, 0x09DE);
	write_cmos_sensor_twobyte(0x6F12, 0x0231);
	write_cmos_sensor_twobyte(0x602A, 0x09E2);
	write_cmos_sensor_twobyte(0x6F12, 0x0234);
	write_cmos_sensor_twobyte(0x602A, 0x09E6);
	write_cmos_sensor_twobyte(0x6F12, 0x026A);
	write_cmos_sensor_twobyte(0x602A, 0x09EA);
	write_cmos_sensor_twobyte(0x6F12, 0x026F);
	write_cmos_sensor_twobyte(0x602A, 0x09EE);
	write_cmos_sensor_twobyte(0x6F12, 0x027B);
	write_cmos_sensor_twobyte(0x602A, 0x09F2);
	write_cmos_sensor_twobyte(0x6F12, 0x0280);
	write_cmos_sensor_twobyte(0x602A, 0x09F6);
	write_cmos_sensor_twobyte(0x6F12, 0x0501);
	write_cmos_sensor_twobyte(0x602A, 0x09FA);
	write_cmos_sensor_twobyte(0x6F12, 0x0504);
	write_cmos_sensor_twobyte(0x602A, 0x09FE);
	write_cmos_sensor_twobyte(0x6F12, 0x05A2);
	write_cmos_sensor_twobyte(0x602A, 0x0A02);
	write_cmos_sensor_twobyte(0x6F12, 0x05A5);
	write_cmos_sensor_twobyte(0x602A, 0x0A06);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x0A0A);
	write_cmos_sensor_twobyte(0x6F12, 0x0008);
	write_cmos_sensor_twobyte(0x602A, 0x0A0E);
	write_cmos_sensor_twobyte(0x6F12, 0x022C);
	write_cmos_sensor_twobyte(0x602A, 0x0A12);
	write_cmos_sensor_twobyte(0x6F12, 0x022F);
	write_cmos_sensor_twobyte(0x602A, 0x0A16);
	write_cmos_sensor_twobyte(0x6F12, 0x0265);
	write_cmos_sensor_twobyte(0x602A, 0x0A1A);
	write_cmos_sensor_twobyte(0x6F12, 0x0268);
	write_cmos_sensor_twobyte(0x602A, 0x0A1E);
	write_cmos_sensor_twobyte(0x6F12, 0x0276);
	write_cmos_sensor_twobyte(0x602A, 0x0A22);
	write_cmos_sensor_twobyte(0x6F12, 0x0279);
	write_cmos_sensor_twobyte(0x602A, 0x0A26);
	write_cmos_sensor_twobyte(0x6F12, 0x04FC);
	write_cmos_sensor_twobyte(0x602A, 0x0A2A);
	write_cmos_sensor_twobyte(0x6F12, 0x04FF);
	write_cmos_sensor_twobyte(0x602A, 0x0A2E);
	write_cmos_sensor_twobyte(0x6F12, 0x059D);
	write_cmos_sensor_twobyte(0x602A, 0x0A32);
	write_cmos_sensor_twobyte(0x6F12, 0x05A0);
	write_cmos_sensor_twobyte(0x602A, 0x0A36);
	write_cmos_sensor_twobyte(0x6F12, 0x0231);
	write_cmos_sensor_twobyte(0x602A, 0x0A3A);
	write_cmos_sensor_twobyte(0x6F12, 0x026A);
	write_cmos_sensor_twobyte(0x602A, 0x0A3E);
	write_cmos_sensor_twobyte(0x6F12, 0x0501);
	write_cmos_sensor_twobyte(0x602A, 0x0A42);
	write_cmos_sensor_twobyte(0x6F12, 0x059F);
	write_cmos_sensor_twobyte(0x602A, 0x0A46);
	write_cmos_sensor_twobyte(0x6F12, 0x0203);
	write_cmos_sensor_twobyte(0x602A, 0x0A4A);
	write_cmos_sensor_twobyte(0x6F12, 0x0263);
	write_cmos_sensor_twobyte(0x602A, 0x0A4E);
	write_cmos_sensor_twobyte(0x6F12, 0x0469);
	write_cmos_sensor_twobyte(0x602A, 0x0A52);
	write_cmos_sensor_twobyte(0x6F12, 0x059C);
	write_cmos_sensor_twobyte(0x602A, 0x0A56);
	write_cmos_sensor_twobyte(0x6F12, 0x0001);
	write_cmos_sensor_twobyte(0x602A, 0x0A5A);
	write_cmos_sensor_twobyte(0x6F12, 0x0008);
	write_cmos_sensor_twobyte(0x602A, 0x0A5E);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x602A, 0x0A62);
	write_cmos_sensor_twobyte(0x6F12, 0x0014);
	write_cmos_sensor_twobyte(0x602A, 0x0A66);
	write_cmos_sensor_twobyte(0x6F12, 0x0267);
	write_cmos_sensor_twobyte(0x602A, 0x0A6A);
	write_cmos_sensor_twobyte(0x6F12, 0x0281);
	write_cmos_sensor_twobyte(0x602A, 0x0A6E);
	write_cmos_sensor_twobyte(0x6F12, 0x000C);
	write_cmos_sensor_twobyte(0x602A, 0x0A72);
	write_cmos_sensor_twobyte(0x6F12, 0x0014);
	write_cmos_sensor_twobyte(0x602A, 0x0A76);
	write_cmos_sensor_twobyte(0x6F12, 0x0267);
	write_cmos_sensor_twobyte(0x602A, 0x0A7A);
	write_cmos_sensor_twobyte(0x6F12, 0x026C);
	write_cmos_sensor_twobyte(0x602A, 0x0A7E);
	write_cmos_sensor_twobyte(0x6F12, 0x003C);
	write_cmos_sensor_twobyte(0x602A, 0x0A82);
	write_cmos_sensor_twobyte(0x6F12, 0x000A);
	write_cmos_sensor_twobyte(0x602A, 0x0A86);
	write_cmos_sensor_twobyte(0x6F12, 0x0132);
	write_cmos_sensor_twobyte(0x602A, 0x0A8A);
	write_cmos_sensor_twobyte(0x6F12, 0x05A1);
	write_cmos_sensor_twobyte(0x602A, 0x0A8E);
	write_cmos_sensor_twobyte(0x6F12, 0x0133);
	write_cmos_sensor_twobyte(0x602A, 0x0A92);
	write_cmos_sensor_twobyte(0x6F12, 0x0135);
	write_cmos_sensor_twobyte(0x602A, 0x0A96);
	write_cmos_sensor_twobyte(0x6F12, 0x0264);
	write_cmos_sensor_twobyte(0x602A, 0x0A9A);
	write_cmos_sensor_twobyte(0x6F12, 0x0267);
	write_cmos_sensor_twobyte(0x602A, 0x0A9E);
	write_cmos_sensor_twobyte(0x6F12, 0x059D);
	write_cmos_sensor_twobyte(0x602A, 0x0AA2);
	write_cmos_sensor_twobyte(0x6F12, 0x05A0);
	write_cmos_sensor_twobyte(0x602A, 0x0AA6);
	write_cmos_sensor_twobyte(0x6F12, 0x0264);
	write_cmos_sensor_twobyte(0x602A, 0x0AAA);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AAE);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AB2);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AB6);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0ABA);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0ABE);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AC2);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AC6);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0ACA);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0ACE);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AD2);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AD6);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0ADA);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0ADE);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AE2);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AE6);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AEA);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AEE);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AF2);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AF6);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AFA);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0AFE);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B02);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B06);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B52);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B56);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B5A);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B5E);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B62);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B66);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B6A);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B6E);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B72);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B76);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B7A);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B7E);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B82);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B86);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B8A);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B8E);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0BF6);
	write_cmos_sensor_twobyte(0x6F12, 0x0006);
	write_cmos_sensor_twobyte(0x602A, 0x0BFA);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0B92);
	write_cmos_sensor_twobyte(0x6F12, 0x0008);
	write_cmos_sensor_twobyte(0x602A, 0x0B96);
	write_cmos_sensor_twobyte(0x6F12, 0x000E);
	write_cmos_sensor_twobyte(0x602A, 0x0BFE);
	write_cmos_sensor_twobyte(0x6F12, 0x001E);
	write_cmos_sensor_twobyte(0x602A, 0x0C02);
	write_cmos_sensor_twobyte(0x6F12, 0x0024);
	write_cmos_sensor_twobyte(0x602A, 0x0C06);
	write_cmos_sensor_twobyte(0x6F12, 0x0034);
	write_cmos_sensor_twobyte(0x602A, 0x0C0A);
	write_cmos_sensor_twobyte(0x6F12, 0x003A);
	write_cmos_sensor_twobyte(0x602A, 0x0C0E);
	write_cmos_sensor_twobyte(0x6F12, 0x004A);
	write_cmos_sensor_twobyte(0x602A, 0x0C12);
	write_cmos_sensor_twobyte(0x6F12, 0x0050);
	write_cmos_sensor_twobyte(0x602A, 0x0C16);
	write_cmos_sensor_twobyte(0x6F12, 0x0060);
	write_cmos_sensor_twobyte(0x602A, 0x0C1A);
	write_cmos_sensor_twobyte(0x6F12, 0x0066);
	write_cmos_sensor_twobyte(0x602A, 0x0C1E);
	write_cmos_sensor_twobyte(0x6F12, 0x0076);
	write_cmos_sensor_twobyte(0x602A, 0x0C22);
	write_cmos_sensor_twobyte(0x6F12, 0x007C);
	write_cmos_sensor_twobyte(0x602A, 0x0C26);
	write_cmos_sensor_twobyte(0x6F12, 0x008C);
	write_cmos_sensor_twobyte(0x602A, 0x0C2A);
	write_cmos_sensor_twobyte(0x6F12, 0x0092);
	write_cmos_sensor_twobyte(0x602A, 0x0C2E);
	write_cmos_sensor_twobyte(0x6F12, 0x00A2);
	write_cmos_sensor_twobyte(0x602A, 0x0C32);
	write_cmos_sensor_twobyte(0x6F12, 0x00A8);
	write_cmos_sensor_twobyte(0x602A, 0x0C36);
	write_cmos_sensor_twobyte(0x6F12, 0x00B8);
	write_cmos_sensor_twobyte(0x602A, 0x0C3A);
	write_cmos_sensor_twobyte(0x6F12, 0x00BE);
	write_cmos_sensor_twobyte(0x602A, 0x0C3E);
	write_cmos_sensor_twobyte(0x6F12, 0x00CE);
	write_cmos_sensor_twobyte(0x602A, 0x0C42);
	write_cmos_sensor_twobyte(0x6F12, 0x00D4);
	write_cmos_sensor_twobyte(0x602A, 0x0C46);
	write_cmos_sensor_twobyte(0x6F12, 0x00E4);
	write_cmos_sensor_twobyte(0x602A, 0x0C4A);
	write_cmos_sensor_twobyte(0x6F12, 0x00EA);
	write_cmos_sensor_twobyte(0x602A, 0x0C4E);
	write_cmos_sensor_twobyte(0x6F12, 0x0015);
	write_cmos_sensor_twobyte(0x602A, 0x0C52);
	write_cmos_sensor_twobyte(0x6F12, 0x002B);
	write_cmos_sensor_twobyte(0x602A, 0x0C56);
	write_cmos_sensor_twobyte(0x6F12, 0x0041);
	write_cmos_sensor_twobyte(0x602A, 0x0C5A);
	write_cmos_sensor_twobyte(0x6F12, 0x0057);
	write_cmos_sensor_twobyte(0x602A, 0x0C5E);
	write_cmos_sensor_twobyte(0x6F12, 0x006D);
	write_cmos_sensor_twobyte(0x602A, 0x0C62);
	write_cmos_sensor_twobyte(0x6F12, 0x0083);
	write_cmos_sensor_twobyte(0x602A, 0x0C66);
	write_cmos_sensor_twobyte(0x6F12, 0x0099);
	write_cmos_sensor_twobyte(0x602A, 0x0C6A);
	write_cmos_sensor_twobyte(0x6F12, 0x00AF);
	write_cmos_sensor_twobyte(0x602A, 0x0C6E);
	write_cmos_sensor_twobyte(0x6F12, 0x00C5);
	write_cmos_sensor_twobyte(0x602A, 0x0C72);
	write_cmos_sensor_twobyte(0x6F12, 0x00DB);
	write_cmos_sensor_twobyte(0x602A, 0x0C76);
	write_cmos_sensor_twobyte(0x6F12, 0x00F1);
	write_cmos_sensor_twobyte(0x602A, 0x0C7A);
	write_cmos_sensor_twobyte(0x6F12, 0x0329);
	write_cmos_sensor_twobyte(0x602A, 0x0C7E);
	write_cmos_sensor_twobyte(0x6F12, 0x0328);
	write_cmos_sensor_twobyte(0x602A, 0x0C82);
	write_cmos_sensor_twobyte(0x6F12, 0x064E);
	write_cmos_sensor_twobyte(0x602A, 0x0C86);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0C8A);
	write_cmos_sensor_twobyte(0x6F12, 0x03BE);
	write_cmos_sensor_twobyte(0x602A, 0x0C8E);
	write_cmos_sensor_twobyte(0x6F12, 0x05EF);
	write_cmos_sensor_twobyte(0x602A, 0x0C92);
	write_cmos_sensor_twobyte(0x6F12, 0x0025);
	write_cmos_sensor_twobyte(0x602A, 0x0C96);
	write_cmos_sensor_twobyte(0x6F12, 0x0329);
	write_cmos_sensor_twobyte(0x602A, 0x0C9A);
	write_cmos_sensor_twobyte(0x6F12, 0x0328);
	write_cmos_sensor_twobyte(0x602A, 0x0C9E);
	write_cmos_sensor_twobyte(0x6F12, 0x064E);
	write_cmos_sensor_twobyte(0x602A, 0x0CA2);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0CA6);
	write_cmos_sensor_twobyte(0x6F12, 0x03BE);
	write_cmos_sensor_twobyte(0x602A, 0x0CAA);
	write_cmos_sensor_twobyte(0x6F12, 0x05EF);
	write_cmos_sensor_twobyte(0x602A, 0x0CAE);
	write_cmos_sensor_twobyte(0x6F12, 0x0025);
	write_cmos_sensor_twobyte(0x602A, 0x0CB2);
	write_cmos_sensor_twobyte(0x6F12, 0x0329);
	write_cmos_sensor_twobyte(0x602A, 0x0CB6);
	write_cmos_sensor_twobyte(0x6F12, 0x0328);
	write_cmos_sensor_twobyte(0x602A, 0x0CBA);
	write_cmos_sensor_twobyte(0x6F12, 0x064E);
	write_cmos_sensor_twobyte(0x602A, 0x0CBE);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0CC2);
	write_cmos_sensor_twobyte(0x6F12, 0x03BE);
	write_cmos_sensor_twobyte(0x602A, 0x0CC6);
	write_cmos_sensor_twobyte(0x6F12, 0x05EF);
	write_cmos_sensor_twobyte(0x602A, 0x0CCA);
	write_cmos_sensor_twobyte(0x6F12, 0x0025);
	write_cmos_sensor_twobyte(0x602A, 0x0CCE);
	write_cmos_sensor_twobyte(0x6F12, 0x0329);
	write_cmos_sensor_twobyte(0x602A, 0x0CD2);
	write_cmos_sensor_twobyte(0x6F12, 0x0328);
	write_cmos_sensor_twobyte(0x602A, 0x0CD6);
	write_cmos_sensor_twobyte(0x6F12, 0x064E);
	write_cmos_sensor_twobyte(0x602A, 0x0CDA);
	write_cmos_sensor_twobyte(0x6F12, 0x0000);
	write_cmos_sensor_twobyte(0x602A, 0x0CDE);
	write_cmos_sensor_twobyte(0x6F12, 0x03BE);
	write_cmos_sensor_twobyte(0x602A, 0x0CE2);
	write_cmos_sensor_twobyte(0x6F12, 0x05EF);
	write_cmos_sensor_twobyte(0x602A, 0x0CE6);
	write_cmos_sensor_twobyte(0x6F12, 0x0025);
	write_cmos_sensor_twobyte(0x602A, 0x0E90);
	write_cmos_sensor_twobyte(0x6F12, 0x0004);
	write_cmos_sensor_twobyte(0x602A, 0x0E8E);
	write_cmos_sensor_twobyte(0x6F12, 0x0004);
	write_cmos_sensor_twobyte(0x602A, 0x0E94);
	write_cmos_sensor_twobyte(0x6F12, 0x000D);
	write_cmos_sensor_twobyte(0x6F12, 0x000D);
	write_cmos_sensor_twobyte(0x6F12, 0x000D);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0xF480, 0x0010);
	write_cmos_sensor_twobyte(0xF4D0, 0x0020);
	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x150E);
	write_cmos_sensor_twobyte(0x6F12, 0x0610);
	write_cmos_sensor_twobyte(0x6F12, 0x0610);
	write_cmos_sensor_twobyte(0x602A, 0x3E32);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6F12, 0x01FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1070);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x03FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1070);
	write_cmos_sensor_twobyte(0x6F12, 0x1080);
	write_cmos_sensor_twobyte(0x6F12, 0x0400);
	write_cmos_sensor_twobyte(0x6F12, 0x07FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1080);
	write_cmos_sensor_twobyte(0x6F12, 0x1090);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x0FFF);
	write_cmos_sensor_twobyte(0x6F12, 0x1090);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x602A, 0x3E72);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6F12, 0x01FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x03FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x0400);
	write_cmos_sensor_twobyte(0x6F12, 0x07FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x0FFF);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x602A, 0x3EB2);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6F12, 0x01FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x03FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x0400);
	write_cmos_sensor_twobyte(0x6F12, 0x07FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x0FFF);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x602A, 0x3EF2);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6F12, 0x01FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1070);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x03FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1070);
	write_cmos_sensor_twobyte(0x6F12, 0x1080);
	write_cmos_sensor_twobyte(0x6F12, 0x0400);
	write_cmos_sensor_twobyte(0x6F12, 0x07FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1080);
	write_cmos_sensor_twobyte(0x6F12, 0x1090);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x0FFF);
	write_cmos_sensor_twobyte(0x6F12, 0x1090);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x602A, 0x3F32);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6F12, 0x01FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1070);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x03FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1070);
	write_cmos_sensor_twobyte(0x6F12, 0x1080);
	write_cmos_sensor_twobyte(0x6F12, 0x0400);
	write_cmos_sensor_twobyte(0x6F12, 0x07FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1080);
	write_cmos_sensor_twobyte(0x6F12, 0x1090);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x0FFF);
	write_cmos_sensor_twobyte(0x6F12, 0x1090);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x602A, 0x3F72);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6F12, 0x01FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x03FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x0400);
	write_cmos_sensor_twobyte(0x6F12, 0x07FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x0FFF);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x602A, 0x3FB2);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6F12, 0x01FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x03FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x0400);
	write_cmos_sensor_twobyte(0x6F12, 0x07FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1040);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x0FFF);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x6F12, 0x1020);
	write_cmos_sensor_twobyte(0x602A, 0x3FF2);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x6F12, 0x01FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1070);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x6F12, 0x03FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1070);
	write_cmos_sensor_twobyte(0x6F12, 0x1080);
	write_cmos_sensor_twobyte(0x6F12, 0x0400);
	write_cmos_sensor_twobyte(0x6F12, 0x07FF);
	write_cmos_sensor_twobyte(0x6F12, 0x1080);
	write_cmos_sensor_twobyte(0x6F12, 0x1090);
	write_cmos_sensor_twobyte(0x6F12, 0x0800);
	write_cmos_sensor_twobyte(0x6F12, 0x0FFF);
	write_cmos_sensor_twobyte(0x6F12, 0x1090);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x6F12, 0x10A0);
	write_cmos_sensor_twobyte(0x602A, 0x0E88);
	write_cmos_sensor_twobyte(0x6F12, 0x3EF8);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0xF4AA, 0x0048);
#endif
}

static void preview_setting_11_new(void)
{
	LOG_INF("Dual PD preview");

	// Stream Off
	write_cmos_sensor(0x0100,0x00);


	//no bin
#ifdef SENSOR_M1
	#ifdef NEW_2L7_SETTING // 30fps setting for 2L7
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6010);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	mDELAY(3);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X7970);
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X7150);
	write_cmos_sensor_twobyte(0X602A, 0X0136);
	write_cmos_sensor_twobyte(0X6F12, 0X1800);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X7D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0449);
	write_cmos_sensor_twobyte(0X6F12, 0X0348);
	write_cmos_sensor_twobyte(0X6F12, 0X044A);
	write_cmos_sensor_twobyte(0X6F12, 0X4860);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0881);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC2BB);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X87AC);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5830);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XA3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0168);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X57FC);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X59FC);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0XC28E);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB1);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0X0280);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X48BC);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0XF148);
	write_cmos_sensor_twobyte(0X6F12, 0X8A46);
	write_cmos_sensor_twobyte(0X6F12, 0X9146);
	write_cmos_sensor_twobyte(0X6F12, 0X4168);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X39FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0027);
	write_cmos_sensor_twobyte(0X6F12, 0XA780);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X5146);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFC);
	write_cmos_sensor_twobyte(0X6F12, 0X98F8);
	write_cmos_sensor_twobyte(0X6F12, 0XE200);
	write_cmos_sensor_twobyte(0X6F12, 0XE64A);
	write_cmos_sensor_twobyte(0X6F12, 0XE749);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD9);
	write_cmos_sensor_twobyte(0X6F12, 0XE648);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6C09);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF087);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XF8E7);
	write_cmos_sensor_twobyte(0X6F12, 0XFEB5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XD848);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X8168);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X17FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X03FC);
	write_cmos_sensor_twobyte(0X6F12, 0XD048);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X0029);
	write_cmos_sensor_twobyte(0X6F12, 0X1CD0);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XD900);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0XCB48);
	write_cmos_sensor_twobyte(0X6F12, 0X0292);
	write_cmos_sensor_twobyte(0X6F12, 0X0192);
	write_cmos_sensor_twobyte(0X6F12, 0X4A30);
	write_cmos_sensor_twobyte(0X6F12, 0XCA00);
	write_cmos_sensor_twobyte(0X6F12, 0XC17D);
	write_cmos_sensor_twobyte(0X6F12, 0XCD4C);
	write_cmos_sensor_twobyte(0X6F12, 0XC91D);
	write_cmos_sensor_twobyte(0X6F12, 0XC908);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0123);
	write_cmos_sensor_twobyte(0X6F12, 0X43EA);
	write_cmos_sensor_twobyte(0X6F12, 0X4101);
	write_cmos_sensor_twobyte(0X6F12, 0X6180);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X6B46);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF2);
	write_cmos_sensor_twobyte(0X6F12, 0X1010);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF7FB);
	write_cmos_sensor_twobyte(0X6F12, 0X0C20);
	write_cmos_sensor_twobyte(0X6F12, 0X2080);
	write_cmos_sensor_twobyte(0X6F12, 0XFEBD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC068);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD3FB);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE8FB);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0XC18A);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0268);
	write_cmos_sensor_twobyte(0X6F12, 0X02F5);
	write_cmos_sensor_twobyte(0X6F12, 0XC972);
	write_cmos_sensor_twobyte(0X6F12, 0X907A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0XBB49);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0XB4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF600);
	write_cmos_sensor_twobyte(0X6F12, 0X917A);
	write_cmos_sensor_twobyte(0X6F12, 0X411A);
	write_cmos_sensor_twobyte(0X6F12, 0XB948);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X34F8);
	write_cmos_sensor_twobyte(0X6F12, 0XFA1F);
	write_cmos_sensor_twobyte(0X6F12, 0X638A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X937A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X8180);
	write_cmos_sensor_twobyte(0X6F12, 0X1078);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD1);
	write_cmos_sensor_twobyte(0X6F12, 0XAE48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1115);
	write_cmos_sensor_twobyte(0X6F12, 0XB248);
	write_cmos_sensor_twobyte(0X6F12, 0X19B1);
	write_cmos_sensor_twobyte(0X6F12, 0XA749);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5D10);
	write_cmos_sensor_twobyte(0X6F12, 0X491C);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCC10);
	write_cmos_sensor_twobyte(0X6F12, 0XA348);
	write_cmos_sensor_twobyte(0X6F12, 0XB830);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X4188);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X9FBB);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0X9D48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0169);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XAFFB);
	write_cmos_sensor_twobyte(0X6F12, 0X9C48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1202);
	write_cmos_sensor_twobyte(0X6F12, 0X40B1);
	write_cmos_sensor_twobyte(0X6F12, 0X9D49);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X0968);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X7DBB);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4F);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0X8B48);
	write_cmos_sensor_twobyte(0X6F12, 0X81B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X4069);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X80B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0091);
	write_cmos_sensor_twobyte(0X6F12, 0X8346);
	write_cmos_sensor_twobyte(0X6F12, 0X0146);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6EFB);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0X1492);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF0);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X09F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3409);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X28A0);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X24A0);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X50A0);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0600);
	write_cmos_sensor_twobyte(0X6F12, 0X0028);
	write_cmos_sensor_twobyte(0X6F12, 0X76D0);
	write_cmos_sensor_twobyte(0X6F12, 0X8948);
	write_cmos_sensor_twobyte(0X6F12, 0X0026);
	write_cmos_sensor_twobyte(0X6F12, 0X3746);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D02);
	write_cmos_sensor_twobyte(0X6F12, 0XD8B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0024);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8405);
	write_cmos_sensor_twobyte(0X6F12, 0X296E);
	write_cmos_sensor_twobyte(0X6F12, 0X81B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0A22);
	write_cmos_sensor_twobyte(0X6F12, 0X286A);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X72FB);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4011);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XC021);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF001);
	write_cmos_sensor_twobyte(0X6F12, 0X5143);
	write_cmos_sensor_twobyte(0X6F12, 0X4031);
	write_cmos_sensor_twobyte(0X6F12, 0X00EB);
	write_cmos_sensor_twobyte(0X6F12, 0XE110);
	write_cmos_sensor_twobyte(0X6F12, 0X0644);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X641C);
	write_cmos_sensor_twobyte(0X6F12, 0X0744);
	write_cmos_sensor_twobyte(0X6F12, 0X102C);
	write_cmos_sensor_twobyte(0X6F12, 0XE4D3);
	write_cmos_sensor_twobyte(0X6F12, 0XF817);
	write_cmos_sensor_twobyte(0X6F12, 0X07EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0511);
	write_cmos_sensor_twobyte(0X6F12, 0XF017);
	write_cmos_sensor_twobyte(0X6F12, 0X06EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0411);
	write_cmos_sensor_twobyte(0X6F12, 0X99F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0700);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X0428);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X654E);
	write_cmos_sensor_twobyte(0X6F12, 0XD071);
	write_cmos_sensor_twobyte(0X6F12, 0X8236);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X2ADD);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X08D0);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X01EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X8B6A);
	write_cmos_sensor_twobyte(0X6F12, 0XCB62);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C00);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0XC462);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X92F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0730);
	write_cmos_sensor_twobyte(0X6F12, 0X8857);
	write_cmos_sensor_twobyte(0X6F12, 0X9842);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X8855);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C30);
	write_cmos_sensor_twobyte(0X6F12, 0X9957);
	write_cmos_sensor_twobyte(0X6F12, 0X0129);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDD);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0313);
	write_cmos_sensor_twobyte(0X6F12, 0X04E0);
	write_cmos_sensor_twobyte(0X6F12, 0X03EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8007);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XFF6A);
	write_cmos_sensor_twobyte(0X6F12, 0X3C44);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D8);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0XF1F4);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0800);
	write_cmos_sensor_twobyte(0X6F12, 0X50B3);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X14DB);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X10DB);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X1BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FB);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X21DB);
	write_cmos_sensor_twobyte(0X6F12, 0X22E0);
	write_cmos_sensor_twobyte(0X6F12, 0X34E0);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X11DC);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XFEFA);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0XAC42);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X6F12, 0X601B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X281B);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X18B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C20);
	write_cmos_sensor_twobyte(0X6F12, 0X02F8);
	write_cmos_sensor_twobyte(0X6F12, 0X06A0);
	write_cmos_sensor_twobyte(0X6F12, 0X0299);
	write_cmos_sensor_twobyte(0X6F12, 0X8968);
	write_cmos_sensor_twobyte(0X6F12, 0X4A78);
	write_cmos_sensor_twobyte(0X6F12, 0X0243);
	write_cmos_sensor_twobyte(0X6F12, 0X4A70);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0XC1E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0954);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5000);
	write_cmos_sensor_twobyte(0X6F12, 0X6C40);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5440);
	write_cmos_sensor_twobyte(0X6F12, 0X02AC);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X0ECC);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDEFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X05B0);
	write_cmos_sensor_twobyte(0X6F12, 0X5946);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF04F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA4BA);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X8246);
	write_cmos_sensor_twobyte(0X6F12, 0X1F48);
	write_cmos_sensor_twobyte(0X6F12, 0X8946);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X8169);
	write_cmos_sensor_twobyte(0X6F12, 0X1E46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0A9C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FA);
	write_cmos_sensor_twobyte(0X6F12, 0X1848);
	write_cmos_sensor_twobyte(0X6F12, 0X3346);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C40);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0X5046);
	write_cmos_sensor_twobyte(0X6F12, 0X0094);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC0FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X82BA);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0023);
	write_cmos_sensor_twobyte(0X6F12, 0X03E0);
	write_cmos_sensor_twobyte(0X6F12, 0X51F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2340);
	write_cmos_sensor_twobyte(0X6F12, 0X6040);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1C);
	write_cmos_sensor_twobyte(0X6F12, 0XB3EB);
	write_cmos_sensor_twobyte(0X6F12, 0X920F);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D3);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0948);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC069);
	write_cmos_sensor_twobyte(0X6F12, 0X87B2);
	write_cmos_sensor_twobyte(0X6F12, 0X4FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X1048);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X034C);
	write_cmos_sensor_twobyte(0X6F12, 0X3068);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A11);
	write_cmos_sensor_twobyte(0X6F12, 0XF1B3);
	write_cmos_sensor_twobyte(0X6F12, 0X19E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF4D2);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X8000);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X9000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF110);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5810);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X6AF0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XE304);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X7000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4B);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X51D1);
	write_cmos_sensor_twobyte(0X6F12, 0XFD48);
	write_cmos_sensor_twobyte(0X6F12, 0XFD49);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0XF0C3);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2FF);
	write_cmos_sensor_twobyte(0X6F12, 0XE066);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0XF949);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XABFF);
	write_cmos_sensor_twobyte(0X6F12, 0X2067);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XF749);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XA5FF);
	write_cmos_sensor_twobyte(0X6F12, 0X6067);
	write_cmos_sensor_twobyte(0X6F12, 0X3168);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XE36D);
	write_cmos_sensor_twobyte(0X6F12, 0XE26E);
	write_cmos_sensor_twobyte(0X6F12, 0X0126);
	write_cmos_sensor_twobyte(0X6F12, 0X9A42);
	write_cmos_sensor_twobyte(0X6F12, 0X01D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X60C0);
	write_cmos_sensor_twobyte(0X6F12, 0X236F);
	write_cmos_sensor_twobyte(0X6F12, 0X6345);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X020C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X64C0);
	write_cmos_sensor_twobyte(0X6F12, 0X6045);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X040C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XBCF1);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X07D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X5A40);
	write_cmos_sensor_twobyte(0X6F12, 0X4240);
	write_cmos_sensor_twobyte(0X6F12, 0XA266);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A51);
	write_cmos_sensor_twobyte(0X6F12, 0X0AE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B61);
	write_cmos_sensor_twobyte(0X6F12, 0XF6E7);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XA566);
	write_cmos_sensor_twobyte(0X6F12, 0XE566);
	write_cmos_sensor_twobyte(0X6F12, 0X2567);
	write_cmos_sensor_twobyte(0X6F12, 0X6567);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X27FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0XD348);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FA);
	write_cmos_sensor_twobyte(0X6F12, 0XCF48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A01);
	write_cmos_sensor_twobyte(0X6F12, 0X48B3);
	write_cmos_sensor_twobyte(0X6F12, 0XC348);
	write_cmos_sensor_twobyte(0X6F12, 0XC44B);
	write_cmos_sensor_twobyte(0X6F12, 0XC44A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X20D1);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0DFA);
	write_cmos_sensor_twobyte(0X6F12, 0XC148);
	write_cmos_sensor_twobyte(0X6F12, 0XC149);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XC04F);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFF);
	write_cmos_sensor_twobyte(0X6F12, 0XC14C);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0XBD49);
	write_cmos_sensor_twobyte(0X6F12, 0XE065);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X32FF);
	write_cmos_sensor_twobyte(0X6F12, 0X2066);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XBA49);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFF);
	write_cmos_sensor_twobyte(0X6F12, 0X6066);
	write_cmos_sensor_twobyte(0X6F12, 0XD4E9);
	write_cmos_sensor_twobyte(0X6F12, 0X1712);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X4140);
	write_cmos_sensor_twobyte(0X6F12, 0XA165);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA1B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XB148);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X406A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDBF9);
	write_cmos_sensor_twobyte(0X6F12, 0XE068);
	write_cmos_sensor_twobyte(0X6F12, 0X07F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8047);
	write_cmos_sensor_twobyte(0X6F12, 0X8069);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0XC288);
	write_cmos_sensor_twobyte(0X6F12, 0X0389);
	write_cmos_sensor_twobyte(0X6F12, 0XA1EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5201);
	write_cmos_sensor_twobyte(0X6F12, 0X827B);
	write_cmos_sensor_twobyte(0X6F12, 0X891A);
	write_cmos_sensor_twobyte(0X6F12, 0X8289);
	write_cmos_sensor_twobyte(0X6F12, 0XC07B);
	write_cmos_sensor_twobyte(0X6F12, 0XA2EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5302);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0XB980);
	write_cmos_sensor_twobyte(0X6F12, 0XE268);
	write_cmos_sensor_twobyte(0X6F12, 0X491E);
	write_cmos_sensor_twobyte(0X6F12, 0X9269);
	write_cmos_sensor_twobyte(0X6F12, 0X128A);
	write_cmos_sensor_twobyte(0X6F12, 0X1144);
	write_cmos_sensor_twobyte(0X6F12, 0XF980);
	write_cmos_sensor_twobyte(0X6F12, 0X3881);
	write_cmos_sensor_twobyte(0X6F12, 0XE168);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X8969);
	write_cmos_sensor_twobyte(0X6F12, 0X498A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X7881);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BB9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8146);
	write_cmos_sensor_twobyte(0X6F12, 0X9648);
	write_cmos_sensor_twobyte(0X6F12, 0X0D46);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X816A);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0E0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X5CF9);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X4846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA6F9);
	write_cmos_sensor_twobyte(0X6F12, 0X8E48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9201);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X15D1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2900);
	write_cmos_sensor_twobyte(0X6F12, 0X70B1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCE02);
	write_cmos_sensor_twobyte(0X6F12, 0X08B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6F12, 0X07E0);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XDA00);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X8748);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5C00);
	write_cmos_sensor_twobyte(0X6F12, 0X10B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4210);
	write_cmos_sensor_twobyte(0X6F12, 0X88F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1110);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X33B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1001);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X83F9);
	write_cmos_sensor_twobyte(0X6F12, 0X774D);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XF006);
	write_cmos_sensor_twobyte(0X6F12, 0X05F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3405);
	write_cmos_sensor_twobyte(0X6F12, 0X1ED5);
	write_cmos_sensor_twobyte(0X6F12, 0X7748);
	write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XB207);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X03D2);
	write_cmos_sensor_twobyte(0X6F12, 0X2146);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X76F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X1346);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0X39B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0F88);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7870);
	write_cmos_sensor_twobyte(0X6F12, 0XC08A);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0X01E0);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7830);
	write_cmos_sensor_twobyte(0X6F12, 0X521C);
	write_cmos_sensor_twobyte(0X6F12, 0X062A);
	write_cmos_sensor_twobyte(0X6F12, 0XEED3);
	write_cmos_sensor_twobyte(0X6F12, 0X7004);
	write_cmos_sensor_twobyte(0X6F12, 0X1AD5);
	write_cmos_sensor_twobyte(0X6F12, 0XB5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8400);
	write_cmos_sensor_twobyte(0X6F12, 0X411E);
	write_cmos_sensor_twobyte(0X6F12, 0XA142);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X04D2);
	write_cmos_sensor_twobyte(0X6F12, 0X44F4);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X54F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4002);
	write_cmos_sensor_twobyte(0X6F12, 0X5189);
	write_cmos_sensor_twobyte(0X6F12, 0X29B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7820);
	write_cmos_sensor_twobyte(0X6F12, 0X12B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0A80);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X0628);
	write_cmos_sensor_twobyte(0X6F12, 0XF2D3);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF081);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X5448);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC16A);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDAF8);
	write_cmos_sensor_twobyte(0X6F12, 0X43F6);
	write_cmos_sensor_twobyte(0X6F12, 0XEF73);
	write_cmos_sensor_twobyte(0X6F12, 0X0322);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XB901);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X36F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X38F9);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC8B8);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X484C);
	write_cmos_sensor_twobyte(0X6F12, 0XE38B);
	write_cmos_sensor_twobyte(0X6F12, 0XCB1A);
	write_cmos_sensor_twobyte(0X6F12, 0X218C);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X0144);
	write_cmos_sensor_twobyte(0X6F12, 0X9942);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X0346);
	write_cmos_sensor_twobyte(0X6F12, 0X4149);
	write_cmos_sensor_twobyte(0X6F12, 0X3431);
	write_cmos_sensor_twobyte(0X6F12, 0X9040);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8630);
	write_cmos_sensor_twobyte(0X6F12, 0X03FA);
	write_cmos_sensor_twobyte(0X6F12, 0X02F3);
	write_cmos_sensor_twobyte(0X6F12, 0X414A);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XD181);
	write_cmos_sensor_twobyte(0X6F12, 0X1382);
	write_cmos_sensor_twobyte(0X6F12, 0X03D0);
	write_cmos_sensor_twobyte(0X6F12, 0X5181);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X9381);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF70);
	write_cmos_sensor_twobyte(0X6F12, 0X5081);
	write_cmos_sensor_twobyte(0X6F12, 0X9081);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7B71);
	write_cmos_sensor_twobyte(0X6F12, 0X3948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0FF9);
	write_cmos_sensor_twobyte(0X6F12, 0X334C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5371);
	write_cmos_sensor_twobyte(0X6F12, 0X2060);
	write_cmos_sensor_twobyte(0X6F12, 0X3648);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X07F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XFF61);
	write_cmos_sensor_twobyte(0X6F12, 0X6060);
	write_cmos_sensor_twobyte(0X6F12, 0X3448);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X00F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XA060);
	write_cmos_sensor_twobyte(0X6F12, 0X3148);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2D61);
	write_cmos_sensor_twobyte(0X6F12, 0XE060);
	write_cmos_sensor_twobyte(0X6F12, 0X2F48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XF551);
	write_cmos_sensor_twobyte(0X6F12, 0X2061);
	write_cmos_sensor_twobyte(0X6F12, 0X2C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XEBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5341);
	write_cmos_sensor_twobyte(0X6F12, 0X6061);
	write_cmos_sensor_twobyte(0X6F12, 0X2A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XA061);
	write_cmos_sensor_twobyte(0X6F12, 0X2021);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X6000);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8006);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3400);
	write_cmos_sensor_twobyte(0X6F12, 0X7580);
	write_cmos_sensor_twobyte(0X6F12, 0X4562);
	write_cmos_sensor_twobyte(0X6F12, 0X8562);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C50);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5050);
	write_cmos_sensor_twobyte(0X6F12, 0X2A46);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7D21);
	write_cmos_sensor_twobyte(0X6F12, 0X4565);
	write_cmos_sensor_twobyte(0X6F12, 0X1E48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XCBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2121);
	write_cmos_sensor_twobyte(0X6F12, 0X6062);
	write_cmos_sensor_twobyte(0X6F12, 0X1C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X4541);
	write_cmos_sensor_twobyte(0X6F12, 0XA062);
	write_cmos_sensor_twobyte(0X6F12, 0X1948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XBDF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F31);
	write_cmos_sensor_twobyte(0X6F12, 0XE061);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0XABBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0345);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0X1234);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5423);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0XFECA);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X7C80);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0B27);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0E5D);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X5C51);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X1F65);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X98A9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X852F);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XDE2F);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X10B3);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0823);
	write_cmos_sensor_twobyte(0X6F12, 0X0A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X87F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X2062);
	write_cmos_sensor_twobyte(0X6F12, 0X0848);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X6F11);
	write_cmos_sensor_twobyte(0X6F12, 0XE062);
	write_cmos_sensor_twobyte(0X6F12, 0X0548);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X79F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2063);
	write_cmos_sensor_twobyte(0X6F12, 0XF562);
	write_cmos_sensor_twobyte(0X6F12, 0X3563);
	write_cmos_sensor_twobyte(0X6F12, 0X7563);
	write_cmos_sensor_twobyte(0X6F12, 0X70BD);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X339B);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X2ABB);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X135F);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0XE91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X273C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X5D6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X033C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X3B2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X45F6);
	write_cmos_sensor_twobyte(0X6F12, 0X514C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X657C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D0C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X471C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X49F6);
	write_cmos_sensor_twobyte(0X6F12, 0XA90C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X48F2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X230C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X43F2);
	write_cmos_sensor_twobyte(0X6F12, 0X9B3C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3B7C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X4DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X2F6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB30C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X871C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X6F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1B5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0XF91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X42F6);
	write_cmos_sensor_twobyte(0X6F12, 0XBB2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB71C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X602A, 0X1150);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF466);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X111C);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X1120);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X602A, 0X113E);
	write_cmos_sensor_twobyte(0X6F12, 0X2101);
	write_cmos_sensor_twobyte(0X602A, 0X1106);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A28);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A52);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A48);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A34);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A84);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
	write_cmos_sensor_twobyte(0X602A, 0X3AB6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
	write_cmos_sensor_twobyte(0X602A, 0X3B44);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3B48);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X3B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0200);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0224);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X1124);
	write_cmos_sensor_twobyte(0X6F12, 0X0328);
	write_cmos_sensor_twobyte(0X602A, 0X0ACE);
	write_cmos_sensor_twobyte(0X6F12, 0X0030);
	write_cmos_sensor_twobyte(0X602A, 0X09C0);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X09C2);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X6F12, 0X008C);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0XF408);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X54A2);
	write_cmos_sensor_twobyte(0X6F12, 0XF418);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X000D);
	write_cmos_sensor_twobyte(0X6F12, 0XF472);
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF482);
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF484);
	write_cmos_sensor_twobyte(0X602A, 0X47F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FE);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);
	write_cmos_sensor_twobyte(0X602A, 0X1108);
	write_cmos_sensor_twobyte(0X6F12, 0X054C);
	write_cmos_sensor_twobyte(0X6F12, 0X0183);
	write_cmos_sensor_twobyte(0X6F12, 0X07FF);
	write_cmos_sensor_twobyte(0X602A, 0X1154);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X162F);
	write_cmos_sensor_twobyte(0X6F12, 0X4328);
	write_cmos_sensor_twobyte(0X602A, 0X1126);
	write_cmos_sensor_twobyte(0X6F12, 0X00A2);
	write_cmos_sensor_twobyte(0X602A, 0X114A);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);
	write_cmos_sensor_twobyte(0X602A, 0X114E);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF41E);
	write_cmos_sensor_twobyte(0X6F12, 0X2100);
	write_cmos_sensor_twobyte(0X602A, 0XF488);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X112A);
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
	write_cmos_sensor_twobyte(0X602A, 0X1128);
	write_cmos_sensor_twobyte(0X6F12, 0X001B);
	write_cmos_sensor_twobyte(0X602A, 0X10F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);
	write_cmos_sensor_twobyte(0X602A, 0X10F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);
	write_cmos_sensor_twobyte(0X602A, 0X10FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);
	write_cmos_sensor_twobyte(0X602A, 0X10FE);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF414);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X10C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X602A, 0X10B6);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X0B26);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);
	write_cmos_sensor_twobyte(0X602A, 0X0B2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X602A, 0X0B36);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);
	write_cmos_sensor_twobyte(0X6F12, 0X00B7);
	write_cmos_sensor_twobyte(0X6F12, 0X013E);
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);
	write_cmos_sensor_twobyte(0X6F12, 0X0140);
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);
	write_cmos_sensor_twobyte(0X6F12, 0X00EC);
	write_cmos_sensor_twobyte(0X6F12, 0X00A6);
	write_cmos_sensor_twobyte(0X602A, 0X0B76);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);
	write_cmos_sensor_twobyte(0X602A, 0X0B7E);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);
	write_cmos_sensor_twobyte(0X6F12, 0X009F);
	write_cmos_sensor_twobyte(0X602A, 0X0B86);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);
	write_cmos_sensor_twobyte(0X602A, 0X0B8E);
	write_cmos_sensor_twobyte(0X6F12, 0X0047);
	write_cmos_sensor_twobyte(0X6F12, 0X0048);
	write_cmos_sensor_twobyte(0X602A, 0X0B94);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);
	write_cmos_sensor_twobyte(0X6F12, 0X009F);
	write_cmos_sensor_twobyte(0X602A, 0X0B9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);
	write_cmos_sensor_twobyte(0X602A, 0X0BAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0156);
	write_cmos_sensor_twobyte(0X6F12, 0X00FF);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0057);
	write_cmos_sensor_twobyte(0X6F12, 0X0057);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X004E);
	write_cmos_sensor_twobyte(0X6F12, 0X004E);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0055);
	write_cmos_sensor_twobyte(0X6F12, 0X0055);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X0016);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X013E);
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);
	write_cmos_sensor_twobyte(0X6F12, 0X014E);
	write_cmos_sensor_twobyte(0X6F12, 0X00FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X006B);
	write_cmos_sensor_twobyte(0X6F12, 0X006B);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X602A, 0X0C0A);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X6F12, 0X00D7);
	write_cmos_sensor_twobyte(0X6F12, 0X0087);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);
	write_cmos_sensor_twobyte(0X6F12, 0X01D6);
	write_cmos_sensor_twobyte(0X6F12, 0X0134);
	write_cmos_sensor_twobyte(0X6F12, 0X026A);
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0119);
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X00C2);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);
	write_cmos_sensor_twobyte(0X602A, 0X0C62);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);
	write_cmos_sensor_twobyte(0X6F12, 0X00C0);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X602A, 0X0C72);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);
	write_cmos_sensor_twobyte(0X6F12, 0X0085);
	write_cmos_sensor_twobyte(0X6F12, 0X0104);
	write_cmos_sensor_twobyte(0X6F12, 0X00B5);
	write_cmos_sensor_twobyte(0X6F12, 0X01D4);
	write_cmos_sensor_twobyte(0X6F12, 0X0132);
	write_cmos_sensor_twobyte(0X6F12, 0X026C);
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);
	write_cmos_sensor_twobyte(0X602A, 0X0C92);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X6F12, 0X001C);
	write_cmos_sensor_twobyte(0X6F12, 0X0016);
	write_cmos_sensor_twobyte(0X6F12, 0X001E);
	write_cmos_sensor_twobyte(0X6F12, 0X0017);
	write_cmos_sensor_twobyte(0X6F12, 0X0038);
	write_cmos_sensor_twobyte(0X6F12, 0X002B);
	write_cmos_sensor_twobyte(0X6F12, 0X003A);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X0054);
	write_cmos_sensor_twobyte(0X6F12, 0X0040);
	write_cmos_sensor_twobyte(0X6F12, 0X0076);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X0090);
	write_cmos_sensor_twobyte(0X6F12, 0X0055);
	write_cmos_sensor_twobyte(0X6F12, 0X010B);
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0125);
	write_cmos_sensor_twobyte(0X6F12, 0X00C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0127);
	write_cmos_sensor_twobyte(0X6F12, 0X00C7);
	write_cmos_sensor_twobyte(0X6F12, 0X0141);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0143);
	write_cmos_sensor_twobyte(0X6F12, 0X00DC);
	write_cmos_sensor_twobyte(0X6F12, 0X015D);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X015F);
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);
	write_cmos_sensor_twobyte(0X6F12, 0X0179);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X6F12, 0X017B);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);
	write_cmos_sensor_twobyte(0X6F12, 0X0195);
	write_cmos_sensor_twobyte(0X6F12, 0X011A);
	write_cmos_sensor_twobyte(0X6F12, 0X0197);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);
	write_cmos_sensor_twobyte(0X6F12, 0X01B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01B4);
	write_cmos_sensor_twobyte(0X6F12, 0X01B3);
	write_cmos_sensor_twobyte(0X6F12, 0X01B5);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0D22);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X602A, 0X0D32);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);
	write_cmos_sensor_twobyte(0X6F12, 0X006F);
	write_cmos_sensor_twobyte(0X6F12, 0X006F);
	write_cmos_sensor_twobyte(0X6F12, 0X0270);
	write_cmos_sensor_twobyte(0X6F12, 0X01CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0071);
	write_cmos_sensor_twobyte(0X6F12, 0X0070);
	write_cmos_sensor_twobyte(0X602A, 0X0D4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0073);
	write_cmos_sensor_twobyte(0X6F12, 0X0072);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00D3);
	write_cmos_sensor_twobyte(0X6F12, 0X0125);
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X0069);
	write_cmos_sensor_twobyte(0X6F12, 0X0264);
	write_cmos_sensor_twobyte(0X6F12, 0X01C4);
	write_cmos_sensor_twobyte(0X602A, 0X0D74);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X6F12, 0X0014);
	write_cmos_sensor_twobyte(0X6F12, 0X0014);
	write_cmos_sensor_twobyte(0X6F12, 0X026C);
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);
	write_cmos_sensor_twobyte(0X602A, 0X0D96);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X00D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0132);
	write_cmos_sensor_twobyte(0X6F12, 0X00E3);
	write_cmos_sensor_twobyte(0X602A, 0X0DB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0119);
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);
	write_cmos_sensor_twobyte(0X602A, 0X0DD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00D1);
	write_cmos_sensor_twobyte(0X602A, 0X0DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);
	write_cmos_sensor_twobyte(0X6F12, 0X01CA);
	write_cmos_sensor_twobyte(0X602A, 0X0E16);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X602A, 0X0FA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0287);
	write_cmos_sensor_twobyte(0X6F12, 0X01CC);
	write_cmos_sensor_twobyte(0X6F12, 0X003C);
	write_cmos_sensor_twobyte(0X6F12, 0X003C);
	write_cmos_sensor_twobyte(0X602A, 0X0FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0124);
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);
	write_cmos_sensor_twobyte(0X6F12, 0X00DF);
	write_cmos_sensor_twobyte(0X6F12, 0X0099);
	write_cmos_sensor_twobyte(0X6F12, 0X0296);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X602A, 0X0FD8);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0FDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);
	write_cmos_sensor_twobyte(0X602A, 0X0FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0301);
	write_cmos_sensor_twobyte(0X602A, 0X0FEC);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0003);
	write_cmos_sensor_twobyte(0X602A, 0X101C);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X101E);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1020);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1022);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1024);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1026);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1028);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X602A, 0X104C);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);
	write_cmos_sensor_twobyte(0X602A, 0X106C);
	write_cmos_sensor_twobyte(0X6F12, 0X0607);
	write_cmos_sensor_twobyte(0X602A, 0X10AE);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X10B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X0B0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);
	write_cmos_sensor_twobyte(0X602A, 0X3064);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0B02);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X0ABC);
	write_cmos_sensor_twobyte(0X6F12, 0X0207);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X070C);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X602A, 0X0110);
	write_cmos_sensor_twobyte(0X6F12, 0X1002);
	write_cmos_sensor_twobyte(0X602A, 0X0114);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X602A, 0X0408);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0344);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X1F6F);
	write_cmos_sensor_twobyte(0X6F12, 0X0BC7);
	write_cmos_sensor_twobyte(0X6F12, 0X0FB0);
	write_cmos_sensor_twobyte(0X6F12, 0X05E0);
	write_cmos_sensor_twobyte(0X602A, 0X0380);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0003);
	write_cmos_sensor_twobyte(0X602A, 0X0900);
	write_cmos_sensor_twobyte(0X6F12, 0X0112);
	write_cmos_sensor_twobyte(0X602A, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0404);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);
	write_cmos_sensor_twobyte(0X602A, 0X0304);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X030C);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0306);
	write_cmos_sensor_twobyte(0X6F12, 0X01F4);
	write_cmos_sensor_twobyte(0X602A, 0X0302);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X0005);
	write_cmos_sensor_twobyte(0X602A, 0X0312);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X030E);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6F12, 0X0153);
	write_cmos_sensor_twobyte(0X602A, 0X030A);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0308);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X602A, 0X0342);
	write_cmos_sensor_twobyte(0X6F12, 0X2100);
	write_cmos_sensor_twobyte(0X602A, 0X0340);
	write_cmos_sensor_twobyte(0X6F12, 0X0C54);
	write_cmos_sensor_twobyte(0X602A, 0X021E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X30A6);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X602A, 0X30A8);
	write_cmos_sensor_twobyte(0X6f12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X30CA);
	write_cmos_sensor_twobyte(0X6F12, 0X0031);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A70);
	write_cmos_sensor_twobyte(0X6F12, 0X6E33);
	write_cmos_sensor_twobyte(0X602A, 0X4814);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X4876);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X2412);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);
	write_cmos_sensor_twobyte(0X602A, 0X3128);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0007);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0007);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0007);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0007);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X8724);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A30);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X30CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0702);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X228E);
	write_cmos_sensor_twobyte(0X6F12, 0X0245);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X602A, 0X8726);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X872C);
	write_cmos_sensor_twobyte(0X6F12, 0X0038);
	write_cmos_sensor_twobyte(0X602A, 0X09B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X515E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X5160);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X872E);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E0);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E2);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X4D70);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D80);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D90);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0B04);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X30A2);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X5400);
	write_cmos_sensor_twobyte(0X602A, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);

	#else // 12 fps for 2L1
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//Do not uwrite_cmos_sensor_twobyte(0Xe Memory retention
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6010);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	// Reset
	mDELAY(3);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X7970);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X7150);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X0136);
	write_cmos_sensor_twobyte(0X6F12, 0X1800);	// 24

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X7D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0449);
	write_cmos_sensor_twobyte(0X6F12, 0X0348);
	write_cmos_sensor_twobyte(0X6F12, 0X044A);
	write_cmos_sensor_twobyte(0X6F12, 0X4860);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0881);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC2BB);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X87AC);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5830);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XA3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0168);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X57FC);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X59FC);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0XC28E);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB1);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0X0280);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X48BC);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0XF148);
	write_cmos_sensor_twobyte(0X6F12, 0X8A46);
	write_cmos_sensor_twobyte(0X6F12, 0X9146);
	write_cmos_sensor_twobyte(0X6F12, 0X4168);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X39FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0027);
	write_cmos_sensor_twobyte(0X6F12, 0XA780);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X5146);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFC);
	write_cmos_sensor_twobyte(0X6F12, 0X98F8);
	write_cmos_sensor_twobyte(0X6F12, 0XE200);
	write_cmos_sensor_twobyte(0X6F12, 0XE64A);
	write_cmos_sensor_twobyte(0X6F12, 0XE749);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD9);
	write_cmos_sensor_twobyte(0X6F12, 0XE648);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6C09);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF087);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XF8E7);
	write_cmos_sensor_twobyte(0X6F12, 0XFEB5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XD848);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X8168);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X17FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X03FC);
	write_cmos_sensor_twobyte(0X6F12, 0XD048);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X0029);
	write_cmos_sensor_twobyte(0X6F12, 0X1CD0);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XD900);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0XCB48);
	write_cmos_sensor_twobyte(0X6F12, 0X0292);
	write_cmos_sensor_twobyte(0X6F12, 0X0192);
	write_cmos_sensor_twobyte(0X6F12, 0X4A30);
	write_cmos_sensor_twobyte(0X6F12, 0XCA00);
	write_cmos_sensor_twobyte(0X6F12, 0XC17D);
	write_cmos_sensor_twobyte(0X6F12, 0XCD4C);
	write_cmos_sensor_twobyte(0X6F12, 0XC91D);
	write_cmos_sensor_twobyte(0X6F12, 0XC908);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0123);
	write_cmos_sensor_twobyte(0X6F12, 0X43EA);
	write_cmos_sensor_twobyte(0X6F12, 0X4101);
	write_cmos_sensor_twobyte(0X6F12, 0X6180);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X6B46);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF2);
	write_cmos_sensor_twobyte(0X6F12, 0X1010);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF7FB);
	write_cmos_sensor_twobyte(0X6F12, 0X0C20);
	write_cmos_sensor_twobyte(0X6F12, 0X2080);
	write_cmos_sensor_twobyte(0X6F12, 0XFEBD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC068);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD3FB);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE8FB);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0XC18A);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0268);
	write_cmos_sensor_twobyte(0X6F12, 0X02F5);
	write_cmos_sensor_twobyte(0X6F12, 0XC972);
	write_cmos_sensor_twobyte(0X6F12, 0X907A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0XBB49);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0XB4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF600);
	write_cmos_sensor_twobyte(0X6F12, 0X917A);
	write_cmos_sensor_twobyte(0X6F12, 0X411A);
	write_cmos_sensor_twobyte(0X6F12, 0XB948);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X34F8);
	write_cmos_sensor_twobyte(0X6F12, 0XFA1F);
	write_cmos_sensor_twobyte(0X6F12, 0X638A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X937A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X8180);
	write_cmos_sensor_twobyte(0X6F12, 0X1078);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD1);
	write_cmos_sensor_twobyte(0X6F12, 0XAE48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1115);
	write_cmos_sensor_twobyte(0X6F12, 0XB248);
	write_cmos_sensor_twobyte(0X6F12, 0X19B1);
	write_cmos_sensor_twobyte(0X6F12, 0XA749);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5D10);
	write_cmos_sensor_twobyte(0X6F12, 0X491C);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCC10);
	write_cmos_sensor_twobyte(0X6F12, 0XA348);
	write_cmos_sensor_twobyte(0X6F12, 0XB830);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X4188);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X9FBB);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0X9D48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0169);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XAFFB);
	write_cmos_sensor_twobyte(0X6F12, 0X9C48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1202);
	write_cmos_sensor_twobyte(0X6F12, 0X40B1);
	write_cmos_sensor_twobyte(0X6F12, 0X9D49);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X0968);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X7DBB);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4F);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0X8B48);
	write_cmos_sensor_twobyte(0X6F12, 0X81B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X4069);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X80B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0091);
	write_cmos_sensor_twobyte(0X6F12, 0X8346);
	write_cmos_sensor_twobyte(0X6F12, 0X0146);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6EFB);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0X1492);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF0);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X09F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3409);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X28A0);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X24A0);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X50A0);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0600);
	write_cmos_sensor_twobyte(0X6F12, 0X0028);
	write_cmos_sensor_twobyte(0X6F12, 0X76D0);
	write_cmos_sensor_twobyte(0X6F12, 0X8948);
	write_cmos_sensor_twobyte(0X6F12, 0X0026);
	write_cmos_sensor_twobyte(0X6F12, 0X3746);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D02);
	write_cmos_sensor_twobyte(0X6F12, 0XD8B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0024);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8405);
	write_cmos_sensor_twobyte(0X6F12, 0X296E);
	write_cmos_sensor_twobyte(0X6F12, 0X81B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0A22);
	write_cmos_sensor_twobyte(0X6F12, 0X286A);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X72FB);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4011);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XC021);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF001);
	write_cmos_sensor_twobyte(0X6F12, 0X5143);
	write_cmos_sensor_twobyte(0X6F12, 0X4031);
	write_cmos_sensor_twobyte(0X6F12, 0X00EB);
	write_cmos_sensor_twobyte(0X6F12, 0XE110);
	write_cmos_sensor_twobyte(0X6F12, 0X0644);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X641C);
	write_cmos_sensor_twobyte(0X6F12, 0X0744);
	write_cmos_sensor_twobyte(0X6F12, 0X102C);
	write_cmos_sensor_twobyte(0X6F12, 0XE4D3);
	write_cmos_sensor_twobyte(0X6F12, 0XF817);
	write_cmos_sensor_twobyte(0X6F12, 0X07EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0511);
	write_cmos_sensor_twobyte(0X6F12, 0XF017);
	write_cmos_sensor_twobyte(0X6F12, 0X06EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0411);
	write_cmos_sensor_twobyte(0X6F12, 0X99F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0700);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X0428);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X654E);
	write_cmos_sensor_twobyte(0X6F12, 0XD071);
	write_cmos_sensor_twobyte(0X6F12, 0X8236);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X2ADD);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X08D0);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X01EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X8B6A);
	write_cmos_sensor_twobyte(0X6F12, 0XCB62);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C00);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0XC462);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X92F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0730);
	write_cmos_sensor_twobyte(0X6F12, 0X8857);
	write_cmos_sensor_twobyte(0X6F12, 0X9842);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X8855);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C30);
	write_cmos_sensor_twobyte(0X6F12, 0X9957);
	write_cmos_sensor_twobyte(0X6F12, 0X0129);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDD);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0313);
	write_cmos_sensor_twobyte(0X6F12, 0X04E0);
	write_cmos_sensor_twobyte(0X6F12, 0X03EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8007);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XFF6A);
	write_cmos_sensor_twobyte(0X6F12, 0X3C44);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D8);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0XF1F4);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0800);
	write_cmos_sensor_twobyte(0X6F12, 0X50B3);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X14DB);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X10DB);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X1BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FB);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X21DB);
	write_cmos_sensor_twobyte(0X6F12, 0X22E0);
	write_cmos_sensor_twobyte(0X6F12, 0X34E0);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X11DC);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XFEFA);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0XAC42);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X6F12, 0X601B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X281B);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X18B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C20);
	write_cmos_sensor_twobyte(0X6F12, 0X02F8);
	write_cmos_sensor_twobyte(0X6F12, 0X06A0);
	write_cmos_sensor_twobyte(0X6F12, 0X0299);
	write_cmos_sensor_twobyte(0X6F12, 0X8968);
	write_cmos_sensor_twobyte(0X6F12, 0X4A78);
	write_cmos_sensor_twobyte(0X6F12, 0X0243);
	write_cmos_sensor_twobyte(0X6F12, 0X4A70);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0XC1E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0954);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5000);
	write_cmos_sensor_twobyte(0X6F12, 0X6C40);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5440);
	write_cmos_sensor_twobyte(0X6F12, 0X02AC);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X0ECC);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDEFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X05B0);
	write_cmos_sensor_twobyte(0X6F12, 0X5946);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF04F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA4BA);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X8246);
	write_cmos_sensor_twobyte(0X6F12, 0X1F48);
	write_cmos_sensor_twobyte(0X6F12, 0X8946);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X8169);
	write_cmos_sensor_twobyte(0X6F12, 0X1E46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0A9C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FA);
	write_cmos_sensor_twobyte(0X6F12, 0X1848);
	write_cmos_sensor_twobyte(0X6F12, 0X3346);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C40);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0X5046);
	write_cmos_sensor_twobyte(0X6F12, 0X0094);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC0FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X82BA);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0023);
	write_cmos_sensor_twobyte(0X6F12, 0X03E0);
	write_cmos_sensor_twobyte(0X6F12, 0X51F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2340);
	write_cmos_sensor_twobyte(0X6F12, 0X6040);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1C);
	write_cmos_sensor_twobyte(0X6F12, 0XB3EB);
	write_cmos_sensor_twobyte(0X6F12, 0X920F);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D3);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0948);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC069);
	write_cmos_sensor_twobyte(0X6F12, 0X87B2);
	write_cmos_sensor_twobyte(0X6F12, 0X4FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X1048);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X034C);
	write_cmos_sensor_twobyte(0X6F12, 0X3068);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A11);
	write_cmos_sensor_twobyte(0X6F12, 0XF1B3);
	write_cmos_sensor_twobyte(0X6F12, 0X19E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF4D2);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X8000);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X9000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF110);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5810);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X6AF0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XE304);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X7000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4B);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X51D1);
	write_cmos_sensor_twobyte(0X6F12, 0XFD48);
	write_cmos_sensor_twobyte(0X6F12, 0XFD49);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0XF0C3);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2FF);
	write_cmos_sensor_twobyte(0X6F12, 0XE066);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0XF949);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XABFF);
	write_cmos_sensor_twobyte(0X6F12, 0X2067);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XF749);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XA5FF);
	write_cmos_sensor_twobyte(0X6F12, 0X6067);
	write_cmos_sensor_twobyte(0X6F12, 0X3168);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XE36D);
	write_cmos_sensor_twobyte(0X6F12, 0XE26E);
	write_cmos_sensor_twobyte(0X6F12, 0X0126);
	write_cmos_sensor_twobyte(0X6F12, 0X9A42);
	write_cmos_sensor_twobyte(0X6F12, 0X01D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X60C0);
	write_cmos_sensor_twobyte(0X6F12, 0X236F);
	write_cmos_sensor_twobyte(0X6F12, 0X6345);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X020C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X64C0);
	write_cmos_sensor_twobyte(0X6F12, 0X6045);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X040C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XBCF1);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X07D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X5A40);
	write_cmos_sensor_twobyte(0X6F12, 0X4240);
	write_cmos_sensor_twobyte(0X6F12, 0XA266);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A51);
	write_cmos_sensor_twobyte(0X6F12, 0X0AE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B61);
	write_cmos_sensor_twobyte(0X6F12, 0XF6E7);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XA566);
	write_cmos_sensor_twobyte(0X6F12, 0XE566);
	write_cmos_sensor_twobyte(0X6F12, 0X2567);
	write_cmos_sensor_twobyte(0X6F12, 0X6567);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X27FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0XD348);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FA);
	write_cmos_sensor_twobyte(0X6F12, 0XCF48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A01);
	write_cmos_sensor_twobyte(0X6F12, 0X48B3);
	write_cmos_sensor_twobyte(0X6F12, 0XC348);
	write_cmos_sensor_twobyte(0X6F12, 0XC44B);
	write_cmos_sensor_twobyte(0X6F12, 0XC44A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X20D1);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0DFA);
	write_cmos_sensor_twobyte(0X6F12, 0XC148);
	write_cmos_sensor_twobyte(0X6F12, 0XC149);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XC04F);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFF);
	write_cmos_sensor_twobyte(0X6F12, 0XC14C);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0XBD49);
	write_cmos_sensor_twobyte(0X6F12, 0XE065);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X32FF);
	write_cmos_sensor_twobyte(0X6F12, 0X2066);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XBA49);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFF);
	write_cmos_sensor_twobyte(0X6F12, 0X6066);
	write_cmos_sensor_twobyte(0X6F12, 0XD4E9);
	write_cmos_sensor_twobyte(0X6F12, 0X1712);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X4140);
	write_cmos_sensor_twobyte(0X6F12, 0XA165);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA1B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XB148);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X406A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDBF9);
	write_cmos_sensor_twobyte(0X6F12, 0XE068);
	write_cmos_sensor_twobyte(0X6F12, 0X07F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8047);
	write_cmos_sensor_twobyte(0X6F12, 0X8069);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0XC288);
	write_cmos_sensor_twobyte(0X6F12, 0X0389);
	write_cmos_sensor_twobyte(0X6F12, 0XA1EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5201);
	write_cmos_sensor_twobyte(0X6F12, 0X827B);
	write_cmos_sensor_twobyte(0X6F12, 0X891A);
	write_cmos_sensor_twobyte(0X6F12, 0X8289);
	write_cmos_sensor_twobyte(0X6F12, 0XC07B);
	write_cmos_sensor_twobyte(0X6F12, 0XA2EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5302);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0XB980);
	write_cmos_sensor_twobyte(0X6F12, 0XE268);
	write_cmos_sensor_twobyte(0X6F12, 0X491E);
	write_cmos_sensor_twobyte(0X6F12, 0X9269);
	write_cmos_sensor_twobyte(0X6F12, 0X128A);
	write_cmos_sensor_twobyte(0X6F12, 0X1144);
	write_cmos_sensor_twobyte(0X6F12, 0XF980);
	write_cmos_sensor_twobyte(0X6F12, 0X3881);
	write_cmos_sensor_twobyte(0X6F12, 0XE168);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X8969);
	write_cmos_sensor_twobyte(0X6F12, 0X498A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X7881);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BB9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8146);
	write_cmos_sensor_twobyte(0X6F12, 0X9648);
	write_cmos_sensor_twobyte(0X6F12, 0X0D46);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X816A);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0E0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X5CF9);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X4846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA6F9);
	write_cmos_sensor_twobyte(0X6F12, 0X8E48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9201);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X15D1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2900);
	write_cmos_sensor_twobyte(0X6F12, 0X70B1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCE02);
	write_cmos_sensor_twobyte(0X6F12, 0X08B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6F12, 0X07E0);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XDA00);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X8748);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5C00);
	write_cmos_sensor_twobyte(0X6F12, 0X10B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4210);
	write_cmos_sensor_twobyte(0X6F12, 0X88F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1110);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X33B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1001);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X83F9);
	write_cmos_sensor_twobyte(0X6F12, 0X774D);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XF006);
	write_cmos_sensor_twobyte(0X6F12, 0X05F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3405);
	write_cmos_sensor_twobyte(0X6F12, 0X1ED5);
	write_cmos_sensor_twobyte(0X6F12, 0X7748);
	write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XB207);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X03D2);
	write_cmos_sensor_twobyte(0X6F12, 0X2146);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X76F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X1346);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0X39B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0F88);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7870);
	write_cmos_sensor_twobyte(0X6F12, 0XC08A);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0X01E0);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7830);
	write_cmos_sensor_twobyte(0X6F12, 0X521C);
	write_cmos_sensor_twobyte(0X6F12, 0X062A);
	write_cmos_sensor_twobyte(0X6F12, 0XEED3);
	write_cmos_sensor_twobyte(0X6F12, 0X7004);
	write_cmos_sensor_twobyte(0X6F12, 0X1AD5);
	write_cmos_sensor_twobyte(0X6F12, 0XB5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8400);
	write_cmos_sensor_twobyte(0X6F12, 0X411E);
	write_cmos_sensor_twobyte(0X6F12, 0XA142);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X04D2);
	write_cmos_sensor_twobyte(0X6F12, 0X44F4);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X54F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4002);
	write_cmos_sensor_twobyte(0X6F12, 0X5189);
	write_cmos_sensor_twobyte(0X6F12, 0X29B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7820);
	write_cmos_sensor_twobyte(0X6F12, 0X12B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0A80);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X0628);
	write_cmos_sensor_twobyte(0X6F12, 0XF2D3);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF081);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X5448);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC16A);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDAF8);
	write_cmos_sensor_twobyte(0X6F12, 0X43F6);
	write_cmos_sensor_twobyte(0X6F12, 0XEF73);
	write_cmos_sensor_twobyte(0X6F12, 0X0322);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XB901);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X36F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X38F9);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC8B8);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X484C);
	write_cmos_sensor_twobyte(0X6F12, 0XE38B);
	write_cmos_sensor_twobyte(0X6F12, 0XCB1A);
	write_cmos_sensor_twobyte(0X6F12, 0X218C);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X0144);
	write_cmos_sensor_twobyte(0X6F12, 0X9942);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X0346);
	write_cmos_sensor_twobyte(0X6F12, 0X4149);
	write_cmos_sensor_twobyte(0X6F12, 0X3431);
	write_cmos_sensor_twobyte(0X6F12, 0X9040);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8630);
	write_cmos_sensor_twobyte(0X6F12, 0X03FA);
	write_cmos_sensor_twobyte(0X6F12, 0X02F3);
	write_cmos_sensor_twobyte(0X6F12, 0X414A);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XD181);
	write_cmos_sensor_twobyte(0X6F12, 0X1382);
	write_cmos_sensor_twobyte(0X6F12, 0X03D0);
	write_cmos_sensor_twobyte(0X6F12, 0X5181);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X9381);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF70);
	write_cmos_sensor_twobyte(0X6F12, 0X5081);
	write_cmos_sensor_twobyte(0X6F12, 0X9081);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7B71);
	write_cmos_sensor_twobyte(0X6F12, 0X3948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0FF9);
	write_cmos_sensor_twobyte(0X6F12, 0X334C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5371);
	write_cmos_sensor_twobyte(0X6F12, 0X2060);
	write_cmos_sensor_twobyte(0X6F12, 0X3648);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X07F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XFF61);
	write_cmos_sensor_twobyte(0X6F12, 0X6060);
	write_cmos_sensor_twobyte(0X6F12, 0X3448);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X00F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XA060);
	write_cmos_sensor_twobyte(0X6F12, 0X3148);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2D61);
	write_cmos_sensor_twobyte(0X6F12, 0XE060);
	write_cmos_sensor_twobyte(0X6F12, 0X2F48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XF551);
	write_cmos_sensor_twobyte(0X6F12, 0X2061);
	write_cmos_sensor_twobyte(0X6F12, 0X2C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XEBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5341);
	write_cmos_sensor_twobyte(0X6F12, 0X6061);
	write_cmos_sensor_twobyte(0X6F12, 0X2A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XA061);
	write_cmos_sensor_twobyte(0X6F12, 0X2021);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X6000);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8006);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3400);
	write_cmos_sensor_twobyte(0X6F12, 0X7580);
	write_cmos_sensor_twobyte(0X6F12, 0X4562);
	write_cmos_sensor_twobyte(0X6F12, 0X8562);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C50);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5050);
	write_cmos_sensor_twobyte(0X6F12, 0X2A46);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7D21);
	write_cmos_sensor_twobyte(0X6F12, 0X4565);
	write_cmos_sensor_twobyte(0X6F12, 0X1E48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XCBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2121);
	write_cmos_sensor_twobyte(0X6F12, 0X6062);
	write_cmos_sensor_twobyte(0X6F12, 0X1C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X4541);
	write_cmos_sensor_twobyte(0X6F12, 0XA062);
	write_cmos_sensor_twobyte(0X6F12, 0X1948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XBDF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F31);
	write_cmos_sensor_twobyte(0X6F12, 0XE061);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0XABBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0345);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0X1234);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5423);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0XFECA);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X7C80);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0B27);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0E5D);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X5C51);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X1F65);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X98A9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X852F);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XDE2F);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X10B3);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0823);
	write_cmos_sensor_twobyte(0X6F12, 0X0A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X87F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X2062);
	write_cmos_sensor_twobyte(0X6F12, 0X0848);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X6F11);
	write_cmos_sensor_twobyte(0X6F12, 0XE062);
	write_cmos_sensor_twobyte(0X6F12, 0X0548);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X79F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2063);
	write_cmos_sensor_twobyte(0X6F12, 0XF562);
	write_cmos_sensor_twobyte(0X6F12, 0X3563);
	write_cmos_sensor_twobyte(0X6F12, 0X7563);
	write_cmos_sensor_twobyte(0X6F12, 0X70BD);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X339B);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X2ABB);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X135F);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0XE91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X273C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X5D6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X033C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X3B2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X45F6);
	write_cmos_sensor_twobyte(0X6F12, 0X514C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X657C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D0C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X471C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X49F6);
	write_cmos_sensor_twobyte(0X6F12, 0XA90C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X48F2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X230C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X43F2);
	write_cmos_sensor_twobyte(0X6F12, 0X9B3C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3B7C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X4DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X2F6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB30C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X871C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X6F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1B5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0XF91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X42F6);
	write_cmos_sensor_twobyte(0X6F12, 0XBB2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB71C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);

	write_cmos_sensor_twobyte(0X602A, 0X1150);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ah 680mV
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ch 680mV
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF466);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	// aig_dbr_tune_tg_l 3.05V
	write_cmos_sensor_twobyte(0X6F12, 0X000C);	// aig_dbr_tune_tg_r 3.10V
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X111C);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	// F46Ah 3.7V
	write_cmos_sensor_twobyte(0X602A, 0X1120);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	// F470h -1.1V
	write_cmos_sensor_twobyte(0X602A, 0X113E);
	write_cmos_sensor_twobyte(0X6F12, 0X2101);	// it equalwrite_cmos_sensor_twobyte(0X to aig_left_right_tg_write_cmos_sensor_twobyte(0Xel 06h
	write_cmos_sensor_twobyte(0X602A, 0X1106);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);


	write_cmos_sensor_twobyte(0X602A, 0X3A28);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A52);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A48);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:8 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:4 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A34);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:2 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A84);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// write_cmos_sensor_twobyte(0Xhort Red 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3AB6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// write_cmos_sensor_twobyte(0Xhort Blue 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3B44);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3B48);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X3B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);

	// EIT write_cmos_sensor_twobyte, 0X(0Xe);tting
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0200);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0224);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0224h Addrewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0226h Addrewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X

	// ETC write_cmos_sensor_twobyte, 0X(0Xe);tting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X1124);
	write_cmos_sensor_twobyte(0X6F12, 0X0328);	// ADBUwrite_cmos_sensor_twobyte(0X data order
	write_cmos_sensor_twobyte(0X602A, 0X0ACE);
	write_cmos_sensor_twobyte(0X6F12, 0X0030);

	write_cmos_sensor_twobyte(0X602A, 0X09C0);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X09C2);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X6F12, 0X008C);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0XF408);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X54A2);
	write_cmos_sensor_twobyte(0X6F12, 0XF418);	// CDwrite_cmos_sensor_twobyte(0X_Option_L
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X000D);
	write_cmos_sensor_twobyte(0X6F12, 0XF472);	// CLP_Level
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF482);	// COMP_BIAwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF484);	// COMP_BIAwrite_cmos_sensor_twobyte(0X

	// ADLC & Dithering write_cmos_s, 0Xenso);r_twobyte(0Xetting
	write_cmos_sensor_twobyte(0X602A, 0X47F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FE);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);

	// ATOP write_cmos_sensor_twobyt, 0Xe(0X);etting
	write_cmos_sensor_twobyte(0X602A, 0X1108);
	write_cmos_sensor_twobyte(0X6F12, 0X054C);	// F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt on & LP_VBLK_EN)
	write_cmos_sensor_twobyte(0X6F12, 0X0183);	// F406h Def. 0083h
	write_cmos_sensor_twobyte(0X6F12, 0X07FF);	// F408h Def. 07DFh (Dabx 0ff & Dwrite_cmos_sensor_twobyte(0Xrx en)


	write_cmos_sensor_twobyte(0X602A, 0X1154);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);	// F418h LDB on x4 write_cmos_sensor_twobyte(0X3
	write_cmos_sensor_twobyte(0X6F12, 0X162F);	// F41Ah // CRwrite_cmos_sensor_twobyte(0Xon
	write_cmos_sensor_twobyte(0X6F12, 0X4328);	// F41Ch
	write_cmos_sensor_twobyte(0X602A, 0X1126);
	write_cmos_sensor_twobyte(0X6F12, 0X00A2);	// F4A6h  IPI write_cmos_sensor_twobyte(0Xignal dip control
	write_cmos_sensor_twobyte(0X602A, 0X114A);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_write_cmos_sensor_twobyte(0XenAnalog_rmp_option_top
	write_cmos_sensor_twobyte(0X602A, 0X114E);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_write_cmos_sensor_twobyte(0XenAnalog_rmp_option_bot
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF41E);
	write_cmos_sensor_twobyte(0X6F12, 0X2100);	// DBUwrite_cmos_sensor_twobyte(0X option (Hybrid Gain)
	write_cmos_sensor_twobyte(0X602A, 0XF488);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// REF option

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X112A);
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
	write_cmos_sensor_twobyte(0X602A, 0X1128);
	write_cmos_sensor_twobyte(0X6F12, 0X001B);

	// Current write_cmos_sensor_two, 0Xbyte);(0Xetting
	write_cmos_sensor_twobyte(0X602A, 0X10F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);	// F476h
	write_cmos_sensor_twobyte(0X602A, 0X10F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);	// F478h
	write_cmos_sensor_twobyte(0X602A, 0X10FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);	// F47Ah
	write_cmos_sensor_twobyte(0X602A, 0X10FE);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);	// F47Ch

	// DBUwrite_cmos_sensor_twobyte(, 0X0X L);atch write_cmos_sensor_twobyte(0Xetting
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF414);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// aig_latch_write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X6F12, 0X0004);	// aig_latch_width

	// Clock write_cmos_sensor_twoby, 0Xte(0);Xetting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X10C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X602A, 0X10B6);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X0B26);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X602A, 0X0B2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X602A, 0X0B36);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);


	write_cmos_sensor_twobyte(0X602A, 0X0B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_write_cmos_sensor_twobyte(0Xl_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_write_cmos_sensor_twobyte(0Xl_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_write_cmos_sensor_twobyte(0Xl_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_write_cmos_sensor_twobyte(0Xl_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//aig_rx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//aig_rx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_rx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_rx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_tx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B7);	//aig_tx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_tx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);	//aig_tx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0105);	//aig_fx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);	//aig_fx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0140);	//aig_fx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);	//aig_fx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00EC);	//aig_dwrite_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X00A6);	//aig_dwrite_cmos_sensor_twobyte(0Xrx_reg0

	write_cmos_sensor_twobyte(0X602A, 0X0B76);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);	//aig_write_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X004D);	//aig_write_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B7E);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dwrite_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X009F);	//aig_dwrite_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B86);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_write_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_write_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B8E);
	write_cmos_sensor_twobyte(0X6F12, 0X0047);	//aig_write_cmos_sensor_twobyte(0Xfx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0048);	//aig_write_cmos_sensor_twobyte(0Xfx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B94);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_write_cmos_sensor_twobyte(0Xfx_reg1
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dabx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X009F);	//aig_dabx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_abx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_abx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0BAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0101 //aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 00B2 //aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026E //aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 01CD //aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0106 //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 00B7 //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026C //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 01CB //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_write_cmos_sensor_twobyte(0X1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_write_cmos_sensor_twobyte(0X1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0156);	//aig_write_cmos_sensor_twobyte(0X1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00FF);	//aig_write_cmos_sensor_twobyte(0X1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0057);	//aig_write_cmos_sensor_twobyte(0X3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0057);	//aig_write_cmos_sensor_twobyte(0X3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X004E);	//aig_write_cmos_sensor_twobyte(0X4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X004E);	//aig_write_cmos_sensor_twobyte(0X4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_clp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_clp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_clp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_clp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0016);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X014E);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00FA);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006B);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X006B);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_lp_hblk_cdwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_lp_hblk_cdwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C0A);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000F);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000F);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00D7);	//aig_cnt_en_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0087);	//aig_cnt_en_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_cnt_en_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_cnt_en_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01D6);	//aig_cnt_en_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0134);	//aig_cnt_en_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026A);	//aig_cnt_en_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);	//aig_cnt_en_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_enb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_conv_enb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_conv_enb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);	//aig_conv_enb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_conv1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);	//aig_conv1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0111);	//aig_conv2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00C2);	//aig_conv2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C62);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00C0);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C72);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//aig_comp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0085);	//aig_comp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0104);	//aig_comp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B5);	//aig_comp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01D4);	//aig_comp_en_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0132);	//aig_comp_en_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_comp_en_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_comp_en_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C92);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);	//aig_rd_en_10_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0002);	//aig_rd_en_10_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X001C);	//aig_rd_en_10_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0016);	//aig_rd_en_10_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X001E);	//aig_rd_en_9_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0017);	//aig_rd_en_9_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0038);	//aig_rd_en_9_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X002B);	//aig_rd_en_9_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X003A);	//aig_rd_en_8_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X002C);	//aig_rd_en_8_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0054);	//aig_rd_en_8_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0040);	//aig_rd_en_8_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0076);	//aig_rd_en_7_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0041);	//aig_rd_en_7_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0090);	//aig_rd_en_7_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_rd_en_7_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X010B);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00C6);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0127);	//aig_rd_en_5_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00C7);	//aig_rd_en_5_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0141);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0143);	//aig_rd_en_4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00DC);	//aig_rd_en_4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X015D);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X015F);	//aig_rd_en_3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);	//aig_rd_en_3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0179);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0105);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X017B);	//aig_rd_en_2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_rd_en_2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0195);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X011A);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0197);	//aig_rd_en_1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	//aig_rd_en_1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01B1);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01B4);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01B3);	//aig_rd_en_0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01B5);	//aig_rd_en_0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr6
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr6
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr7
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr7
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr8
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr8
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr9
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr9
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr10
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr10
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr11
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr11
	write_cmos_sensor_twobyte(0X602A, 0X0D22);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D32);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X006F);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006F);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0270);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CE);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0071);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0070);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0073);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0072);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0122);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00D3);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0103);	//aig_rmp_mode_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);	//aig_rmp_mode_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006A);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0069);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0264);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C4);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D74);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);	//aig_tg_hwrite_cmos_sensor_twobyte(0Xync_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0014);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0014);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_optl0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_optl0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D96);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);	//aig_optl1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00D9);	//aig_optl1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0132);	//aig_optl1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00E3);	//aig_optl1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_optl2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_optl2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_optl2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);	//aig_optl2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);	//aig_optl3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00D1);	//aig_optl3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CA);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0E16);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_opth0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);	//aig_opth0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_opth0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_opth0_ptr1

	write_cmos_sensor_twobyte(0X602A, 0X0FA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0287);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X01CC);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X003C);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X003C);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0124);	//VIR_2_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//VIR_2_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X00DF);	//VIR_Mwrite_cmos_sensor_twobyte(0XO_PTR0_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X0099);	//VIR_Mwrite_cmos_sensor_twobyte(0XO_PTR0_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X0296);	//VIR_MIN_ADC_COLwrite_cmos_sensor_twobyte(0X_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);	//VIR_MIN_ADC_COLwrite_cmos_sensor_twobyte(0X_1_1_1

	write_cmos_sensor_twobyte(0X602A, 0X0FD8);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0FDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);	//aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0301);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0FEC);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0003);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X101C);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X101E);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1020);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1022);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1024);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1026);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1028);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X104C);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X106C);
	write_cmos_sensor_twobyte(0X6F12, 0X0607);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X10AE);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X10B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1

	/////// APwrite_cmos_sensor_twob, 0Xyte();0X PD
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_AigMain1Value 0540	// F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt off & LP_VBLK_EN)
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_cdwrite_cmos_sensor_twobyte(0X_option_m	4338	// F41Ch
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_cdwrite_cmos_sensor_twobyte(0X_option_md	162E	// F41Ah
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__low_gain_val 000C	// F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__high_gain_val 000C	// F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_1__low_gain_val 0080
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_1__high_gain_val 0080


	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_AigMain1Value 0544   // F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt off)
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__low_gain_val 000F // F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__high_gain_val 000F // F408h

	write_cmos_sensor_twobyte(0X602A, 0X0B0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3064);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0B02);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X0ABC);
	write_cmos_sensor_twobyte(0X6F12, 0X0207);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X070C);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);

	write_cmos_sensor_twobyte(0X602A, 0X0110);
	write_cmos_sensor_twobyte(0X6F12, 0X1002);
	write_cmos_sensor_twobyte(0X602A, 0X0114);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);

	write_cmos_sensor_twobyte(0X602A, 0X0408);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X602A, 0X0344);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X1F6F);	// 8063
	write_cmos_sensor_twobyte(0X6F12, 0X0BC7);	// 3023

	write_cmos_sensor_twobyte(0X6F12, 0X1F60);	// 8064
	write_cmos_sensor_twobyte(0X6F12, 0X0BC0);	// 3024

	write_cmos_sensor_twobyte(0X602A, 0X0380);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0900);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);

	write_cmos_sensor_twobyte(0X602A, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0404);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);

	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);

	//PLL write_cmos_sensor_twobyte(, 0X0Xyw);rite_cmos_sensor_twobyte(0X = 800 , write_cmos_sensor_twobyte(0Xec = 798
	write_cmos_sensor_twobyte(0X602A, 0X0304);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X030C);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0306);
	write_cmos_sensor_twobyte(0X6F12, 0X01F4);	// 500
	write_cmos_sensor_twobyte(0X602A, 0X0302);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X0005);

	write_cmos_sensor_twobyte(0X602A, 0X0312);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X030E);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	// 266
	write_cmos_sensor_twobyte(0X602A, 0X030A);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0308);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);

	write_cmos_sensor_twobyte(0X602A, 0X0342);
	write_cmos_sensor_twobyte(0X6F12, 0X5120);	// 20768
	write_cmos_sensor_twobyte(0X602A, 0X0340);
	write_cmos_sensor_twobyte(0X6F12, 0X0C8A);	// 3210

	write_cmos_sensor_twobyte(0X602A, 0X021E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	// write_cmos_sensor_twobyte(0Xu, 0Xmmat);ion
	write_cmos_sensor_twobyte(0X602A, 0X30A6);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);	//Interleaved LRLR

	write_cmos_sensor_twobyte(0X602A, 0X30A8);
	write_cmos_sensor_twobyte(0X6f12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X30CA);
	write_cmos_sensor_twobyte(0X6F12, 0X0031);

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A70);
	write_cmos_sensor_twobyte(0X6F12, 0X6E33);

	// Bypawrite_cmos_sensor_twobyte, 0X(0Xw);rite_cmos_sensor_twobyte(0X all chain procewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0Xing
	write_cmos_sensor_twobyte(0X602A, 0X4814);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X4876);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);

	write_cmos_sensor_twobyte(0X602A, 0X2412);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);	//4x4 mwrite_cmos_sensor_twobyte(0Xm
	write_cmos_sensor_twobyte(0X602A, 0X3128);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb

	// wait to write_cmos_sensor_two, 0Xbyte);(0Xtart Work around - Add delay from H time write_cmos_sensor_twobyte(0Xtart.
	write_cmos_sensor_twobyte(0X602A, 0X8724);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);	// ex. add 32 TG CLC'write_cmos_sensor_twobyte(0X Delay (Delay in TG CLK'write_cmos_sensor_twobyte(0X)

	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY

	//Modify on-Line write_cmos_sens, 0Xor_t);wobyte(0Xettingwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A30);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);

	//emb linewrite_cmos_sensor_twob, 0Xyte();
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X30CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);

	//TnP fix: line write_cmos_senso, 0Xr_tw);obyte(0Xwap write_cmos_sensor_twobyte(0Xupport for WDR Off
	write_cmos_sensor_twobyte(0X602A, 0X0702);
	write_cmos_sensor_twobyte(0X6F12, 0X01	);//01 Enabled ;00 Diwrite_cmos_sensor_twobyte(0Xabled

	//TnP fix: iir filter rewrite_cm, 0Xos_s);ensor_twobyte(0Xet uwrite_cmos_sensor_twobyte(0Xing FOB write_cmos_sensor_twobyte(0Xtatiwrite_cmos_sensor_twobyte(0Xticwrite_cmos_sensor_twobyte(0X diff
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X228E);
	write_cmos_sensor_twobyte(0X6F12, 0X0245);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X602A, 0X8726);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X872C);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);

	//write_cmos_sensor_twobyte(0Xub, 0Xtrac);t on write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X602A, 0X09B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	//enable write_cmos_sensor_twobyte(0Xubtract on write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X602A, 0X515E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//Diwrite_cmos_sensor_twobyte(0Xable write_cmos_sensor_twobyte(0Xubtract of Y Data
	write_cmos_sensor_twobyte(0X602A, 0X5160);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	//TnP:	HW regiwrite_cmos_sensor, 0X_two);byte(0Xter update on V-Blank	(modify different HW write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X on V-Blank)
	write_cmos_sensor_twobyte(0X602A, 0X872E);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E0);	//aig_dummy_addr_dr_rb_e
	write_cmos_sensor_twobyte(0X6F12, 0XF4E2);	//aig_dummy_addr_dr_rb_o
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X for tewrite_cmos_sensor_twobyte(0Xt
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X for tewrite_cmos_sensor_twobyte(0Xt
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X602A, 0X4D70);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D80);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D90);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0B04);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X30A2);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X5400);

	write_cmos_sensor_twobyte(0X602A, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);

	#endif
#else
	// M2 setting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//Do not use Memory retention
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6010);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	// Reset
	mDELAY(3);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X7970);	// Modify CLK to allow HW writes on STBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X7150);	// Modify CLK to allow HW writes on STBY
	write_cmos_sensor_twobyte(0X602A, 0X0136);
	write_cmos_sensor_twobyte(0X6F12, 0X1800);	// 24
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X7D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0449);
	write_cmos_sensor_twobyte(0X6F12, 0X0348);
	write_cmos_sensor_twobyte(0X6F12, 0X044A);
	write_cmos_sensor_twobyte(0X6F12, 0X4860);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0881);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC2BB);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X87AC);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5830);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XA3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0168);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X57FC);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X59FC);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0XC28E);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB1);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0X0280);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X48BC);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0XF148);
	write_cmos_sensor_twobyte(0X6F12, 0X8A46);
	write_cmos_sensor_twobyte(0X6F12, 0X9146);
	write_cmos_sensor_twobyte(0X6F12, 0X4168);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X39FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0027);
	write_cmos_sensor_twobyte(0X6F12, 0XA780);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X5146);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFC);
	write_cmos_sensor_twobyte(0X6F12, 0X98F8);
	write_cmos_sensor_twobyte(0X6F12, 0XE200);
	write_cmos_sensor_twobyte(0X6F12, 0XE64A);
	write_cmos_sensor_twobyte(0X6F12, 0XE749);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD9);
	write_cmos_sensor_twobyte(0X6F12, 0XE648);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6C09);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF087);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XF8E7);
	write_cmos_sensor_twobyte(0X6F12, 0XFEB5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XD848);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X8168);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X17FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X03FC);
	write_cmos_sensor_twobyte(0X6F12, 0XD048);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X0029);
	write_cmos_sensor_twobyte(0X6F12, 0X1CD0);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XD900);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0XCB48);
	write_cmos_sensor_twobyte(0X6F12, 0X0292);
	write_cmos_sensor_twobyte(0X6F12, 0X0192);
	write_cmos_sensor_twobyte(0X6F12, 0X4A30);
	write_cmos_sensor_twobyte(0X6F12, 0XCA00);
	write_cmos_sensor_twobyte(0X6F12, 0XC17D);
	write_cmos_sensor_twobyte(0X6F12, 0XCD4C);
	write_cmos_sensor_twobyte(0X6F12, 0XC91D);
	write_cmos_sensor_twobyte(0X6F12, 0XC908);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0123);
	write_cmos_sensor_twobyte(0X6F12, 0X43EA);
	write_cmos_sensor_twobyte(0X6F12, 0X4101);
	write_cmos_sensor_twobyte(0X6F12, 0X6180);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X6B46);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF2);
	write_cmos_sensor_twobyte(0X6F12, 0X1010);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF7FB);
	write_cmos_sensor_twobyte(0X6F12, 0X0C20);
	write_cmos_sensor_twobyte(0X6F12, 0X2080);
	write_cmos_sensor_twobyte(0X6F12, 0XFEBD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC068);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD3FB);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE8FB);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0XC18A);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0268);
	write_cmos_sensor_twobyte(0X6F12, 0X02F5);
	write_cmos_sensor_twobyte(0X6F12, 0XC972);
	write_cmos_sensor_twobyte(0X6F12, 0X907A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0XBB49);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0XB4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF600);
	write_cmos_sensor_twobyte(0X6F12, 0X917A);
	write_cmos_sensor_twobyte(0X6F12, 0X411A);
	write_cmos_sensor_twobyte(0X6F12, 0XB948);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X34F8);
	write_cmos_sensor_twobyte(0X6F12, 0XFA1F);
	write_cmos_sensor_twobyte(0X6F12, 0X638A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X937A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X8180);
	write_cmos_sensor_twobyte(0X6F12, 0X1078);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD1);
	write_cmos_sensor_twobyte(0X6F12, 0XAE48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1115);
	write_cmos_sensor_twobyte(0X6F12, 0XB248);
	write_cmos_sensor_twobyte(0X6F12, 0X19B1);
	write_cmos_sensor_twobyte(0X6F12, 0XA749);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5D10);
	write_cmos_sensor_twobyte(0X6F12, 0X491C);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCC10);
	write_cmos_sensor_twobyte(0X6F12, 0XA348);
	write_cmos_sensor_twobyte(0X6F12, 0XB830);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X4188);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X9FBB);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0X9D48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0169);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XAFFB);
	write_cmos_sensor_twobyte(0X6F12, 0X9C48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1202);
	write_cmos_sensor_twobyte(0X6F12, 0X40B1);
	write_cmos_sensor_twobyte(0X6F12, 0X9D49);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X0968);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X7DBB);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4F);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0X8B48);
	write_cmos_sensor_twobyte(0X6F12, 0X81B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X4069);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X80B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0091);
	write_cmos_sensor_twobyte(0X6F12, 0X8346);
	write_cmos_sensor_twobyte(0X6F12, 0X0146);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6EFB);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0X1492);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF0);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X09F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3409);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X28A0);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X24A0);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X50A0);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0600);
	write_cmos_sensor_twobyte(0X6F12, 0X0028);
	write_cmos_sensor_twobyte(0X6F12, 0X76D0);
	write_cmos_sensor_twobyte(0X6F12, 0X8948);
	write_cmos_sensor_twobyte(0X6F12, 0X0026);
	write_cmos_sensor_twobyte(0X6F12, 0X3746);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D02);
	write_cmos_sensor_twobyte(0X6F12, 0XD8B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0024);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8405);
	write_cmos_sensor_twobyte(0X6F12, 0X296E);
	write_cmos_sensor_twobyte(0X6F12, 0X81B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0A22);
	write_cmos_sensor_twobyte(0X6F12, 0X286A);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X72FB);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4011);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XC021);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF001);
	write_cmos_sensor_twobyte(0X6F12, 0X5143);
	write_cmos_sensor_twobyte(0X6F12, 0X4031);
	write_cmos_sensor_twobyte(0X6F12, 0X00EB);
	write_cmos_sensor_twobyte(0X6F12, 0XE110);
	write_cmos_sensor_twobyte(0X6F12, 0X0644);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X641C);
	write_cmos_sensor_twobyte(0X6F12, 0X0744);
	write_cmos_sensor_twobyte(0X6F12, 0X102C);
	write_cmos_sensor_twobyte(0X6F12, 0XE4D3);
	write_cmos_sensor_twobyte(0X6F12, 0XF817);
	write_cmos_sensor_twobyte(0X6F12, 0X07EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0511);
	write_cmos_sensor_twobyte(0X6F12, 0XF017);
	write_cmos_sensor_twobyte(0X6F12, 0X06EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0411);
	write_cmos_sensor_twobyte(0X6F12, 0X99F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0700);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X0428);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X654E);
	write_cmos_sensor_twobyte(0X6F12, 0XD071);
	write_cmos_sensor_twobyte(0X6F12, 0X8236);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X2ADD);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X08D0);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X01EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X8B6A);
	write_cmos_sensor_twobyte(0X6F12, 0XCB62);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C00);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0XC462);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X92F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0730);
	write_cmos_sensor_twobyte(0X6F12, 0X8857);
	write_cmos_sensor_twobyte(0X6F12, 0X9842);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X8855);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C30);
	write_cmos_sensor_twobyte(0X6F12, 0X9957);
	write_cmos_sensor_twobyte(0X6F12, 0X0129);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDD);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0313);
	write_cmos_sensor_twobyte(0X6F12, 0X04E0);
	write_cmos_sensor_twobyte(0X6F12, 0X03EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8007);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XFF6A);
	write_cmos_sensor_twobyte(0X6F12, 0X3C44);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D8);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0XF1F4);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0800);
	write_cmos_sensor_twobyte(0X6F12, 0X50B3);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X14DB);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X10DB);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X1BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FB);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X21DB);
	write_cmos_sensor_twobyte(0X6F12, 0X22E0);
	write_cmos_sensor_twobyte(0X6F12, 0X34E0);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X11DC);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XFEFA);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0XAC42);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X6F12, 0X601B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X281B);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X18B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C20);
	write_cmos_sensor_twobyte(0X6F12, 0X02F8);
	write_cmos_sensor_twobyte(0X6F12, 0X06A0);
	write_cmos_sensor_twobyte(0X6F12, 0X0299);
	write_cmos_sensor_twobyte(0X6F12, 0X8968);
	write_cmos_sensor_twobyte(0X6F12, 0X4A78);
	write_cmos_sensor_twobyte(0X6F12, 0X0243);
	write_cmos_sensor_twobyte(0X6F12, 0X4A70);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0XC1E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0954);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5000);
	write_cmos_sensor_twobyte(0X6F12, 0X6C40);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5440);
	write_cmos_sensor_twobyte(0X6F12, 0X02AC);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X0ECC);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDEFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X05B0);
	write_cmos_sensor_twobyte(0X6F12, 0X5946);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF04F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA4BA);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X8246);
	write_cmos_sensor_twobyte(0X6F12, 0X1F48);
	write_cmos_sensor_twobyte(0X6F12, 0X8946);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X8169);
	write_cmos_sensor_twobyte(0X6F12, 0X1E46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0A9C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FA);
	write_cmos_sensor_twobyte(0X6F12, 0X1848);
	write_cmos_sensor_twobyte(0X6F12, 0X3346);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C40);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0X5046);
	write_cmos_sensor_twobyte(0X6F12, 0X0094);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC0FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X82BA);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0023);
	write_cmos_sensor_twobyte(0X6F12, 0X03E0);
	write_cmos_sensor_twobyte(0X6F12, 0X51F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2340);
	write_cmos_sensor_twobyte(0X6F12, 0X6040);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1C);
	write_cmos_sensor_twobyte(0X6F12, 0XB3EB);
	write_cmos_sensor_twobyte(0X6F12, 0X920F);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D3);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0948);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC069);
	write_cmos_sensor_twobyte(0X6F12, 0X87B2);
	write_cmos_sensor_twobyte(0X6F12, 0X4FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X1048);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X034C);
	write_cmos_sensor_twobyte(0X6F12, 0X3068);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A11);
	write_cmos_sensor_twobyte(0X6F12, 0XF1B3);
	write_cmos_sensor_twobyte(0X6F12, 0X19E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF4D2);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X8000);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X9000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF110);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5810);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X6AF0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XE304);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X7000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4B);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X51D1);
	write_cmos_sensor_twobyte(0X6F12, 0XFD48);
	write_cmos_sensor_twobyte(0X6F12, 0XFD49);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0XF0C3);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2FF);
	write_cmos_sensor_twobyte(0X6F12, 0XE066);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0XF949);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XABFF);
	write_cmos_sensor_twobyte(0X6F12, 0X2067);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XF749);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XA5FF);
	write_cmos_sensor_twobyte(0X6F12, 0X6067);
	write_cmos_sensor_twobyte(0X6F12, 0X3168);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XE36D);
	write_cmos_sensor_twobyte(0X6F12, 0XE26E);
	write_cmos_sensor_twobyte(0X6F12, 0X0126);
	write_cmos_sensor_twobyte(0X6F12, 0X9A42);
	write_cmos_sensor_twobyte(0X6F12, 0X01D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X60C0);
	write_cmos_sensor_twobyte(0X6F12, 0X236F);
	write_cmos_sensor_twobyte(0X6F12, 0X6345);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X020C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X64C0);
	write_cmos_sensor_twobyte(0X6F12, 0X6045);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X040C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XBCF1);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X07D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X5A40);
	write_cmos_sensor_twobyte(0X6F12, 0X4240);
	write_cmos_sensor_twobyte(0X6F12, 0XA266);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A51);
	write_cmos_sensor_twobyte(0X6F12, 0X0AE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B61);
	write_cmos_sensor_twobyte(0X6F12, 0XF6E7);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XA566);
	write_cmos_sensor_twobyte(0X6F12, 0XE566);
	write_cmos_sensor_twobyte(0X6F12, 0X2567);
	write_cmos_sensor_twobyte(0X6F12, 0X6567);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X27FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0XD348);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FA);
	write_cmos_sensor_twobyte(0X6F12, 0XCF48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A01);
	write_cmos_sensor_twobyte(0X6F12, 0X48B3);
	write_cmos_sensor_twobyte(0X6F12, 0XC348);
	write_cmos_sensor_twobyte(0X6F12, 0XC44B);
	write_cmos_sensor_twobyte(0X6F12, 0XC44A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X20D1);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0DFA);
	write_cmos_sensor_twobyte(0X6F12, 0XC148);
	write_cmos_sensor_twobyte(0X6F12, 0XC149);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XC04F);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFF);
	write_cmos_sensor_twobyte(0X6F12, 0XC14C);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0XBD49);
	write_cmos_sensor_twobyte(0X6F12, 0XE065);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X32FF);
	write_cmos_sensor_twobyte(0X6F12, 0X2066);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XBA49);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFF);
	write_cmos_sensor_twobyte(0X6F12, 0X6066);
	write_cmos_sensor_twobyte(0X6F12, 0XD4E9);
	write_cmos_sensor_twobyte(0X6F12, 0X1712);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X4140);
	write_cmos_sensor_twobyte(0X6F12, 0XA165);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA1B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XB148);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X406A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDBF9);
	write_cmos_sensor_twobyte(0X6F12, 0XE068);
	write_cmos_sensor_twobyte(0X6F12, 0X07F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8047);
	write_cmos_sensor_twobyte(0X6F12, 0X8069);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0XC288);
	write_cmos_sensor_twobyte(0X6F12, 0X0389);
	write_cmos_sensor_twobyte(0X6F12, 0XA1EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5201);
	write_cmos_sensor_twobyte(0X6F12, 0X827B);
	write_cmos_sensor_twobyte(0X6F12, 0X891A);
	write_cmos_sensor_twobyte(0X6F12, 0X8289);
	write_cmos_sensor_twobyte(0X6F12, 0XC07B);
	write_cmos_sensor_twobyte(0X6F12, 0XA2EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5302);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0XB980);
	write_cmos_sensor_twobyte(0X6F12, 0XE268);
	write_cmos_sensor_twobyte(0X6F12, 0X491E);
	write_cmos_sensor_twobyte(0X6F12, 0X9269);
	write_cmos_sensor_twobyte(0X6F12, 0X128A);
	write_cmos_sensor_twobyte(0X6F12, 0X1144);
	write_cmos_sensor_twobyte(0X6F12, 0XF980);
	write_cmos_sensor_twobyte(0X6F12, 0X3881);
	write_cmos_sensor_twobyte(0X6F12, 0XE168);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X8969);
	write_cmos_sensor_twobyte(0X6F12, 0X498A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X7881);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BB9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8146);
	write_cmos_sensor_twobyte(0X6F12, 0X9648);
	write_cmos_sensor_twobyte(0X6F12, 0X0D46);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X816A);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0E0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X5CF9);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X4846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA6F9);
	write_cmos_sensor_twobyte(0X6F12, 0X8E48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9201);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X15D1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2900);
	write_cmos_sensor_twobyte(0X6F12, 0X70B1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCE02);
	write_cmos_sensor_twobyte(0X6F12, 0X08B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6F12, 0X07E0);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XDA00);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X8748);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5C00);
	write_cmos_sensor_twobyte(0X6F12, 0X10B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4210);
	write_cmos_sensor_twobyte(0X6F12, 0X88F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1110);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X33B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1001);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X83F9);
	write_cmos_sensor_twobyte(0X6F12, 0X774D);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XF006);
	write_cmos_sensor_twobyte(0X6F12, 0X05F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3405);
	write_cmos_sensor_twobyte(0X6F12, 0X1ED5);
	write_cmos_sensor_twobyte(0X6F12, 0X7748);
	write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XB207);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X03D2);
	write_cmos_sensor_twobyte(0X6F12, 0X2146);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X76F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X1346);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0X39B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0F88);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7870);
	write_cmos_sensor_twobyte(0X6F12, 0XC08A);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0X01E0);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7830);
	write_cmos_sensor_twobyte(0X6F12, 0X521C);
	write_cmos_sensor_twobyte(0X6F12, 0X062A);
	write_cmos_sensor_twobyte(0X6F12, 0XEED3);
	write_cmos_sensor_twobyte(0X6F12, 0X7004);
	write_cmos_sensor_twobyte(0X6F12, 0X1AD5);
	write_cmos_sensor_twobyte(0X6F12, 0XB5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8400);
	write_cmos_sensor_twobyte(0X6F12, 0X411E);
	write_cmos_sensor_twobyte(0X6F12, 0XA142);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X04D2);
	write_cmos_sensor_twobyte(0X6F12, 0X44F4);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X54F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4002);
	write_cmos_sensor_twobyte(0X6F12, 0X5189);
	write_cmos_sensor_twobyte(0X6F12, 0X29B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7820);
	write_cmos_sensor_twobyte(0X6F12, 0X12B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0A80);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X0628);
	write_cmos_sensor_twobyte(0X6F12, 0XF2D3);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF081);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X5448);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC16A);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDAF8);
	write_cmos_sensor_twobyte(0X6F12, 0X43F6);
	write_cmos_sensor_twobyte(0X6F12, 0XEF73);
	write_cmos_sensor_twobyte(0X6F12, 0X0322);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XB901);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X36F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X38F9);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC8B8);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X484C);
	write_cmos_sensor_twobyte(0X6F12, 0XE38B);
	write_cmos_sensor_twobyte(0X6F12, 0XCB1A);
	write_cmos_sensor_twobyte(0X6F12, 0X218C);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X0144);
	write_cmos_sensor_twobyte(0X6F12, 0X9942);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X0346);
	write_cmos_sensor_twobyte(0X6F12, 0X4149);
	write_cmos_sensor_twobyte(0X6F12, 0X3431);
	write_cmos_sensor_twobyte(0X6F12, 0X9040);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8630);
	write_cmos_sensor_twobyte(0X6F12, 0X03FA);
	write_cmos_sensor_twobyte(0X6F12, 0X02F3);
	write_cmos_sensor_twobyte(0X6F12, 0X414A);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XD181);
	write_cmos_sensor_twobyte(0X6F12, 0X1382);
	write_cmos_sensor_twobyte(0X6F12, 0X03D0);
	write_cmos_sensor_twobyte(0X6F12, 0X5181);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X9381);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF70);
	write_cmos_sensor_twobyte(0X6F12, 0X5081);
	write_cmos_sensor_twobyte(0X6F12, 0X9081);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7B71);
	write_cmos_sensor_twobyte(0X6F12, 0X3948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0FF9);
	write_cmos_sensor_twobyte(0X6F12, 0X334C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5371);
	write_cmos_sensor_twobyte(0X6F12, 0X2060);
	write_cmos_sensor_twobyte(0X6F12, 0X3648);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X07F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XFF61);
	write_cmos_sensor_twobyte(0X6F12, 0X6060);
	write_cmos_sensor_twobyte(0X6F12, 0X3448);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X00F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XA060);
	write_cmos_sensor_twobyte(0X6F12, 0X3148);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2D61);
	write_cmos_sensor_twobyte(0X6F12, 0XE060);
	write_cmos_sensor_twobyte(0X6F12, 0X2F48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XF551);
	write_cmos_sensor_twobyte(0X6F12, 0X2061);
	write_cmos_sensor_twobyte(0X6F12, 0X2C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XEBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5341);
	write_cmos_sensor_twobyte(0X6F12, 0X6061);
	write_cmos_sensor_twobyte(0X6F12, 0X2A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XA061);
	write_cmos_sensor_twobyte(0X6F12, 0X2021);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X6000);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8006);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3400);
	write_cmos_sensor_twobyte(0X6F12, 0X7580);
	write_cmos_sensor_twobyte(0X6F12, 0X4562);
	write_cmos_sensor_twobyte(0X6F12, 0X8562);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C50);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5050);
	write_cmos_sensor_twobyte(0X6F12, 0X2A46);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7D21);
	write_cmos_sensor_twobyte(0X6F12, 0X4565);
	write_cmos_sensor_twobyte(0X6F12, 0X1E48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XCBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2121);
	write_cmos_sensor_twobyte(0X6F12, 0X6062);
	write_cmos_sensor_twobyte(0X6F12, 0X1C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X4541);
	write_cmos_sensor_twobyte(0X6F12, 0XA062);
	write_cmos_sensor_twobyte(0X6F12, 0X1948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XBDF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F31);
	write_cmos_sensor_twobyte(0X6F12, 0XE061);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0XABBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0345);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0X1234);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5423);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0XFECA);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X7C80);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0B27);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0E5D);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X5C51);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X1F65);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X98A9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X852F);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XDE2F);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X10B3);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0823);
	write_cmos_sensor_twobyte(0X6F12, 0X0A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X87F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X2062);
	write_cmos_sensor_twobyte(0X6F12, 0X0848);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X6F11);
	write_cmos_sensor_twobyte(0X6F12, 0XE062);
	write_cmos_sensor_twobyte(0X6F12, 0X0548);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X79F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2063);
	write_cmos_sensor_twobyte(0X6F12, 0XF562);
	write_cmos_sensor_twobyte(0X6F12, 0X3563);
	write_cmos_sensor_twobyte(0X6F12, 0X7563);
	write_cmos_sensor_twobyte(0X6F12, 0X70BD);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X339B);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X2ABB);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X135F);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0XE91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X273C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X5D6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X033C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X3B2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X45F6);
	write_cmos_sensor_twobyte(0X6F12, 0X514C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X657C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D0C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X471C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X49F6);
	write_cmos_sensor_twobyte(0X6F12, 0XA90C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X48F2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X230C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X43F2);
	write_cmos_sensor_twobyte(0X6F12, 0X9B3C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3B7C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X4DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X2F6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB30C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X871C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X6F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1B5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0XF91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X42F6);
	write_cmos_sensor_twobyte(0X6F12, 0XBB2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB71C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X602A, 0X1150);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ah 680mV
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ch 680mV
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF466);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X111C);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	// F46Ah 3.7V
	write_cmos_sensor_twobyte(0X602A, 0X1120);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	// F470h -1.1V
	write_cmos_sensor_twobyte(0X602A, 0X113E);
	write_cmos_sensor_twobyte(0X6F12, 0X2101);
	write_cmos_sensor_twobyte(0X602A, 0X1106);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A28);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A52);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// Short Green 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A48);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// Short Green 1:8 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// Short Green 1:4 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A34);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// Short Green 1:2 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A84);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// Short Red 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3AB6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// Short Blue 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3B44);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3B48);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X3B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0200);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0224);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0224h Address
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0226h Address
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X1124);
	write_cmos_sensor_twobyte(0X6F12, 0X0328);	// ADBUS data order
	write_cmos_sensor_twobyte(0X602A, 0X0ACE);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X602A, 0X09C0);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X09C2);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X6F12, 0X008C);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0XF408);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X54A2);
	write_cmos_sensor_twobyte(0X6F12, 0XF418);	// CDS_Option_L
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X000D);
	write_cmos_sensor_twobyte(0X6F12, 0XF472);	// CLP_Level
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF482);	// COMP_BIAS
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF484);	// COMP_BIAS
	write_cmos_sensor_twobyte(0X602A, 0X47F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FE);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);
	write_cmos_sensor_twobyte(0X602A, 0X1108);
	write_cmos_sensor_twobyte(0X6F12, 0X054C);	// F404h Def. 050Ch (Pxbst on & LP_VBLK_EN)
	write_cmos_sensor_twobyte(0X6F12, 0X0183);	// F406h Def. 0083h
	write_cmos_sensor_twobyte(0X6F12, 0X07FF);	// F408h Def. 07DFh (Dabx 0ff & Dsrx en)
	write_cmos_sensor_twobyte(0X602A, 0X1154);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);	// F418h LDB on x4 S3
	write_cmos_sensor_twobyte(0X6F12, 0X162F);	// F41Ah  // No Avg // CRSon
	write_cmos_sensor_twobyte(0X6F12, 0X4328);	// F41Ch
	write_cmos_sensor_twobyte(0X602A, 0X1126);
	write_cmos_sensor_twobyte(0X6F12, 0X0082);	// F4A6h  IPI signal dip control
	write_cmos_sensor_twobyte(0X602A, 0X114A);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_SenAnalog_rmp_option_top
	write_cmos_sensor_twobyte(0X602A, 0X114E);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_SenAnalog_rmp_option_bot
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF41E);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);	// DBUS option (Hybrid Gain)
	write_cmos_sensor_twobyte(0X602A, 0XF488);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// REF option
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X112A);
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
	write_cmos_sensor_twobyte(0X602A, 0X1128);
	write_cmos_sensor_twobyte(0X6F12, 0X001B);
	write_cmos_sensor_twobyte(0X602A, 0X10F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);
	write_cmos_sensor_twobyte(0X602A, 0X10F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);
	write_cmos_sensor_twobyte(0X602A, 0X10FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);
	write_cmos_sensor_twobyte(0X602A, 0X10FE);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF414);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// aig_latch_start
	write_cmos_sensor_twobyte(0X6F12, 0X0004);	// aig_latch_width
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X10C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X602A, 0X10B6);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X0B26);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X602A, 0X0B2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X602A, 0X0B36);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_sl_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0B52);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_sl_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0B56);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//aig_rx_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0B5A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_rx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B5E);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_tx_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0B62);
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_tx_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0B66);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);	//aig_fx_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0B6A);
	write_cmos_sensor_twobyte(0X6F12, 0X0140);	//aig_fx_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0B6E);
	write_cmos_sensor_twobyte(0X6F12, 0X00EC);	//aig_dsrx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B76);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);	//aig_srx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B7E);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dstx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B86);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_stx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B8E);
	write_cmos_sensor_twobyte(0X6F12, 0X0047);	//aig_sfx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B96);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dabx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_abx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0BAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0101 //aig_sr_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BAE);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026E //aig_sr_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0106 //aig_ss_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026C //aig_ss_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_s1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BBE);
	write_cmos_sensor_twobyte(0X6F12, 0X0156);	//aig_s1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BC2);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_s3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BC6);
	write_cmos_sensor_twobyte(0X6F12, 0X0057);	//aig_s3_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BCA);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_s4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BCE);
	write_cmos_sensor_twobyte(0X6F12, 0X004E);	//aig_s4_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BD2);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_s4dmy_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_s4dmy_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_clp_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BDE);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_clp_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BE2);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_pixbst_p0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BE6);
	write_cmos_sensor_twobyte(0X6F12, 0X0016);	//aig_pixbst_p0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BEA);
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_pixbst_p1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BEE);
	write_cmos_sensor_twobyte(0X6F12, 0X014E);	//aig_pixbst_p1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BF2);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BF6);
	write_cmos_sensor_twobyte(0X6F12, 0X006B);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_vsl_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BFE);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vsl_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C02);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_lp_hblk_cds_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C0A);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vload_smp_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C0E);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_smp_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C12);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);	//aig_vload_smpdmy_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C16);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_smpdmy_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C1A);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	//aig_vbgr_smp_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C1E);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vbgr_smp_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C22);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vref_smp_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C26);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vref_smp_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C2A);
	write_cmos_sensor_twobyte(0X6F12, 0X00D7);	//aig_cnt_en_p0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_cnt_en_p0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C32);
	write_cmos_sensor_twobyte(0X6F12, 0X01D6);	//aig_cnt_en_p1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C36);
	write_cmos_sensor_twobyte(0X6F12, 0X026A);	//aig_cnt_en_p1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C3A);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_enb_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_conv_enb_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C42);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_conv1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C46);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C4A);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);	//aig_conv2_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C52);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lsb_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C56);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lsb_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C62);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);	//aig_conv_lsb_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0C66);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_lsb_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C72);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_rst_div_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0C76);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_rst_div_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C7A);
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//aig_comp_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C7E);
	write_cmos_sensor_twobyte(0X6F12, 0X0104);	//aig_comp_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C82);
	write_cmos_sensor_twobyte(0X6F12, 0X01D4);	//aig_comp_en_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0C86);
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_comp_en_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C92);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);	//aig_rd_en_10_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C96);
	write_cmos_sensor_twobyte(0X6F12, 0X001C);	//aig_rd_en_10_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C9A);
	write_cmos_sensor_twobyte(0X6F12, 0X001E);	//aig_rd_en_9_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0038);	//aig_rd_en_9_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CA2);
	write_cmos_sensor_twobyte(0X6F12, 0X003A);	//aig_rd_en_8_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0054);	//aig_rd_en_8_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0076);	//aig_rd_en_7_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CAE);
	write_cmos_sensor_twobyte(0X6F12, 0X0090);	//aig_rd_en_7_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CB2);
	write_cmos_sensor_twobyte(0X6F12, 0X010B);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0127);	//aig_rd_en_5_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CBE);
	write_cmos_sensor_twobyte(0X6F12, 0X0141);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CC2);
	write_cmos_sensor_twobyte(0X6F12, 0X0143);	//aig_rd_en_4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CC6);
	write_cmos_sensor_twobyte(0X6F12, 0X015D);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CCA);
	write_cmos_sensor_twobyte(0X6F12, 0X015F);	//aig_rd_en_3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CCE);
	write_cmos_sensor_twobyte(0X6F12, 0X0179);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CD2);
	write_cmos_sensor_twobyte(0X6F12, 0X017B);	//aig_rd_en_2_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0195);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0197);	//aig_rd_en_1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CDE);
	write_cmos_sensor_twobyte(0X6F12, 0X01B1);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CE2);
	write_cmos_sensor_twobyte(0X6F12, 0X01B3);	//aig_rd_en_0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CE6);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CEE);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CF2);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0CF6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0CFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr4
	write_cmos_sensor_twobyte(0X602A, 0X0CFE);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr5
	write_cmos_sensor_twobyte(0X602A, 0X0D02);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr6
	write_cmos_sensor_twobyte(0X602A, 0X0D06);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr7
	write_cmos_sensor_twobyte(0X602A, 0X0D0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr8
	write_cmos_sensor_twobyte(0X602A, 0X0D0E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr9
	write_cmos_sensor_twobyte(0X602A, 0X0D12);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr10
	write_cmos_sensor_twobyte(0X602A, 0X0D16);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr11
	write_cmos_sensor_twobyte(0X602A, 0X0D22);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_en_offset_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0D26);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_en_offset_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D32);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lsb_offset_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0D36);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lsb_offset_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D3A);
	write_cmos_sensor_twobyte(0X6F12, 0X006F);	//aig_off_rst_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0270);	//aig_off_rst_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D42);
	write_cmos_sensor_twobyte(0X6F12, 0X0071);	//aig_rmp_rst_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0073);	//aig_rmp_rst_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D52);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);	//aig_rmp_rst_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0D56);
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rmp_rst_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D5A);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_rmp_rst_ptr4
	write_cmos_sensor_twobyte(0X602A, 0X0D5E);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_rmp_rst_ptr5
	write_cmos_sensor_twobyte(0X602A, 0X0D62);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);	//aig_rmp_mode_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D66);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);	//aig_rvs_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D6A);
	write_cmos_sensor_twobyte(0X6F12, 0X0264);	//aig_rvs_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D76);
	write_cmos_sensor_twobyte(0X6F12, 0X0014);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D7A);
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_optl0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D96);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);	//aig_optl1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D9A);
	write_cmos_sensor_twobyte(0X6F12, 0X0132);	//aig_optl1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_optl2_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0DBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_optl2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);	//aig_optl3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0E16);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_opth0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0E1A);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_opth0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0FA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0287);	//VIR_1_FINE_STX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FAA);
	write_cmos_sensor_twobyte(0X6F12, 0X003C);	//VIR_1_FINE_DSTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0124);	//VIR_2_FINE_DSTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FB6);
	write_cmos_sensor_twobyte(0X6F12, 0X00DF);	//VIR_MSO_PTR0_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0296);	//VIR_MIN_ADC_COLS_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0301);
	write_cmos_sensor_twobyte(0X602A, 0X0FEC);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0FD8);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0FDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);
	write_cmos_sensor_twobyte(0X602A, 0X104C);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);
	write_cmos_sensor_twobyte(0X602A, 0X106C);
	write_cmos_sensor_twobyte(0X6F12, 0X0607);
	write_cmos_sensor_twobyte(0X602A, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0003);
	write_cmos_sensor_twobyte(0X602A, 0X101C);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X101E);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1020);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1022);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1024);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1026);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1028);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X602A, 0X10AE);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X10B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X0B0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B52);
	write_cmos_sensor_twobyte(0X6F12, 0X0579);	//aig_sl_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BFE);
	write_cmos_sensor_twobyte(0X6F12, 0X0579);	//aig_vsl_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C0E);
	write_cmos_sensor_twobyte(0X6F12, 0X0579);	//aig_vload_smp_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C16);
	write_cmos_sensor_twobyte(0X6F12, 0X0579);	//aig_vload_smpdmy_ptr1
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3064);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0B02);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X0ABC);
	write_cmos_sensor_twobyte(0X6F12, 0X0207);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X070C);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X602A, 0X0110);
	write_cmos_sensor_twobyte(0X6F12, 0X1002);
	write_cmos_sensor_twobyte(0X602A, 0X0114);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X602A, 0X0408);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X602A, 0X0344);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X1F6F);
	write_cmos_sensor_twobyte(0X6F12, 0X0BC7);
	write_cmos_sensor_twobyte(0X6F12, 0X0Fb0);
	write_cmos_sensor_twobyte(0X6F12, 0X0Bc0);
	write_cmos_sensor_twobyte(0X602A, 0X0380);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0900);
	write_cmos_sensor_twobyte(0X6F12, 0X0221);
	write_cmos_sensor_twobyte(0X602A, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0404);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0304);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X030C);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0306);
	write_cmos_sensor_twobyte(0X6F12, 0X01F4);	// 500
	write_cmos_sensor_twobyte(0X602A, 0X0302);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X0005);
	write_cmos_sensor_twobyte(0X602A, 0X0312);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X030E);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6F12, 0X0153);
	write_cmos_sensor_twobyte(0X602A, 0X030A);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0308);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X602A, 0X0342);
	write_cmos_sensor_twobyte(0X6F12, 0X1118);
	write_cmos_sensor_twobyte(0X602A, 0X0340);
	write_cmos_sensor_twobyte(0X6F12, 0X17CD);
	write_cmos_sensor_twobyte(0X602A, 0X021E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X30A6);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X30A8);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X30CA);
	write_cmos_sensor_twobyte(0X6F12, 0X0031);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A70);
	write_cmos_sensor_twobyte(0X6F12, 0X6E33);
	write_cmos_sensor_twobyte(0X602A, 0X4814);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X4876);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X2412);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);	//4x4 msm
	write_cmos_sensor_twobyte(0X602A, 0X3BE8);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//gb
	write_cmos_sensor_twobyte(0X602A, 0X8724);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writes on STBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writes on STBY
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A30);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X30CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0702);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X228E);
	write_cmos_sensor_twobyte(0X6F12, 0X0245);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X602A, 0X8726);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X872A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X872C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X872E);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E0);	//aig_dummy_addr_dr_rb_e
	write_cmos_sensor_twobyte(0X6F12, 0XF4E2);	//aig_dummy_addr_dr_rb_o
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//settings for test
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//settings for test
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X4D70);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D80);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D90);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0B04);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X30A2);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X5400);
	write_cmos_sensor_twobyte(0X602A, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
#endif

	// Stream On
	write_cmos_sensor_twobyte(0x0100,0x0100);

	mDELAY(10);


}

#ifndef MARK_HDR
static void capture_setting_WDR(kal_uint16 currefps)
{
//$MIPI[Width:5632,Height:4224,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:5632,Height:4224,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]

	LOG_INF("Capture WDR(fps = %d)",currefps);

	////1. ADLC setting
	write_cmos_sensor_twobyte(0x6028,0x2000);
	write_cmos_sensor_twobyte(0x602A,0x168C);
	write_cmos_sensor_twobyte(0x6F12,0x0010);
	write_cmos_sensor_twobyte(0x6F12,0x0011);
	write_cmos_sensor_twobyte(0x6F12,0x0011);

	//2. Clock setting, related with ATOP remove
	write_cmos_sensor_twobyte(0x602A,0x0E24);
	write_cmos_sensor_twobyte(0x6F12,0x0101); /*for 24fps*/

	//3. ATOP Setting (Option)
	write_cmos_sensor_twobyte(0x602A,0x0E9C);
	write_cmos_sensor_twobyte(0x6F12,0x0084);	// RDV option

	//// CDS Current Setting
	write_cmos_sensor_twobyte(0x602A,0x0E46);
	write_cmos_sensor_twobyte(0x6F12,0x0301); // CDS current // EVT1.1 0429 lkh
	write_cmos_sensor_twobyte(0x602A,0x0E48);
	write_cmos_sensor_twobyte(0x6F12,0x0303); // CDS current
	write_cmos_sensor_twobyte(0x602A,0x0E4A);
	write_cmos_sensor_twobyte(0x6F12,0x0306); // CDS current
	write_cmos_sensor_twobyte(0x602A,0x0E4C);
	write_cmos_sensor_twobyte(0x6F12,0x0a0a); // Pixel Bias current
	write_cmos_sensor_twobyte(0x602A,0x0E4E);
	write_cmos_sensor_twobyte(0x6F12,0x0A0F); // Pixel Bias current
	write_cmos_sensor_twobyte(0x602A,0x0E50);
	write_cmos_sensor_twobyte(0x6F12,0x0707); // Pixel Boost current
	write_cmos_sensor_twobyte(0x602A,0x0E52);
	write_cmos_sensor_twobyte(0x6F12,0x0700); // Pixel Boost current

	// EVT1.1 TnP
	write_cmos_sensor_twobyte(0x6028,0x2001);
	write_cmos_sensor_twobyte(0x602A,0xAB00);
	write_cmos_sensor_twobyte(0x6F12,0x0000);


	//Af correction for 4x4 binning
	write_cmos_sensor_twobyte(0x6028,0x2000);
	write_cmos_sensor_twobyte(0x602A,0x14C8);
	write_cmos_sensor_twobyte(0x6F12,0x0010);
	write_cmos_sensor_twobyte(0x602A,0x14CA);
	write_cmos_sensor_twobyte(0x6F12,0x1004);
	write_cmos_sensor_twobyte(0x602A,0x14CC);
	write_cmos_sensor_twobyte(0x6F12,0x0406);
	write_cmos_sensor_twobyte(0x602A,0x14CE);
	write_cmos_sensor_twobyte(0x6F12,0x0506);
	write_cmos_sensor_twobyte(0x602A,0x14D0);
	write_cmos_sensor_twobyte(0x6F12,0x0906);
	write_cmos_sensor_twobyte(0x602A,0x14D2);
	write_cmos_sensor_twobyte(0x6F12,0x0906);
	write_cmos_sensor_twobyte(0x602A,0x14D4);
	write_cmos_sensor_twobyte(0x6F12,0x0D0A);
	write_cmos_sensor_twobyte(0x602A,0x14DE);
	write_cmos_sensor_twobyte(0x6F12,0x1BE4);
	write_cmos_sensor_twobyte(0x6F12,0xB14E);

	//MSM gain for 4x4 binning
	write_cmos_sensor_twobyte(0x602A,0x427A);
	write_cmos_sensor_twobyte(0x6F12,0x0440);
	write_cmos_sensor_twobyte(0x602A,0x430C);
	write_cmos_sensor_twobyte(0x6F12,0x1B20);
	write_cmos_sensor_twobyte(0x602A,0x430E);
	write_cmos_sensor_twobyte(0x6F12,0x2128);
	write_cmos_sensor_twobyte(0x602A,0x4310);
	write_cmos_sensor_twobyte(0x6F12,0x2900);
	write_cmos_sensor_twobyte(0x602A,0x4314);
	write_cmos_sensor_twobyte(0x6F12,0x0000);
	write_cmos_sensor_twobyte(0x602A,0x4316);
	write_cmos_sensor_twobyte(0x6F12,0x0000);
	write_cmos_sensor_twobyte(0x602A,0x4318);
	write_cmos_sensor_twobyte(0x6F12,0x0000);

	// AF
	write_cmos_sensor_twobyte(0x6028,0x4000);
	//
	write_cmos_sensor_twobyte(0x0B0E, 0x0100); /*for 24fps*/
	/*PD compensation(0x3068): 0: Turn on, 1: Turn off*/
	write_cmos_sensor_twobyte(0x3068, 0x0001);
	write_cmos_sensor_twobyte(0x0B08, 0x0000);
	write_cmos_sensor_twobyte(0x0B04, 0x0101);

	//PDAF on/off
	write_cmos_sensor_twobyte(0x0B0E,0x0000);
	write_cmos_sensor_twobyte(0x3068,0x0001);
	//BPC on/off
	write_cmos_sensor_twobyte(0x0B08,0x0000);
	write_cmos_sensor_twobyte(0x0B04,0x0101);
	//WDR on/off  ZHDR setting*/
	sensor_WDR_zhdr();
	//write_cmos_sensor_twobyte(0x0216, 0x0101); /*For WDR*/
	//write_cmos_sensor_twobyte(0x0218, 0x0101);
	//write_cmos_sensor_twobyte(0x021A, 0x0100);

	//write_cmos_sensor_twobyte(0x0202, 0x0400);
	write_cmos_sensor_twobyte(0x0200, 0x0001);
	write_cmos_sensor_twobyte(0x0086, 0x0200);
	write_cmos_sensor_twobyte(0x0204, 0x0020);
	write_cmos_sensor_twobyte(0x0344, 0x0010);/*for 24fps*/
	write_cmos_sensor_twobyte(0x0348, 0x160F);/*for 24fps*/
	write_cmos_sensor_twobyte(0x0346, 0x0010);
	write_cmos_sensor_twobyte(0x034A, 0x108f);/*for 24fps*/
	write_cmos_sensor_twobyte(0x034C, 0x1600);
	write_cmos_sensor_twobyte(0x034E, 0x1080);
	write_cmos_sensor_twobyte(0x0342, 0x1ACC);/*for 24fps*/
	write_cmos_sensor_twobyte(0x0340, 0x10E4);/*for 24fps*/
	write_cmos_sensor_twobyte(0x0900, 0x0111);
	write_cmos_sensor_twobyte(0x0380, 0x0001);
	write_cmos_sensor_twobyte(0x0382, 0x0001);
	write_cmos_sensor_twobyte(0x0384, 0x0001);
	write_cmos_sensor_twobyte(0x0386, 0x0001);

	write_cmos_sensor_twobyte(0x6028, 0x2000);
	write_cmos_sensor_twobyte(0x602A, 0x6944);/*For WDR*/
	write_cmos_sensor_twobyte(0x6F12, 0x0000);

	write_cmos_sensor_twobyte(0x602A, 0x06A4);
	write_cmos_sensor_twobyte(0x6F12, 0x0080);
	write_cmos_sensor_twobyte(0x602A, 0x06AC);
	write_cmos_sensor_twobyte(0x6F12, 0x0100);
	write_cmos_sensor_twobyte(0x602A, 0x06E0);
	write_cmos_sensor_twobyte(0x6F12, 0x0200);
	write_cmos_sensor_twobyte(0x602A, 0x06E4);
	write_cmos_sensor_twobyte(0x6F12, 0x1000);
	write_cmos_sensor_twobyte(0x6028, 0x4000);
	write_cmos_sensor_twobyte(0x0408, 0x0000);/*for 24fps*/
	write_cmos_sensor_twobyte(0x040A, 0x0000);
	write_cmos_sensor_twobyte(0x0136, 0x1800);
	write_cmos_sensor_twobyte(0x0304, 0x0005);
	write_cmos_sensor_twobyte(0x0306, 0x0173);
	write_cmos_sensor_twobyte(0x030C, 0x0000);
	write_cmos_sensor_twobyte(0x0302, 0x0001);
	write_cmos_sensor_twobyte(0x0300, 0x0005);
	write_cmos_sensor_twobyte(0x030A, 0x0001);
	write_cmos_sensor_twobyte(0x0308, 0x000A);
	write_cmos_sensor_twobyte(0x0318, 0x0003);
	write_cmos_sensor_twobyte(0x031A, 0x00A4);
	write_cmos_sensor_twobyte(0x031C, 0x0001);
	write_cmos_sensor_twobyte(0x0316, 0x0001);
	write_cmos_sensor_twobyte(0x0314, 0x0003);
	write_cmos_sensor_twobyte(0x030E, 0x0004);
	write_cmos_sensor_twobyte(0x0310, 0x0153);/*for 24fps*/
	write_cmos_sensor_twobyte(0x0312, 0x0000);
	write_cmos_sensor_twobyte(0x0110, 0x0002);
	write_cmos_sensor_twobyte(0x0114, 0x0300);
	write_cmos_sensor_twobyte(0x0112, 0x0A0A);
	write_cmos_sensor_twobyte(0xB0CA, 0x7E00);
	write_cmos_sensor_twobyte(0xB136, 0x2000);
	write_cmos_sensor_twobyte(0xD0D0, 0x1000);
	//Gain issue fixed
	write_cmos_sensor_twobyte(0x6028,0x4000);
	write_cmos_sensor_twobyte(0x602A,0x0086);
	write_cmos_sensor_twobyte(0x6F12,0x0200);
	write_cmos_sensor_twobyte(0x6028,0x2000);
	write_cmos_sensor_twobyte(0x602A,0x06A4);
	write_cmos_sensor_twobyte(0x6F12,0x0400);

	/*Streaming  output */
	write_cmos_sensor_twobyte(0x0100, 0x0100);

}
#endif

static void capture_setting(void)
{
	LOG_INF("Dual PD capture");
	// Stream Off
	write_cmos_sensor(0x0100,0x00);

//no bin
#ifdef SENSOR_M1

#ifdef NEW_2L7_SETTING // 30fps setting for 2L7
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6010);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	mDELAY(3);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X7970);
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X7150);
	write_cmos_sensor_twobyte(0X602A, 0X0136);
	write_cmos_sensor_twobyte(0X6F12, 0X1800);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X7D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0449);
	write_cmos_sensor_twobyte(0X6F12, 0X0348);
	write_cmos_sensor_twobyte(0X6F12, 0X044A);
	write_cmos_sensor_twobyte(0X6F12, 0X4860);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0881);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC2BB);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X87AC);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5830);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XA3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0168);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X57FC);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X59FC);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0XC28E);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB1);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0X0280);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X48BC);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0XF148);
	write_cmos_sensor_twobyte(0X6F12, 0X8A46);
	write_cmos_sensor_twobyte(0X6F12, 0X9146);
	write_cmos_sensor_twobyte(0X6F12, 0X4168);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X39FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0027);
	write_cmos_sensor_twobyte(0X6F12, 0XA780);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X5146);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFC);
	write_cmos_sensor_twobyte(0X6F12, 0X98F8);
	write_cmos_sensor_twobyte(0X6F12, 0XE200);
	write_cmos_sensor_twobyte(0X6F12, 0XE64A);
	write_cmos_sensor_twobyte(0X6F12, 0XE749);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD9);
	write_cmos_sensor_twobyte(0X6F12, 0XE648);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6C09);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF087);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XF8E7);
	write_cmos_sensor_twobyte(0X6F12, 0XFEB5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XD848);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X8168);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X17FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X03FC);
	write_cmos_sensor_twobyte(0X6F12, 0XD048);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X0029);
	write_cmos_sensor_twobyte(0X6F12, 0X1CD0);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XD900);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0XCB48);
	write_cmos_sensor_twobyte(0X6F12, 0X0292);
	write_cmos_sensor_twobyte(0X6F12, 0X0192);
	write_cmos_sensor_twobyte(0X6F12, 0X4A30);
	write_cmos_sensor_twobyte(0X6F12, 0XCA00);
	write_cmos_sensor_twobyte(0X6F12, 0XC17D);
	write_cmos_sensor_twobyte(0X6F12, 0XCD4C);
	write_cmos_sensor_twobyte(0X6F12, 0XC91D);
	write_cmos_sensor_twobyte(0X6F12, 0XC908);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0123);
	write_cmos_sensor_twobyte(0X6F12, 0X43EA);
	write_cmos_sensor_twobyte(0X6F12, 0X4101);
	write_cmos_sensor_twobyte(0X6F12, 0X6180);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X6B46);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF2);
	write_cmos_sensor_twobyte(0X6F12, 0X1010);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF7FB);
	write_cmos_sensor_twobyte(0X6F12, 0X0C20);
	write_cmos_sensor_twobyte(0X6F12, 0X2080);
	write_cmos_sensor_twobyte(0X6F12, 0XFEBD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC068);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD3FB);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE8FB);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0XC18A);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0268);
	write_cmos_sensor_twobyte(0X6F12, 0X02F5);
	write_cmos_sensor_twobyte(0X6F12, 0XC972);
	write_cmos_sensor_twobyte(0X6F12, 0X907A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0XBB49);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0XB4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF600);
	write_cmos_sensor_twobyte(0X6F12, 0X917A);
	write_cmos_sensor_twobyte(0X6F12, 0X411A);
	write_cmos_sensor_twobyte(0X6F12, 0XB948);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X34F8);
	write_cmos_sensor_twobyte(0X6F12, 0XFA1F);
	write_cmos_sensor_twobyte(0X6F12, 0X638A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X937A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X8180);
	write_cmos_sensor_twobyte(0X6F12, 0X1078);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD1);
	write_cmos_sensor_twobyte(0X6F12, 0XAE48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1115);
	write_cmos_sensor_twobyte(0X6F12, 0XB248);
	write_cmos_sensor_twobyte(0X6F12, 0X19B1);
	write_cmos_sensor_twobyte(0X6F12, 0XA749);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5D10);
	write_cmos_sensor_twobyte(0X6F12, 0X491C);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCC10);
	write_cmos_sensor_twobyte(0X6F12, 0XA348);
	write_cmos_sensor_twobyte(0X6F12, 0XB830);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X4188);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X9FBB);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0X9D48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0169);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XAFFB);
	write_cmos_sensor_twobyte(0X6F12, 0X9C48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1202);
	write_cmos_sensor_twobyte(0X6F12, 0X40B1);
	write_cmos_sensor_twobyte(0X6F12, 0X9D49);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X0968);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X7DBB);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4F);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0X8B48);
	write_cmos_sensor_twobyte(0X6F12, 0X81B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X4069);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X80B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0091);
	write_cmos_sensor_twobyte(0X6F12, 0X8346);
	write_cmos_sensor_twobyte(0X6F12, 0X0146);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6EFB);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0X1492);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF0);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X09F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3409);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X28A0);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X24A0);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X50A0);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0600);
	write_cmos_sensor_twobyte(0X6F12, 0X0028);
	write_cmos_sensor_twobyte(0X6F12, 0X76D0);
	write_cmos_sensor_twobyte(0X6F12, 0X8948);
	write_cmos_sensor_twobyte(0X6F12, 0X0026);
	write_cmos_sensor_twobyte(0X6F12, 0X3746);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D02);
	write_cmos_sensor_twobyte(0X6F12, 0XD8B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0024);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8405);
	write_cmos_sensor_twobyte(0X6F12, 0X296E);
	write_cmos_sensor_twobyte(0X6F12, 0X81B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0A22);
	write_cmos_sensor_twobyte(0X6F12, 0X286A);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X72FB);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4011);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XC021);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF001);
	write_cmos_sensor_twobyte(0X6F12, 0X5143);
	write_cmos_sensor_twobyte(0X6F12, 0X4031);
	write_cmos_sensor_twobyte(0X6F12, 0X00EB);
	write_cmos_sensor_twobyte(0X6F12, 0XE110);
	write_cmos_sensor_twobyte(0X6F12, 0X0644);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X641C);
	write_cmos_sensor_twobyte(0X6F12, 0X0744);
	write_cmos_sensor_twobyte(0X6F12, 0X102C);
	write_cmos_sensor_twobyte(0X6F12, 0XE4D3);
	write_cmos_sensor_twobyte(0X6F12, 0XF817);
	write_cmos_sensor_twobyte(0X6F12, 0X07EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0511);
	write_cmos_sensor_twobyte(0X6F12, 0XF017);
	write_cmos_sensor_twobyte(0X6F12, 0X06EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0411);
	write_cmos_sensor_twobyte(0X6F12, 0X99F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0700);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X0428);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X654E);
	write_cmos_sensor_twobyte(0X6F12, 0XD071);
	write_cmos_sensor_twobyte(0X6F12, 0X8236);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X2ADD);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X08D0);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X01EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X8B6A);
	write_cmos_sensor_twobyte(0X6F12, 0XCB62);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C00);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0XC462);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X92F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0730);
	write_cmos_sensor_twobyte(0X6F12, 0X8857);
	write_cmos_sensor_twobyte(0X6F12, 0X9842);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X8855);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C30);
	write_cmos_sensor_twobyte(0X6F12, 0X9957);
	write_cmos_sensor_twobyte(0X6F12, 0X0129);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDD);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0313);
	write_cmos_sensor_twobyte(0X6F12, 0X04E0);
	write_cmos_sensor_twobyte(0X6F12, 0X03EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8007);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XFF6A);
	write_cmos_sensor_twobyte(0X6F12, 0X3C44);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D8);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0XF1F4);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0800);
	write_cmos_sensor_twobyte(0X6F12, 0X50B3);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X14DB);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X10DB);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X1BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FB);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X21DB);
	write_cmos_sensor_twobyte(0X6F12, 0X22E0);
	write_cmos_sensor_twobyte(0X6F12, 0X34E0);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X11DC);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XFEFA);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0XAC42);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X6F12, 0X601B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X281B);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X18B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C20);
	write_cmos_sensor_twobyte(0X6F12, 0X02F8);
	write_cmos_sensor_twobyte(0X6F12, 0X06A0);
	write_cmos_sensor_twobyte(0X6F12, 0X0299);
	write_cmos_sensor_twobyte(0X6F12, 0X8968);
	write_cmos_sensor_twobyte(0X6F12, 0X4A78);
	write_cmos_sensor_twobyte(0X6F12, 0X0243);
	write_cmos_sensor_twobyte(0X6F12, 0X4A70);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0XC1E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0954);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5000);
	write_cmos_sensor_twobyte(0X6F12, 0X6C40);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5440);
	write_cmos_sensor_twobyte(0X6F12, 0X02AC);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X0ECC);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDEFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X05B0);
	write_cmos_sensor_twobyte(0X6F12, 0X5946);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF04F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA4BA);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X8246);
	write_cmos_sensor_twobyte(0X6F12, 0X1F48);
	write_cmos_sensor_twobyte(0X6F12, 0X8946);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X8169);
	write_cmos_sensor_twobyte(0X6F12, 0X1E46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0A9C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FA);
	write_cmos_sensor_twobyte(0X6F12, 0X1848);
	write_cmos_sensor_twobyte(0X6F12, 0X3346);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C40);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0X5046);
	write_cmos_sensor_twobyte(0X6F12, 0X0094);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC0FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X82BA);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0023);
	write_cmos_sensor_twobyte(0X6F12, 0X03E0);
	write_cmos_sensor_twobyte(0X6F12, 0X51F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2340);
	write_cmos_sensor_twobyte(0X6F12, 0X6040);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1C);
	write_cmos_sensor_twobyte(0X6F12, 0XB3EB);
	write_cmos_sensor_twobyte(0X6F12, 0X920F);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D3);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0948);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC069);
	write_cmos_sensor_twobyte(0X6F12, 0X87B2);
	write_cmos_sensor_twobyte(0X6F12, 0X4FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X1048);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X034C);
	write_cmos_sensor_twobyte(0X6F12, 0X3068);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A11);
	write_cmos_sensor_twobyte(0X6F12, 0XF1B3);
	write_cmos_sensor_twobyte(0X6F12, 0X19E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF4D2);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X8000);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X9000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF110);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5810);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X6AF0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XE304);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X7000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4B);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X51D1);
	write_cmos_sensor_twobyte(0X6F12, 0XFD48);
	write_cmos_sensor_twobyte(0X6F12, 0XFD49);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0XF0C3);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2FF);
	write_cmos_sensor_twobyte(0X6F12, 0XE066);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0XF949);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XABFF);
	write_cmos_sensor_twobyte(0X6F12, 0X2067);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XF749);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XA5FF);
	write_cmos_sensor_twobyte(0X6F12, 0X6067);
	write_cmos_sensor_twobyte(0X6F12, 0X3168);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XE36D);
	write_cmos_sensor_twobyte(0X6F12, 0XE26E);
	write_cmos_sensor_twobyte(0X6F12, 0X0126);
	write_cmos_sensor_twobyte(0X6F12, 0X9A42);
	write_cmos_sensor_twobyte(0X6F12, 0X01D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X60C0);
	write_cmos_sensor_twobyte(0X6F12, 0X236F);
	write_cmos_sensor_twobyte(0X6F12, 0X6345);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X020C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X64C0);
	write_cmos_sensor_twobyte(0X6F12, 0X6045);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X040C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XBCF1);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X07D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X5A40);
	write_cmos_sensor_twobyte(0X6F12, 0X4240);
	write_cmos_sensor_twobyte(0X6F12, 0XA266);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A51);
	write_cmos_sensor_twobyte(0X6F12, 0X0AE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B61);
	write_cmos_sensor_twobyte(0X6F12, 0XF6E7);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XA566);
	write_cmos_sensor_twobyte(0X6F12, 0XE566);
	write_cmos_sensor_twobyte(0X6F12, 0X2567);
	write_cmos_sensor_twobyte(0X6F12, 0X6567);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X27FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0XD348);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FA);
	write_cmos_sensor_twobyte(0X6F12, 0XCF48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A01);
	write_cmos_sensor_twobyte(0X6F12, 0X48B3);
	write_cmos_sensor_twobyte(0X6F12, 0XC348);
	write_cmos_sensor_twobyte(0X6F12, 0XC44B);
	write_cmos_sensor_twobyte(0X6F12, 0XC44A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X20D1);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0DFA);
	write_cmos_sensor_twobyte(0X6F12, 0XC148);
	write_cmos_sensor_twobyte(0X6F12, 0XC149);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XC04F);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFF);
	write_cmos_sensor_twobyte(0X6F12, 0XC14C);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0XBD49);
	write_cmos_sensor_twobyte(0X6F12, 0XE065);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X32FF);
	write_cmos_sensor_twobyte(0X6F12, 0X2066);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XBA49);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFF);
	write_cmos_sensor_twobyte(0X6F12, 0X6066);
	write_cmos_sensor_twobyte(0X6F12, 0XD4E9);
	write_cmos_sensor_twobyte(0X6F12, 0X1712);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X4140);
	write_cmos_sensor_twobyte(0X6F12, 0XA165);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA1B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XB148);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X406A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDBF9);
	write_cmos_sensor_twobyte(0X6F12, 0XE068);
	write_cmos_sensor_twobyte(0X6F12, 0X07F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8047);
	write_cmos_sensor_twobyte(0X6F12, 0X8069);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0XC288);
	write_cmos_sensor_twobyte(0X6F12, 0X0389);
	write_cmos_sensor_twobyte(0X6F12, 0XA1EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5201);
	write_cmos_sensor_twobyte(0X6F12, 0X827B);
	write_cmos_sensor_twobyte(0X6F12, 0X891A);
	write_cmos_sensor_twobyte(0X6F12, 0X8289);
	write_cmos_sensor_twobyte(0X6F12, 0XC07B);
	write_cmos_sensor_twobyte(0X6F12, 0XA2EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5302);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0XB980);
	write_cmos_sensor_twobyte(0X6F12, 0XE268);
	write_cmos_sensor_twobyte(0X6F12, 0X491E);
	write_cmos_sensor_twobyte(0X6F12, 0X9269);
	write_cmos_sensor_twobyte(0X6F12, 0X128A);
	write_cmos_sensor_twobyte(0X6F12, 0X1144);
	write_cmos_sensor_twobyte(0X6F12, 0XF980);
	write_cmos_sensor_twobyte(0X6F12, 0X3881);
	write_cmos_sensor_twobyte(0X6F12, 0XE168);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X8969);
	write_cmos_sensor_twobyte(0X6F12, 0X498A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X7881);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BB9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8146);
	write_cmos_sensor_twobyte(0X6F12, 0X9648);
	write_cmos_sensor_twobyte(0X6F12, 0X0D46);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X816A);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0E0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X5CF9);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X4846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA6F9);
	write_cmos_sensor_twobyte(0X6F12, 0X8E48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9201);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X15D1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2900);
	write_cmos_sensor_twobyte(0X6F12, 0X70B1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCE02);
	write_cmos_sensor_twobyte(0X6F12, 0X08B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6F12, 0X07E0);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XDA00);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X8748);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5C00);
	write_cmos_sensor_twobyte(0X6F12, 0X10B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4210);
	write_cmos_sensor_twobyte(0X6F12, 0X88F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1110);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X33B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1001);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X83F9);
	write_cmos_sensor_twobyte(0X6F12, 0X774D);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XF006);
	write_cmos_sensor_twobyte(0X6F12, 0X05F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3405);
	write_cmos_sensor_twobyte(0X6F12, 0X1ED5);
	write_cmos_sensor_twobyte(0X6F12, 0X7748);
	write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XB207);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X03D2);
	write_cmos_sensor_twobyte(0X6F12, 0X2146);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X76F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X1346);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0X39B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0F88);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7870);
	write_cmos_sensor_twobyte(0X6F12, 0XC08A);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0X01E0);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7830);
	write_cmos_sensor_twobyte(0X6F12, 0X521C);
	write_cmos_sensor_twobyte(0X6F12, 0X062A);
	write_cmos_sensor_twobyte(0X6F12, 0XEED3);
	write_cmos_sensor_twobyte(0X6F12, 0X7004);
	write_cmos_sensor_twobyte(0X6F12, 0X1AD5);
	write_cmos_sensor_twobyte(0X6F12, 0XB5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8400);
	write_cmos_sensor_twobyte(0X6F12, 0X411E);
	write_cmos_sensor_twobyte(0X6F12, 0XA142);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X04D2);
	write_cmos_sensor_twobyte(0X6F12, 0X44F4);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X54F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4002);
	write_cmos_sensor_twobyte(0X6F12, 0X5189);
	write_cmos_sensor_twobyte(0X6F12, 0X29B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7820);
	write_cmos_sensor_twobyte(0X6F12, 0X12B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0A80);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X0628);
	write_cmos_sensor_twobyte(0X6F12, 0XF2D3);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF081);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X5448);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC16A);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDAF8);
	write_cmos_sensor_twobyte(0X6F12, 0X43F6);
	write_cmos_sensor_twobyte(0X6F12, 0XEF73);
	write_cmos_sensor_twobyte(0X6F12, 0X0322);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XB901);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X36F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X38F9);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC8B8);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X484C);
	write_cmos_sensor_twobyte(0X6F12, 0XE38B);
	write_cmos_sensor_twobyte(0X6F12, 0XCB1A);
	write_cmos_sensor_twobyte(0X6F12, 0X218C);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X0144);
	write_cmos_sensor_twobyte(0X6F12, 0X9942);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X0346);
	write_cmos_sensor_twobyte(0X6F12, 0X4149);
	write_cmos_sensor_twobyte(0X6F12, 0X3431);
	write_cmos_sensor_twobyte(0X6F12, 0X9040);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8630);
	write_cmos_sensor_twobyte(0X6F12, 0X03FA);
	write_cmos_sensor_twobyte(0X6F12, 0X02F3);
	write_cmos_sensor_twobyte(0X6F12, 0X414A);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XD181);
	write_cmos_sensor_twobyte(0X6F12, 0X1382);
	write_cmos_sensor_twobyte(0X6F12, 0X03D0);
	write_cmos_sensor_twobyte(0X6F12, 0X5181);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X9381);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF70);
	write_cmos_sensor_twobyte(0X6F12, 0X5081);
	write_cmos_sensor_twobyte(0X6F12, 0X9081);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7B71);
	write_cmos_sensor_twobyte(0X6F12, 0X3948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0FF9);
	write_cmos_sensor_twobyte(0X6F12, 0X334C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5371);
	write_cmos_sensor_twobyte(0X6F12, 0X2060);
	write_cmos_sensor_twobyte(0X6F12, 0X3648);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X07F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XFF61);
	write_cmos_sensor_twobyte(0X6F12, 0X6060);
	write_cmos_sensor_twobyte(0X6F12, 0X3448);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X00F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XA060);
	write_cmos_sensor_twobyte(0X6F12, 0X3148);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2D61);
	write_cmos_sensor_twobyte(0X6F12, 0XE060);
	write_cmos_sensor_twobyte(0X6F12, 0X2F48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XF551);
	write_cmos_sensor_twobyte(0X6F12, 0X2061);
	write_cmos_sensor_twobyte(0X6F12, 0X2C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XEBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5341);
	write_cmos_sensor_twobyte(0X6F12, 0X6061);
	write_cmos_sensor_twobyte(0X6F12, 0X2A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XA061);
	write_cmos_sensor_twobyte(0X6F12, 0X2021);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X6000);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8006);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3400);
	write_cmos_sensor_twobyte(0X6F12, 0X7580);
	write_cmos_sensor_twobyte(0X6F12, 0X4562);
	write_cmos_sensor_twobyte(0X6F12, 0X8562);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C50);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5050);
	write_cmos_sensor_twobyte(0X6F12, 0X2A46);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7D21);
	write_cmos_sensor_twobyte(0X6F12, 0X4565);
	write_cmos_sensor_twobyte(0X6F12, 0X1E48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XCBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2121);
	write_cmos_sensor_twobyte(0X6F12, 0X6062);
	write_cmos_sensor_twobyte(0X6F12, 0X1C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X4541);
	write_cmos_sensor_twobyte(0X6F12, 0XA062);
	write_cmos_sensor_twobyte(0X6F12, 0X1948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XBDF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F31);
	write_cmos_sensor_twobyte(0X6F12, 0XE061);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0XABBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0345);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0X1234);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5423);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0XFECA);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X7C80);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0B27);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0E5D);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X5C51);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X1F65);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X98A9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X852F);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XDE2F);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X10B3);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0823);
	write_cmos_sensor_twobyte(0X6F12, 0X0A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X87F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X2062);
	write_cmos_sensor_twobyte(0X6F12, 0X0848);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X6F11);
	write_cmos_sensor_twobyte(0X6F12, 0XE062);
	write_cmos_sensor_twobyte(0X6F12, 0X0548);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X79F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2063);
	write_cmos_sensor_twobyte(0X6F12, 0XF562);
	write_cmos_sensor_twobyte(0X6F12, 0X3563);
	write_cmos_sensor_twobyte(0X6F12, 0X7563);
	write_cmos_sensor_twobyte(0X6F12, 0X70BD);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X339B);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X2ABB);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X135F);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0XE91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X273C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X5D6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X033C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X3B2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X45F6);
	write_cmos_sensor_twobyte(0X6F12, 0X514C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X657C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D0C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X471C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X49F6);
	write_cmos_sensor_twobyte(0X6F12, 0XA90C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X48F2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X230C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X43F2);
	write_cmos_sensor_twobyte(0X6F12, 0X9B3C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3B7C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X4DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X2F6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB30C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X871C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X6F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1B5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0XF91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X42F6);
	write_cmos_sensor_twobyte(0X6F12, 0XBB2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB71C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X602A, 0X1150);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF466);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X111C);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X1120);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X602A, 0X113E);
	write_cmos_sensor_twobyte(0X6F12, 0X2101);
	write_cmos_sensor_twobyte(0X602A, 0X1106);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A28);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A52);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A48);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A34);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A84);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
	write_cmos_sensor_twobyte(0X602A, 0X3AB6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);
	write_cmos_sensor_twobyte(0X602A, 0X3B44);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3B48);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X3B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0200);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0224);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X1124);
	write_cmos_sensor_twobyte(0X6F12, 0X0328);
	write_cmos_sensor_twobyte(0X602A, 0X0ACE);
	write_cmos_sensor_twobyte(0X6F12, 0X0030);
	write_cmos_sensor_twobyte(0X602A, 0X09C0);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X09C2);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X6F12, 0X008C);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0XF408);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X54A2);
	write_cmos_sensor_twobyte(0X6F12, 0XF418);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X000D);
	write_cmos_sensor_twobyte(0X6F12, 0XF472);
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF482);
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF484);
	write_cmos_sensor_twobyte(0X602A, 0X47F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FE);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);
	write_cmos_sensor_twobyte(0X602A, 0X1108);
	write_cmos_sensor_twobyte(0X6F12, 0X054C);
	write_cmos_sensor_twobyte(0X6F12, 0X0183);
	write_cmos_sensor_twobyte(0X6F12, 0X07FF);
	write_cmos_sensor_twobyte(0X602A, 0X1154);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X162F);
	write_cmos_sensor_twobyte(0X6F12, 0X4328);
	write_cmos_sensor_twobyte(0X602A, 0X1126);
	write_cmos_sensor_twobyte(0X6F12, 0X00A2);
	write_cmos_sensor_twobyte(0X602A, 0X114A);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);
	write_cmos_sensor_twobyte(0X602A, 0X114E);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF41E);
	write_cmos_sensor_twobyte(0X6F12, 0X2100);
	write_cmos_sensor_twobyte(0X602A, 0XF488);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X112A);
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
	write_cmos_sensor_twobyte(0X602A, 0X1128);
	write_cmos_sensor_twobyte(0X6F12, 0X001B);
	write_cmos_sensor_twobyte(0X602A, 0X10F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);
	write_cmos_sensor_twobyte(0X602A, 0X10F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);
	write_cmos_sensor_twobyte(0X602A, 0X10FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);
	write_cmos_sensor_twobyte(0X602A, 0X10FE);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF414);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X10C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X602A, 0X10B6);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X0B26);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);
	write_cmos_sensor_twobyte(0X602A, 0X0B2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X602A, 0X0B36);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);
	write_cmos_sensor_twobyte(0X6F12, 0X00B7);
	write_cmos_sensor_twobyte(0X6F12, 0X013E);
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);
	write_cmos_sensor_twobyte(0X6F12, 0X0140);
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);
	write_cmos_sensor_twobyte(0X6F12, 0X00EC);
	write_cmos_sensor_twobyte(0X6F12, 0X00A6);
	write_cmos_sensor_twobyte(0X602A, 0X0B76);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);
	write_cmos_sensor_twobyte(0X602A, 0X0B7E);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);
	write_cmos_sensor_twobyte(0X6F12, 0X009F);
	write_cmos_sensor_twobyte(0X602A, 0X0B86);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);
	write_cmos_sensor_twobyte(0X602A, 0X0B8E);
	write_cmos_sensor_twobyte(0X6F12, 0X0047);
	write_cmos_sensor_twobyte(0X6F12, 0X0048);
	write_cmos_sensor_twobyte(0X602A, 0X0B94);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);
	write_cmos_sensor_twobyte(0X6F12, 0X009F);
	write_cmos_sensor_twobyte(0X602A, 0X0B9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);
	write_cmos_sensor_twobyte(0X602A, 0X0BAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0156);
	write_cmos_sensor_twobyte(0X6F12, 0X00FF);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0057);
	write_cmos_sensor_twobyte(0X6F12, 0X0057);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X004E);
	write_cmos_sensor_twobyte(0X6F12, 0X004E);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0055);
	write_cmos_sensor_twobyte(0X6F12, 0X0055);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X0016);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X013E);
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);
	write_cmos_sensor_twobyte(0X6F12, 0X014E);
	write_cmos_sensor_twobyte(0X6F12, 0X00FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X006B);
	write_cmos_sensor_twobyte(0X6F12, 0X006B);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X602A, 0X0C0A);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X6F12, 0X00D7);
	write_cmos_sensor_twobyte(0X6F12, 0X0087);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);
	write_cmos_sensor_twobyte(0X6F12, 0X01D6);
	write_cmos_sensor_twobyte(0X6F12, 0X0134);
	write_cmos_sensor_twobyte(0X6F12, 0X026A);
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0119);
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X00C2);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);
	write_cmos_sensor_twobyte(0X602A, 0X0C62);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);
	write_cmos_sensor_twobyte(0X6F12, 0X00C0);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X602A, 0X0C72);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);
	write_cmos_sensor_twobyte(0X6F12, 0X0085);
	write_cmos_sensor_twobyte(0X6F12, 0X0104);
	write_cmos_sensor_twobyte(0X6F12, 0X00B5);
	write_cmos_sensor_twobyte(0X6F12, 0X01D4);
	write_cmos_sensor_twobyte(0X6F12, 0X0132);
	write_cmos_sensor_twobyte(0X6F12, 0X026C);
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);
	write_cmos_sensor_twobyte(0X602A, 0X0C92);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X6F12, 0X001C);
	write_cmos_sensor_twobyte(0X6F12, 0X0016);
	write_cmos_sensor_twobyte(0X6F12, 0X001E);
	write_cmos_sensor_twobyte(0X6F12, 0X0017);
	write_cmos_sensor_twobyte(0X6F12, 0X0038);
	write_cmos_sensor_twobyte(0X6F12, 0X002B);
	write_cmos_sensor_twobyte(0X6F12, 0X003A);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X0054);
	write_cmos_sensor_twobyte(0X6F12, 0X0040);
	write_cmos_sensor_twobyte(0X6F12, 0X0076);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X0090);
	write_cmos_sensor_twobyte(0X6F12, 0X0055);
	write_cmos_sensor_twobyte(0X6F12, 0X010B);
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0125);
	write_cmos_sensor_twobyte(0X6F12, 0X00C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0127);
	write_cmos_sensor_twobyte(0X6F12, 0X00C7);
	write_cmos_sensor_twobyte(0X6F12, 0X0141);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0143);
	write_cmos_sensor_twobyte(0X6F12, 0X00DC);
	write_cmos_sensor_twobyte(0X6F12, 0X015D);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X015F);
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);
	write_cmos_sensor_twobyte(0X6F12, 0X0179);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X6F12, 0X017B);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);
	write_cmos_sensor_twobyte(0X6F12, 0X0195);
	write_cmos_sensor_twobyte(0X6F12, 0X011A);
	write_cmos_sensor_twobyte(0X6F12, 0X0197);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);
	write_cmos_sensor_twobyte(0X6F12, 0X01B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01B4);
	write_cmos_sensor_twobyte(0X6F12, 0X01B3);
	write_cmos_sensor_twobyte(0X6F12, 0X01B5);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0D22);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X602A, 0X0D32);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);
	write_cmos_sensor_twobyte(0X6F12, 0X006F);
	write_cmos_sensor_twobyte(0X6F12, 0X006F);
	write_cmos_sensor_twobyte(0X6F12, 0X0270);
	write_cmos_sensor_twobyte(0X6F12, 0X01CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0071);
	write_cmos_sensor_twobyte(0X6F12, 0X0070);
	write_cmos_sensor_twobyte(0X602A, 0X0D4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0073);
	write_cmos_sensor_twobyte(0X6F12, 0X0072);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00D3);
	write_cmos_sensor_twobyte(0X6F12, 0X0125);
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X0069);
	write_cmos_sensor_twobyte(0X6F12, 0X0264);
	write_cmos_sensor_twobyte(0X6F12, 0X01C4);
	write_cmos_sensor_twobyte(0X602A, 0X0D74);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X6F12, 0X0014);
	write_cmos_sensor_twobyte(0X6F12, 0X0014);
	write_cmos_sensor_twobyte(0X6F12, 0X026C);
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);
	write_cmos_sensor_twobyte(0X602A, 0X0D96);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X00D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0132);
	write_cmos_sensor_twobyte(0X6F12, 0X00E3);
	write_cmos_sensor_twobyte(0X602A, 0X0DB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0119);
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);
	write_cmos_sensor_twobyte(0X602A, 0X0DD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00D1);
	write_cmos_sensor_twobyte(0X602A, 0X0DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);
	write_cmos_sensor_twobyte(0X6F12, 0X01CA);
	write_cmos_sensor_twobyte(0X602A, 0X0E16);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);
	write_cmos_sensor_twobyte(0X602A, 0X0FA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0287);
	write_cmos_sensor_twobyte(0X6F12, 0X01CC);
	write_cmos_sensor_twobyte(0X6F12, 0X003C);
	write_cmos_sensor_twobyte(0X6F12, 0X003C);
	write_cmos_sensor_twobyte(0X602A, 0X0FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0124);
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);
	write_cmos_sensor_twobyte(0X6F12, 0X00DF);
	write_cmos_sensor_twobyte(0X6F12, 0X0099);
	write_cmos_sensor_twobyte(0X6F12, 0X0296);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X602A, 0X0FD8);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0FDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);
	write_cmos_sensor_twobyte(0X602A, 0X0FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0301);
	write_cmos_sensor_twobyte(0X602A, 0X0FEC);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0003);
	write_cmos_sensor_twobyte(0X602A, 0X101C);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X101E);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1020);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1022);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1024);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1026);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1028);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X602A, 0X104C);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);
	write_cmos_sensor_twobyte(0X602A, 0X106C);
	write_cmos_sensor_twobyte(0X6F12, 0X0607);
	write_cmos_sensor_twobyte(0X602A, 0X10AE);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X10B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X0B0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3064);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0B02);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X0ABC);
	write_cmos_sensor_twobyte(0X6F12, 0X0207);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X070C);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X602A, 0X0110);
	write_cmos_sensor_twobyte(0X6F12, 0X1002);
	write_cmos_sensor_twobyte(0X602A, 0X0114);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X602A, 0X0408);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0344);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X1F6F);
	write_cmos_sensor_twobyte(0X6F12, 0X0BC7);
	write_cmos_sensor_twobyte(0X6F12, 0X1F60);
	write_cmos_sensor_twobyte(0X6F12, 0X0BC0);
	write_cmos_sensor_twobyte(0X602A, 0X0380);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0900);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X602A, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0404);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0304);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X030C);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0306);
	write_cmos_sensor_twobyte(0X6F12, 0X01F4);
	write_cmos_sensor_twobyte(0X602A, 0X0302);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X0005);
	write_cmos_sensor_twobyte(0X602A, 0X0312);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X030E);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6F12, 0X0153);
	write_cmos_sensor_twobyte(0X602A, 0X030A);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0308);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X602A, 0X0342);
	write_cmos_sensor_twobyte(0X6F12, 0X2100);
	write_cmos_sensor_twobyte(0X602A, 0X0340);
	write_cmos_sensor_twobyte(0X6F12, 0X0C54);
	write_cmos_sensor_twobyte(0X602A, 0X021E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X30A6);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X602A, 0X30A8);
	write_cmos_sensor_twobyte(0X6f12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X30CA);
	write_cmos_sensor_twobyte(0X6F12, 0X0031);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A70);
	write_cmos_sensor_twobyte(0X6F12, 0X6E33);
	write_cmos_sensor_twobyte(0X602A, 0X4814);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X4876);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X2412);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);
	write_cmos_sensor_twobyte(0X602A, 0X3128);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0007);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0007);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0007);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X0007);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X8724);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A30);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X30CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0702);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X228E);
	write_cmos_sensor_twobyte(0X6F12, 0X0245);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X602A, 0X8726);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X872C);
	write_cmos_sensor_twobyte(0X6F12, 0X0038);
	write_cmos_sensor_twobyte(0X602A, 0X09B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X515E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X5160);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X872E);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E0);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E2);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X4D70);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D80);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D90);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0B04);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X30A2);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X5400);
	write_cmos_sensor_twobyte(0X602A, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	#else // 12fps for 2L1 setting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//Do not uwrite_cmos_sensor_twobyte(0Xe Memory retention
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6010);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	// Rewrite_cmos_sensor_twobyte(0Xet
    mDELAY(3);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X7970);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X7150);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X0136);
	write_cmos_sensor_twobyte(0X6F12, 0X1800);	// 24

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X7D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0449);
	write_cmos_sensor_twobyte(0X6F12, 0X0348);
	write_cmos_sensor_twobyte(0X6F12, 0X044A);
	write_cmos_sensor_twobyte(0X6F12, 0X4860);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0881);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC2BB);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X87AC);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5830);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XA3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0168);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X57FC);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X59FC);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0XC28E);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB1);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0X0280);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X48BC);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0XF148);
	write_cmos_sensor_twobyte(0X6F12, 0X8A46);
	write_cmos_sensor_twobyte(0X6F12, 0X9146);
	write_cmos_sensor_twobyte(0X6F12, 0X4168);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X39FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0027);
	write_cmos_sensor_twobyte(0X6F12, 0XA780);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X5146);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFC);
	write_cmos_sensor_twobyte(0X6F12, 0X98F8);
	write_cmos_sensor_twobyte(0X6F12, 0XE200);
	write_cmos_sensor_twobyte(0X6F12, 0XE64A);
	write_cmos_sensor_twobyte(0X6F12, 0XE749);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD9);
	write_cmos_sensor_twobyte(0X6F12, 0XE648);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6C09);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF087);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XF8E7);
	write_cmos_sensor_twobyte(0X6F12, 0XFEB5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XD848);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X8168);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X17FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X03FC);
	write_cmos_sensor_twobyte(0X6F12, 0XD048);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X0029);
	write_cmos_sensor_twobyte(0X6F12, 0X1CD0);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XD900);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0XCB48);
	write_cmos_sensor_twobyte(0X6F12, 0X0292);
	write_cmos_sensor_twobyte(0X6F12, 0X0192);
	write_cmos_sensor_twobyte(0X6F12, 0X4A30);
	write_cmos_sensor_twobyte(0X6F12, 0XCA00);
	write_cmos_sensor_twobyte(0X6F12, 0XC17D);
	write_cmos_sensor_twobyte(0X6F12, 0XCD4C);
	write_cmos_sensor_twobyte(0X6F12, 0XC91D);
	write_cmos_sensor_twobyte(0X6F12, 0XC908);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0123);
	write_cmos_sensor_twobyte(0X6F12, 0X43EA);
	write_cmos_sensor_twobyte(0X6F12, 0X4101);
	write_cmos_sensor_twobyte(0X6F12, 0X6180);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X6B46);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF2);
	write_cmos_sensor_twobyte(0X6F12, 0X1010);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF7FB);
	write_cmos_sensor_twobyte(0X6F12, 0X0C20);
	write_cmos_sensor_twobyte(0X6F12, 0X2080);
	write_cmos_sensor_twobyte(0X6F12, 0XFEBD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC068);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD3FB);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE8FB);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0XC18A);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0268);
	write_cmos_sensor_twobyte(0X6F12, 0X02F5);
	write_cmos_sensor_twobyte(0X6F12, 0XC972);
	write_cmos_sensor_twobyte(0X6F12, 0X907A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0XBB49);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0XB4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF600);
	write_cmos_sensor_twobyte(0X6F12, 0X917A);
	write_cmos_sensor_twobyte(0X6F12, 0X411A);
	write_cmos_sensor_twobyte(0X6F12, 0XB948);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X34F8);
	write_cmos_sensor_twobyte(0X6F12, 0XFA1F);
	write_cmos_sensor_twobyte(0X6F12, 0X638A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X937A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X8180);
	write_cmos_sensor_twobyte(0X6F12, 0X1078);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD1);
	write_cmos_sensor_twobyte(0X6F12, 0XAE48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1115);
	write_cmos_sensor_twobyte(0X6F12, 0XB248);
	write_cmos_sensor_twobyte(0X6F12, 0X19B1);
	write_cmos_sensor_twobyte(0X6F12, 0XA749);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5D10);
	write_cmos_sensor_twobyte(0X6F12, 0X491C);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCC10);
	write_cmos_sensor_twobyte(0X6F12, 0XA348);
	write_cmos_sensor_twobyte(0X6F12, 0XB830);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X4188);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X9FBB);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0X9D48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0169);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XAFFB);
	write_cmos_sensor_twobyte(0X6F12, 0X9C48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1202);
	write_cmos_sensor_twobyte(0X6F12, 0X40B1);
	write_cmos_sensor_twobyte(0X6F12, 0X9D49);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X0968);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X7DBB);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4F);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0X8B48);
	write_cmos_sensor_twobyte(0X6F12, 0X81B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X4069);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X80B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0091);
	write_cmos_sensor_twobyte(0X6F12, 0X8346);
	write_cmos_sensor_twobyte(0X6F12, 0X0146);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6EFB);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0X1492);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF0);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X09F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3409);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X28A0);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X24A0);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X50A0);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0600);
	write_cmos_sensor_twobyte(0X6F12, 0X0028);
	write_cmos_sensor_twobyte(0X6F12, 0X76D0);
	write_cmos_sensor_twobyte(0X6F12, 0X8948);
	write_cmos_sensor_twobyte(0X6F12, 0X0026);
	write_cmos_sensor_twobyte(0X6F12, 0X3746);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D02);
	write_cmos_sensor_twobyte(0X6F12, 0XD8B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0024);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8405);
	write_cmos_sensor_twobyte(0X6F12, 0X296E);
	write_cmos_sensor_twobyte(0X6F12, 0X81B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0A22);
	write_cmos_sensor_twobyte(0X6F12, 0X286A);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X72FB);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4011);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XC021);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF001);
	write_cmos_sensor_twobyte(0X6F12, 0X5143);
	write_cmos_sensor_twobyte(0X6F12, 0X4031);
	write_cmos_sensor_twobyte(0X6F12, 0X00EB);
	write_cmos_sensor_twobyte(0X6F12, 0XE110);
	write_cmos_sensor_twobyte(0X6F12, 0X0644);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X641C);
	write_cmos_sensor_twobyte(0X6F12, 0X0744);
	write_cmos_sensor_twobyte(0X6F12, 0X102C);
	write_cmos_sensor_twobyte(0X6F12, 0XE4D3);
	write_cmos_sensor_twobyte(0X6F12, 0XF817);
	write_cmos_sensor_twobyte(0X6F12, 0X07EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0511);
	write_cmos_sensor_twobyte(0X6F12, 0XF017);
	write_cmos_sensor_twobyte(0X6F12, 0X06EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0411);
	write_cmos_sensor_twobyte(0X6F12, 0X99F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0700);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X0428);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X654E);
	write_cmos_sensor_twobyte(0X6F12, 0XD071);
	write_cmos_sensor_twobyte(0X6F12, 0X8236);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X2ADD);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X08D0);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X01EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X8B6A);
	write_cmos_sensor_twobyte(0X6F12, 0XCB62);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C00);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0XC462);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X92F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0730);
	write_cmos_sensor_twobyte(0X6F12, 0X8857);
	write_cmos_sensor_twobyte(0X6F12, 0X9842);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X8855);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C30);
	write_cmos_sensor_twobyte(0X6F12, 0X9957);
	write_cmos_sensor_twobyte(0X6F12, 0X0129);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDD);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0313);
	write_cmos_sensor_twobyte(0X6F12, 0X04E0);
	write_cmos_sensor_twobyte(0X6F12, 0X03EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8007);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XFF6A);
	write_cmos_sensor_twobyte(0X6F12, 0X3C44);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D8);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0XF1F4);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0800);
	write_cmos_sensor_twobyte(0X6F12, 0X50B3);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X14DB);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X10DB);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X1BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FB);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X21DB);
	write_cmos_sensor_twobyte(0X6F12, 0X22E0);
	write_cmos_sensor_twobyte(0X6F12, 0X34E0);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X11DC);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XFEFA);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0XAC42);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X6F12, 0X601B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X281B);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X18B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C20);
	write_cmos_sensor_twobyte(0X6F12, 0X02F8);
	write_cmos_sensor_twobyte(0X6F12, 0X06A0);
	write_cmos_sensor_twobyte(0X6F12, 0X0299);
	write_cmos_sensor_twobyte(0X6F12, 0X8968);
	write_cmos_sensor_twobyte(0X6F12, 0X4A78);
	write_cmos_sensor_twobyte(0X6F12, 0X0243);
	write_cmos_sensor_twobyte(0X6F12, 0X4A70);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0XC1E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0954);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5000);
	write_cmos_sensor_twobyte(0X6F12, 0X6C40);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5440);
	write_cmos_sensor_twobyte(0X6F12, 0X02AC);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X0ECC);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDEFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X05B0);
	write_cmos_sensor_twobyte(0X6F12, 0X5946);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF04F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA4BA);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X8246);
	write_cmos_sensor_twobyte(0X6F12, 0X1F48);
	write_cmos_sensor_twobyte(0X6F12, 0X8946);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X8169);
	write_cmos_sensor_twobyte(0X6F12, 0X1E46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0A9C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FA);
	write_cmos_sensor_twobyte(0X6F12, 0X1848);
	write_cmos_sensor_twobyte(0X6F12, 0X3346);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C40);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0X5046);
	write_cmos_sensor_twobyte(0X6F12, 0X0094);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC0FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X82BA);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0023);
	write_cmos_sensor_twobyte(0X6F12, 0X03E0);
	write_cmos_sensor_twobyte(0X6F12, 0X51F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2340);
	write_cmos_sensor_twobyte(0X6F12, 0X6040);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1C);
	write_cmos_sensor_twobyte(0X6F12, 0XB3EB);
	write_cmos_sensor_twobyte(0X6F12, 0X920F);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D3);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0948);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC069);
	write_cmos_sensor_twobyte(0X6F12, 0X87B2);
	write_cmos_sensor_twobyte(0X6F12, 0X4FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X1048);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X034C);
	write_cmos_sensor_twobyte(0X6F12, 0X3068);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A11);
	write_cmos_sensor_twobyte(0X6F12, 0XF1B3);
	write_cmos_sensor_twobyte(0X6F12, 0X19E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF4D2);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X8000);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X9000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF110);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5810);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X6AF0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XE304);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X7000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4B);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X51D1);
	write_cmos_sensor_twobyte(0X6F12, 0XFD48);
	write_cmos_sensor_twobyte(0X6F12, 0XFD49);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0XF0C3);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2FF);
	write_cmos_sensor_twobyte(0X6F12, 0XE066);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0XF949);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XABFF);
	write_cmos_sensor_twobyte(0X6F12, 0X2067);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XF749);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XA5FF);
	write_cmos_sensor_twobyte(0X6F12, 0X6067);
	write_cmos_sensor_twobyte(0X6F12, 0X3168);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XE36D);
	write_cmos_sensor_twobyte(0X6F12, 0XE26E);
	write_cmos_sensor_twobyte(0X6F12, 0X0126);
	write_cmos_sensor_twobyte(0X6F12, 0X9A42);
	write_cmos_sensor_twobyte(0X6F12, 0X01D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X60C0);
	write_cmos_sensor_twobyte(0X6F12, 0X236F);
	write_cmos_sensor_twobyte(0X6F12, 0X6345);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X020C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X64C0);
	write_cmos_sensor_twobyte(0X6F12, 0X6045);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X040C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XBCF1);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X07D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X5A40);
	write_cmos_sensor_twobyte(0X6F12, 0X4240);
	write_cmos_sensor_twobyte(0X6F12, 0XA266);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A51);
	write_cmos_sensor_twobyte(0X6F12, 0X0AE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B61);
	write_cmos_sensor_twobyte(0X6F12, 0XF6E7);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XA566);
	write_cmos_sensor_twobyte(0X6F12, 0XE566);
	write_cmos_sensor_twobyte(0X6F12, 0X2567);
	write_cmos_sensor_twobyte(0X6F12, 0X6567);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X27FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0XD348);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FA);
	write_cmos_sensor_twobyte(0X6F12, 0XCF48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A01);
	write_cmos_sensor_twobyte(0X6F12, 0X48B3);
	write_cmos_sensor_twobyte(0X6F12, 0XC348);
	write_cmos_sensor_twobyte(0X6F12, 0XC44B);
	write_cmos_sensor_twobyte(0X6F12, 0XC44A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X20D1);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0DFA);
	write_cmos_sensor_twobyte(0X6F12, 0XC148);
	write_cmos_sensor_twobyte(0X6F12, 0XC149);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XC04F);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFF);
	write_cmos_sensor_twobyte(0X6F12, 0XC14C);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0XBD49);
	write_cmos_sensor_twobyte(0X6F12, 0XE065);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X32FF);
	write_cmos_sensor_twobyte(0X6F12, 0X2066);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XBA49);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFF);
	write_cmos_sensor_twobyte(0X6F12, 0X6066);
	write_cmos_sensor_twobyte(0X6F12, 0XD4E9);
	write_cmos_sensor_twobyte(0X6F12, 0X1712);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X4140);
	write_cmos_sensor_twobyte(0X6F12, 0XA165);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA1B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XB148);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X406A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDBF9);
	write_cmos_sensor_twobyte(0X6F12, 0XE068);
	write_cmos_sensor_twobyte(0X6F12, 0X07F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8047);
	write_cmos_sensor_twobyte(0X6F12, 0X8069);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0XC288);
	write_cmos_sensor_twobyte(0X6F12, 0X0389);
	write_cmos_sensor_twobyte(0X6F12, 0XA1EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5201);
	write_cmos_sensor_twobyte(0X6F12, 0X827B);
	write_cmos_sensor_twobyte(0X6F12, 0X891A);
	write_cmos_sensor_twobyte(0X6F12, 0X8289);
	write_cmos_sensor_twobyte(0X6F12, 0XC07B);
	write_cmos_sensor_twobyte(0X6F12, 0XA2EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5302);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0XB980);
	write_cmos_sensor_twobyte(0X6F12, 0XE268);
	write_cmos_sensor_twobyte(0X6F12, 0X491E);
	write_cmos_sensor_twobyte(0X6F12, 0X9269);
	write_cmos_sensor_twobyte(0X6F12, 0X128A);
	write_cmos_sensor_twobyte(0X6F12, 0X1144);
	write_cmos_sensor_twobyte(0X6F12, 0XF980);
	write_cmos_sensor_twobyte(0X6F12, 0X3881);
	write_cmos_sensor_twobyte(0X6F12, 0XE168);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X8969);
	write_cmos_sensor_twobyte(0X6F12, 0X498A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X7881);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BB9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8146);
	write_cmos_sensor_twobyte(0X6F12, 0X9648);
	write_cmos_sensor_twobyte(0X6F12, 0X0D46);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X816A);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0E0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X5CF9);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X4846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA6F9);
	write_cmos_sensor_twobyte(0X6F12, 0X8E48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9201);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X15D1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2900);
	write_cmos_sensor_twobyte(0X6F12, 0X70B1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCE02);
	write_cmos_sensor_twobyte(0X6F12, 0X08B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6F12, 0X07E0);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XDA00);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X8748);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5C00);
	write_cmos_sensor_twobyte(0X6F12, 0X10B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4210);
	write_cmos_sensor_twobyte(0X6F12, 0X88F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1110);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X33B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1001);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X83F9);
	write_cmos_sensor_twobyte(0X6F12, 0X774D);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XF006);
	write_cmos_sensor_twobyte(0X6F12, 0X05F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3405);
	write_cmos_sensor_twobyte(0X6F12, 0X1ED5);
	write_cmos_sensor_twobyte(0X6F12, 0X7748);
	write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XB207);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X03D2);
	write_cmos_sensor_twobyte(0X6F12, 0X2146);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X76F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X1346);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0X39B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0F88);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7870);
	write_cmos_sensor_twobyte(0X6F12, 0XC08A);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0X01E0);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7830);
	write_cmos_sensor_twobyte(0X6F12, 0X521C);
	write_cmos_sensor_twobyte(0X6F12, 0X062A);
	write_cmos_sensor_twobyte(0X6F12, 0XEED3);
	write_cmos_sensor_twobyte(0X6F12, 0X7004);
	write_cmos_sensor_twobyte(0X6F12, 0X1AD5);
	write_cmos_sensor_twobyte(0X6F12, 0XB5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8400);
	write_cmos_sensor_twobyte(0X6F12, 0X411E);
	write_cmos_sensor_twobyte(0X6F12, 0XA142);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X04D2);
	write_cmos_sensor_twobyte(0X6F12, 0X44F4);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X54F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4002);
	write_cmos_sensor_twobyte(0X6F12, 0X5189);
	write_cmos_sensor_twobyte(0X6F12, 0X29B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7820);
	write_cmos_sensor_twobyte(0X6F12, 0X12B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0A80);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X0628);
	write_cmos_sensor_twobyte(0X6F12, 0XF2D3);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF081);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X5448);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC16A);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDAF8);
	write_cmos_sensor_twobyte(0X6F12, 0X43F6);
	write_cmos_sensor_twobyte(0X6F12, 0XEF73);
	write_cmos_sensor_twobyte(0X6F12, 0X0322);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XB901);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X36F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X38F9);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC8B8);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X484C);
	write_cmos_sensor_twobyte(0X6F12, 0XE38B);
	write_cmos_sensor_twobyte(0X6F12, 0XCB1A);
	write_cmos_sensor_twobyte(0X6F12, 0X218C);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X0144);
	write_cmos_sensor_twobyte(0X6F12, 0X9942);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X0346);
	write_cmos_sensor_twobyte(0X6F12, 0X4149);
	write_cmos_sensor_twobyte(0X6F12, 0X3431);
	write_cmos_sensor_twobyte(0X6F12, 0X9040);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8630);
	write_cmos_sensor_twobyte(0X6F12, 0X03FA);
	write_cmos_sensor_twobyte(0X6F12, 0X02F3);
	write_cmos_sensor_twobyte(0X6F12, 0X414A);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XD181);
	write_cmos_sensor_twobyte(0X6F12, 0X1382);
	write_cmos_sensor_twobyte(0X6F12, 0X03D0);
	write_cmos_sensor_twobyte(0X6F12, 0X5181);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X9381);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF70);
	write_cmos_sensor_twobyte(0X6F12, 0X5081);
	write_cmos_sensor_twobyte(0X6F12, 0X9081);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7B71);
	write_cmos_sensor_twobyte(0X6F12, 0X3948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0FF9);
	write_cmos_sensor_twobyte(0X6F12, 0X334C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5371);
	write_cmos_sensor_twobyte(0X6F12, 0X2060);
	write_cmos_sensor_twobyte(0X6F12, 0X3648);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X07F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XFF61);
	write_cmos_sensor_twobyte(0X6F12, 0X6060);
	write_cmos_sensor_twobyte(0X6F12, 0X3448);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X00F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XA060);
	write_cmos_sensor_twobyte(0X6F12, 0X3148);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2D61);
	write_cmos_sensor_twobyte(0X6F12, 0XE060);
	write_cmos_sensor_twobyte(0X6F12, 0X2F48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XF551);
	write_cmos_sensor_twobyte(0X6F12, 0X2061);
	write_cmos_sensor_twobyte(0X6F12, 0X2C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XEBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5341);
	write_cmos_sensor_twobyte(0X6F12, 0X6061);
	write_cmos_sensor_twobyte(0X6F12, 0X2A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XA061);
	write_cmos_sensor_twobyte(0X6F12, 0X2021);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X6000);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8006);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3400);
	write_cmos_sensor_twobyte(0X6F12, 0X7580);
	write_cmos_sensor_twobyte(0X6F12, 0X4562);
	write_cmos_sensor_twobyte(0X6F12, 0X8562);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C50);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5050);
	write_cmos_sensor_twobyte(0X6F12, 0X2A46);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7D21);
	write_cmos_sensor_twobyte(0X6F12, 0X4565);
	write_cmos_sensor_twobyte(0X6F12, 0X1E48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XCBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2121);
	write_cmos_sensor_twobyte(0X6F12, 0X6062);
	write_cmos_sensor_twobyte(0X6F12, 0X1C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X4541);
	write_cmos_sensor_twobyte(0X6F12, 0XA062);
	write_cmos_sensor_twobyte(0X6F12, 0X1948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XBDF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F31);
	write_cmos_sensor_twobyte(0X6F12, 0XE061);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0XABBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0345);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0X1234);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5423);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0XFECA);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X7C80);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0B27);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0E5D);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X5C51);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X1F65);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X98A9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X852F);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XDE2F);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X10B3);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0823);
	write_cmos_sensor_twobyte(0X6F12, 0X0A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X87F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X2062);
	write_cmos_sensor_twobyte(0X6F12, 0X0848);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X6F11);
	write_cmos_sensor_twobyte(0X6F12, 0XE062);
	write_cmos_sensor_twobyte(0X6F12, 0X0548);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X79F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2063);
	write_cmos_sensor_twobyte(0X6F12, 0XF562);
	write_cmos_sensor_twobyte(0X6F12, 0X3563);
	write_cmos_sensor_twobyte(0X6F12, 0X7563);
	write_cmos_sensor_twobyte(0X6F12, 0X70BD);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X339B);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X2ABB);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X135F);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0XE91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X273C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X5D6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X033C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X3B2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X45F6);
	write_cmos_sensor_twobyte(0X6F12, 0X514C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X657C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D0C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X471C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X49F6);
	write_cmos_sensor_twobyte(0X6F12, 0XA90C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X48F2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X230C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X43F2);
	write_cmos_sensor_twobyte(0X6F12, 0X9B3C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3B7C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X4DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X2F6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB30C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X871C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X6F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1B5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0XF91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X42F6);
	write_cmos_sensor_twobyte(0X6F12, 0XBB2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB71C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);

	write_cmos_sensor_twobyte(0X602A, 0X1150);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ah 680mV
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ch 680mV
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF466);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	// aig_dbr_tune_tg_l 3.05V
	write_cmos_sensor_twobyte(0X6F12, 0X000C);	// aig_dbr_tune_tg_r 3.10V
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X111C);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	// F46Ah 3.7V
	write_cmos_sensor_twobyte(0X602A, 0X1120);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	// F470h -1.1V
	write_cmos_sensor_twobyte(0X602A, 0X113E);
	write_cmos_sensor_twobyte(0X6F12, 0X2101);	// it equalwrite_cmos_sensor_twobyte(0X to aig_left_right_tg_write_cmos_sensor_twobyte(0Xel 06h
	write_cmos_sensor_twobyte(0X602A, 0X1106);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);


	write_cmos_sensor_twobyte(0X602A, 0X3A28);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A52);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A48);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:8 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:4 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A34);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:2 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A84);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// write_cmos_sensor_twobyte(0Xhort Red 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3AB6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// write_cmos_sensor_twobyte(0Xhort Blue 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3B44);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3B48);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X3B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);

	// EIT write_cmos_sensor_twobyte, 0X(0Xe);tting
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0200);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0224);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0224h Addrewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0226h Addrewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X

	// ETC write_cmos_sensor_twobyte, 0X(0Xe);tting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X1124);
	write_cmos_sensor_twobyte(0X6F12, 0X0328);	// ADBUwrite_cmos_sensor_twobyte(0X data order
	write_cmos_sensor_twobyte(0X602A, 0X0ACE);
	write_cmos_sensor_twobyte(0X6F12, 0X0030);

	write_cmos_sensor_twobyte(0X602A, 0X09C0);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X09C2);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X6F12, 0X008C);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0XF408);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X54A2);
	write_cmos_sensor_twobyte(0X6F12, 0XF418);	// CDwrite_cmos_sensor_twobyte(0X_Option_L
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X000D);
	write_cmos_sensor_twobyte(0X6F12, 0XF472);	// CLP_Level
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF482);	// COMP_BIAwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF484);	// COMP_BIAwrite_cmos_sensor_twobyte(0X

	// ADLC & Dithering write_cmos_s, 0Xenso);r_twobyte(0Xetting
	write_cmos_sensor_twobyte(0X602A, 0X47F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FE);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);

	// ATOP write_cmos_sensor_twobyt, 0Xe(0X);etting
	write_cmos_sensor_twobyte(0X602A, 0X1108);
	write_cmos_sensor_twobyte(0X6F12, 0X054C);	// F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt on & LP_VBLK_EN)
	write_cmos_sensor_twobyte(0X6F12, 0X0183);	// F406h Def. 0083h
	write_cmos_sensor_twobyte(0X6F12, 0X07FF);	// F408h Def. 07DFh (Dabx 0ff & Dwrite_cmos_sensor_twobyte(0Xrx en)


	write_cmos_sensor_twobyte(0X602A, 0X1154);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);	// F418h LDB on x4 write_cmos_sensor_twobyte(0X3
	write_cmos_sensor_twobyte(0X6F12, 0X162F);	// F41Ah // CRwrite_cmos_sensor_twobyte(0Xon
	write_cmos_sensor_twobyte(0X6F12, 0X4328);	// F41Ch
	write_cmos_sensor_twobyte(0X602A, 0X1126);
	write_cmos_sensor_twobyte(0X6F12, 0X00A2);	// F4A6h  IPI write_cmos_sensor_twobyte(0Xignal dip control
	write_cmos_sensor_twobyte(0X602A, 0X114A);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_write_cmos_sensor_twobyte(0XenAnalog_rmp_option_top
	write_cmos_sensor_twobyte(0X602A, 0X114E);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_write_cmos_sensor_twobyte(0XenAnalog_rmp_option_bot
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF41E);
	write_cmos_sensor_twobyte(0X6F12, 0X2100);	// DBUwrite_cmos_sensor_twobyte(0X option (Hybrid Gain)
	write_cmos_sensor_twobyte(0X602A, 0XF488);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// REF option

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X112A);
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
	write_cmos_sensor_twobyte(0X602A, 0X1128);
	write_cmos_sensor_twobyte(0X6F12, 0X001B);

	// Current write_cmos_sensor_two, 0Xbyte);(0Xetting
	write_cmos_sensor_twobyte(0X602A, 0X10F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);	// F476h
	write_cmos_sensor_twobyte(0X602A, 0X10F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);	// F478h
	write_cmos_sensor_twobyte(0X602A, 0X10FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);	// F47Ah
	write_cmos_sensor_twobyte(0X602A, 0X10FE);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);	// F47Ch

	// DBUwrite_cmos_sensor_twobyte(, 0X0X L);atch write_cmos_sensor_twobyte(0Xetting
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF414);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// aig_latch_write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X6F12, 0X0004);	// aig_latch_width

	// Clock write_cmos_sensor_twoby, 0Xte(0);Xetting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X10C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X602A, 0X10B6);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X0B26);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X602A, 0X0B2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X602A, 0X0B36);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);


	write_cmos_sensor_twobyte(0X602A, 0X0B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_write_cmos_sensor_twobyte(0Xl_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_write_cmos_sensor_twobyte(0Xl_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_write_cmos_sensor_twobyte(0Xl_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_write_cmos_sensor_twobyte(0Xl_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//aig_rx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//aig_rx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_rx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_rx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_tx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B7);	//aig_tx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_tx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);	//aig_tx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0105);	//aig_fx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);	//aig_fx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0140);	//aig_fx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);	//aig_fx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00EC);	//aig_dwrite_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X00A6);	//aig_dwrite_cmos_sensor_twobyte(0Xrx_reg0

	write_cmos_sensor_twobyte(0X602A, 0X0B76);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);	//aig_write_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X004D);	//aig_write_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B7E);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dwrite_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X009F);	//aig_dwrite_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B86);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_write_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_write_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B8E);
	write_cmos_sensor_twobyte(0X6F12, 0X0047);	//aig_write_cmos_sensor_twobyte(0Xfx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0048);	//aig_write_cmos_sensor_twobyte(0Xfx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B94);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_write_cmos_sensor_twobyte(0Xfx_reg1
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dabx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X009F);	//aig_dabx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_abx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_abx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0BAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0101 //aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 00B2 //aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026E //aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 01CD //aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0106 //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 00B7 //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026C //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 01CB //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_write_cmos_sensor_twobyte(0X1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_write_cmos_sensor_twobyte(0X1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0156);	//aig_write_cmos_sensor_twobyte(0X1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00FF);	//aig_write_cmos_sensor_twobyte(0X1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0057);	//aig_write_cmos_sensor_twobyte(0X3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0057);	//aig_write_cmos_sensor_twobyte(0X3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X004E);	//aig_write_cmos_sensor_twobyte(0X4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X004E);	//aig_write_cmos_sensor_twobyte(0X4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_clp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_clp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_clp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_clp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0016);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X014E);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00FA);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006B);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X006B);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_lp_hblk_cdwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_lp_hblk_cdwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C0A);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000F);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000F);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00D7);	//aig_cnt_en_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0087);	//aig_cnt_en_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_cnt_en_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_cnt_en_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01D6);	//aig_cnt_en_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0134);	//aig_cnt_en_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026A);	//aig_cnt_en_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);	//aig_cnt_en_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_enb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_conv_enb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_conv_enb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);	//aig_conv_enb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_conv1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);	//aig_conv1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0111);	//aig_conv2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00C2);	//aig_conv2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C62);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00C0);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C72);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//aig_comp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0085);	//aig_comp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0104);	//aig_comp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B5);	//aig_comp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01D4);	//aig_comp_en_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0132);	//aig_comp_en_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_comp_en_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_comp_en_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C92);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);	//aig_rd_en_10_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0002);	//aig_rd_en_10_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X001C);	//aig_rd_en_10_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0016);	//aig_rd_en_10_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X001E);	//aig_rd_en_9_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0017);	//aig_rd_en_9_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0038);	//aig_rd_en_9_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X002B);	//aig_rd_en_9_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X003A);	//aig_rd_en_8_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X002C);	//aig_rd_en_8_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0054);	//aig_rd_en_8_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0040);	//aig_rd_en_8_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0076);	//aig_rd_en_7_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0041);	//aig_rd_en_7_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0090);	//aig_rd_en_7_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_rd_en_7_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X010B);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00C6);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0127);	//aig_rd_en_5_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00C7);	//aig_rd_en_5_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0141);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0143);	//aig_rd_en_4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00DC);	//aig_rd_en_4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X015D);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X015F);	//aig_rd_en_3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);	//aig_rd_en_3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0179);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0105);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X017B);	//aig_rd_en_2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_rd_en_2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0195);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X011A);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0197);	//aig_rd_en_1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	//aig_rd_en_1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01B1);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01B4);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01B3);	//aig_rd_en_0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01B5);	//aig_rd_en_0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr6
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr6
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr7
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr7
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr8
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr8
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr9
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr9
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr10
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr10
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr11
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr11
	write_cmos_sensor_twobyte(0X602A, 0X0D22);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D32);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X006F);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006F);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0270);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CE);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0071);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0070);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0073);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0072);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0122);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00D3);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0103);	//aig_rmp_mode_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);	//aig_rmp_mode_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006A);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0069);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0264);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C4);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D74);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);	//aig_tg_hwrite_cmos_sensor_twobyte(0Xync_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0014);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0014);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_optl0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_optl0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D96);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);	//aig_optl1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00D9);	//aig_optl1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0132);	//aig_optl1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00E3);	//aig_optl1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_optl2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_optl2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_optl2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);	//aig_optl2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);	//aig_optl3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00D1);	//aig_optl3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CA);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0E16);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_opth0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);	//aig_opth0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_opth0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_opth0_ptr1

	write_cmos_sensor_twobyte(0X602A, 0X0FA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0287);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X01CC);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X003C);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X003C);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0124);	//VIR_2_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//VIR_2_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X00DF);	//VIR_Mwrite_cmos_sensor_twobyte(0XO_PTR0_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X0099);	//VIR_Mwrite_cmos_sensor_twobyte(0XO_PTR0_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X0296);	//VIR_MIN_ADC_COLwrite_cmos_sensor_twobyte(0X_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);	//VIR_MIN_ADC_COLwrite_cmos_sensor_twobyte(0X_1_1_1

	write_cmos_sensor_twobyte(0X602A, 0X0FD8);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0FDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);	//aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0301);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0FEC);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0003);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X101C);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X101E);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1020);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1022);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1024);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1026);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1028);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X104C);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X106C);
	write_cmos_sensor_twobyte(0X6F12, 0X0607);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X10AE);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X10B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1

	/////// APwrite_cmos_sensor_twob, 0Xyte();0X PD
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_AigMain1Value 0540	// F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt off & LP_VBLK_EN)
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_cdwrite_cmos_sensor_twobyte(0X_option_m	4338	// F41Ch
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_cdwrite_cmos_sensor_twobyte(0X_option_md	162E	// F41Ah
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__low_gain_val 000C	// F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__high_gain_val 000C	// F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_1__low_gain_val 0080
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_1__high_gain_val 0080


	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_AigMain1Value 0544   // F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt off)
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__low_gain_val 000F // F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__high_gain_val 000F // F408h

	write_cmos_sensor_twobyte(0X602A, 0X0B0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3064);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0B02);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X0ABC);
	write_cmos_sensor_twobyte(0X6F12, 0X0207);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X070C);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);

	write_cmos_sensor_twobyte(0X602A, 0X0110);
	write_cmos_sensor_twobyte(0X6F12, 0X1002);
	write_cmos_sensor_twobyte(0X602A, 0X0114);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);

	write_cmos_sensor_twobyte(0X602A, 0X0408);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X602A, 0X0344);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X1F6F);	// 8063
	write_cmos_sensor_twobyte(0X6F12, 0X0BC7);	// 3023

	write_cmos_sensor_twobyte(0X6F12, 0X1F60);	// 8064
	write_cmos_sensor_twobyte(0X6F12, 0X0BC0);	// 3024

	write_cmos_sensor_twobyte(0X602A, 0X0380);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0900);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);

	write_cmos_sensor_twobyte(0X602A, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0404);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);

	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);

	//PLL write_cmos_sensor_twobyte(, 0X0Xyw);rite_cmos_sensor_twobyte(0X = 800 , write_cmos_sensor_twobyte(0Xec = 798
	write_cmos_sensor_twobyte(0X602A, 0X0304);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X030C);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0306);
	write_cmos_sensor_twobyte(0X6F12, 0X01F4);	// 500
	write_cmos_sensor_twobyte(0X602A, 0X0302);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X0005);

	write_cmos_sensor_twobyte(0X602A, 0X0312);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X030E);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	// 266
	write_cmos_sensor_twobyte(0X602A, 0X030A);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0308);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);

	write_cmos_sensor_twobyte(0X602A, 0X0342);
	write_cmos_sensor_twobyte(0X6F12, 0X5120);	// 20768
	write_cmos_sensor_twobyte(0X602A, 0X0340);
	write_cmos_sensor_twobyte(0X6F12, 0X0C8A);	// 3210

	write_cmos_sensor_twobyte(0X602A, 0X021E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	// write_cmos_sensor_twobyte(0Xu, 0Xmmat);ion
	write_cmos_sensor_twobyte(0X602A, 0X30A6);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);	//Interleaved LRLR

	write_cmos_sensor_twobyte(0X602A, 0X30A8);
	write_cmos_sensor_twobyte(0X6f12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X30CA);
	write_cmos_sensor_twobyte(0X6F12, 0X0031);

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A70);
	write_cmos_sensor_twobyte(0X6F12, 0X6E33);

	// Bypawrite_cmos_sensor_twobyte, 0X(0Xw);rite_cmos_sensor_twobyte(0X all chain procewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0Xing
	write_cmos_sensor_twobyte(0X602A, 0X4814);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X4876);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);

	write_cmos_sensor_twobyte(0X602A, 0X2412);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);	//4x4 mwrite_cmos_sensor_twobyte(0Xm
	write_cmos_sensor_twobyte(0X602A, 0X3128);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb

	// wait to write_cmos_sensor_two, 0Xbyte);(0Xtart Work around - Add delay from H time write_cmos_sensor_twobyte(0Xtart.
	write_cmos_sensor_twobyte(0X602A, 0X8724);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);	// ex. add 32 TG CLC'write_cmos_sensor_twobyte(0X Delay (Delay in TG CLK'write_cmos_sensor_twobyte(0X)

	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY

	//Modify on-Line write_cmos_sens, 0Xor_t);wobyte(0Xettingwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A30);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);

	//emb linewrite_cmos_sensor_twob, 0Xyte();
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X30CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);

	//TnP fix: line write_cmos_senso, 0Xr_tw);obyte(0Xwap write_cmos_sensor_twobyte(0Xupport for WDR Off
	write_cmos_sensor_twobyte(0X602A, 0X0702);
	write_cmos_sensor_twobyte(0X6F12, 0X01	);//01 Enabled ;00 Diwrite_cmos_sensor_twobyte(0Xabled

	//TnP fix: iir filter rewrite_cm, 0Xos_s);ensor_twobyte(0Xet uwrite_cmos_sensor_twobyte(0Xing FOB write_cmos_sensor_twobyte(0Xtatiwrite_cmos_sensor_twobyte(0Xticwrite_cmos_sensor_twobyte(0X diff
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X228E);
	write_cmos_sensor_twobyte(0X6F12, 0X0245);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X602A, 0X8726);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X872C);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);

	//write_cmos_sensor_twobyte(0Xub, 0Xtrac);t on write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X602A, 0X09B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	//enable write_cmos_sensor_twobyte(0Xubtract on write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X602A, 0X515E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//Diwrite_cmos_sensor_twobyte(0Xable write_cmos_sensor_twobyte(0Xubtract of Y Data
	write_cmos_sensor_twobyte(0X602A, 0X5160);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	//TnP:	HW regiwrite_cmos_sensor, 0X_two);byte(0Xter update on V-Blank	(modify different HW write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X on V-Blank)
	write_cmos_sensor_twobyte(0X602A, 0X872E);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E0);	//aig_dummy_addr_dr_rb_e
	write_cmos_sensor_twobyte(0X6F12, 0XF4E2);	//aig_dummy_addr_dr_rb_o
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X for tewrite_cmos_sensor_twobyte(0Xt
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X for tewrite_cmos_sensor_twobyte(0Xt
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X602A, 0X4D70);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D80);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D90);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0B04);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X30A2);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X5400);

	write_cmos_sensor_twobyte(0X602A, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);

	#endif

#else
	// M2 setting
	// M2 setting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//Do not use Memory retention
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6010);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	// Reset
	mDELAY(3);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X7970);	// Modify CLK to allow HW writes on STBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X7150);	// Modify CLK to allow HW writes on STBY
	write_cmos_sensor_twobyte(0X602A, 0X0136);
	write_cmos_sensor_twobyte(0X6F12, 0X1800);	// 24
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X7D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0449);
	write_cmos_sensor_twobyte(0X6F12, 0X0348);
	write_cmos_sensor_twobyte(0X6F12, 0X044A);
	write_cmos_sensor_twobyte(0X6F12, 0X4860);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0881);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC2BB);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X87AC);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5830);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XA3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0168);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X57FC);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X59FC);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0XC28E);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB1);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0X0280);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X48BC);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0XF148);
	write_cmos_sensor_twobyte(0X6F12, 0X8A46);
	write_cmos_sensor_twobyte(0X6F12, 0X9146);
	write_cmos_sensor_twobyte(0X6F12, 0X4168);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X39FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0027);
	write_cmos_sensor_twobyte(0X6F12, 0XA780);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X5146);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFC);
	write_cmos_sensor_twobyte(0X6F12, 0X98F8);
	write_cmos_sensor_twobyte(0X6F12, 0XE200);
	write_cmos_sensor_twobyte(0X6F12, 0XE64A);
	write_cmos_sensor_twobyte(0X6F12, 0XE749);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD9);
	write_cmos_sensor_twobyte(0X6F12, 0XE648);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6C09);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF087);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XF8E7);
	write_cmos_sensor_twobyte(0X6F12, 0XFEB5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XD848);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X8168);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X17FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X03FC);
	write_cmos_sensor_twobyte(0X6F12, 0XD048);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X0029);
	write_cmos_sensor_twobyte(0X6F12, 0X1CD0);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XD900);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0XCB48);
	write_cmos_sensor_twobyte(0X6F12, 0X0292);
	write_cmos_sensor_twobyte(0X6F12, 0X0192);
	write_cmos_sensor_twobyte(0X6F12, 0X4A30);
	write_cmos_sensor_twobyte(0X6F12, 0XCA00);
	write_cmos_sensor_twobyte(0X6F12, 0XC17D);
	write_cmos_sensor_twobyte(0X6F12, 0XCD4C);
	write_cmos_sensor_twobyte(0X6F12, 0XC91D);
	write_cmos_sensor_twobyte(0X6F12, 0XC908);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0123);
	write_cmos_sensor_twobyte(0X6F12, 0X43EA);
	write_cmos_sensor_twobyte(0X6F12, 0X4101);
	write_cmos_sensor_twobyte(0X6F12, 0X6180);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X6B46);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF2);
	write_cmos_sensor_twobyte(0X6F12, 0X1010);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF7FB);
	write_cmos_sensor_twobyte(0X6F12, 0X0C20);
	write_cmos_sensor_twobyte(0X6F12, 0X2080);
	write_cmos_sensor_twobyte(0X6F12, 0XFEBD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC068);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD3FB);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE8FB);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0XC18A);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0268);
	write_cmos_sensor_twobyte(0X6F12, 0X02F5);
	write_cmos_sensor_twobyte(0X6F12, 0XC972);
	write_cmos_sensor_twobyte(0X6F12, 0X907A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0XBB49);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0XB4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF600);
	write_cmos_sensor_twobyte(0X6F12, 0X917A);
	write_cmos_sensor_twobyte(0X6F12, 0X411A);
	write_cmos_sensor_twobyte(0X6F12, 0XB948);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X34F8);
	write_cmos_sensor_twobyte(0X6F12, 0XFA1F);
	write_cmos_sensor_twobyte(0X6F12, 0X638A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X937A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X8180);
	write_cmos_sensor_twobyte(0X6F12, 0X1078);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD1);
	write_cmos_sensor_twobyte(0X6F12, 0XAE48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1115);
	write_cmos_sensor_twobyte(0X6F12, 0XB248);
	write_cmos_sensor_twobyte(0X6F12, 0X19B1);
	write_cmos_sensor_twobyte(0X6F12, 0XA749);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5D10);
	write_cmos_sensor_twobyte(0X6F12, 0X491C);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCC10);
	write_cmos_sensor_twobyte(0X6F12, 0XA348);
	write_cmos_sensor_twobyte(0X6F12, 0XB830);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X4188);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X9FBB);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0X9D48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0169);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XAFFB);
	write_cmos_sensor_twobyte(0X6F12, 0X9C48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1202);
	write_cmos_sensor_twobyte(0X6F12, 0X40B1);
	write_cmos_sensor_twobyte(0X6F12, 0X9D49);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X0968);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X7DBB);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4F);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0X8B48);
	write_cmos_sensor_twobyte(0X6F12, 0X81B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X4069);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X80B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0091);
	write_cmos_sensor_twobyte(0X6F12, 0X8346);
	write_cmos_sensor_twobyte(0X6F12, 0X0146);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6EFB);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0X1492);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF0);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X09F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3409);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X28A0);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X24A0);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X50A0);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0600);
	write_cmos_sensor_twobyte(0X6F12, 0X0028);
	write_cmos_sensor_twobyte(0X6F12, 0X76D0);
	write_cmos_sensor_twobyte(0X6F12, 0X8948);
	write_cmos_sensor_twobyte(0X6F12, 0X0026);
	write_cmos_sensor_twobyte(0X6F12, 0X3746);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D02);
	write_cmos_sensor_twobyte(0X6F12, 0XD8B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0024);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8405);
	write_cmos_sensor_twobyte(0X6F12, 0X296E);
	write_cmos_sensor_twobyte(0X6F12, 0X81B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0A22);
	write_cmos_sensor_twobyte(0X6F12, 0X286A);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X72FB);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4011);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XC021);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF001);
	write_cmos_sensor_twobyte(0X6F12, 0X5143);
	write_cmos_sensor_twobyte(0X6F12, 0X4031);
	write_cmos_sensor_twobyte(0X6F12, 0X00EB);
	write_cmos_sensor_twobyte(0X6F12, 0XE110);
	write_cmos_sensor_twobyte(0X6F12, 0X0644);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X641C);
	write_cmos_sensor_twobyte(0X6F12, 0X0744);
	write_cmos_sensor_twobyte(0X6F12, 0X102C);
	write_cmos_sensor_twobyte(0X6F12, 0XE4D3);
	write_cmos_sensor_twobyte(0X6F12, 0XF817);
	write_cmos_sensor_twobyte(0X6F12, 0X07EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0511);
	write_cmos_sensor_twobyte(0X6F12, 0XF017);
	write_cmos_sensor_twobyte(0X6F12, 0X06EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0411);
	write_cmos_sensor_twobyte(0X6F12, 0X99F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0700);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X0428);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X654E);
	write_cmos_sensor_twobyte(0X6F12, 0XD071);
	write_cmos_sensor_twobyte(0X6F12, 0X8236);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X2ADD);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X08D0);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X01EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X8B6A);
	write_cmos_sensor_twobyte(0X6F12, 0XCB62);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C00);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0XC462);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X92F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0730);
	write_cmos_sensor_twobyte(0X6F12, 0X8857);
	write_cmos_sensor_twobyte(0X6F12, 0X9842);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X8855);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C30);
	write_cmos_sensor_twobyte(0X6F12, 0X9957);
	write_cmos_sensor_twobyte(0X6F12, 0X0129);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDD);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0313);
	write_cmos_sensor_twobyte(0X6F12, 0X04E0);
	write_cmos_sensor_twobyte(0X6F12, 0X03EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8007);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XFF6A);
	write_cmos_sensor_twobyte(0X6F12, 0X3C44);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D8);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0XF1F4);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0800);
	write_cmos_sensor_twobyte(0X6F12, 0X50B3);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X14DB);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X10DB);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X1BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FB);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X21DB);
	write_cmos_sensor_twobyte(0X6F12, 0X22E0);
	write_cmos_sensor_twobyte(0X6F12, 0X34E0);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X11DC);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XFEFA);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0XAC42);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X6F12, 0X601B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X281B);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X18B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C20);
	write_cmos_sensor_twobyte(0X6F12, 0X02F8);
	write_cmos_sensor_twobyte(0X6F12, 0X06A0);
	write_cmos_sensor_twobyte(0X6F12, 0X0299);
	write_cmos_sensor_twobyte(0X6F12, 0X8968);
	write_cmos_sensor_twobyte(0X6F12, 0X4A78);
	write_cmos_sensor_twobyte(0X6F12, 0X0243);
	write_cmos_sensor_twobyte(0X6F12, 0X4A70);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0XC1E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0954);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5000);
	write_cmos_sensor_twobyte(0X6F12, 0X6C40);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5440);
	write_cmos_sensor_twobyte(0X6F12, 0X02AC);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X0ECC);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDEFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X05B0);
	write_cmos_sensor_twobyte(0X6F12, 0X5946);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF04F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA4BA);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X8246);
	write_cmos_sensor_twobyte(0X6F12, 0X1F48);
	write_cmos_sensor_twobyte(0X6F12, 0X8946);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X8169);
	write_cmos_sensor_twobyte(0X6F12, 0X1E46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0A9C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FA);
	write_cmos_sensor_twobyte(0X6F12, 0X1848);
	write_cmos_sensor_twobyte(0X6F12, 0X3346);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C40);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0X5046);
	write_cmos_sensor_twobyte(0X6F12, 0X0094);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC0FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X82BA);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0023);
	write_cmos_sensor_twobyte(0X6F12, 0X03E0);
	write_cmos_sensor_twobyte(0X6F12, 0X51F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2340);
	write_cmos_sensor_twobyte(0X6F12, 0X6040);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1C);
	write_cmos_sensor_twobyte(0X6F12, 0XB3EB);
	write_cmos_sensor_twobyte(0X6F12, 0X920F);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D3);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0948);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC069);
	write_cmos_sensor_twobyte(0X6F12, 0X87B2);
	write_cmos_sensor_twobyte(0X6F12, 0X4FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X1048);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X034C);
	write_cmos_sensor_twobyte(0X6F12, 0X3068);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A11);
	write_cmos_sensor_twobyte(0X6F12, 0XF1B3);
	write_cmos_sensor_twobyte(0X6F12, 0X19E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF4D2);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X8000);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X9000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF110);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5810);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X6AF0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XE304);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X7000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4B);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X51D1);
	write_cmos_sensor_twobyte(0X6F12, 0XFD48);
	write_cmos_sensor_twobyte(0X6F12, 0XFD49);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0XF0C3);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2FF);
	write_cmos_sensor_twobyte(0X6F12, 0XE066);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0XF949);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XABFF);
	write_cmos_sensor_twobyte(0X6F12, 0X2067);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XF749);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XA5FF);
	write_cmos_sensor_twobyte(0X6F12, 0X6067);
	write_cmos_sensor_twobyte(0X6F12, 0X3168);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XE36D);
	write_cmos_sensor_twobyte(0X6F12, 0XE26E);
	write_cmos_sensor_twobyte(0X6F12, 0X0126);
	write_cmos_sensor_twobyte(0X6F12, 0X9A42);
	write_cmos_sensor_twobyte(0X6F12, 0X01D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X60C0);
	write_cmos_sensor_twobyte(0X6F12, 0X236F);
	write_cmos_sensor_twobyte(0X6F12, 0X6345);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X020C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X64C0);
	write_cmos_sensor_twobyte(0X6F12, 0X6045);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X040C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XBCF1);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X07D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X5A40);
	write_cmos_sensor_twobyte(0X6F12, 0X4240);
	write_cmos_sensor_twobyte(0X6F12, 0XA266);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A51);
	write_cmos_sensor_twobyte(0X6F12, 0X0AE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B61);
	write_cmos_sensor_twobyte(0X6F12, 0XF6E7);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XA566);
	write_cmos_sensor_twobyte(0X6F12, 0XE566);
	write_cmos_sensor_twobyte(0X6F12, 0X2567);
	write_cmos_sensor_twobyte(0X6F12, 0X6567);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X27FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0XD348);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FA);
	write_cmos_sensor_twobyte(0X6F12, 0XCF48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A01);
	write_cmos_sensor_twobyte(0X6F12, 0X48B3);
	write_cmos_sensor_twobyte(0X6F12, 0XC348);
	write_cmos_sensor_twobyte(0X6F12, 0XC44B);
	write_cmos_sensor_twobyte(0X6F12, 0XC44A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X20D1);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0DFA);
	write_cmos_sensor_twobyte(0X6F12, 0XC148);
	write_cmos_sensor_twobyte(0X6F12, 0XC149);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XC04F);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFF);
	write_cmos_sensor_twobyte(0X6F12, 0XC14C);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0XBD49);
	write_cmos_sensor_twobyte(0X6F12, 0XE065);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X32FF);
	write_cmos_sensor_twobyte(0X6F12, 0X2066);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XBA49);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFF);
	write_cmos_sensor_twobyte(0X6F12, 0X6066);
	write_cmos_sensor_twobyte(0X6F12, 0XD4E9);
	write_cmos_sensor_twobyte(0X6F12, 0X1712);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X4140);
	write_cmos_sensor_twobyte(0X6F12, 0XA165);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA1B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XB148);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X406A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDBF9);
	write_cmos_sensor_twobyte(0X6F12, 0XE068);
	write_cmos_sensor_twobyte(0X6F12, 0X07F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8047);
	write_cmos_sensor_twobyte(0X6F12, 0X8069);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0XC288);
	write_cmos_sensor_twobyte(0X6F12, 0X0389);
	write_cmos_sensor_twobyte(0X6F12, 0XA1EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5201);
	write_cmos_sensor_twobyte(0X6F12, 0X827B);
	write_cmos_sensor_twobyte(0X6F12, 0X891A);
	write_cmos_sensor_twobyte(0X6F12, 0X8289);
	write_cmos_sensor_twobyte(0X6F12, 0XC07B);
	write_cmos_sensor_twobyte(0X6F12, 0XA2EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5302);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0XB980);
	write_cmos_sensor_twobyte(0X6F12, 0XE268);
	write_cmos_sensor_twobyte(0X6F12, 0X491E);
	write_cmos_sensor_twobyte(0X6F12, 0X9269);
	write_cmos_sensor_twobyte(0X6F12, 0X128A);
	write_cmos_sensor_twobyte(0X6F12, 0X1144);
	write_cmos_sensor_twobyte(0X6F12, 0XF980);
	write_cmos_sensor_twobyte(0X6F12, 0X3881);
	write_cmos_sensor_twobyte(0X6F12, 0XE168);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X8969);
	write_cmos_sensor_twobyte(0X6F12, 0X498A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X7881);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BB9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8146);
	write_cmos_sensor_twobyte(0X6F12, 0X9648);
	write_cmos_sensor_twobyte(0X6F12, 0X0D46);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X816A);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0E0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X5CF9);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X4846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA6F9);
	write_cmos_sensor_twobyte(0X6F12, 0X8E48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9201);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X15D1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2900);
	write_cmos_sensor_twobyte(0X6F12, 0X70B1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCE02);
	write_cmos_sensor_twobyte(0X6F12, 0X08B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6F12, 0X07E0);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XDA00);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X8748);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5C00);
	write_cmos_sensor_twobyte(0X6F12, 0X10B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4210);
	write_cmos_sensor_twobyte(0X6F12, 0X88F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1110);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X33B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1001);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X83F9);
	write_cmos_sensor_twobyte(0X6F12, 0X774D);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XF006);
	write_cmos_sensor_twobyte(0X6F12, 0X05F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3405);
	write_cmos_sensor_twobyte(0X6F12, 0X1ED5);
	write_cmos_sensor_twobyte(0X6F12, 0X7748);
	write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XB207);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X03D2);
	write_cmos_sensor_twobyte(0X6F12, 0X2146);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X76F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X1346);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0X39B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0F88);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7870);
	write_cmos_sensor_twobyte(0X6F12, 0XC08A);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0X01E0);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7830);
	write_cmos_sensor_twobyte(0X6F12, 0X521C);
	write_cmos_sensor_twobyte(0X6F12, 0X062A);
	write_cmos_sensor_twobyte(0X6F12, 0XEED3);
	write_cmos_sensor_twobyte(0X6F12, 0X7004);
	write_cmos_sensor_twobyte(0X6F12, 0X1AD5);
	write_cmos_sensor_twobyte(0X6F12, 0XB5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8400);
	write_cmos_sensor_twobyte(0X6F12, 0X411E);
	write_cmos_sensor_twobyte(0X6F12, 0XA142);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X04D2);
	write_cmos_sensor_twobyte(0X6F12, 0X44F4);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X54F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4002);
	write_cmos_sensor_twobyte(0X6F12, 0X5189);
	write_cmos_sensor_twobyte(0X6F12, 0X29B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7820);
	write_cmos_sensor_twobyte(0X6F12, 0X12B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0A80);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X0628);
	write_cmos_sensor_twobyte(0X6F12, 0XF2D3);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF081);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X5448);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC16A);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDAF8);
	write_cmos_sensor_twobyte(0X6F12, 0X43F6);
	write_cmos_sensor_twobyte(0X6F12, 0XEF73);
	write_cmos_sensor_twobyte(0X6F12, 0X0322);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XB901);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X36F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X38F9);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC8B8);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X484C);
	write_cmos_sensor_twobyte(0X6F12, 0XE38B);
	write_cmos_sensor_twobyte(0X6F12, 0XCB1A);
	write_cmos_sensor_twobyte(0X6F12, 0X218C);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X0144);
	write_cmos_sensor_twobyte(0X6F12, 0X9942);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X0346);
	write_cmos_sensor_twobyte(0X6F12, 0X4149);
	write_cmos_sensor_twobyte(0X6F12, 0X3431);
	write_cmos_sensor_twobyte(0X6F12, 0X9040);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8630);
	write_cmos_sensor_twobyte(0X6F12, 0X03FA);
	write_cmos_sensor_twobyte(0X6F12, 0X02F3);
	write_cmos_sensor_twobyte(0X6F12, 0X414A);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XD181);
	write_cmos_sensor_twobyte(0X6F12, 0X1382);
	write_cmos_sensor_twobyte(0X6F12, 0X03D0);
	write_cmos_sensor_twobyte(0X6F12, 0X5181);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X9381);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF70);
	write_cmos_sensor_twobyte(0X6F12, 0X5081);
	write_cmos_sensor_twobyte(0X6F12, 0X9081);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7B71);
	write_cmos_sensor_twobyte(0X6F12, 0X3948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0FF9);
	write_cmos_sensor_twobyte(0X6F12, 0X334C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5371);
	write_cmos_sensor_twobyte(0X6F12, 0X2060);
	write_cmos_sensor_twobyte(0X6F12, 0X3648);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X07F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XFF61);
	write_cmos_sensor_twobyte(0X6F12, 0X6060);
	write_cmos_sensor_twobyte(0X6F12, 0X3448);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X00F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XA060);
	write_cmos_sensor_twobyte(0X6F12, 0X3148);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2D61);
	write_cmos_sensor_twobyte(0X6F12, 0XE060);
	write_cmos_sensor_twobyte(0X6F12, 0X2F48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XF551);
	write_cmos_sensor_twobyte(0X6F12, 0X2061);
	write_cmos_sensor_twobyte(0X6F12, 0X2C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XEBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5341);
	write_cmos_sensor_twobyte(0X6F12, 0X6061);
	write_cmos_sensor_twobyte(0X6F12, 0X2A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XA061);
	write_cmos_sensor_twobyte(0X6F12, 0X2021);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X6000);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8006);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3400);
	write_cmos_sensor_twobyte(0X6F12, 0X7580);
	write_cmos_sensor_twobyte(0X6F12, 0X4562);
	write_cmos_sensor_twobyte(0X6F12, 0X8562);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C50);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5050);
	write_cmos_sensor_twobyte(0X6F12, 0X2A46);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7D21);
	write_cmos_sensor_twobyte(0X6F12, 0X4565);
	write_cmos_sensor_twobyte(0X6F12, 0X1E48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XCBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2121);
	write_cmos_sensor_twobyte(0X6F12, 0X6062);
	write_cmos_sensor_twobyte(0X6F12, 0X1C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X4541);
	write_cmos_sensor_twobyte(0X6F12, 0XA062);
	write_cmos_sensor_twobyte(0X6F12, 0X1948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XBDF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F31);
	write_cmos_sensor_twobyte(0X6F12, 0XE061);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0XABBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0345);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0X1234);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5423);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0XFECA);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X7C80);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0B27);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0E5D);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X5C51);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X1F65);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X98A9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X852F);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XDE2F);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X10B3);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0823);
	write_cmos_sensor_twobyte(0X6F12, 0X0A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X87F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X2062);
	write_cmos_sensor_twobyte(0X6F12, 0X0848);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X6F11);
	write_cmos_sensor_twobyte(0X6F12, 0XE062);
	write_cmos_sensor_twobyte(0X6F12, 0X0548);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X79F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2063);
	write_cmos_sensor_twobyte(0X6F12, 0XF562);
	write_cmos_sensor_twobyte(0X6F12, 0X3563);
	write_cmos_sensor_twobyte(0X6F12, 0X7563);
	write_cmos_sensor_twobyte(0X6F12, 0X70BD);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X339B);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X2ABB);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X135F);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0XE91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X273C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X5D6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X033C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X3B2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X45F6);
	write_cmos_sensor_twobyte(0X6F12, 0X514C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X657C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D0C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X471C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X49F6);
	write_cmos_sensor_twobyte(0X6F12, 0XA90C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X48F2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X230C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X43F2);
	write_cmos_sensor_twobyte(0X6F12, 0X9B3C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3B7C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X4DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X2F6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB30C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X871C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X6F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1B5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0XF91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X42F6);
	write_cmos_sensor_twobyte(0X6F12, 0XBB2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB71C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X602A, 0X1150);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ah 680mV
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ch 680mV
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF466);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X111C);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X1120);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X602A, 0X113E);
	write_cmos_sensor_twobyte(0X6F12, 0X2101);
	write_cmos_sensor_twobyte(0X602A, 0X1106);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A28);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A52);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A48);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A34);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);
	write_cmos_sensor_twobyte(0X602A, 0X3A84);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// Short Red 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3AB6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// Short Blue 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3B44);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3B48);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X3B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0200);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0224);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0224h Address
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0226h Address
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X1124);
	write_cmos_sensor_twobyte(0X6F12, 0X0328);	// ADBUS data order
	write_cmos_sensor_twobyte(0X602A, 0X0ACE);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X602A, 0X09C0);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X09C2);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X6F12, 0X008C);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0XF408);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X54A2);
	write_cmos_sensor_twobyte(0X6F12, 0XF418);	// CDS_Option_L
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X000D);
	write_cmos_sensor_twobyte(0X6F12, 0XF472);	// CLP_Level
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF482);	// COMP_BIAS
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF484);	// COMP_BIAS
	write_cmos_sensor_twobyte(0X602A, 0X47F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FE);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);
	write_cmos_sensor_twobyte(0X602A, 0X1108);
	write_cmos_sensor_twobyte(0X6F12, 0X054C);	// F404h Def. 050Ch (Pxbst on & LP_VBLK_EN)
	write_cmos_sensor_twobyte(0X6F12, 0X0183);	// F406h Def. 0083h
	write_cmos_sensor_twobyte(0X6F12, 0X07FF);	// F408h Def. 07DFh (Dabx 0ff & Dsrx en)
	write_cmos_sensor_twobyte(0X602A, 0X1154);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);	// F418h LDB on x4 S3
	write_cmos_sensor_twobyte(0X6F12, 0X162F);	// F41Ah  // No Avg // CRSon
	write_cmos_sensor_twobyte(0X6F12, 0X4328);	// F41Ch
	write_cmos_sensor_twobyte(0X602A, 0X1126);
	write_cmos_sensor_twobyte(0X6F12, 0X0082);	// F4A6h  IPI signal dip control
	write_cmos_sensor_twobyte(0X602A, 0X114A);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_SenAnalog_rmp_option_top
	write_cmos_sensor_twobyte(0X602A, 0X114E);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_SenAnalog_rmp_option_bot
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF41E);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);	// DBUS option (Hybrid Gain)
	write_cmos_sensor_twobyte(0X602A, 0XF488);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// REF option
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X112A);
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
	write_cmos_sensor_twobyte(0X602A, 0X1128);
	write_cmos_sensor_twobyte(0X6F12, 0X001B);
	write_cmos_sensor_twobyte(0X602A, 0X10F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);
	write_cmos_sensor_twobyte(0X602A, 0X10F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);
	write_cmos_sensor_twobyte(0X602A, 0X10FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);
	write_cmos_sensor_twobyte(0X602A, 0X10FE);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF414);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X10C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X602A, 0X10B6);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X0B26);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X602A, 0X0B2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X602A, 0X0B36);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_sl_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0B52);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_sl_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0B56);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//aig_rx_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0B5A);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_rx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B5E);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_tx_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0B62);
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_tx_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0B66);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);	//aig_fx_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0B6A);
	write_cmos_sensor_twobyte(0X6F12, 0X0140);	//aig_fx_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0B6E);
	write_cmos_sensor_twobyte(0X6F12, 0X00EC);	//aig_dsrx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B76);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);	//aig_srx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B7E);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dstx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B86);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_stx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B8E);
	write_cmos_sensor_twobyte(0X6F12, 0X0047);	//aig_sfx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B96);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dabx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_abx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0BAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0101 //aig_sr_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BAE);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026E //aig_sr_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0106 //aig_ss_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026C //aig_ss_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_s1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BBE);
	write_cmos_sensor_twobyte(0X6F12, 0X0156);	//aig_s1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BC2);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_s3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BC6);
	write_cmos_sensor_twobyte(0X6F12, 0X0057);	//aig_s3_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BCA);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_s4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BCE);
	write_cmos_sensor_twobyte(0X6F12, 0X004E);	//aig_s4_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BD2);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_s4dmy_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_s4dmy_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_clp_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BDE);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_clp_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BE2);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_pixbst_p0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BE6);
	write_cmos_sensor_twobyte(0X6F12, 0X0016);	//aig_pixbst_p0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BEA);
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_pixbst_p1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BEE);
	write_cmos_sensor_twobyte(0X6F12, 0X014E);	//aig_pixbst_p1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BF2);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BF6);
	write_cmos_sensor_twobyte(0X6F12, 0X006B);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_vsl_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0BFE);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vsl_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C02);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_lp_hblk_cds_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C0A);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vload_smp_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C0E);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_smp_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C12);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);	//aig_vload_smpdmy_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C16);
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_smpdmy_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C1A);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	//aig_vbgr_smp_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C1E);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vbgr_smp_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C22);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vref_smp_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C26);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vref_smp_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C2A);
	write_cmos_sensor_twobyte(0X6F12, 0X00D7);	//aig_cnt_en_p0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_cnt_en_p0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C32);
	write_cmos_sensor_twobyte(0X6F12, 0X01D6);	//aig_cnt_en_p1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C36);
	write_cmos_sensor_twobyte(0X6F12, 0X026A);	//aig_cnt_en_p1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C3A);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_enb_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_conv_enb_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C42);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_conv1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C46);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C4A);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);	//aig_conv2_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C52);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lsb_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C56);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lsb_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C62);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);	//aig_conv_lsb_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0C66);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_lsb_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C72);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_rst_div_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0C76);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_rst_div_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C7A);
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//aig_comp_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C7E);
	write_cmos_sensor_twobyte(0X6F12, 0X0104);	//aig_comp_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C82);
	write_cmos_sensor_twobyte(0X6F12, 0X01D4);	//aig_comp_en_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0C86);
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_comp_en_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C92);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);	//aig_rd_en_10_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C96);
	write_cmos_sensor_twobyte(0X6F12, 0X001C);	//aig_rd_en_10_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C9A);
	write_cmos_sensor_twobyte(0X6F12, 0X001E);	//aig_rd_en_9_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0038);	//aig_rd_en_9_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CA2);
	write_cmos_sensor_twobyte(0X6F12, 0X003A);	//aig_rd_en_8_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0054);	//aig_rd_en_8_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0076);	//aig_rd_en_7_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CAE);
	write_cmos_sensor_twobyte(0X6F12, 0X0090);	//aig_rd_en_7_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CB2);
	write_cmos_sensor_twobyte(0X6F12, 0X010B);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0127);	//aig_rd_en_5_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CBE);
	write_cmos_sensor_twobyte(0X6F12, 0X0141);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CC2);
	write_cmos_sensor_twobyte(0X6F12, 0X0143);	//aig_rd_en_4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CC6);
	write_cmos_sensor_twobyte(0X6F12, 0X015D);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CCA);
	write_cmos_sensor_twobyte(0X6F12, 0X015F);	//aig_rd_en_3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CCE);
	write_cmos_sensor_twobyte(0X6F12, 0X0179);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CD2);
	write_cmos_sensor_twobyte(0X6F12, 0X017B);	//aig_rd_en_2_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0195);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0197);	//aig_rd_en_1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CDE);
	write_cmos_sensor_twobyte(0X6F12, 0X01B1);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CE2);
	write_cmos_sensor_twobyte(0X6F12, 0X01B3);	//aig_rd_en_0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CE6);
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0CEE);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0CF2);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0CF6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0CFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr4
	write_cmos_sensor_twobyte(0X602A, 0X0CFE);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr5
	write_cmos_sensor_twobyte(0X602A, 0X0D02);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr6
	write_cmos_sensor_twobyte(0X602A, 0X0D06);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr7
	write_cmos_sensor_twobyte(0X602A, 0X0D0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr8
	write_cmos_sensor_twobyte(0X602A, 0X0D0E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr9
	write_cmos_sensor_twobyte(0X602A, 0X0D12);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr10
	write_cmos_sensor_twobyte(0X602A, 0X0D16);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr11
	write_cmos_sensor_twobyte(0X602A, 0X0D22);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_en_offset_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0D26);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_en_offset_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D32);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lsb_offset_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0D36);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lsb_offset_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D3A);
	write_cmos_sensor_twobyte(0X6F12, 0X006F);	//aig_off_rst_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0270);	//aig_off_rst_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D42);
	write_cmos_sensor_twobyte(0X6F12, 0X0071);	//aig_rmp_rst_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0073);	//aig_rmp_rst_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D52);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);	//aig_rmp_rst_ptr2
	write_cmos_sensor_twobyte(0X602A, 0X0D56);
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rmp_rst_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D5A);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_rmp_rst_ptr4
	write_cmos_sensor_twobyte(0X602A, 0X0D5E);
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_rmp_rst_ptr5
	write_cmos_sensor_twobyte(0X602A, 0X0D62);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);	//aig_rmp_mode_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D66);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);	//aig_rvs_en_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D6A);
	write_cmos_sensor_twobyte(0X6F12, 0X0264);	//aig_rvs_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D76);
	write_cmos_sensor_twobyte(0X6F12, 0X0014);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D7A);
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_optl0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D96);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);	//aig_optl1_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D9A);
	write_cmos_sensor_twobyte(0X6F12, 0X0132);	//aig_optl1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_optl2_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0DBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_optl2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);	//aig_optl3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0E16);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_opth0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0E1A);
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_opth0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0FA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0287);	//VIR_1_FINE_STX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FAA);
	write_cmos_sensor_twobyte(0X6F12, 0X003C);	//VIR_1_FINE_DSTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0124);	//VIR_2_FINE_DSTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FB6);
	write_cmos_sensor_twobyte(0X6F12, 0X00DF);	//VIR_MSO_PTR0_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0296);	//VIR_MIN_ADC_COLS_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0301);
	write_cmos_sensor_twobyte(0X602A, 0X0FEC);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0FD8);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0FDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);
	write_cmos_sensor_twobyte(0X602A, 0X104C);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);
	write_cmos_sensor_twobyte(0X602A, 0X106C);
	write_cmos_sensor_twobyte(0X6F12, 0X0607);
	write_cmos_sensor_twobyte(0X602A, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0003);
	write_cmos_sensor_twobyte(0X602A, 0X101C);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X101E);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1020);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1022);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1024);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1026);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X602A, 0X1028);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X602A, 0X10AE);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X10B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X0B0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B52);
	write_cmos_sensor_twobyte(0X6F12, 0X0579);	//aig_sl_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0BFE);
	write_cmos_sensor_twobyte(0X6F12, 0X0579);	//aig_vsl_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C0E);
	write_cmos_sensor_twobyte(0X6F12, 0X0579);	//aig_vload_smp_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C16);
	write_cmos_sensor_twobyte(0X6F12, 0X0579);	//aig_vload_smpdmy_ptr1
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3064);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0B02);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X0ABC);
	write_cmos_sensor_twobyte(0X6F12, 0X0207);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X070C);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X602A, 0X0110);
	write_cmos_sensor_twobyte(0X6F12, 0X1002);
	write_cmos_sensor_twobyte(0X602A, 0X0114);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X602A, 0X0408);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X602A, 0X0344);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X6F12, 0X1F6F);
	write_cmos_sensor_twobyte(0X6F12, 0X0BC7);
	write_cmos_sensor_twobyte(0X6F12, 0X0Fb0);
	write_cmos_sensor_twobyte(0X6F12, 0X0Bc0);
	write_cmos_sensor_twobyte(0X602A, 0X0380);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0900);
	write_cmos_sensor_twobyte(0X6F12, 0X0221);
	write_cmos_sensor_twobyte(0X602A, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0404);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0304);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X030C);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0306);
	write_cmos_sensor_twobyte(0X6F12, 0X01F4);	// 500
	write_cmos_sensor_twobyte(0X602A, 0X0302);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X0005);
	write_cmos_sensor_twobyte(0X602A, 0X0312);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X030E);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6F12, 0X0153);
	write_cmos_sensor_twobyte(0X602A, 0X030A);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0308);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);
	write_cmos_sensor_twobyte(0X602A, 0X0342);
	write_cmos_sensor_twobyte(0X6F12, 0X1118);
	write_cmos_sensor_twobyte(0X602A, 0X0340);
	write_cmos_sensor_twobyte(0X6F12, 0X17CD);
	write_cmos_sensor_twobyte(0X602A, 0X021E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X30A6);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X30A8);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X30CA);
	write_cmos_sensor_twobyte(0X6F12, 0X0031);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A70);
	write_cmos_sensor_twobyte(0X6F12, 0X6E33);
	write_cmos_sensor_twobyte(0X602A, 0X4814);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X4876);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X2412);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);	//4x4 msm
	write_cmos_sensor_twobyte(0X602A, 0X3BE8);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//gb
	write_cmos_sensor_twobyte(0X602A, 0X8724);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writes on STBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writes on STBY
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A30);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X30CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0702);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X228E);
	write_cmos_sensor_twobyte(0X6F12, 0X0245);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X602A, 0X8726);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X872A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X872C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X872E);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E0);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E2);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//settings for test
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//settings for test
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X4D70);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D80);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D90);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0B04);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X30A2);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X5400);
	write_cmos_sensor_twobyte(0X602A, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
//}}} config }}}
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

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//Do not uwrite_cmos_sensor_twobyte(0Xe Memory retention
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6010);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	// Rewrite_cmos_sensor_twobyte(0Xet

	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X7970);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X7150);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X0136);
	write_cmos_sensor_twobyte(0X6F12, 0X1800);	// 24

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X7D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0449);
	write_cmos_sensor_twobyte(0X6F12, 0X0348);
	write_cmos_sensor_twobyte(0X6F12, 0X044A);
	write_cmos_sensor_twobyte(0X6F12, 0X4860);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0881);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC2BB);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X87AC);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5830);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XA3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0168);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X57FC);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X59FC);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0XC28E);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB1);
	write_cmos_sensor_twobyte(0X6F12, 0XF948);
	write_cmos_sensor_twobyte(0X6F12, 0X0280);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X48BC);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0XF148);
	write_cmos_sensor_twobyte(0X6F12, 0X8A46);
	write_cmos_sensor_twobyte(0X6F12, 0X9146);
	write_cmos_sensor_twobyte(0X6F12, 0X4168);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X39FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0027);
	write_cmos_sensor_twobyte(0X6F12, 0XA780);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X5146);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFC);
	write_cmos_sensor_twobyte(0X6F12, 0X98F8);
	write_cmos_sensor_twobyte(0X6F12, 0XE200);
	write_cmos_sensor_twobyte(0X6F12, 0XE64A);
	write_cmos_sensor_twobyte(0X6F12, 0XE749);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD9);
	write_cmos_sensor_twobyte(0X6F12, 0XE648);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6C09);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E01);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF087);
	write_cmos_sensor_twobyte(0X6F12, 0XA2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X6E71);
	write_cmos_sensor_twobyte(0X6F12, 0XF8E7);
	write_cmos_sensor_twobyte(0X6F12, 0XFEB5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XD848);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X8168);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFC);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X17FC);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X03FC);
	write_cmos_sensor_twobyte(0X6F12, 0XD048);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X0029);
	write_cmos_sensor_twobyte(0X6F12, 0X1CD0);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XD900);
	write_cmos_sensor_twobyte(0X6F12, 0X8DF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0XCB48);
	write_cmos_sensor_twobyte(0X6F12, 0X0292);
	write_cmos_sensor_twobyte(0X6F12, 0X0192);
	write_cmos_sensor_twobyte(0X6F12, 0X4A30);
	write_cmos_sensor_twobyte(0X6F12, 0XCA00);
	write_cmos_sensor_twobyte(0X6F12, 0XC17D);
	write_cmos_sensor_twobyte(0X6F12, 0XCD4C);
	write_cmos_sensor_twobyte(0X6F12, 0XC91D);
	write_cmos_sensor_twobyte(0X6F12, 0XC908);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0123);
	write_cmos_sensor_twobyte(0X6F12, 0X43EA);
	write_cmos_sensor_twobyte(0X6F12, 0X4101);
	write_cmos_sensor_twobyte(0X6F12, 0X6180);
	write_cmos_sensor_twobyte(0X6F12, 0X0188);
	write_cmos_sensor_twobyte(0X6F12, 0X6B46);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF2);
	write_cmos_sensor_twobyte(0X6F12, 0X1010);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF7FB);
	write_cmos_sensor_twobyte(0X6F12, 0X0C20);
	write_cmos_sensor_twobyte(0X6F12, 0X2080);
	write_cmos_sensor_twobyte(0X6F12, 0XFEBD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC068);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD3FB);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE8FB);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0XC18A);
	write_cmos_sensor_twobyte(0X6F12, 0XBD48);
	write_cmos_sensor_twobyte(0X6F12, 0X0268);
	write_cmos_sensor_twobyte(0X6F12, 0X02F5);
	write_cmos_sensor_twobyte(0X6F12, 0XC972);
	write_cmos_sensor_twobyte(0X6F12, 0X907A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0XBB49);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0XB4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF600);
	write_cmos_sensor_twobyte(0X6F12, 0X917A);
	write_cmos_sensor_twobyte(0X6F12, 0X411A);
	write_cmos_sensor_twobyte(0X6F12, 0XB948);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X34F8);
	write_cmos_sensor_twobyte(0X6F12, 0XFA1F);
	write_cmos_sensor_twobyte(0X6F12, 0X638A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X937A);
	write_cmos_sensor_twobyte(0X6F12, 0X1944);
	write_cmos_sensor_twobyte(0X6F12, 0X8180);
	write_cmos_sensor_twobyte(0X6F12, 0X1078);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X0AD1);
	write_cmos_sensor_twobyte(0X6F12, 0XAE48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1115);
	write_cmos_sensor_twobyte(0X6F12, 0XB248);
	write_cmos_sensor_twobyte(0X6F12, 0X19B1);
	write_cmos_sensor_twobyte(0X6F12, 0XA749);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5D10);
	write_cmos_sensor_twobyte(0X6F12, 0X491C);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCC10);
	write_cmos_sensor_twobyte(0X6F12, 0XA348);
	write_cmos_sensor_twobyte(0X6F12, 0XB830);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X4188);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X9FBB);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0X9D48);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X0169);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8EB2);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XAFFB);
	write_cmos_sensor_twobyte(0X6F12, 0X9C48);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1202);
	write_cmos_sensor_twobyte(0X6F12, 0X40B1);
	write_cmos_sensor_twobyte(0X6F12, 0X9D49);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X0968);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0XAA02);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X7DBB);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4F);
	write_cmos_sensor_twobyte(0X6F12, 0X8046);
	write_cmos_sensor_twobyte(0X6F12, 0X8B48);
	write_cmos_sensor_twobyte(0X6F12, 0X81B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X4069);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X80B2);
	write_cmos_sensor_twobyte(0X6F12, 0X0091);
	write_cmos_sensor_twobyte(0X6F12, 0X8346);
	write_cmos_sensor_twobyte(0X6F12, 0X0146);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6EFB);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0X1492);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF0);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);
	write_cmos_sensor_twobyte(0X6F12, 0X09F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3409);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X28A0);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X24A0);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X50A0);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0600);
	write_cmos_sensor_twobyte(0X6F12, 0X0028);
	write_cmos_sensor_twobyte(0X6F12, 0X76D0);
	write_cmos_sensor_twobyte(0X6F12, 0X8948);
	write_cmos_sensor_twobyte(0X6F12, 0X0026);
	write_cmos_sensor_twobyte(0X6F12, 0X3746);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9D02);
	write_cmos_sensor_twobyte(0X6F12, 0XD8B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0024);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8405);
	write_cmos_sensor_twobyte(0X6F12, 0X296E);
	write_cmos_sensor_twobyte(0X6F12, 0X81B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0A22);
	write_cmos_sensor_twobyte(0X6F12, 0X286A);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X72FB);
	write_cmos_sensor_twobyte(0X6F12, 0X08EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4011);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XC021);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0XF001);
	write_cmos_sensor_twobyte(0X6F12, 0X5143);
	write_cmos_sensor_twobyte(0X6F12, 0X4031);
	write_cmos_sensor_twobyte(0X6F12, 0X00EB);
	write_cmos_sensor_twobyte(0X6F12, 0XE110);
	write_cmos_sensor_twobyte(0X6F12, 0X0644);
	write_cmos_sensor_twobyte(0X6F12, 0XD5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X641C);
	write_cmos_sensor_twobyte(0X6F12, 0X0744);
	write_cmos_sensor_twobyte(0X6F12, 0X102C);
	write_cmos_sensor_twobyte(0X6F12, 0XE4D3);
	write_cmos_sensor_twobyte(0X6F12, 0XF817);
	write_cmos_sensor_twobyte(0X6F12, 0X07EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0511);
	write_cmos_sensor_twobyte(0X6F12, 0XF017);
	write_cmos_sensor_twobyte(0X6F12, 0X06EB);
	write_cmos_sensor_twobyte(0X6F12, 0X1070);
	write_cmos_sensor_twobyte(0X6F12, 0X0411);
	write_cmos_sensor_twobyte(0X6F12, 0X99F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0700);
	write_cmos_sensor_twobyte(0X6F12, 0X4A46);
	write_cmos_sensor_twobyte(0X6F12, 0X0428);
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);
	write_cmos_sensor_twobyte(0X6F12, 0X654E);
	write_cmos_sensor_twobyte(0X6F12, 0XD071);
	write_cmos_sensor_twobyte(0X6F12, 0X8236);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X2ADD);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X08D0);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0111);
	write_cmos_sensor_twobyte(0X6F12, 0X01EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8001);
	write_cmos_sensor_twobyte(0X6F12, 0X8B6A);
	write_cmos_sensor_twobyte(0X6F12, 0XCB62);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C00);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0XC462);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C10);
	write_cmos_sensor_twobyte(0X6F12, 0X92F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0730);
	write_cmos_sensor_twobyte(0X6F12, 0X8857);
	write_cmos_sensor_twobyte(0X6F12, 0X9842);
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X8855);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C30);
	write_cmos_sensor_twobyte(0X6F12, 0X9957);
	write_cmos_sensor_twobyte(0X6F12, 0X0129);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDD);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X02EB);
	write_cmos_sensor_twobyte(0X6F12, 0X0313);
	write_cmos_sensor_twobyte(0X6F12, 0X04E0);
	write_cmos_sensor_twobyte(0X6F12, 0X03EB);
	write_cmos_sensor_twobyte(0X6F12, 0X8007);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XFF6A);
	write_cmos_sensor_twobyte(0X6F12, 0X3C44);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D8);
	write_cmos_sensor_twobyte(0X6F12, 0X94FB);
	write_cmos_sensor_twobyte(0X6F12, 0XF1F4);
	write_cmos_sensor_twobyte(0X6F12, 0X99F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0800);
	write_cmos_sensor_twobyte(0X6F12, 0X50B3);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X14DB);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X10DB);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X1BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FB);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X21DB);
	write_cmos_sensor_twobyte(0X6F12, 0X22E0);
	write_cmos_sensor_twobyte(0X6F12, 0X34E0);
	write_cmos_sensor_twobyte(0X6F12, 0X002C);
	write_cmos_sensor_twobyte(0X6F12, 0X11DC);
	write_cmos_sensor_twobyte(0X6F12, 0X002D);
	write_cmos_sensor_twobyte(0X6F12, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0BFB);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0210);
	write_cmos_sensor_twobyte(0X6F12, 0X00FB);
	write_cmos_sensor_twobyte(0X6F12, 0X01F7);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XFEFA);
	write_cmos_sensor_twobyte(0X6F12, 0XB7EB);
	write_cmos_sensor_twobyte(0X6F12, 0X002F);
	write_cmos_sensor_twobyte(0X6F12, 0X0CDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X0DE0);
	write_cmos_sensor_twobyte(0X6F12, 0XAC42);
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);
	write_cmos_sensor_twobyte(0X6F12, 0X601B);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X281B);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X8142);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X18B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X4C20);
	write_cmos_sensor_twobyte(0X6F12, 0X02F8);
	write_cmos_sensor_twobyte(0X6F12, 0X06A0);
	write_cmos_sensor_twobyte(0X6F12, 0X0299);
	write_cmos_sensor_twobyte(0X6F12, 0X8968);
	write_cmos_sensor_twobyte(0X6F12, 0X4A78);
	write_cmos_sensor_twobyte(0X6F12, 0X0243);
	write_cmos_sensor_twobyte(0X6F12, 0X4A70);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0XC1E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0954);
	write_cmos_sensor_twobyte(0X6F12, 0X89F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5000);
	write_cmos_sensor_twobyte(0X6F12, 0X6C40);
	write_cmos_sensor_twobyte(0X6F12, 0XC9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5440);
	write_cmos_sensor_twobyte(0X6F12, 0X02AC);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X0ECC);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDEFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0098);
	write_cmos_sensor_twobyte(0X6F12, 0X05B0);
	write_cmos_sensor_twobyte(0X6F12, 0X5946);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF04F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA4BA);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X8246);
	write_cmos_sensor_twobyte(0X6F12, 0X1F48);
	write_cmos_sensor_twobyte(0X6F12, 0X8946);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X8169);
	write_cmos_sensor_twobyte(0X6F12, 0X1E46);
	write_cmos_sensor_twobyte(0X6F12, 0X0D0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0A9C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94FA);
	write_cmos_sensor_twobyte(0X6F12, 0X1848);
	write_cmos_sensor_twobyte(0X6F12, 0X3346);
	write_cmos_sensor_twobyte(0X6F12, 0X3430);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C40);
	write_cmos_sensor_twobyte(0X6F12, 0X4946);
	write_cmos_sensor_twobyte(0X6F12, 0X5046);
	write_cmos_sensor_twobyte(0X6F12, 0X0094);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC0FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF84F);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X82BA);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0023);
	write_cmos_sensor_twobyte(0X6F12, 0X03E0);
	write_cmos_sensor_twobyte(0X6F12, 0X51F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2340);
	write_cmos_sensor_twobyte(0X6F12, 0X6040);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1C);
	write_cmos_sensor_twobyte(0X6F12, 0XB3EB);
	write_cmos_sensor_twobyte(0X6F12, 0X920F);
	write_cmos_sensor_twobyte(0X6F12, 0XF8D3);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0948);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC069);
	write_cmos_sensor_twobyte(0X6F12, 0X87B2);
	write_cmos_sensor_twobyte(0X6F12, 0X4FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X1048);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BFA);
	write_cmos_sensor_twobyte(0X6F12, 0X0C4E);
	write_cmos_sensor_twobyte(0X6F12, 0X034C);
	write_cmos_sensor_twobyte(0X6F12, 0X3068);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A11);
	write_cmos_sensor_twobyte(0X6F12, 0XF1B3);
	write_cmos_sensor_twobyte(0X6F12, 0X19E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF4D2);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X8000);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X9000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF110);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5810);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X6AF0);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XE304);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0X7000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0XFE48);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4B);
	write_cmos_sensor_twobyte(0X6F12, 0XFF4A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X51D1);
	write_cmos_sensor_twobyte(0X6F12, 0XFD48);
	write_cmos_sensor_twobyte(0X6F12, 0XFD49);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XDFF8);
	write_cmos_sensor_twobyte(0X6F12, 0XF0C3);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XB2FF);
	write_cmos_sensor_twobyte(0X6F12, 0XE066);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0XF949);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XABFF);
	write_cmos_sensor_twobyte(0X6F12, 0X2067);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XF749);
	write_cmos_sensor_twobyte(0X6F12, 0X6046);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0XA5FF);
	write_cmos_sensor_twobyte(0X6F12, 0X6067);
	write_cmos_sensor_twobyte(0X6F12, 0X3168);
	write_cmos_sensor_twobyte(0X6F12, 0X00E0);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XE36D);
	write_cmos_sensor_twobyte(0X6F12, 0XE26E);
	write_cmos_sensor_twobyte(0X6F12, 0X0126);
	write_cmos_sensor_twobyte(0X6F12, 0X9A42);
	write_cmos_sensor_twobyte(0X6F12, 0X01D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X60C0);
	write_cmos_sensor_twobyte(0X6F12, 0X236F);
	write_cmos_sensor_twobyte(0X6F12, 0X6345);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X020C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XD4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X64C0);
	write_cmos_sensor_twobyte(0X6F12, 0X6045);
	write_cmos_sensor_twobyte(0X6F12, 0X05D0);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X4CF0);
	write_cmos_sensor_twobyte(0X6F12, 0X040C);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0X91F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9FC1);
	write_cmos_sensor_twobyte(0X6F12, 0XBCF1);
	write_cmos_sensor_twobyte(0X6F12, 0X000F);
	write_cmos_sensor_twobyte(0X6F12, 0X07D0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X5A40);
	write_cmos_sensor_twobyte(0X6F12, 0X4240);
	write_cmos_sensor_twobyte(0X6F12, 0XA266);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A51);
	write_cmos_sensor_twobyte(0X6F12, 0X0AE0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B61);
	write_cmos_sensor_twobyte(0X6F12, 0XF6E7);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9B51);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9F51);
	write_cmos_sensor_twobyte(0X6F12, 0XA566);
	write_cmos_sensor_twobyte(0X6F12, 0XE566);
	write_cmos_sensor_twobyte(0X6F12, 0X2567);
	write_cmos_sensor_twobyte(0X6F12, 0X6567);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X27FA);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X4046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0XD348);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X006A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XD9F9);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X18FA);
	write_cmos_sensor_twobyte(0X6F12, 0XCF48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9A01);
	write_cmos_sensor_twobyte(0X6F12, 0X48B3);
	write_cmos_sensor_twobyte(0X6F12, 0XC348);
	write_cmos_sensor_twobyte(0X6F12, 0XC44B);
	write_cmos_sensor_twobyte(0X6F12, 0XC44A);
	write_cmos_sensor_twobyte(0X6F12, 0XD0E9);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);
	write_cmos_sensor_twobyte(0X6F12, 0X5840);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X0843);
	write_cmos_sensor_twobyte(0X6F12, 0X20D1);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0DFA);
	write_cmos_sensor_twobyte(0X6F12, 0XC148);
	write_cmos_sensor_twobyte(0X6F12, 0XC149);
	write_cmos_sensor_twobyte(0X6F12, 0X20F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X421A);
	write_cmos_sensor_twobyte(0X6F12, 0XC04F);
	write_cmos_sensor_twobyte(0X6F12, 0X403A);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X3BFF);
	write_cmos_sensor_twobyte(0X6F12, 0XC14C);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0X4432);
	write_cmos_sensor_twobyte(0X6F12, 0X3434);
	write_cmos_sensor_twobyte(0X6F12, 0XBD49);
	write_cmos_sensor_twobyte(0X6F12, 0XE065);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X32FF);
	write_cmos_sensor_twobyte(0X6F12, 0X2066);
	write_cmos_sensor_twobyte(0X6F12, 0X8C22);
	write_cmos_sensor_twobyte(0X6F12, 0XBA49);
	write_cmos_sensor_twobyte(0X6F12, 0X3846);
	write_cmos_sensor_twobyte(0X6F12, 0XFFF7);
	write_cmos_sensor_twobyte(0X6F12, 0X2CFF);
	write_cmos_sensor_twobyte(0X6F12, 0X6066);
	write_cmos_sensor_twobyte(0X6F12, 0XD4E9);
	write_cmos_sensor_twobyte(0X6F12, 0X1712);
	write_cmos_sensor_twobyte(0X6F12, 0X5140);
	write_cmos_sensor_twobyte(0X6F12, 0X4140);
	write_cmos_sensor_twobyte(0X6F12, 0XA165);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA1B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XB148);
	write_cmos_sensor_twobyte(0X6F12, 0X0F46);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X406A);
	write_cmos_sensor_twobyte(0X6F12, 0X86B2);
	write_cmos_sensor_twobyte(0X6F12, 0X050C);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X94F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDBF9);
	write_cmos_sensor_twobyte(0X6F12, 0XE068);
	write_cmos_sensor_twobyte(0X6F12, 0X07F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8047);
	write_cmos_sensor_twobyte(0X6F12, 0X8069);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0XC288);
	write_cmos_sensor_twobyte(0X6F12, 0X0389);
	write_cmos_sensor_twobyte(0X6F12, 0XA1EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5201);
	write_cmos_sensor_twobyte(0X6F12, 0X827B);
	write_cmos_sensor_twobyte(0X6F12, 0X891A);
	write_cmos_sensor_twobyte(0X6F12, 0X8289);
	write_cmos_sensor_twobyte(0X6F12, 0XC07B);
	write_cmos_sensor_twobyte(0X6F12, 0XA2EB);
	write_cmos_sensor_twobyte(0X6F12, 0X5302);
	write_cmos_sensor_twobyte(0X6F12, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0XB980);
	write_cmos_sensor_twobyte(0X6F12, 0XE268);
	write_cmos_sensor_twobyte(0X6F12, 0X491E);
	write_cmos_sensor_twobyte(0X6F12, 0X9269);
	write_cmos_sensor_twobyte(0X6F12, 0X128A);
	write_cmos_sensor_twobyte(0X6F12, 0X1144);
	write_cmos_sensor_twobyte(0X6F12, 0XF980);
	write_cmos_sensor_twobyte(0X6F12, 0X3881);
	write_cmos_sensor_twobyte(0X6F12, 0XE168);
	write_cmos_sensor_twobyte(0X6F12, 0X401E);
	write_cmos_sensor_twobyte(0X6F12, 0X8969);
	write_cmos_sensor_twobyte(0X6F12, 0X498A);
	write_cmos_sensor_twobyte(0X6F12, 0X0844);
	write_cmos_sensor_twobyte(0X6F12, 0X7881);
	write_cmos_sensor_twobyte(0X6F12, 0X3146);
	write_cmos_sensor_twobyte(0X6F12, 0X2846);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X6BB9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X8146);
	write_cmos_sensor_twobyte(0X6F12, 0X9648);
	write_cmos_sensor_twobyte(0X6F12, 0X0D46);
	write_cmos_sensor_twobyte(0X6F12, 0X9046);
	write_cmos_sensor_twobyte(0X6F12, 0X816A);
	write_cmos_sensor_twobyte(0X6F12, 0X1C46);
	write_cmos_sensor_twobyte(0X6F12, 0X0E0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X5CF9);
	write_cmos_sensor_twobyte(0X6F12, 0X2346);
	write_cmos_sensor_twobyte(0X6F12, 0X4246);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X4846);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XA6F9);
	write_cmos_sensor_twobyte(0X6F12, 0X8E48);
	write_cmos_sensor_twobyte(0X6F12, 0X0068);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X9201);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);
	write_cmos_sensor_twobyte(0X6F12, 0X15D1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2900);
	write_cmos_sensor_twobyte(0X6F12, 0X70B1);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XCE02);
	write_cmos_sensor_twobyte(0X6F12, 0X08B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);
	write_cmos_sensor_twobyte(0X6F12, 0X07E0);
	write_cmos_sensor_twobyte(0X6F12, 0X95F8);
	write_cmos_sensor_twobyte(0X6F12, 0XDA00);
	write_cmos_sensor_twobyte(0X6F12, 0X30B1);
	write_cmos_sensor_twobyte(0X6F12, 0X8748);
	write_cmos_sensor_twobyte(0X6F12, 0X90F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5C00);
	write_cmos_sensor_twobyte(0X6F12, 0X10B1);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);
	write_cmos_sensor_twobyte(0X6F12, 0X84F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X94F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4210);
	write_cmos_sensor_twobyte(0X6F12, 0X88F8);
	write_cmos_sensor_twobyte(0X6F12, 0X1110);
	write_cmos_sensor_twobyte(0X6F12, 0X3946);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF047);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X33B9);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE9);
	write_cmos_sensor_twobyte(0X6F12, 0XF041);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1001);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X81F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X83F9);
	write_cmos_sensor_twobyte(0X6F12, 0X774D);
	write_cmos_sensor_twobyte(0X6F12, 0X0446);
	write_cmos_sensor_twobyte(0X6F12, 0XF006);
	write_cmos_sensor_twobyte(0X6F12, 0X05F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3405);
	write_cmos_sensor_twobyte(0X6F12, 0X1ED5);
	write_cmos_sensor_twobyte(0X6F12, 0X7748);
	write_cmos_sensor_twobyte(0X6F12, 0XB0F8);
	write_cmos_sensor_twobyte(0X6F12, 0XB207);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X03D2);
	write_cmos_sensor_twobyte(0X6F12, 0X2146);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X76F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0X1346);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4200);
	write_cmos_sensor_twobyte(0X6F12, 0X4189);
	write_cmos_sensor_twobyte(0X6F12, 0X39B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0F88);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7870);
	write_cmos_sensor_twobyte(0X6F12, 0XC08A);
	write_cmos_sensor_twobyte(0X6F12, 0X0880);
	write_cmos_sensor_twobyte(0X6F12, 0X01E0);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7830);
	write_cmos_sensor_twobyte(0X6F12, 0X521C);
	write_cmos_sensor_twobyte(0X6F12, 0X062A);
	write_cmos_sensor_twobyte(0X6F12, 0XEED3);
	write_cmos_sensor_twobyte(0X6F12, 0X7004);
	write_cmos_sensor_twobyte(0X6F12, 0X1AD5);
	write_cmos_sensor_twobyte(0X6F12, 0XB5F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8400);
	write_cmos_sensor_twobyte(0X6F12, 0X411E);
	write_cmos_sensor_twobyte(0X6F12, 0XA142);
	write_cmos_sensor_twobyte(0X6F12, 0X02D8);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0XA042);
	write_cmos_sensor_twobyte(0X6F12, 0X04D2);
	write_cmos_sensor_twobyte(0X6F12, 0X44F4);
	write_cmos_sensor_twobyte(0X6F12, 0X0041);
	write_cmos_sensor_twobyte(0X6F12, 0X1A20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X54F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X05EB);
	write_cmos_sensor_twobyte(0X6F12, 0X4002);
	write_cmos_sensor_twobyte(0X6F12, 0X5189);
	write_cmos_sensor_twobyte(0X6F12, 0X29B1);
	write_cmos_sensor_twobyte(0X6F12, 0XB2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X7820);
	write_cmos_sensor_twobyte(0X6F12, 0X12B1);
	write_cmos_sensor_twobyte(0X6F12, 0X01F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8041);
	write_cmos_sensor_twobyte(0X6F12, 0X0A80);
	write_cmos_sensor_twobyte(0X6F12, 0X401C);
	write_cmos_sensor_twobyte(0X6F12, 0X0628);
	write_cmos_sensor_twobyte(0X6F12, 0XF2D3);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0XF081);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0646);
	write_cmos_sensor_twobyte(0X6F12, 0X5448);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XC16A);
	write_cmos_sensor_twobyte(0X6F12, 0X0C0C);
	write_cmos_sensor_twobyte(0X6F12, 0X8DB2);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XDAF8);
	write_cmos_sensor_twobyte(0X6F12, 0X43F6);
	write_cmos_sensor_twobyte(0X6F12, 0XEF73);
	write_cmos_sensor_twobyte(0X6F12, 0X0322);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XB901);
	write_cmos_sensor_twobyte(0X6F12, 0X0F20);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X36F9);
	write_cmos_sensor_twobyte(0X6F12, 0X3046);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X38F9);
	write_cmos_sensor_twobyte(0X6F12, 0X2946);
	write_cmos_sensor_twobyte(0X6F12, 0X2046);
	write_cmos_sensor_twobyte(0X6F12, 0XBDE8);
	write_cmos_sensor_twobyte(0X6F12, 0X7040);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC8B8);
	write_cmos_sensor_twobyte(0X6F12, 0X10B5);
	write_cmos_sensor_twobyte(0X6F12, 0X484C);
	write_cmos_sensor_twobyte(0X6F12, 0XE38B);
	write_cmos_sensor_twobyte(0X6F12, 0XCB1A);
	write_cmos_sensor_twobyte(0X6F12, 0X218C);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X0144);
	write_cmos_sensor_twobyte(0X6F12, 0X9942);
	write_cmos_sensor_twobyte(0X6F12, 0X01D9);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6F12, 0X0346);
	write_cmos_sensor_twobyte(0X6F12, 0X4149);
	write_cmos_sensor_twobyte(0X6F12, 0X3431);
	write_cmos_sensor_twobyte(0X6F12, 0X9040);
	write_cmos_sensor_twobyte(0X6F12, 0XA1F8);
	write_cmos_sensor_twobyte(0X6F12, 0X8630);
	write_cmos_sensor_twobyte(0X6F12, 0X03FA);
	write_cmos_sensor_twobyte(0X6F12, 0X02F3);
	write_cmos_sensor_twobyte(0X6F12, 0X414A);
	write_cmos_sensor_twobyte(0X6F12, 0X81B2);
	write_cmos_sensor_twobyte(0X6F12, 0XD181);
	write_cmos_sensor_twobyte(0X6F12, 0X1382);
	write_cmos_sensor_twobyte(0X6F12, 0X03D0);
	write_cmos_sensor_twobyte(0X6F12, 0X5181);
	write_cmos_sensor_twobyte(0X6F12, 0X5B1E);
	write_cmos_sensor_twobyte(0X6F12, 0X9381);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X4FF6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF70);
	write_cmos_sensor_twobyte(0X6F12, 0X5081);
	write_cmos_sensor_twobyte(0X6F12, 0X9081);
	write_cmos_sensor_twobyte(0X6F12, 0X10BD);
	write_cmos_sensor_twobyte(0X6F12, 0X70B5);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7B71);
	write_cmos_sensor_twobyte(0X6F12, 0X3948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X0FF9);
	write_cmos_sensor_twobyte(0X6F12, 0X334C);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5371);
	write_cmos_sensor_twobyte(0X6F12, 0X2060);
	write_cmos_sensor_twobyte(0X6F12, 0X3648);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X07F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XFF61);
	write_cmos_sensor_twobyte(0X6F12, 0X6060);
	write_cmos_sensor_twobyte(0X6F12, 0X3448);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X00F9);
	write_cmos_sensor_twobyte(0X6F12, 0X0122);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9F61);
	write_cmos_sensor_twobyte(0X6F12, 0XA060);
	write_cmos_sensor_twobyte(0X6F12, 0X3148);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF9F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2D61);
	write_cmos_sensor_twobyte(0X6F12, 0XE060);
	write_cmos_sensor_twobyte(0X6F12, 0X2F48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XF2F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0XF551);
	write_cmos_sensor_twobyte(0X6F12, 0X2061);
	write_cmos_sensor_twobyte(0X6F12, 0X2C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XEBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X5341);
	write_cmos_sensor_twobyte(0X6F12, 0X6061);
	write_cmos_sensor_twobyte(0X6F12, 0X2A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE4F8);
	write_cmos_sensor_twobyte(0X6F12, 0XA061);
	write_cmos_sensor_twobyte(0X6F12, 0X2021);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X6000);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XE3F8);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X8006);
	write_cmos_sensor_twobyte(0X6F12, 0X0025);
	write_cmos_sensor_twobyte(0X6F12, 0X04F1);
	write_cmos_sensor_twobyte(0X6F12, 0X3400);
	write_cmos_sensor_twobyte(0X6F12, 0X7580);
	write_cmos_sensor_twobyte(0X6F12, 0X4562);
	write_cmos_sensor_twobyte(0X6F12, 0X8562);
	write_cmos_sensor_twobyte(0X6F12, 0XA0F8);
	write_cmos_sensor_twobyte(0X6F12, 0X4C50);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X5050);
	write_cmos_sensor_twobyte(0X6F12, 0X2A46);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X7D21);
	write_cmos_sensor_twobyte(0X6F12, 0X4565);
	write_cmos_sensor_twobyte(0X6F12, 0X1E48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XCBF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2121);
	write_cmos_sensor_twobyte(0X6F12, 0X6062);
	write_cmos_sensor_twobyte(0X6F12, 0X1C48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XC4F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X4541);
	write_cmos_sensor_twobyte(0X6F12, 0XA062);
	write_cmos_sensor_twobyte(0X6F12, 0X1948);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0XBDF8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F31);
	write_cmos_sensor_twobyte(0X6F12, 0XE061);
	write_cmos_sensor_twobyte(0X6F12, 0X2DE0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0XBBF8);
	write_cmos_sensor_twobyte(0X6F12, 0XABBA);
	write_cmos_sensor_twobyte(0X6F12, 0X0345);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0X1234);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5423);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XCAFE);
	write_cmos_sensor_twobyte(0X6F12, 0XFECA);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X6930);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X7C80);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X86F0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X0AB0);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X5900);
	write_cmos_sensor_twobyte(0X6F12, 0X4000);
	write_cmos_sensor_twobyte(0X6F12, 0XF000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0B27);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0E5D);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X5C51);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X1F65);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X98A9);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X852F);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0XDE2F);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X10B3);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0823);
	write_cmos_sensor_twobyte(0X6F12, 0X0A48);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X87F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X9D11);
	write_cmos_sensor_twobyte(0X6F12, 0X2062);
	write_cmos_sensor_twobyte(0X6F12, 0X0848);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X80F8);
	write_cmos_sensor_twobyte(0X6F12, 0X0022);
	write_cmos_sensor_twobyte(0X6F12, 0XAFF2);
	write_cmos_sensor_twobyte(0X6F12, 0X6F11);
	write_cmos_sensor_twobyte(0X6F12, 0XE062);
	write_cmos_sensor_twobyte(0X6F12, 0X0548);
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);
	write_cmos_sensor_twobyte(0X6F12, 0X79F8);
	write_cmos_sensor_twobyte(0X6F12, 0X2063);
	write_cmos_sensor_twobyte(0X6F12, 0XF562);
	write_cmos_sensor_twobyte(0X6F12, 0X3563);
	write_cmos_sensor_twobyte(0X6F12, 0X7563);
	write_cmos_sensor_twobyte(0X6F12, 0X70BD);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X339B);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X2ABB);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X135F);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0XE91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X273C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X5D6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X033C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X3B2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X45F6);
	write_cmos_sensor_twobyte(0X6F12, 0X514C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X657C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D0C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X471C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X49F6);
	write_cmos_sensor_twobyte(0X6F12, 0XA90C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X48F2);
	write_cmos_sensor_twobyte(0X6F12, 0X2F5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X230C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X43F2);
	write_cmos_sensor_twobyte(0X6F12, 0X9B3C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X46F6);
	write_cmos_sensor_twobyte(0X6F12, 0X3B7C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X4DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X2F6C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB30C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0X871C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F6);
	write_cmos_sensor_twobyte(0X6F12, 0X6F1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0X1B5C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X40F2);
	write_cmos_sensor_twobyte(0X6F12, 0XF91C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X42F6);
	write_cmos_sensor_twobyte(0X6F12, 0XBB2C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X44F2);
	write_cmos_sensor_twobyte(0X6F12, 0XB71C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X000C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);
	write_cmos_sensor_twobyte(0X6F12, 0X41F6);
	write_cmos_sensor_twobyte(0X6F12, 0X8D1C);
	write_cmos_sensor_twobyte(0X6F12, 0XC0F2);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);
	write_cmos_sensor_twobyte(0X6F12, 0X6047);

	write_cmos_sensor_twobyte(0X602A, 0X1150);
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ah 680mV
	write_cmos_sensor_twobyte(0X6F12, 0X0051);	// F49Ch 680mV
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF466);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	// aig_dbr_tune_tg_l 3.05V
	write_cmos_sensor_twobyte(0X6F12, 0X000C);	// aig_dbr_tune_tg_r 3.10V
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X111C);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	// F46Ah 3.7V
	write_cmos_sensor_twobyte(0X602A, 0X1120);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	// F470h -1.1V
	write_cmos_sensor_twobyte(0X602A, 0X113E);
	write_cmos_sensor_twobyte(0X6F12, 0X2101);	// it equalwrite_cmos_sensor_twobyte(0X to aig_left_right_tg_write_cmos_sensor_twobyte(0Xel 06h
	write_cmos_sensor_twobyte(0X602A, 0X1106);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);


	write_cmos_sensor_twobyte(0X602A, 0X3A28);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3A52);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A48);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:8 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A3E);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:4 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A34);
	write_cmos_sensor_twobyte(0X6F12, 0X0066);	// write_cmos_sensor_twobyte(0Xhort Green 1:2 x1
	write_cmos_sensor_twobyte(0X602A, 0X3A84);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// write_cmos_sensor_twobyte(0Xhort Red 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3AB6);
	write_cmos_sensor_twobyte(0X6F12, 0XFF9A);	// write_cmos_sensor_twobyte(0Xhort Blue 1:16 x1
	write_cmos_sensor_twobyte(0X602A, 0X3B44);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3B48);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X3B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);

	// EIT write_cmos_sensor_twobyte, 0X(0Xe);tting
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0200);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X0224);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0224h Addrewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	// 0226h Addrewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X

	// ETC write_cmos_sensor_twobyte, 0X(0Xe);tting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X1124);
	write_cmos_sensor_twobyte(0X6F12, 0X0328);	// ADBUwrite_cmos_sensor_twobyte(0X data order
	write_cmos_sensor_twobyte(0X602A, 0X0ACE);
	write_cmos_sensor_twobyte(0X6F12, 0X0030);

	write_cmos_sensor_twobyte(0X602A, 0X09C0);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X09C2);
	write_cmos_sensor_twobyte(0X6F12, 0X0080);
	write_cmos_sensor_twobyte(0X6F12, 0X008C);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0X0F7F);
	write_cmos_sensor_twobyte(0X6F12, 0XF408);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);
	write_cmos_sensor_twobyte(0X6F12, 0X54A2);
	write_cmos_sensor_twobyte(0X6F12, 0XF418);	// CDwrite_cmos_sensor_twobyte(0X_Option_L
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X6F12, 0X000D);
	write_cmos_sensor_twobyte(0X6F12, 0XF472);	// CLP_Level
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF482);	// COMP_BIAwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6F12, 0X0009);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);
	write_cmos_sensor_twobyte(0X6F12, 0XF484);	// COMP_BIAwrite_cmos_sensor_twobyte(0X

	// ADLC & Dithering write_cmos_s, 0Xenso);r_twobyte(0Xetting
	write_cmos_sensor_twobyte(0X602A, 0X47F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X47FE);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);

	// ATOP write_cmos_sensor_twobyt, 0Xe(0X);etting
	write_cmos_sensor_twobyte(0X602A, 0X1108);
	write_cmos_sensor_twobyte(0X6F12, 0X054C);	// F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt on & LP_VBLK_EN)
	write_cmos_sensor_twobyte(0X6F12, 0X0183);	// F406h Def. 0083h
	write_cmos_sensor_twobyte(0X6F12, 0X07FF);	// F408h Def. 07DFh (Dabx 0ff & Dwrite_cmos_sensor_twobyte(0Xrx en)


	write_cmos_sensor_twobyte(0X602A, 0X1154);
	write_cmos_sensor_twobyte(0X6F12, 0X54C2);	// F418h LDB on x4 write_cmos_sensor_twobyte(0X3
	write_cmos_sensor_twobyte(0X6F12, 0X162F);	// F41Ah // CRwrite_cmos_sensor_twobyte(0Xon
	write_cmos_sensor_twobyte(0X6F12, 0X4328);	// F41Ch
	write_cmos_sensor_twobyte(0X602A, 0X1126);
	write_cmos_sensor_twobyte(0X6F12, 0X00A2);	// F4A6h  IPI write_cmos_sensor_twobyte(0Xignal dip control
	write_cmos_sensor_twobyte(0X602A, 0X114A);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_write_cmos_sensor_twobyte(0XenAnalog_rmp_option_top
	write_cmos_sensor_twobyte(0X602A, 0X114E);
	write_cmos_sensor_twobyte(0X6F12, 0X2608);	// T_write_cmos_sensor_twobyte(0XenAnalog_rmp_option_bot
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF41E);
	write_cmos_sensor_twobyte(0X6F12, 0X2100);	// DBUwrite_cmos_sensor_twobyte(0X option (Hybrid Gain)
	write_cmos_sensor_twobyte(0X602A, 0XF488);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// REF option

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X112A);
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
	write_cmos_sensor_twobyte(0X602A, 0X1128);
	write_cmos_sensor_twobyte(0X6F12, 0X001B);

	// Current write_cmos_sensor_two, 0Xbyte);(0Xetting
	write_cmos_sensor_twobyte(0X602A, 0X10F2);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);	// F476h
	write_cmos_sensor_twobyte(0X602A, 0X10F6);
	write_cmos_sensor_twobyte(0X6F12, 0X0507);	// F478h
	write_cmos_sensor_twobyte(0X602A, 0X10FA);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);	// F47Ah
	write_cmos_sensor_twobyte(0X602A, 0X10FE);
	write_cmos_sensor_twobyte(0X6F12, 0X0A04);	// F47Ch

	// DBUwrite_cmos_sensor_twobyte(, 0X0X L);atch write_cmos_sensor_twobyte(0Xetting
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0XF414);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	// aig_latch_write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X6F12, 0X0004);	// aig_latch_width

	// Clock write_cmos_sensor_twoby, 0Xte(0);Xetting
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X10C6);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);
	write_cmos_sensor_twobyte(0X602A, 0X10B6);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X0B26);
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	// 416d
	write_cmos_sensor_twobyte(0X602A, 0X0B2E);
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X6F12, 0X0420);	// 1056d
	write_cmos_sensor_twobyte(0X602A, 0X0B36);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);


	write_cmos_sensor_twobyte(0X602A, 0X0B4E);
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_write_cmos_sensor_twobyte(0Xl_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_write_cmos_sensor_twobyte(0Xl_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_write_cmos_sensor_twobyte(0Xl_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_write_cmos_sensor_twobyte(0Xl_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//aig_rx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//aig_rx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_rx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_rx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_tx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B7);	//aig_tx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_tx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);	//aig_tx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0105);	//aig_fx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);	//aig_fx_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0140);	//aig_fx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);	//aig_fx_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00EC);	//aig_dwrite_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X00A6);	//aig_dwrite_cmos_sensor_twobyte(0Xrx_reg0

	write_cmos_sensor_twobyte(0X602A, 0X0B76);
	write_cmos_sensor_twobyte(0X6F12, 0X004D);	//aig_write_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X004D);	//aig_write_cmos_sensor_twobyte(0Xrx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B7E);
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dwrite_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X009F);	//aig_dwrite_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B86);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_write_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_write_cmos_sensor_twobyte(0Xtx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B8E);
	write_cmos_sensor_twobyte(0X6F12, 0X0047);	//aig_write_cmos_sensor_twobyte(0Xfx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0048);	//aig_write_cmos_sensor_twobyte(0Xfx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B94);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_write_cmos_sensor_twobyte(0Xfx_reg1
	write_cmos_sensor_twobyte(0X6F12, 0X00E5);	//aig_dabx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X009F);	//aig_dabx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0B9E);
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_abx_reg0
	write_cmos_sensor_twobyte(0X6F12, 0X0046);	//aig_abx_reg0
	write_cmos_sensor_twobyte(0X602A, 0X0BAA);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0101 //aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 00B2 //aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026E //aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 01CD //aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0106 //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 00B7 //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 026C //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 01CB //aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_write_cmos_sensor_twobyte(0X1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_write_cmos_sensor_twobyte(0X1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0156);	//aig_write_cmos_sensor_twobyte(0X1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00FF);	//aig_write_cmos_sensor_twobyte(0X1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0057);	//aig_write_cmos_sensor_twobyte(0X3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0057);	//aig_write_cmos_sensor_twobyte(0X3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X004E);	//aig_write_cmos_sensor_twobyte(0X4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X004E);	//aig_write_cmos_sensor_twobyte(0X4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_write_cmos_sensor_twobyte(0X4dmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_clp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0018);	//aig_clp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_clp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_clp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0016);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0010);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X013E);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00EF);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X014E);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00FA);	//aig_pixbwrite_cmos_sensor_twobyte(0Xt_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006B);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X006B);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000A);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vwrite_cmos_sensor_twobyte(0Xl_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_lp_hblk_cdwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_lp_hblk_cdwrite_cmos_sensor_twobyte(0X_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0C0A);
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vload_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000F);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000F);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0295);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01DA);	//aig_vload_write_cmos_sensor_twobyte(0Xmpdmy_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0008);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_vbgr_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X000B);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_vref_write_cmos_sensor_twobyte(0Xmp_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00D7);	//aig_cnt_en_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0087);	//aig_cnt_en_p0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_cnt_en_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_cnt_en_p0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01D6);	//aig_cnt_en_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0134);	//aig_cnt_en_p1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026A);	//aig_cnt_en_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);	//aig_cnt_en_p1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_enb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_conv_enb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_conv_enb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);	//aig_conv_enb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_conv1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);	//aig_conv1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0111);	//aig_conv2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00C2);	//aig_conv2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0C62);
	write_cmos_sensor_twobyte(0X6F12, 0X010F);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00C0);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv_lwrite_cmos_sensor_twobyte(0Xb_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C72);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_rwrite_cmos_sensor_twobyte(0Xt_div_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//aig_comp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0085);	//aig_comp_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0104);	//aig_comp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00B5);	//aig_comp_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01D4);	//aig_comp_en_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0132);	//aig_comp_en_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_comp_en_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_comp_en_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0C92);
	write_cmos_sensor_twobyte(0X6F12, 0X0002);	//aig_rd_en_10_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0002);	//aig_rd_en_10_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X001C);	//aig_rd_en_10_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0016);	//aig_rd_en_10_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X001E);	//aig_rd_en_9_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0017);	//aig_rd_en_9_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0038);	//aig_rd_en_9_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X002B);	//aig_rd_en_9_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X003A);	//aig_rd_en_8_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X002C);	//aig_rd_en_8_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0054);	//aig_rd_en_8_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0040);	//aig_rd_en_8_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0076);	//aig_rd_en_7_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0041);	//aig_rd_en_7_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0090);	//aig_rd_en_7_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0055);	//aig_rd_en_7_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X010B);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B2);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00C6);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0127);	//aig_rd_en_5_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00C7);	//aig_rd_en_5_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0141);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00DB);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0143);	//aig_rd_en_4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00DC);	//aig_rd_en_4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X015D);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00F0);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X015F);	//aig_rd_en_3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00F1);	//aig_rd_en_3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0179);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0105);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X017B);	//aig_rd_en_2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_rd_en_2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0195);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X011A);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0197);	//aig_rd_en_1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01A0);	//aig_rd_en_1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01B1);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01B4);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01B3);	//aig_rd_en_0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01B5);	//aig_rd_en_0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C9);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr6
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr6
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr7
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr7
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr8
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr8
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr9
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr9
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr10
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr10
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr11
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//aig_rd_en_a_ptr11
	write_cmos_sensor_twobyte(0X602A, 0X0D22);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_conv_en_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X602A, 0X0D32);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00BB);	//aig_lat_lwrite_cmos_sensor_twobyte(0Xb_offwrite_cmos_sensor_twobyte(0Xet_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X006F);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006F);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0270);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CE);	//aig_off_rwrite_cmos_sensor_twobyte(0Xt_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0071);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0070);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0D4E);
	write_cmos_sensor_twobyte(0X6F12, 0X0073);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0072);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0122);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X00D3);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr2
	write_cmos_sensor_twobyte(0X6F12, 0X0125);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr3
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr4
	write_cmos_sensor_twobyte(0X6F12, 0X026E);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X01CD);	//aig_rmp_rwrite_cmos_sensor_twobyte(0Xt_ptr5
	write_cmos_sensor_twobyte(0X6F12, 0X0103);	//aig_rmp_mode_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B4);	//aig_rmp_mode_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X006A);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0069);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0264);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01C4);	//aig_rvwrite_cmos_sensor_twobyte(0X_en_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D74);
	write_cmos_sensor_twobyte(0X6F12, 0X0202);	//aig_tg_hwrite_cmos_sensor_twobyte(0Xync_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X0014);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0014);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X026C);	//aig_optl0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X01CB);	//aig_optl0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0D96);
	write_cmos_sensor_twobyte(0X6F12, 0X0128);	//aig_optl1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00D9);	//aig_optl1_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0132);	//aig_optl1_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00E3);	//aig_optl1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DB6);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//aig_optl2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);	//aig_optl2_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X0119);	//aig_optl2_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CA);	//aig_optl2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0DD6);
	write_cmos_sensor_twobyte(0X6F12, 0X0120);	//aig_optl3_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00D1);	//aig_optl3_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0DF6);
	write_cmos_sensor_twobyte(0X6F12, 0X026B);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X01CA);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0E16);
	write_cmos_sensor_twobyte(0X6F12, 0X010C);	//aig_opth0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X00BD);	//aig_opth0_ptr0
	write_cmos_sensor_twobyte(0X6F12, 0X011E);	//aig_opth0_ptr1
	write_cmos_sensor_twobyte(0X6F12, 0X00CF);	//aig_opth0_ptr1

	write_cmos_sensor_twobyte(0X602A, 0X0FA6);
	write_cmos_sensor_twobyte(0X6F12, 0X0287);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X01CC);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X003C);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X003C);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X0FB2);
	write_cmos_sensor_twobyte(0X6F12, 0X0124);	//VIR_2_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X00D5);	//VIR_2_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X00DF);	//VIR_Mwrite_cmos_sensor_twobyte(0XO_PTR0_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X0099);	//VIR_Mwrite_cmos_sensor_twobyte(0XO_PTR0_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X0296);	//VIR_MIN_ADC_COLwrite_cmos_sensor_twobyte(0X_1_1_1
	write_cmos_sensor_twobyte(0X6F12, 0X01DB);	//VIR_MIN_ADC_COLwrite_cmos_sensor_twobyte(0X_1_1_1

	write_cmos_sensor_twobyte(0X602A, 0X0FD8);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);	//aig_write_cmos_sensor_twobyte(0Xr_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0FDA);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);	//aig_write_cmos_sensor_twobyte(0Xr_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0FDC);
	write_cmos_sensor_twobyte(0X6F12, 0X0103);	//aig_write_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0X_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X0FEA);
	write_cmos_sensor_twobyte(0X6F12, 0X0301);	//aig_ld_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X0FEC);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	//aig_ld_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X101A);
	write_cmos_sensor_twobyte(0X6F12, 0X0003);	//aig_rd_en_6_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X101C);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_6_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X101E);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_5_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1020);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_4_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1022);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_3_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1024);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_2_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1026);
	write_cmos_sensor_twobyte(0X6F12, 0X0303);	//aig_rd_en_1_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X1028);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);	//aig_rd_en_0_ptr1
	write_cmos_sensor_twobyte(0X602A, 0X104C);
	write_cmos_sensor_twobyte(0X6F12, 0X0106);	//aig_optl0_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X106C);
	write_cmos_sensor_twobyte(0X6F12, 0X0607);	//aig_optl4_ptr0
	write_cmos_sensor_twobyte(0X602A, 0X10AE);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//VIR_1_FINE_write_cmos_sensor_twobyte(0XTX_1_1_1
	write_cmos_sensor_twobyte(0X602A, 0X10B0);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);	//VIR_1_FINE_Dwrite_cmos_sensor_twobyte(0XTX_1_1_1

	/////// APwrite_cmos_sensor_twob, 0Xyte();0X PD
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_AigMain1Value 0540	// F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt off & LP_VBLK_EN)
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_cdwrite_cmos_sensor_twobyte(0X_option_m	4338	// F41Ch
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_cdwrite_cmos_sensor_twobyte(0X_option_md	162E	// F41Ah
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__low_gain_val 000C	// F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__high_gain_val 000C	// F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_1__low_gain_val 0080
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_1__high_gain_val 0080


	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0XenAnalog_AigMain1Value 0544   // F404h Def. 050Ch (Pxbwrite_cmos_sensor_twobyte(0Xt off)
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__low_gain_val 000F // F408h
	//WRITE #T_write_cmos_sensor_two, 0Xbyte);(0Xhbn_valwrite_cmos_sensor_twobyte(0X_0__high_gain_val 000F // F408h

	write_cmos_sensor_twobyte(0X602A, 0X0B0A);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0B0C);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X3064);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0B02);
	write_cmos_sensor_twobyte(0X6F12, 0X0018);
	write_cmos_sensor_twobyte(0X602A, 0X0ABC);
	write_cmos_sensor_twobyte(0X6F12, 0X0207);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X070C);
	write_cmos_sensor_twobyte(0X6F12, 0X0020);

	write_cmos_sensor_twobyte(0X602A, 0X0110);
	write_cmos_sensor_twobyte(0X6F12, 0X1002);
	write_cmos_sensor_twobyte(0X602A, 0X0114);
	write_cmos_sensor_twobyte(0X6F12, 0X0300);

	write_cmos_sensor_twobyte(0X602A, 0X0408);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X602A, 0X0344);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	// 0
	write_cmos_sensor_twobyte(0X6F12, 0X1F7F);	// 8063
	write_cmos_sensor_twobyte(0X6F12, 0X0BCF);	// 3023

	write_cmos_sensor_twobyte(0X6F12, 0X1F80);	// 8064
	write_cmos_sensor_twobyte(0X6F12, 0X0BD0);	// 3024

	write_cmos_sensor_twobyte(0X602A, 0X0380);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0900);
	write_cmos_sensor_twobyte(0X6F12, 0X0011);

	write_cmos_sensor_twobyte(0X602A, 0X0400);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X602A, 0X0404);
	write_cmos_sensor_twobyte(0X6F12, 0X0010);

	write_cmos_sensor_twobyte(0X602A, 0X3060);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);

	//PLL write_cmos_sensor_twobyte(, 0X0Xyw);rite_cmos_sensor_twobyte(0X = 800 , write_cmos_sensor_twobyte(0Xec = 798
	write_cmos_sensor_twobyte(0X602A, 0X0304);
	write_cmos_sensor_twobyte(0X6F12, 0X0006);
	write_cmos_sensor_twobyte(0X602A, 0X030C);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0306);
	write_cmos_sensor_twobyte(0X6F12, 0X01F4);	// 500
	write_cmos_sensor_twobyte(0X602A, 0X0302);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0300);
	write_cmos_sensor_twobyte(0X6F12, 0X0005);

	write_cmos_sensor_twobyte(0X602A, 0X0312);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X030E);
	write_cmos_sensor_twobyte(0X6F12, 0X0004);
	write_cmos_sensor_twobyte(0X6F12, 0X010A);	// 266
	write_cmos_sensor_twobyte(0X602A, 0X030A);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);
	write_cmos_sensor_twobyte(0X602A, 0X0308);
	write_cmos_sensor_twobyte(0X6F12, 0X0008);

	write_cmos_sensor_twobyte(0X602A, 0X0342);
	write_cmos_sensor_twobyte(0X6F12, 0X5120);	// 20768
	write_cmos_sensor_twobyte(0X602A, 0X0340);
	write_cmos_sensor_twobyte(0X6F12, 0X0C8A);	// 3210

	write_cmos_sensor_twobyte(0X602A, 0X021E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	// write_cmos_sensor_twobyte(0Xu, 0Xmmat);ion
	write_cmos_sensor_twobyte(0X602A, 0X30A6);
	write_cmos_sensor_twobyte(0X6F12, 0X0400);	//Interleaved LRLR

	write_cmos_sensor_twobyte(0X602A, 0X30A8);
	write_cmos_sensor_twobyte(0X6f12, 0X0002);
	write_cmos_sensor_twobyte(0X602A, 0X30CA);
	write_cmos_sensor_twobyte(0X6F12, 0X0031);

	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A70);
	write_cmos_sensor_twobyte(0X6F12, 0X6E33);

	// Bypawrite_cmos_sensor_twobyte, 0X(0Xw);rite_cmos_sensor_twobyte(0X all chain procewrite_cmos_sensor_twobyte(0Xwrite_cmos_sensor_twobyte(0Xing
	write_cmos_sensor_twobyte(0X602A, 0X4814);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);
	write_cmos_sensor_twobyte(0X602A, 0X4876);
	write_cmos_sensor_twobyte(0X6F12, 0X0107);

	write_cmos_sensor_twobyte(0X602A, 0X2412);
	write_cmos_sensor_twobyte(0X6F12, 0X0102);	//4x4 mwrite_cmos_sensor_twobyte(0Xm
	write_cmos_sensor_twobyte(0X602A, 0X3128);
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//r
	write_cmos_sensor_twobyte(0X6F12, 0X0012);	//Gr
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//R
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//b
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb
	write_cmos_sensor_twobyte(0X6F12, 0X0007);	//B
	write_cmos_sensor_twobyte(0X6F12, 0X0006);	//gb

	// wait to write_cmos_sensor_two, 0Xbyte);(0Xtart Work around - Add delay from H time write_cmos_sensor_twobyte(0Xtart.
	write_cmos_sensor_twobyte(0X602A, 0X8724);
	write_cmos_sensor_twobyte(0X6F12, 0X0220);	// ex. add 32 TG CLC'write_cmos_sensor_twobyte(0X Delay (Delay in TG CLK'write_cmos_sensor_twobyte(0X)

	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X6214);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY
	write_cmos_sensor_twobyte(0X602A, 0X6218);
	write_cmos_sensor_twobyte(0X6F12, 0X79F0);	// Modify CLK to allow HW writewrite_cmos_sensor_twobyte(0X on write_cmos_sensor_twobyte(0XTBY

	//Modify on-Line write_cmos_sens, 0Xor_t);wobyte(0Xettingwrite_cmos_sensor_twobyte(0X
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X0A30);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X2000);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X1480);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);
	write_cmos_sensor_twobyte(0X6F12, 0X1950);

	//emb linewrite_cmos_sensor_twob, 0Xyte();
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X30CE);
	write_cmos_sensor_twobyte(0X6F12, 0X0001);

	//TnP fix: line write_cmos_senso, 0Xr_tw);obyte(0Xwap write_cmos_sensor_twobyte(0Xupport for WDR Off
	write_cmos_sensor_twobyte(0X602A, 0X0702);
	write_cmos_sensor_twobyte(0X6F12, 0X01	);//01 Enabled ;00 Diwrite_cmos_sensor_twobyte(0Xabled

	//TnP fix: iir filter rewrite_cm, 0Xos_s);ensor_twobyte(0Xet uwrite_cmos_sensor_twobyte(0Xing FOB write_cmos_sensor_twobyte(0Xtatiwrite_cmos_sensor_twobyte(0Xticwrite_cmos_sensor_twobyte(0X diff
	write_cmos_sensor_twobyte(0X6028, 0X2000);
	write_cmos_sensor_twobyte(0X602A, 0X228E);
	write_cmos_sensor_twobyte(0X6F12, 0X0245);
	write_cmos_sensor_twobyte(0X6F12, 0X0105);
	write_cmos_sensor_twobyte(0X602A, 0X8726);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0180);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X872C);
	write_cmos_sensor_twobyte(0X6F12, 0X00B8);

	//write_cmos_sensor_twobyte(0Xub, 0Xtrac);t on write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X602A, 0X09B8);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);	//enable write_cmos_sensor_twobyte(0Xubtract on write_cmos_sensor_twobyte(0Xtart
	write_cmos_sensor_twobyte(0X602A, 0X515E);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);	//Diwrite_cmos_sensor_twobyte(0Xable write_cmos_sensor_twobyte(0Xubtract of Y Data
	write_cmos_sensor_twobyte(0X602A, 0X5160);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	//TnP:	HW regiwrite_cmos_sensor, 0X_two);byte(0Xter update on V-Blank	(modify different HW write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X on V-Blank)
	write_cmos_sensor_twobyte(0X602A, 0X872E);
	write_cmos_sensor_twobyte(0X6F12, 0XF4E0);	//aig_dummy_addr_dr_rb_e
	write_cmos_sensor_twobyte(0X6F12, 0XF4E2);	//aig_dummy_addr_dr_rb_o
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X for tewrite_cmos_sensor_twobyte(0Xt
	write_cmos_sensor_twobyte(0X6F12, 0XFFFF);	//write_cmos_sensor_twobyte(0Xettingwrite_cmos_sensor_twobyte(0X for tewrite_cmos_sensor_twobyte(0Xt
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);
	write_cmos_sensor_twobyte(0X6F12, 0X0000);

	write_cmos_sensor_twobyte(0X602A, 0X4D70);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D80);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X602A, 0X4D90);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);
	write_cmos_sensor_twobyte(0X6028, 0X4000);
	write_cmos_sensor_twobyte(0X602A, 0X0B04);
	write_cmos_sensor_twobyte(0X6F12, 0X0101);
	write_cmos_sensor_twobyte(0X602A, 0X30A2);
	write_cmos_sensor_twobyte(0X6F12, 0X2800);
	write_cmos_sensor_twobyte(0X6F12, 0X5400);

	write_cmos_sensor_twobyte(0X602A, 0X0100);
	write_cmos_sensor_twobyte(0X6F12, 0X0100);

		/*Log exposure ratio*/
	write_cmos_sensor_twobyte(0x021e, 0X0000);

		// Stream On
	write_cmos_sensor_twobyte(0x0100,0x0100);

	mDELAY(10);

}
#endif

static void hs_video_setting_11(void)
{
//$MIPI[Width:2816,Height:1584,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:2816,Height:1584,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]

// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);

////1. ADLC setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x168C);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning



//2. Clock setting, related with ATOP remove
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E24);
write_cmos_sensor(0x6F12,0x01);	// For 600MHz CCLK

//3. ATOP Setting (Option)
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E9C);
write_cmos_sensor_twobyte(0x6F12,0x0081);	// RDV option // binning

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E47);
write_cmos_sensor(0x6F12,0x01);	// CDS current // EVT1.1 0429 lkh
write_cmos_sensor_twobyte(0x602A,0x0E48);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E49);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E4A);
write_cmos_sensor(0x6F12,0x03);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x0E4B);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4C);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4D);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4E);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x0E4F);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E50);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E51);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E52);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current


// EVT1.1 TnP
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0xAB00);
write_cmos_sensor(0x6F12,0x00);

//Af correction for 4x4 binning
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x14C9);
write_cmos_sensor(0x6F12,0x10);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CA);
write_cmos_sensor(0x6F12,0x10);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CD);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CE);
write_cmos_sensor(0x6F12,0x05);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CF);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D0);
write_cmos_sensor(0x6F12,0x09);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D1);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D2);
write_cmos_sensor(0x6F12,0x09);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D3);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D4);
write_cmos_sensor(0x6F12,0x0D);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14DE);
write_cmos_sensor_twobyte(0x6F12,0x1BE4); // EVT1.1
write_cmos_sensor_twobyte(0x6F12,0xB14E);	// EVT1.1


//MSM gain for 4x4 binning
write_cmos_sensor_twobyte(0x602A,0x427A);
write_cmos_sensor(0x6F12,0x04);
write_cmos_sensor_twobyte(0x602A,0x430D);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x430E);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x430F);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4310);
write_cmos_sensor(0x6F12,0x29);
write_cmos_sensor_twobyte(0x602A,0x4315);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x4316);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4317);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4318);
write_cmos_sensor(0x6F12,0x00);



// AF
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0B0E);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x3069);
write_cmos_sensor(0x6F12,0x01);

// BPC
write_cmos_sensor_twobyte(0x602A,0x0B08);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x0B05);
write_cmos_sensor(0x6F12,0x01);


// WDR
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0216);
write_cmos_sensor(0x6F12,0x00); //1	// smiaRegs_rw_wdr_multiple_exp_mode, EVT1.1
write_cmos_sensor_twobyte(0x602A,0x021B);
write_cmos_sensor(0x6F12,0x00);	// smiaRegs_rw_wdr_exposure_order, EVT1.1


write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0202);
write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x602A,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// Analog Gain
write_cmos_sensor_twobyte(0x602A,0x0204);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);


//4. CASE : FHD 1920x1080(2816x1584)
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0344);
write_cmos_sensor_twobyte(0x6F12,0x0012);
write_cmos_sensor_twobyte(0x602A,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x1621);
write_cmos_sensor_twobyte(0x602A,0x0346);
write_cmos_sensor_twobyte(0x6F12,0x021C);
write_cmos_sensor_twobyte(0x602A,0x034A);
write_cmos_sensor_twobyte(0x6F12,0x0E8B);
write_cmos_sensor_twobyte(0x6F12,0x0B00);
write_cmos_sensor_twobyte(0x6F12,0x0630);
write_cmos_sensor_twobyte(0x602A,0x0342);
write_cmos_sensor_twobyte(0x6F12,0x1AD0);
write_cmos_sensor_twobyte(0x602A,0x0340);
write_cmos_sensor_twobyte(0x6F12,0x06B4);	// 06D6

write_cmos_sensor_twobyte(0x602A,0x0900);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0901);
write_cmos_sensor(0x6F12,0x22);
write_cmos_sensor_twobyte(0x602A,0x0380);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x06E0);
write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x602A,0x06E4);
write_cmos_sensor_twobyte(0x6F12,0x1000);

// PSP BDS/HVbin
write_cmos_sensor_twobyte(0x602A,0x0EFA);
write_cmos_sensor(0x6F12,0x01);	// BDS
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0404);
write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

// CropAndPad
write_cmos_sensor_twobyte(0x602A,0x0408);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

///////////////////////////////////////////////////////////
//PLL Sys = 560 , Sec = 1392
write_cmos_sensor_twobyte(0x602A,0x0136);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x602A,0x0304);
write_cmos_sensor_twobyte(0x6F12,0x0005);	// 3->5 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0173);	// 225->371 EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0x030C);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0302);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0300);
write_cmos_sensor_twobyte(0x6F12,0x0005);
write_cmos_sensor_twobyte(0x602A,0x030A);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0308);
write_cmos_sensor_twobyte(0x6F12,0x000A);

write_cmos_sensor_twobyte(0x602A,0x0318);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x00A4);	// A5 -> A4 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0316);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0314);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x030E);
write_cmos_sensor_twobyte(0x6F12,0x0004);
write_cmos_sensor_twobyte(0x6F12,0x00F2);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// OIF Setting
write_cmos_sensor_twobyte(0x602A,0x0111);
write_cmos_sensor(0x6F12,0x02);	// PVI, 2: MIPI
write_cmos_sensor_twobyte(0x602A,0x0114);
write_cmos_sensor(0x6F12,0x03);
write_cmos_sensor_twobyte(0x602A,0x0112);
write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

write_cmos_sensor_twobyte(0x602A,0xB0CA);
write_cmos_sensor_twobyte(0x6F12,0x7E00);	// M_DPHYCTL[30:25] = 6'11_1111 // EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0xB136);
write_cmos_sensor_twobyte(0x6F12,0x2000);	// B_DPHYCTL[62:60] = 3'b010 // EVT1.1 0429





// Stream On
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x01);


mDELAY(10);

}


static void slim_video_setting(void)
{
//$MIPI[Width:1408,Height:792,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:1408,Height:792,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]


// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);

////1. ADLC setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x168C);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning



//2. Clock setting, related with ATOP remove
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E24);
write_cmos_sensor(0x6F12,0x01);	// For 600MHz CCLK

//3. ATOP Setting (Option)
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E9C);
write_cmos_sensor_twobyte(0x6F12,0x0080);	// RDV option // binning

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E47);
write_cmos_sensor(0x6F12,0x05);	// CDS current // EVT1.1 0429 lkh
write_cmos_sensor_twobyte(0x602A,0x0E48);
write_cmos_sensor(0x6F12,0x05);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E49);
write_cmos_sensor(0x6F12,0x05);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E4A);
write_cmos_sensor(0x6F12,0x05);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x0E4B);
write_cmos_sensor(0x6F12,0x0F);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4C);
write_cmos_sensor(0x6F12,0x0F);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4D);
write_cmos_sensor(0x6F12,0x0F);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4E);
write_cmos_sensor(0x6F12,0x0F);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x0E4F);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E50);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E51);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E52);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Boost current


// EVT1.1 TnP
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0xAB00);
write_cmos_sensor(0x6F12,0x01);

//Af correction for 4x4 binning
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x14C9);
write_cmos_sensor(0x6F12,0x04);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CA);
write_cmos_sensor(0x6F12,0x04);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CD);
write_cmos_sensor(0x6F12,0x02);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CE);
write_cmos_sensor(0x6F12,0x01);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CF);
write_cmos_sensor(0x6F12,0x02);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D0);
write_cmos_sensor(0x6F12,0x03);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D1);
write_cmos_sensor(0x6F12,0x02);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D2);
write_cmos_sensor(0x6F12,0x03);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D3);
write_cmos_sensor(0x6F12,0x02);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D4);
write_cmos_sensor(0x6F12,0x03);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14DE);
write_cmos_sensor_twobyte(0x6F12,0x1414); // EVT1.1
write_cmos_sensor_twobyte(0x6F12,0x4141);	// EVT1.1


//MSM gain for 4x4 binning
write_cmos_sensor_twobyte(0x602A,0x427A);
write_cmos_sensor(0x6F12,0x10);
write_cmos_sensor_twobyte(0x602A,0x430D);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x430E);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x430F);
write_cmos_sensor(0x6F12,0x08);
write_cmos_sensor_twobyte(0x602A,0x4310);
write_cmos_sensor(0x6F12,0x09);
write_cmos_sensor_twobyte(0x602A,0x4315);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x4316);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x4317);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4318);
write_cmos_sensor(0x6F12,0x29);



// AF
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0B0E);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x3069);
write_cmos_sensor(0x6F12,0x00);

// BPC
write_cmos_sensor_twobyte(0x602A,0x0B08);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x0B05);
write_cmos_sensor(0x6F12,0x01);


// WDR
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0216);
write_cmos_sensor(0x6F12,0x00); //1	// smiaRegs_rw_wdr_multiple_exp_mode, EVT1.1
write_cmos_sensor_twobyte(0x602A,0x021B);
write_cmos_sensor(0x6F12,0x00);	// smiaRegs_rw_wdr_exposure_order, EVT1.1


write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0202);
write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x602A,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// Analog Gain
write_cmos_sensor_twobyte(0x602A,0x0204);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);


//4. CASE : HD 1280x720(1408x792)
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0344);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x161F);
write_cmos_sensor_twobyte(0x602A,0x0346);
write_cmos_sensor_twobyte(0x6F12,0x0214);
write_cmos_sensor_twobyte(0x602A,0x034A);
write_cmos_sensor_twobyte(0x6F12,0x0E93);
write_cmos_sensor_twobyte(0x6F12,0x0580);
write_cmos_sensor_twobyte(0x6F12,0x0318);
write_cmos_sensor_twobyte(0x602A,0x0342);
write_cmos_sensor_twobyte(0x6F12,0x23F0);
write_cmos_sensor_twobyte(0x602A,0x0340);
write_cmos_sensor_twobyte(0x6F12,0x0368);	// 06D6

write_cmos_sensor_twobyte(0x602A,0x0900);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0901);
write_cmos_sensor(0x6F12,0x44);
write_cmos_sensor_twobyte(0x602A,0x0380);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0007);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0007);
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x06E0);
write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x602A,0x06E4);
write_cmos_sensor_twobyte(0x6F12,0x1000);

// PSP BDS/HVbin
write_cmos_sensor_twobyte(0x602A,0x0EFA);
write_cmos_sensor(0x6F12,0x01);	// BDS
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0404);
write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

// CropAndPad
write_cmos_sensor_twobyte(0x602A,0x0408);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

///////////////////////////////////////////////////////////
//PLL Sys = 560 , Sec = 1392
write_cmos_sensor_twobyte(0x602A,0x0136);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x602A,0x0304);
write_cmos_sensor_twobyte(0x6F12,0x0005);	// 3->5 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0173);	// 225->371 EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0x030C);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0302);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0300);
write_cmos_sensor_twobyte(0x6F12,0x0005);
write_cmos_sensor_twobyte(0x602A,0x030A);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0308);
write_cmos_sensor_twobyte(0x6F12,0x000A);

write_cmos_sensor_twobyte(0x602A,0x0318);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x00A4);	// A5 -> A4 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0316);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0314);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x030E);
write_cmos_sensor_twobyte(0x6F12,0x0004);
write_cmos_sensor_twobyte(0x6F12,0x00F2);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// OIF Setting
write_cmos_sensor_twobyte(0x602A,0x0111);
write_cmos_sensor(0x6F12,0x02);	// PVI, 2: MIPI
write_cmos_sensor_twobyte(0x602A,0x0114);
write_cmos_sensor(0x6F12,0x03);
write_cmos_sensor_twobyte(0x602A,0x0112);
write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

write_cmos_sensor_twobyte(0x602A,0xB0CA);
write_cmos_sensor_twobyte(0x6F12,0x7E00);	// M_DPHYCTL[30:25] = 6'11_1111 // EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0xB136);
write_cmos_sensor_twobyte(0x6F12,0x2000);	// B_DPHYCTL[62:60] = 3'b010 // EVT1.1 0429





// Stream On
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x01);


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
			   if (*sensor_id == imgsensor_info.sensor_id || *sensor_id == 0x20C7) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);

				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id && *sensor_id != 0x20C7) {
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

			if (sensor_id == imgsensor_info.sensor_id  || sensor_id == 0x20C7) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x, chip_id (0x%x)\n", imgsensor.i2c_write_id,sensor_id, chip_id);
				break;
			}
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id || sensor_id == 0x20C7)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id && 0x20C7 != sensor_id)
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
			//add for s5k2l7 pdaf
			LOG_INF("s5k2l7_read_otp_pdaf_data \n");
			s5k2l7_read_otp_pdaf_data((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
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
