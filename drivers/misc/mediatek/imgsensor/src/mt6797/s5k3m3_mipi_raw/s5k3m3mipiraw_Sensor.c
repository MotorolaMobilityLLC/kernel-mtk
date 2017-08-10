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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 S5K3M3mipiraw_sensor.c
 *
 * Project:
 * --------
 *	 ALPS MT6735
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *	PengtaoFan
 *  1511041520 modify code style from 3m3@mt6755 and move to mt6797
 *  1512291720 update setting from samsung
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

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k3m3mipiraw_Sensor.h"

/*===FEATURE SWITH===*/
 // #define FPTPDAFSUPPORT   //for pdaf switch
 // #define FANPENGTAO   //for debug log

 //#define NONCONTINUEMODE
/*===FEATURE SWITH===*/

/****************************Modify Following Strings for Debug****************************/
#define PFX "S5K3M3"
#define LOG_INF_NEW(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_INF LOG_INF_NEW
#define LOG_1 LOG_INF("S5K3M3,MIPI 4LANE\n")
#define SENSORDB LOG_INF
/****************************   Modify end    *******************************************/

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K3M3_SENSOR_ID,		//Sensor ID Value: 0x30C8//record sensor id defined in Kd_imgsensor.h

	.checksum_value = 0x48b58a2b, //0x49c09f86,		//checksum value for Camera Auto Test

	.pre = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 9408,				//record different mode's linelength
		.framelength = 1689,			//record different mode's framelength
		.startx= 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2104,		//record different mode's width of grabwindow
		.grabwindow_height = 1560,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
#ifdef NONCONTINUEMODE
	.cap = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4704,//5808,				//record different mode's linelength
		.framelength = 3375,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
#else //CONTINUEMODE
	.cap = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4704,				//record different mode's linelength
		.framelength = 3375,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
#endif
#if 1 //fps =24
	.cap1 = {							//capture for PIP 24fps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 5880,				//record different mode's linelength
		.framelength = 3375,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 240,
	},
#endif
#if 0 //fps 15
	.cap1 = {							//capture for PIP 15ps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 400000000,				//record different mode's pclk
		.linelength  = 5808,				//record different mode's linelength
		.framelength = 4589,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 150,
	},
#endif
	.normal_video = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4704,				//record different mode's linelength
		.framelength = 3375,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4456,				//record different mode's linelength
		.framelength = 896,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1392,		//record different mode's width of grabwindow
		.grabwindow_height = 788,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4456,				//record different mode's linelength
		.framelength = 3584,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1392,		//record different mode's width of grabwindow
		.grabwindow_height = 788,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
  .custom1 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
  .custom2 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
  .custom3 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
  .custom4 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
  .custom5 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},

	.margin = 5,			//sensor framelength & shutter margin
	.min_shutter = 4,		//min shutter
	.max_frame_length = 0xFFFF,//REG0x0202 <=REG0x0340-5//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,	//shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num ,don't support Slow motion

	.cap_delay_frame = 3,		//enter capture delay frame num
	.pre_delay_frame = 3, 		//enter preview delay frame num
	.video_delay_frame = 3,		//enter video delay frame num
	.hs_video_delay_frame = 3,	//enter high speed video  delay frame num
	.slim_video_delay_frame = 3,//enter slim video delay frame num
    .custom1_delay_frame = 2,
    .custom2_delay_frame = 2,
    .custom3_delay_frame = 2,
    .custom4_delay_frame = 2,
    .custom5_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_8MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = 1,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
	.i2c_addr_table = {0x5a, 0x20,0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
    .i2c_speed = 300, // i2c read/write speed
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x200,					//current shutter
	.gain = 0x200,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0,//record current sensor's i2c write id
};


/* Sensor output window information*/
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] =
{
 { 4208, 3120,	  0,  	0, 4208, 3120, 2104, 1560,   0,	0, 2104, 1560, 	 0, 0, 2104, 1560}, // Preview
 { 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // capture
 { 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // video
 { 4208, 3120,	16,  378, 4176, 2364, 1392,  788,   0,	0, 1392,  788, 	 0, 0, 1392,  788}, //hight speed video
 { 4208, 3120,	16,  378, 4176, 2364, 1392,  788,   0,	0, 1392,  788, 	 0, 0, 1392,  788},// slim video
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552},// Custom1 (defaultuse preview)
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552},// Custom2
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552},// Custom3
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552},// Custom4
 { 4192, 3104,	  0,  0, 4192, 3104, 2096,  1552, 0000, 0000, 2096, 1552, 0,	0, 2096,  1552},// Custom5
};

 static SET_PD_BLOCK_INFO_T imgsensor_pd_info =
 //for 3m3
{

    .i4OffsetX = 28,

    .i4OffsetY = 31,

    .i4PitchX = 64,

    .i4PitchY = 64,

    .i4PairNum =16,

    .i4SubBlkW =16,

    .i4SubBlkH =16,

    .i4PosL = {{28,31},{80,31},{44,35},{64,35},{32,51},{76,51},{48,55},{60,55},{48,63},{60,63},{32,67},{76,67},{44,83},{64,83},{28,87},{80,87}},

    .i4PosR = {{28,35},{80,35},{44,39},{64,39},{32,47},{76,47},{48,51},{60,51},{48,67},{60,67},{32,71},{76,71},{44,79},{64,79},{28,83},{80,83}},

};

static kal_uint16 read_cmos_sensor_byte(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };


    iReadRegI2C(pu_send_cmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
    return get_byte;
}

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };


    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
    return get_byte;
}

static void write_cmos_sensor_byte(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};


    iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};


    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	write_cmos_sensor(0x0342, imgsensor.line_length & 0xFFFF);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	LOG_INF("framerate = %d, min framelength should enable(%d) \n", framerate,min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	//dummy_line = frame_length - imgsensor.min_frame_length;
	//if (dummy_line < 0)
		//imgsensor.dummy_line = 0;
	//else
		//imgsensor.dummy_line = dummy_line;
	//imgsensor.frame_length = frame_length + imgsensor.dummy_line;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	{
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */

/*

static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;



	// if shutter bigger than frame_length, should extend frame length first
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

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
	} else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0x0202, (shutter) & 0xFFFF);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

	//LOG_INF("frame_length = %d ", frame_length);

}
*/

/*	write_shutter  */



/*************************************************************************
* FUNCTION
*	set_shutter
*
* DESCRIPTION
*	This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*	iShutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	//write_shutter(shutter);
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
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

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
		else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0X0202, shutter & 0xFFFF);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;
	//gain = 64 = 1x real gain.
    reg_gain = gain/2;
	//reg_gain = reg_gain & 0xFFFF;
	return (kal_uint16)reg_gain;
}

/*************************************************************************
* FUNCTION
*	set_gain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*	iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	LOG_INF("set_gain %d \n", gain);
  //gain = 64 = 1x real gain.

	if (gain < BASEGAIN || gain > 16 * BASEGAIN) {
		LOG_INF("Error gain setting");
		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 16 * BASEGAIN)
			gain = 16 * BASEGAIN;
	}

    reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x0204, (reg_gain&0xFFFF));
	return gain;
}	/*	set_gain  */

//ihdr_write_shutter_gain not support for s5k3m3
static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
	if (imgsensor.ihdr_en) {

		spin_lock(&imgsensor_drv_lock);
			if (le > imgsensor.min_frame_length - imgsensor_info.margin)
				imgsensor.frame_length = le + imgsensor_info.margin;
			else
				imgsensor.frame_length = imgsensor.min_frame_length;
			if (imgsensor.frame_length > imgsensor_info.max_frame_length)
				imgsensor.frame_length = imgsensor_info.max_frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (le < imgsensor_info.min_shutter) le = imgsensor_info.min_shutter;
			if (se < imgsensor_info.min_shutter) se = imgsensor_info.min_shutter;


		// Extend frame length first
		write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);

		write_cmos_sensor(0x3502, (le << 4) & 0xFF);
		write_cmos_sensor(0x3501, (le >> 4) & 0xFF);
		write_cmos_sensor(0x3500, (le >> 12) & 0x0F);

		write_cmos_sensor(0x3512, (se << 4) & 0xFF);
		write_cmos_sensor(0x3511, (se >> 4) & 0xFF);
		write_cmos_sensor(0x3510, (se >> 12) & 0x0F);

		set_gain(gain);
	}

}



static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

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
			write_cmos_sensor(0x0101,0X00); //GR
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0101,0X01); //R
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0101,0X02); //B
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x0101,0X03); //GB
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
	}

}

