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
#define VCPDAF
/* Open VCPDAF_PRE when preview mode need PDAF VC */
//#define VCPDAF_PRE 

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

	.checksum_value = 0x30a07776,

	.pre = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4704,				//record different mode's linelength
		.framelength = 3401,			//record different mode's framelength
		.startx= 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2104,		//record different mode's width of grabwindow
		.grabwindow_height = 1560,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},

	.cap = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4704,				//record different mode's linelength
		.framelength = 3401,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},


	.cap1 = {							//capture for PIP 24fps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 5886,				//record different mode's linelength
		.framelength = 3401,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 240,
	},

#if 0
	.cap2 = {							//capture for PIP 15ps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 400000000,				//record different mode's pclk
		.linelength  = 9408,				//record different mode's linelength
		.framelength = 3401,			//record different mode's framelength
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
		.framelength = 3401,			//record different mode's framelength
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

// VC_Num, VC_PixelNum, ModeSelect, EXPO_Ratio, ODValue, RG_STATSMODE
// VC0_ID, VC0_DataType, VC0_SIZEH, VC0_SIZE, VC1_ID, VC1_DataType, VC1_SIZEH, VC1_SIZEV
// VC2_ID, VC2_DataType, VC2_SIZEH, VC2_SIZE, VC3_ID, VC3_DataType, VC3_SIZEH, VC3_SIZEV
static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
{/* Preview mode setting */
 {0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
  0x00, 0x2B, 0x0838, 0x0618, 0x01, 0x00, 0x0000, 0x0000,
  0x01, 0x30, 0x00A0, 0x0300, 0x03, 0x00, 0x0000, 0x0000},
  /* Video mode setting */
 {0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
  0x00, 0x2B, 0x1070, 0x0C30, 0x01, 0x00, 0x0000, 0x0000,
  0x01, 0x30, 0x00A0, 0x0300, 0x03, 0x00, 0x0000, 0x0000},
  /* Capture mode setting */
 {0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
  0x00, 0x2B, 0x1070, 0x0C30, 0x01, 0x00, 0x0000, 0x0000,
  0x01, 0x30, 0x00A0, 0x0300, 0x03, 0x00, 0x0000, 0x0000}};



/* Sensor output window information*/
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] =
{
 { 4208, 3120,	  0,  	0, 4208, 3120, 2104, 1560,   0,	0, 2104, 1560, 	 0, 0, 2104, 1560}, // Preview
 { 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // capture
 { 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // video
 { 4208, 3120,	16,  378, 4176, 2364, 1392,  788,   0,	0, 1392,  788, 	 0, 0, 1392,  788}, //hight speed video
 { 4208, 3120,	16,  378, 4176, 2364, 1392,  788,   0,	0, 1392,  788, 	 0, 0, 1392,  788},// slim video
 { 4208, 3120,	  0,  	0, 4208, 3120, 2104, 1560,   0,	0, 2104, 1560, 	 0, 0, 2104, 1560},// Custom1 (defaultuse preview)
 { 4208, 3120,	  0,  	0, 4208, 3120, 2104, 1560,   0,	0, 2104, 1560, 	 0, 0, 2104, 1560},// Custom2
 { 4208, 3120,	  0,  	0, 4208, 3120, 2104, 1560,   0,	0, 2104, 1560, 	 0, 0, 2104, 1560},// Custom3
 { 4208, 3120,	  0,  	0, 4208, 3120, 2104, 1560,   0,	0, 2104, 1560, 	 0, 0, 2104, 1560},// Custom4
 { 4208, 3120,	  0,  	0, 4208, 3120, 2104, 1560,   0,	0, 2104, 1560, 	 0, 0, 2104, 1560},// Custom5
};

 static SET_PD_BLOCK_INFO_T imgsensor_pd_info =
 //for 3m3
{
    .i4OffsetX = 88,
    .i4OffsetY = 24,
    .i4PitchX = 64,
    .i4PitchY = 64,
    .i4PairNum =16,
    .i4SubBlkW =16,
    .i4SubBlkH =16,
    .i4PosL = {{92,31},{144,31},{108,35},{128,35},{96,51},{140,51},{112,55},{124,55},{112,63},{124,63},{96,67},{140,67},{108,83},{128,83},{92,87},{144,87}},
    .i4PosR = {{92,35},{144,35},{108,39},{128,39},{96,47},{140,47},{112,51},{124,51},{112,67},{124,67},{96,71},{140,71},{108,79},{128,79},{92,83},{144,83}},

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
*	check_stremoff
*
* DESCRIPTION
*	waiting function until sensor streaming finish.
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
static void check_stremoff(void)
{
    unsigned int i=0,framecnt=0;
    for(i=0;i<100;i++)
    {
        framecnt = read_cmos_sensor_byte(0x0005);
        if(framecnt == 0xFF)
            return;
    }
	LOG_INF(" Stream Off Fail!\n");
    return;
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
write_cmos_sensor(0X6010, 0X0001);
mdelay(3);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);   
#ifdef VCPDAF     
write_cmos_sensor(0x6028,0x2000);
write_cmos_sensor(0x602A,0x303C);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0449);
write_cmos_sensor(0x6F12,0x0348);
write_cmos_sensor(0x6F12,0x0860);
write_cmos_sensor(0x6F12,0xCA8D);
write_cmos_sensor(0x6F12,0x101A);
write_cmos_sensor(0x6F12,0x8880);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xA6BE);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x401C);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1E80);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x70B5);
write_cmos_sensor(0x6F12,0xFF4C);
write_cmos_sensor(0x6F12,0x0E46);
write_cmos_sensor(0x6F12,0xA4F8);
write_cmos_sensor(0x6F12,0x0001);
write_cmos_sensor(0x6F12,0xFE4D);
write_cmos_sensor(0x6F12,0x15F8);
write_cmos_sensor(0x6F12,0x940F);
write_cmos_sensor(0x6F12,0xA4F8);
write_cmos_sensor(0x6F12,0x0601);
write_cmos_sensor(0x6F12,0x6878);
write_cmos_sensor(0x6F12,0xA4F8);
write_cmos_sensor(0x6F12,0x0801);
write_cmos_sensor(0x6F12,0x1046);
write_cmos_sensor(0x6F12,0x04F5);
write_cmos_sensor(0x6F12,0xE874);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x46FF);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x48FF);
write_cmos_sensor(0x6F12,0xB5F8);
write_cmos_sensor(0x6F12,0x9A01);
write_cmos_sensor(0x6F12,0x6086);
write_cmos_sensor(0x6F12,0xB5F8);
write_cmos_sensor(0x6F12,0x9E01);
write_cmos_sensor(0x6F12,0xE086);
write_cmos_sensor(0x6F12,0xB5F8);
write_cmos_sensor(0x6F12,0xA001);
write_cmos_sensor(0x6F12,0x2087);
write_cmos_sensor(0x6F12,0xB5F8);
write_cmos_sensor(0x6F12,0xA601);
write_cmos_sensor(0x6F12,0xE087);
write_cmos_sensor(0x6F12,0x70BD);
write_cmos_sensor(0x6F12,0x2DE9);
write_cmos_sensor(0x6F12,0xF05F);
write_cmos_sensor(0x6F12,0xEF4A);
write_cmos_sensor(0x6F12,0xF049);
write_cmos_sensor(0x6F12,0xED4C);
write_cmos_sensor(0x6F12,0x0646);
write_cmos_sensor(0x6F12,0x92F8);
write_cmos_sensor(0x6F12,0xEC30);
write_cmos_sensor(0x6F12,0x0F89);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0xBE00);
write_cmos_sensor(0x6F12,0x4FF4);
write_cmos_sensor(0x6F12,0x8079);
write_cmos_sensor(0x6F12,0x13B1);
write_cmos_sensor(0x6F12,0xA8B9);
write_cmos_sensor(0x6F12,0x4F46);
write_cmos_sensor(0x6F12,0x13E0);
write_cmos_sensor(0x6F12,0xEA4B);
write_cmos_sensor(0x6F12,0x0968);
write_cmos_sensor(0x6F12,0x92F8);
write_cmos_sensor(0x6F12,0xED20);
write_cmos_sensor(0x6F12,0x1B6D);
write_cmos_sensor(0x6F12,0xB1FB);
write_cmos_sensor(0x6F12,0xF3F1);
write_cmos_sensor(0x6F12,0x7943);
write_cmos_sensor(0x6F12,0xD140);
write_cmos_sensor(0x6F12,0x8BB2);
write_cmos_sensor(0x6F12,0x30B1);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x4FF4);
write_cmos_sensor(0x6F12,0x8051);
write_cmos_sensor(0x6F12,0x1846);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x1EFF);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x4846);
write_cmos_sensor(0x6F12,0x87B2);
write_cmos_sensor(0x6F12,0x3088);
write_cmos_sensor(0x6F12,0x18B1);
write_cmos_sensor(0x6F12,0x0121);
write_cmos_sensor(0x6F12,0xDF48);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x1AFF);
write_cmos_sensor(0x6F12,0x0021);
write_cmos_sensor(0x6F12,0xDE48);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x16FF);
write_cmos_sensor(0x6F12,0xDFF8);
write_cmos_sensor(0x6F12,0x78A3);
write_cmos_sensor(0x6F12,0x0025);
write_cmos_sensor(0x6F12,0xA046);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x010B);
write_cmos_sensor(0x6F12,0x3088);
write_cmos_sensor(0x6F12,0x70B1);
write_cmos_sensor(0x6F12,0x7088);
write_cmos_sensor(0x6F12,0x38B1);
write_cmos_sensor(0x6F12,0xF089);
write_cmos_sensor(0x6F12,0x06E0);
write_cmos_sensor(0x6F12,0x2A46);
write_cmos_sensor(0x6F12,0xD549);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x0BFF);
write_cmos_sensor(0x6F12,0x0446);
write_cmos_sensor(0x6F12,0x05E0);
write_cmos_sensor(0x6F12,0xB089);
write_cmos_sensor(0x6F12,0x0028);
write_cmos_sensor(0x6F12,0xF6D1);
write_cmos_sensor(0x6F12,0x3846);
write_cmos_sensor(0x6F12,0xF4E7);
write_cmos_sensor(0x6F12,0x4C46);
write_cmos_sensor(0x6F12,0x98F8);
write_cmos_sensor(0x6F12,0xBE00);
write_cmos_sensor(0x6F12,0x28B1);
write_cmos_sensor(0x6F12,0x2A46);
write_cmos_sensor(0x6F12,0xCF49);
write_cmos_sensor(0x6F12,0x3846);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xFBFE);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x4846);
write_cmos_sensor(0x6F12,0xAAF8);
write_cmos_sensor(0x6F12,0x5802);
write_cmos_sensor(0x6F12,0x4443);
write_cmos_sensor(0x6F12,0x98F8);
write_cmos_sensor(0x6F12,0x2B02);
write_cmos_sensor(0x6F12,0x98F8);
write_cmos_sensor(0x6F12,0x2922);
write_cmos_sensor(0x6F12,0xC440);
write_cmos_sensor(0x6F12,0x0244);
write_cmos_sensor(0x6F12,0x0BFA);
write_cmos_sensor(0x6F12,0x02F0);
write_cmos_sensor(0x6F12,0x421E);
write_cmos_sensor(0x6F12,0xA242);
write_cmos_sensor(0x6F12,0x01DA);
write_cmos_sensor(0x6F12,0x1346);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x2346);
write_cmos_sensor(0x6F12,0x002B);
write_cmos_sensor(0x6F12,0x01DA);
write_cmos_sensor(0x6F12,0x0024);
write_cmos_sensor(0x6F12,0x02E0);
write_cmos_sensor(0x6F12,0xA242);
write_cmos_sensor(0x6F12,0x00DA);
write_cmos_sensor(0x6F12,0x1446);
write_cmos_sensor(0x6F12,0x0AEB);
write_cmos_sensor(0x6F12,0x4500);
write_cmos_sensor(0x6F12,0x6D1C);
write_cmos_sensor(0x6F12,0xA0F8);
write_cmos_sensor(0x6F12,0x5A42);
write_cmos_sensor(0x6F12,0x042D);
write_cmos_sensor(0x6F12,0xC4DB);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xF09F);
write_cmos_sensor(0x6F12,0xB94A);
write_cmos_sensor(0x6F12,0x92F8);
write_cmos_sensor(0x6F12,0xB330);
write_cmos_sensor(0x6F12,0x53B1);
write_cmos_sensor(0x6F12,0x92F8);
write_cmos_sensor(0x6F12,0xFD20);
write_cmos_sensor(0x6F12,0x0420);
write_cmos_sensor(0x6F12,0x52B1);
write_cmos_sensor(0x6F12,0xB94A);
write_cmos_sensor(0x6F12,0x92F8);
write_cmos_sensor(0x6F12,0x3221);
write_cmos_sensor(0x6F12,0x012A);
write_cmos_sensor(0x6F12,0x05D1);
write_cmos_sensor(0x6F12,0x0220);
write_cmos_sensor(0x6F12,0x03E0);
write_cmos_sensor(0x6F12,0x5379);
write_cmos_sensor(0x6F12,0x2BB9);
write_cmos_sensor(0x6F12,0x20B1);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x8842);
write_cmos_sensor(0x6F12,0x00D8);
write_cmos_sensor(0x6F12,0x0846);
write_cmos_sensor(0x6F12,0x7047);
write_cmos_sensor(0x6F12,0x1078);
write_cmos_sensor(0x6F12,0xF9E7);
write_cmos_sensor(0x6F12,0xFEB5);
write_cmos_sensor(0x6F12,0x0646);
write_cmos_sensor(0x6F12,0xB148);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xC068);
write_cmos_sensor(0x6F12,0x84B2);
write_cmos_sensor(0x6F12,0x050C);
write_cmos_sensor(0x6F12,0x2146);
write_cmos_sensor(0x6F12,0x2846);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xB9FE);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xBBFE);
write_cmos_sensor(0x6F12,0xA64E);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x8DF8);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0xFD00);
write_cmos_sensor(0x6F12,0x8DF8);
write_cmos_sensor(0x6F12,0x0100);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0xB310);
write_cmos_sensor(0x6F12,0xA54F);
write_cmos_sensor(0x6F12,0x19B1);
write_cmos_sensor(0x6F12,0x97F8);
write_cmos_sensor(0x6F12,0x3221);
write_cmos_sensor(0x6F12,0x012A);
write_cmos_sensor(0x6F12,0x0FD0);
write_cmos_sensor(0x6F12,0x0023);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0x0292);
write_cmos_sensor(0x6F12,0x0192);
write_cmos_sensor(0x6F12,0x21B1);
write_cmos_sensor(0x6F12,0x18B1);
write_cmos_sensor(0x6F12,0x97F8);
write_cmos_sensor(0x6F12,0x3201);
write_cmos_sensor(0x6F12,0x0128);
write_cmos_sensor(0x6F12,0x07D0);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0x6801);
write_cmos_sensor(0x6F12,0x4208);
write_cmos_sensor(0x6F12,0x33B1);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x06E0);
write_cmos_sensor(0x6F12,0x0123);
write_cmos_sensor(0x6F12,0xEEE7);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0x6821);
write_cmos_sensor(0x6F12,0xF7E7);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0xA400);
write_cmos_sensor(0x6F12,0x4100);
write_cmos_sensor(0x6F12,0x002A);
write_cmos_sensor(0x6F12,0x02D0);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x0200);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x41EA);
write_cmos_sensor(0x6F12,0x4020);
write_cmos_sensor(0x6F12,0x9349);
write_cmos_sensor(0x6F12,0xA1F8);
write_cmos_sensor(0x6F12,0x5201);
write_cmos_sensor(0x6F12,0x07D0);
write_cmos_sensor(0x6F12,0x9248);
write_cmos_sensor(0x6F12,0x6B46);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x4810);
write_cmos_sensor(0x6F12,0x4FF2);
write_cmos_sensor(0x6F12,0x5010);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x83FE);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0x2146);
write_cmos_sensor(0x6F12,0x2846);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x74FE);
write_cmos_sensor(0x6F12,0xFEBD);
write_cmos_sensor(0x6F12,0x2DE9);
write_cmos_sensor(0x6F12,0xF047);
write_cmos_sensor(0x6F12,0x0546);
write_cmos_sensor(0x6F12,0x8848);
write_cmos_sensor(0x6F12,0x9046);
write_cmos_sensor(0x6F12,0x8946);
write_cmos_sensor(0x6F12,0x8068);
write_cmos_sensor(0x6F12,0x1C46);
write_cmos_sensor(0x6F12,0x86B2);
write_cmos_sensor(0x6F12,0x070C);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x3146);
write_cmos_sensor(0x6F12,0x3846);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x64FE);
write_cmos_sensor(0x6F12,0x2346);
write_cmos_sensor(0x6F12,0x4246);
write_cmos_sensor(0x6F12,0x4946);
write_cmos_sensor(0x6F12,0x2846);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x68FE);
write_cmos_sensor(0x6F12,0x7A48);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0xB310);
write_cmos_sensor(0x6F12,0xD1B1);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0xFD00);
write_cmos_sensor(0x6F12,0xB8B1);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0xA5F5);
write_cmos_sensor(0x6F12,0x7141);
write_cmos_sensor(0x6F12,0x05F1);
write_cmos_sensor(0x6F12,0x8044);
write_cmos_sensor(0x6F12,0x9039);
write_cmos_sensor(0x6F12,0x02D0);
write_cmos_sensor(0x6F12,0x4031);
write_cmos_sensor(0x6F12,0x02D0);
write_cmos_sensor(0x6F12,0x0DE0);
write_cmos_sensor(0x6F12,0xA081);
write_cmos_sensor(0x6F12,0x0BE0);
write_cmos_sensor(0x6F12,0xA081);
write_cmos_sensor(0x6F12,0x7448);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0x3201);
write_cmos_sensor(0x6F12,0x0128);
write_cmos_sensor(0x6F12,0x05D1);
write_cmos_sensor(0x6F12,0x0220);
write_cmos_sensor(0x6F12,0xA080);
write_cmos_sensor(0x6F12,0x2D20);
write_cmos_sensor(0x6F12,0x2081);
write_cmos_sensor(0x6F12,0x2E20);
write_cmos_sensor(0x6F12,0x6081);
write_cmos_sensor(0x6F12,0x3146);
write_cmos_sensor(0x6F12,0x3846);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xF047);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x38BE);
write_cmos_sensor(0x6F12,0xF0B5);
write_cmos_sensor(0x6F12,0x30F8);
write_cmos_sensor(0x6F12,0x4A3F);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0xC488);
write_cmos_sensor(0x6F12,0x6746);
write_cmos_sensor(0x6F12,0x1B1B);
write_cmos_sensor(0x6F12,0x0489);
write_cmos_sensor(0x6F12,0x30F8);
write_cmos_sensor(0x6F12,0x0A0C);
write_cmos_sensor(0x6F12,0x1B1B);
write_cmos_sensor(0x6F12,0x9DB2);
write_cmos_sensor(0x6F12,0x32F8);
write_cmos_sensor(0x6F12,0x5A3F);
write_cmos_sensor(0x6F12,0x8C78);
write_cmos_sensor(0x6F12,0x117C);
write_cmos_sensor(0x6F12,0x92F8);
write_cmos_sensor(0x6F12,0x58E0);
write_cmos_sensor(0x6F12,0x04FB);
write_cmos_sensor(0x6F12,0x1133);
write_cmos_sensor(0x6F12,0x9BB2);
write_cmos_sensor(0x6F12,0x9489);
write_cmos_sensor(0x6F12,0xA0EB);
write_cmos_sensor(0x6F12,0x0E00);
write_cmos_sensor(0x6F12,0x04FB);
write_cmos_sensor(0x6F12,0x0134);
write_cmos_sensor(0x6F12,0x12F8);
write_cmos_sensor(0x6F12,0x3F6C);
write_cmos_sensor(0x6F12,0x80B2);
write_cmos_sensor(0x6F12,0x00FB);
write_cmos_sensor(0x6F12,0x013E);
write_cmos_sensor(0x6F12,0xA41B);
write_cmos_sensor(0x6F12,0xD27C);
write_cmos_sensor(0x6F12,0xA4B2);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x8EFE);
write_cmos_sensor(0x6F12,0x22B1);
write_cmos_sensor(0x6F12,0x3444);
write_cmos_sensor(0x6F12,0x00FB);
write_cmos_sensor(0x6F12,0x1144);
write_cmos_sensor(0x6F12,0x641E);
write_cmos_sensor(0x6F12,0x07E0);
write_cmos_sensor(0x6F12,0x20B1);
write_cmos_sensor(0x6F12,0xAEF1);
write_cmos_sensor(0x6F12,0x0104);
write_cmos_sensor(0x6F12,0x05FB);
write_cmos_sensor(0x6F12,0x0144);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0x641E);
write_cmos_sensor(0x6F12,0x3444);
write_cmos_sensor(0x6F12,0xA4B2);
write_cmos_sensor(0x6F12,0x2AB1);
write_cmos_sensor(0x6F12,0x10B1);
write_cmos_sensor(0x6F12,0x05FB);
write_cmos_sensor(0x6F12,0x1143);
write_cmos_sensor(0x6F12,0x5B1C);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x83FE);
write_cmos_sensor(0x6F12,0xBEF1);
write_cmos_sensor(0x6F12,0x700F);
write_cmos_sensor(0x6F12,0x02D2);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x010E);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0xAEF1);
write_cmos_sensor(0x6F12,0x6F0E);
write_cmos_sensor(0x6F12,0x4FF4);
write_cmos_sensor(0x6F12,0x4061);
write_cmos_sensor(0x6F12,0x6F3C);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x8EF0);
write_cmos_sensor(0x6F12,0x8C42);
write_cmos_sensor(0x6F12,0x00DD);
write_cmos_sensor(0x6F12,0x0C46);
write_cmos_sensor(0x6F12,0x211A);
write_cmos_sensor(0x6F12,0x491C);
write_cmos_sensor(0x6F12,0x8BB2);
write_cmos_sensor(0x6F12,0x401E);
write_cmos_sensor(0x6F12,0xC117);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x9161);
write_cmos_sensor(0x6F12,0x21F0);
write_cmos_sensor(0x6F12,0x3F01);
write_cmos_sensor(0x6F12,0x401A);
write_cmos_sensor(0x6F12,0xC117);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x1171);
write_cmos_sensor(0x6F12,0x0911);
write_cmos_sensor(0x6F12,0xA0EB);
write_cmos_sensor(0x6F12,0x0110);
write_cmos_sensor(0x6F12,0xC0F1);
write_cmos_sensor(0x6F12,0x1000);
write_cmos_sensor(0x6F12,0x81B2);
write_cmos_sensor(0x6F12,0x04F0);
write_cmos_sensor(0x6F12,0x0F00);
write_cmos_sensor(0x6F12,0xCC06);
write_cmos_sensor(0x6F12,0x00D5);
write_cmos_sensor(0x6F12,0x0021);
write_cmos_sensor(0x6F12,0xC406);
write_cmos_sensor(0x6F12,0x00D5);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x0C18);
write_cmos_sensor(0x6F12,0x1B1B);
write_cmos_sensor(0x6F12,0x9BB2);
write_cmos_sensor(0x6F12,0x89B1);
write_cmos_sensor(0x6F12,0xB2B1);
write_cmos_sensor(0x6F12,0x0829);
write_cmos_sensor(0x6F12,0x02D9);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x040C);
write_cmos_sensor(0x6F12,0x0BE0);
write_cmos_sensor(0x6F12,0x0529);
write_cmos_sensor(0x6F12,0x02D9);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x030C);
write_cmos_sensor(0x6F12,0x06E0);
write_cmos_sensor(0x6F12,0x0429);
write_cmos_sensor(0x6F12,0x02D9);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x020C);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x010C);
write_cmos_sensor(0x6F12,0xB0B1);
write_cmos_sensor(0x6F12,0x72B1);
write_cmos_sensor(0x6F12,0x0828);
write_cmos_sensor(0x6F12,0x06D2);
write_cmos_sensor(0x6F12,0x0027);
write_cmos_sensor(0x6F12,0x11E0);
write_cmos_sensor(0x6F12,0x0829);
write_cmos_sensor(0x6F12,0xE8D8);
write_cmos_sensor(0x6F12,0x0429);
write_cmos_sensor(0x6F12,0xF3D9);
write_cmos_sensor(0x6F12,0xEAE7);
write_cmos_sensor(0x6F12,0x0C28);
write_cmos_sensor(0x6F12,0x01D2);
write_cmos_sensor(0x6F12,0x0127);
write_cmos_sensor(0x6F12,0x08E0);
write_cmos_sensor(0x6F12,0x0327);
write_cmos_sensor(0x6F12,0x06E0);
write_cmos_sensor(0x6F12,0x0828);
write_cmos_sensor(0x6F12,0xF0D3);
write_cmos_sensor(0x6F12,0x0C28);
write_cmos_sensor(0x6F12,0xF7D3);
write_cmos_sensor(0x6F12,0x0D28);
write_cmos_sensor(0x6F12,0xF7D2);
write_cmos_sensor(0x6F12,0x0227);
write_cmos_sensor(0x6F12,0x9809);
write_cmos_sensor(0x6F12,0x0001);
write_cmos_sensor(0x6F12,0xC3F3);
write_cmos_sensor(0x6F12,0x0111);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x8100);
write_cmos_sensor(0x6F12,0x6044);
write_cmos_sensor(0x6F12,0x3844);
write_cmos_sensor(0x6F12,0x80B2);
write_cmos_sensor(0x6F12,0xF0BD);
write_cmos_sensor(0x6F12,0x2DE9);
write_cmos_sensor(0x6F12,0xFF41);
write_cmos_sensor(0x6F12,0x8046);
write_cmos_sensor(0x6F12,0x1B48);
write_cmos_sensor(0x6F12,0x1446);
write_cmos_sensor(0x6F12,0x0F46);
write_cmos_sensor(0x6F12,0x4069);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x85B2);
write_cmos_sensor(0x6F12,0x060C);
write_cmos_sensor(0x6F12,0x2946);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x8CFD);
write_cmos_sensor(0x6F12,0x2246);
write_cmos_sensor(0x6F12,0x3946);
write_cmos_sensor(0x6F12,0x4046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x96FD);
write_cmos_sensor(0x6F12,0x0C4A);
write_cmos_sensor(0x6F12,0x92F8);
write_cmos_sensor(0x6F12,0xCC00);
write_cmos_sensor(0x6F12,0x58B3);
write_cmos_sensor(0x6F12,0x97F8);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x40B3);
write_cmos_sensor(0x6F12,0x4FF4);
write_cmos_sensor(0x6F12,0x8050);
write_cmos_sensor(0x6F12,0x0090);
write_cmos_sensor(0x6F12,0x1148);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0xBA13);
write_cmos_sensor(0x6F12,0xB1FA);
write_cmos_sensor(0x6F12,0x81F0);
write_cmos_sensor(0x6F12,0xC0F1);
write_cmos_sensor(0x6F12,0x1700);
write_cmos_sensor(0x6F12,0xC140);
write_cmos_sensor(0x6F12,0xC9B2);
write_cmos_sensor(0x6F12,0x02EB);
write_cmos_sensor(0x6F12,0x4000);
write_cmos_sensor(0x6F12,0x1AE0);
write_cmos_sensor(0x6F12,0x4000);
write_cmos_sensor(0x6F12,0xE000);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1350);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x4A00);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x2970);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1EB0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1BAA);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1B58);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x2530);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1CC0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x3FF0);
write_cmos_sensor(0x6F12,0x4000);
write_cmos_sensor(0x6F12,0xF000);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x05C0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x2120);
write_cmos_sensor(0x6F12,0x26E0);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0xD030);
write_cmos_sensor(0x6F12,0x30F8);
write_cmos_sensor(0x6F12,0xCE2F);
write_cmos_sensor(0x6F12,0x9B1A);
write_cmos_sensor(0x6F12,0x4B43);
write_cmos_sensor(0x6F12,0x8033);
write_cmos_sensor(0x6F12,0x02EB);
write_cmos_sensor(0x6F12,0x2322);
write_cmos_sensor(0x6F12,0x0192);
write_cmos_sensor(0x6F12,0x8389);
write_cmos_sensor(0x6F12,0x4289);
write_cmos_sensor(0x6F12,0x9B1A);
write_cmos_sensor(0x6F12,0x4B43);
write_cmos_sensor(0x6F12,0x8033);
write_cmos_sensor(0x6F12,0x02EB);
write_cmos_sensor(0x6F12,0x2322);
write_cmos_sensor(0x6F12,0x0292);
write_cmos_sensor(0x6F12,0xC28A);
write_cmos_sensor(0x6F12,0x808A);
write_cmos_sensor(0x6F12,0x121A);
write_cmos_sensor(0x6F12,0x4A43);
write_cmos_sensor(0x6F12,0x8032);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x2220);
write_cmos_sensor(0x6F12,0x0023);
write_cmos_sensor(0x6F12,0x6946);
write_cmos_sensor(0x6F12,0x0390);
write_cmos_sensor(0x6F12,0x54F8);
write_cmos_sensor(0x6F12,0x2300);
write_cmos_sensor(0x6F12,0x51F8);
write_cmos_sensor(0x6F12,0x2320);
write_cmos_sensor(0x6F12,0x5043);
write_cmos_sensor(0x6F12,0x000B);
write_cmos_sensor(0x6F12,0x44F8);
write_cmos_sensor(0x6F12,0x2300);
write_cmos_sensor(0x6F12,0x5B1C);
write_cmos_sensor(0x6F12,0x042B);
write_cmos_sensor(0x6F12,0xF4D3);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0x2946);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x2AFD);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xFF81);
write_cmos_sensor(0x6F12,0x70B5);
write_cmos_sensor(0x6F12,0x0123);
write_cmos_sensor(0x6F12,0x11E0);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2350);
write_cmos_sensor(0x6F12,0x5A1E);
write_cmos_sensor(0x6F12,0x03E0);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x8206);
write_cmos_sensor(0x6F12,0x521E);
write_cmos_sensor(0x6F12,0x7460);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2240);
write_cmos_sensor(0x6F12,0xAC42);
write_cmos_sensor(0x6F12,0x01D9);
write_cmos_sensor(0x6F12,0x002A);
write_cmos_sensor(0x6F12,0xF5DA);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x8202);
write_cmos_sensor(0x6F12,0x5B1C);
write_cmos_sensor(0x6F12,0x5560);
write_cmos_sensor(0x6F12,0x8B42);
write_cmos_sensor(0x6F12,0xEBDB);
write_cmos_sensor(0x6F12,0x70BD);
write_cmos_sensor(0x6F12,0x70B5);
write_cmos_sensor(0x6F12,0x0123);
write_cmos_sensor(0x6F12,0x11E0);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2350);
write_cmos_sensor(0x6F12,0x5A1E);
write_cmos_sensor(0x6F12,0x03E0);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x8206);
write_cmos_sensor(0x6F12,0x521E);
write_cmos_sensor(0x6F12,0x7460);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2240);
write_cmos_sensor(0x6F12,0xAC42);
write_cmos_sensor(0x6F12,0x01D2);
write_cmos_sensor(0x6F12,0x002A);
write_cmos_sensor(0x6F12,0xF5DA);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x8202);
write_cmos_sensor(0x6F12,0x5B1C);
write_cmos_sensor(0x6F12,0x5560);
write_cmos_sensor(0x6F12,0x8B42);
write_cmos_sensor(0x6F12,0xEBDB);
write_cmos_sensor(0x6F12,0x70BD);
write_cmos_sensor(0x6F12,0x2DE9);
write_cmos_sensor(0x6F12,0xF34F);
write_cmos_sensor(0x6F12,0xF84F);
write_cmos_sensor(0x6F12,0x0646);
write_cmos_sensor(0x6F12,0x91B0);
write_cmos_sensor(0x6F12,0xB87F);
write_cmos_sensor(0x6F12,0x0028);
write_cmos_sensor(0x6F12,0x79D0);
write_cmos_sensor(0x6F12,0x307E);
write_cmos_sensor(0x6F12,0x717E);
write_cmos_sensor(0x6F12,0x129C);
write_cmos_sensor(0x6F12,0x0844);
write_cmos_sensor(0x6F12,0x0690);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0x6B10);
write_cmos_sensor(0x6F12,0xB07E);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xFAFC);
write_cmos_sensor(0x6F12,0x1090);
write_cmos_sensor(0x6F12,0xB07E);
write_cmos_sensor(0x6F12,0xF17E);
write_cmos_sensor(0x6F12,0x0844);
write_cmos_sensor(0x6F12,0x0590);
write_cmos_sensor(0x6F12,0x0090);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0x6D00);
write_cmos_sensor(0x6F12,0x08B1);
write_cmos_sensor(0x6F12,0x0125);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x0025);
write_cmos_sensor(0x6F12,0xB6F8);
write_cmos_sensor(0x6F12,0x5800);
write_cmos_sensor(0x6F12,0x0699);
write_cmos_sensor(0x6F12,0xC5F1);
write_cmos_sensor(0x6F12,0x0108);
write_cmos_sensor(0x6F12,0xB0FB);
write_cmos_sensor(0x6F12,0xF1F2);
write_cmos_sensor(0x6F12,0x01FB);
write_cmos_sensor(0x6F12,0x1201);
write_cmos_sensor(0x6F12,0x069A);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x000A);
write_cmos_sensor(0x6F12,0xB0FB);
write_cmos_sensor(0x6F12,0xF2F0);
write_cmos_sensor(0x6F12,0xB6F8);
write_cmos_sensor(0x6F12,0x8E20);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x5200);
write_cmos_sensor(0x6F12,0x0990);
write_cmos_sensor(0x6F12,0xE148);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x8000);
write_cmos_sensor(0x6F12,0x401A);
write_cmos_sensor(0x6F12,0x0F90);
write_cmos_sensor(0x6F12,0x45B1);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0xFF30);
write_cmos_sensor(0x6F12,0x0A90);
write_cmos_sensor(0x6F12,0xD7E9);
write_cmos_sensor(0x6F12,0x0419);
write_cmos_sensor(0x6F12,0x091F);
write_cmos_sensor(0x6F12,0x0B91);
write_cmos_sensor(0x6F12,0x0146);
write_cmos_sensor(0x6F12,0x07E0);
write_cmos_sensor(0x6F12,0x0120);
write_cmos_sensor(0x6F12,0x0A90);
write_cmos_sensor(0x6F12,0xD7E9);
write_cmos_sensor(0x6F12,0x0490);
write_cmos_sensor(0x6F12,0x001D);
write_cmos_sensor(0x6F12,0x0B90);
write_cmos_sensor(0x6F12,0x0099);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0x6C20);
write_cmos_sensor(0x6F12,0x12B1);
write_cmos_sensor(0x6F12,0xAFF2);
write_cmos_sensor(0x6F12,0xC902);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0xAFF2);
write_cmos_sensor(0x6F12,0xFD02);
write_cmos_sensor(0x6F12,0xD34B);
write_cmos_sensor(0x6F12,0x9A61);
write_cmos_sensor(0x6F12,0xD04B);
write_cmos_sensor(0x6F12,0x9A68);
write_cmos_sensor(0x6F12,0x0392);
write_cmos_sensor(0x6F12,0xDB68);
write_cmos_sensor(0x6F12,0x0493);
write_cmos_sensor(0x6F12,0xCDE9);
write_cmos_sensor(0x6F12,0x0123);
write_cmos_sensor(0x6F12,0x7288);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0x6A30);
write_cmos_sensor(0x6F12,0x368E);
write_cmos_sensor(0x6F12,0x03FB);
write_cmos_sensor(0x6F12,0x0622);
write_cmos_sensor(0x6F12,0x0892);
write_cmos_sensor(0x6F12,0xD0E0);
write_cmos_sensor(0x6F12,0x0A9E);
write_cmos_sensor(0x6F12,0xD9F8);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x09EB);
write_cmos_sensor(0x6F12,0x8609);
write_cmos_sensor(0x6F12,0x089E);
write_cmos_sensor(0x6F12,0x130C);
write_cmos_sensor(0x6F12,0xB342);
write_cmos_sensor(0x6F12,0xF5D3);
write_cmos_sensor(0x6F12,0x9E1B);
write_cmos_sensor(0x6F12,0x0F9B);
write_cmos_sensor(0x6F12,0x92B2);
write_cmos_sensor(0x6F12,0x069F);
write_cmos_sensor(0x6F12,0x1344);
write_cmos_sensor(0x6F12,0xB3FB);
write_cmos_sensor(0x6F12,0xF7F3);
write_cmos_sensor(0x6F12,0x099F);
write_cmos_sensor(0x6F12,0xBB42);
write_cmos_sensor(0x6F12,0x7DD3);
write_cmos_sensor(0x6F12,0xDB1B);
write_cmos_sensor(0x6F12,0x63F3);
write_cmos_sensor(0x6F12,0x5F02);
write_cmos_sensor(0x6F12,0x0E92);
write_cmos_sensor(0x6F12,0xB8F1);
write_cmos_sensor(0x6F12,0x000F);
write_cmos_sensor(0x6F12,0x01D0);
write_cmos_sensor(0x6F12,0x8E42);
write_cmos_sensor(0x6F12,0x02D2);
write_cmos_sensor(0x6F12,0x25B1);
write_cmos_sensor(0x6F12,0x8642);
write_cmos_sensor(0x6F12,0x02D2);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0x39E1);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x002A);
write_cmos_sensor(0x6F12,0x6CD0);
write_cmos_sensor(0x6F12,0x01A8);
write_cmos_sensor(0x6F12,0x4AEA);
write_cmos_sensor(0x6F12,0x0547);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2500);
write_cmos_sensor(0x6F12,0x0D90);
write_cmos_sensor(0x6F12,0x03A8);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x25B0);
write_cmos_sensor(0x6F12,0x0D98);
write_cmos_sensor(0x6F12,0xA0EB);
write_cmos_sensor(0x6F12,0x0B00);
write_cmos_sensor(0x6F12,0x8010);
write_cmos_sensor(0x6F12,0x0790);
write_cmos_sensor(0x6F12,0xB048);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0xFC00);
write_cmos_sensor(0x6F12,0x08B1);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x02E0);
write_cmos_sensor(0x6F12,0xAD48);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x2C01);
write_cmos_sensor(0x6F12,0x0C90);
write_cmos_sensor(0x6F12,0xAD48);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x4E15);
write_cmos_sensor(0x6F12,0x0798);
write_cmos_sensor(0x6F12,0x8142);
write_cmos_sensor(0x6F12,0x03D2);
write_cmos_sensor(0x6F12,0x390C);
write_cmos_sensor(0x6F12,0x3620);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x69FC);
write_cmos_sensor(0x6F12,0xA648);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0xFD00);
write_cmos_sensor(0x6F12,0x20B9);
write_cmos_sensor(0x6F12,0xA448);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x2801);
write_cmos_sensor(0x6F12,0x07EB);
write_cmos_sensor(0x6F12,0x0047);
write_cmos_sensor(0x6F12,0x0798);
write_cmos_sensor(0x6F12,0x0128);
write_cmos_sensor(0x6F12,0x07D0);
write_cmos_sensor(0x6F12,0x15D9);
write_cmos_sensor(0x6F12,0xA04A);
write_cmos_sensor(0x6F12,0x0146);
write_cmos_sensor(0x6F12,0x5846);
write_cmos_sensor(0x6F12,0x9269);
write_cmos_sensor(0x6F12,0x9047);
write_cmos_sensor(0x6F12,0x5846);
write_cmos_sensor(0x6F12,0x0BE0);
write_cmos_sensor(0x6F12,0xDBF8);
write_cmos_sensor(0x6F12,0x0010);
write_cmos_sensor(0x6F12,0x0C98);
write_cmos_sensor(0x6F12,0x0844);
write_cmos_sensor(0x6F12,0x3843);
write_cmos_sensor(0x6F12,0x01C4);
write_cmos_sensor(0x6F12,0x07E0);
write_cmos_sensor(0x6F12,0x04C8);
write_cmos_sensor(0x6F12,0x0C99);
write_cmos_sensor(0x6F12,0x1144);
write_cmos_sensor(0x6F12,0x3943);
write_cmos_sensor(0x6F12,0x02C4);
write_cmos_sensor(0x6F12,0x0D99);
write_cmos_sensor(0x6F12,0x8842);
write_cmos_sensor(0x6F12,0xF7D1);
write_cmos_sensor(0x6F12,0x01A8);
write_cmos_sensor(0x6F12,0x4AEA);
write_cmos_sensor(0x6F12,0x0847);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2800);
write_cmos_sensor(0x6F12,0x0C90);
write_cmos_sensor(0x6F12,0x03A8);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x28B0);
write_cmos_sensor(0x6F12,0x0C98);
write_cmos_sensor(0x6F12,0xA0EB);
write_cmos_sensor(0x6F12,0x0B00);
write_cmos_sensor(0x6F12,0x8010);
write_cmos_sensor(0x6F12,0x0790);
write_cmos_sensor(0x6F12,0x8D48);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0xFC00);
write_cmos_sensor(0x6F12,0x08B1);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x02E0);
write_cmos_sensor(0x6F12,0x8A48);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x2C01);
write_cmos_sensor(0x6F12,0x8246);
write_cmos_sensor(0x6F12,0x8A48);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x4E15);
write_cmos_sensor(0x6F12,0x0798);
write_cmos_sensor(0x6F12,0x8142);
write_cmos_sensor(0x6F12,0x03D2);
write_cmos_sensor(0x6F12,0x390C);
write_cmos_sensor(0x6F12,0x3620);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x24FC);
write_cmos_sensor(0x6F12,0x8348);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0xFD10);
write_cmos_sensor(0x6F12,0x31B9);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0x3EE0);
write_cmos_sensor(0x6F12,0x2FE0);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x2801);
write_cmos_sensor(0x6F12,0x07EB);
write_cmos_sensor(0x6F12,0x0047);
write_cmos_sensor(0x6F12,0x0798);
write_cmos_sensor(0x6F12,0x0128);
write_cmos_sensor(0x6F12,0x07D0);
write_cmos_sensor(0x6F12,0x15D9);
write_cmos_sensor(0x6F12,0x7D4A);
write_cmos_sensor(0x6F12,0x0146);
write_cmos_sensor(0x6F12,0x5846);
write_cmos_sensor(0x6F12,0x9269);
write_cmos_sensor(0x6F12,0x9047);
write_cmos_sensor(0x6F12,0x5846);
write_cmos_sensor(0x6F12,0x0BE0);
write_cmos_sensor(0x6F12,0xDBF8);
write_cmos_sensor(0x6F12,0x0010);
write_cmos_sensor(0x6F12,0x01EB);
write_cmos_sensor(0x6F12,0x0A00);
write_cmos_sensor(0x6F12,0x3843);
write_cmos_sensor(0x6F12,0x01C4);
write_cmos_sensor(0x6F12,0x07E0);
write_cmos_sensor(0x6F12,0x04C8);
write_cmos_sensor(0x6F12,0x02EB);
write_cmos_sensor(0x6F12,0x0A01);
write_cmos_sensor(0x6F12,0x3943);
write_cmos_sensor(0x6F12,0x02C4);
write_cmos_sensor(0x6F12,0x0C99);
write_cmos_sensor(0x6F12,0x8842);
write_cmos_sensor(0x6F12,0xF7D1);
write_cmos_sensor(0x6F12,0x0398);
write_cmos_sensor(0x6F12,0x0190);
write_cmos_sensor(0x6F12,0x0498);
write_cmos_sensor(0x6F12,0x0290);
write_cmos_sensor(0x6F12,0x0598);
write_cmos_sensor(0x6F12,0xB6FB);
write_cmos_sensor(0x6F12,0xF0F1);
write_cmos_sensor(0x6F12,0xB6FB);
write_cmos_sensor(0x6F12,0xF0F2);
write_cmos_sensor(0x6F12,0x00FB);
write_cmos_sensor(0x6F12,0x1163);
write_cmos_sensor(0x6F12,0x0098);
write_cmos_sensor(0x6F12,0x0099);
write_cmos_sensor(0x6F12,0x5043);
write_cmos_sensor(0x6F12,0x0144);
write_cmos_sensor(0x6F12,0x4FEA);
write_cmos_sensor(0x6F12,0x424A);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x331A);
write_cmos_sensor(0x6F12,0x109A);
write_cmos_sensor(0x6F12,0xDA40);
write_cmos_sensor(0x6F12,0xD207);
write_cmos_sensor(0x6F12,0x08D0);
write_cmos_sensor(0x6F12,0x06F0);
write_cmos_sensor(0x6F12,0x0103);
write_cmos_sensor(0x6F12,0x01AE);
write_cmos_sensor(0x6F12,0x0E9F);
write_cmos_sensor(0x6F12,0x56F8);
write_cmos_sensor(0x6F12,0x2320);
write_cmos_sensor(0x6F12,0x80C2);
write_cmos_sensor(0x6F12,0x46F8);
write_cmos_sensor(0x6F12,0x2320);
write_cmos_sensor(0x6F12,0x0B9B);
write_cmos_sensor(0x6F12,0x9945);
write_cmos_sensor(0x6F12,0x7FF4);
write_cmos_sensor(0x6F12,0x2BAF);
write_cmos_sensor(0x6F12,0x01A8);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2590);
write_cmos_sensor(0x6F12,0x03A8);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2560);
write_cmos_sensor(0x6F12,0x4AEA);
write_cmos_sensor(0x6F12,0x0545);
write_cmos_sensor(0x6F12,0xA9EB);
write_cmos_sensor(0x6F12,0x0600);
write_cmos_sensor(0x6F12,0x4FEA);
write_cmos_sensor(0x6F12,0xA00B);
write_cmos_sensor(0x6F12,0x5948);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0xFC00);
write_cmos_sensor(0x6F12,0x08B1);
write_cmos_sensor(0x6F12,0x0027);
write_cmos_sensor(0x6F12,0x02E0);
write_cmos_sensor(0x6F12,0x5648);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x2C71);
write_cmos_sensor(0x6F12,0x5648);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x4E15);
write_cmos_sensor(0x6F12,0x5945);
write_cmos_sensor(0x6F12,0x03D2);
write_cmos_sensor(0x6F12,0x290C);
write_cmos_sensor(0x6F12,0x3620);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xBDFB);
write_cmos_sensor(0x6F12,0x5048);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0xFD00);
write_cmos_sensor(0x6F12,0x20B9);
write_cmos_sensor(0x6F12,0x4E48);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x2801);
write_cmos_sensor(0x6F12,0x05EB);
write_cmos_sensor(0x6F12,0x0045);
write_cmos_sensor(0x6F12,0x5846);
write_cmos_sensor(0x6F12,0xBBF1);
write_cmos_sensor(0x6F12,0x010F);
write_cmos_sensor(0x6F12,0x06D0);
write_cmos_sensor(0x6F12,0x10D9);
write_cmos_sensor(0x6F12,0x4A4A);
write_cmos_sensor(0x6F12,0x0146);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0x9269);
write_cmos_sensor(0x6F12,0x9047);
write_cmos_sensor(0x6F12,0x08E0);
write_cmos_sensor(0x6F12,0x3068);
write_cmos_sensor(0x6F12,0x3844);
write_cmos_sensor(0x6F12,0x2843);
write_cmos_sensor(0x6F12,0x01C4);
write_cmos_sensor(0x6F12,0x05E0);
write_cmos_sensor(0x6F12,0x01CE);
write_cmos_sensor(0x6F12,0x3844);
write_cmos_sensor(0x6F12,0x2843);
write_cmos_sensor(0x6F12,0x01C4);
write_cmos_sensor(0x6F12,0x4E45);
write_cmos_sensor(0x6F12,0xF9D1);
write_cmos_sensor(0x6F12,0x01A8);
write_cmos_sensor(0x6F12,0x4AEA);
write_cmos_sensor(0x6F12,0x0845);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2890);
write_cmos_sensor(0x6F12,0x03A8);
write_cmos_sensor(0x6F12,0xDFF8);
write_cmos_sensor(0x6F12,0xF4A0);
write_cmos_sensor(0x6F12,0x50F8);
write_cmos_sensor(0x6F12,0x2860);
write_cmos_sensor(0x6F12,0xA9EB);
write_cmos_sensor(0x6F12,0x0600);
write_cmos_sensor(0x6F12,0x4FEA);
write_cmos_sensor(0x6F12,0xA008);
write_cmos_sensor(0x6F12,0x9AF8);
write_cmos_sensor(0x6F12,0xFC00);
write_cmos_sensor(0x6F12,0x08B1);
write_cmos_sensor(0x6F12,0x0027);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0xBAF8);
write_cmos_sensor(0x6F12,0x2C71);
write_cmos_sensor(0x6F12,0x3848);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x4E15);
write_cmos_sensor(0x6F12,0x4145);
write_cmos_sensor(0x6F12,0x03D2);
write_cmos_sensor(0x6F12,0x290C);
write_cmos_sensor(0x6F12,0x3620);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x80FB);
write_cmos_sensor(0x6F12,0x9AF8);
write_cmos_sensor(0x6F12,0xFD10);
write_cmos_sensor(0x6F12,0x19B9);
write_cmos_sensor(0x6F12,0xBAF8);
write_cmos_sensor(0x6F12,0x2801);
write_cmos_sensor(0x6F12,0x05EB);
write_cmos_sensor(0x6F12,0x0045);
write_cmos_sensor(0x6F12,0x4046);
write_cmos_sensor(0x6F12,0xB8F1);
write_cmos_sensor(0x6F12,0x010F);
write_cmos_sensor(0x6F12,0x06D0);
write_cmos_sensor(0x6F12,0x10D9);
write_cmos_sensor(0x6F12,0x2C4A);
write_cmos_sensor(0x6F12,0x0146);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0x9269);
write_cmos_sensor(0x6F12,0x9047);
write_cmos_sensor(0x6F12,0x08E0);
write_cmos_sensor(0x6F12,0x3068);
write_cmos_sensor(0x6F12,0x3844);
write_cmos_sensor(0x6F12,0x2843);
write_cmos_sensor(0x6F12,0x01C4);
write_cmos_sensor(0x6F12,0x05E0);
write_cmos_sensor(0x6F12,0x01CE);
write_cmos_sensor(0x6F12,0x3844);
write_cmos_sensor(0x6F12,0x2843);
write_cmos_sensor(0x6F12,0x01C4);
write_cmos_sensor(0x6F12,0x4E45);
write_cmos_sensor(0x6F12,0xF9D1);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0xFF30);
write_cmos_sensor(0x6F12,0x2060);
write_cmos_sensor(0x6F12,0x1298);
write_cmos_sensor(0x6F12,0x8442);
write_cmos_sensor(0x6F12,0x01D0);
write_cmos_sensor(0x6F12,0x0120);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x1D49);
write_cmos_sensor(0x6F12,0x0870);
write_cmos_sensor(0x6F12,0x1298);
write_cmos_sensor(0x6F12,0x201A);
write_cmos_sensor(0x6F12,0x8008);
write_cmos_sensor(0x6F12,0x0884);
write_cmos_sensor(0x6F12,0x13B0);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xF08F);
write_cmos_sensor(0x6F12,0x2DE9);
write_cmos_sensor(0x6F12,0xF041);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0x6920);
write_cmos_sensor(0x6F12,0x0D46);
write_cmos_sensor(0x6F12,0x0446);
write_cmos_sensor(0x6F12,0x032A);
write_cmos_sensor(0x6F12,0x15D0);
write_cmos_sensor(0x6F12,0x062A);
write_cmos_sensor(0x6F12,0x13D0);
write_cmos_sensor(0x6F12,0x1648);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xC069);
write_cmos_sensor(0x6F12,0x87B2);
write_cmos_sensor(0x6F12,0x060C);
write_cmos_sensor(0x6F12,0x3946);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x25FB);
write_cmos_sensor(0x6F12,0x2946);
write_cmos_sensor(0x6F12,0x2046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x3FFB);
write_cmos_sensor(0x6F12,0x3946);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xF041);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x1ABB);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xF041);
write_cmos_sensor(0x6F12,0x1EE6);
write_cmos_sensor(0x6F12,0x0346);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x0029);
write_cmos_sensor(0x6F12,0x0BD0);
write_cmos_sensor(0x6F12,0x0749);
write_cmos_sensor(0x6F12,0x0978);
write_cmos_sensor(0x6F12,0x0029);
write_cmos_sensor(0x6F12,0x07D0);
write_cmos_sensor(0x6F12,0x002A);
write_cmos_sensor(0x6F12,0x05D0);
write_cmos_sensor(0x6F12,0x73B1);
write_cmos_sensor(0x6F12,0x02F0);
write_cmos_sensor(0x6F12,0x0300);
write_cmos_sensor(0x6F12,0x0128);
write_cmos_sensor(0x6F12,0x0CD1);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x7047);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1C20);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1EB0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x3FF0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1350);
write_cmos_sensor(0x6F12,0x9007);
write_cmos_sensor(0x6F12,0xF3D0);
write_cmos_sensor(0x6F12,0x0120);
write_cmos_sensor(0x6F12,0xF1E7);
write_cmos_sensor(0x6F12,0x2DE9);
write_cmos_sensor(0x6F12,0xF05F);
write_cmos_sensor(0x6F12,0x0C46);
write_cmos_sensor(0x6F12,0x0546);
write_cmos_sensor(0x6F12,0x9089);
write_cmos_sensor(0x6F12,0x518A);
write_cmos_sensor(0x6F12,0x1646);
write_cmos_sensor(0x6F12,0x401A);
write_cmos_sensor(0x6F12,0x918A);
write_cmos_sensor(0x6F12,0x401A);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x80FA);
write_cmos_sensor(0x6F12,0xFD48);
write_cmos_sensor(0x6F12,0xB5F8);
write_cmos_sensor(0x6F12,0x5A10);
write_cmos_sensor(0x6F12,0x8278);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6A00);
write_cmos_sensor(0x6F12,0x02FB);
write_cmos_sensor(0x6F12,0x1011);
write_cmos_sensor(0x6F12,0xB5F8);
write_cmos_sensor(0x6F12,0x6620);
write_cmos_sensor(0x6F12,0x89B2);
write_cmos_sensor(0x6F12,0x02FB);
write_cmos_sensor(0x6F12,0x0010);
write_cmos_sensor(0x6F12,0xE97E);
write_cmos_sensor(0x6F12,0x401A);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x80F8);
write_cmos_sensor(0x6F12,0x7088);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0xB210);
write_cmos_sensor(0x6F12,0x401A);
write_cmos_sensor(0x6F12,0x87B2);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x2200);
write_cmos_sensor(0x6F12,0x30B1);
write_cmos_sensor(0x6F12,0x2846);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xF4FA);
write_cmos_sensor(0x6F12,0x10B1);
write_cmos_sensor(0x6F12,0xF048);
write_cmos_sensor(0x6F12,0x0088);
write_cmos_sensor(0x6F12,0x00B1);
write_cmos_sensor(0x6F12,0x0120);
write_cmos_sensor(0x6F12,0x2070);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6900);
write_cmos_sensor(0x6F12,0x0228);
write_cmos_sensor(0x6F12,0x03D1);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6B00);
write_cmos_sensor(0x6F12,0x0228);
write_cmos_sensor(0x6F12,0x14D0);
write_cmos_sensor(0x6F12,0xB5F8);
write_cmos_sensor(0x6F12,0x9E00);
write_cmos_sensor(0x6F12,0x0221);
write_cmos_sensor(0x6F12,0xB1EB);
write_cmos_sensor(0x6F12,0x101F);
write_cmos_sensor(0x6F12,0x03D1);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6B00);
write_cmos_sensor(0x6F12,0x0228);
write_cmos_sensor(0x6F12,0x0AD0);
write_cmos_sensor(0x6F12,0x0023);
write_cmos_sensor(0x6F12,0x6370);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6D00);
write_cmos_sensor(0x6F12,0x591C);
write_cmos_sensor(0x6F12,0x30B1);
write_cmos_sensor(0x6F12,0xEA7E);
write_cmos_sensor(0x6F12,0xC8F5);
write_cmos_sensor(0x6F12,0x4960);
write_cmos_sensor(0x6F12,0x801A);
write_cmos_sensor(0x6F12,0x02E0);
write_cmos_sensor(0x6F12,0x0123);
write_cmos_sensor(0x6F12,0xF3E7);
write_cmos_sensor(0x6F12,0x6888);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6B20);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x80F9);
write_cmos_sensor(0x6F12,0xB9FB);
write_cmos_sensor(0x6F12,0xF2F0);
write_cmos_sensor(0x6F12,0x3844);
write_cmos_sensor(0x6F12,0x80B2);
write_cmos_sensor(0x6F12,0x6080);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6B20);
write_cmos_sensor(0x6F12,0x4FF4);
write_cmos_sensor(0x6F12,0x426E);
write_cmos_sensor(0x6F12,0xBEFB);
write_cmos_sensor(0x6F12,0xF2F8);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x0A0C);
write_cmos_sensor(0x6F12,0xE045);
write_cmos_sensor(0x6F12,0x06D9);
write_cmos_sensor(0x6F12,0x2FB1);
write_cmos_sensor(0x6F12,0x728A);
write_cmos_sensor(0x6F12,0x02EB);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x0CEB);
write_cmos_sensor(0x6F12,0x0A02);
write_cmos_sensor(0x6F12,0x04E0);
write_cmos_sensor(0x6F12,0xBEFB);
write_cmos_sensor(0x6F12,0xF2F2);
write_cmos_sensor(0x6F12,0xB6F8);
write_cmos_sensor(0x6F12,0x12C0);
write_cmos_sensor(0x6F12,0x6244);
write_cmos_sensor(0x6F12,0xA280);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6B20);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x200C);
write_cmos_sensor(0x6F12,0xBCFB);
write_cmos_sensor(0x6F12,0xF2F8);
write_cmos_sensor(0x6F12,0x8045);
write_cmos_sensor(0x6F12,0x14D2);
write_cmos_sensor(0x6F12,0x728A);
write_cmos_sensor(0x6F12,0x0AB1);
write_cmos_sensor(0x6F12,0x0246);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0xE280);
write_cmos_sensor(0x6F12,0xB6F8);
write_cmos_sensor(0x6F12,0x12C0);
write_cmos_sensor(0x6F12,0x92B2);
write_cmos_sensor(0x6F12,0x6244);
write_cmos_sensor(0x6F12,0x92B2);
write_cmos_sensor(0x6F12,0xE280);
write_cmos_sensor(0x6F12,0x9346);
write_cmos_sensor(0x6F12,0x7289);
write_cmos_sensor(0x6F12,0x02FB);
write_cmos_sensor(0x6F12,0x01FC);
write_cmos_sensor(0x6F12,0xBCF5);
write_cmos_sensor(0x6F12,0x7C6F);
write_cmos_sensor(0x6F12,0x04DD);
write_cmos_sensor(0x6F12,0x7E22);
write_cmos_sensor(0x6F12,0x1AE0);
write_cmos_sensor(0x6F12,0xBCFB);
write_cmos_sensor(0x6F12,0xF2F2);
write_cmos_sensor(0x6F12,0xECE7);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x400C);
write_cmos_sensor(0x6F12,0xBCFB);
write_cmos_sensor(0x6F12,0xF1FC);
write_cmos_sensor(0x6F12,0xB2FB);
write_cmos_sensor(0x6F12,0xFCF8);
write_cmos_sensor(0x6F12,0x0CFB);
write_cmos_sensor(0x6F12,0x182C);
write_cmos_sensor(0x6F12,0xBCF1);
write_cmos_sensor(0x6F12,0x000F);
write_cmos_sensor(0x6F12,0x4FF0);
write_cmos_sensor(0x6F12,0x400C);
write_cmos_sensor(0x6F12,0xBCFB);
write_cmos_sensor(0x6F12,0xF1FC);
write_cmos_sensor(0x6F12,0xB2FB);
write_cmos_sensor(0x6F12,0xFCF2);
write_cmos_sensor(0x6F12,0x02D0);
write_cmos_sensor(0x6F12,0x5200);
write_cmos_sensor(0x6F12,0x921C);
write_cmos_sensor(0x6F12,0x03E0);
write_cmos_sensor(0x6F12,0x4FF6);
write_cmos_sensor(0x6F12,0xFF7C);
write_cmos_sensor(0x6F12,0x0CEA);
write_cmos_sensor(0x6F12,0x4202);
write_cmos_sensor(0x6F12,0x2281);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x82FC);
write_cmos_sensor(0x6F12,0x0C22);
write_cmos_sensor(0x6F12,0xBCFB);
write_cmos_sensor(0x6F12,0xF2F8);
write_cmos_sensor(0x6F12,0xA4F8);
write_cmos_sensor(0x6F12,0x0A80);
write_cmos_sensor(0x6F12,0xBCFB);
write_cmos_sensor(0x6F12,0xF2F8);
write_cmos_sensor(0x6F12,0x02FB);
write_cmos_sensor(0x6F12,0x18C2);
write_cmos_sensor(0x6F12,0xA281);
write_cmos_sensor(0x6F12,0xAB4A);
write_cmos_sensor(0x6F12,0x5288);
write_cmos_sensor(0x6F12,0xE281);
write_cmos_sensor(0x6F12,0xB6F8);
write_cmos_sensor(0x6F12,0x0AC0);
write_cmos_sensor(0x6F12,0x6FF0);
write_cmos_sensor(0x6F12,0x0A02);
write_cmos_sensor(0x6F12,0x02EB);
write_cmos_sensor(0x6F12,0x9C02);
write_cmos_sensor(0x6F12,0x2282);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6D80);
write_cmos_sensor(0x6F12,0xB8F1);
write_cmos_sensor(0x6F12,0x000F);
write_cmos_sensor(0x6F12,0x02D0);
write_cmos_sensor(0x6F12,0x09F1);
write_cmos_sensor(0x6F12,0x0702);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x4A46);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x82FC);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6B20);
write_cmos_sensor(0x6F12,0x4843);
write_cmos_sensor(0x6F12,0xBCFB);
write_cmos_sensor(0x6F12,0xF2FC);
write_cmos_sensor(0x6F12,0xBC44);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x8CFC);
write_cmos_sensor(0x6F12,0x2028);
write_cmos_sensor(0x6F12,0x3ADC);
write_cmos_sensor(0x6F12,0xB8F1);
write_cmos_sensor(0x6F12,0x000F);
write_cmos_sensor(0x6F12,0x03D0);
write_cmos_sensor(0x6F12,0x0720);
write_cmos_sensor(0x6F12,0xB0FB);
write_cmos_sensor(0x6F12,0xF1F0);
write_cmos_sensor(0x6F12,0x00E0);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x9749);
write_cmos_sensor(0x6F12,0x6082);
write_cmos_sensor(0x6F12,0x8888);
write_cmos_sensor(0x6F12,0xA082);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x9200);
write_cmos_sensor(0x6F12,0x3844);
write_cmos_sensor(0x6F12,0xE082);
write_cmos_sensor(0x6F12,0x3FB1);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6B20);
write_cmos_sensor(0x6F12,0x0AEB);
write_cmos_sensor(0x6F12,0x0700);
write_cmos_sensor(0x6F12,0xBEFB);
write_cmos_sensor(0x6F12,0xF2F2);
write_cmos_sensor(0x6F12,0x8242);
write_cmos_sensor(0x6F12,0x03D8);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6B20);
write_cmos_sensor(0x6F12,0xBEFB);
write_cmos_sensor(0x6F12,0xF2F0);
write_cmos_sensor(0x6F12,0x2083);
write_cmos_sensor(0x6F12,0x82B2);
write_cmos_sensor(0x6F12,0x708A);
write_cmos_sensor(0x6F12,0x08B1);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x9200);
write_cmos_sensor(0x6F12,0x1044);
write_cmos_sensor(0x6F12,0x2083);
write_cmos_sensor(0x6F12,0xC888);
write_cmos_sensor(0x6F12,0x6083);
write_cmos_sensor(0x6F12,0x3088);
write_cmos_sensor(0x6F12,0xA083);
write_cmos_sensor(0x6F12,0xB5F8);
write_cmos_sensor(0x6F12,0x9E20);
write_cmos_sensor(0x6F12,0x3088);
write_cmos_sensor(0x6F12,0x7189);
write_cmos_sensor(0x6F12,0x1209);
write_cmos_sensor(0x6F12,0x01FB);
write_cmos_sensor(0x6F12,0x0200);
write_cmos_sensor(0x6F12,0xE083);
write_cmos_sensor(0x6F12,0x708A);
write_cmos_sensor(0x6F12,0x40B3);
write_cmos_sensor(0x6F12,0x608A);
write_cmos_sensor(0x6F12,0x0828);
write_cmos_sensor(0x6F12,0x16D0);
write_cmos_sensor(0x6F12,0x0C28);
write_cmos_sensor(0x6F12,0x14D0);
write_cmos_sensor(0x6F12,0x0D28);
write_cmos_sensor(0x6F12,0x12D0);
write_cmos_sensor(0x6F12,0x12E0);
write_cmos_sensor(0x6F12,0x02FB);
write_cmos_sensor(0x6F12,0x0CF2);
write_cmos_sensor(0x6F12,0x203A);
write_cmos_sensor(0x6F12,0xD017);
write_cmos_sensor(0x6F12,0x02EB);
write_cmos_sensor(0x6F12,0x9060);
write_cmos_sensor(0x6F12,0x20F0);
write_cmos_sensor(0x6F12,0x3F00);
write_cmos_sensor(0x6F12,0x101A);
write_cmos_sensor(0x6F12,0xC217);
write_cmos_sensor(0x6F12,0x00EB);
write_cmos_sensor(0x6F12,0x1272);
write_cmos_sensor(0x6F12,0x1211);
write_cmos_sensor(0x6F12,0xA0EB);
write_cmos_sensor(0x6F12,0x0210);
write_cmos_sensor(0x6F12,0x90FB);
write_cmos_sensor(0x6F12,0xF1F0);
write_cmos_sensor(0x6F12,0xBAE7);
write_cmos_sensor(0x6F12,0x13B1);
write_cmos_sensor(0x6F12,0x0428);
write_cmos_sensor(0x6F12,0x05D0);
write_cmos_sensor(0x6F12,0x0AE0);
write_cmos_sensor(0x6F12,0x801C);
write_cmos_sensor(0x6F12,0x6082);
write_cmos_sensor(0x6F12,0x0BF1);
write_cmos_sensor(0x6F12,0x0200);
write_cmos_sensor(0x6F12,0x04E0);
write_cmos_sensor(0x6F12,0x23B1);
write_cmos_sensor(0x6F12,0x0520);
write_cmos_sensor(0x6F12,0x6082);
write_cmos_sensor(0x6F12,0x0BF1);
write_cmos_sensor(0x6F12,0x0100);
write_cmos_sensor(0x6F12,0xE080);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x6D00);
write_cmos_sensor(0x6F12,0x3A46);
write_cmos_sensor(0x6F12,0x1946);
write_cmos_sensor(0x6F12,0xFFF7);
write_cmos_sensor(0x6F12,0xAEFE);
write_cmos_sensor(0x6F12,0x6B49);
write_cmos_sensor(0x6F12,0x0880);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xF09F);
write_cmos_sensor(0x6F12,0x2DE9);
write_cmos_sensor(0x6F12,0xF041);
write_cmos_sensor(0x6F12,0x0446);
write_cmos_sensor(0x6F12,0x6948);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x406A);
write_cmos_sensor(0x6F12,0x87B2);
write_cmos_sensor(0x6F12,0x4FEA);
write_cmos_sensor(0x6F12,0x1048);
write_cmos_sensor(0x6F12,0x3946);
write_cmos_sensor(0x6F12,0x4046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xB4F9);
write_cmos_sensor(0x6F12,0x2046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xD9F9);
write_cmos_sensor(0x6F12,0x634E);
write_cmos_sensor(0x6F12,0x04F1);
write_cmos_sensor(0x6F12,0x5805);
write_cmos_sensor(0x6F12,0xB6F8);
write_cmos_sensor(0x6F12,0xB400);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xD7F9);
write_cmos_sensor(0x6F12,0x80B2);
write_cmos_sensor(0x6F12,0xE880);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0xB810);
write_cmos_sensor(0x6F12,0x0129);
write_cmos_sensor(0x6F12,0x03D0);
write_cmos_sensor(0x6F12,0xB6F8);
write_cmos_sensor(0x6F12,0xB600);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xCDF9);
write_cmos_sensor(0x6F12,0xA881);
write_cmos_sensor(0x6F12,0x96F8);
write_cmos_sensor(0x6F12,0xB800);
write_cmos_sensor(0x6F12,0x0128);
write_cmos_sensor(0x6F12,0x0ED0);
write_cmos_sensor(0x6F12,0x5948);
write_cmos_sensor(0x6F12,0x0068);
write_cmos_sensor(0x6F12,0xB0F8);
write_cmos_sensor(0x6F12,0x4800);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xC2F9);
write_cmos_sensor(0x6F12,0xA4F8);
write_cmos_sensor(0x6F12,0x4800);
write_cmos_sensor(0x6F12,0x3946);
write_cmos_sensor(0x6F12,0x4046);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xF041);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x8CB9);
write_cmos_sensor(0x6F12,0x34F8);
write_cmos_sensor(0x6F12,0x420F);
write_cmos_sensor(0x6F12,0xE080);
write_cmos_sensor(0x6F12,0xF4E7);
write_cmos_sensor(0x6F12,0x2DE9);
write_cmos_sensor(0x6F12,0xF041);
write_cmos_sensor(0x6F12,0x4A4C);
write_cmos_sensor(0x6F12,0x4F4D);
write_cmos_sensor(0x6F12,0x0F46);
write_cmos_sensor(0x6F12,0x8046);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x4430);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0xFD20);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0x5C11);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x23B1);
write_cmos_sensor(0x6F12,0x09B1);
write_cmos_sensor(0x6F12,0x0220);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0x02B1);
write_cmos_sensor(0x6F12,0x0120);
write_cmos_sensor(0x6F12,0x484E);
write_cmos_sensor(0x6F12,0xA6F8);
write_cmos_sensor(0x6F12,0x4602);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x2030);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x23B1);
write_cmos_sensor(0x6F12,0x09B1);
write_cmos_sensor(0x6F12,0x0220);
write_cmos_sensor(0x6F12,0x01E0);
write_cmos_sensor(0x6F12,0x02B1);
write_cmos_sensor(0x6F12,0x0120);
write_cmos_sensor(0x6F12,0x70B1);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x0221);
write_cmos_sensor(0x6F12,0x47F2);
write_cmos_sensor(0x6F12,0xC600);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x62F9);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0xFD10);
write_cmos_sensor(0x6F12,0x39B1);
write_cmos_sensor(0x6F12,0x3E48);
write_cmos_sensor(0x6F12,0x90F8);
write_cmos_sensor(0x6F12,0x5D00);
write_cmos_sensor(0x6F12,0x18B1);
write_cmos_sensor(0x6F12,0x0120);
write_cmos_sensor(0x6F12,0x02E0);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0xEFE7);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0xB120);
write_cmos_sensor(0x6F12,0x5040);
write_cmos_sensor(0x6F12,0x84F8);
write_cmos_sensor(0x6F12,0x5C01);
write_cmos_sensor(0x6F12,0x84F8);
write_cmos_sensor(0x6F12,0x5D01);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x2020);
write_cmos_sensor(0x6F12,0x022A);
write_cmos_sensor(0x6F12,0x04D1);
write_cmos_sensor(0x6F12,0x19B1);
write_cmos_sensor(0x6F12,0x80EA);
write_cmos_sensor(0x6F12,0x0102);
write_cmos_sensor(0x6F12,0x84F8);
write_cmos_sensor(0x6F12,0x5C21);
write_cmos_sensor(0x6F12,0xA6F8);
write_cmos_sensor(0x6F12,0x6A02);
write_cmos_sensor(0x6F12,0x2948);
write_cmos_sensor(0x6F12,0x9030);
write_cmos_sensor(0x6F12,0x0646);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x71F9);
write_cmos_sensor(0x6F12,0xC4F8);
write_cmos_sensor(0x6F12,0x6001);
write_cmos_sensor(0x6F12,0x3046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x71F9);
write_cmos_sensor(0x6F12,0xC4F8);
write_cmos_sensor(0x6F12,0x6401);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0x2020);
write_cmos_sensor(0x6F12,0x2B4D);
write_cmos_sensor(0x6F12,0x022A);
write_cmos_sensor(0x6F12,0x3FD0);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0x5C01);
write_cmos_sensor(0x6F12,0xE8B3);
write_cmos_sensor(0x6F12,0x0021);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0xAB32);
write_cmos_sensor(0x6F12,0x5940);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0xFC30);
write_cmos_sensor(0x6F12,0x5840);
write_cmos_sensor(0x6F12,0x14F8);
write_cmos_sensor(0x6F12,0xEB3F);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0x3640);
write_cmos_sensor(0x6F12,0x2344);
write_cmos_sensor(0x6F12,0x9B07);
write_cmos_sensor(0x6F12,0x00D0);
write_cmos_sensor(0x6F12,0x0123);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0xAA42);
write_cmos_sensor(0x6F12,0x6340);
write_cmos_sensor(0x6F12,0x5840);
write_cmos_sensor(0x6F12,0xA2B1);
write_cmos_sensor(0x6F12,0x40EA);
write_cmos_sensor(0x6F12,0x4100);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0xC213);
write_cmos_sensor(0x6F12,0x40EA);
write_cmos_sensor(0x6F12,0x8100);
write_cmos_sensor(0x6F12,0x95F8);
write_cmos_sensor(0x6F12,0xC313);
write_cmos_sensor(0x6F12,0x40EA);
write_cmos_sensor(0x6F12,0xC100);
write_cmos_sensor(0x6F12,0x1A49);
write_cmos_sensor(0x6F12,0xA1F8);
write_cmos_sensor(0x6F12,0x4A03);
write_cmos_sensor(0x6F12,0x8915);
write_cmos_sensor(0x6F12,0x4AF2);
write_cmos_sensor(0x6F12,0x4A30);
write_cmos_sensor(0x6F12,0x022A);
write_cmos_sensor(0x6F12,0x34D0);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x07F9);
write_cmos_sensor(0x6F12,0x0D48);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x816A);
write_cmos_sensor(0x6F12,0x0C0C);
write_cmos_sensor(0x6F12,0x8DB2);
write_cmos_sensor(0x6F12,0x2946);
write_cmos_sensor(0x6F12,0x2046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xFEF8);
write_cmos_sensor(0x6F12,0x3946);
write_cmos_sensor(0x6F12,0x4046);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x36F9);
write_cmos_sensor(0x6F12,0x2946);
write_cmos_sensor(0x6F12,0x2046);
write_cmos_sensor(0x6F12,0xBDE8);
write_cmos_sensor(0x6F12,0xF041);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xF3B8);
write_cmos_sensor(0x6F12,0x16E0);
write_cmos_sensor(0x6F12,0x19E0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1EB0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x19E0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x4A00);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x3FF0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x0010);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x0520);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x2970);
write_cmos_sensor(0x6F12,0x4000);
write_cmos_sensor(0x6F12,0xF000);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x05C0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1350);
write_cmos_sensor(0x6F12,0x4000);
write_cmos_sensor(0x6F12,0xA000);
write_cmos_sensor(0x6F12,0x94F8);
write_cmos_sensor(0x6F12,0x5C11);
write_cmos_sensor(0x6F12,0x0846);
write_cmos_sensor(0x6F12,0xBAE7);
write_cmos_sensor(0x6F12,0x0121);
write_cmos_sensor(0x6F12,0xA5E7);
write_cmos_sensor(0x6F12,0x0122);
write_cmos_sensor(0x6F12,0xC9E7);
write_cmos_sensor(0x6F12,0x70B5);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF6);
write_cmos_sensor(0x6F12,0x4351);
write_cmos_sensor(0x6F12,0x4648);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x0DF9);
write_cmos_sensor(0x6F12,0x464D);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF6);
write_cmos_sensor(0x6F12,0x0B51);
write_cmos_sensor(0x6F12,0x2860);
write_cmos_sensor(0x6F12,0x4448);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x05F9);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF6);
write_cmos_sensor(0x6F12,0x3741);
write_cmos_sensor(0x6F12,0x6860);
write_cmos_sensor(0x6F12,0x4248);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xFEF8);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF6);
write_cmos_sensor(0x6F12,0x1141);
write_cmos_sensor(0x6F12,0x4048);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xF8F8);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF6);
write_cmos_sensor(0x6F12,0x7B31);
write_cmos_sensor(0x6F12,0xE860);
write_cmos_sensor(0x6F12,0x3D48);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xF1F8);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF6);
write_cmos_sensor(0x6F12,0x1531);
write_cmos_sensor(0x6F12,0xA860);
write_cmos_sensor(0x6F12,0x3B48);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xEAF8);
write_cmos_sensor(0x6F12,0x3A4C);
write_cmos_sensor(0x6F12,0x2861);
write_cmos_sensor(0x6F12,0x44F6);
write_cmos_sensor(0x6F12,0x9C60);
write_cmos_sensor(0x6F12,0xE18C);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xE8F8);
write_cmos_sensor(0x6F12,0xE08C);
write_cmos_sensor(0x6F12,0x384E);
write_cmos_sensor(0x6F12,0x3749);
write_cmos_sensor(0x6F12,0x46F8);
write_cmos_sensor(0x6F12,0x2010);
write_cmos_sensor(0x6F12,0x401C);
write_cmos_sensor(0x6F12,0x81B2);
write_cmos_sensor(0x6F12,0xE184);
write_cmos_sensor(0x6F12,0x43F2);
write_cmos_sensor(0x6F12,0xAC00);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xDCF8);
write_cmos_sensor(0x6F12,0xE08C);
write_cmos_sensor(0x6F12,0x3349);
write_cmos_sensor(0x6F12,0x46F8);
write_cmos_sensor(0x6F12,0x2010);
write_cmos_sensor(0x6F12,0x401C);
write_cmos_sensor(0x6F12,0x81B2);
write_cmos_sensor(0x6F12,0xE184);
write_cmos_sensor(0x6F12,0x43F2);
write_cmos_sensor(0x6F12,0xB000);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xD1F8);
write_cmos_sensor(0x6F12,0xE08C);
write_cmos_sensor(0x6F12,0x2F49);
write_cmos_sensor(0x6F12,0x46F8);
write_cmos_sensor(0x6F12,0x2010);
write_cmos_sensor(0x6F12,0x401C);
write_cmos_sensor(0x6F12,0x81B2);
write_cmos_sensor(0x6F12,0xE184);
write_cmos_sensor(0x6F12,0x43F2);
write_cmos_sensor(0x6F12,0x1840);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xC6F8);
write_cmos_sensor(0x6F12,0xE08C);
write_cmos_sensor(0x6F12,0x2A49);
write_cmos_sensor(0x6F12,0x46F8);
write_cmos_sensor(0x6F12,0x2010);
write_cmos_sensor(0x6F12,0x401C);
write_cmos_sensor(0x6F12,0x81B2);
write_cmos_sensor(0x6F12,0xE184);
write_cmos_sensor(0x6F12,0x43F2);
write_cmos_sensor(0x6F12,0x1C40);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xBBF8);
write_cmos_sensor(0x6F12,0xE08C);
write_cmos_sensor(0x6F12,0x2649);
write_cmos_sensor(0x6F12,0x46F8);
write_cmos_sensor(0x6F12,0x2010);
write_cmos_sensor(0x6F12,0x401C);
write_cmos_sensor(0x6F12,0xE084);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF6);
write_cmos_sensor(0x6F12,0x5921);
write_cmos_sensor(0x6F12,0x2348);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0xAAF8);
write_cmos_sensor(0x6F12,0x224C);
write_cmos_sensor(0x6F12,0x6861);
write_cmos_sensor(0x6F12,0x4FF4);
write_cmos_sensor(0x6F12,0x8050);
write_cmos_sensor(0x6F12,0x24F8);
write_cmos_sensor(0x6F12,0xCE0F);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0x6081);
write_cmos_sensor(0x6F12,0xA082);
write_cmos_sensor(0x6F12,0xAFF2);
write_cmos_sensor(0x6F12,0xB151);
write_cmos_sensor(0x6F12,0x1E48);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x9CF8);
write_cmos_sensor(0x6F12,0xE861);
write_cmos_sensor(0x6F12,0x0020);
write_cmos_sensor(0x6F12,0x24F8);
write_cmos_sensor(0x6F12,0xCE0C);
write_cmos_sensor(0x6F12,0x0246);
write_cmos_sensor(0x6F12,0xAFF2);
write_cmos_sensor(0x6F12,0x4751);
write_cmos_sensor(0x6F12,0x1A48);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x92F8);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF2);
write_cmos_sensor(0x6F12,0xE321);
write_cmos_sensor(0x6F12,0x2862);
write_cmos_sensor(0x6F12,0x1748);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x8BF8);
write_cmos_sensor(0x6F12,0x0022);
write_cmos_sensor(0x6F12,0xAFF2);
write_cmos_sensor(0x6F12,0x8121);
write_cmos_sensor(0x6F12,0x6862);
write_cmos_sensor(0x6F12,0x1548);
write_cmos_sensor(0x6F12,0x00F0);
write_cmos_sensor(0x6F12,0x84F8);
write_cmos_sensor(0x6F12,0xA862);
write_cmos_sensor(0x6F12,0x70BD);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x333B);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x3FF0);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x3757);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x7BED);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0xAC61);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0xA831);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x9E01);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x1E80);
write_cmos_sensor(0x6F12,0x9E04);
write_cmos_sensor(0x6F12,0x0BE0);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x2E10);
write_cmos_sensor(0x6F12,0x18BF);
write_cmos_sensor(0x6F12,0x06F2);
write_cmos_sensor(0x6F12,0x0144);
write_cmos_sensor(0x6F12,0x04F5);
write_cmos_sensor(0x6F12,0x18BF);
write_cmos_sensor(0x6F12,0x0AF2);
write_cmos_sensor(0x6F12,0x0148);
write_cmos_sensor(0x6F12,0x1FFA);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x79B3);
write_cmos_sensor(0x6F12,0x2000);
write_cmos_sensor(0x6F12,0x4A00);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x3C7B);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0A65);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x6979);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x79D9);
write_cmos_sensor(0x6F12,0x43F2);
write_cmos_sensor(0x6F12,0x1B3C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x43F2);
write_cmos_sensor(0x6F12,0xC32C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x40F2);
write_cmos_sensor(0x6F12,0xF76C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x43F2);
write_cmos_sensor(0x6F12,0xBD6C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x43F2);
write_cmos_sensor(0x6F12,0x476C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x40F2);
write_cmos_sensor(0x6F12,0x777C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x4AF6);
write_cmos_sensor(0x6F12,0x614C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x4AF6);
write_cmos_sensor(0x6F12,0x310C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x47F6);
write_cmos_sensor(0x6F12,0xB31C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x43F6);
write_cmos_sensor(0x6F12,0xB53C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x40F2);
write_cmos_sensor(0x6F12,0xA90C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x43F6);
write_cmos_sensor(0x6F12,0x7B4C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x40F6);
write_cmos_sensor(0x6F12,0x2D2C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x46F6);
write_cmos_sensor(0x6F12,0x791C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x46F6);
write_cmos_sensor(0x6F12,0x6D1C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x47F6);
write_cmos_sensor(0x6F12,0xDB2C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x47F6);
write_cmos_sensor(0x6F12,0xC72C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x47F6);
write_cmos_sensor(0x6F12,0xD91C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x4DF2);
write_cmos_sensor(0x6F12,0x536C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x4DF2);
write_cmos_sensor(0x6F12,0xF35C);
write_cmos_sensor(0x6F12,0xC0F2);
write_cmos_sensor(0x6F12,0x000C);
write_cmos_sensor(0x6F12,0x6047);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x30D3);
write_cmos_sensor(0x6F12,0x01C0);
write_cmos_sensor(0x6F12,0x0000);
write_cmos_sensor(0x6F12,0x0FFF);
write_cmos_sensor(0x6028,0x2000);
write_cmos_sensor(0x602A,0x1930);
write_cmos_sensor(0x6F12,0x0001);
write_cmos_sensor(0x602A,0x1940);
write_cmos_sensor(0x6F12,0x00AE);
write_cmos_sensor(0x602A,0x194A);
write_cmos_sensor(0x6F12,0x007A);
write_cmos_sensor(0x602A,0x1954);
write_cmos_sensor(0x6F12,0x0047);
write_cmos_sensor(0x602A,0x195E);
write_cmos_sensor(0x6F12,0x0066);
write_cmos_sensor(0x602A,0x1976);
write_cmos_sensor(0x6F12,0x00B8);
write_cmos_sensor(0x602A,0x1980);
write_cmos_sensor(0x6F12,0x003D);
write_cmos_sensor(0x602A,0x198A);
write_cmos_sensor(0x6F12,0x0014);
write_cmos_sensor(0x602A,0x1994);
write_cmos_sensor(0x6F12,0x001E);
write_cmos_sensor(0x602A,0x19AC);
write_cmos_sensor(0x6F12,0x0133);
write_cmos_sensor(0x602A,0x19B6);
write_cmos_sensor(0x6F12,0x0151);
write_cmos_sensor(0x602A,0x19C0);
write_cmos_sensor(0x6F12,0x0166);
write_cmos_sensor(0x602A,0x19CA);
write_cmos_sensor(0x6F12,0x0133);
write_cmos_sensor(0x602A,0x4ACC);
write_cmos_sensor(0x6F12,0x0172);
write_cmos_sensor(0x602A,0x4AD0);
write_cmos_sensor(0x6F12,0x0FEB);
write_cmos_sensor(0x6F12,0x0FF3);
write_cmos_sensor(0x6F12,0x1000);
write_cmos_sensor(0x6F12,0x1000);
write_cmos_sensor(0x602A,0x4ADA);
write_cmos_sensor(0x6F12,0x0FEB);
write_cmos_sensor(0x6F12,0x0FF3);
write_cmos_sensor(0x6F12,0x0FF3);
write_cmos_sensor(0x6F12,0x1000);
write_cmos_sensor(0x602A,0x4AE4);
write_cmos_sensor(0x6F12,0x0FD7);
write_cmos_sensor(0x6F12,0x0FCA);
write_cmos_sensor(0x6F12,0x0FAE);
write_cmos_sensor(0x6F12,0x0FAE);
write_cmos_sensor(0x602A,0x162C);
write_cmos_sensor(0x6F12,0x8080);
write_cmos_sensor(0x602A,0x164C);
write_cmos_sensor(0x6F12,0x5555);
write_cmos_sensor(0x602A,0x164E);
write_cmos_sensor(0x6F12,0x5500);
write_cmos_sensor(0x602A,0x166C);
write_cmos_sensor(0x6F12,0x4040);
write_cmos_sensor(0x602A,0x166E);
write_cmos_sensor(0x6F12,0x4040);
write_cmos_sensor(0x6028,0x4000);
write_cmos_sensor(0x3D6C,0x0080);
write_cmos_sensor(0x3D64,0x0105);
write_cmos_sensor(0x3D66,0x0105);
write_cmos_sensor(0x3D6A,0x0001);
write_cmos_sensor(0x3D70,0x0002);
write_cmos_sensor(0xF496,0x0048);
write_cmos_sensor(0xF470,0x0008);
write_cmos_sensor(0xF43A,0x0015);
write_cmos_sensor(0x3676,0x0008);
write_cmos_sensor(0x3678,0x0008);
write_cmos_sensor(0x3AC8,0x0A04);
write_cmos_sensor(0x3D34,0x0010);
write_cmos_sensor(0x30A0,0x0008);
write_cmos_sensor(0x0112,0x0A0A);
write_cmos_sensor(0x322E,0x000C);
write_cmos_sensor(0x3230,0x000C);
write_cmos_sensor(0x3236,0x000B);
write_cmos_sensor(0x3238,0x000B);
write_cmos_sensor(0x32A6,0x000C);
write_cmos_sensor(0x32A8,0x000C);
write_cmos_sensor(0x3616,0x0707);
write_cmos_sensor(0x3622,0x0808);
write_cmos_sensor(0x3626,0x0808);
write_cmos_sensor(0x32EE,0x0001);
write_cmos_sensor(0x32F0,0x0001);
write_cmos_sensor(0x32F6,0x0003);
write_cmos_sensor(0x32F8,0x0003);
write_cmos_sensor(0x361E,0x3030);
write_cmos_sensor(0x362A,0x0303);
write_cmos_sensor(0x3664,0x0019);
write_cmos_sensor(0x3666,0x030B);
write_cmos_sensor(0x3670,0x0001);
write_cmos_sensor(0x31B6,0x0008);
write_cmos_sensor(0xF442,0x44C6);
write_cmos_sensor(0xF408,0xFFF7);
write_cmos_sensor(0xF47C,0x001F);
write_cmos_sensor(0x347E,0x00CE);
write_cmos_sensor(0x3480,0x00CE);
write_cmos_sensor(0x3482,0x00E6);
write_cmos_sensor(0x3484,0x00E6);
write_cmos_sensor(0x3486,0x00FE);
write_cmos_sensor(0x3488,0x00FE);
write_cmos_sensor(0x348A,0x0116);
write_cmos_sensor(0x348C,0x0116);
write_cmos_sensor(0xF636,0x00D6);
write_cmos_sensor(0xF638,0x00DE);
write_cmos_sensor(0xF63A,0x00EE);
write_cmos_sensor(0xF63C,0x00F6);
write_cmos_sensor(0xF63E,0x0106);
write_cmos_sensor(0xF640,0x010E);
write_cmos_sensor(0xF4D0,0x0034);
write_cmos_sensor(0xF4D8,0x0034);