/*************************************************************************
* FUNCTION
*	night_mode
*
* DESCRIPTION
*	This function night mode of sensor.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}	/*	night_mode	*/
static void sensor_init(void)
{
  LOG_INF("E\n");
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6010, 0X0000);
mdelay(3);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X303C);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0449);
write_cmos_sensor(0X6F12, 0X0348);
write_cmos_sensor(0X6F12, 0X0860);
write_cmos_sensor(0X6F12, 0XCA8D);
write_cmos_sensor(0X6F12, 0X101A);
write_cmos_sensor(0X6F12, 0X8880);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XA6BE);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X401C);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1E80);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X70B5);
write_cmos_sensor(0X6F12, 0XFF4C);
write_cmos_sensor(0X6F12, 0X0E46);
write_cmos_sensor(0X6F12, 0XA4F8);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6F12, 0XFE4D);
write_cmos_sensor(0X6F12, 0X15F8);
write_cmos_sensor(0X6F12, 0X940F);
write_cmos_sensor(0X6F12, 0XA4F8);
write_cmos_sensor(0X6F12, 0X0601);
write_cmos_sensor(0X6F12, 0X6878);
write_cmos_sensor(0X6F12, 0XA4F8);
write_cmos_sensor(0X6F12, 0X0801);
write_cmos_sensor(0X6F12, 0X1046);
write_cmos_sensor(0X6F12, 0X04F5);
write_cmos_sensor(0X6F12, 0XE874);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X46FF);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X48FF);
write_cmos_sensor(0X6F12, 0XB5F8);
write_cmos_sensor(0X6F12, 0X9A01);
write_cmos_sensor(0X6F12, 0X6086);
write_cmos_sensor(0X6F12, 0XB5F8);
write_cmos_sensor(0X6F12, 0X9E01);
write_cmos_sensor(0X6F12, 0XE086);
write_cmos_sensor(0X6F12, 0XB5F8);
write_cmos_sensor(0X6F12, 0XA001);
write_cmos_sensor(0X6F12, 0X2087);
write_cmos_sensor(0X6F12, 0XB5F8);
write_cmos_sensor(0X6F12, 0XA601);
write_cmos_sensor(0X6F12, 0XE087);
write_cmos_sensor(0X6F12, 0X70BD);
write_cmos_sensor(0X6F12, 0X2DE9);
write_cmos_sensor(0X6F12, 0XF05F);
write_cmos_sensor(0X6F12, 0XEF4A);
write_cmos_sensor(0X6F12, 0XF049);
write_cmos_sensor(0X6F12, 0XED4C);
write_cmos_sensor(0X6F12, 0X0646);
write_cmos_sensor(0X6F12, 0X92F8);
write_cmos_sensor(0X6F12, 0XEC30);
write_cmos_sensor(0X6F12, 0X0F89);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0XBE00);
write_cmos_sensor(0X6F12, 0X4FF4);
write_cmos_sensor(0X6F12, 0X8079);
write_cmos_sensor(0X6F12, 0X13B1);
write_cmos_sensor(0X6F12, 0XA8B9);
write_cmos_sensor(0X6F12, 0X4F46);
write_cmos_sensor(0X6F12, 0X13E0);
write_cmos_sensor(0X6F12, 0XEA4B);
write_cmos_sensor(0X6F12, 0X0968);
write_cmos_sensor(0X6F12, 0X92F8);
write_cmos_sensor(0X6F12, 0XED20);
write_cmos_sensor(0X6F12, 0X1B6D);
write_cmos_sensor(0X6F12, 0XB1FB);
write_cmos_sensor(0X6F12, 0XF3F1);
write_cmos_sensor(0X6F12, 0X7943);
write_cmos_sensor(0X6F12, 0XD140);
write_cmos_sensor(0X6F12, 0X8BB2);
write_cmos_sensor(0X6F12, 0X30B1);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X4FF4);
write_cmos_sensor(0X6F12, 0X8051);
write_cmos_sensor(0X6F12, 0X1846);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X1EFF);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X4846);
write_cmos_sensor(0X6F12, 0X87B2);
write_cmos_sensor(0X6F12, 0X3088);
write_cmos_sensor(0X6F12, 0X18B1);
write_cmos_sensor(0X6F12, 0X0121);
write_cmos_sensor(0X6F12, 0XDF48);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X1AFF);
write_cmos_sensor(0X6F12, 0X0021);
write_cmos_sensor(0X6F12, 0XDE48);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X16FF);
write_cmos_sensor(0X6F12, 0XDFF8);
write_cmos_sensor(0X6F12, 0X78A3);
write_cmos_sensor(0X6F12, 0X0025);
write_cmos_sensor(0X6F12, 0XA046);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X010B);
write_cmos_sensor(0X6F12, 0X3088);
write_cmos_sensor(0X6F12, 0X70B1);
write_cmos_sensor(0X6F12, 0X7088);
write_cmos_sensor(0X6F12, 0X38B1);
write_cmos_sensor(0X6F12, 0XF089);
write_cmos_sensor(0X6F12, 0X06E0);
write_cmos_sensor(0X6F12, 0X2A46);
write_cmos_sensor(0X6F12, 0XD549);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X0BFF);
write_cmos_sensor(0X6F12, 0X0446);
write_cmos_sensor(0X6F12, 0X05E0);
write_cmos_sensor(0X6F12, 0XB089);
write_cmos_sensor(0X6F12, 0X0028);
write_cmos_sensor(0X6F12, 0XF6D1);
write_cmos_sensor(0X6F12, 0X3846);
write_cmos_sensor(0X6F12, 0XF4E7);
write_cmos_sensor(0X6F12, 0X4C46);
write_cmos_sensor(0X6F12, 0X98F8);
write_cmos_sensor(0X6F12, 0XBE00);
write_cmos_sensor(0X6F12, 0X28B1);
write_cmos_sensor(0X6F12, 0X2A46);
write_cmos_sensor(0X6F12, 0XCF49);
write_cmos_sensor(0X6F12, 0X3846);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XFBFE);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X4846);
write_cmos_sensor(0X6F12, 0XAAF8);
write_cmos_sensor(0X6F12, 0X5802);
write_cmos_sensor(0X6F12, 0X4443);
write_cmos_sensor(0X6F12, 0X98F8);
write_cmos_sensor(0X6F12, 0X2B02);
write_cmos_sensor(0X6F12, 0X98F8);
write_cmos_sensor(0X6F12, 0X2922);
write_cmos_sensor(0X6F12, 0XC440);
write_cmos_sensor(0X6F12, 0X0244);
write_cmos_sensor(0X6F12, 0X0BFA);
write_cmos_sensor(0X6F12, 0X02F0);
write_cmos_sensor(0X6F12, 0X421E);
write_cmos_sensor(0X6F12, 0XA242);
write_cmos_sensor(0X6F12, 0X01DA);
write_cmos_sensor(0X6F12, 0X1346);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X2346);
write_cmos_sensor(0X6F12, 0X002B);
write_cmos_sensor(0X6F12, 0X01DA);
write_cmos_sensor(0X6F12, 0X0024);
write_cmos_sensor(0X6F12, 0X02E0);
write_cmos_sensor(0X6F12, 0XA242);
write_cmos_sensor(0X6F12, 0X00DA);
write_cmos_sensor(0X6F12, 0X1446);
write_cmos_sensor(0X6F12, 0X0AEB);
write_cmos_sensor(0X6F12, 0X4500);
write_cmos_sensor(0X6F12, 0X6D1C);
write_cmos_sensor(0X6F12, 0XA0F8);
write_cmos_sensor(0X6F12, 0X5A42);
write_cmos_sensor(0X6F12, 0X042D);
write_cmos_sensor(0X6F12, 0XC4DB);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XF09F);
write_cmos_sensor(0X6F12, 0XB94A);
write_cmos_sensor(0X6F12, 0X92F8);
write_cmos_sensor(0X6F12, 0XB330);
write_cmos_sensor(0X6F12, 0X53B1);
write_cmos_sensor(0X6F12, 0X92F8);
write_cmos_sensor(0X6F12, 0XFD20);
write_cmos_sensor(0X6F12, 0X0420);
write_cmos_sensor(0X6F12, 0X52B1);
write_cmos_sensor(0X6F12, 0XB94A);
write_cmos_sensor(0X6F12, 0X92F8);
write_cmos_sensor(0X6F12, 0X3221);
write_cmos_sensor(0X6F12, 0X012A);
write_cmos_sensor(0X6F12, 0X05D1);
write_cmos_sensor(0X6F12, 0X0220);
write_cmos_sensor(0X6F12, 0X03E0);
write_cmos_sensor(0X6F12, 0X5379);
write_cmos_sensor(0X6F12, 0X2BB9);
write_cmos_sensor(0X6F12, 0X20B1);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X8842);
write_cmos_sensor(0X6F12, 0X00D8);
write_cmos_sensor(0X6F12, 0X0846);
write_cmos_sensor(0X6F12, 0X7047);
write_cmos_sensor(0X6F12, 0X1078);
write_cmos_sensor(0X6F12, 0XF9E7);
write_cmos_sensor(0X6F12, 0XFEB5);
write_cmos_sensor(0X6F12, 0X0646);
write_cmos_sensor(0X6F12, 0XB148);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XC068);
write_cmos_sensor(0X6F12, 0X84B2);
write_cmos_sensor(0X6F12, 0X050C);
write_cmos_sensor(0X6F12, 0X2146);
write_cmos_sensor(0X6F12, 0X2846);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XB9FE);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XBBFE);
write_cmos_sensor(0X6F12, 0XA64E);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X8DF8);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0XFD00);
write_cmos_sensor(0X6F12, 0X8DF8);
write_cmos_sensor(0X6F12, 0X0100);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0XB310);
write_cmos_sensor(0X6F12, 0XA54F);
write_cmos_sensor(0X6F12, 0X19B1);
write_cmos_sensor(0X6F12, 0X97F8);
write_cmos_sensor(0X6F12, 0X3221);
write_cmos_sensor(0X6F12, 0X012A);
write_cmos_sensor(0X6F12, 0X0FD0);
write_cmos_sensor(0X6F12, 0X0023);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0X0292);
write_cmos_sensor(0X6F12, 0X0192);
write_cmos_sensor(0X6F12, 0X21B1);
write_cmos_sensor(0X6F12, 0X18B1);
write_cmos_sensor(0X6F12, 0X97F8);
write_cmos_sensor(0X6F12, 0X3201);
write_cmos_sensor(0X6F12, 0X0128);
write_cmos_sensor(0X6F12, 0X07D0);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0X6801);
write_cmos_sensor(0X6F12, 0X4208);
write_cmos_sensor(0X6F12, 0X33B1);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X06E0);
write_cmos_sensor(0X6F12, 0X0123);
write_cmos_sensor(0X6F12, 0XEEE7);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0X6821);
write_cmos_sensor(0X6F12, 0XF7E7);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0XA400);
write_cmos_sensor(0X6F12, 0X4100);
write_cmos_sensor(0X6F12, 0X002A);
write_cmos_sensor(0X6F12, 0X02D0);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X0200);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X41EA);
write_cmos_sensor(0X6F12, 0X4020);
write_cmos_sensor(0X6F12, 0X9349);
write_cmos_sensor(0X6F12, 0XA1F8);
write_cmos_sensor(0X6F12, 0X5201);
write_cmos_sensor(0X6F12, 0X07D0);
write_cmos_sensor(0X6F12, 0X9248);
write_cmos_sensor(0X6F12, 0X6B46);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X4810);
write_cmos_sensor(0X6F12, 0X4FF2);
write_cmos_sensor(0X6F12, 0X5010);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X83FE);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0X2146);
write_cmos_sensor(0X6F12, 0X2846);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X74FE);
write_cmos_sensor(0X6F12, 0XFEBD);
write_cmos_sensor(0X6F12, 0X2DE9);
write_cmos_sensor(0X6F12, 0XF047);
write_cmos_sensor(0X6F12, 0X0546);
write_cmos_sensor(0X6F12, 0X8848);
write_cmos_sensor(0X6F12, 0X9046);
write_cmos_sensor(0X6F12, 0X8946);
write_cmos_sensor(0X6F12, 0X8068);
write_cmos_sensor(0X6F12, 0X1C46);
write_cmos_sensor(0X6F12, 0X86B2);
write_cmos_sensor(0X6F12, 0X070C);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X3146);
write_cmos_sensor(0X6F12, 0X3846);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X64FE);
write_cmos_sensor(0X6F12, 0X2346);
write_cmos_sensor(0X6F12, 0X4246);
write_cmos_sensor(0X6F12, 0X4946);
write_cmos_sensor(0X6F12, 0X2846);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X68FE);
write_cmos_sensor(0X6F12, 0X7A48);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0XB310);
write_cmos_sensor(0X6F12, 0XD1B1);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0XFD00);
write_cmos_sensor(0X6F12, 0XB8B1);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0XA5F5);
write_cmos_sensor(0X6F12, 0X7141);
write_cmos_sensor(0X6F12, 0X05F1);
write_cmos_sensor(0X6F12, 0X8044);
write_cmos_sensor(0X6F12, 0X9039);
write_cmos_sensor(0X6F12, 0X02D0);
write_cmos_sensor(0X6F12, 0X4031);
write_cmos_sensor(0X6F12, 0X02D0);
write_cmos_sensor(0X6F12, 0X0DE0);
write_cmos_sensor(0X6F12, 0XA081);
write_cmos_sensor(0X6F12, 0X0BE0);
write_cmos_sensor(0X6F12, 0XA081);
write_cmos_sensor(0X6F12, 0X7448);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0X3201);
write_cmos_sensor(0X6F12, 0X0128);
write_cmos_sensor(0X6F12, 0X05D1);
write_cmos_sensor(0X6F12, 0X0220);
write_cmos_sensor(0X6F12, 0XA080);
write_cmos_sensor(0X6F12, 0X2D20);
write_cmos_sensor(0X6F12, 0X2081);
write_cmos_sensor(0X6F12, 0X2E20);
write_cmos_sensor(0X6F12, 0X6081);
write_cmos_sensor(0X6F12, 0X3146);
write_cmos_sensor(0X6F12, 0X3846);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XF047);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X38BE);
write_cmos_sensor(0X6F12, 0XF0B5);
write_cmos_sensor(0X6F12, 0X30F8);
write_cmos_sensor(0X6F12, 0X4A3F);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0XC488);
write_cmos_sensor(0X6F12, 0X6746);
write_cmos_sensor(0X6F12, 0X1B1B);
write_cmos_sensor(0X6F12, 0X0489);
write_cmos_sensor(0X6F12, 0X30F8);
write_cmos_sensor(0X6F12, 0X0A0C);
write_cmos_sensor(0X6F12, 0X1B1B);
write_cmos_sensor(0X6F12, 0X9DB2);
write_cmos_sensor(0X6F12, 0X32F8);
write_cmos_sensor(0X6F12, 0X5A3F);
write_cmos_sensor(0X6F12, 0X8C78);
write_cmos_sensor(0X6F12, 0X117C);
write_cmos_sensor(0X6F12, 0X92F8);
write_cmos_sensor(0X6F12, 0X58E0);
write_cmos_sensor(0X6F12, 0X04FB);
write_cmos_sensor(0X6F12, 0X1133);
write_cmos_sensor(0X6F12, 0X9BB2);
write_cmos_sensor(0X6F12, 0X9489);
write_cmos_sensor(0X6F12, 0XA0EB);
write_cmos_sensor(0X6F12, 0X0E00);
write_cmos_sensor(0X6F12, 0X04FB);
write_cmos_sensor(0X6F12, 0X0134);
write_cmos_sensor(0X6F12, 0X12F8);
write_cmos_sensor(0X6F12, 0X3F6C);
write_cmos_sensor(0X6F12, 0X80B2);
write_cmos_sensor(0X6F12, 0X00FB);
write_cmos_sensor(0X6F12, 0X013E);
write_cmos_sensor(0X6F12, 0XA41B);
write_cmos_sensor(0X6F12, 0XD27C);
write_cmos_sensor(0X6F12, 0XA4B2);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X8EFE);
write_cmos_sensor(0X6F12, 0X22B1);
write_cmos_sensor(0X6F12, 0X3444);
write_cmos_sensor(0X6F12, 0X00FB);
write_cmos_sensor(0X6F12, 0X1144);
write_cmos_sensor(0X6F12, 0X641E);
write_cmos_sensor(0X6F12, 0X07E0);
write_cmos_sensor(0X6F12, 0X20B1);
write_cmos_sensor(0X6F12, 0XAEF1);
write_cmos_sensor(0X6F12, 0X0104);
write_cmos_sensor(0X6F12, 0X05FB);
write_cmos_sensor(0X6F12, 0X0144);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0X641E);
write_cmos_sensor(0X6F12, 0X3444);
write_cmos_sensor(0X6F12, 0XA4B2);
write_cmos_sensor(0X6F12, 0X2AB1);
write_cmos_sensor(0X6F12, 0X10B1);
write_cmos_sensor(0X6F12, 0X05FB);
write_cmos_sensor(0X6F12, 0X1143);
write_cmos_sensor(0X6F12, 0X5B1C);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X83FE);
write_cmos_sensor(0X6F12, 0XBEF1);
write_cmos_sensor(0X6F12, 0X700F);
write_cmos_sensor(0X6F12, 0X02D2);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X010E);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0XAEF1);
write_cmos_sensor(0X6F12, 0X6F0E);
write_cmos_sensor(0X6F12, 0X4FF4);
write_cmos_sensor(0X6F12, 0X4061);
write_cmos_sensor(0X6F12, 0X6F3C);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X8EF0);
write_cmos_sensor(0X6F12, 0X8C42);
write_cmos_sensor(0X6F12, 0X00DD);
write_cmos_sensor(0X6F12, 0X0C46);
write_cmos_sensor(0X6F12, 0X211A);
write_cmos_sensor(0X6F12, 0X491C);
write_cmos_sensor(0X6F12, 0X8BB2);
write_cmos_sensor(0X6F12, 0X401E);
write_cmos_sensor(0X6F12, 0XC117);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X9161);
write_cmos_sensor(0X6F12, 0X21F0);
write_cmos_sensor(0X6F12, 0X3F01);
write_cmos_sensor(0X6F12, 0X401A);
write_cmos_sensor(0X6F12, 0XC117);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X1171);
write_cmos_sensor(0X6F12, 0X0911);
write_cmos_sensor(0X6F12, 0XA0EB);
write_cmos_sensor(0X6F12, 0X0110);
write_cmos_sensor(0X6F12, 0XC0F1);
write_cmos_sensor(0X6F12, 0X1000);
write_cmos_sensor(0X6F12, 0X81B2);
write_cmos_sensor(0X6F12, 0X04F0);
write_cmos_sensor(0X6F12, 0X0F00);
write_cmos_sensor(0X6F12, 0XCC06);
write_cmos_sensor(0X6F12, 0X00D5);
write_cmos_sensor(0X6F12, 0X0021);
write_cmos_sensor(0X6F12, 0XC406);
write_cmos_sensor(0X6F12, 0X00D5);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X0C18);
write_cmos_sensor(0X6F12, 0X1B1B);
write_cmos_sensor(0X6F12, 0X9BB2);
write_cmos_sensor(0X6F12, 0X89B1);
write_cmos_sensor(0X6F12, 0XB2B1);
write_cmos_sensor(0X6F12, 0X0829);
write_cmos_sensor(0X6F12, 0X02D9);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X040C);
write_cmos_sensor(0X6F12, 0X0BE0);
write_cmos_sensor(0X6F12, 0X0529);
write_cmos_sensor(0X6F12, 0X02D9);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X030C);
write_cmos_sensor(0X6F12, 0X06E0);
write_cmos_sensor(0X6F12, 0X0429);
write_cmos_sensor(0X6F12, 0X02D9);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X020C);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X010C);
write_cmos_sensor(0X6F12, 0XB0B1);
write_cmos_sensor(0X6F12, 0X72B1);
write_cmos_sensor(0X6F12, 0X0828);
write_cmos_sensor(0X6F12, 0X06D2);
write_cmos_sensor(0X6F12, 0X0027);
write_cmos_sensor(0X6F12, 0X11E0);
write_cmos_sensor(0X6F12, 0X0829);
write_cmos_sensor(0X6F12, 0XE8D8);
write_cmos_sensor(0X6F12, 0X0429);
write_cmos_sensor(0X6F12, 0XF3D9);
write_cmos_sensor(0X6F12, 0XEAE7);
write_cmos_sensor(0X6F12, 0X0C28);
write_cmos_sensor(0X6F12, 0X01D2);
write_cmos_sensor(0X6F12, 0X0127);
write_cmos_sensor(0X6F12, 0X08E0);
write_cmos_sensor(0X6F12, 0X0327);
write_cmos_sensor(0X6F12, 0X06E0);
write_cmos_sensor(0X6F12, 0X0828);
write_cmos_sensor(0X6F12, 0XF0D3);
write_cmos_sensor(0X6F12, 0X0C28);
write_cmos_sensor(0X6F12, 0XF7D3);
write_cmos_sensor(0X6F12, 0X0D28);
write_cmos_sensor(0X6F12, 0XF7D2);
write_cmos_sensor(0X6F12, 0X0227);
write_cmos_sensor(0X6F12, 0X9809);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6F12, 0XC3F3);
write_cmos_sensor(0X6F12, 0X0111);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X8100);
write_cmos_sensor(0X6F12, 0X6044);
write_cmos_sensor(0X6F12, 0X3844);
write_cmos_sensor(0X6F12, 0X80B2);
write_cmos_sensor(0X6F12, 0XF0BD);
write_cmos_sensor(0X6F12, 0X2DE9);
write_cmos_sensor(0X6F12, 0XFF41);
write_cmos_sensor(0X6F12, 0X8046);
write_cmos_sensor(0X6F12, 0X1B48);
write_cmos_sensor(0X6F12, 0X1446);
write_cmos_sensor(0X6F12, 0X0F46);
write_cmos_sensor(0X6F12, 0X4069);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X85B2);
write_cmos_sensor(0X6F12, 0X060C);
write_cmos_sensor(0X6F12, 0X2946);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X8CFD);
write_cmos_sensor(0X6F12, 0X2246);
write_cmos_sensor(0X6F12, 0X3946);
write_cmos_sensor(0X6F12, 0X4046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X96FD);
write_cmos_sensor(0X6F12, 0X0C4A);
write_cmos_sensor(0X6F12, 0X92F8);
write_cmos_sensor(0X6F12, 0XCC00);
write_cmos_sensor(0X6F12, 0X58B3);
write_cmos_sensor(0X6F12, 0X97F8);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X40B3);
write_cmos_sensor(0X6F12, 0X4FF4);
write_cmos_sensor(0X6F12, 0X8050);
write_cmos_sensor(0X6F12, 0X0090);
write_cmos_sensor(0X6F12, 0X1148);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0XBA13);
write_cmos_sensor(0X6F12, 0XB1FA);
write_cmos_sensor(0X6F12, 0X81F0);
write_cmos_sensor(0X6F12, 0XC0F1);
write_cmos_sensor(0X6F12, 0X1700);
write_cmos_sensor(0X6F12, 0XC140);
write_cmos_sensor(0X6F12, 0XC9B2);
write_cmos_sensor(0X6F12, 0X02EB);
write_cmos_sensor(0X6F12, 0X4000);
write_cmos_sensor(0X6F12, 0X1AE0);
write_cmos_sensor(0X6F12, 0X4000);
write_cmos_sensor(0X6F12, 0XE000);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1350);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X4A00);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X2970);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1EB0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1BAA);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1B58);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X2530);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1CC0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X3FF0);
write_cmos_sensor(0X6F12, 0X4000);
write_cmos_sensor(0X6F12, 0XF000);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X05C0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X2120);
write_cmos_sensor(0X6F12, 0X26E0);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0XD030);
write_cmos_sensor(0X6F12, 0X30F8);
write_cmos_sensor(0X6F12, 0XCE2F);
write_cmos_sensor(0X6F12, 0X9B1A);
write_cmos_sensor(0X6F12, 0X4B43);
write_cmos_sensor(0X6F12, 0X8033);
write_cmos_sensor(0X6F12, 0X02EB);
write_cmos_sensor(0X6F12, 0X2322);
write_cmos_sensor(0X6F12, 0X0192);
write_cmos_sensor(0X6F12, 0X8389);
write_cmos_sensor(0X6F12, 0X4289);
write_cmos_sensor(0X6F12, 0X9B1A);
write_cmos_sensor(0X6F12, 0X4B43);
write_cmos_sensor(0X6F12, 0X8033);
write_cmos_sensor(0X6F12, 0X02EB);
write_cmos_sensor(0X6F12, 0X2322);
write_cmos_sensor(0X6F12, 0X0292);
write_cmos_sensor(0X6F12, 0XC28A);
write_cmos_sensor(0X6F12, 0X808A);
write_cmos_sensor(0X6F12, 0X121A);
write_cmos_sensor(0X6F12, 0X4A43);
write_cmos_sensor(0X6F12, 0X8032);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X2220);
write_cmos_sensor(0X6F12, 0X0023);
write_cmos_sensor(0X6F12, 0X6946);
write_cmos_sensor(0X6F12, 0X0390);
write_cmos_sensor(0X6F12, 0X54F8);
write_cmos_sensor(0X6F12, 0X2300);
write_cmos_sensor(0X6F12, 0X51F8);
write_cmos_sensor(0X6F12, 0X2320);
write_cmos_sensor(0X6F12, 0X5043);
write_cmos_sensor(0X6F12, 0X000B);
write_cmos_sensor(0X6F12, 0X44F8);
write_cmos_sensor(0X6F12, 0X2300);
write_cmos_sensor(0X6F12, 0X5B1C);
write_cmos_sensor(0X6F12, 0X042B);
write_cmos_sensor(0X6F12, 0XF4D3);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0X2946);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X2AFD);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XFF81);
write_cmos_sensor(0X6F12, 0X70B5);
write_cmos_sensor(0X6F12, 0X0123);
write_cmos_sensor(0X6F12, 0X11E0);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2350);
write_cmos_sensor(0X6F12, 0X5A1E);
write_cmos_sensor(0X6F12, 0X03E0);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X8206);
write_cmos_sensor(0X6F12, 0X521E);
write_cmos_sensor(0X6F12, 0X7460);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2240);
write_cmos_sensor(0X6F12, 0XAC42);
write_cmos_sensor(0X6F12, 0X01D9);
write_cmos_sensor(0X6F12, 0X002A);
write_cmos_sensor(0X6F12, 0XF5DA);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X8202);
write_cmos_sensor(0X6F12, 0X5B1C);
write_cmos_sensor(0X6F12, 0X5560);
write_cmos_sensor(0X6F12, 0X8B42);
write_cmos_sensor(0X6F12, 0XEBDB);
write_cmos_sensor(0X6F12, 0X70BD);
write_cmos_sensor(0X6F12, 0X70B5);
write_cmos_sensor(0X6F12, 0X0123);
write_cmos_sensor(0X6F12, 0X11E0);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2350);
write_cmos_sensor(0X6F12, 0X5A1E);
write_cmos_sensor(0X6F12, 0X03E0);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X8206);
write_cmos_sensor(0X6F12, 0X521E);
write_cmos_sensor(0X6F12, 0X7460);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2240);
write_cmos_sensor(0X6F12, 0XAC42);
write_cmos_sensor(0X6F12, 0X01D2);
write_cmos_sensor(0X6F12, 0X002A);
write_cmos_sensor(0X6F12, 0XF5DA);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X8202);
write_cmos_sensor(0X6F12, 0X5B1C);
write_cmos_sensor(0X6F12, 0X5560);
write_cmos_sensor(0X6F12, 0X8B42);
write_cmos_sensor(0X6F12, 0XEBDB);
write_cmos_sensor(0X6F12, 0X70BD);
write_cmos_sensor(0X6F12, 0X2DE9);
write_cmos_sensor(0X6F12, 0XF34F);
write_cmos_sensor(0X6F12, 0XF84F);
write_cmos_sensor(0X6F12, 0X0646);
write_cmos_sensor(0X6F12, 0X91B0);
write_cmos_sensor(0X6F12, 0XB87F);
write_cmos_sensor(0X6F12, 0X0028);
write_cmos_sensor(0X6F12, 0X79D0);
write_cmos_sensor(0X6F12, 0X307E);
write_cmos_sensor(0X6F12, 0X717E);
write_cmos_sensor(0X6F12, 0X129C);
write_cmos_sensor(0X6F12, 0X0844);
write_cmos_sensor(0X6F12, 0X0690);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0X6B10);
write_cmos_sensor(0X6F12, 0XB07E);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XFAFC);
write_cmos_sensor(0X6F12, 0X1090);
write_cmos_sensor(0X6F12, 0XB07E);
write_cmos_sensor(0X6F12, 0XF17E);
write_cmos_sensor(0X6F12, 0X0844);
write_cmos_sensor(0X6F12, 0X0590);
write_cmos_sensor(0X6F12, 0X0090);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0X6D00);
write_cmos_sensor(0X6F12, 0X08B1);
write_cmos_sensor(0X6F12, 0X0125);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X0025);
write_cmos_sensor(0X6F12, 0XB6F8);
write_cmos_sensor(0X6F12, 0X5800);
write_cmos_sensor(0X6F12, 0X0699);
write_cmos_sensor(0X6F12, 0XC5F1);
write_cmos_sensor(0X6F12, 0X0108);
write_cmos_sensor(0X6F12, 0XB0FB);
write_cmos_sensor(0X6F12, 0XF1F2);
write_cmos_sensor(0X6F12, 0X01FB);
write_cmos_sensor(0X6F12, 0X1201);
write_cmos_sensor(0X6F12, 0X069A);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X000A);
write_cmos_sensor(0X6F12, 0XB0FB);
write_cmos_sensor(0X6F12, 0XF2F0);
write_cmos_sensor(0X6F12, 0XB6F8);
write_cmos_sensor(0X6F12, 0X8E20);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X5200);
write_cmos_sensor(0X6F12, 0X0990);
write_cmos_sensor(0X6F12, 0XE148);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X8000);
write_cmos_sensor(0X6F12, 0X401A);
write_cmos_sensor(0X6F12, 0X0F90);
write_cmos_sensor(0X6F12, 0X45B1);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0XFF30);
write_cmos_sensor(0X6F12, 0X0A90);
write_cmos_sensor(0X6F12, 0XD7E9);
write_cmos_sensor(0X6F12, 0X0419);
write_cmos_sensor(0X6F12, 0X091F);
write_cmos_sensor(0X6F12, 0X0B91);
write_cmos_sensor(0X6F12, 0X0146);
write_cmos_sensor(0X6F12, 0X07E0);
write_cmos_sensor(0X6F12, 0X0120);
write_cmos_sensor(0X6F12, 0X0A90);
write_cmos_sensor(0X6F12, 0XD7E9);
write_cmos_sensor(0X6F12, 0X0490);
write_cmos_sensor(0X6F12, 0X001D);
write_cmos_sensor(0X6F12, 0X0B90);
write_cmos_sensor(0X6F12, 0X0099);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0X6C20);
write_cmos_sensor(0X6F12, 0X12B1);
write_cmos_sensor(0X6F12, 0XAFF2);
write_cmos_sensor(0X6F12, 0XC902);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0XAFF2);
write_cmos_sensor(0X6F12, 0XFD02);
write_cmos_sensor(0X6F12, 0XD34B);
write_cmos_sensor(0X6F12, 0X9A61);
write_cmos_sensor(0X6F12, 0XD04B);
write_cmos_sensor(0X6F12, 0X9A68);
write_cmos_sensor(0X6F12, 0X0392);
write_cmos_sensor(0X6F12, 0XDB68);
write_cmos_sensor(0X6F12, 0X0493);
write_cmos_sensor(0X6F12, 0XCDE9);
write_cmos_sensor(0X6F12, 0X0123);
write_cmos_sensor(0X6F12, 0X7288);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0X6A30);
write_cmos_sensor(0X6F12, 0X368E);
write_cmos_sensor(0X6F12, 0X03FB);
write_cmos_sensor(0X6F12, 0X0622);
write_cmos_sensor(0X6F12, 0X0892);
write_cmos_sensor(0X6F12, 0XD0E0);
write_cmos_sensor(0X6F12, 0X0A9E);
write_cmos_sensor(0X6F12, 0XD9F8);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X09EB);
write_cmos_sensor(0X6F12, 0X8609);
write_cmos_sensor(0X6F12, 0X089E);
write_cmos_sensor(0X6F12, 0X130C);
write_cmos_sensor(0X6F12, 0XB342);
write_cmos_sensor(0X6F12, 0XF5D3);
write_cmos_sensor(0X6F12, 0X9E1B);
write_cmos_sensor(0X6F12, 0X0F9B);
write_cmos_sensor(0X6F12, 0X92B2);
write_cmos_sensor(0X6F12, 0X069F);
write_cmos_sensor(0X6F12, 0X1344);
write_cmos_sensor(0X6F12, 0XB3FB);
write_cmos_sensor(0X6F12, 0XF7F3);
write_cmos_sensor(0X6F12, 0X099F);
write_cmos_sensor(0X6F12, 0XBB42);
write_cmos_sensor(0X6F12, 0X7DD3);
write_cmos_sensor(0X6F12, 0XDB1B);
write_cmos_sensor(0X6F12, 0X63F3);
write_cmos_sensor(0X6F12, 0X5F02);
write_cmos_sensor(0X6F12, 0X0E92);
write_cmos_sensor(0X6F12, 0XB8F1);
write_cmos_sensor(0X6F12, 0X000F);
write_cmos_sensor(0X6F12, 0X01D0);
write_cmos_sensor(0X6F12, 0X8E42);
write_cmos_sensor(0X6F12, 0X02D2);
write_cmos_sensor(0X6F12, 0X25B1);
write_cmos_sensor(0X6F12, 0X8642);
write_cmos_sensor(0X6F12, 0X02D2);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0X39E1);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X002A);
write_cmos_sensor(0X6F12, 0X6CD0);
write_cmos_sensor(0X6F12, 0X01A8);
write_cmos_sensor(0X6F12, 0X4AEA);
write_cmos_sensor(0X6F12, 0X0547);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2500);
write_cmos_sensor(0X6F12, 0X0D90);
write_cmos_sensor(0X6F12, 0X03A8);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X25B0);
write_cmos_sensor(0X6F12, 0X0D98);
write_cmos_sensor(0X6F12, 0XA0EB);
write_cmos_sensor(0X6F12, 0X0B00);
write_cmos_sensor(0X6F12, 0X8010);
write_cmos_sensor(0X6F12, 0X0790);
write_cmos_sensor(0X6F12, 0XB048);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0XFC00);
write_cmos_sensor(0X6F12, 0X08B1);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X02E0);
write_cmos_sensor(0X6F12, 0XAD48);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X2C01);
write_cmos_sensor(0X6F12, 0X0C90);
write_cmos_sensor(0X6F12, 0XAD48);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X4E15);
write_cmos_sensor(0X6F12, 0X0798);
write_cmos_sensor(0X6F12, 0X8142);
write_cmos_sensor(0X6F12, 0X03D2);
write_cmos_sensor(0X6F12, 0X390C);
write_cmos_sensor(0X6F12, 0X3620);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X69FC);
write_cmos_sensor(0X6F12, 0XA648);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0XFD00);
write_cmos_sensor(0X6F12, 0X20B9);
write_cmos_sensor(0X6F12, 0XA448);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X2801);
write_cmos_sensor(0X6F12, 0X07EB);
write_cmos_sensor(0X6F12, 0X0047);
write_cmos_sensor(0X6F12, 0X0798);
write_cmos_sensor(0X6F12, 0X0128);
write_cmos_sensor(0X6F12, 0X07D0);
write_cmos_sensor(0X6F12, 0X15D9);
write_cmos_sensor(0X6F12, 0XA04A);
write_cmos_sensor(0X6F12, 0X0146);
write_cmos_sensor(0X6F12, 0X5846);
write_cmos_sensor(0X6F12, 0X9269);
write_cmos_sensor(0X6F12, 0X9047);
write_cmos_sensor(0X6F12, 0X5846);
write_cmos_sensor(0X6F12, 0X0BE0);
write_cmos_sensor(0X6F12, 0XDBF8);
write_cmos_sensor(0X6F12, 0X0010);
write_cmos_sensor(0X6F12, 0X0C98);
write_cmos_sensor(0X6F12, 0X0844);
write_cmos_sensor(0X6F12, 0X3843);
write_cmos_sensor(0X6F12, 0X01C4);
write_cmos_sensor(0X6F12, 0X07E0);
write_cmos_sensor(0X6F12, 0X04C8);
write_cmos_sensor(0X6F12, 0X0C99);
write_cmos_sensor(0X6F12, 0X1144);
write_cmos_sensor(0X6F12, 0X3943);
write_cmos_sensor(0X6F12, 0X02C4);
write_cmos_sensor(0X6F12, 0X0D99);
write_cmos_sensor(0X6F12, 0X8842);
write_cmos_sensor(0X6F12, 0XF7D1);
write_cmos_sensor(0X6F12, 0X01A8);
write_cmos_sensor(0X6F12, 0X4AEA);
write_cmos_sensor(0X6F12, 0X0847);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2800);
write_cmos_sensor(0X6F12, 0X0C90);
write_cmos_sensor(0X6F12, 0X03A8);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X28B0);
write_cmos_sensor(0X6F12, 0X0C98);
write_cmos_sensor(0X6F12, 0XA0EB);
write_cmos_sensor(0X6F12, 0X0B00);
write_cmos_sensor(0X6F12, 0X8010);
write_cmos_sensor(0X6F12, 0X0790);
write_cmos_sensor(0X6F12, 0X8D48);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0XFC00);
write_cmos_sensor(0X6F12, 0X08B1);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X02E0);
write_cmos_sensor(0X6F12, 0X8A48);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X2C01);
write_cmos_sensor(0X6F12, 0X8246);
write_cmos_sensor(0X6F12, 0X8A48);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X4E15);
write_cmos_sensor(0X6F12, 0X0798);
write_cmos_sensor(0X6F12, 0X8142);
write_cmos_sensor(0X6F12, 0X03D2);
write_cmos_sensor(0X6F12, 0X390C);
write_cmos_sensor(0X6F12, 0X3620);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X24FC);
write_cmos_sensor(0X6F12, 0X8348);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0XFD10);
write_cmos_sensor(0X6F12, 0X31B9);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0X3EE0);
write_cmos_sensor(0X6F12, 0X2FE0);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X2801);
write_cmos_sensor(0X6F12, 0X07EB);
write_cmos_sensor(0X6F12, 0X0047);
write_cmos_sensor(0X6F12, 0X0798);
write_cmos_sensor(0X6F12, 0X0128);
write_cmos_sensor(0X6F12, 0X07D0);
write_cmos_sensor(0X6F12, 0X15D9);
write_cmos_sensor(0X6F12, 0X7D4A);
write_cmos_sensor(0X6F12, 0X0146);
write_cmos_sensor(0X6F12, 0X5846);
write_cmos_sensor(0X6F12, 0X9269);
write_cmos_sensor(0X6F12, 0X9047);
write_cmos_sensor(0X6F12, 0X5846);
write_cmos_sensor(0X6F12, 0X0BE0);
write_cmos_sensor(0X6F12, 0XDBF8);
write_cmos_sensor(0X6F12, 0X0010);
write_cmos_sensor(0X6F12, 0X01EB);
write_cmos_sensor(0X6F12, 0X0A00);
write_cmos_sensor(0X6F12, 0X3843);
write_cmos_sensor(0X6F12, 0X01C4);
write_cmos_sensor(0X6F12, 0X07E0);
write_cmos_sensor(0X6F12, 0X04C8);
write_cmos_sensor(0X6F12, 0X02EB);
write_cmos_sensor(0X6F12, 0X0A01);
write_cmos_sensor(0X6F12, 0X3943);
write_cmos_sensor(0X6F12, 0X02C4);
write_cmos_sensor(0X6F12, 0X0C99);
write_cmos_sensor(0X6F12, 0X8842);
write_cmos_sensor(0X6F12, 0XF7D1);
write_cmos_sensor(0X6F12, 0X0398);
write_cmos_sensor(0X6F12, 0X0190);
write_cmos_sensor(0X6F12, 0X0498);
write_cmos_sensor(0X6F12, 0X0290);
write_cmos_sensor(0X6F12, 0X0598);
write_cmos_sensor(0X6F12, 0XB6FB);
write_cmos_sensor(0X6F12, 0XF0F1);
write_cmos_sensor(0X6F12, 0XB6FB);
write_cmos_sensor(0X6F12, 0XF0F2);
write_cmos_sensor(0X6F12, 0X00FB);
write_cmos_sensor(0X6F12, 0X1163);
write_cmos_sensor(0X6F12, 0X0098);
write_cmos_sensor(0X6F12, 0X0099);
write_cmos_sensor(0X6F12, 0X5043);
write_cmos_sensor(0X6F12, 0X0144);
write_cmos_sensor(0X6F12, 0X4FEA);
write_cmos_sensor(0X6F12, 0X424A);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X331A);
write_cmos_sensor(0X6F12, 0X109A);
write_cmos_sensor(0X6F12, 0XDA40);
write_cmos_sensor(0X6F12, 0XD207);
write_cmos_sensor(0X6F12, 0X08D0);
write_cmos_sensor(0X6F12, 0X06F0);
write_cmos_sensor(0X6F12, 0X0103);
write_cmos_sensor(0X6F12, 0X01AE);
write_cmos_sensor(0X6F12, 0X0E9F);
write_cmos_sensor(0X6F12, 0X56F8);
write_cmos_sensor(0X6F12, 0X2320);
write_cmos_sensor(0X6F12, 0X80C2);
write_cmos_sensor(0X6F12, 0X46F8);
write_cmos_sensor(0X6F12, 0X2320);
write_cmos_sensor(0X6F12, 0X0B9B);
write_cmos_sensor(0X6F12, 0X9945);
write_cmos_sensor(0X6F12, 0X7FF4);
write_cmos_sensor(0X6F12, 0X2BAF);
write_cmos_sensor(0X6F12, 0X01A8);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2590);
write_cmos_sensor(0X6F12, 0X03A8);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2560);
write_cmos_sensor(0X6F12, 0X4AEA);
write_cmos_sensor(0X6F12, 0X0545);
write_cmos_sensor(0X6F12, 0XA9EB);
write_cmos_sensor(0X6F12, 0X0600);
write_cmos_sensor(0X6F12, 0X4FEA);
write_cmos_sensor(0X6F12, 0XA00B);
write_cmos_sensor(0X6F12, 0X5948);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0XFC00);
write_cmos_sensor(0X6F12, 0X08B1);
write_cmos_sensor(0X6F12, 0X0027);
write_cmos_sensor(0X6F12, 0X02E0);
write_cmos_sensor(0X6F12, 0X5648);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X2C71);
write_cmos_sensor(0X6F12, 0X5648);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X4E15);
write_cmos_sensor(0X6F12, 0X5945);
write_cmos_sensor(0X6F12, 0X03D2);
write_cmos_sensor(0X6F12, 0X290C);
write_cmos_sensor(0X6F12, 0X3620);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XBDFB);
write_cmos_sensor(0X6F12, 0X5048);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0XFD00);
write_cmos_sensor(0X6F12, 0X20B9);
write_cmos_sensor(0X6F12, 0X4E48);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X2801);
write_cmos_sensor(0X6F12, 0X05EB);
write_cmos_sensor(0X6F12, 0X0045);
write_cmos_sensor(0X6F12, 0X5846);
write_cmos_sensor(0X6F12, 0XBBF1);
write_cmos_sensor(0X6F12, 0X010F);
write_cmos_sensor(0X6F12, 0X06D0);
write_cmos_sensor(0X6F12, 0X10D9);
write_cmos_sensor(0X6F12, 0X4A4A);
write_cmos_sensor(0X6F12, 0X0146);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0X9269);
write_cmos_sensor(0X6F12, 0X9047);
write_cmos_sensor(0X6F12, 0X08E0);
write_cmos_sensor(0X6F12, 0X3068);
write_cmos_sensor(0X6F12, 0X3844);
write_cmos_sensor(0X6F12, 0X2843);
write_cmos_sensor(0X6F12, 0X01C4);
write_cmos_sensor(0X6F12, 0X05E0);
write_cmos_sensor(0X6F12, 0X01CE);
write_cmos_sensor(0X6F12, 0X3844);
write_cmos_sensor(0X6F12, 0X2843);
write_cmos_sensor(0X6F12, 0X01C4);
write_cmos_sensor(0X6F12, 0X4E45);
write_cmos_sensor(0X6F12, 0XF9D1);
write_cmos_sensor(0X6F12, 0X01A8);
write_cmos_sensor(0X6F12, 0X4AEA);
write_cmos_sensor(0X6F12, 0X0845);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2890);
write_cmos_sensor(0X6F12, 0X03A8);
write_cmos_sensor(0X6F12, 0XDFF8);
write_cmos_sensor(0X6F12, 0XF4A0);
write_cmos_sensor(0X6F12, 0X50F8);
write_cmos_sensor(0X6F12, 0X2860);
write_cmos_sensor(0X6F12, 0XA9EB);
write_cmos_sensor(0X6F12, 0X0600);
write_cmos_sensor(0X6F12, 0X4FEA);
write_cmos_sensor(0X6F12, 0XA008);
write_cmos_sensor(0X6F12, 0X9AF8);
write_cmos_sensor(0X6F12, 0XFC00);
write_cmos_sensor(0X6F12, 0X08B1);
write_cmos_sensor(0X6F12, 0X0027);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0XBAF8);
write_cmos_sensor(0X6F12, 0X2C71);
write_cmos_sensor(0X6F12, 0X3848);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X4E15);
write_cmos_sensor(0X6F12, 0X4145);
write_cmos_sensor(0X6F12, 0X03D2);
write_cmos_sensor(0X6F12, 0X290C);
write_cmos_sensor(0X6F12, 0X3620);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X80FB);
write_cmos_sensor(0X6F12, 0X9AF8);
write_cmos_sensor(0X6F12, 0XFD10);
write_cmos_sensor(0X6F12, 0X19B9);
write_cmos_sensor(0X6F12, 0XBAF8);
write_cmos_sensor(0X6F12, 0X2801);
write_cmos_sensor(0X6F12, 0X05EB);
write_cmos_sensor(0X6F12, 0X0045);
write_cmos_sensor(0X6F12, 0X4046);
write_cmos_sensor(0X6F12, 0XB8F1);
write_cmos_sensor(0X6F12, 0X010F);
write_cmos_sensor(0X6F12, 0X06D0);
write_cmos_sensor(0X6F12, 0X10D9);
write_cmos_sensor(0X6F12, 0X2C4A);
write_cmos_sensor(0X6F12, 0X0146);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0X9269);
write_cmos_sensor(0X6F12, 0X9047);
write_cmos_sensor(0X6F12, 0X08E0);
write_cmos_sensor(0X6F12, 0X3068);
write_cmos_sensor(0X6F12, 0X3844);
write_cmos_sensor(0X6F12, 0X2843);
write_cmos_sensor(0X6F12, 0X01C4);
write_cmos_sensor(0X6F12, 0X05E0);
write_cmos_sensor(0X6F12, 0X01CE);
write_cmos_sensor(0X6F12, 0X3844);
write_cmos_sensor(0X6F12, 0X2843);
write_cmos_sensor(0X6F12, 0X01C4);
write_cmos_sensor(0X6F12, 0X4E45);
write_cmos_sensor(0X6F12, 0XF9D1);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0XFF30);
write_cmos_sensor(0X6F12, 0X2060);
write_cmos_sensor(0X6F12, 0X1298);
write_cmos_sensor(0X6F12, 0X8442);
write_cmos_sensor(0X6F12, 0X01D0);
write_cmos_sensor(0X6F12, 0X0120);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X1D49);
write_cmos_sensor(0X6F12, 0X0870);
write_cmos_sensor(0X6F12, 0X1298);
write_cmos_sensor(0X6F12, 0X201A);
write_cmos_sensor(0X6F12, 0X8008);
write_cmos_sensor(0X6F12, 0X0884);
write_cmos_sensor(0X6F12, 0X13B0);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XF08F);
write_cmos_sensor(0X6F12, 0X2DE9);
write_cmos_sensor(0X6F12, 0XF041);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0X6920);
write_cmos_sensor(0X6F12, 0X0D46);
write_cmos_sensor(0X6F12, 0X0446);
write_cmos_sensor(0X6F12, 0X032A);
write_cmos_sensor(0X6F12, 0X15D0);
write_cmos_sensor(0X6F12, 0X062A);
write_cmos_sensor(0X6F12, 0X13D0);
write_cmos_sensor(0X6F12, 0X1648);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XC069);
write_cmos_sensor(0X6F12, 0X87B2);
write_cmos_sensor(0X6F12, 0X060C);
write_cmos_sensor(0X6F12, 0X3946);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X25FB);
write_cmos_sensor(0X6F12, 0X2946);
write_cmos_sensor(0X6F12, 0X2046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X3FFB);
write_cmos_sensor(0X6F12, 0X3946);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XF041);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X1ABB);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XF041);
write_cmos_sensor(0X6F12, 0X1EE6);
write_cmos_sensor(0X6F12, 0X0346);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X0029);
write_cmos_sensor(0X6F12, 0X0BD0);
write_cmos_sensor(0X6F12, 0X0749);
write_cmos_sensor(0X6F12, 0X0978);
write_cmos_sensor(0X6F12, 0X0029);
write_cmos_sensor(0X6F12, 0X07D0);
write_cmos_sensor(0X6F12, 0X002A);
write_cmos_sensor(0X6F12, 0X05D0);
write_cmos_sensor(0X6F12, 0X73B1);
write_cmos_sensor(0X6F12, 0X02F0);
write_cmos_sensor(0X6F12, 0X0300);
write_cmos_sensor(0X6F12, 0X0128);
write_cmos_sensor(0X6F12, 0X0CD1);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X7047);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1C20);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1EB0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X3FF0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1350);
write_cmos_sensor(0X6F12, 0X9007);
write_cmos_sensor(0X6F12, 0XF3D0);
write_cmos_sensor(0X6F12, 0X0120);
write_cmos_sensor(0X6F12, 0XF1E7);
write_cmos_sensor(0X6F12, 0X2DE9);
write_cmos_sensor(0X6F12, 0XF05F);
write_cmos_sensor(0X6F12, 0X0C46);
write_cmos_sensor(0X6F12, 0X0546);
write_cmos_sensor(0X6F12, 0X9089);
write_cmos_sensor(0X6F12, 0X518A);
write_cmos_sensor(0X6F12, 0X1646);
write_cmos_sensor(0X6F12, 0X401A);
write_cmos_sensor(0X6F12, 0X918A);
write_cmos_sensor(0X6F12, 0X401A);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X80FA);
write_cmos_sensor(0X6F12, 0XFD48);
write_cmos_sensor(0X6F12, 0XB5F8);
write_cmos_sensor(0X6F12, 0X5A10);
write_cmos_sensor(0X6F12, 0X8278);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6A00);
write_cmos_sensor(0X6F12, 0X02FB);
write_cmos_sensor(0X6F12, 0X1011);
write_cmos_sensor(0X6F12, 0XB5F8);
write_cmos_sensor(0X6F12, 0X6620);
write_cmos_sensor(0X6F12, 0X89B2);
write_cmos_sensor(0X6F12, 0X02FB);
write_cmos_sensor(0X6F12, 0X0010);
write_cmos_sensor(0X6F12, 0XE97E);
write_cmos_sensor(0X6F12, 0X401A);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X80F8);
write_cmos_sensor(0X6F12, 0X7088);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0XB210);
write_cmos_sensor(0X6F12, 0X401A);
write_cmos_sensor(0X6F12, 0X87B2);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X2200);
write_cmos_sensor(0X6F12, 0X30B1);
write_cmos_sensor(0X6F12, 0X2846);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XF4FA);
write_cmos_sensor(0X6F12, 0X10B1);
write_cmos_sensor(0X6F12, 0XF048);
write_cmos_sensor(0X6F12, 0X0088);
write_cmos_sensor(0X6F12, 0X00B1);
write_cmos_sensor(0X6F12, 0X0120);
write_cmos_sensor(0X6F12, 0X2070);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6900);
write_cmos_sensor(0X6F12, 0X0228);
write_cmos_sensor(0X6F12, 0X03D1);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6B00);
write_cmos_sensor(0X6F12, 0X0228);
write_cmos_sensor(0X6F12, 0X14D0);
write_cmos_sensor(0X6F12, 0XB5F8);
write_cmos_sensor(0X6F12, 0X9E00);
write_cmos_sensor(0X6F12, 0X0221);
write_cmos_sensor(0X6F12, 0XB1EB);
write_cmos_sensor(0X6F12, 0X101F);
write_cmos_sensor(0X6F12, 0X03D1);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6B00);
write_cmos_sensor(0X6F12, 0X0228);
write_cmos_sensor(0X6F12, 0X0AD0);
write_cmos_sensor(0X6F12, 0X0023);
write_cmos_sensor(0X6F12, 0X6370);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6D00);
write_cmos_sensor(0X6F12, 0X591C);
write_cmos_sensor(0X6F12, 0X30B1);
write_cmos_sensor(0X6F12, 0XEA7E);
write_cmos_sensor(0X6F12, 0XC8F5);
write_cmos_sensor(0X6F12, 0X4960);
write_cmos_sensor(0X6F12, 0X801A);
write_cmos_sensor(0X6F12, 0X02E0);
write_cmos_sensor(0X6F12, 0X0123);
write_cmos_sensor(0X6F12, 0XF3E7);
write_cmos_sensor(0X6F12, 0X6888);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6B20);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X80F9);
write_cmos_sensor(0X6F12, 0XB9FB);
write_cmos_sensor(0X6F12, 0XF2F0);
write_cmos_sensor(0X6F12, 0X3844);
write_cmos_sensor(0X6F12, 0X80B2);
write_cmos_sensor(0X6F12, 0X6080);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6B20);
write_cmos_sensor(0X6F12, 0X4FF4);
write_cmos_sensor(0X6F12, 0X426E);
write_cmos_sensor(0X6F12, 0XBEFB);
write_cmos_sensor(0X6F12, 0XF2F8);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X0A0C);
write_cmos_sensor(0X6F12, 0XE045);
write_cmos_sensor(0X6F12, 0X06D9);
write_cmos_sensor(0X6F12, 0X2FB1);
write_cmos_sensor(0X6F12, 0X728A);
write_cmos_sensor(0X6F12, 0X02EB);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X0CEB);
write_cmos_sensor(0X6F12, 0X0A02);
write_cmos_sensor(0X6F12, 0X04E0);
write_cmos_sensor(0X6F12, 0XBEFB);
write_cmos_sensor(0X6F12, 0XF2F2);
write_cmos_sensor(0X6F12, 0XB6F8);
write_cmos_sensor(0X6F12, 0X12C0);
write_cmos_sensor(0X6F12, 0X6244);
write_cmos_sensor(0X6F12, 0XA280);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6B20);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X200C);
write_cmos_sensor(0X6F12, 0XBCFB);
write_cmos_sensor(0X6F12, 0XF2F8);
write_cmos_sensor(0X6F12, 0X8045);
write_cmos_sensor(0X6F12, 0X14D2);
write_cmos_sensor(0X6F12, 0X728A);
write_cmos_sensor(0X6F12, 0X0AB1);
write_cmos_sensor(0X6F12, 0X0246);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0XE280);
write_cmos_sensor(0X6F12, 0XB6F8);
write_cmos_sensor(0X6F12, 0X12C0);
write_cmos_sensor(0X6F12, 0X92B2);
write_cmos_sensor(0X6F12, 0X6244);
write_cmos_sensor(0X6F12, 0X92B2);
write_cmos_sensor(0X6F12, 0XE280);
write_cmos_sensor(0X6F12, 0X9346);
write_cmos_sensor(0X6F12, 0X7289);
write_cmos_sensor(0X6F12, 0X02FB);
write_cmos_sensor(0X6F12, 0X01FC);
write_cmos_sensor(0X6F12, 0XBCF5);
write_cmos_sensor(0X6F12, 0X7C6F);
write_cmos_sensor(0X6F12, 0X04DD);
write_cmos_sensor(0X6F12, 0X7E22);
write_cmos_sensor(0X6F12, 0X1AE0);
write_cmos_sensor(0X6F12, 0XBCFB);
write_cmos_sensor(0X6F12, 0XF2F2);
write_cmos_sensor(0X6F12, 0XECE7);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X400C);
write_cmos_sensor(0X6F12, 0XBCFB);
write_cmos_sensor(0X6F12, 0XF1FC);
write_cmos_sensor(0X6F12, 0XB2FB);
write_cmos_sensor(0X6F12, 0XFCF8);
write_cmos_sensor(0X6F12, 0X0CFB);
write_cmos_sensor(0X6F12, 0X182C);
write_cmos_sensor(0X6F12, 0XBCF1);
write_cmos_sensor(0X6F12, 0X000F);
write_cmos_sensor(0X6F12, 0X4FF0);
write_cmos_sensor(0X6F12, 0X400C);
write_cmos_sensor(0X6F12, 0XBCFB);
write_cmos_sensor(0X6F12, 0XF1FC);
write_cmos_sensor(0X6F12, 0XB2FB);
write_cmos_sensor(0X6F12, 0XFCF2);
write_cmos_sensor(0X6F12, 0X02D0);
write_cmos_sensor(0X6F12, 0X5200);
write_cmos_sensor(0X6F12, 0X921C);
write_cmos_sensor(0X6F12, 0X03E0);
write_cmos_sensor(0X6F12, 0X4FF6);
write_cmos_sensor(0X6F12, 0XFF7C);
write_cmos_sensor(0X6F12, 0X0CEA);
write_cmos_sensor(0X6F12, 0X4202);
write_cmos_sensor(0X6F12, 0X2281);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X82FC);
write_cmos_sensor(0X6F12, 0X0C22);
write_cmos_sensor(0X6F12, 0XBCFB);
write_cmos_sensor(0X6F12, 0XF2F8);
write_cmos_sensor(0X6F12, 0XA4F8);
write_cmos_sensor(0X6F12, 0X0A80);
write_cmos_sensor(0X6F12, 0XBCFB);
write_cmos_sensor(0X6F12, 0XF2F8);
write_cmos_sensor(0X6F12, 0X02FB);
write_cmos_sensor(0X6F12, 0X18C2);
write_cmos_sensor(0X6F12, 0XA281);
write_cmos_sensor(0X6F12, 0XAB4A);
write_cmos_sensor(0X6F12, 0X5288);
write_cmos_sensor(0X6F12, 0XE281);
write_cmos_sensor(0X6F12, 0XB6F8);
write_cmos_sensor(0X6F12, 0X0AC0);
write_cmos_sensor(0X6F12, 0X6FF0);
write_cmos_sensor(0X6F12, 0X0A02);
write_cmos_sensor(0X6F12, 0X02EB);
write_cmos_sensor(0X6F12, 0X9C02);
write_cmos_sensor(0X6F12, 0X2282);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6D80);
write_cmos_sensor(0X6F12, 0XB8F1);
write_cmos_sensor(0X6F12, 0X000F);
write_cmos_sensor(0X6F12, 0X02D0);
write_cmos_sensor(0X6F12, 0X09F1);
write_cmos_sensor(0X6F12, 0X0702);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X4A46);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X82FC);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6B20);
write_cmos_sensor(0X6F12, 0X4843);
write_cmos_sensor(0X6F12, 0XBCFB);
write_cmos_sensor(0X6F12, 0XF2FC);
write_cmos_sensor(0X6F12, 0XBC44);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X8CFC);
write_cmos_sensor(0X6F12, 0X2028);
write_cmos_sensor(0X6F12, 0X3ADC);
write_cmos_sensor(0X6F12, 0XB8F1);
write_cmos_sensor(0X6F12, 0X000F);
write_cmos_sensor(0X6F12, 0X03D0);
write_cmos_sensor(0X6F12, 0X0720);
write_cmos_sensor(0X6F12, 0XB0FB);
write_cmos_sensor(0X6F12, 0XF1F0);
write_cmos_sensor(0X6F12, 0X00E0);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X9749);
write_cmos_sensor(0X6F12, 0X6082);
write_cmos_sensor(0X6F12, 0X8888);
write_cmos_sensor(0X6F12, 0XA082);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X9200);
write_cmos_sensor(0X6F12, 0X3844);
write_cmos_sensor(0X6F12, 0XE082);
write_cmos_sensor(0X6F12, 0X3FB1);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6B20);
write_cmos_sensor(0X6F12, 0X0AEB);
write_cmos_sensor(0X6F12, 0X0700);
write_cmos_sensor(0X6F12, 0XBEFB);
write_cmos_sensor(0X6F12, 0XF2F2);
write_cmos_sensor(0X6F12, 0X8242);
write_cmos_sensor(0X6F12, 0X03D8);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6B20);
write_cmos_sensor(0X6F12, 0XBEFB);
write_cmos_sensor(0X6F12, 0XF2F0);
write_cmos_sensor(0X6F12, 0X2083);
write_cmos_sensor(0X6F12, 0X82B2);
write_cmos_sensor(0X6F12, 0X708A);
write_cmos_sensor(0X6F12, 0X08B1);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X9200);
write_cmos_sensor(0X6F12, 0X1044);
write_cmos_sensor(0X6F12, 0X2083);
write_cmos_sensor(0X6F12, 0XC888);
write_cmos_sensor(0X6F12, 0X6083);
write_cmos_sensor(0X6F12, 0X3088);
write_cmos_sensor(0X6F12, 0XA083);
write_cmos_sensor(0X6F12, 0XB5F8);
write_cmos_sensor(0X6F12, 0X9E20);
write_cmos_sensor(0X6F12, 0X3088);
write_cmos_sensor(0X6F12, 0X7189);
write_cmos_sensor(0X6F12, 0X1209);
write_cmos_sensor(0X6F12, 0X01FB);
write_cmos_sensor(0X6F12, 0X0200);
write_cmos_sensor(0X6F12, 0XE083);
write_cmos_sensor(0X6F12, 0X708A);
write_cmos_sensor(0X6F12, 0X40B3);
write_cmos_sensor(0X6F12, 0X608A);
write_cmos_sensor(0X6F12, 0X0828);
write_cmos_sensor(0X6F12, 0X16D0);
write_cmos_sensor(0X6F12, 0X0C28);
write_cmos_sensor(0X6F12, 0X14D0);
write_cmos_sensor(0X6F12, 0X0D28);
write_cmos_sensor(0X6F12, 0X12D0);
write_cmos_sensor(0X6F12, 0X12E0);
write_cmos_sensor(0X6F12, 0X02FB);
write_cmos_sensor(0X6F12, 0X0CF2);
write_cmos_sensor(0X6F12, 0X203A);
write_cmos_sensor(0X6F12, 0XD017);
write_cmos_sensor(0X6F12, 0X02EB);
write_cmos_sensor(0X6F12, 0X9060);
write_cmos_sensor(0X6F12, 0X20F0);
write_cmos_sensor(0X6F12, 0X3F00);
write_cmos_sensor(0X6F12, 0X101A);
write_cmos_sensor(0X6F12, 0XC217);
write_cmos_sensor(0X6F12, 0X00EB);
write_cmos_sensor(0X6F12, 0X1272);
write_cmos_sensor(0X6F12, 0X1211);
write_cmos_sensor(0X6F12, 0XA0EB);
write_cmos_sensor(0X6F12, 0X0210);
write_cmos_sensor(0X6F12, 0X90FB);
write_cmos_sensor(0X6F12, 0XF1F0);
write_cmos_sensor(0X6F12, 0XBAE7);
write_cmos_sensor(0X6F12, 0X13B1);
write_cmos_sensor(0X6F12, 0X0428);
write_cmos_sensor(0X6F12, 0X05D0);
write_cmos_sensor(0X6F12, 0X0AE0);
write_cmos_sensor(0X6F12, 0X801C);
write_cmos_sensor(0X6F12, 0X6082);
write_cmos_sensor(0X6F12, 0X0BF1);
write_cmos_sensor(0X6F12, 0X0200);
write_cmos_sensor(0X6F12, 0X04E0);
write_cmos_sensor(0X6F12, 0X23B1);
write_cmos_sensor(0X6F12, 0X0520);
write_cmos_sensor(0X6F12, 0X6082);
write_cmos_sensor(0X6F12, 0X0BF1);
write_cmos_sensor(0X6F12, 0X0100);
write_cmos_sensor(0X6F12, 0XE080);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X6D00);
write_cmos_sensor(0X6F12, 0X3A46);
write_cmos_sensor(0X6F12, 0X1946);
write_cmos_sensor(0X6F12, 0XFFF7);
write_cmos_sensor(0X6F12, 0XAEFE);
write_cmos_sensor(0X6F12, 0X6B49);
write_cmos_sensor(0X6F12, 0X0880);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XF09F);
write_cmos_sensor(0X6F12, 0X2DE9);
write_cmos_sensor(0X6F12, 0XF041);
write_cmos_sensor(0X6F12, 0X0446);
write_cmos_sensor(0X6F12, 0X6948);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X406A);
write_cmos_sensor(0X6F12, 0X87B2);
write_cmos_sensor(0X6F12, 0X4FEA);
write_cmos_sensor(0X6F12, 0X1048);
write_cmos_sensor(0X6F12, 0X3946);
write_cmos_sensor(0X6F12, 0X4046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XB4F9);
write_cmos_sensor(0X6F12, 0X2046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XD9F9);
write_cmos_sensor(0X6F12, 0X634E);
write_cmos_sensor(0X6F12, 0X04F1);
write_cmos_sensor(0X6F12, 0X5805);
write_cmos_sensor(0X6F12, 0XB6F8);
write_cmos_sensor(0X6F12, 0XB400);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XD7F9);
write_cmos_sensor(0X6F12, 0X80B2);
write_cmos_sensor(0X6F12, 0XE880);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0XB810);
write_cmos_sensor(0X6F12, 0X0129);
write_cmos_sensor(0X6F12, 0X03D0);
write_cmos_sensor(0X6F12, 0XB6F8);
write_cmos_sensor(0X6F12, 0XB600);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XCDF9);
write_cmos_sensor(0X6F12, 0XA881);
write_cmos_sensor(0X6F12, 0X96F8);
write_cmos_sensor(0X6F12, 0XB800);
write_cmos_sensor(0X6F12, 0X0128);
write_cmos_sensor(0X6F12, 0X0ED0);
write_cmos_sensor(0X6F12, 0X5948);
write_cmos_sensor(0X6F12, 0X0068);
write_cmos_sensor(0X6F12, 0XB0F8);
write_cmos_sensor(0X6F12, 0X4800);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XC2F9);
write_cmos_sensor(0X6F12, 0XA4F8);
write_cmos_sensor(0X6F12, 0X4800);
write_cmos_sensor(0X6F12, 0X3946);
write_cmos_sensor(0X6F12, 0X4046);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XF041);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X8CB9);
write_cmos_sensor(0X6F12, 0X34F8);
write_cmos_sensor(0X6F12, 0X420F);
write_cmos_sensor(0X6F12, 0XE080);
write_cmos_sensor(0X6F12, 0XF4E7);
write_cmos_sensor(0X6F12, 0X2DE9);
write_cmos_sensor(0X6F12, 0XF041);
write_cmos_sensor(0X6F12, 0X4A4C);
write_cmos_sensor(0X6F12, 0X4F4D);
write_cmos_sensor(0X6F12, 0X0F46);
write_cmos_sensor(0X6F12, 0X8046);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X4430);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0XFD20);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0X5C11);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X23B1);
write_cmos_sensor(0X6F12, 0X09B1);
write_cmos_sensor(0X6F12, 0X0220);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0X02B1);
write_cmos_sensor(0X6F12, 0X0120);
write_cmos_sensor(0X6F12, 0X484E);
write_cmos_sensor(0X6F12, 0XA6F8);
write_cmos_sensor(0X6F12, 0X4602);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X2030);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X23B1);
write_cmos_sensor(0X6F12, 0X09B1);
write_cmos_sensor(0X6F12, 0X0220);
write_cmos_sensor(0X6F12, 0X01E0);
write_cmos_sensor(0X6F12, 0X02B1);
write_cmos_sensor(0X6F12, 0X0120);
write_cmos_sensor(0X6F12, 0X70B1);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X0221);
write_cmos_sensor(0X6F12, 0X47F2);
write_cmos_sensor(0X6F12, 0XC600);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X62F9);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0XFD10);
write_cmos_sensor(0X6F12, 0X39B1);
write_cmos_sensor(0X6F12, 0X3E48);
write_cmos_sensor(0X6F12, 0X90F8);
write_cmos_sensor(0X6F12, 0X5D00);
write_cmos_sensor(0X6F12, 0X18B1);
write_cmos_sensor(0X6F12, 0X0120);
write_cmos_sensor(0X6F12, 0X02E0);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0XEFE7);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0XB120);
write_cmos_sensor(0X6F12, 0X5040);
write_cmos_sensor(0X6F12, 0X84F8);
write_cmos_sensor(0X6F12, 0X5C01);
write_cmos_sensor(0X6F12, 0X84F8);
write_cmos_sensor(0X6F12, 0X5D01);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X2020);
write_cmos_sensor(0X6F12, 0X022A);
write_cmos_sensor(0X6F12, 0X04D1);
write_cmos_sensor(0X6F12, 0X19B1);
write_cmos_sensor(0X6F12, 0X80EA);
write_cmos_sensor(0X6F12, 0X0102);
write_cmos_sensor(0X6F12, 0X84F8);
write_cmos_sensor(0X6F12, 0X5C21);
write_cmos_sensor(0X6F12, 0XA6F8);
write_cmos_sensor(0X6F12, 0X6A02);
write_cmos_sensor(0X6F12, 0X2948);
write_cmos_sensor(0X6F12, 0X9030);
write_cmos_sensor(0X6F12, 0X0646);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X71F9);
write_cmos_sensor(0X6F12, 0XC4F8);
write_cmos_sensor(0X6F12, 0X6001);
write_cmos_sensor(0X6F12, 0X3046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X71F9);
write_cmos_sensor(0X6F12, 0XC4F8);
write_cmos_sensor(0X6F12, 0X6401);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0X2020);
write_cmos_sensor(0X6F12, 0X2B4D);
write_cmos_sensor(0X6F12, 0X022A);
write_cmos_sensor(0X6F12, 0X3FD0);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0X5C01);
write_cmos_sensor(0X6F12, 0XE8B3);
write_cmos_sensor(0X6F12, 0X0021);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0XAB32);
write_cmos_sensor(0X6F12, 0X5940);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0XFC30);
write_cmos_sensor(0X6F12, 0X5840);
write_cmos_sensor(0X6F12, 0X14F8);
write_cmos_sensor(0X6F12, 0XEB3F);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0X3640);
write_cmos_sensor(0X6F12, 0X2344);
write_cmos_sensor(0X6F12, 0X9B07);
write_cmos_sensor(0X6F12, 0X00D0);
write_cmos_sensor(0X6F12, 0X0123);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0XAA42);
write_cmos_sensor(0X6F12, 0X6340);
write_cmos_sensor(0X6F12, 0X5840);
write_cmos_sensor(0X6F12, 0XA2B1);
write_cmos_sensor(0X6F12, 0X40EA);
write_cmos_sensor(0X6F12, 0X4100);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0XC213);
write_cmos_sensor(0X6F12, 0X40EA);
write_cmos_sensor(0X6F12, 0X8100);
write_cmos_sensor(0X6F12, 0X95F8);
write_cmos_sensor(0X6F12, 0XC313);
write_cmos_sensor(0X6F12, 0X40EA);
write_cmos_sensor(0X6F12, 0XC100);
write_cmos_sensor(0X6F12, 0X1A49);
write_cmos_sensor(0X6F12, 0XA1F8);
write_cmos_sensor(0X6F12, 0X4A03);
write_cmos_sensor(0X6F12, 0X8915);
write_cmos_sensor(0X6F12, 0X4AF2);
write_cmos_sensor(0X6F12, 0X4A30);
write_cmos_sensor(0X6F12, 0X022A);
write_cmos_sensor(0X6F12, 0X34D0);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X07F9);
write_cmos_sensor(0X6F12, 0X0D48);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X816A);
write_cmos_sensor(0X6F12, 0X0C0C);
write_cmos_sensor(0X6F12, 0X8DB2);
write_cmos_sensor(0X6F12, 0X2946);
write_cmos_sensor(0X6F12, 0X2046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XFEF8);
write_cmos_sensor(0X6F12, 0X3946);
write_cmos_sensor(0X6F12, 0X4046);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X36F9);
write_cmos_sensor(0X6F12, 0X2946);
write_cmos_sensor(0X6F12, 0X2046);
write_cmos_sensor(0X6F12, 0XBDE8);
write_cmos_sensor(0X6F12, 0XF041);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XF3B8);
write_cmos_sensor(0X6F12, 0X16E0);
write_cmos_sensor(0X6F12, 0X19E0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1EB0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X19E0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X4A00);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X3FF0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X0010);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X0520);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X2970);
write_cmos_sensor(0X6F12, 0X4000);
write_cmos_sensor(0X6F12, 0XF000);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X05C0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1350);
write_cmos_sensor(0X6F12, 0X4000);
write_cmos_sensor(0X6F12, 0XA000);
write_cmos_sensor(0X6F12, 0X94F8);
write_cmos_sensor(0X6F12, 0X5C11);
write_cmos_sensor(0X6F12, 0X0846);
write_cmos_sensor(0X6F12, 0XBAE7);
write_cmos_sensor(0X6F12, 0X0121);
write_cmos_sensor(0X6F12, 0XA5E7);
write_cmos_sensor(0X6F12, 0X0122);
write_cmos_sensor(0X6F12, 0XC9E7);
write_cmos_sensor(0X6F12, 0X70B5);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF6);
write_cmos_sensor(0X6F12, 0X4351);
write_cmos_sensor(0X6F12, 0X4648);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X0DF9);
write_cmos_sensor(0X6F12, 0X464D);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF6);
write_cmos_sensor(0X6F12, 0X0B51);
write_cmos_sensor(0X6F12, 0X2860);
write_cmos_sensor(0X6F12, 0X4448);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X05F9);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF6);
write_cmos_sensor(0X6F12, 0X3741);
write_cmos_sensor(0X6F12, 0X6860);
write_cmos_sensor(0X6F12, 0X4248);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XFEF8);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF6);
write_cmos_sensor(0X6F12, 0X1141);
write_cmos_sensor(0X6F12, 0X4048);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XF8F8);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF6);
write_cmos_sensor(0X6F12, 0X7B31);
write_cmos_sensor(0X6F12, 0XE860);
write_cmos_sensor(0X6F12, 0X3D48);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XF1F8);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF6);
write_cmos_sensor(0X6F12, 0X1531);
write_cmos_sensor(0X6F12, 0XA860);
write_cmos_sensor(0X6F12, 0X3B48);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XEAF8);
write_cmos_sensor(0X6F12, 0X3A4C);
write_cmos_sensor(0X6F12, 0X2861);
write_cmos_sensor(0X6F12, 0X44F6);
write_cmos_sensor(0X6F12, 0X9C60);
write_cmos_sensor(0X6F12, 0XE18C);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XE8F8);
write_cmos_sensor(0X6F12, 0XE08C);
write_cmos_sensor(0X6F12, 0X384E);
write_cmos_sensor(0X6F12, 0X3749);
write_cmos_sensor(0X6F12, 0X46F8);
write_cmos_sensor(0X6F12, 0X2010);
write_cmos_sensor(0X6F12, 0X401C);
write_cmos_sensor(0X6F12, 0X81B2);
write_cmos_sensor(0X6F12, 0XE184);
write_cmos_sensor(0X6F12, 0X43F2);
write_cmos_sensor(0X6F12, 0XAC00);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XDCF8);
write_cmos_sensor(0X6F12, 0XE08C);
write_cmos_sensor(0X6F12, 0X3349);
write_cmos_sensor(0X6F12, 0X46F8);
write_cmos_sensor(0X6F12, 0X2010);
write_cmos_sensor(0X6F12, 0X401C);
write_cmos_sensor(0X6F12, 0X81B2);
write_cmos_sensor(0X6F12, 0XE184);
write_cmos_sensor(0X6F12, 0X43F2);
write_cmos_sensor(0X6F12, 0XB000);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XD1F8);
write_cmos_sensor(0X6F12, 0XE08C);
write_cmos_sensor(0X6F12, 0X2F49);
write_cmos_sensor(0X6F12, 0X46F8);
write_cmos_sensor(0X6F12, 0X2010);
write_cmos_sensor(0X6F12, 0X401C);
write_cmos_sensor(0X6F12, 0X81B2);
write_cmos_sensor(0X6F12, 0XE184);
write_cmos_sensor(0X6F12, 0X43F2);
write_cmos_sensor(0X6F12, 0X1840);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XC6F8);
write_cmos_sensor(0X6F12, 0XE08C);
write_cmos_sensor(0X6F12, 0X2A49);
write_cmos_sensor(0X6F12, 0X46F8);
write_cmos_sensor(0X6F12, 0X2010);
write_cmos_sensor(0X6F12, 0X401C);
write_cmos_sensor(0X6F12, 0X81B2);
write_cmos_sensor(0X6F12, 0XE184);
write_cmos_sensor(0X6F12, 0X43F2);
write_cmos_sensor(0X6F12, 0X1C40);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XBBF8);
write_cmos_sensor(0X6F12, 0XE08C);
write_cmos_sensor(0X6F12, 0X2649);
write_cmos_sensor(0X6F12, 0X46F8);
write_cmos_sensor(0X6F12, 0X2010);
write_cmos_sensor(0X6F12, 0X401C);
write_cmos_sensor(0X6F12, 0XE084);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF6);
write_cmos_sensor(0X6F12, 0X5921);
write_cmos_sensor(0X6F12, 0X2348);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0XAAF8);
write_cmos_sensor(0X6F12, 0X224C);
write_cmos_sensor(0X6F12, 0X6861);
write_cmos_sensor(0X6F12, 0X4FF4);
write_cmos_sensor(0X6F12, 0X8050);
write_cmos_sensor(0X6F12, 0X24F8);
write_cmos_sensor(0X6F12, 0XCE0F);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0X6081);
write_cmos_sensor(0X6F12, 0XA082);
write_cmos_sensor(0X6F12, 0XAFF2);
write_cmos_sensor(0X6F12, 0XB151);
write_cmos_sensor(0X6F12, 0X1E48);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X9CF8);
write_cmos_sensor(0X6F12, 0XE861);
write_cmos_sensor(0X6F12, 0X0020);
write_cmos_sensor(0X6F12, 0X24F8);
write_cmos_sensor(0X6F12, 0XCE0C);
write_cmos_sensor(0X6F12, 0X0246);
write_cmos_sensor(0X6F12, 0XAFF2);
write_cmos_sensor(0X6F12, 0X4751);
write_cmos_sensor(0X6F12, 0X1A48);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X92F8);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF2);
write_cmos_sensor(0X6F12, 0XE321);
write_cmos_sensor(0X6F12, 0X2862);
write_cmos_sensor(0X6F12, 0X1748);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X8BF8);
write_cmos_sensor(0X6F12, 0X0022);
write_cmos_sensor(0X6F12, 0XAFF2);
write_cmos_sensor(0X6F12, 0X8121);
write_cmos_sensor(0X6F12, 0X6862);
write_cmos_sensor(0X6F12, 0X1548);
write_cmos_sensor(0X6F12, 0X00F0);
write_cmos_sensor(0X6F12, 0X84F8);
write_cmos_sensor(0X6F12, 0XA862);
write_cmos_sensor(0X6F12, 0X70BD);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X333B);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X3FF0);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X3757);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X7BED);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0XAC61);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0XA831);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X9E01);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X1E80);
write_cmos_sensor(0X6F12, 0X9E04);
write_cmos_sensor(0X6F12, 0X0BE0);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X2E10);
write_cmos_sensor(0X6F12, 0X18BF);
write_cmos_sensor(0X6F12, 0X06F2);
write_cmos_sensor(0X6F12, 0X0144);
write_cmos_sensor(0X6F12, 0X04F5);
write_cmos_sensor(0X6F12, 0X18BF);
write_cmos_sensor(0X6F12, 0X0AF2);
write_cmos_sensor(0X6F12, 0X0148);
write_cmos_sensor(0X6F12, 0X1FFA);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X79B3);
write_cmos_sensor(0X6F12, 0X2000);
write_cmos_sensor(0X6F12, 0X4A00);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X3C7B);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0A65);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X6979);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X79D9);
write_cmos_sensor(0X6F12, 0X43F2);
write_cmos_sensor(0X6F12, 0X1B3C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X43F2);
write_cmos_sensor(0X6F12, 0XC32C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X40F2);
write_cmos_sensor(0X6F12, 0XF76C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X43F2);
write_cmos_sensor(0X6F12, 0XBD6C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X43F2);
write_cmos_sensor(0X6F12, 0X476C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X40F2);
write_cmos_sensor(0X6F12, 0X777C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X4AF6);
write_cmos_sensor(0X6F12, 0X614C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X4AF6);
write_cmos_sensor(0X6F12, 0X310C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X47F6);
write_cmos_sensor(0X6F12, 0XB31C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X43F6);
write_cmos_sensor(0X6F12, 0XB53C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X40F2);
write_cmos_sensor(0X6F12, 0XA90C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X43F6);
write_cmos_sensor(0X6F12, 0X7B4C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X40F6);
write_cmos_sensor(0X6F12, 0X2D2C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X46F6);
write_cmos_sensor(0X6F12, 0X791C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X46F6);
write_cmos_sensor(0X6F12, 0X6D1C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X47F6);
write_cmos_sensor(0X6F12, 0XDB2C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X47F6);
write_cmos_sensor(0X6F12, 0XC72C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X47F6);
write_cmos_sensor(0X6F12, 0XD91C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X4DF2);
write_cmos_sensor(0X6F12, 0X536C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X4DF2);
write_cmos_sensor(0X6F12, 0XF35C);
write_cmos_sensor(0X6F12, 0XC0F2);
write_cmos_sensor(0X6F12, 0X000C);
write_cmos_sensor(0X6F12, 0X6047);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X30D3);
write_cmos_sensor(0X6F12, 0X01C0);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6F12, 0X0FFF);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X1930);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X602A, 0X1940);
write_cmos_sensor(0X6F12, 0X00AE);
write_cmos_sensor(0X602A, 0X194A);
write_cmos_sensor(0X6F12, 0X007A);
write_cmos_sensor(0X602A, 0X1954);
write_cmos_sensor(0X6F12, 0X0047);
write_cmos_sensor(0X602A, 0X195E);
write_cmos_sensor(0X6F12, 0X0066);
write_cmos_sensor(0X602A, 0X1976);
write_cmos_sensor(0X6F12, 0X00B8);
write_cmos_sensor(0X602A, 0X1980);
write_cmos_sensor(0X6F12, 0X003D);
write_cmos_sensor(0X602A, 0X198A);
write_cmos_sensor(0X6F12, 0X0014);
write_cmos_sensor(0X602A, 0X1994);
write_cmos_sensor(0X6F12, 0X001E);
write_cmos_sensor(0X602A, 0X19AC);
write_cmos_sensor(0X6F12, 0X0133);
write_cmos_sensor(0X602A, 0X19B6);
write_cmos_sensor(0X6F12, 0X0151);
write_cmos_sensor(0X602A, 0X19C0);
write_cmos_sensor(0X6F12, 0X0166);
write_cmos_sensor(0X602A, 0X19CA);
write_cmos_sensor(0X6F12, 0X0133);
write_cmos_sensor(0X602A, 0X4ACC);
write_cmos_sensor(0X6F12, 0X0172);
write_cmos_sensor(0X602A, 0X4AD0);
write_cmos_sensor(0X6F12, 0X0FEB);
write_cmos_sensor(0X6F12, 0X0FF3);
write_cmos_sensor(0X6F12, 0X1000);
write_cmos_sensor(0X6F12, 0X1000);
write_cmos_sensor(0X602A, 0X4ADA);
write_cmos_sensor(0X6F12, 0X0FEB);
write_cmos_sensor(0X6F12, 0X0FF3);
write_cmos_sensor(0X6F12, 0X0FF3);
write_cmos_sensor(0X6F12, 0X1000);
write_cmos_sensor(0X602A, 0X4AE4);
write_cmos_sensor(0X6F12, 0X0FD7);
write_cmos_sensor(0X6F12, 0X0FCA);
write_cmos_sensor(0X6F12, 0X0FAE);
write_cmos_sensor(0X6F12, 0X0FAE);
write_cmos_sensor(0X602A, 0X162C);
write_cmos_sensor(0X6F12, 0X8080);
write_cmos_sensor(0X602A, 0X164C);
write_cmos_sensor(0X6F12, 0X5555);
write_cmos_sensor(0X602A, 0X164E);
write_cmos_sensor(0X6F12, 0X5500);
write_cmos_sensor(0X602A, 0X166C);
write_cmos_sensor(0X6F12, 0X4040);
write_cmos_sensor(0X602A, 0X166E);
write_cmos_sensor(0X6F12, 0X4040);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X3D6C, 0X0080);
write_cmos_sensor(0X3D64, 0X0105);
write_cmos_sensor(0X3D66, 0X0105);
write_cmos_sensor(0X3D6A, 0X0001);
write_cmos_sensor(0X3D70, 0X0002);
write_cmos_sensor(0XF496, 0X0048);
write_cmos_sensor(0XF470, 0X0008);
write_cmos_sensor(0XF43A, 0X0015);
write_cmos_sensor(0X3676, 0X0008);
write_cmos_sensor(0X3678, 0X0008);
write_cmos_sensor(0X3AC8, 0X0A04);
write_cmos_sensor(0X3D34, 0X0010);
write_cmos_sensor(0X30A0, 0X0008);
write_cmos_sensor(0X0112, 0X0A0A);
write_cmos_sensor(0X322E, 0X000C);
write_cmos_sensor(0X3230, 0X000C);
write_cmos_sensor(0X3236, 0X000B);
write_cmos_sensor(0X3238, 0X000B);
write_cmos_sensor(0X32A6, 0X000C);
write_cmos_sensor(0X32A8, 0X000C);
write_cmos_sensor(0X3616, 0X0707);
write_cmos_sensor(0X3622, 0X0808);
write_cmos_sensor(0X3626, 0X0808);
write_cmos_sensor(0X32EE, 0X0001);
write_cmos_sensor(0X32F0, 0X0001);
write_cmos_sensor(0X32F6, 0X0003);
write_cmos_sensor(0X32F8, 0X0003);
write_cmos_sensor(0X361E, 0X3030);
write_cmos_sensor(0X362A, 0X0303);
write_cmos_sensor(0X3664, 0X0019);
write_cmos_sensor(0X3666, 0X030B);
write_cmos_sensor(0X3670, 0X0001);
write_cmos_sensor(0X31B6, 0X0008);
write_cmos_sensor(0XF442, 0X44C6);
write_cmos_sensor(0XF408, 0XFFF7);
write_cmos_sensor(0XF47C, 0X001F);
write_cmos_sensor(0X347E, 0X00CE);
write_cmos_sensor(0X3480, 0X00CE);
write_cmos_sensor(0X3482, 0X00E6);
write_cmos_sensor(0X3484, 0X00E6);
write_cmos_sensor(0X3486, 0X00FE);
write_cmos_sensor(0X3488, 0X00FE);
write_cmos_sensor(0X348A, 0X0116);
write_cmos_sensor(0X348C, 0X0116);
write_cmos_sensor(0XF636, 0X00D6);
write_cmos_sensor(0XF638, 0X00DE);
write_cmos_sensor(0XF63A, 0X00EE);
write_cmos_sensor(0XF63C, 0X00F6);
write_cmos_sensor(0XF63E, 0X0106);
write_cmos_sensor(0XF640, 0X010E);
write_cmos_sensor(0XF4D0, 0X0034);
write_cmos_sensor(0XF4D8, 0X0034);
write_cmos_sensor(0x0100, 0x0000);

}	/*	sensor_init  */