#else
write_cmos_sensor(0X6028, 0X2000 );
write_cmos_sensor(0X602A, 0X303C );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X0449 );
write_cmos_sensor(0X6F12, 0X0348 );
write_cmos_sensor(0X6F12, 0X044A );
write_cmos_sensor(0X6F12, 0X0860 );
write_cmos_sensor(0X6F12, 0X101A );
write_cmos_sensor(0X6F12, 0X8880 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X7DB9 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X34F4 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X1E80 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X4A00 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X70B5 );
write_cmos_sensor(0X6F12, 0XE74C );
write_cmos_sensor(0X6F12, 0X0E46 );
write_cmos_sensor(0X6F12, 0XA4F8 );
write_cmos_sensor(0X6F12, 0X0001 );
write_cmos_sensor(0X6F12, 0XE64D );
write_cmos_sensor(0X6F12, 0X15F8 );
write_cmos_sensor(0X6F12, 0X940F );
write_cmos_sensor(0X6F12, 0XA4F8 );
write_cmos_sensor(0X6F12, 0X0601 );
write_cmos_sensor(0X6F12, 0X6878 );
write_cmos_sensor(0X6F12, 0XA4F8 );
write_cmos_sensor(0X6F12, 0X0801 );
write_cmos_sensor(0X6F12, 0X1046 );
write_cmos_sensor(0X6F12, 0X04F5 );
write_cmos_sensor(0X6F12, 0XE874 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XEEF9 );
write_cmos_sensor(0X6F12, 0X3046 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XF0F9 );
write_cmos_sensor(0X6F12, 0XB5F8 );
write_cmos_sensor(0X6F12, 0X9A01 );
write_cmos_sensor(0X6F12, 0X6086 );
write_cmos_sensor(0X6F12, 0XB5F8 );
write_cmos_sensor(0X6F12, 0X9E01 );
write_cmos_sensor(0X6F12, 0XE086 );
write_cmos_sensor(0X6F12, 0XB5F8 );
write_cmos_sensor(0X6F12, 0XA001 );
write_cmos_sensor(0X6F12, 0X2087 );
write_cmos_sensor(0X6F12, 0XB5F8 );
write_cmos_sensor(0X6F12, 0XA601 );
write_cmos_sensor(0X6F12, 0XE087 );
write_cmos_sensor(0X6F12, 0X70BD );
write_cmos_sensor(0X6F12, 0X2DE9 );
write_cmos_sensor(0X6F12, 0XF05F );
write_cmos_sensor(0X6F12, 0XD74A );
write_cmos_sensor(0X6F12, 0XD849 );
write_cmos_sensor(0X6F12, 0XD54C );
write_cmos_sensor(0X6F12, 0X0646 );
write_cmos_sensor(0X6F12, 0X92F8 );
write_cmos_sensor(0X6F12, 0XEC30 );
write_cmos_sensor(0X6F12, 0X0F89 );
write_cmos_sensor(0X6F12, 0X94F8 );
write_cmos_sensor(0X6F12, 0XBE00 );
write_cmos_sensor(0X6F12, 0X4FF4 );
write_cmos_sensor(0X6F12, 0X8079 );
write_cmos_sensor(0X6F12, 0X13B1 );
write_cmos_sensor(0X6F12, 0XA8B9 );
write_cmos_sensor(0X6F12, 0X4F46 );
write_cmos_sensor(0X6F12, 0X13E0 );
write_cmos_sensor(0X6F12, 0XD24B );
write_cmos_sensor(0X6F12, 0X0968 );
write_cmos_sensor(0X6F12, 0X92F8 );
write_cmos_sensor(0X6F12, 0XED20 );
write_cmos_sensor(0X6F12, 0X1B6D );
write_cmos_sensor(0X6F12, 0XB1FB );
write_cmos_sensor(0X6F12, 0XF3F1 );
write_cmos_sensor(0X6F12, 0X7943 );
write_cmos_sensor(0X6F12, 0XD140 );
write_cmos_sensor(0X6F12, 0X8BB2 );
write_cmos_sensor(0X6F12, 0X30B1 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0X4FF4 );
write_cmos_sensor(0X6F12, 0X8051 );
write_cmos_sensor(0X6F12, 0X1846 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XC6F9 );
write_cmos_sensor(0X6F12, 0X00E0 );
write_cmos_sensor(0X6F12, 0X4846 );
write_cmos_sensor(0X6F12, 0X87B2 );
write_cmos_sensor(0X6F12, 0X3088 );
write_cmos_sensor(0X6F12, 0X18B1 );
write_cmos_sensor(0X6F12, 0X0121 );
write_cmos_sensor(0X6F12, 0XC748 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XC2F9 );
write_cmos_sensor(0X6F12, 0X0021 );
write_cmos_sensor(0X6F12, 0XC648 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XBEF9 );
write_cmos_sensor(0X6F12, 0XDFF8 );
write_cmos_sensor(0X6F12, 0X18A3 );
write_cmos_sensor(0X6F12, 0X0025 );
write_cmos_sensor(0X6F12, 0XA046 );
write_cmos_sensor(0X6F12, 0X4FF0 );
write_cmos_sensor(0X6F12, 0X010B );
write_cmos_sensor(0X6F12, 0X3088 );
write_cmos_sensor(0X6F12, 0X70B1 );
write_cmos_sensor(0X6F12, 0X7088 );
write_cmos_sensor(0X6F12, 0X38B1 );
write_cmos_sensor(0X6F12, 0XF089 );
write_cmos_sensor(0X6F12, 0X06E0 );
write_cmos_sensor(0X6F12, 0X2A46 );
write_cmos_sensor(0X6F12, 0XBD49 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XB3F9 );
write_cmos_sensor(0X6F12, 0X0446 );
write_cmos_sensor(0X6F12, 0X05E0 );
write_cmos_sensor(0X6F12, 0XB089 );
write_cmos_sensor(0X6F12, 0X0028 );
write_cmos_sensor(0X6F12, 0XF6D1 );
write_cmos_sensor(0X6F12, 0X3846 );
write_cmos_sensor(0X6F12, 0XF4E7 );
write_cmos_sensor(0X6F12, 0X4C46 );
write_cmos_sensor(0X6F12, 0X98F8 );
write_cmos_sensor(0X6F12, 0XBE00 );
write_cmos_sensor(0X6F12, 0X28B1 );
write_cmos_sensor(0X6F12, 0X2A46 );
write_cmos_sensor(0X6F12, 0XB749 );
write_cmos_sensor(0X6F12, 0X3846 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XA3F9 );
write_cmos_sensor(0X6F12, 0X00E0 );
write_cmos_sensor(0X6F12, 0X4846 );
write_cmos_sensor(0X6F12, 0XAAF8 );
write_cmos_sensor(0X6F12, 0X5802 );
write_cmos_sensor(0X6F12, 0X4443 );
write_cmos_sensor(0X6F12, 0X98F8 );
write_cmos_sensor(0X6F12, 0X2B02 );
write_cmos_sensor(0X6F12, 0X98F8 );
write_cmos_sensor(0X6F12, 0X2922 );
write_cmos_sensor(0X6F12, 0XC440 );
write_cmos_sensor(0X6F12, 0X0244 );
write_cmos_sensor(0X6F12, 0X0BFA );
write_cmos_sensor(0X6F12, 0X02F0 );
write_cmos_sensor(0X6F12, 0X421E );
write_cmos_sensor(0X6F12, 0XA242 );
write_cmos_sensor(0X6F12, 0X01DA );
write_cmos_sensor(0X6F12, 0X1346 );
write_cmos_sensor(0X6F12, 0X00E0 );
write_cmos_sensor(0X6F12, 0X2346 );
write_cmos_sensor(0X6F12, 0X002B );
write_cmos_sensor(0X6F12, 0X01DA );
write_cmos_sensor(0X6F12, 0X0024 );
write_cmos_sensor(0X6F12, 0X02E0 );
write_cmos_sensor(0X6F12, 0XA242 );
write_cmos_sensor(0X6F12, 0X00DA );
write_cmos_sensor(0X6F12, 0X1446 );
write_cmos_sensor(0X6F12, 0X0AEB );
write_cmos_sensor(0X6F12, 0X4500 );
write_cmos_sensor(0X6F12, 0X6D1C );
write_cmos_sensor(0X6F12, 0XA0F8 );
write_cmos_sensor(0X6F12, 0X5A42 );
write_cmos_sensor(0X6F12, 0X042D );
write_cmos_sensor(0X6F12, 0XC4DB );
write_cmos_sensor(0X6F12, 0XBDE8 );
write_cmos_sensor(0X6F12, 0XF09F );
write_cmos_sensor(0X6F12, 0XA14A );
write_cmos_sensor(0X6F12, 0X92F8 );
write_cmos_sensor(0X6F12, 0XB330 );
write_cmos_sensor(0X6F12, 0X53B1 );
write_cmos_sensor(0X6F12, 0X92F8 );
write_cmos_sensor(0X6F12, 0XFD20 );
write_cmos_sensor(0X6F12, 0X0420 );
write_cmos_sensor(0X6F12, 0X52B1 );
write_cmos_sensor(0X6F12, 0XA14A );
write_cmos_sensor(0X6F12, 0X92F8 );
write_cmos_sensor(0X6F12, 0X3221 );
write_cmos_sensor(0X6F12, 0X012A );
write_cmos_sensor(0X6F12, 0X05D1 );
write_cmos_sensor(0X6F12, 0X0220 );
write_cmos_sensor(0X6F12, 0X03E0 );
write_cmos_sensor(0X6F12, 0X5379 );
write_cmos_sensor(0X6F12, 0X2BB9 );
write_cmos_sensor(0X6F12, 0X20B1 );
write_cmos_sensor(0X6F12, 0X0020 );
write_cmos_sensor(0X6F12, 0X8842 );
write_cmos_sensor(0X6F12, 0X00D8 );
write_cmos_sensor(0X6F12, 0X0846 );
write_cmos_sensor(0X6F12, 0X7047 );
write_cmos_sensor(0X6F12, 0X1078 );
write_cmos_sensor(0X6F12, 0XF9E7 );
write_cmos_sensor(0X6F12, 0XFEB5 );
write_cmos_sensor(0X6F12, 0X0646 );
write_cmos_sensor(0X6F12, 0X9948 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0XC068 );
write_cmos_sensor(0X6F12, 0X84B2 );
write_cmos_sensor(0X6F12, 0X050C );
write_cmos_sensor(0X6F12, 0X2146 );
write_cmos_sensor(0X6F12, 0X2846 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X61F9 );
write_cmos_sensor(0X6F12, 0X3046 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X63F9 );
write_cmos_sensor(0X6F12, 0X8E4E );
write_cmos_sensor(0X6F12, 0X0020 );
write_cmos_sensor(0X6F12, 0X8DF8 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X96F8 );
write_cmos_sensor(0X6F12, 0XFD00 );
write_cmos_sensor(0X6F12, 0X8DF8 );
write_cmos_sensor(0X6F12, 0X0100 );
write_cmos_sensor(0X6F12, 0X96F8 );
write_cmos_sensor(0X6F12, 0XB310 );
write_cmos_sensor(0X6F12, 0X8D4F );
write_cmos_sensor(0X6F12, 0X19B1 );
write_cmos_sensor(0X6F12, 0X97F8 );
write_cmos_sensor(0X6F12, 0X3221 );
write_cmos_sensor(0X6F12, 0X012A );
write_cmos_sensor(0X6F12, 0X0FD0 );
write_cmos_sensor(0X6F12, 0X0023 );
write_cmos_sensor(0X6F12, 0X0122 );
write_cmos_sensor(0X6F12, 0X0292 );
write_cmos_sensor(0X6F12, 0X0192 );
write_cmos_sensor(0X6F12, 0X21B1 );
write_cmos_sensor(0X6F12, 0X18B1 );
write_cmos_sensor(0X6F12, 0X97F8 );
write_cmos_sensor(0X6F12, 0X3201 );
write_cmos_sensor(0X6F12, 0X0128 );
write_cmos_sensor(0X6F12, 0X07D0 );
write_cmos_sensor(0X6F12, 0X96F8 );
write_cmos_sensor(0X6F12, 0X6801 );
write_cmos_sensor(0X6F12, 0X4208 );
write_cmos_sensor(0X6F12, 0X33B1 );
write_cmos_sensor(0X6F12, 0X0020 );
write_cmos_sensor(0X6F12, 0X06E0 );
write_cmos_sensor(0X6F12, 0X0123 );
write_cmos_sensor(0X6F12, 0XEEE7 );
write_cmos_sensor(0X6F12, 0X96F8 );
write_cmos_sensor(0X6F12, 0X6821 );
write_cmos_sensor(0X6F12, 0XF7E7 );
write_cmos_sensor(0X6F12, 0X96F8 );
write_cmos_sensor(0X6F12, 0XA400 );
write_cmos_sensor(0X6F12, 0X4100 );
write_cmos_sensor(0X6F12, 0X002A );
write_cmos_sensor(0X6F12, 0X02D0 );
write_cmos_sensor(0X6F12, 0X4FF0 );
write_cmos_sensor(0X6F12, 0X0200 );
write_cmos_sensor(0X6F12, 0X01E0 );
write_cmos_sensor(0X6F12, 0X4FF0 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X41EA );
write_cmos_sensor(0X6F12, 0X4020 );
write_cmos_sensor(0X6F12, 0X7B49 );
write_cmos_sensor(0X6F12, 0XA1F8 );
write_cmos_sensor(0X6F12, 0X5201 );
write_cmos_sensor(0X6F12, 0X07D0 );
write_cmos_sensor(0X6F12, 0X7A48 );
write_cmos_sensor(0X6F12, 0X6B46 );
write_cmos_sensor(0X6F12, 0XB0F8 );
write_cmos_sensor(0X6F12, 0X4810 );
write_cmos_sensor(0X6F12, 0X4FF2 );
write_cmos_sensor(0X6F12, 0X5010 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X2BF9 );
write_cmos_sensor(0X6F12, 0X0122 );
write_cmos_sensor(0X6F12, 0X2146 );
write_cmos_sensor(0X6F12, 0X2846 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X1CF9 );
write_cmos_sensor(0X6F12, 0XFEBD );
write_cmos_sensor(0X6F12, 0X2DE9 );
write_cmos_sensor(0X6F12, 0XF047 );
write_cmos_sensor(0X6F12, 0X0546 );
write_cmos_sensor(0X6F12, 0X7048 );
write_cmos_sensor(0X6F12, 0X9046 );
write_cmos_sensor(0X6F12, 0X8946 );
write_cmos_sensor(0X6F12, 0X8068 );
write_cmos_sensor(0X6F12, 0X1C46 );
write_cmos_sensor(0X6F12, 0X86B2 );
write_cmos_sensor(0X6F12, 0X070C );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0X3146 );
write_cmos_sensor(0X6F12, 0X3846 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X0CF9 );
write_cmos_sensor(0X6F12, 0X2346 );
write_cmos_sensor(0X6F12, 0X4246 );
write_cmos_sensor(0X6F12, 0X4946 );
write_cmos_sensor(0X6F12, 0X2846 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X10F9 );
write_cmos_sensor(0X6F12, 0X6248 );
write_cmos_sensor(0X6F12, 0X90F8 );
write_cmos_sensor(0X6F12, 0XB310 );
write_cmos_sensor(0X6F12, 0XD1B1 );
write_cmos_sensor(0X6F12, 0X90F8 );
write_cmos_sensor(0X6F12, 0XFD00 );
write_cmos_sensor(0X6F12, 0XB8B1 );
write_cmos_sensor(0X6F12, 0X0020 );
write_cmos_sensor(0X6F12, 0XA5F5 );
write_cmos_sensor(0X6F12, 0X7141 );
write_cmos_sensor(0X6F12, 0X05F1 );
write_cmos_sensor(0X6F12, 0X8044 );
write_cmos_sensor(0X6F12, 0X9039 );
write_cmos_sensor(0X6F12, 0X02D0 );
write_cmos_sensor(0X6F12, 0X4031 );
write_cmos_sensor(0X6F12, 0X02D0 );
write_cmos_sensor(0X6F12, 0X0DE0 );
write_cmos_sensor(0X6F12, 0XA081 );
write_cmos_sensor(0X6F12, 0X0BE0 );
write_cmos_sensor(0X6F12, 0XA081 );
write_cmos_sensor(0X6F12, 0X5C48 );
write_cmos_sensor(0X6F12, 0X90F8 );
write_cmos_sensor(0X6F12, 0X3201 );
write_cmos_sensor(0X6F12, 0X0128 );
write_cmos_sensor(0X6F12, 0X05D1 );
write_cmos_sensor(0X6F12, 0X0220 );
write_cmos_sensor(0X6F12, 0XA080 );
write_cmos_sensor(0X6F12, 0X2D20 );
write_cmos_sensor(0X6F12, 0X2081 );
write_cmos_sensor(0X6F12, 0X2E20 );
write_cmos_sensor(0X6F12, 0X6081 );
write_cmos_sensor(0X6F12, 0X3146 );
write_cmos_sensor(0X6F12, 0X3846 );
write_cmos_sensor(0X6F12, 0XBDE8 );
write_cmos_sensor(0X6F12, 0XF047 );
write_cmos_sensor(0X6F12, 0X0122 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XE0B8 );
write_cmos_sensor(0X6F12, 0X2DE9 );
write_cmos_sensor(0X6F12, 0XF041 );
write_cmos_sensor(0X6F12, 0X0546 );
write_cmos_sensor(0X6F12, 0X4D48 );
write_cmos_sensor(0X6F12, 0X4C4B );
write_cmos_sensor(0X6F12, 0X0C46 );
write_cmos_sensor(0X6F12, 0X90F8 );
write_cmos_sensor(0X6F12, 0X5C11 );
write_cmos_sensor(0X6F12, 0X93F8 );
write_cmos_sensor(0X6F12, 0X4460 );
write_cmos_sensor(0X6F12, 0X90F8 );
write_cmos_sensor(0X6F12, 0XFD00 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0X26B1 );
write_cmos_sensor(0X6F12, 0X09B1 );
write_cmos_sensor(0X6F12, 0X0222 );
write_cmos_sensor(0X6F12, 0X01E0 );
write_cmos_sensor(0X6F12, 0X00B1 );
write_cmos_sensor(0X6F12, 0X0122 );
write_cmos_sensor(0X6F12, 0X4B4E );
write_cmos_sensor(0X6F12, 0XA6F8 );
write_cmos_sensor(0X6F12, 0X4622 );
write_cmos_sensor(0X6F12, 0X93F8 );
write_cmos_sensor(0X6F12, 0X2030 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0X23B1 );
write_cmos_sensor(0X6F12, 0X09B1 );
write_cmos_sensor(0X6F12, 0X0222 );
write_cmos_sensor(0X6F12, 0X01E0 );
write_cmos_sensor(0X6F12, 0X00B1 );
write_cmos_sensor(0X6F12, 0X0122 );
write_cmos_sensor(0X6F12, 0XCAB1 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0X0221 );
write_cmos_sensor(0X6F12, 0X47F2 );
write_cmos_sensor(0X6F12, 0XC600 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XBAF8 );
write_cmos_sensor(0X6F12, 0X4148 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0X0069 );
write_cmos_sensor(0X6F12, 0X87B2 );
write_cmos_sensor(0X6F12, 0X060C );
write_cmos_sensor(0X6F12, 0X3946 );
write_cmos_sensor(0X6F12, 0X3046 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XB1F8 );
write_cmos_sensor(0X6F12, 0X2146 );
write_cmos_sensor(0X6F12, 0X2846 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XBCF8 );
write_cmos_sensor(0X6F12, 0X3946 );
write_cmos_sensor(0X6F12, 0X3046 );
write_cmos_sensor(0X6F12, 0XBDE8 );
write_cmos_sensor(0X6F12, 0XF041 );
write_cmos_sensor(0X6F12, 0X0122 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XA6B8 );
write_cmos_sensor(0X6F12, 0X0122 );
write_cmos_sensor(0X6F12, 0XE4E7 );
write_cmos_sensor(0X6F12, 0X70B5 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0XAFF2 );
write_cmos_sensor(0X6F12, 0XEF21 );
write_cmos_sensor(0X6F12, 0X3748 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XB1F8 );
write_cmos_sensor(0X6F12, 0X324D );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0XAFF2 );
write_cmos_sensor(0X6F12, 0XBB21 );
write_cmos_sensor(0X6F12, 0X2860 );
write_cmos_sensor(0X6F12, 0X3448 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XA9F8 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0XAFF2 );
write_cmos_sensor(0X6F12, 0XE311 );
write_cmos_sensor(0X6F12, 0X6860 );
write_cmos_sensor(0X6F12, 0X3148 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0XA2F8 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0XAFF2 );
write_cmos_sensor(0X6F12, 0XC111 );
write_cmos_sensor(0X6F12, 0X2F48 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X9CF8 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0XAFF2 );
write_cmos_sensor(0X6F12, 0X2B11 );
write_cmos_sensor(0X6F12, 0XE860 );
write_cmos_sensor(0X6F12, 0X2D48 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X95F8 );
write_cmos_sensor(0X6F12, 0X2C4C );
write_cmos_sensor(0X6F12, 0XA860 );
write_cmos_sensor(0X6F12, 0X43F2 );
write_cmos_sensor(0X6F12, 0XAC00 );
write_cmos_sensor(0X6F12, 0XE18C );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X93F8 );
write_cmos_sensor(0X6F12, 0XE08C );
write_cmos_sensor(0X6F12, 0X2A4E );
write_cmos_sensor(0X6F12, 0X2949 );
write_cmos_sensor(0X6F12, 0X46F8 );
write_cmos_sensor(0X6F12, 0X2010 );
write_cmos_sensor(0X6F12, 0X401C );
write_cmos_sensor(0X6F12, 0X81B2 );
write_cmos_sensor(0X6F12, 0XE184 );
write_cmos_sensor(0X6F12, 0X43F2 );
write_cmos_sensor(0X6F12, 0XB000 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X87F8 );
write_cmos_sensor(0X6F12, 0XE08C );
write_cmos_sensor(0X6F12, 0X2549 );
write_cmos_sensor(0X6F12, 0X46F8 );
write_cmos_sensor(0X6F12, 0X2010 );
write_cmos_sensor(0X6F12, 0X401C );
write_cmos_sensor(0X6F12, 0X81B2 );
write_cmos_sensor(0X6F12, 0XE184 );
write_cmos_sensor(0X6F12, 0X43F2 );
write_cmos_sensor(0X6F12, 0X1840 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X7CF8 );
write_cmos_sensor(0X6F12, 0XE08C );
write_cmos_sensor(0X6F12, 0X2149 );
write_cmos_sensor(0X6F12, 0X46F8 );
write_cmos_sensor(0X6F12, 0X2010 );
write_cmos_sensor(0X6F12, 0X401C );
write_cmos_sensor(0X6F12, 0X81B2 );
write_cmos_sensor(0X6F12, 0XE184 );
write_cmos_sensor(0X6F12, 0X43F2 );
write_cmos_sensor(0X6F12, 0X1C40 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X71F8 );
write_cmos_sensor(0X6F12, 0XE08C );
write_cmos_sensor(0X6F12, 0X1C49 );
write_cmos_sensor(0X6F12, 0X46F8 );
write_cmos_sensor(0X6F12, 0X2010 );
write_cmos_sensor(0X6F12, 0X401C );
write_cmos_sensor(0X6F12, 0XE084 );
write_cmos_sensor(0X6F12, 0X0022 );
write_cmos_sensor(0X6F12, 0XAFF2 );
write_cmos_sensor(0X6F12, 0X2111 );
write_cmos_sensor(0X6F12, 0X1948 );
write_cmos_sensor(0X6F12, 0X00F0 );
write_cmos_sensor(0X6F12, 0X60F8 );
write_cmos_sensor(0X6F12, 0X2861 );
write_cmos_sensor(0X6F12, 0X70BD );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X4000 );
write_cmos_sensor(0X6F12, 0XE000 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X1350 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X4A00 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X2970 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X1EB0 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X1BAA );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X1B58 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X2530 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X1CC0 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X34E0 );
write_cmos_sensor(0X6F12, 0X4000 );
write_cmos_sensor(0X6F12, 0XF000 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X05C0 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X333B );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X3757 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X7BED );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0XAC61 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0XA831 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X1E80 );
write_cmos_sensor(0X6F12, 0X18BF );
write_cmos_sensor(0X6F12, 0X06F2 );
write_cmos_sensor(0X6F12, 0X2000 );
write_cmos_sensor(0X6F12, 0X2E10 );
write_cmos_sensor(0X6F12, 0X0144 );
write_cmos_sensor(0X6F12, 0X04F5 );
write_cmos_sensor(0X6F12, 0X18BF );
write_cmos_sensor(0X6F12, 0X0AF2 );
write_cmos_sensor(0X6F12, 0X0148 );
write_cmos_sensor(0X6F12, 0X1FFA );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X79D9 );
write_cmos_sensor(0X6F12, 0X43F2 );
write_cmos_sensor(0X6F12, 0X1B3C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X43F2 );
write_cmos_sensor(0X6F12, 0XC32C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X40F2 );
write_cmos_sensor(0X6F12, 0XF76C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X43F2 );
write_cmos_sensor(0X6F12, 0XBD6C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X43F2 );
write_cmos_sensor(0X6F12, 0X476C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X40F2 );
write_cmos_sensor(0X6F12, 0X777C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X4AF6 );
write_cmos_sensor(0X6F12, 0X614C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X4AF6 );
write_cmos_sensor(0X6F12, 0X310C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X47F6 );
write_cmos_sensor(0X6F12, 0XD91C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X4DF2 );
write_cmos_sensor(0X6F12, 0X536C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X4DF2 );
write_cmos_sensor(0X6F12, 0XF35C );
write_cmos_sensor(0X6F12, 0XC0F2 );
write_cmos_sensor(0X6F12, 0X000C );
write_cmos_sensor(0X6F12, 0X6047 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X30D3 );
write_cmos_sensor(0X6F12, 0X0244 );
write_cmos_sensor(0X6F12, 0X0000 );
write_cmos_sensor(0X6F12, 0X060D );
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
#endif
}	/*	sensor_init  */


static void preview_setting(void)
{
  LOG_INF("E\n");
write_cmos_sensor_byte(0x0100, 0x00);
    check_stremoff();
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0008);
write_cmos_sensor(0X0346, 0X0008);
write_cmos_sensor(0X0348, 0X1077);
write_cmos_sensor(0X034A, 0X0C37);
write_cmos_sensor(0X034C, 0X0838);
write_cmos_sensor(0X034E, 0X0618);
write_cmos_sensor(0X0340, 0X0d49);
write_cmos_sensor(0X0342, 0X1260);
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
#ifdef VCPDAF
write_cmos_sensor(0X0B0E, 0X0100); 
#else 
write_cmos_sensor(0X0B0E, 0X0000);
#endif 
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0001);
write_cmos_sensor(0X3D06, 0X0010);
#ifdef VCPDAF_PRE
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0001);
write_cmos_sensor(0X317A, 0X0115);
#else 
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
#endif 
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
write_cmos_sensor_byte(0x0100, 0x01);

}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	if (currefps == 300) {
write_cmos_sensor_byte(0x0100, 0x00);
    check_stremoff();
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0008);
write_cmos_sensor(0X0346, 0X0008);
write_cmos_sensor(0X0348, 0X1077);
write_cmos_sensor(0X034A, 0X0C37);
write_cmos_sensor(0X034C, 0X1070);
write_cmos_sensor(0X034E, 0X0C30);
write_cmos_sensor(0X0340, 0X0d49);
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
#ifdef VCPDAF
write_cmos_sensor(0X0B0E, 0X0100); 
#else 
write_cmos_sensor(0X0B0E, 0X0000);
#endif 
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0001);
write_cmos_sensor(0X3D06, 0X0010);
#ifdef VCPDAF
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0001);
write_cmos_sensor(0X317A, 0X0115);
#else 
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
#endif 
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
write_cmos_sensor_byte(0x0100, 0x01);

	}
	else if (currefps == 240) {
write_cmos_sensor_byte(0x0100, 0x00);
    check_stremoff();
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0008);
write_cmos_sensor(0X0346, 0X0008);
write_cmos_sensor(0X0348, 0X1077);
write_cmos_sensor(0X034A, 0X0C37);
write_cmos_sensor(0X034C, 0X1070);
write_cmos_sensor(0X034E, 0X0C30);
write_cmos_sensor(0X0340, 0X0d49);
write_cmos_sensor(0X0342, 0X16FE);
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
#ifdef VCPDAF
write_cmos_sensor(0X0B0E, 0X0100); 
#else 
write_cmos_sensor(0X0B0E, 0X0000);
#endif  
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0001);
write_cmos_sensor(0X3D06, 0X0010);
#ifdef VCPDAF
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0001);
write_cmos_sensor(0X317A, 0X0115);
#else 
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
#endif 
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
write_cmos_sensor(0X030E, 0X0050);
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
write_cmos_sensor_byte(0x0100, 0x01);

	}
	else if (currefps == 150) {
//PIP 15fps settings,??Full 30fps
//    -VT : 560-> 400M
//    -Frame length: 3206-> 4589
//   -Linelength: 5808??
//
//$MV1[MCLK:24,Width:4208,Height:3120,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:1124,pvi_pclk_inverse:0]
		LOG_INF("else if (currefps == 150)\n");
write_cmos_sensor_byte(0x0100, 0x00);
    check_stremoff();
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0008);
write_cmos_sensor(0X0346, 0X0008);
write_cmos_sensor(0X0348, 0X1077);
write_cmos_sensor(0X034A, 0X0C37);
write_cmos_sensor(0X034C, 0X1070);
write_cmos_sensor(0X034E, 0X0C30);
write_cmos_sensor(0X0340, 0X0d49);
write_cmos_sensor(0X0342, 0X24c0);
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
#ifdef VCPDAF
write_cmos_sensor(0X0B0E, 0X0100); 
#else 
write_cmos_sensor(0X0B0E, 0X0000);
#endif  
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0001);
write_cmos_sensor(0X3D06, 0X0010);
#ifdef VCPDAF
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0001);
write_cmos_sensor(0X317A, 0X0115);
#else 
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
#endif 
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
write_cmos_sensor(0X030E, 0X0037);
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
write_cmos_sensor_byte(0x0100, 0x01);
	}
	else { //default fps =15
//PIP 15fps settings, v.s. Full 30fps
//    -VT : 560-> 400M
//    -Frame length: 3206-> 4589
//   -Linelength: 5808 same
//
//$MV1[MCLK:24,Width:4208,Height:3120,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:1124,pvi_pclk_inverse:0]
		LOG_INF("else  150fps\n");
write_cmos_sensor_byte(0x0100, 0x00);
    check_stremoff();
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0008);
write_cmos_sensor(0X0346, 0X0008);
write_cmos_sensor(0X0348, 0X1077);
write_cmos_sensor(0X034A, 0X0C37);
write_cmos_sensor(0X034C, 0X1070);
write_cmos_sensor(0X034E, 0X0C30);
write_cmos_sensor(0X0340, 0X0d49);
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
#ifdef VCPDAF
write_cmos_sensor(0X0B0E, 0X0100); 
#else 
write_cmos_sensor(0X0B0E, 0X0000);
#endif  
write_cmos_sensor(0X3090, 0X0904);
write_cmos_sensor(0X3058, 0X0001);
write_cmos_sensor(0X3D06, 0X0010);
#ifdef VCPDAF
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0000);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0001);
write_cmos_sensor(0X317A, 0X0115);
#else 
write_cmos_sensor(0X6028, 0X2000);
write_cmos_sensor(0X602A, 0X19E0);
write_cmos_sensor(0X6F12, 0X0001);
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X30E2, 0X0000);
write_cmos_sensor(0X317A, 0X0007);
#endif 
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
write_cmos_sensor_byte(0x0100, 0x00);
    check_stremoff();
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
write_cmos_sensor(0X030E, 0X0062);
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
write_cmos_sensor_byte(0x0100, 0x01);

}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
write_cmos_sensor_byte(0x0100, 0x00);
    check_stremoff();