static void preview_setting(void)
{
  LOG_INF("E\n");
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0008);
write_cmos_sensor(0X0346, 0X0008);
write_cmos_sensor(0X0348, 0X1077);
write_cmos_sensor(0X034A, 0X0C37);
write_cmos_sensor(0X034C, 0X0838);
write_cmos_sensor(0X034E, 0X0618);
write_cmos_sensor(0X0340, 0X0699);
write_cmos_sensor(0X0342, 0X24C0);
write_cmos_sensor(0X3000, 0X0001);
write_cmos_sensor(0X0900, 0X0122);
write_cmos_sensor(0X0380, 0X0001);
write_cmos_sensor(0X0382, 0X0003);
write_cmos_sensor(0X0384, 0X0001);
write_cmos_sensor(0X0386, 0X0003);
write_cmos_sensor(0X0400, 0X0000);
write_cmos_sensor(0X0404, 0X0010);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X157C);
write_cmos_sensor(0X6F12, 0X0100);
write_cmos_sensor(0X602A, 0X15F0);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B00, 0X0080);
write_cmos_sensor(0X3070, 0X0100);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X1898);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B04, 0X0101);
write_cmos_sensor(0X0B08, 0X0000);
write_cmos_sensor(0X0B0E, 0X0000);
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0001);
write_cmos_sensor(0X3D06, 0X0010);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X4AEC);
write_cmos_sensor(0X6F12, 0X0105);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0208, 0X0100);
write_cmos_sensor(0X3604, 0X0000);
write_cmos_sensor(0X3606, 0X0104);
write_cmos_sensor(0X3002, 0X0001);
write_cmos_sensor(0X0136, 0X1800);
write_cmos_sensor(0X0300, 0X0004);
write_cmos_sensor(0X0302, 0X0001);
write_cmos_sensor(0X0304, 0X0006);
write_cmos_sensor(0X0306, 0X0078);
write_cmos_sensor(0X0308, 0X0008);
write_cmos_sensor(0X030A, 0X0001);
write_cmos_sensor(0X030C, 0X0004);
write_cmos_sensor(0X030E, 0X0032);
write_cmos_sensor(0X0200, 0X0618);
write_cmos_sensor(0X0202, 0X059D);
write_cmos_sensor(0X021E, 0X0400);
write_cmos_sensor(0X021C, 0X0000);
write_cmos_sensor(0X0204, 0X0020);
write_cmos_sensor(0X0206, 0X0020);
write_cmos_sensor(0X0114, 0X0300);
write_cmos_sensor(0X0110, 0X1002);
write_cmos_sensor(0X0216, 0X0000);
write_cmos_sensor(0XF440, 0X006F);
write_cmos_sensor(0XF494, 0X0020);
write_cmos_sensor(0XF4CC, 0X0029);
write_cmos_sensor(0XF4CE, 0X002C);
write_cmos_sensor(0XF4D2, 0X0035);
write_cmos_sensor(0XF4D4, 0X0038);
write_cmos_sensor(0XF4D6, 0X0039);
write_cmos_sensor(0XF4DA, 0X0035);
write_cmos_sensor(0XF4DC, 0X0038);
write_cmos_sensor(0XF4DE, 0X0039);
write_cmos_sensor(0X0100, 0X0100);

}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	if (currefps == 300) {
  write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0008);
write_cmos_sensor(0X0346, 0X0008);
write_cmos_sensor(0X0348, 0X1077);
write_cmos_sensor(0X034A, 0X0C37);
write_cmos_sensor(0X034C, 0X1070);
write_cmos_sensor(0X034E, 0X0C30);
write_cmos_sensor(0X0340, 0X0D2F);
write_cmos_sensor(0X0342, 0X1260);
write_cmos_sensor(0X3000, 0X0001);
write_cmos_sensor(0X0900, 0X0011);
write_cmos_sensor(0X0380, 0X0001);
write_cmos_sensor(0X0382, 0X0001);
write_cmos_sensor(0X0384, 0X0001);
write_cmos_sensor(0X0386, 0X0001);
write_cmos_sensor(0X0400, 0X0000);
write_cmos_sensor(0X0404, 0X0010);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X157C);
write_cmos_sensor(0X6F12, 0X0100);
write_cmos_sensor(0X602A, 0X15F0);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B00, 0X0080);
write_cmos_sensor(0X3070, 0X0100);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X1898);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B04, 0X0101);
write_cmos_sensor(0X0B08, 0X0000);
write_cmos_sensor(0X0B0E, 0X0000);
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0001);
write_cmos_sensor(0X3D06, 0X0010);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X4AEC);
write_cmos_sensor(0X6F12, 0X0105);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0208, 0X0100);
write_cmos_sensor(0X3604, 0X0000);
write_cmos_sensor(0X3606, 0X0104);
write_cmos_sensor(0X3002, 0X0001);
write_cmos_sensor(0X0136, 0X1800);
write_cmos_sensor(0X0300, 0X0004);
write_cmos_sensor(0X0302, 0X0001);
write_cmos_sensor(0X0304, 0X0006);
write_cmos_sensor(0X0306, 0X0078);
write_cmos_sensor(0X0308, 0X0008);
write_cmos_sensor(0X030A, 0X0001);
write_cmos_sensor(0X030C, 0X0004);
write_cmos_sensor(0X030E, 0X0062);
write_cmos_sensor(0X0200, 0X0618);
write_cmos_sensor(0X0202, 0X0D27);
write_cmos_sensor(0X021E, 0X0400);
write_cmos_sensor(0X021C, 0X0000);
write_cmos_sensor(0X0204, 0X0020);
write_cmos_sensor(0X0206, 0X0020);
write_cmos_sensor(0X0114, 0X0300);
write_cmos_sensor(0X0110, 0X1002);
write_cmos_sensor(0X0216, 0X0000);
write_cmos_sensor(0XF440, 0X002F);
write_cmos_sensor(0XF494, 0X0030);
write_cmos_sensor(0XF4CC, 0X0029);
write_cmos_sensor(0XF4CE, 0X002C);
write_cmos_sensor(0XF4D2, 0X0035);
write_cmos_sensor(0XF4D4, 0X0038);
write_cmos_sensor(0XF4D6, 0X0039);
write_cmos_sensor(0XF4DA, 0X0035);
write_cmos_sensor(0XF4DC, 0X0038);
write_cmos_sensor(0XF4DE, 0X0039);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0x0100, 0x0100);

	}
	else if (currefps == 240) {
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0008);
write_cmos_sensor(0X0346, 0X0008);
write_cmos_sensor(0X0348, 0X1077);
write_cmos_sensor(0X034A, 0X0C37);
write_cmos_sensor(0X034C, 0X1070);
write_cmos_sensor(0X034E, 0X0C30);
write_cmos_sensor(0X0340, 0X0D2F);
write_cmos_sensor(0X0342, 0X16F8);
write_cmos_sensor(0X3000, 0X0001);
write_cmos_sensor(0X0900, 0X0011);
write_cmos_sensor(0X0380, 0X0001);
write_cmos_sensor(0X0382, 0X0001);
write_cmos_sensor(0X0384, 0X0001);
write_cmos_sensor(0X0386, 0X0001);
write_cmos_sensor(0X0400, 0X0000);
write_cmos_sensor(0X0404, 0X0010);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X157C);
write_cmos_sensor(0X6F12, 0X0100);
write_cmos_sensor(0X602A, 0X15F0);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B00, 0X0080);
write_cmos_sensor(0X3070, 0X0100);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X1898);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B04, 0X0101);
write_cmos_sensor(0X0B08, 0X0000);
write_cmos_sensor(0X0B0E, 0X0000);
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0001);
write_cmos_sensor(0X3D06, 0X0010);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X4AEC);
write_cmos_sensor(0X6F12, 0X0105);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0208, 0X0100);
write_cmos_sensor(0X3604, 0X0000);
write_cmos_sensor(0X3606, 0X0104);
write_cmos_sensor(0X3002, 0X0001);
write_cmos_sensor(0X0136, 0X1800);
write_cmos_sensor(0X0300, 0X0004);
write_cmos_sensor(0X0302, 0X0001);
write_cmos_sensor(0X0304, 0X0006);
write_cmos_sensor(0X0306, 0X0078);
write_cmos_sensor(0X0308, 0X0008);
write_cmos_sensor(0X030A, 0X0001);
write_cmos_sensor(0X030C, 0X0004);
write_cmos_sensor(0X030E, 0X004C);
write_cmos_sensor(0X0200, 0X0618);
write_cmos_sensor(0X0202, 0X0D27);
write_cmos_sensor(0X021E, 0X0400);
write_cmos_sensor(0X021C, 0X0000);
write_cmos_sensor(0X0204, 0X0020);
write_cmos_sensor(0X0206, 0X0020);
write_cmos_sensor(0X0114, 0X0300);
write_cmos_sensor(0X0110, 0X1002);
write_cmos_sensor(0X0216, 0X0000);
write_cmos_sensor(0XF440, 0X002F);
write_cmos_sensor(0XF494, 0X0030);
write_cmos_sensor(0XF4CC, 0X0029);
write_cmos_sensor(0XF4CE, 0X002C);
write_cmos_sensor(0XF4D2, 0X0035);
write_cmos_sensor(0XF4D4, 0X0038);
write_cmos_sensor(0XF4D6, 0X0039);
write_cmos_sensor(0XF4DA, 0X0035);
write_cmos_sensor(0XF4DC, 0X0038);
write_cmos_sensor(0XF4DE, 0X0039);
write_cmos_sensor(0X6028, 0X4000);		
write_cmos_sensor(0x0100, 0x0100);

	}
	else if (currefps == 150) {
//PIP 15fps settings,相比Full 30fps
//    -VT : 560-> 400M
//    -Frame length: 3206-> 4589
//   -Linelength: 5808不變
//
//$MV1[MCLK:24,Width:4208,Height:3120,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:1124,pvi_pclk_inverse:0]
		LOG_INF("else if (currefps == 150)\n");
write_cmos_sensor(0x0100, 0x0000);
write_cmos_sensor(0x6028, 0x2000);
write_cmos_sensor(0x602A, 0x0F74);
write_cmos_sensor(0x6F12, 0x0040);	 // 64
write_cmos_sensor(0x6F12, 0x0040);	 // 64
write_cmos_sensor(0x6028, 0x4000);
write_cmos_sensor(0x0344, 0x0008);	 // 8
write_cmos_sensor(0x0346, 0x0008);	 // 8
write_cmos_sensor(0x0348, 0x1077);	 // 4215
write_cmos_sensor(0x034A, 0x0C37);	 // 3127
write_cmos_sensor(0x034C, 0x1070);	 // 4208
write_cmos_sensor(0x034E, 0x0C30);	 // 3120
write_cmos_sensor(0x0900, 0x0011);
write_cmos_sensor(0x0380, 0x0001);
write_cmos_sensor(0x0382, 0x0001);
write_cmos_sensor(0x0384, 0x0001);
write_cmos_sensor(0x0386, 0x0001);
write_cmos_sensor(0x0400, 0x0000);
write_cmos_sensor(0x0404, 0x0010);
write_cmos_sensor(0x0114, 0x0300);
write_cmos_sensor(0x0110, 0x0002);
write_cmos_sensor(0x0136, 0x1800);	 // 24MHz
write_cmos_sensor(0x0304, 0x0006);	 // 6
write_cmos_sensor(0x0306, 0x007D);	 // 125
write_cmos_sensor(0x0302, 0x0001);	 // 1
write_cmos_sensor(0x0300, 0x0005);	 // 5
write_cmos_sensor(0x030C, 0x0006);	 // 6
write_cmos_sensor(0x030E, 0x00c8);	 // 281
write_cmos_sensor(0x030A, 0x0001);	 // 1
write_cmos_sensor(0x0308, 0x0008);	 // 8
  #ifdef NONCONTINUEMODE
  write_cmos_sensor(0x0342, 0x1720);
  #else
  write_cmos_sensor(0x0342, 0x16B0);
  #endif
write_cmos_sensor(0x0340, 0x11ED);	 // 4589
write_cmos_sensor(0x0202, 0x0200);	 // 512
write_cmos_sensor(0x0200, 0x00C6);	 // 198
write_cmos_sensor(0x0B04, 0x0101);	//M.BPC_On
write_cmos_sensor(0x0B08, 0x0000);	//D.BPC_Off
write_cmos_sensor(0x0B00, 0x0007);	//LSC_Off
write_cmos_sensor(0x316A, 0x00A0);	// OUTIF threshold
write_cmos_sensor(0x0100, 0x0100);
	}
	else { //default fps =15
//PIP 15fps settings,相比Full 30fps
//    -VT : 560-> 400M
//    -Frame length: 3206-> 4589
//   -Linelength: 5808不變
//
//$MV1[MCLK:24,Width:4208,Height:3120,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:1124,pvi_pclk_inverse:0]
		LOG_INF("else  150fps\n");
write_cmos_sensor_byte(0x0100, 0x00);
write_cmos_sensor(0x6028, 0x2000);
write_cmos_sensor(0x602A, 0x0F74);
write_cmos_sensor(0x6F12, 0x0040);	 // 64
write_cmos_sensor(0x6F12, 0x0040);	 // 64
write_cmos_sensor(0x6028, 0x4000);
write_cmos_sensor(0x0344, 0x0008);	 // 8
write_cmos_sensor(0x0346, 0x0008);	 // 8
write_cmos_sensor(0x0348, 0x1077);	 // 4215
write_cmos_sensor(0x034A, 0x0C37);	 // 3127
write_cmos_sensor(0x034C, 0x1070);	 // 4208
write_cmos_sensor(0x034E, 0x0C30);	 // 3120
write_cmos_sensor(0x0900, 0x0011);
write_cmos_sensor(0x0380, 0x0001);
write_cmos_sensor(0x0382, 0x0001);
write_cmos_sensor(0x0384, 0x0001);
write_cmos_sensor(0x0386, 0x0001);
write_cmos_sensor(0x0400, 0x0000);
write_cmos_sensor(0x0404, 0x0010);
write_cmos_sensor(0x0114, 0x0300);
write_cmos_sensor(0x0110, 0x0002);
write_cmos_sensor(0x0136, 0x1800);	 // 24MHz
write_cmos_sensor(0x0304, 0x0006);	 // 6
write_cmos_sensor(0x0306, 0x007D);	 // 125
write_cmos_sensor(0x0302, 0x0001);	 // 1
write_cmos_sensor(0x0300, 0x0005);	 // 5
write_cmos_sensor(0x030C, 0x0006);	 // 6
write_cmos_sensor(0x030E, 0x00c8);	 // 281
write_cmos_sensor(0x030A, 0x0001);	 // 1
write_cmos_sensor(0x0308, 0x0008);	 // 8
  #ifdef NONCONTINUEMODE
  write_cmos_sensor(0x0342, 0x1720);
  #else
  write_cmos_sensor(0x0342, 0x16B0);
  #endif
write_cmos_sensor(0x0340, 0x11ED);	 // 4589
write_cmos_sensor(0x0202, 0x0200);	 // 512
write_cmos_sensor(0x0200, 0x00C6);	 // 198
write_cmos_sensor(0x0B04, 0x0101);	//M.BPC_On
write_cmos_sensor(0x0B08, 0x0000);	//D.BPC_Off
write_cmos_sensor(0x0B00, 0x0007);	//LSC_Off
write_cmos_sensor(0x316A, 0x00A0);	// OUTIF threshold
write_cmos_sensor_byte(0x0100, 0x01);
	}
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
  capture_setting(currefps);
}