write_cmos_sensor(0X6028, 0X4000);
write_cmos_sensor(0X6214, 0X7971);
write_cmos_sensor(0X6218, 0X7150);
write_cmos_sensor(0X0344, 0X0018);
write_cmos_sensor(0X0346, 0X0182);
write_cmos_sensor(0X0348, 0X1067);
write_cmos_sensor(0X034A, 0X0ABD);
write_cmos_sensor(0X034C, 0X0570);
write_cmos_sensor(0X034E, 0X0314);
write_cmos_sensor(0X0340, 0X0D49);
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
write_cmos_sensor(0X030E, 0X0037);
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
write_cmos_sensor_byte(0x0100, 0x01);
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
	#ifdef VCPDAF
	sensor_info->PDAF_Support = 2;
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

static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor_byte(0x0100, 0x01);
	else
		write_cmos_sensor_byte(0x0100, 0x00);
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
	SENSOR_VC_INFO_STRUCT *pvcinfo;    
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
	SET_PD_BLOCK_INFO_T *PDAFinfo;

    /*LOG_INF("feature_id = %d\n", feature_id);*/
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
			/*temp soliution+*/
			break;
		case SENSOR_FEATURE_GET_VC_INFO:
			LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
			pvcinfo = (SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
			switch (*feature_data_32) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[1],sizeof(SENSOR_VC_INFO_STRUCT));
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[2],sizeof(SENSOR_VC_INFO_STRUCT));
				break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[0],sizeof(SENSOR_VC_INFO_STRUCT));
				break;
			}
			break;			  
		/******************** PDAF END	 <<< *********/
		case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
			LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
			streaming_control(KAL_FALSE);
			break;
		case SENSOR_FEATURE_SET_STREAMING_RESUME:
			LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n", *feature_data);
			if (*feature_data != 0)
				set_shutter(*feature_data);
			streaming_control(KAL_TRUE);
			break;
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