static void hs_video_setting(void)
{
	LOG_INF("E\n");
  write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0018);
write_cmos_sensor(0X0346, 0X0182);
write_cmos_sensor(0X0348, 0X1067);
write_cmos_sensor(0X034A, 0X0ABD);
write_cmos_sensor(0X034C, 0X0570);
write_cmos_sensor(0X034E, 0X0314);
write_cmos_sensor(0X0340, 0X0380);
write_cmos_sensor(0X0342, 0X1168);
write_cmos_sensor(0X3000, 0X0000);
write_cmos_sensor(0X0900, 0X0113);
write_cmos_sensor(0X0380, 0X0001);
write_cmos_sensor(0X0382, 0X0001);
write_cmos_sensor(0X0384, 0X0001);
write_cmos_sensor(0X0386, 0X0005);
write_cmos_sensor(0X0400, 0X0001);
write_cmos_sensor(0X0404, 0X0030);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X157C);
write_cmos_sensor(0X6F12, 0X0100);
write_cmos_sensor(0X602A, 0X15F0);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B00, 0X0080);
write_cmos_sensor(0X3070, 0X0100);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X1898);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B04, 0X0101);
write_cmos_sensor(0X0B08, 0X0000);
write_cmos_sensor(0X0B0E, 0X0000);
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0000);
write_cmos_sensor(0X3D06, 0X0010);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X4AEC);
write_cmos_sensor(0X6F12, 0X0105);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0208, 0X0100);
write_cmos_sensor(0X3604, 0X0001);
write_cmos_sensor(0X3606, 0X0104);
write_cmos_sensor(0X3002, 0X0001);
write_cmos_sensor(0X0136, 0X1800);
write_cmos_sensor(0X0300, 0X0004);
write_cmos_sensor(0X0302, 0X0001);
write_cmos_sensor(0X0304, 0X0006);
write_cmos_sensor(0X0306, 0X0078);
write_cmos_sensor(0X0308, 0X0008);
write_cmos_sensor(0X030A, 0X0001);
write_cmos_sensor(0X030C, 0X0004);
write_cmos_sensor(0X030E, 0X0032);
write_cmos_sensor(0X0200, 0X0618);
write_cmos_sensor(0X0202, 0X0369);
write_cmos_sensor(0X021E, 0X0400);
write_cmos_sensor(0X021C, 0X0000);
write_cmos_sensor(0X0204, 0X0020);
write_cmos_sensor(0X0206, 0X0020);
write_cmos_sensor(0X0114, 0X0300);
write_cmos_sensor(0X0110, 0X1002);
write_cmos_sensor(0X0216, 0X0000);
write_cmos_sensor(0XF440, 0X00AF);
write_cmos_sensor(0XF494, 0X0020);
write_cmos_sensor(0XF4CC, 0X0028);
write_cmos_sensor(0XF4CE, 0X0028);
write_cmos_sensor(0XF4D2, 0X0034);
write_cmos_sensor(0XF4D4, 0X0FFF);
write_cmos_sensor(0XF4D6, 0X0FFF);
write_cmos_sensor(0XF4DA, 0X0034);
write_cmos_sensor(0XF4DC, 0X0FFF);
write_cmos_sensor(0XF4DE, 0X0FFF);
  write_cmos_sensor(0x0100, 0x0100);

}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
  write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0018);
write_cmos_sensor(0X0346, 0X0182);
write_cmos_sensor(0X0348, 0X1067);
write_cmos_sensor(0X034A, 0X0ABD);
write_cmos_sensor(0X034C, 0X0570);
write_cmos_sensor(0X034E, 0X0314);
write_cmos_sensor(0X0340, 0X0E00);
write_cmos_sensor(0X0342, 0X1168);
write_cmos_sensor(0X3000, 0X0000);
write_cmos_sensor(0X0900, 0X0113);
write_cmos_sensor(0X0380, 0X0001);
write_cmos_sensor(0X0382, 0X0001);
write_cmos_sensor(0X0384, 0X0001);
write_cmos_sensor(0X0386, 0X0005);
write_cmos_sensor(0X0400, 0X0001);
write_cmos_sensor(0X0404, 0X0030);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X157C);
write_cmos_sensor(0X6F12, 0X0100);
write_cmos_sensor(0X602A, 0X15F0);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B00, 0X0080);
write_cmos_sensor(0X3070, 0X0100);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X1898);
write_cmos_sensor(0X6F12, 0X0101);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0B04, 0X0101);
write_cmos_sensor(0X0B08, 0X0000);
write_cmos_sensor(0X0B0E, 0X0000);
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0000);
write_cmos_sensor(0X3D06, 0X0010);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X4AEC);
write_cmos_sensor(0X6F12, 0X0105);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X0208, 0X0100);
write_cmos_sensor(0X3604, 0X0001);
write_cmos_sensor(0X3606, 0X0104);
write_cmos_sensor(0X3002, 0X0001);
write_cmos_sensor(0X0136, 0X1800);
write_cmos_sensor(0X0300, 0X0004);
write_cmos_sensor(0X0302, 0X0001);
write_cmos_sensor(0X0304, 0X0006);
write_cmos_sensor(0X0306, 0X0078);
write_cmos_sensor(0X0308, 0X0008);
write_cmos_sensor(0X030A, 0X0001);
write_cmos_sensor(0X030C, 0X0004);
write_cmos_sensor(0X030E, 0X0032);
write_cmos_sensor(0X0200, 0X0618);
write_cmos_sensor(0X0202, 0X0369);
write_cmos_sensor(0X021E, 0X0400);
write_cmos_sensor(0X021C, 0X0000);
write_cmos_sensor(0X0204, 0X0020);
write_cmos_sensor(0X0206, 0X0020);
write_cmos_sensor(0X0114, 0X0300);
write_cmos_sensor(0X0110, 0X1002);
write_cmos_sensor(0X0216, 0X0000);
write_cmos_sensor(0XF440, 0X00AF);
write_cmos_sensor(0XF494, 0X0020);
write_cmos_sensor(0XF4CC, 0X0028);
write_cmos_sensor(0XF4CE, 0X0028);
write_cmos_sensor(0XF4D2, 0X0034);
write_cmos_sensor(0XF4D4, 0X0FFF);
write_cmos_sensor(0XF4D6, 0X0FFF);
write_cmos_sensor(0XF4DA, 0X0034);
write_cmos_sensor(0XF4DC, 0X0FFF);
write_cmos_sensor(0XF4DE, 0X0FFF);
  write_cmos_sensor(0x0100, 0x0100);

}


static kal_uint32 return_sensor_id(void)
{
    return ((read_cmos_sensor_byte(0x0000) << 8) | read_cmos_sensor_byte(0x0001));
}

/*************************************************************************
* FUNCTION
*	get_imgsensor_id
*
* DESCRIPTION
*	This function get the sensor ID
*
* PARAMETERS
*	*sensorID : return the sensor ID
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    //kal_uint32 retry = 0xFFFFFFFF;
    kal_uint32 retry = 0x2;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if (*sensor_id == imgsensor_info.sensor_id) {
#ifdef CONFIG_MTK_CAM_CAL
		//read_imx135_otp_mtk_fmt();
#endif
				LOG_INF("i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id,*sensor_id,imgsensor_info.sensor_id);
                return ERROR_NONE;
            }

            if(retry > 0)
			    LOG_INF("Read sensor id fail, i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x., retry:%d\n", imgsensor.i2c_write_id,*sensor_id,imgsensor_info.sensor_id, retry);
            retry--;
        } while(retry > 0);
        i++;
        retry = 0x2;
    }
    if (*sensor_id != imgsensor_info.sensor_id) {
        // if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF
        *sensor_id = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
    //const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
    kal_uint32 sensor_id = 0;
	LOG_1;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            sensor_id = return_sensor_id();
            if (sensor_id == imgsensor_info.sensor_id) {
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
                break;
            }
            LOG_INF("Read sensor id fail, id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        if (sensor_id == imgsensor_info.sensor_id)
            break;
        retry = 2;
    }
    if (imgsensor_info.sensor_id != sensor_id)
        return ERROR_SENSOR_CONNECT_FAIL;

    /* initail sequence write in  */
    sensor_init();

    spin_lock(&imgsensor_drv_lock);

    imgsensor.autoflicker_en= KAL_FALSE;
    imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.dummy_pixel = 0;
    imgsensor.dummy_line = 0;
    imgsensor.ihdr_en = KAL_FALSE;
    imgsensor.test_pattern = KAL_FALSE;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
    spin_unlock(&imgsensor_drv_lock);

    return ERROR_NONE;
}   /*  open  */



/*************************************************************************
* FUNCTION
*	close
*
* DESCRIPTION
*
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/

	return ERROR_NONE;
}	/*	close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
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
	preview_setting();
	set_mirror_flip(IMAGE_NORMAL);
	mdelay(10);
	#ifdef FANPENGTAO
	int i=0;
	for(i=0; i<10; i++){
		LOG_INF("delay time = %d, the frame no = %d\n", i*10, read_cmos_sensor(0x0005));
		mdelay(10);
	}
	#endif
	return ERROR_NONE;
}	/*	preview   */

/*************************************************************************
* FUNCTION
*	capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
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
	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) {
		LOG_INF("capture30fps: use cap30FPS's setting: %d fps!\n",imgsensor.current_fps/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		LOG_INF("cap115fps: use cap1's setting: %d fps!\n",imgsensor.current_fps/10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else  { //PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		LOG_INF("Warning:=== current_fps %d fps is not support, so use cap1's setting\n",imgsensor.current_fps/10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);
	set_mirror_flip(IMAGE_NORMAL);
	mdelay(10);

#if 0
	if(imgsensor.test_pattern == KAL_TRUE)
	{
		//write_cmos_sensor(0x5002,0x00);
  }
#endif

	return ERROR_NONE;
}	/* capture() */

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
	normal_video_setting(imgsensor.current_fps);
	set_mirror_flip(IMAGE_NORMAL);


	return ERROR_NONE;
}	/*	normal_video   */

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
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	set_mirror_flip(IMAGE_NORMAL);

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	set_mirror_flip(IMAGE_NORMAL);
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
* Custom1
*
* DESCRIPTION
*   This function start the sensor Custom1.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
    imgsensor.pclk = imgsensor_info.custom1.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom1.linelength;
    imgsensor.frame_length = imgsensor_info.custom1.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom1   */

static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
    imgsensor.pclk = imgsensor_info.custom2.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom2.linelength;
    imgsensor.frame_length = imgsensor_info.custom2.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom2   */

static kal_uint32 Custom3(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
    imgsensor.pclk = imgsensor_info.custom3.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom3.linelength;
    imgsensor.frame_length = imgsensor_info.custom3.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom3   */

static kal_uint32 Custom4(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM4;
    imgsensor.pclk = imgsensor_info.custom4.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom4.linelength;
    imgsensor.frame_length = imgsensor_info.custom4.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom4.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom4   */
static kal_uint32 Custom5(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM5;
    imgsensor.pclk = imgsensor_info.custom5.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom5.linelength;
    imgsensor.frame_length = imgsensor_info.custom5.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom5.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom5   */
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
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;
    sensor_resolution->SensorCustom1Width  = imgsensor_info.custom1.grabwindow_width;
    sensor_resolution->SensorCustom1Height     = imgsensor_info.custom1.grabwindow_height;

    sensor_resolution->SensorCustom2Width  = imgsensor_info.custom2.grabwindow_width;
    sensor_resolution->SensorCustom2Height     = imgsensor_info.custom2.grabwindow_height;

    sensor_resolution->SensorCustom3Width  = imgsensor_info.custom3.grabwindow_width;
    sensor_resolution->SensorCustom3Height     = imgsensor_info.custom3.grabwindow_height;

    sensor_resolution->SensorCustom4Width  = imgsensor_info.custom4.grabwindow_width;
    sensor_resolution->SensorCustom4Height     = imgsensor_info.custom4.grabwindow_height;

    sensor_resolution->SensorCustom5Width  = imgsensor_info.custom5.grabwindow_width;
    sensor_resolution->SensorCustom5Height     = imgsensor_info.custom5.grabwindow_height;
	return ERROR_NONE;
}	/*	get_resolution	*/

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
    sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
    sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;
    sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame;
    sensor_info->Custom3DelayFrame = imgsensor_info.custom3_delay_frame;
    sensor_info->Custom4DelayFrame = imgsensor_info.custom4_delay_frame;
    sensor_info->Custom5DelayFrame = imgsensor_info.custom5_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

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
	sensor_info->SensorHightSampling = 0;	// 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;
	#ifdef FPTPDAFSUPPORT
	sensor_info->PDAF_Support = 1;
	#else
	sensor_info->PDAF_Support = 0;
	#endif

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
        case MSDK_SCENARIO_ID_CUSTOM1:
            sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            sensor_info->SensorGrabStartX = imgsensor_info.custom3.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.custom3.starty;
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            sensor_info->SensorGrabStartX = imgsensor_info.custom4.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.custom4.starty;
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;

            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            sensor_info->SensorGrabStartX = imgsensor_info.custom5.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.custom5.starty;
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;

            break;
		default:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}	/*	get_info  */


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
        case MSDK_SCENARIO_ID_CUSTOM1:
            Custom1(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            Custom2(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            Custom3(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            Custom4(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            Custom5(image_window, sensor_config_data); // Custom1
			break;
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
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
			set_dummy();
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
			set_dummy();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			if(framerate==300)
			{
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
			else
			{
			frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
			set_dummy();
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			set_dummy();
        case MSDK_SCENARIO_ID_CUSTOM1:
            frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            frame_length = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom2.framelength) ? (frame_length - imgsensor_info.custom2.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
           // set_dummy();
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            frame_length = imgsensor_info.custom3.pclk / framerate * 10 / imgsensor_info.custom3.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom3.framelength) ? (frame_length - imgsensor_info.custom3.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom3.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            frame_length = imgsensor_info.custom4.pclk / framerate * 10 / imgsensor_info.custom4.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom4.framelength) ? (frame_length - imgsensor_info.custom4.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom4.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            frame_length = imgsensor_info.custom5.pclk / framerate * 10 / imgsensor_info.custom5.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom5.framelength) ? (frame_length - imgsensor_info.custom5.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
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
        case MSDK_SCENARIO_ID_CUSTOM1:
            *framerate = imgsensor_info.custom1.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            *framerate = imgsensor_info.custom2.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            *framerate = imgsensor_info.custom3.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            *framerate = imgsensor_info.custom4.max_framerate;
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            *framerate = imgsensor_info.custom5.max_framerate;
            break;
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		 write_cmos_sensor(0x0600, 0x0003);
	} else {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor(0x0600, 0x0000);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
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

    SENSOR_WINSIZE_INFO_STRUCT *wininfo;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
	SET_PD_BLOCK_INFO_T *PDAFinfo;

    LOG_INF("feature_id = %d\n", feature_id);
    switch (feature_id) {
        case SENSOR_FEATURE_GET_PERIOD:
            *feature_return_para_16++ = imgsensor.line_length;
            *feature_return_para_16 = imgsensor.frame_length;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
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
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            set_test_pattern_mode((BOOL)*feature_data);
            break;
        case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
            *feature_return_para_32 = imgsensor_info.checksum_value;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_SET_FRAMERATE:
            LOG_INF("current fps :%d\n", (UINT32)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case SENSOR_FEATURE_SET_HDR:
            LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.ihdr_en = (BOOL)*feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case SENSOR_FEATURE_GET_CROP_INFO:
            LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data);

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
        case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
            LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            break;
        /******************** PDAF START >>> *********/
		case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
			LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n", *feature_data);
			//PDAF capacity enable or not, 2p8 only full size support PDAF
			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1; // video & capture use same setting
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				default:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
			}
			break;
		case SENSOR_FEATURE_GET_PDAF_INFO:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%llu\n", *feature_data);
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
		case SENSOR_FEATURE_GET_PDAF_DATA:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
			S5K3M3_read_eeprom((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
			break;
        /******************** PDAF END   <<< *********/
        default:
            break;
    }

    return ERROR_NONE;
}    /*    feature_control()  */


static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};


UINT32 S5K3M3_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	S5K3M3_MIPI_RAW_SensorInit	*/



