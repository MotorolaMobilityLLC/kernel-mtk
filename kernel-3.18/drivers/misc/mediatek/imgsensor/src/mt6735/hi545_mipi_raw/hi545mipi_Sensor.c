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
 *     HI545mipi_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
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
//#include <asm/system.h>
/*#include <linux/xlog.h>*/

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "hi545mipi_Sensor.h"

extern void kdSetI2CSpeed(u32 i2cSpeed);  

/****************************Modify Following Strings for Debug****************************/
#define DEBUG_TAG   "[V28]"
#define PFX "[HI545]"DEBUG_TAG
#define LOG_1 LOG_INF("HI545,MIPI 2LANE\n")
#define LOG_2 LOG_INF("preview 1296*972@30fps; video 1296*972@30fps; capture 5M@30fps\n")
/****************************   Modify end    *******************************************/

#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)

#define USE_I2C_400K    1

static void capture_setting(kal_uint16 currefps);

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static imgsensor_info_struct imgsensor_info = {
    .sensor_id = HI545MIPI_SENSOR_ID,        //record sensor id defined in Kd_imgsensor.h

    .checksum_value = 0x55e2a82f,        //checksum value for Camera Auto Test, 0x40aeb1f5

	.pre = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength = 2880,				//record different mode's linelength
		.framelength = 2017,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2592,		//record different mode's width of grabwindow
		.grabwindow_height = 1944,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 176000000,
		.linelength = 2880,
		.framelength = 2017,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 176000000,
		.linelength = 2880,
		.framelength = 2017,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2592,
		.grabwindow_height = 1944,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,	
	},
	.normal_video = {
		.pclk = 176000000,
		.linelength = 2880,
		.framelength = 2017,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1296,
		.grabwindow_height = 972,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 176000000,
		.linelength = 2880,
		.framelength = 506,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 640,
		.grabwindow_height = 480,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 176000000,
		.linelength = 2880,
		.framelength = 2017,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},

    .margin = 4,            //sensor framelength & shutter margin
    .min_shutter = 1,        //min shutter
    .max_frame_length = 0xffff,//max framelength by sensor register's limitation
    .ae_shut_delay_frame = 0,    //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
    .ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
    .ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
    .ihdr_support = 0,      //1, support; 0,not support
    .ihdr_le_firstline = 0,  //1,le first ; 0, se first
    .sensor_mode_num = 5,      //support sensor mode num

    .cap_delay_frame = 1,        //enter capture delay frame num
    .pre_delay_frame = 1,         //enter preview delay frame num
    .video_delay_frame = 4,        //enter video delay frame num
    .hs_video_delay_frame = 3,    //enter high speed video  delay frame num
    .slim_video_delay_frame = 3,//enter slim video delay frame num

    .isp_driving_current = ISP_DRIVING_6MA, //mclk driving current
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,//sensor output first pixel color
    .mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
    .mipi_lane_num = SENSOR_MIPI_2_LANE,//mipi lane num
    .i2c_addr_table = {0x40, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
};


static imgsensor_struct imgsensor = {
    .mirror = IMAGE_V_MIRROR,                //mirrorflip information
    .sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
    .shutter = 0x3D0,                    //current shutter
    .gain = 0x100,                        //current gain
    .dummy_pixel = 0,                    //current dummypixel
    .dummy_line = 0,                    //current dummyline
    .current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
    .autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
    .test_pattern = KAL_FALSE,        //test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
    .current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
    .ihdr_en = 0, //sensor need support LE, SE with HDR feature
    .i2c_write_id = 0x40,//record current sensor's i2c write id
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{{ 2592, 1944,	  0,	0, 2592, 1944, 1296,   972, 0000, 0000, 1296,   972,	  0,	0, 1296,   972}, // Preview 
 { 2592, 1944,	  0,	0, 2592, 1944, 2592,  1944, 0000, 0000, 2592,  1944,	  0,	0, 2592,  1944}, // capture 
 { 2592, 1944,	  0,	0, 2592, 1944, 1296,   972, 0000, 0000, 1296,   972,	  0,	0, 1296,   972}, // video 
 { 2592, 1944,	  0,	0, 2592, 1944, 648,   488, 0000, 0000, 648,   488,	  0,	0, 640,   480}, //hight speed video 
 { 2592, 1944,	  0,	0, 2592, 1944, 1296,   972, 0000, 0000, 1296,   972,	  0,	0, 1296,   972}};// slim video 



static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;

    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

    return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	iWriteReg((u16)addr, (u32)para, 2, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor(0x0006, imgsensor.frame_length);  
	write_cmos_sensor(0x0008, imgsensor.line_length);

}    /*    set_dummy  */

static kal_uint32 return_sensor_id(void)
{
    return ((read_cmos_sensor(0x0F17) << 8) | read_cmos_sensor(0x0F16));
}
static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
    kal_uint32 frame_length = imgsensor.frame_length;
    //unsigned long flags;

    LOG_INF("framerate = %d, min framelength should enable = %d\n", framerate,min_framelength_en);

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
}    /*    set_max_framerate  */



/*************************************************************************
* FUNCTION
*    set_shutter
*
* DESCRIPTION
*    This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*    iShutter : exposured lines
*
* RETURNS
*    None
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
       		write_cmos_sensor(0x0006, imgsensor.frame_length);
        }
    } else {
        // Extend frame length
			write_cmos_sensor(0x0006, imgsensor.frame_length);

    }

    // Update Shutter
	write_cmos_sensor(0x0046, 0x0100);		
	write_cmos_sensor(0x0004, shutter);
	write_cmos_sensor(0x0046, 0x0000);	

    LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}    /*    set_shutter */



static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;
	
	reg_gain = gain / 4 - 16;
	
	return (kal_uint16)reg_gain;

}

/*************************************************************************
* FUNCTION
*    set_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    iGain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
    kal_uint16 reg_gain;

    /* 0x350A[0:1], 0x350B[0:7] AGC real gain */
    /* [0:3] = N meams N /16 X    */
    /* [4:9] = M meams M X         */
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
    LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x0046, 0x0100);	
	write_cmos_sensor(0x003a, reg_gain << 8);  
	write_cmos_sensor(0x0046, 0x0000);	

    return gain;
}    /*    set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
    LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
}



static void set_mirror_flip(kal_uint8 image_mirror)
{
    LOG_INF("image_mirror = %d\n", image_mirror);

	switch (image_mirror) {
		case IMAGE_NORMAL:
			write_cmos_sensor(0x0000, 0x0000);
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0000, 0x0200);
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0000, 0x0100);
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x0000, 0x0300);
			break;
		default:
			LOG_INF("Error image_mirror setting");
	}

}

/*************************************************************************
* FUNCTION
*    night_mode
*
* DESCRIPTION
*    This function night mode of sensor.
*
* PARAMETERS
*    bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}    /*    night_mode    */

static void sensor_init(void)
{
	LOG_INF("E");
	//////////////////////////////////////////////////////////////////////////
	//<<<<<<<<<  Sensor Information  >>>>>>>>>>///////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//
	//	Sensor        	: Hi-545
	//
	//	Initial Ver.  	: v0.46S for All
	//	Initial Date		: 2014-07-31
	//
	//	Customs          : Internal
	//	AP or B/E        : 
	//	Demo Board       : USB3.0 Tool
	//	
	//	Image size       : 2604x1956
	//	mclk/pclk        : 24mhz / 88Mhz
	//	MIPI speed(Mbps) : 880Mbps (each lane)
	//	MIPI						 : Non-continuous 
	//	Frame Length     : 1984
	//	V-Blank          : 546us
	//	Line Length	     : 2880
	//	H-Blank          : 386ns / 387ns	
	//	Max Fps          : 30fps (= Exp.time : 33ms )	
	//	Pixel order      : Blue 1st (=BGGR)
	//	X/Y-flip         : No-X/Y flip
	//	I2C Address      : 0x40(Write), 0x41(Read)
	//	AG               : x1
	//	DG               : x1
	//	OTP Type         : Single Write/read
	//	
	//
	//////////////////////////////////////////////////////////////////////////
	//<<<<<<<<<  Notice >>>>>>>>>/////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//
	//	Notice
	//	1) I2C Address & Data Type is 2Byte. 
	//	2) I2C Data construction that high byte is low addres, and low byte is high address. 
	//		 Initial register address must have used the even number. 
	//		 ex){Address, Data} = {Address(Even, 2byte), Data(2byte)} <==== Used Type
	//												= {Address(Even, 2byte(low address)), Data(1byte)} + {Address(Odd, 2byte(high address)), Data(1byte)} <== Not Used Type
	//												= write_cmos_sensor(0x0000, 0x0F03} => write_cmos_sensor(0x0000, 0x0F} + write_cmos_sensor(0x0001, 0x03}
	//	3) The Continuous Mode of MIPI 2 Lane set is write_cmos_sensor(0x0902, 0x4301}. And, 1lane set is write_cmos_sensor(0x0902, 0x0301}.
	//		 The Non-continuous Mode of MIPI 2 Lane set is write_cmos_sensor(0x0902, 0x4101}. And, 1lane set is write_cmos_sensor(0x0902, 0x0101}.	
	//	4) Analog Gain address is 0x003a. 
	//			ex)	0x0000 = x1, 0x7000 = x8, 0xf000 = x16
	//
	/////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////
	////////////// Hi-545 Initial //////////////////
	////////////////////////////////////////////////


	write_cmos_sensor(0x0118, 0x0000); //sleep On / standby

	//--- SRAM timing control---//
	write_cmos_sensor(0x0E00, 0x0101);
	write_cmos_sensor(0x0E02, 0x0101);
	write_cmos_sensor(0x0E04, 0x0101);
	write_cmos_sensor(0x0E06, 0x0101);
	write_cmos_sensor(0x0E08, 0x0101);
	write_cmos_sensor(0x0E0A, 0x0101);
	write_cmos_sensor(0x0E0C, 0x0101);
	write_cmos_sensor(0x0E0E, 0x0101);

	//Firmware 2Lane v0.38, All, AE+545 OTP Burst off FW 20140729
	write_cmos_sensor(0x2000, 0x4031);
	write_cmos_sensor(0x2002, 0x83F8);
	write_cmos_sensor(0x2004, 0x4104);
	write_cmos_sensor(0x2006, 0x4307);
	write_cmos_sensor(0x2008, 0x430A);
	write_cmos_sensor(0x200a, 0x4382);
	write_cmos_sensor(0x200c, 0x80CC);
	write_cmos_sensor(0x200e, 0x4382);
	write_cmos_sensor(0x2010, 0x8070);
	write_cmos_sensor(0x2012, 0x43A2);
	write_cmos_sensor(0x2014, 0x0B80);
	write_cmos_sensor(0x2016, 0x0C0A);
	write_cmos_sensor(0x2018, 0x4382);
	write_cmos_sensor(0x201a, 0x0B90);
	write_cmos_sensor(0x201c, 0x0C0A);
	write_cmos_sensor(0x201e, 0x4382);
	write_cmos_sensor(0x2020, 0x0B9C);
	write_cmos_sensor(0x2022, 0x0C0A);
	write_cmos_sensor(0x2024, 0x93D2);
	write_cmos_sensor(0x2026, 0x003D);
	write_cmos_sensor(0x2028, 0x2002);
	write_cmos_sensor(0x202a, 0x4030);
	write_cmos_sensor(0x202c, 0xF69C);
	write_cmos_sensor(0x202e, 0x43C2);
	write_cmos_sensor(0x2030, 0x0F82);
	write_cmos_sensor(0x2032, 0x425F);
	write_cmos_sensor(0x2034, 0x0118);
	write_cmos_sensor(0x2036, 0xF37F);
	write_cmos_sensor(0x2038, 0x930F);
	write_cmos_sensor(0x203a, 0x2002);
	write_cmos_sensor(0x203c, 0x0CC8);
	write_cmos_sensor(0x203e, 0x3FF9);
	write_cmos_sensor(0x2040, 0x4F82);
	write_cmos_sensor(0x2042, 0x8098);
	write_cmos_sensor(0x2044, 0x43D2);
	write_cmos_sensor(0x2046, 0x0A80);
	write_cmos_sensor(0x2048, 0x43D2);
	write_cmos_sensor(0x204a, 0x0180);
	write_cmos_sensor(0x204c, 0x43D2);
	write_cmos_sensor(0x204e, 0x019A);
	write_cmos_sensor(0x2050, 0x40F2);
	write_cmos_sensor(0x2052, 0x0009);
	write_cmos_sensor(0x2054, 0x019B);
	write_cmos_sensor(0x2056, 0x12B0);
	write_cmos_sensor(0x2058, 0xFD70);
	write_cmos_sensor(0x205a, 0x93D2);
	write_cmos_sensor(0x205c, 0x003E);
	write_cmos_sensor(0x205e, 0x2002);
	write_cmos_sensor(0x2060, 0x4030);
	write_cmos_sensor(0x2062, 0xF580);
	write_cmos_sensor(0x2064, 0x4308);
	write_cmos_sensor(0x2066, 0x5038);
	write_cmos_sensor(0x2068, 0x0030);
	write_cmos_sensor(0x206a, 0x480F);
	write_cmos_sensor(0x206c, 0x12B0);
	write_cmos_sensor(0x206e, 0xFD82);
	write_cmos_sensor(0x2070, 0x403B);
	write_cmos_sensor(0x2072, 0x7606);
	write_cmos_sensor(0x2074, 0x4B29);
	write_cmos_sensor(0x2076, 0x5318);
	write_cmos_sensor(0x2078, 0x480F);
	write_cmos_sensor(0x207a, 0x12B0);
	write_cmos_sensor(0x207c, 0xFD82);
	write_cmos_sensor(0x207e, 0x4B2A);
	write_cmos_sensor(0x2080, 0x5318);
	write_cmos_sensor(0x2082, 0x480F);
	write_cmos_sensor(0x2084, 0x12B0);
	write_cmos_sensor(0x2086, 0xFD82);
	write_cmos_sensor(0x2088, 0x4A0D);
	write_cmos_sensor(0x208a, 0xF03D);
	write_cmos_sensor(0x208c, 0x000F);
	write_cmos_sensor(0x208e, 0x108D);
	write_cmos_sensor(0x2090, 0x4B2E);
	write_cmos_sensor(0x2092, 0x5E0E);
	write_cmos_sensor(0x2094, 0x5E0E);
	write_cmos_sensor(0x2096, 0x5E0E);
	write_cmos_sensor(0x2098, 0x5E0E);
	write_cmos_sensor(0x209a, 0x4A0F);
	write_cmos_sensor(0x209c, 0xC312);
	write_cmos_sensor(0x209e, 0x100F);
	write_cmos_sensor(0x20a0, 0x110F);
	write_cmos_sensor(0x20a2, 0x110F);
	write_cmos_sensor(0x20a4, 0x110F);
	write_cmos_sensor(0x20a6, 0x590D);
	write_cmos_sensor(0x20a8, 0x4D87);
	write_cmos_sensor(0x20aa, 0x5000);
	write_cmos_sensor(0x20ac, 0x5F0E);
	write_cmos_sensor(0x20ae, 0x4E87);
	write_cmos_sensor(0x20b0, 0x6000);
	write_cmos_sensor(0x20b2, 0x5327);
	write_cmos_sensor(0x20b4, 0x5038);
	write_cmos_sensor(0x20b6, 0xFFD1);
	write_cmos_sensor(0x20b8, 0x9038);
	write_cmos_sensor(0x20ba, 0x0300);
	write_cmos_sensor(0x20bc, 0x2BD4);
	write_cmos_sensor(0x20be, 0x0261);
	write_cmos_sensor(0x20c0, 0x0000);
	write_cmos_sensor(0x20c2, 0x43A2);
	write_cmos_sensor(0x20c4, 0x0384);
	write_cmos_sensor(0x20c6, 0x42B2);
	write_cmos_sensor(0x20c8, 0x0386);
	write_cmos_sensor(0x20ca, 0x43C2);
	write_cmos_sensor(0x20cc, 0x0180);
	write_cmos_sensor(0x20ce, 0x43D2);
	write_cmos_sensor(0x20d0, 0x003D);
	write_cmos_sensor(0x20d2, 0x40B2);
	write_cmos_sensor(0x20d4, 0x808B);
	write_cmos_sensor(0x20d6, 0x0B88);
	write_cmos_sensor(0x20d8, 0x0C0A);
	write_cmos_sensor(0x20da, 0x40B2);
	write_cmos_sensor(0x20dc, 0x1009);
	write_cmos_sensor(0x20de, 0x0B8A);
	write_cmos_sensor(0x20e0, 0x0C0A);
	write_cmos_sensor(0x20e2, 0x40B2);
	write_cmos_sensor(0x20e4, 0xC40C);
	write_cmos_sensor(0x20e6, 0x0B8C);
	write_cmos_sensor(0x20e8, 0x0C0A);
	write_cmos_sensor(0x20ea, 0x40B2);
	write_cmos_sensor(0x20ec, 0xC9E1);
	write_cmos_sensor(0x20ee, 0x0B8E);
	write_cmos_sensor(0x20f0, 0x0C0A);
	write_cmos_sensor(0x20f2, 0x40B2);
	write_cmos_sensor(0x20f4, 0x0C1E);
	write_cmos_sensor(0x20f6, 0x0B92);
	write_cmos_sensor(0x20f8, 0x0C0A);
	write_cmos_sensor(0x20fa, 0x43D2);
	write_cmos_sensor(0x20fc, 0x0F82);
	write_cmos_sensor(0x20fe, 0x0C3C);
	write_cmos_sensor(0x2100, 0x0C3C);
	write_cmos_sensor(0x2102, 0x0C3C);
	write_cmos_sensor(0x2104, 0x0C3C);
	write_cmos_sensor(0x2106, 0x421F);
	write_cmos_sensor(0x2108, 0x00A6);
	write_cmos_sensor(0x210a, 0x503F);
	write_cmos_sensor(0x210c, 0x07D0);
	write_cmos_sensor(0x210e, 0x3811);
	write_cmos_sensor(0x2110, 0x4F82);
	write_cmos_sensor(0x2112, 0x7100);
	write_cmos_sensor(0x2114, 0x0004);
	write_cmos_sensor(0x2116, 0x0C0D);
	write_cmos_sensor(0x2118, 0x0005);
	write_cmos_sensor(0x211a, 0x0C04);
	write_cmos_sensor(0x211c, 0x000D);
	write_cmos_sensor(0x211e, 0x0C09);
	write_cmos_sensor(0x2120, 0x003D);
	write_cmos_sensor(0x2122, 0x0C1D);
	write_cmos_sensor(0x2124, 0x003C);
	write_cmos_sensor(0x2126, 0x0C13);
	write_cmos_sensor(0x2128, 0x0004);
	write_cmos_sensor(0x212a, 0x0C09);
	write_cmos_sensor(0x212c, 0x0004);
	write_cmos_sensor(0x212e, 0x533F);
	write_cmos_sensor(0x2130, 0x37EF);
	write_cmos_sensor(0x2132, 0x4392);
	write_cmos_sensor(0x2134, 0x8094);
	write_cmos_sensor(0x2136, 0x4382);
	write_cmos_sensor(0x2138, 0x809C);
	write_cmos_sensor(0x213a, 0x4382);
	write_cmos_sensor(0x213c, 0x80B4);
	write_cmos_sensor(0x213e, 0x4382);
	write_cmos_sensor(0x2140, 0x80BA);
	write_cmos_sensor(0x2142, 0x4382);
	write_cmos_sensor(0x2144, 0x80A0);
	write_cmos_sensor(0x2146, 0x40B2);
	write_cmos_sensor(0x2148, 0x0028);
	write_cmos_sensor(0x214a, 0x7000);
	write_cmos_sensor(0x214c, 0x43A2);
	write_cmos_sensor(0x214e, 0x809E);
	write_cmos_sensor(0x2150, 0xB3E2);
	write_cmos_sensor(0x2152, 0x00B4);
	write_cmos_sensor(0x2154, 0x2402);
	write_cmos_sensor(0x2156, 0x4392);
	write_cmos_sensor(0x2158, 0x809E);
	write_cmos_sensor(0x215a, 0x4328);
	write_cmos_sensor(0x215c, 0xB3D2);
	write_cmos_sensor(0x215e, 0x00B4);
	write_cmos_sensor(0x2160, 0x2002);
	write_cmos_sensor(0x2162, 0x4030);
	write_cmos_sensor(0x2164, 0xF570);
	write_cmos_sensor(0x2166, 0x4308);
	write_cmos_sensor(0x2168, 0x4384);
	write_cmos_sensor(0x216a, 0x0002);
	write_cmos_sensor(0x216c, 0x4384);
	write_cmos_sensor(0x216e, 0x0006);
	write_cmos_sensor(0x2170, 0x4382);
	write_cmos_sensor(0x2172, 0x809A);
	write_cmos_sensor(0x2174, 0x4382);
	write_cmos_sensor(0x2176, 0x8096);
	write_cmos_sensor(0x2178, 0x40B2);
	write_cmos_sensor(0x217a, 0x0005);
	write_cmos_sensor(0x217c, 0x7320);
	write_cmos_sensor(0x217e, 0x4392);
	write_cmos_sensor(0x2180, 0x7326);
	write_cmos_sensor(0x2182, 0x12B0);
	write_cmos_sensor(0x2184, 0xF92E);
	write_cmos_sensor(0x2186, 0x4392);
	write_cmos_sensor(0x2188, 0x731C);
	write_cmos_sensor(0x218a, 0x9382);
	write_cmos_sensor(0x218c, 0x8094);
	write_cmos_sensor(0x218e, 0x200A);
	write_cmos_sensor(0x2190, 0x0B00);
	write_cmos_sensor(0x2192, 0x7302);
	write_cmos_sensor(0x2194, 0x02BC);
	write_cmos_sensor(0x2196, 0x4382);
	write_cmos_sensor(0x2198, 0x7004);
	write_cmos_sensor(0x219a, 0x430F);
	write_cmos_sensor(0x219c, 0x12B0);
	write_cmos_sensor(0x219e, 0xF72E);
	write_cmos_sensor(0x21a0, 0x12B0);
	write_cmos_sensor(0x21a2, 0xF92E);
	write_cmos_sensor(0x21a4, 0x4392);
	write_cmos_sensor(0x21a6, 0x80B8);
	write_cmos_sensor(0x21a8, 0x4382);
	write_cmos_sensor(0x21aa, 0x740E);
	write_cmos_sensor(0x21ac, 0xB3E2);
	write_cmos_sensor(0x21ae, 0x0080);
	write_cmos_sensor(0x21b0, 0x2402);
	write_cmos_sensor(0x21b2, 0x4392);
	write_cmos_sensor(0x21b4, 0x740E);
	write_cmos_sensor(0x21b6, 0x431F);
	write_cmos_sensor(0x21b8, 0x12B0);
	write_cmos_sensor(0x21ba, 0xF72E);
	write_cmos_sensor(0x21bc, 0x4392);
	write_cmos_sensor(0x21be, 0x7004);
	write_cmos_sensor(0x21c0, 0x4882);
	write_cmos_sensor(0x21c2, 0x7110);
	write_cmos_sensor(0x21c4, 0x9382);
	write_cmos_sensor(0x21c6, 0x8092);
	write_cmos_sensor(0x21c8, 0x2005);
	write_cmos_sensor(0x21ca, 0x9392);
	write_cmos_sensor(0x21cc, 0x7110);
	write_cmos_sensor(0x21ce, 0x2402);
	write_cmos_sensor(0x21d0, 0x4030);
	write_cmos_sensor(0x21d2, 0xF474);
	write_cmos_sensor(0x21d4, 0x9392);
	write_cmos_sensor(0x21d6, 0x7110);
	write_cmos_sensor(0x21d8, 0x2096);
	write_cmos_sensor(0x21da, 0x0B00);
	write_cmos_sensor(0x21dc, 0x7302);
	write_cmos_sensor(0x21de, 0x0032);
	write_cmos_sensor(0x21e0, 0x4382);
	write_cmos_sensor(0x21e2, 0x7004);
	write_cmos_sensor(0x21e4, 0x0B00);
	write_cmos_sensor(0x21e6, 0x7302);
	write_cmos_sensor(0x21e8, 0x03E8);
	write_cmos_sensor(0x21ea, 0x0800);
	write_cmos_sensor(0x21ec, 0x7114);
	write_cmos_sensor(0x21ee, 0x425F);
	write_cmos_sensor(0x21f0, 0x0C9C);
	write_cmos_sensor(0x21f2, 0x4F4E);
	write_cmos_sensor(0x21f4, 0x430F);
	write_cmos_sensor(0x21f6, 0x4E0D);
	write_cmos_sensor(0x21f8, 0x430C);
	write_cmos_sensor(0x21fa, 0x421F);
	write_cmos_sensor(0x21fc, 0x0C9A);
	write_cmos_sensor(0x21fe, 0xDF0C);
	write_cmos_sensor(0x2200, 0x1204);
	write_cmos_sensor(0x2202, 0x440F);
	write_cmos_sensor(0x2204, 0x532F);
	write_cmos_sensor(0x2206, 0x120F);
	write_cmos_sensor(0x2208, 0x1212);
	write_cmos_sensor(0x220a, 0x0CA2);
	write_cmos_sensor(0x220c, 0x403E);
	write_cmos_sensor(0x220e, 0x80BC);
	write_cmos_sensor(0x2210, 0x403F);
	write_cmos_sensor(0x2212, 0x8072);
	write_cmos_sensor(0x2214, 0x12B0);
	write_cmos_sensor(0x2216, 0xF796);
	write_cmos_sensor(0x2218, 0x4F09);
	write_cmos_sensor(0x221a, 0x425F);
	write_cmos_sensor(0x221c, 0x0CA0);
	write_cmos_sensor(0x221e, 0x4F4E);
	write_cmos_sensor(0x2220, 0x430F);
	write_cmos_sensor(0x2222, 0x4E0D);
	write_cmos_sensor(0x2224, 0x430C);
	write_cmos_sensor(0x2226, 0x421F);
	write_cmos_sensor(0x2228, 0x0C9E);
	write_cmos_sensor(0x222a, 0xDF0C);
	write_cmos_sensor(0x222c, 0x440F);
	write_cmos_sensor(0x222e, 0x522F);
	write_cmos_sensor(0x2230, 0x120F);
	write_cmos_sensor(0x2232, 0x532F);
	write_cmos_sensor(0x2234, 0x120F);
	write_cmos_sensor(0x2236, 0x1212);
	write_cmos_sensor(0x2238, 0x0CA4);
	write_cmos_sensor(0x223a, 0x403E);
	write_cmos_sensor(0x223c, 0x80A2);
	write_cmos_sensor(0x223e, 0x403F);
	write_cmos_sensor(0x2240, 0x8050);
	write_cmos_sensor(0x2242, 0x12B0);
	write_cmos_sensor(0x2244, 0xF796);
	write_cmos_sensor(0x2246, 0x4F0B);
	write_cmos_sensor(0x2248, 0x430D);
	write_cmos_sensor(0x224a, 0x441E);
	write_cmos_sensor(0x224c, 0x0004);
	write_cmos_sensor(0x224e, 0x442F);
	write_cmos_sensor(0x2250, 0x5031);
	write_cmos_sensor(0x2252, 0x000C);
	write_cmos_sensor(0x2254, 0x9E0F);
	write_cmos_sensor(0x2256, 0x2C01);
	write_cmos_sensor(0x2258, 0x431D);
	write_cmos_sensor(0x225a, 0x8E0F);
	write_cmos_sensor(0x225c, 0x930F);
	write_cmos_sensor(0x225e, 0x3402);
	write_cmos_sensor(0x2260, 0xE33F);
	write_cmos_sensor(0x2262, 0x531F);
	write_cmos_sensor(0x2264, 0x421E);
	write_cmos_sensor(0x2266, 0x0CA2);
	write_cmos_sensor(0x2268, 0xC312);
	write_cmos_sensor(0x226a, 0x100E);
	write_cmos_sensor(0x226c, 0x9E0F);
	write_cmos_sensor(0x226e, 0x2804);
	write_cmos_sensor(0x2270, 0x930D);
	write_cmos_sensor(0x2272, 0x2001);
	write_cmos_sensor(0x2274, 0x5319);
	write_cmos_sensor(0x2276, 0x5D0B);
	write_cmos_sensor(0x2278, 0x403D);
	write_cmos_sensor(0x227a, 0x0196);
	write_cmos_sensor(0x227c, 0x4D2F);
	write_cmos_sensor(0x227e, 0x490A);
	write_cmos_sensor(0x2280, 0x4F0C);
	write_cmos_sensor(0x2282, 0x12B0);
	write_cmos_sensor(0x2284, 0xFDA2);
	write_cmos_sensor(0x2286, 0x4E09);
	write_cmos_sensor(0x2288, 0xC312);
	write_cmos_sensor(0x228a, 0x1009);
	write_cmos_sensor(0x228c, 0x1109);
	write_cmos_sensor(0x228e, 0x1109);
	write_cmos_sensor(0x2290, 0x1109);
	write_cmos_sensor(0x2292, 0x1109);
	write_cmos_sensor(0x2294, 0x1109);
	write_cmos_sensor(0x2296, 0x4D2F);
	write_cmos_sensor(0x2298, 0x4B0A);
	write_cmos_sensor(0x229a, 0x4F0C);
	write_cmos_sensor(0x229c, 0x12B0);
	write_cmos_sensor(0x229e, 0xFDA2);
	write_cmos_sensor(0x22a0, 0x4E0B);
	write_cmos_sensor(0x22a2, 0xC312);
	write_cmos_sensor(0x22a4, 0x100B);
	write_cmos_sensor(0x22a6, 0x110B);
	write_cmos_sensor(0x22a8, 0x110B);
	write_cmos_sensor(0x22aa, 0x110B);
	write_cmos_sensor(0x22ac, 0x110B);
	write_cmos_sensor(0x22ae, 0x110B);
	write_cmos_sensor(0x22b0, 0x425F);
	write_cmos_sensor(0x22b2, 0x00BA);
	write_cmos_sensor(0x22b4, 0xC312);
	write_cmos_sensor(0x22b6, 0x104F);
	write_cmos_sensor(0x22b8, 0x114F);
	write_cmos_sensor(0x22ba, 0x114F);
	write_cmos_sensor(0x22bc, 0x114F);
	write_cmos_sensor(0x22be, 0xF37F);
	write_cmos_sensor(0x22c0, 0x5F0F);
	write_cmos_sensor(0x22c2, 0x5F0F);
	write_cmos_sensor(0x22c4, 0x5F0F);
	write_cmos_sensor(0x22c6, 0x5F0F);
	write_cmos_sensor(0x22c8, 0x503F);
	write_cmos_sensor(0x22ca, 0x0010);
	write_cmos_sensor(0x22cc, 0x92E2);
	write_cmos_sensor(0x22ce, 0x00A0);
	write_cmos_sensor(0x22d0, 0x2406);
	write_cmos_sensor(0x22d2, 0x990F);
	write_cmos_sensor(0x22d4, 0x2C01);
	write_cmos_sensor(0x22d6, 0x4F09);
	write_cmos_sensor(0x22d8, 0x9B0F);
	write_cmos_sensor(0x22da, 0x2C01);
	write_cmos_sensor(0x22dc, 0x4F0B);
	write_cmos_sensor(0x22de, 0x92B2);
	write_cmos_sensor(0x22e0, 0x80BA);
	write_cmos_sensor(0x22e2, 0x280C);
	write_cmos_sensor(0x22e4, 0x90B2);
	write_cmos_sensor(0x22e6, 0x0096);
	write_cmos_sensor(0x22e8, 0x80B4);
	write_cmos_sensor(0x22ea, 0x2408);
	write_cmos_sensor(0x22ec, 0x0900);
	write_cmos_sensor(0x22ee, 0x710E);
	write_cmos_sensor(0x22f0, 0x0B00);
	write_cmos_sensor(0x22f2, 0x7302);
	write_cmos_sensor(0x22f4, 0x0320);
	write_cmos_sensor(0x22f6, 0x12B0);
	write_cmos_sensor(0x22f8, 0xF6CE);
	write_cmos_sensor(0x22fa, 0x3F64);
	write_cmos_sensor(0x22fc, 0x4982);
	write_cmos_sensor(0x22fe, 0x0CAC);
	write_cmos_sensor(0x2300, 0x4B82);
	write_cmos_sensor(0x2302, 0x0CAE);
	write_cmos_sensor(0x2304, 0x3FF3);
	write_cmos_sensor(0x2306, 0x0B00);
	write_cmos_sensor(0x2308, 0x7302);
	write_cmos_sensor(0x230a, 0x0002);
	write_cmos_sensor(0x230c, 0x069A);
	write_cmos_sensor(0x230e, 0x0C1F);
	write_cmos_sensor(0x2310, 0x0403);
	write_cmos_sensor(0x2312, 0x0C05);
	write_cmos_sensor(0x2314, 0x0001);
	write_cmos_sensor(0x2316, 0x0C01);
	write_cmos_sensor(0x2318, 0x0003);
	write_cmos_sensor(0x231a, 0x0C03);
	write_cmos_sensor(0x231c, 0x000B);
	write_cmos_sensor(0x231e, 0x0C33);
	write_cmos_sensor(0x2320, 0x0003);
	write_cmos_sensor(0x2322, 0x0C03);
	write_cmos_sensor(0x2324, 0x0653);
	write_cmos_sensor(0x2326, 0x0C03);
	write_cmos_sensor(0x2328, 0x065B);
	write_cmos_sensor(0x232a, 0x0C13);
	write_cmos_sensor(0x232c, 0x065F);
	write_cmos_sensor(0x232e, 0x0C43);
	write_cmos_sensor(0x2330, 0x0657);
	write_cmos_sensor(0x2332, 0x0C03);
	write_cmos_sensor(0x2334, 0x0653);
	write_cmos_sensor(0x2336, 0x0C03);
	write_cmos_sensor(0x2338, 0x0643);
	write_cmos_sensor(0x233a, 0x0C0F);
	write_cmos_sensor(0x233c, 0x067D);
	write_cmos_sensor(0x233e, 0x0C01);
	write_cmos_sensor(0x2340, 0x077F);
	write_cmos_sensor(0x2342, 0x0C01);
	write_cmos_sensor(0x2344, 0x0677);
	write_cmos_sensor(0x2346, 0x0C01);
	write_cmos_sensor(0x2348, 0x0673);
	write_cmos_sensor(0x234a, 0x0C67);
	write_cmos_sensor(0x234c, 0x0677);
	write_cmos_sensor(0x234e, 0x0C03);
	write_cmos_sensor(0x2350, 0x077D);
	write_cmos_sensor(0x2352, 0x0C19);
	write_cmos_sensor(0x2354, 0x0013);
	write_cmos_sensor(0x2356, 0x0C27);
	write_cmos_sensor(0x2358, 0x0003);
	write_cmos_sensor(0x235a, 0x0C45);
	write_cmos_sensor(0x235c, 0x0675);
	write_cmos_sensor(0x235e, 0x0C01);
	write_cmos_sensor(0x2360, 0x0671);
	write_cmos_sensor(0x2362, 0x4392);
	write_cmos_sensor(0x2364, 0x7004);
	write_cmos_sensor(0x2366, 0x430F);
	write_cmos_sensor(0x2368, 0x9382);
	write_cmos_sensor(0x236a, 0x80B8);
	write_cmos_sensor(0x236c, 0x2001);
	write_cmos_sensor(0x236e, 0x431F);
	write_cmos_sensor(0x2370, 0x4F82);
	write_cmos_sensor(0x2372, 0x80B8);
	write_cmos_sensor(0x2374, 0x930F);
	write_cmos_sensor(0x2376, 0x2472);
	write_cmos_sensor(0x2378, 0x0B00);
	write_cmos_sensor(0x237a, 0x7302);
	write_cmos_sensor(0x237c, 0x033A);
	write_cmos_sensor(0x237e, 0x0675);
	write_cmos_sensor(0x2380, 0x0C02);
	write_cmos_sensor(0x2382, 0x0339);
	write_cmos_sensor(0x2384, 0xAE0C);
	write_cmos_sensor(0x2386, 0x0C01);
	write_cmos_sensor(0x2388, 0x003C);
	write_cmos_sensor(0x238a, 0x0C01);
	write_cmos_sensor(0x238c, 0x0004);
	write_cmos_sensor(0x238e, 0x0C01);
	write_cmos_sensor(0x2390, 0x0642);
	write_cmos_sensor(0x2392, 0x0B00);
	write_cmos_sensor(0x2394, 0x7302);
	write_cmos_sensor(0x2396, 0x0386);
	write_cmos_sensor(0x2398, 0x0643);
	write_cmos_sensor(0x239a, 0x0C05);
	write_cmos_sensor(0x239c, 0x0001);
	write_cmos_sensor(0x239e, 0x0C01);
	write_cmos_sensor(0x23a0, 0x0003);
	write_cmos_sensor(0x23a2, 0x0C03);
	write_cmos_sensor(0x23a4, 0x000B);
	write_cmos_sensor(0x23a6, 0x0C33);
	write_cmos_sensor(0x23a8, 0x0003);
	write_cmos_sensor(0x23aa, 0x0C03);
	write_cmos_sensor(0x23ac, 0x0653);
	write_cmos_sensor(0x23ae, 0x0C03);
	write_cmos_sensor(0x23b0, 0x065B);
	write_cmos_sensor(0x23b2, 0x0C13);
	write_cmos_sensor(0x23b4, 0x065F);
	write_cmos_sensor(0x23b6, 0x0C43);
	write_cmos_sensor(0x23b8, 0x0657);
	write_cmos_sensor(0x23ba, 0x0C03);
	write_cmos_sensor(0x23bc, 0x0653);
	write_cmos_sensor(0x23be, 0x0C03);
	write_cmos_sensor(0x23c0, 0x0643);
	write_cmos_sensor(0x23c2, 0x0C0F);
	write_cmos_sensor(0x23c4, 0x067D);
	write_cmos_sensor(0x23c6, 0x0C01);
	write_cmos_sensor(0x23c8, 0x077F);
	write_cmos_sensor(0x23ca, 0x0C01);
	write_cmos_sensor(0x23cc, 0x0677);
	write_cmos_sensor(0x23ce, 0x0C01);
	write_cmos_sensor(0x23d0, 0x0673);
	write_cmos_sensor(0x23d2, 0x0C67);
	write_cmos_sensor(0x23d4, 0x0677);
	write_cmos_sensor(0x23d6, 0x0C03);
	write_cmos_sensor(0x23d8, 0x077D);
	write_cmos_sensor(0x23da, 0x0C19);
	write_cmos_sensor(0x23dc, 0x0013);
	write_cmos_sensor(0x23de, 0x0C27);
	write_cmos_sensor(0x23e0, 0x0003);
	write_cmos_sensor(0x23e2, 0x0C45);
	write_cmos_sensor(0x23e4, 0x0675);
	write_cmos_sensor(0x23e6, 0x0C01);
	write_cmos_sensor(0x23e8, 0x0671);
	write_cmos_sensor(0x23ea, 0x12B0);
	write_cmos_sensor(0x23ec, 0xF6CE);
	write_cmos_sensor(0x23ee, 0x930F);
	write_cmos_sensor(0x23f0, 0x2405);
	write_cmos_sensor(0x23f2, 0x4292);
	write_cmos_sensor(0x23f4, 0x8094);
	write_cmos_sensor(0x23f6, 0x809C);
	write_cmos_sensor(0x23f8, 0x4382);
	write_cmos_sensor(0x23fa, 0x8094);
	write_cmos_sensor(0x23fc, 0x9382);
	write_cmos_sensor(0x23fe, 0x80B8);
	write_cmos_sensor(0x2400, 0x241D);
	write_cmos_sensor(0x2402, 0x0B00);
	write_cmos_sensor(0x2404, 0x7302);
	write_cmos_sensor(0x2406, 0x069E);
	write_cmos_sensor(0x2408, 0x0675);
	write_cmos_sensor(0x240a, 0x0C02);
	write_cmos_sensor(0x240c, 0x0339);
	write_cmos_sensor(0x240e, 0xAE0C);
	write_cmos_sensor(0x2410, 0x0C01);
	write_cmos_sensor(0x2412, 0x003C);
	write_cmos_sensor(0x2414, 0x0C01);
	write_cmos_sensor(0x2416, 0x0004);
	write_cmos_sensor(0x2418, 0x0C01);
	write_cmos_sensor(0x241a, 0x0642);
	write_cmos_sensor(0x241c, 0x0C01);
	write_cmos_sensor(0x241e, 0x06A1);
	write_cmos_sensor(0x2420, 0x0C03);
	write_cmos_sensor(0x2422, 0x06A0);
	write_cmos_sensor(0x2424, 0x9382);
	write_cmos_sensor(0x2426, 0x80CC);
	write_cmos_sensor(0x2428, 0x2003);
	write_cmos_sensor(0x242a, 0x930F);
	write_cmos_sensor(0x242c, 0x26CB);
	write_cmos_sensor(0x242e, 0x3EAD);
	write_cmos_sensor(0x2430, 0x43C2);
	write_cmos_sensor(0x2432, 0x0A80);
	write_cmos_sensor(0x2434, 0x0B00);
	write_cmos_sensor(0x2436, 0x7302);
	write_cmos_sensor(0x2438, 0xFFF0);
	write_cmos_sensor(0x243a, 0x3EC4);
	write_cmos_sensor(0x243c, 0x0B00);
	write_cmos_sensor(0x243e, 0x7302);
	write_cmos_sensor(0x2440, 0x069E);
	write_cmos_sensor(0x2442, 0x0675);
	write_cmos_sensor(0x2444, 0x0C02);
	write_cmos_sensor(0x2446, 0x0301);
	write_cmos_sensor(0x2448, 0xAE0C);
	write_cmos_sensor(0x244a, 0x0C01);
	write_cmos_sensor(0x244c, 0x0004);
	write_cmos_sensor(0x244e, 0x0C03);
	write_cmos_sensor(0x2450, 0x0642);
	write_cmos_sensor(0x2452, 0x0C01);
	write_cmos_sensor(0x2454, 0x06A1);
	write_cmos_sensor(0x2456, 0x0C03);
	write_cmos_sensor(0x2458, 0x06A0);
	write_cmos_sensor(0x245a, 0x3FE4);
	write_cmos_sensor(0x245c, 0x0B00);
	write_cmos_sensor(0x245e, 0x7302);
	write_cmos_sensor(0x2460, 0x033A);
	write_cmos_sensor(0x2462, 0x0675);
	write_cmos_sensor(0x2464, 0x0C02);
	write_cmos_sensor(0x2466, 0x0301);
	write_cmos_sensor(0x2468, 0xAE0C);
	write_cmos_sensor(0x246a, 0x0C01);
	write_cmos_sensor(0x246c, 0x0004);
	write_cmos_sensor(0x246e, 0x0C03);
	write_cmos_sensor(0x2470, 0x0642);
	write_cmos_sensor(0x2472, 0x3F8F);
	write_cmos_sensor(0x2474, 0x0B00);
	write_cmos_sensor(0x2476, 0x7302);
	write_cmos_sensor(0x2478, 0x0002);
	write_cmos_sensor(0x247a, 0x069A);
	write_cmos_sensor(0x247c, 0x0C1F);
	write_cmos_sensor(0x247e, 0x0402);
	write_cmos_sensor(0x2480, 0x0C05);
	write_cmos_sensor(0x2482, 0x0001);
	write_cmos_sensor(0x2484, 0x0C01);
	write_cmos_sensor(0x2486, 0x0003);
	write_cmos_sensor(0x2488, 0x0C03);
	write_cmos_sensor(0x248a, 0x000B);
	write_cmos_sensor(0x248c, 0x0C33);
	write_cmos_sensor(0x248e, 0x0003);
	write_cmos_sensor(0x2490, 0x0C03);
	write_cmos_sensor(0x2492, 0x0653);
	write_cmos_sensor(0x2494, 0x0C03);
	write_cmos_sensor(0x2496, 0x065B);
	write_cmos_sensor(0x2498, 0x0C13);
	write_cmos_sensor(0x249a, 0x065F);
	write_cmos_sensor(0x249c, 0x0C43);
	write_cmos_sensor(0x249e, 0x0657);
	write_cmos_sensor(0x24a0, 0x0C03);
	write_cmos_sensor(0x24a2, 0x0653);
	write_cmos_sensor(0x24a4, 0x0C03);
	write_cmos_sensor(0x24a6, 0x0643);
	write_cmos_sensor(0x24a8, 0x0C0F);
	write_cmos_sensor(0x24aa, 0x077D);
	write_cmos_sensor(0x24ac, 0x0C01);
	write_cmos_sensor(0x24ae, 0x067F);
	write_cmos_sensor(0x24b0, 0x0C01);
	write_cmos_sensor(0x24b2, 0x0677);
	write_cmos_sensor(0x24b4, 0x0C01);
	write_cmos_sensor(0x24b6, 0x0673);
	write_cmos_sensor(0x24b8, 0x0C5F);
	write_cmos_sensor(0x24ba, 0x0663);
	write_cmos_sensor(0x24bc, 0x0C6F);
	write_cmos_sensor(0x24be, 0x0667);
	write_cmos_sensor(0x24c0, 0x0C01);
	write_cmos_sensor(0x24c2, 0x0677);
	write_cmos_sensor(0x24c4, 0x0C01);
	write_cmos_sensor(0x24c6, 0x077D);
	write_cmos_sensor(0x24c8, 0x0C33);
	write_cmos_sensor(0x24ca, 0x0013);
	write_cmos_sensor(0x24cc, 0x0C27);
	write_cmos_sensor(0x24ce, 0x0003);
	write_cmos_sensor(0x24d0, 0x0C4F);
	write_cmos_sensor(0x24d2, 0x0675);
	write_cmos_sensor(0x24d4, 0x0C01);
	write_cmos_sensor(0x24d6, 0x0671);
	write_cmos_sensor(0x24d8, 0x0CFF);
	write_cmos_sensor(0x24da, 0x0C78);
	write_cmos_sensor(0x24dc, 0x0661);
	write_cmos_sensor(0x24de, 0x4392);
	write_cmos_sensor(0x24e0, 0x7004);
	write_cmos_sensor(0x24e2, 0x430F);
	write_cmos_sensor(0x24e4, 0x9382);
	write_cmos_sensor(0x24e6, 0x80B8);
	write_cmos_sensor(0x24e8, 0x2001);
	write_cmos_sensor(0x24ea, 0x431F);
	write_cmos_sensor(0x24ec, 0x4F82);
	write_cmos_sensor(0x24ee, 0x80B8);
	write_cmos_sensor(0x24f0, 0x12B0);
	write_cmos_sensor(0x24f2, 0xF6CE);
	write_cmos_sensor(0x24f4, 0x930F);
	write_cmos_sensor(0x24f6, 0x2405);
	write_cmos_sensor(0x24f8, 0x4292);
	write_cmos_sensor(0x24fa, 0x8094);
	write_cmos_sensor(0x24fc, 0x809C);
	write_cmos_sensor(0x24fe, 0x4382);
	write_cmos_sensor(0x2500, 0x8094);
	write_cmos_sensor(0x2502, 0x9382);
	write_cmos_sensor(0x2504, 0x80B8);
	write_cmos_sensor(0x2506, 0x2019);
	write_cmos_sensor(0x2508, 0x0B00);
	write_cmos_sensor(0x250a, 0x7302);
	write_cmos_sensor(0x250c, 0x0562);
	write_cmos_sensor(0x250e, 0x0665);
	write_cmos_sensor(0x2510, 0x0C02);
	write_cmos_sensor(0x2512, 0x0301);
	write_cmos_sensor(0x2514, 0xA60C);
	write_cmos_sensor(0x2516, 0x0204);
	write_cmos_sensor(0x2518, 0xAE0C);
	write_cmos_sensor(0x251a, 0x0C03);
	write_cmos_sensor(0x251c, 0x0642);
	write_cmos_sensor(0x251e, 0x0C13);
	write_cmos_sensor(0x2520, 0x06A1);
	write_cmos_sensor(0x2522, 0x0C03);
	write_cmos_sensor(0x2524, 0x06A0);
	write_cmos_sensor(0x2526, 0x9382);
	write_cmos_sensor(0x2528, 0x80CC);
	write_cmos_sensor(0x252a, 0x277F);
	write_cmos_sensor(0x252c, 0x43C2);
	write_cmos_sensor(0x252e, 0x0A80);
	write_cmos_sensor(0x2530, 0x0B00);
	write_cmos_sensor(0x2532, 0x7302);
	write_cmos_sensor(0x2534, 0xFFF0);
	write_cmos_sensor(0x2536, 0x4030);
	write_cmos_sensor(0x2538, 0xF1C4);
	write_cmos_sensor(0x253a, 0x0B00);
	write_cmos_sensor(0x253c, 0x7302);
	write_cmos_sensor(0x253e, 0x0562);
	write_cmos_sensor(0x2540, 0x0665);
	write_cmos_sensor(0x2542, 0x0C02);
	write_cmos_sensor(0x2544, 0x0339);
	write_cmos_sensor(0x2546, 0xA60C);
	write_cmos_sensor(0x2548, 0x023C);
	write_cmos_sensor(0x254a, 0xAE0C);
	write_cmos_sensor(0x254c, 0x0C01);
	write_cmos_sensor(0x254e, 0x0004);
	write_cmos_sensor(0x2550, 0x0C01);
	write_cmos_sensor(0x2552, 0x0642);
	write_cmos_sensor(0x2554, 0x0C13);
	write_cmos_sensor(0x2556, 0x06A1);
	write_cmos_sensor(0x2558, 0x0C03);
	write_cmos_sensor(0x255a, 0x06A0);
	write_cmos_sensor(0x255c, 0x9382);
	write_cmos_sensor(0x255e, 0x80CC);
	write_cmos_sensor(0x2560, 0x2764);
	write_cmos_sensor(0x2562, 0x43C2);
	write_cmos_sensor(0x2564, 0x0A80);
	write_cmos_sensor(0x2566, 0x0B00);
	write_cmos_sensor(0x2568, 0x7302);
	write_cmos_sensor(0x256a, 0xFFF0);
	write_cmos_sensor(0x256c, 0x4030);
	write_cmos_sensor(0x256e, 0xF1C4);
	write_cmos_sensor(0x2570, 0xB3E2);
	write_cmos_sensor(0x2572, 0x00B4);
	write_cmos_sensor(0x2574, 0x2002);
	write_cmos_sensor(0x2576, 0x4030);
	write_cmos_sensor(0x2578, 0xF168);
	write_cmos_sensor(0x257a, 0x4318);
	write_cmos_sensor(0x257c, 0x4030);
	write_cmos_sensor(0x257e, 0xF168);
	write_cmos_sensor(0x2580, 0x4392);
	write_cmos_sensor(0x2582, 0x760E);
	write_cmos_sensor(0x2584, 0x425F);
	write_cmos_sensor(0x2586, 0x0118);
	write_cmos_sensor(0x2588, 0xF37F);
	write_cmos_sensor(0x258a, 0x930F);
	write_cmos_sensor(0x258c, 0x2005);
	write_cmos_sensor(0x258e, 0x43C2);
	write_cmos_sensor(0x2590, 0x0A80);
	write_cmos_sensor(0x2592, 0x0B00);
	write_cmos_sensor(0x2594, 0x7302);
	write_cmos_sensor(0x2596, 0xFFF0);
	write_cmos_sensor(0x2598, 0x9382);
	write_cmos_sensor(0x259a, 0x760C);
	write_cmos_sensor(0x259c, 0x2002);
	write_cmos_sensor(0x259e, 0x0C64);
	write_cmos_sensor(0x25a0, 0x3FF1);
	write_cmos_sensor(0x25a2, 0x4F82);
	write_cmos_sensor(0x25a4, 0x8098);
	write_cmos_sensor(0x25a6, 0x12B0);
	write_cmos_sensor(0x25a8, 0xFD70);
	write_cmos_sensor(0x25aa, 0x421F);
	write_cmos_sensor(0x25ac, 0x760A);
	write_cmos_sensor(0x25ae, 0x903F);
	write_cmos_sensor(0x25b0, 0x0200);
	write_cmos_sensor(0x25b2, 0x2469);
	write_cmos_sensor(0x25b4, 0x930F);
	write_cmos_sensor(0x25b6, 0x2467);
	write_cmos_sensor(0x25b8, 0x903F);
	write_cmos_sensor(0x25ba, 0x0100);
	write_cmos_sensor(0x25bc, 0x23E1);
	write_cmos_sensor(0x25be, 0x40B2);
	write_cmos_sensor(0x25c0, 0x0005);
	write_cmos_sensor(0x25c2, 0x7600);
	write_cmos_sensor(0x25c4, 0x4382);
	write_cmos_sensor(0x25c6, 0x7602);
	write_cmos_sensor(0x25c8, 0x0262);
	write_cmos_sensor(0x25ca, 0x0000);
	write_cmos_sensor(0x25cc, 0x0222);
	write_cmos_sensor(0x25ce, 0x0000);
	write_cmos_sensor(0x25d0, 0x0262);
	write_cmos_sensor(0x25d2, 0x0000);
	write_cmos_sensor(0x25d4, 0x0260);
	write_cmos_sensor(0x25d6, 0x0000);
	write_cmos_sensor(0x25d8, 0x425F);
	write_cmos_sensor(0x25da, 0x0186);
	write_cmos_sensor(0x25dc, 0x4F4C);
	write_cmos_sensor(0x25de, 0x421B);
	write_cmos_sensor(0x25e0, 0x018A);
	write_cmos_sensor(0x25e2, 0x93D2);
	write_cmos_sensor(0x25e4, 0x018F);
	write_cmos_sensor(0x25e6, 0x244D);
	write_cmos_sensor(0x25e8, 0x425F);
	write_cmos_sensor(0x25ea, 0x018F);
	write_cmos_sensor(0x25ec, 0x4F4D);
	write_cmos_sensor(0x25ee, 0x4308);
	write_cmos_sensor(0x25f0, 0x431F);
	write_cmos_sensor(0x25f2, 0x480E);
	write_cmos_sensor(0x25f4, 0x930E);
	write_cmos_sensor(0x25f6, 0x2403);
	write_cmos_sensor(0x25f8, 0x5F0F);
	write_cmos_sensor(0x25fa, 0x831E);
	write_cmos_sensor(0x25fc, 0x23FD);
	write_cmos_sensor(0x25fe, 0xFC0F);
	write_cmos_sensor(0x2600, 0x242E);
	write_cmos_sensor(0x2602, 0x430F);
	write_cmos_sensor(0x2604, 0x9D0F);
	write_cmos_sensor(0x2606, 0x2C2B);
	write_cmos_sensor(0x2608, 0x4B82);
	write_cmos_sensor(0x260a, 0x7600);
	write_cmos_sensor(0x260c, 0x4882);
	write_cmos_sensor(0x260e, 0x7602);
	write_cmos_sensor(0x2610, 0x4C82);
	write_cmos_sensor(0x2612, 0x7604);
	write_cmos_sensor(0x2614, 0x0264);
	write_cmos_sensor(0x2616, 0x0000);
	write_cmos_sensor(0x2618, 0x0224);
	write_cmos_sensor(0x261a, 0x0000);
	write_cmos_sensor(0x261c, 0x0264);
	write_cmos_sensor(0x261e, 0x0000);
	write_cmos_sensor(0x2620, 0x0260);
	write_cmos_sensor(0x2622, 0x0000);
	write_cmos_sensor(0x2624, 0x0268);
	write_cmos_sensor(0x2626, 0x0000);
	write_cmos_sensor(0x2628, 0x0C18);
	write_cmos_sensor(0x262a, 0x02E8);
	write_cmos_sensor(0x262c, 0x0000);
	write_cmos_sensor(0x262e, 0x0C30);
	write_cmos_sensor(0x2630, 0x02A8);
	write_cmos_sensor(0x2632, 0x0000);
	write_cmos_sensor(0x2634, 0x0C30);
	write_cmos_sensor(0x2636, 0x0C30);
	write_cmos_sensor(0x2638, 0x0C30);
	write_cmos_sensor(0x263a, 0x0C30);
	write_cmos_sensor(0x263c, 0x0C30);
	write_cmos_sensor(0x263e, 0x0C30);
	write_cmos_sensor(0x2640, 0x0C30);
	write_cmos_sensor(0x2642, 0x0C30);
	write_cmos_sensor(0x2644, 0x0C00);
	write_cmos_sensor(0x2646, 0x02E8);
	write_cmos_sensor(0x2648, 0x0000);
	write_cmos_sensor(0x264a, 0x0C30);
	write_cmos_sensor(0x264c, 0x0268);
	write_cmos_sensor(0x264e, 0x0000);
	write_cmos_sensor(0x2650, 0x0C18);
	write_cmos_sensor(0x2652, 0x0260);
	write_cmos_sensor(0x2654, 0x0000);
	write_cmos_sensor(0x2656, 0x0C18);
	write_cmos_sensor(0x2658, 0x531F);
	write_cmos_sensor(0x265a, 0x9D0F);
	write_cmos_sensor(0x265c, 0x2BD5);
	write_cmos_sensor(0x265e, 0x5318);
	write_cmos_sensor(0x2660, 0x9238);
	write_cmos_sensor(0x2662, 0x2BC6);
	write_cmos_sensor(0x2664, 0x0261);
	write_cmos_sensor(0x2666, 0x0000);
	write_cmos_sensor(0x2668, 0x12B0);
	write_cmos_sensor(0x266a, 0xFD70);
	write_cmos_sensor(0x266c, 0x4A0F);
	write_cmos_sensor(0x266e, 0x12B0);
	write_cmos_sensor(0x2670, 0xFD82);
	write_cmos_sensor(0x2672, 0x421F);
	write_cmos_sensor(0x2674, 0x7606);
	write_cmos_sensor(0x2676, 0x4FC2);
	write_cmos_sensor(0x2678, 0x0188);
	write_cmos_sensor(0x267a, 0x4B0A);
	write_cmos_sensor(0x267c, 0x0261);
	write_cmos_sensor(0x267e, 0x0000);
	write_cmos_sensor(0x2680, 0x3F7F);
	write_cmos_sensor(0x2682, 0x432D);
	write_cmos_sensor(0x2684, 0x3FB4);
	write_cmos_sensor(0x2686, 0x421F);
	write_cmos_sensor(0x2688, 0x018A);
	write_cmos_sensor(0x268a, 0x12B0);
	write_cmos_sensor(0x268c, 0xFD82);
	write_cmos_sensor(0x268e, 0x421F);
	write_cmos_sensor(0x2690, 0x7606);
	write_cmos_sensor(0x2692, 0x4FC2);
	write_cmos_sensor(0x2694, 0x0188);
	write_cmos_sensor(0x2696, 0x0261);
	write_cmos_sensor(0x2698, 0x0000);
	write_cmos_sensor(0x269a, 0x3F72);
	write_cmos_sensor(0x269c, 0x4382);
	write_cmos_sensor(0x269e, 0x0B88);
	write_cmos_sensor(0x26a0, 0x0C0A);
	write_cmos_sensor(0x26a2, 0x4382);
	write_cmos_sensor(0x26a4, 0x0B8A);
	write_cmos_sensor(0x26a6, 0x0C0A);
	write_cmos_sensor(0x26a8, 0x40B2);
	write_cmos_sensor(0x26aa, 0x000C);
	write_cmos_sensor(0x26ac, 0x0B8C);
	write_cmos_sensor(0x26ae, 0x0C0A);
	write_cmos_sensor(0x26b0, 0x40B2);
	write_cmos_sensor(0x26b2, 0xB5E1);
	write_cmos_sensor(0x26b4, 0x0B8E);
	write_cmos_sensor(0x26b6, 0x0C0A);
	write_cmos_sensor(0x26b8, 0x40B2);
	write_cmos_sensor(0x26ba, 0x641C);
	write_cmos_sensor(0x26bc, 0x0B92);
	write_cmos_sensor(0x26be, 0x0C0A);
	write_cmos_sensor(0x26c0, 0x43C2);
	write_cmos_sensor(0x26c2, 0x003D);
	write_cmos_sensor(0x26c4, 0x4030);
	write_cmos_sensor(0x26c6, 0xF02E);
	write_cmos_sensor(0x26c8, 0x5231);
	write_cmos_sensor(0x26ca, 0x4030);
	write_cmos_sensor(0x26cc, 0xFD9E);
	write_cmos_sensor(0x26ce, 0xE3B2);
	write_cmos_sensor(0x26d0, 0x740E);
	write_cmos_sensor(0x26d2, 0x425F);
	write_cmos_sensor(0x26d4, 0x0118);
	write_cmos_sensor(0x26d6, 0xF37F);
	write_cmos_sensor(0x26d8, 0x4F82);
	write_cmos_sensor(0x26da, 0x8098);
	write_cmos_sensor(0x26dc, 0x930F);
	write_cmos_sensor(0x26de, 0x2005);
	write_cmos_sensor(0x26e0, 0x93C2);
	write_cmos_sensor(0x26e2, 0x0A82);
	write_cmos_sensor(0x26e4, 0x2402);
	write_cmos_sensor(0x26e6, 0x4392);
	write_cmos_sensor(0x26e8, 0x80CC);
	write_cmos_sensor(0x26ea, 0x9382);
	write_cmos_sensor(0x26ec, 0x8098);
	write_cmos_sensor(0x26ee, 0x2002);
	write_cmos_sensor(0x26f0, 0x4392);
	write_cmos_sensor(0x26f2, 0x8070);
	write_cmos_sensor(0x26f4, 0x421F);
	write_cmos_sensor(0x26f6, 0x710E);
	write_cmos_sensor(0x26f8, 0x93A2);
	write_cmos_sensor(0x26fa, 0x7110);
	write_cmos_sensor(0x26fc, 0x2411);
	write_cmos_sensor(0x26fe, 0x9382);
	write_cmos_sensor(0x2700, 0x710E);
	write_cmos_sensor(0x2702, 0x240C);
	write_cmos_sensor(0x2704, 0x5292);
	write_cmos_sensor(0x2706, 0x809E);
	write_cmos_sensor(0x2708, 0x7110);
	write_cmos_sensor(0x270a, 0x4382);
	write_cmos_sensor(0x270c, 0x740E);
	write_cmos_sensor(0x270e, 0x9382);
	write_cmos_sensor(0x2710, 0x80B6);
	write_cmos_sensor(0x2712, 0x2402);
	write_cmos_sensor(0x2714, 0x4392);
	write_cmos_sensor(0x2716, 0x740E);
	write_cmos_sensor(0x2718, 0x4392);
	write_cmos_sensor(0x271a, 0x80B8);
	write_cmos_sensor(0x271c, 0x430F);
	write_cmos_sensor(0x271e, 0x4130);
	write_cmos_sensor(0x2720, 0xF31F);
	write_cmos_sensor(0x2722, 0x27ED);
	write_cmos_sensor(0x2724, 0x40B2);
	write_cmos_sensor(0x2726, 0x0003);
	write_cmos_sensor(0x2728, 0x7110);
	write_cmos_sensor(0x272a, 0x431F);
	write_cmos_sensor(0x272c, 0x4130);
	write_cmos_sensor(0x272e, 0x4F0E);
	write_cmos_sensor(0x2730, 0x421D);
	write_cmos_sensor(0x2732, 0x8070);
	write_cmos_sensor(0x2734, 0x425F);
	write_cmos_sensor(0x2736, 0x0118);
	write_cmos_sensor(0x2738, 0xF37F);
	write_cmos_sensor(0x273a, 0x903E);
	write_cmos_sensor(0x273c, 0x0003);
	write_cmos_sensor(0x273e, 0x2405);
	write_cmos_sensor(0x2740, 0x931E);
	write_cmos_sensor(0x2742, 0x2403);
	write_cmos_sensor(0x2744, 0x0B00);
	write_cmos_sensor(0x2746, 0x7302);
	write_cmos_sensor(0x2748, 0x0384);
	write_cmos_sensor(0x274a, 0x930F);
	write_cmos_sensor(0x274c, 0x241A);
	write_cmos_sensor(0x274e, 0x930D);
	write_cmos_sensor(0x2750, 0x2018);
	write_cmos_sensor(0x2752, 0x9382);
	write_cmos_sensor(0x2754, 0x7308);
	write_cmos_sensor(0x2756, 0x2402);
	write_cmos_sensor(0x2758, 0x930E);
	write_cmos_sensor(0x275a, 0x2419);
	write_cmos_sensor(0x275c, 0x9382);
	write_cmos_sensor(0x275e, 0x7328);
	write_cmos_sensor(0x2760, 0x2402);
	write_cmos_sensor(0x2762, 0x931E);
	write_cmos_sensor(0x2764, 0x2414);
	write_cmos_sensor(0x2766, 0x9382);
	write_cmos_sensor(0x2768, 0x710E);
	write_cmos_sensor(0x276a, 0x2402);
	write_cmos_sensor(0x276c, 0x932E);
	write_cmos_sensor(0x276e, 0x240F);
	write_cmos_sensor(0x2770, 0x9382);
	write_cmos_sensor(0x2772, 0x7114);
	write_cmos_sensor(0x2774, 0x2402);
	write_cmos_sensor(0x2776, 0x922E);
	write_cmos_sensor(0x2778, 0x240A);
	write_cmos_sensor(0x277a, 0x903E);
	write_cmos_sensor(0x277c, 0x0003);
	write_cmos_sensor(0x277e, 0x23DA);
	write_cmos_sensor(0x2780, 0x3C06);
	write_cmos_sensor(0x2782, 0x43C2);
	write_cmos_sensor(0x2784, 0x0A80);
	write_cmos_sensor(0x2786, 0x0B00);
	write_cmos_sensor(0x2788, 0x7302);
	write_cmos_sensor(0x278a, 0xFFF0);
	write_cmos_sensor(0x278c, 0x3FD3);
	write_cmos_sensor(0x278e, 0x4F82);
	write_cmos_sensor(0x2790, 0x8098);
	write_cmos_sensor(0x2792, 0x431F);
	write_cmos_sensor(0x2794, 0x4130);
	write_cmos_sensor(0x2796, 0x120B);
	write_cmos_sensor(0x2798, 0x120A);
	write_cmos_sensor(0x279a, 0x1209);
	write_cmos_sensor(0x279c, 0x1208);
	write_cmos_sensor(0x279e, 0x1207);
	write_cmos_sensor(0x27a0, 0x1206);
	write_cmos_sensor(0x27a2, 0x1205);
	write_cmos_sensor(0x27a4, 0x1204);
	write_cmos_sensor(0x27a6, 0x8221);
	write_cmos_sensor(0x27a8, 0x403B);
	write_cmos_sensor(0x27aa, 0x0016);
	write_cmos_sensor(0x27ac, 0x510B);
	write_cmos_sensor(0x27ae, 0x4F08);
	write_cmos_sensor(0x27b0, 0x4E09);
	write_cmos_sensor(0x27b2, 0x4BA1);
	write_cmos_sensor(0x27b4, 0x0000);
	write_cmos_sensor(0x27b6, 0x4B1A);
	write_cmos_sensor(0x27b8, 0x0002);
	write_cmos_sensor(0x27ba, 0x4B91);
	write_cmos_sensor(0x27bc, 0x0004);
	write_cmos_sensor(0x27be, 0x0002);
	write_cmos_sensor(0x27c0, 0x4304);
	write_cmos_sensor(0x27c2, 0x4305);
	write_cmos_sensor(0x27c4, 0x4306);
	write_cmos_sensor(0x27c6, 0x4307);
	write_cmos_sensor(0x27c8, 0x9382);
	write_cmos_sensor(0x27ca, 0x80B2);
	write_cmos_sensor(0x27cc, 0x2425);
	write_cmos_sensor(0x27ce, 0x438A);
	write_cmos_sensor(0x27d0, 0x0000);
	write_cmos_sensor(0x27d2, 0x430B);
	write_cmos_sensor(0x27d4, 0x4B0F);
	write_cmos_sensor(0x27d6, 0x5F0F);
	write_cmos_sensor(0x27d8, 0x5F0F);
	write_cmos_sensor(0x27da, 0x580F);
	write_cmos_sensor(0x27dc, 0x4C8F);
	write_cmos_sensor(0x27de, 0x0000);
	write_cmos_sensor(0x27e0, 0x4D8F);
	write_cmos_sensor(0x27e2, 0x0002);
	write_cmos_sensor(0x27e4, 0x4B0F);
	write_cmos_sensor(0x27e6, 0x5F0F);
	write_cmos_sensor(0x27e8, 0x590F);
	write_cmos_sensor(0x27ea, 0x41AF);
	write_cmos_sensor(0x27ec, 0x0000);
	write_cmos_sensor(0x27ee, 0x531B);
	write_cmos_sensor(0x27f0, 0x923B);
	write_cmos_sensor(0x27f2, 0x2BF0);
	write_cmos_sensor(0x27f4, 0x430B);
	write_cmos_sensor(0x27f6, 0x4B0F);
	write_cmos_sensor(0x27f8, 0x5F0F);
	write_cmos_sensor(0x27fa, 0x5F0F);
	write_cmos_sensor(0x27fc, 0x580F);
	write_cmos_sensor(0x27fe, 0x5F34);
	write_cmos_sensor(0x2800, 0x6F35);
	write_cmos_sensor(0x2802, 0x4B0F);
	write_cmos_sensor(0x2804, 0x5F0F);
	write_cmos_sensor(0x2806, 0x590F);
	write_cmos_sensor(0x2808, 0x4F2E);
	write_cmos_sensor(0x280a, 0x430F);
	write_cmos_sensor(0x280c, 0x5E06);
	write_cmos_sensor(0x280e, 0x6F07);
	write_cmos_sensor(0x2810, 0x531B);
	write_cmos_sensor(0x2812, 0x923B);
	write_cmos_sensor(0x2814, 0x2BF0);
	write_cmos_sensor(0x2816, 0x3C18);
	write_cmos_sensor(0x2818, 0x4A2E);
	write_cmos_sensor(0x281a, 0x4E0F);
	write_cmos_sensor(0x281c, 0x5F0F);
	write_cmos_sensor(0x281e, 0x5F0F);
	write_cmos_sensor(0x2820, 0x580F);
	write_cmos_sensor(0x2822, 0x4C8F);
	write_cmos_sensor(0x2824, 0x0000);
	write_cmos_sensor(0x2826, 0x4D8F);
	write_cmos_sensor(0x2828, 0x0002);
	write_cmos_sensor(0x282a, 0x5E0E);
	write_cmos_sensor(0x282c, 0x590E);
	write_cmos_sensor(0x282e, 0x41AE);
	write_cmos_sensor(0x2830, 0x0000);
	write_cmos_sensor(0x2832, 0x4A2F);
	write_cmos_sensor(0x2834, 0x903F);
	write_cmos_sensor(0x2836, 0x0007);
	write_cmos_sensor(0x2838, 0x2404);
	write_cmos_sensor(0x283a, 0x531F);
	write_cmos_sensor(0x283c, 0x4F8A);
	write_cmos_sensor(0x283e, 0x0000);
	write_cmos_sensor(0x2840, 0x3FD9);
	write_cmos_sensor(0x2842, 0x438A);
	write_cmos_sensor(0x2844, 0x0000);
	write_cmos_sensor(0x2846, 0x3FD6);
	write_cmos_sensor(0x2848, 0x440C);
	write_cmos_sensor(0x284a, 0x450D);
	write_cmos_sensor(0x284c, 0x460A);
	write_cmos_sensor(0x284e, 0x470B);
	write_cmos_sensor(0x2850, 0x12B0);
	write_cmos_sensor(0x2852, 0xFDF4);
	write_cmos_sensor(0x2854, 0x4C08);
	write_cmos_sensor(0x2856, 0x4D09);
	write_cmos_sensor(0x2858, 0x4C0E);
	write_cmos_sensor(0x285a, 0x430F);
	write_cmos_sensor(0x285c, 0x4E0A);
	write_cmos_sensor(0x285e, 0x4F0B);
	write_cmos_sensor(0x2860, 0x460C);
	write_cmos_sensor(0x2862, 0x470D);
	write_cmos_sensor(0x2864, 0x12B0);
	write_cmos_sensor(0x2866, 0xFDB8);
	write_cmos_sensor(0x2868, 0x8E04);
	write_cmos_sensor(0x286a, 0x7F05);
	write_cmos_sensor(0x286c, 0x440E);
	write_cmos_sensor(0x286e, 0x450F);
	write_cmos_sensor(0x2870, 0xC312);
	write_cmos_sensor(0x2872, 0x100F);
	write_cmos_sensor(0x2874, 0x100E);
	write_cmos_sensor(0x2876, 0x110F);
	write_cmos_sensor(0x2878, 0x100E);
	write_cmos_sensor(0x287a, 0x110F);
	write_cmos_sensor(0x287c, 0x100E);
	write_cmos_sensor(0x287e, 0x411D);
	write_cmos_sensor(0x2880, 0x0002);
	write_cmos_sensor(0x2882, 0x4E8D);
	write_cmos_sensor(0x2884, 0x0000);
	write_cmos_sensor(0x2886, 0x480F);
	write_cmos_sensor(0x2888, 0x5221);
	write_cmos_sensor(0x288a, 0x4134);
	write_cmos_sensor(0x288c, 0x4135);
	write_cmos_sensor(0x288e, 0x4136);
	write_cmos_sensor(0x2890, 0x4137);
	write_cmos_sensor(0x2892, 0x4138);
	write_cmos_sensor(0x2894, 0x4139);
	write_cmos_sensor(0x2896, 0x413A);
	write_cmos_sensor(0x2898, 0x413B);
	write_cmos_sensor(0x289a, 0x4130);
	write_cmos_sensor(0x289c, 0x120A);
	write_cmos_sensor(0x289e, 0x4F0D);
	write_cmos_sensor(0x28a0, 0x4E0C);
	write_cmos_sensor(0x28a2, 0x425F);
	write_cmos_sensor(0x28a4, 0x00BA);
	write_cmos_sensor(0x28a6, 0x4F4A);
	write_cmos_sensor(0x28a8, 0x503A);
	write_cmos_sensor(0x28aa, 0x0010);
	write_cmos_sensor(0x28ac, 0x931D);
	write_cmos_sensor(0x28ae, 0x242B);
	write_cmos_sensor(0x28b0, 0x932D);
	write_cmos_sensor(0x28b2, 0x2421);
	write_cmos_sensor(0x28b4, 0x903D);
	write_cmos_sensor(0x28b6, 0x0003);
	write_cmos_sensor(0x28b8, 0x2418);
	write_cmos_sensor(0x28ba, 0x922D);
	write_cmos_sensor(0x28bc, 0x2413);
	write_cmos_sensor(0x28be, 0x903D);
	write_cmos_sensor(0x28c0, 0x0005);
	write_cmos_sensor(0x28c2, 0x2407);
	write_cmos_sensor(0x28c4, 0x903D);
	write_cmos_sensor(0x28c6, 0x0006);
	write_cmos_sensor(0x28c8, 0x2028);
	write_cmos_sensor(0x28ca, 0xC312);
	write_cmos_sensor(0x28cc, 0x100A);
	write_cmos_sensor(0x28ce, 0x110A);
	write_cmos_sensor(0x28d0, 0x3C24);
	write_cmos_sensor(0x28d2, 0x4A0E);
	write_cmos_sensor(0x28d4, 0xC312);
	write_cmos_sensor(0x28d6, 0x100E);
	write_cmos_sensor(0x28d8, 0x110E);
	write_cmos_sensor(0x28da, 0x4E0F);
	write_cmos_sensor(0x28dc, 0x110F);
	write_cmos_sensor(0x28de, 0x4E0A);
	write_cmos_sensor(0x28e0, 0x5F0A);
	write_cmos_sensor(0x28e2, 0x3C1B);
	write_cmos_sensor(0x28e4, 0xC312);
	write_cmos_sensor(0x28e6, 0x100A);
	write_cmos_sensor(0x28e8, 0x3C18);
	write_cmos_sensor(0x28ea, 0x4A0E);
	write_cmos_sensor(0x28ec, 0xC312);
	write_cmos_sensor(0x28ee, 0x100E);
	write_cmos_sensor(0x28f0, 0x4E0F);
	write_cmos_sensor(0x28f2, 0x110F);
	write_cmos_sensor(0x28f4, 0x3FF3);
	write_cmos_sensor(0x28f6, 0x4A0F);
	write_cmos_sensor(0x28f8, 0xC312);
	write_cmos_sensor(0x28fa, 0x100F);
	write_cmos_sensor(0x28fc, 0x4F0E);
	write_cmos_sensor(0x28fe, 0x110E);
	write_cmos_sensor(0x2900, 0x4F0A);
	write_cmos_sensor(0x2902, 0x5E0A);
	write_cmos_sensor(0x2904, 0x3C0A);
	write_cmos_sensor(0x2906, 0x4A0F);
	write_cmos_sensor(0x2908, 0xC312);
	write_cmos_sensor(0x290a, 0x100F);
	write_cmos_sensor(0x290c, 0x4F0E);
	write_cmos_sensor(0x290e, 0x110E);
	write_cmos_sensor(0x2910, 0x4F0A);
	write_cmos_sensor(0x2912, 0x5E0A);
	write_cmos_sensor(0x2914, 0x4E0F);
	write_cmos_sensor(0x2916, 0x110F);
	write_cmos_sensor(0x2918, 0x3FE3);
	write_cmos_sensor(0x291a, 0x12B0);
	write_cmos_sensor(0x291c, 0xFDA2);
	write_cmos_sensor(0x291e, 0x4E0F);
	write_cmos_sensor(0x2920, 0xC312);
	write_cmos_sensor(0x2922, 0x100F);
	write_cmos_sensor(0x2924, 0x110F);
	write_cmos_sensor(0x2926, 0x110F);
	write_cmos_sensor(0x2928, 0x110F);
	write_cmos_sensor(0x292a, 0x413A);
	write_cmos_sensor(0x292c, 0x4130);
	write_cmos_sensor(0x292e, 0x120B);
	write_cmos_sensor(0x2930, 0x120A);
	write_cmos_sensor(0x2932, 0x1209);
	write_cmos_sensor(0x2934, 0x1208);
	write_cmos_sensor(0x2936, 0x425F);
	write_cmos_sensor(0x2938, 0x0080);
	write_cmos_sensor(0x293a, 0xF36F);
	write_cmos_sensor(0x293c, 0x4F0E);
	write_cmos_sensor(0x293e, 0xF32E);
	write_cmos_sensor(0x2940, 0x4E82);
	write_cmos_sensor(0x2942, 0x80B6);
	write_cmos_sensor(0x2944, 0xB3D2);
	write_cmos_sensor(0x2946, 0x0786);
	write_cmos_sensor(0x2948, 0x2402);
	write_cmos_sensor(0x294a, 0x4392);
	write_cmos_sensor(0x294c, 0x7002);
	write_cmos_sensor(0x294e, 0x4392);
	write_cmos_sensor(0x2950, 0x80B8);
	write_cmos_sensor(0x2952, 0x4382);
	write_cmos_sensor(0x2954, 0x740E);
	write_cmos_sensor(0x2956, 0x9382);
	write_cmos_sensor(0x2958, 0x80B6);
	write_cmos_sensor(0x295a, 0x2402);
	write_cmos_sensor(0x295c, 0x4392);
	write_cmos_sensor(0x295e, 0x740E);
	write_cmos_sensor(0x2960, 0x93C2);
	write_cmos_sensor(0x2962, 0x00C6);
	write_cmos_sensor(0x2964, 0x2406);
	write_cmos_sensor(0x2966, 0xB392);
	write_cmos_sensor(0x2968, 0x732A);
	write_cmos_sensor(0x296a, 0x2403);
	write_cmos_sensor(0x296c, 0xB3D2);
	write_cmos_sensor(0x296e, 0x00C7);
	write_cmos_sensor(0x2970, 0x2412);
	write_cmos_sensor(0x2972, 0x4292);
	write_cmos_sensor(0x2974, 0x01A8);
	write_cmos_sensor(0x2976, 0x0688);
	write_cmos_sensor(0x2978, 0x4292);
	write_cmos_sensor(0x297a, 0x01AA);
	write_cmos_sensor(0x297c, 0x068A);
	write_cmos_sensor(0x297e, 0x4292);
	write_cmos_sensor(0x2980, 0x01AC);
	write_cmos_sensor(0x2982, 0x068C);
	write_cmos_sensor(0x2984, 0x4292);
	write_cmos_sensor(0x2986, 0x01AE);
	write_cmos_sensor(0x2988, 0x068E);
	write_cmos_sensor(0x298a, 0x4292);
	write_cmos_sensor(0x298c, 0x0190);
	write_cmos_sensor(0x298e, 0x0A92);
	write_cmos_sensor(0x2990, 0x4292);
	write_cmos_sensor(0x2992, 0x0192);
	write_cmos_sensor(0x2994, 0x0A94);
	write_cmos_sensor(0x2996, 0x430E);
	write_cmos_sensor(0x2998, 0x425F);
	write_cmos_sensor(0x299a, 0x00C7);
	write_cmos_sensor(0x299c, 0xF35F);
	write_cmos_sensor(0x299e, 0xF37F);
	write_cmos_sensor(0x29a0, 0xF21F);
	write_cmos_sensor(0x29a2, 0x732A);
	write_cmos_sensor(0x29a4, 0x200C);
	write_cmos_sensor(0x29a6, 0xB3D2);
	write_cmos_sensor(0x29a8, 0x00C7);
	write_cmos_sensor(0x29aa, 0x2003);
	write_cmos_sensor(0x29ac, 0xB392);
	write_cmos_sensor(0x29ae, 0x80A0);
	write_cmos_sensor(0x29b0, 0x2006);
	write_cmos_sensor(0x29b2, 0xB3A2);
	write_cmos_sensor(0x29b4, 0x732A);
	write_cmos_sensor(0x29b6, 0x2003);
	write_cmos_sensor(0x29b8, 0x9382);
	write_cmos_sensor(0x29ba, 0x8094);
	write_cmos_sensor(0x29bc, 0x2401);
	write_cmos_sensor(0x29be, 0x431E);
	write_cmos_sensor(0x29c0, 0x4E82);
	write_cmos_sensor(0x29c2, 0x80B2);
	write_cmos_sensor(0x29c4, 0x930E);
	write_cmos_sensor(0x29c6, 0x25B6);
	write_cmos_sensor(0x29c8, 0x4382);
	write_cmos_sensor(0x29ca, 0x80BA);
	write_cmos_sensor(0x29cc, 0x421F);
	write_cmos_sensor(0x29ce, 0x732A);
	write_cmos_sensor(0x29d0, 0xF31F);
	write_cmos_sensor(0x29d2, 0x4F82);
	write_cmos_sensor(0x29d4, 0x80A0);
	write_cmos_sensor(0x29d6, 0x425F);
	write_cmos_sensor(0x29d8, 0x008C);
	write_cmos_sensor(0x29da, 0x4FC2);
	write_cmos_sensor(0x29dc, 0x8092);
	write_cmos_sensor(0x29de, 0x43C2);
	write_cmos_sensor(0x29e0, 0x8093);
	write_cmos_sensor(0x29e2, 0x425F);
	write_cmos_sensor(0x29e4, 0x009E);
	write_cmos_sensor(0x29e6, 0x4F48);
	write_cmos_sensor(0x29e8, 0x425F);
	write_cmos_sensor(0x29ea, 0x009F);
	write_cmos_sensor(0x29ec, 0xF37F);
	write_cmos_sensor(0x29ee, 0x5F08);
	write_cmos_sensor(0x29f0, 0x1108);
	write_cmos_sensor(0x29f2, 0x1108);
	write_cmos_sensor(0x29f4, 0x425F);
	write_cmos_sensor(0x29f6, 0x00B2);
	write_cmos_sensor(0x29f8, 0x4F49);
	write_cmos_sensor(0x29fa, 0x425F);
	write_cmos_sensor(0x29fc, 0x00B3);
	write_cmos_sensor(0x29fe, 0xF37F);
	write_cmos_sensor(0x2a00, 0x5F09);
	write_cmos_sensor(0x2a02, 0x1109);
	write_cmos_sensor(0x2a04, 0x1109);
	write_cmos_sensor(0x2a06, 0x425F);
	write_cmos_sensor(0x2a08, 0x00BA);
	write_cmos_sensor(0x2a0a, 0x4F4C);
	write_cmos_sensor(0x2a0c, 0x407A);
	write_cmos_sensor(0x2a0e, 0x001C);
	write_cmos_sensor(0x2a10, 0x12B0);
	write_cmos_sensor(0x2a12, 0xFDD8);
	write_cmos_sensor(0x2a14, 0x934C);
	write_cmos_sensor(0x2a16, 0x257A);
	write_cmos_sensor(0x2a18, 0x403E);
	write_cmos_sensor(0x2a1a, 0x0080);
	write_cmos_sensor(0x2a1c, 0x4E0F);
	write_cmos_sensor(0x2a1e, 0xF37F);
	write_cmos_sensor(0x2a20, 0x108F);
	write_cmos_sensor(0x2a22, 0xD03F);
	write_cmos_sensor(0x2a24, 0x008B);
	write_cmos_sensor(0x2a26, 0x4F82);
	write_cmos_sensor(0x2a28, 0x0B88);
	write_cmos_sensor(0x2a2a, 0x0C0A);
	write_cmos_sensor(0x2a2c, 0x403F);
	write_cmos_sensor(0x2a2e, 0x00BA);
	write_cmos_sensor(0x2a30, 0x4F6E);
	write_cmos_sensor(0x2a32, 0x403C);
	write_cmos_sensor(0x2a34, 0x0813);
	write_cmos_sensor(0x2a36, 0x403D);
	write_cmos_sensor(0x2a38, 0x007F);
	write_cmos_sensor(0x2a3a, 0x90FF);
	write_cmos_sensor(0x2a3c, 0x0011);
	write_cmos_sensor(0x2a3e, 0x0000);
	write_cmos_sensor(0x2a40, 0x2D13);
	write_cmos_sensor(0x2a42, 0x403C);
	write_cmos_sensor(0x2a44, 0x1009);
	write_cmos_sensor(0x2a46, 0x430D);
	write_cmos_sensor(0x2a48, 0x425E);
	write_cmos_sensor(0x2a4a, 0x00BA);
	write_cmos_sensor(0x2a4c, 0x4E4F);
	write_cmos_sensor(0x2a4e, 0x108F);
	write_cmos_sensor(0x2a50, 0xDD0F);
	write_cmos_sensor(0x2a52, 0x4F82);
	write_cmos_sensor(0x2a54, 0x0B90);
	write_cmos_sensor(0x2a56, 0x0C0A);
	write_cmos_sensor(0x2a58, 0x4C82);
	write_cmos_sensor(0x2a5a, 0x0B8A);
	write_cmos_sensor(0x2a5c, 0x0C0A);
	write_cmos_sensor(0x2a5e, 0x425F);
	write_cmos_sensor(0x2a60, 0x0C87);
	write_cmos_sensor(0x2a62, 0x4F4E);
	write_cmos_sensor(0x2a64, 0x425F);
	write_cmos_sensor(0x2a66, 0x0C88);
	write_cmos_sensor(0x2a68, 0xF37F);
	write_cmos_sensor(0x2a6a, 0x12B0);
	write_cmos_sensor(0x2a6c, 0xF89C);
	write_cmos_sensor(0x2a6e, 0x4F82);
	write_cmos_sensor(0x2a70, 0x0C8C);
	write_cmos_sensor(0x2a72, 0x425F);
	write_cmos_sensor(0x2a74, 0x0C85);
	write_cmos_sensor(0x2a76, 0x4F4E);
	write_cmos_sensor(0x2a78, 0x425F);
	write_cmos_sensor(0x2a7a, 0x0C89);
	write_cmos_sensor(0x2a7c, 0xF37F);
	write_cmos_sensor(0x2a7e, 0x12B0);
	write_cmos_sensor(0x2a80, 0xF89C);
	write_cmos_sensor(0x2a82, 0x4F82);
	write_cmos_sensor(0x2a84, 0x0C8A);
	write_cmos_sensor(0x2a86, 0x425E);
	write_cmos_sensor(0x2a88, 0x00B7);
	write_cmos_sensor(0x2a8a, 0x5E4E);
	write_cmos_sensor(0x2a8c, 0x4EC2);
	write_cmos_sensor(0x2a8e, 0x0CB0);
	write_cmos_sensor(0x2a90, 0x425F);
	write_cmos_sensor(0x2a92, 0x00B8);
	write_cmos_sensor(0x2a94, 0x5F4F);
	write_cmos_sensor(0x2a96, 0x4FC2);
	write_cmos_sensor(0x2a98, 0x0CB1);
	write_cmos_sensor(0x2a9a, 0x480E);
	write_cmos_sensor(0x2a9c, 0x5E0E);
	write_cmos_sensor(0x2a9e, 0x5E0E);
	write_cmos_sensor(0x2aa0, 0x5E0E);
	write_cmos_sensor(0x2aa2, 0x5E0E);
	write_cmos_sensor(0x2aa4, 0x490F);
	write_cmos_sensor(0x2aa6, 0x5F0F);
	write_cmos_sensor(0x2aa8, 0x5F0F);
	write_cmos_sensor(0x2aaa, 0x5F0F);
	write_cmos_sensor(0x2aac, 0x5F0F);
	write_cmos_sensor(0x2aae, 0x5F0F);
	write_cmos_sensor(0x2ab0, 0x5F0F);
	write_cmos_sensor(0x2ab2, 0x5F0F);
	write_cmos_sensor(0x2ab4, 0xDF0E);
	write_cmos_sensor(0x2ab6, 0x4E82);
	write_cmos_sensor(0x2ab8, 0x0A8E);
	write_cmos_sensor(0x2aba, 0xB229);
	write_cmos_sensor(0x2abc, 0x2401);
	write_cmos_sensor(0x2abe, 0x5339);
	write_cmos_sensor(0x2ac0, 0xB3E2);
	write_cmos_sensor(0x2ac2, 0x0080);
	write_cmos_sensor(0x2ac4, 0x2403);
	write_cmos_sensor(0x2ac6, 0x40F2);
	write_cmos_sensor(0x2ac8, 0x0003);
	write_cmos_sensor(0x2aca, 0x00B5);
	write_cmos_sensor(0x2acc, 0x40B2);
	write_cmos_sensor(0x2ace, 0x1000);
	write_cmos_sensor(0x2ad0, 0x7500);
	write_cmos_sensor(0x2ad2, 0x40B2);
	write_cmos_sensor(0x2ad4, 0x1001);
	write_cmos_sensor(0x2ad6, 0x7502);
	write_cmos_sensor(0x2ad8, 0x40B2);
	write_cmos_sensor(0x2ada, 0x0803);
	write_cmos_sensor(0x2adc, 0x7504);
	write_cmos_sensor(0x2ade, 0x40B2);
	write_cmos_sensor(0x2ae0, 0x080F);
	write_cmos_sensor(0x2ae2, 0x7506);
	write_cmos_sensor(0x2ae4, 0x40B2);
	write_cmos_sensor(0x2ae6, 0x6003);
	write_cmos_sensor(0x2ae8, 0x7508);
	write_cmos_sensor(0x2aea, 0x40B2);
	write_cmos_sensor(0x2aec, 0x0801);
	write_cmos_sensor(0x2aee, 0x750A);
	write_cmos_sensor(0x2af0, 0x40B2);
	write_cmos_sensor(0x2af2, 0x0800);
	write_cmos_sensor(0x2af4, 0x750C);
	write_cmos_sensor(0x2af6, 0x40B2);
	write_cmos_sensor(0x2af8, 0x1400);
	write_cmos_sensor(0x2afa, 0x750E);
	write_cmos_sensor(0x2afc, 0x403F);
	write_cmos_sensor(0x2afe, 0x0003);
	write_cmos_sensor(0x2b00, 0x12B0);
	write_cmos_sensor(0x2b02, 0xF72E);
	write_cmos_sensor(0x2b04, 0x421F);
	write_cmos_sensor(0x2b06, 0x0098);
	write_cmos_sensor(0x2b08, 0x821F);
	write_cmos_sensor(0x2b0a, 0x0092);
	write_cmos_sensor(0x2b0c, 0x531F);
	write_cmos_sensor(0x2b0e, 0xC312);
	write_cmos_sensor(0x2b10, 0x100F);
	write_cmos_sensor(0x2b12, 0x4F82);
	write_cmos_sensor(0x2b14, 0x0A86);
	write_cmos_sensor(0x2b16, 0x421F);
	write_cmos_sensor(0x2b18, 0x00AC);
	write_cmos_sensor(0x2b1a, 0x821F);
	write_cmos_sensor(0x2b1c, 0x00A6);
	write_cmos_sensor(0x2b1e, 0x531F);
	write_cmos_sensor(0x2b20, 0x4F82);
	write_cmos_sensor(0x2b22, 0x0A88);
	write_cmos_sensor(0x2b24, 0xB0B2);
	write_cmos_sensor(0x2b26, 0x0010);
	write_cmos_sensor(0x2b28, 0x0A84);
	write_cmos_sensor(0x2b2a, 0x248F);
	write_cmos_sensor(0x2b2c, 0x421E);
	write_cmos_sensor(0x2b2e, 0x068C);
	write_cmos_sensor(0x2b30, 0xC312);
	write_cmos_sensor(0x2b32, 0x100E);
	write_cmos_sensor(0x2b34, 0x4E82);
	write_cmos_sensor(0x2b36, 0x0782);
	write_cmos_sensor(0x2b38, 0x4292);
	write_cmos_sensor(0x2b3a, 0x068E);
	write_cmos_sensor(0x2b3c, 0x0784);
	write_cmos_sensor(0x2b3e, 0xB3D2);
	write_cmos_sensor(0x2b40, 0x0CB6);
	write_cmos_sensor(0x2b42, 0x2418);
	write_cmos_sensor(0x2b44, 0x421A);
	write_cmos_sensor(0x2b46, 0x0CB8);
	write_cmos_sensor(0x2b48, 0x430B);
	write_cmos_sensor(0x2b4a, 0x425F);
	write_cmos_sensor(0x2b4c, 0x0CBA);
	write_cmos_sensor(0x2b4e, 0x4F4E);
	write_cmos_sensor(0x2b50, 0x430F);
	write_cmos_sensor(0x2b52, 0x4E0F);
	write_cmos_sensor(0x2b54, 0x430E);
	write_cmos_sensor(0x2b56, 0xDE0A);
	write_cmos_sensor(0x2b58, 0xDF0B);
	write_cmos_sensor(0x2b5a, 0x421F);
	write_cmos_sensor(0x2b5c, 0x0CBC);
	write_cmos_sensor(0x2b5e, 0x4F0C);
	write_cmos_sensor(0x2b60, 0x430D);
	write_cmos_sensor(0x2b62, 0x421F);
	write_cmos_sensor(0x2b64, 0x0CBE);
	write_cmos_sensor(0x2b66, 0x430E);
	write_cmos_sensor(0x2b68, 0xDE0C);
	write_cmos_sensor(0x2b6a, 0xDF0D);
	write_cmos_sensor(0x2b6c, 0x12B0);
	write_cmos_sensor(0x2b6e, 0xFDF4);
	write_cmos_sensor(0x2b70, 0x4C82);
	write_cmos_sensor(0x2b72, 0x0194);
	write_cmos_sensor(0x2b74, 0xB2A2);
	write_cmos_sensor(0x2b76, 0x0A84);
	write_cmos_sensor(0x2b78, 0x2412);
	write_cmos_sensor(0x2b7a, 0x421E);
	write_cmos_sensor(0x2b7c, 0x0A96);
	write_cmos_sensor(0x2b7e, 0xC312);
	write_cmos_sensor(0x2b80, 0x100E);
	write_cmos_sensor(0x2b82, 0x110E);
	write_cmos_sensor(0x2b84, 0x110E);
	write_cmos_sensor(0x2b86, 0x43C2);
	write_cmos_sensor(0x2b88, 0x0A98);
	write_cmos_sensor(0x2b8a, 0x431D);
	write_cmos_sensor(0x2b8c, 0x4E0F);
	write_cmos_sensor(0x2b8e, 0x9F82);
	write_cmos_sensor(0x2b90, 0x0194);
	write_cmos_sensor(0x2b92, 0x2850);
	write_cmos_sensor(0x2b94, 0x5E0F);
	write_cmos_sensor(0x2b96, 0x531D);
	write_cmos_sensor(0x2b98, 0x903D);
	write_cmos_sensor(0x2b9a, 0x0009);
	write_cmos_sensor(0x2b9c, 0x2BF8);
	write_cmos_sensor(0x2b9e, 0x4292);
	write_cmos_sensor(0x2ba0, 0x0084);
	write_cmos_sensor(0x2ba2, 0x7524);
	write_cmos_sensor(0x2ba4, 0x4292);
	write_cmos_sensor(0x2ba6, 0x0088);
	write_cmos_sensor(0x2ba8, 0x7316);
	write_cmos_sensor(0x2baa, 0x9382);
	write_cmos_sensor(0x2bac, 0x8092);
	write_cmos_sensor(0x2bae, 0x2403);
	write_cmos_sensor(0x2bb0, 0x4292);
	write_cmos_sensor(0x2bb2, 0x008A);
	write_cmos_sensor(0x2bb4, 0x7316);
	write_cmos_sensor(0x2bb6, 0x430E);
	write_cmos_sensor(0x2bb8, 0x421F);
	write_cmos_sensor(0x2bba, 0x0086);
	write_cmos_sensor(0x2bbc, 0x822F);
	write_cmos_sensor(0x2bbe, 0x9F82);
	write_cmos_sensor(0x2bc0, 0x0084);
	write_cmos_sensor(0x2bc2, 0x2801);
	write_cmos_sensor(0x2bc4, 0x431E);
	write_cmos_sensor(0x2bc6, 0x4292);
	write_cmos_sensor(0x2bc8, 0x0086);
	write_cmos_sensor(0x2bca, 0x7314);
	write_cmos_sensor(0x2bcc, 0x93C2);
	write_cmos_sensor(0x2bce, 0x00BC);
	write_cmos_sensor(0x2bd0, 0x2007);
	write_cmos_sensor(0x2bd2, 0xB31E);
	write_cmos_sensor(0x2bd4, 0x2405);
	write_cmos_sensor(0x2bd6, 0x421F);
	write_cmos_sensor(0x2bd8, 0x0084);
	write_cmos_sensor(0x2bda, 0x522F);
	write_cmos_sensor(0x2bdc, 0x4F82);
	write_cmos_sensor(0x2bde, 0x7314);
	write_cmos_sensor(0x2be0, 0x425F);
	write_cmos_sensor(0x2be2, 0x00BC);
	write_cmos_sensor(0x2be4, 0xF37F);
	write_cmos_sensor(0x2be6, 0xFE0F);
	write_cmos_sensor(0x2be8, 0x2406);
	write_cmos_sensor(0x2bea, 0x421E);
	write_cmos_sensor(0x2bec, 0x0086);
	write_cmos_sensor(0x2bee, 0x503E);
	write_cmos_sensor(0x2bf0, 0xFFFB);
	write_cmos_sensor(0x2bf2, 0x4E82);
	write_cmos_sensor(0x2bf4, 0x7524);
	write_cmos_sensor(0x2bf6, 0x430E);
	write_cmos_sensor(0x2bf8, 0x421F);
	write_cmos_sensor(0x2bfa, 0x7524);
	write_cmos_sensor(0x2bfc, 0x9F82);
	write_cmos_sensor(0x2bfe, 0x809A);
	write_cmos_sensor(0x2c00, 0x2C07);
	write_cmos_sensor(0x2c02, 0x9382);
	write_cmos_sensor(0x2c04, 0x8094);
	write_cmos_sensor(0x2c06, 0x2004);
	write_cmos_sensor(0x2c08, 0x9382);
	write_cmos_sensor(0x2c0a, 0x8096);
	write_cmos_sensor(0x2c0c, 0x2001);
	write_cmos_sensor(0x2c0e, 0x431E);
	write_cmos_sensor(0x2c10, 0x40B2);
	write_cmos_sensor(0x2c12, 0x0032);
	write_cmos_sensor(0x2c14, 0x7522);
	write_cmos_sensor(0x2c16, 0x4292);
	write_cmos_sensor(0x2c18, 0x7524);
	write_cmos_sensor(0x2c1a, 0x809A);
	write_cmos_sensor(0x2c1c, 0x930E);
	write_cmos_sensor(0x2c1e, 0x249F);
	write_cmos_sensor(0x2c20, 0x421F);
	write_cmos_sensor(0x2c22, 0x7316);
	write_cmos_sensor(0x2c24, 0xC312);
	write_cmos_sensor(0x2c26, 0x100F);
	write_cmos_sensor(0x2c28, 0x832F);
	write_cmos_sensor(0x2c2a, 0x4F82);
	write_cmos_sensor(0x2c2c, 0x7522);
	write_cmos_sensor(0x2c2e, 0x53B2);
	write_cmos_sensor(0x2c30, 0x7524);
	write_cmos_sensor(0x2c32, 0x3C95);
	write_cmos_sensor(0x2c34, 0x431F);
	write_cmos_sensor(0x2c36, 0x4D0E);
	write_cmos_sensor(0x2c38, 0x533E);
	write_cmos_sensor(0x2c3a, 0x930E);
	write_cmos_sensor(0x2c3c, 0x2403);
	write_cmos_sensor(0x2c3e, 0x5F0F);
	write_cmos_sensor(0x2c40, 0x831E);
	write_cmos_sensor(0x2c42, 0x23FD);
	write_cmos_sensor(0x2c44, 0x4FC2);
	write_cmos_sensor(0x2c46, 0x0A98);
	write_cmos_sensor(0x2c48, 0x3FAA);
	write_cmos_sensor(0x2c4a, 0x4292);
	write_cmos_sensor(0x2c4c, 0x0A86);
	write_cmos_sensor(0x2c4e, 0x0782);
	write_cmos_sensor(0x2c50, 0x421F);
	write_cmos_sensor(0x2c52, 0x0A88);
	write_cmos_sensor(0x2c54, 0x490E);
	write_cmos_sensor(0x2c56, 0x930E);
	write_cmos_sensor(0x2c58, 0x2404);
	write_cmos_sensor(0x2c5a, 0xC312);
	write_cmos_sensor(0x2c5c, 0x100F);
	write_cmos_sensor(0x2c5e, 0x831E);
	write_cmos_sensor(0x2c60, 0x23FC);
	write_cmos_sensor(0x2c62, 0x4F82);
	write_cmos_sensor(0x2c64, 0x0784);
	write_cmos_sensor(0x2c66, 0x3F6B);
	write_cmos_sensor(0x2c68, 0x90F2);
	write_cmos_sensor(0x2c6a, 0x0011);
	write_cmos_sensor(0x2c6c, 0x00BA);
	write_cmos_sensor(0x2c6e, 0x280A);
	write_cmos_sensor(0x2c70, 0x90F2);
	write_cmos_sensor(0x2c72, 0x0051);
	write_cmos_sensor(0x2c74, 0x00BA);
	write_cmos_sensor(0x2c76, 0x2C06);
	write_cmos_sensor(0x2c78, 0x403C);
	write_cmos_sensor(0x2c7a, 0x0A13);
	write_cmos_sensor(0x2c7c, 0x425E);
	write_cmos_sensor(0x2c7e, 0x00BA);
	write_cmos_sensor(0x2c80, 0x535E);
	write_cmos_sensor(0x2c82, 0x3EE4);
	write_cmos_sensor(0x2c84, 0x90F2);
	write_cmos_sensor(0x2c86, 0x0051);
	write_cmos_sensor(0x2c88, 0x00BA);
	write_cmos_sensor(0x2c8a, 0x2804);
	write_cmos_sensor(0x2c8c, 0x90F2);
	write_cmos_sensor(0x2c8e, 0xFF81);
	write_cmos_sensor(0x2c90, 0x00BA);
	write_cmos_sensor(0x2c92, 0x2808);
	write_cmos_sensor(0x2c94, 0x90F2);
	write_cmos_sensor(0x2c96, 0xFF81);
	write_cmos_sensor(0x2c98, 0x00BA);
	write_cmos_sensor(0x2c9a, 0x2807);
	write_cmos_sensor(0x2c9c, 0x90F2);
	write_cmos_sensor(0x2c9e, 0xFF91);
	write_cmos_sensor(0x2ca0, 0x00BA);
	write_cmos_sensor(0x2ca2, 0x2C03);
	write_cmos_sensor(0x2ca4, 0x403C);
	write_cmos_sensor(0x2ca6, 0x0813);
	write_cmos_sensor(0x2ca8, 0x3FE9);
	write_cmos_sensor(0x2caa, 0x90F2);
	write_cmos_sensor(0x2cac, 0xFF91);
	write_cmos_sensor(0x2cae, 0x00BA);
	write_cmos_sensor(0x2cb0, 0x280A);
	write_cmos_sensor(0x2cb2, 0x90F2);
	write_cmos_sensor(0x2cb4, 0xFFB1);
	write_cmos_sensor(0x2cb6, 0x00BA);
	write_cmos_sensor(0x2cb8, 0x2C06);
	write_cmos_sensor(0x2cba, 0x403D);
	write_cmos_sensor(0x2cbc, 0x0060);
	write_cmos_sensor(0x2cbe, 0x425E);
	write_cmos_sensor(0x2cc0, 0x00BA);
	write_cmos_sensor(0x2cc2, 0x536E);
	write_cmos_sensor(0x2cc4, 0x3EC3);
	write_cmos_sensor(0x2cc6, 0x90F2);
	write_cmos_sensor(0x2cc8, 0xFFB1);
	write_cmos_sensor(0x2cca, 0x00BA);
	write_cmos_sensor(0x2ccc, 0x2804);
	write_cmos_sensor(0x2cce, 0x90F2);
	write_cmos_sensor(0x2cd0, 0xFFC1);
	write_cmos_sensor(0x2cd2, 0x00BA);
	write_cmos_sensor(0x2cd4, 0x2808);
	write_cmos_sensor(0x2cd6, 0x90F2);
	write_cmos_sensor(0x2cd8, 0xFFC1);
	write_cmos_sensor(0x2cda, 0x00BA);
	write_cmos_sensor(0x2cdc, 0x280B);
	write_cmos_sensor(0x2cde, 0x90F2);
	write_cmos_sensor(0x2ce0, 0xFFD1);
	write_cmos_sensor(0x2ce2, 0x00BA);
	write_cmos_sensor(0x2ce4, 0x2C07);
	write_cmos_sensor(0x2ce6, 0x403D);
	write_cmos_sensor(0x2ce8, 0x0050);
	write_cmos_sensor(0x2cea, 0x425E);
	write_cmos_sensor(0x2cec, 0x00BA);
	write_cmos_sensor(0x2cee, 0x507E);
	write_cmos_sensor(0x2cf0, 0x0003);
	write_cmos_sensor(0x2cf2, 0x3EAC);
	write_cmos_sensor(0x2cf4, 0x403D);
	write_cmos_sensor(0x2cf6, 0x003C);
	write_cmos_sensor(0x2cf8, 0x403F);
	write_cmos_sensor(0x2cfa, 0x00BA);
	write_cmos_sensor(0x2cfc, 0x4F6E);
	write_cmos_sensor(0x2cfe, 0x526E);
	write_cmos_sensor(0x2d00, 0x90FF);
	write_cmos_sensor(0x2d02, 0xFFFB);
	write_cmos_sensor(0x2d04, 0x0000);
	write_cmos_sensor(0x2d06, 0x2AA2);
	write_cmos_sensor(0x2d08, 0x437E);
	write_cmos_sensor(0x2d0a, 0x3EA0);
	write_cmos_sensor(0x2d0c, 0x425F);
	write_cmos_sensor(0x2d0e, 0x00BA);
	write_cmos_sensor(0x2d10, 0x4F4C);
	write_cmos_sensor(0x2d12, 0x407A);
	write_cmos_sensor(0x2d14, 0x001C);
	write_cmos_sensor(0x2d16, 0x12B0);
	write_cmos_sensor(0x2d18, 0xFDD8);
	write_cmos_sensor(0x2d1a, 0x4E4F);
	write_cmos_sensor(0x2d1c, 0xC312);
	write_cmos_sensor(0x2d1e, 0x104F);
	write_cmos_sensor(0x2d20, 0x114F);
	write_cmos_sensor(0x2d22, 0xF37F);
	write_cmos_sensor(0x2d24, 0x5F0F);
	write_cmos_sensor(0x2d26, 0x5F0F);
	write_cmos_sensor(0x2d28, 0x5F0F);
	write_cmos_sensor(0x2d2a, 0x5F0F);
	write_cmos_sensor(0x2d2c, 0x403E);
	write_cmos_sensor(0x2d2e, 0x00F0);
	write_cmos_sensor(0x2d30, 0x8F0E);
	write_cmos_sensor(0x2d32, 0x3E74);
	write_cmos_sensor(0x2d34, 0x421F);
	write_cmos_sensor(0x2d36, 0x80BA);
	write_cmos_sensor(0x2d38, 0x903F);
	write_cmos_sensor(0x2d3a, 0x0009);
	write_cmos_sensor(0x2d3c, 0x2C04);
	write_cmos_sensor(0x2d3e, 0x531F);
	write_cmos_sensor(0x2d40, 0x4F82);
	write_cmos_sensor(0x2d42, 0x80BA);
	write_cmos_sensor(0x2d44, 0x3E43);
	write_cmos_sensor(0x2d46, 0x421F);
	write_cmos_sensor(0x2d48, 0x80B4);
	write_cmos_sensor(0x2d4a, 0x903F);
	write_cmos_sensor(0x2d4c, 0x0098);
	write_cmos_sensor(0x2d4e, 0x2C04);
	write_cmos_sensor(0x2d50, 0x531F);
	write_cmos_sensor(0x2d52, 0x4F82);
	write_cmos_sensor(0x2d54, 0x80B4);
	write_cmos_sensor(0x2d56, 0x3E3A);
	write_cmos_sensor(0x2d58, 0x4382);
	write_cmos_sensor(0x2d5a, 0x80B4);
	write_cmos_sensor(0x2d5c, 0x3E37);
	write_cmos_sensor(0x2d5e, 0x4E82);
	write_cmos_sensor(0x2d60, 0x8096);
	write_cmos_sensor(0x2d62, 0xD392);
	write_cmos_sensor(0x2d64, 0x7102);
	write_cmos_sensor(0x2d66, 0x4138);
	write_cmos_sensor(0x2d68, 0x4139);
	write_cmos_sensor(0x2d6a, 0x413A);
	write_cmos_sensor(0x2d6c, 0x413B);
	write_cmos_sensor(0x2d6e, 0x4130);
	write_cmos_sensor(0x2d70, 0x0260);
	write_cmos_sensor(0x2d72, 0x0000);
	write_cmos_sensor(0x2d74, 0x0C18);
	write_cmos_sensor(0x2d76, 0x0240);
	write_cmos_sensor(0x2d78, 0x0000);
	write_cmos_sensor(0x2d7a, 0x0260);
	write_cmos_sensor(0x2d7c, 0x0000);
	write_cmos_sensor(0x2d7e, 0x0C05);
	write_cmos_sensor(0x2d80, 0x4130);
	write_cmos_sensor(0x2d82, 0x4382);
	write_cmos_sensor(0x2d84, 0x7602);
	write_cmos_sensor(0x2d86, 0x4F82);
	write_cmos_sensor(0x2d88, 0x7600);
	write_cmos_sensor(0x2d8a, 0x0270);
	write_cmos_sensor(0x2d8c, 0x0000);
	write_cmos_sensor(0x2d8e, 0x0C07);
	write_cmos_sensor(0x2d90, 0x0270);
	write_cmos_sensor(0x2d92, 0x0001);
	write_cmos_sensor(0x2d94, 0x421F);
	write_cmos_sensor(0x2d96, 0x7606);
	write_cmos_sensor(0x2d98, 0x4FC2);
	write_cmos_sensor(0x2d9a, 0x0188);
	write_cmos_sensor(0x2d9c, 0x4130);
	write_cmos_sensor(0x2d9e, 0xDF02);
	write_cmos_sensor(0x2da0, 0x3FFE);
	write_cmos_sensor(0x2da2, 0x430E);
	write_cmos_sensor(0x2da4, 0x930A);
	write_cmos_sensor(0x2da6, 0x2407);
	write_cmos_sensor(0x2da8, 0xC312);
	write_cmos_sensor(0x2daa, 0x100C);
	write_cmos_sensor(0x2dac, 0x2801);
	write_cmos_sensor(0x2dae, 0x5A0E);
	write_cmos_sensor(0x2db0, 0x5A0A);
	write_cmos_sensor(0x2db2, 0x930C);
	write_cmos_sensor(0x2db4, 0x23F7);
	write_cmos_sensor(0x2db6, 0x4130);
	write_cmos_sensor(0x2db8, 0x430E);
	write_cmos_sensor(0x2dba, 0x430F);
	write_cmos_sensor(0x2dbc, 0x3C08);
	write_cmos_sensor(0x2dbe, 0xC312);
	write_cmos_sensor(0x2dc0, 0x100D);
	write_cmos_sensor(0x2dc2, 0x100C);
	write_cmos_sensor(0x2dc4, 0x2802);
	write_cmos_sensor(0x2dc6, 0x5A0E);
	write_cmos_sensor(0x2dc8, 0x6B0F);
	write_cmos_sensor(0x2dca, 0x5A0A);
	write_cmos_sensor(0x2dcc, 0x6B0B);
	write_cmos_sensor(0x2dce, 0x930C);
	write_cmos_sensor(0x2dd0, 0x23F6);
	write_cmos_sensor(0x2dd2, 0x930D);
	write_cmos_sensor(0x2dd4, 0x23F4);
	write_cmos_sensor(0x2dd6, 0x4130);
	write_cmos_sensor(0x2dd8, 0xEE4E);
	write_cmos_sensor(0x2dda, 0x407B);
	write_cmos_sensor(0x2ddc, 0x0009);
	write_cmos_sensor(0x2dde, 0x3C05);
	write_cmos_sensor(0x2de0, 0x100D);
	write_cmos_sensor(0x2de2, 0x6E4E);
	write_cmos_sensor(0x2de4, 0x9A4E);
	write_cmos_sensor(0x2de6, 0x2801);
	write_cmos_sensor(0x2de8, 0x8A4E);
	write_cmos_sensor(0x2dea, 0x6C4C);
	write_cmos_sensor(0x2dec, 0x6D0D);
	write_cmos_sensor(0x2dee, 0x835B);
	write_cmos_sensor(0x2df0, 0x23F7);
	write_cmos_sensor(0x2df2, 0x4130);
	write_cmos_sensor(0x2df4, 0xEF0F);
	write_cmos_sensor(0x2df6, 0xEE0E);
	write_cmos_sensor(0x2df8, 0x4039);
	write_cmos_sensor(0x2dfa, 0x0021);
	write_cmos_sensor(0x2dfc, 0x3C0A);
	write_cmos_sensor(0x2dfe, 0x1008);
	write_cmos_sensor(0x2e00, 0x6E0E);
	write_cmos_sensor(0x2e02, 0x6F0F);
	write_cmos_sensor(0x2e04, 0x9B0F);
	write_cmos_sensor(0x2e06, 0x2805);
	write_cmos_sensor(0x2e08, 0x2002);
	write_cmos_sensor(0x2e0a, 0x9A0E);
	write_cmos_sensor(0x2e0c, 0x2802);
	write_cmos_sensor(0x2e0e, 0x8A0E);
	write_cmos_sensor(0x2e10, 0x7B0F);
	write_cmos_sensor(0x2e12, 0x6C0C);
	write_cmos_sensor(0x2e14, 0x6D0D);
	write_cmos_sensor(0x2e16, 0x6808);
	write_cmos_sensor(0x2e18, 0x8319);
	write_cmos_sensor(0x2e1a, 0x23F1);
	write_cmos_sensor(0x2e1c, 0x4130);
	write_cmos_sensor(0x2e1e, 0x0000);
	write_cmos_sensor(0x2ffe, 0xf000);
	write_cmos_sensor(0x3000, 0x00AE);
	write_cmos_sensor(0x3002, 0x00AE);
	write_cmos_sensor(0x3004, 0x00AE);
	write_cmos_sensor(0x3006, 0x00AE);
	write_cmos_sensor(0x3008, 0x00AE);
	write_cmos_sensor(0x4000, 0x0400);
	write_cmos_sensor(0x4002, 0x0400);
	write_cmos_sensor(0x4004, 0x0C04);
	write_cmos_sensor(0x4006, 0x0C04);
	write_cmos_sensor(0x4008, 0x0C04);

	//--- FW End ---//


	//--- Initial Set file ---//
	write_cmos_sensor(0x0B02, 0x0014);
	write_cmos_sensor(0x0B04, 0x07CB);
	write_cmos_sensor(0x0B06, 0x5ED7);
	write_cmos_sensor(0x0B14, 0x370B); //PLL Main Div[15:8]. 0x1b = Pclk(86.4mhz), 0x37 = Pclk(176mhz)
	write_cmos_sensor(0x0B16, 0x4A0B);
	write_cmos_sensor(0x0B18, 0x0000);
	write_cmos_sensor(0x0B1A, 0x1044);

	write_cmos_sensor(0x004C, 0x0100); //tg_enable.
	write_cmos_sensor(0x0032, 0x0101); //Normal
	write_cmos_sensor(0x0036, 0x0048); //ramp_rst_offset
	write_cmos_sensor(0x0038, 0x4800); //ramp_sig_poffset
	write_cmos_sensor(0x0138, 0x0004); //pxl_drv_pwr
	write_cmos_sensor(0x013A, 0x0100); //tx_idle
	write_cmos_sensor(0x0C00, 0x3BC1); //BLC_ctl1. Line BLC on = 0x3b, off = 0x2A //LBLC_ctl1. [0]en_blc, [1]en_lblc_dpc, [2]en_channel_blc, [3]en_adj_pxl_dpc, [4]en_adp_dead_pxl_th
	write_cmos_sensor(0x0C0E, 0x0500); //0x07 BLC display On, 0x05 BLC D off //BLC_ctl3. Frame BLC On = 0x05, off=0x04 //FBLC_ctl3. [0]en_fblc, [1]en_blc_bypass, [2]en_fblc_dpc, [5]en_fobp_dig_offset, [7]en_lobp_dpc_bypass 
	write_cmos_sensor(0x0C10, 0x0510); //dig_blc_offset_h
	write_cmos_sensor(0x0C16, 0x0000); //fobp_dig_b_offset. red b[7] sign(0:+,1:-), b[6:0] dc offset 128(max)
	write_cmos_sensor(0x0C18, 0x0000); //fobp_dig_Gb_offset. Gr b[7] sign(0:+,1:-), b[6:0] dc offset 128(max)
	write_cmos_sensor(0x0C36, 0x0100); //r_g_sum_ctl. [0]:g_sum_en. '1'enable, '0'disable.

	//--- MIPI blank time --------------//
	write_cmos_sensor(0x0902, 0x4101); //mipi_value_clk_trail. MIPI CLK mode [1]'1'cont(0x43),'0'non-cont(0x41) [6]'1'2lane(0x41), '0'1lane(0x01) 
	write_cmos_sensor(0x090A, 0x03E4); //mipi_vblank_delay_h.
	write_cmos_sensor(0x090C, 0x0020); //mipi_hblank_short_delay_h.
	write_cmos_sensor(0x090E, 0x0020); //mipi_hblank_long_delay_h.
	write_cmos_sensor(0x0910, 0x5D07); //05 mipi_LPX
	write_cmos_sensor(0x0912, 0x061e); //05 mipi_CLK_prepare
	write_cmos_sensor(0x0914, 0x0407); //02 mipi_clk_pre
	write_cmos_sensor(0x0916, 0x0b0a); //09 mipi_data_zero
	write_cmos_sensor(0x0918, 0x0e09); //0c mipi_clk_post
	//----------------------------------//

	//--- Pixel Array Addressing ------//
	write_cmos_sensor(0x000E, 0x0000); //x_addr_start_lobp_h.
	write_cmos_sensor(0x0014, 0x003F); //x_addr_end_lobp_h.
	write_cmos_sensor(0x0010, 0x0050); //x_addr_start_robp_h.
	write_cmos_sensor(0x0016, 0x008F); //x_addr_end_robp_h.
	write_cmos_sensor(0x0012, 0x00A4); //x_addr_start_hact_h. 170
	write_cmos_sensor(0x0018, 0x0AD1); //x_addr_end_hact_h. 2769. 2769-170+1=2606
	write_cmos_sensor(0x0020, 0x0700); //x_regin_sel
	write_cmos_sensor(0x0022, 0x0004); //y_addr_start_fobp_h.
	write_cmos_sensor(0x0028, 0x000B); //y_addr_end_fobp_h.  
	write_cmos_sensor(0x0024, 0xFFFA); //y_addr_start_dummy_h.
	write_cmos_sensor(0x002A, 0xFFFF); //y_addr_end_dummy_h.  
	write_cmos_sensor(0x0026, 0x0012); //y_addr_start_vact_h. 18
	write_cmos_sensor(0x002C, 0x07B5); //y_addr_end_vact_h.  1973. 1973-18+1=1956
	write_cmos_sensor(0x0034, 0x0700); //Y_region_sel
	//------------------------------//

	//--Crop size 2604x1956 ----///
	write_cmos_sensor(0x0128, 0x0002); // digital_crop_x_offset_l
	write_cmos_sensor(0x012A, 0x0000); // digital_crop_y_offset_l
	write_cmos_sensor(0x012C, 0x0A2C); // digital_crop_image_width
	write_cmos_sensor(0x012E, 0x07A4); // digital_crop_image_height
	//------------------------------//

	//----< Image FMT Size >--------------------//
	//Image size 2604x1956
	write_cmos_sensor(0x0110, 0x0A2C); //X_output_size_h     
	write_cmos_sensor(0x0112, 0x07A4); //Y_output_size_h     
	//------------------------------------------//

	//----< Frame / Line Length >--------------//
	write_cmos_sensor(0x0006, 0x07c0); //frame_length_h 1984
	write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	write_cmos_sensor(0x000A, 0x0DB0); //line_length for binning 3504
	//---------------------------------------//

	//--- ETC set ----//
	write_cmos_sensor(0x003C, 0x0000); //fixed frame off. b[0] '1'enable, '0'disable
	write_cmos_sensor(0x0000, 0x0300); //orientation. [0]:x-flip, [1]:y-flip.
	write_cmos_sensor(0x0500, 0x0000); //DGA_ctl.  b[1]'0'OTP_color_ratio_disable, '1' OTP_color_ratio_enable, b[2]'0'data_pedestal_en, '1'data_pedestal_dis.
	write_cmos_sensor(0x0700, 0x0590); //Scaler Normal 
	write_cmos_sensor(0x001E, 0x0101); 
	write_cmos_sensor(0x0032, 0x0101); 
	write_cmos_sensor(0x0A02, 0x0100); // Fast sleep Enable
	write_cmos_sensor(0x0116, 0x003B); // FBLC Ratio
	//----------------//

	//--- AG / DG control ----------------//
	//AG
	write_cmos_sensor(0x003A, 0x0000); //Analog Gain.  0x00=x1, 0x70=x8, 0xf0=x16.

	//DG
	write_cmos_sensor(0x0508, 0x0100); //DG_Gr_h.  0x01=x1, 0x07=x8.
	write_cmos_sensor(0x050a, 0x0100); //DG_Gb_h.  0x01=x1, 0x07=x8.
	write_cmos_sensor(0x050c, 0x0100); //DG_R_h.  0x01=x1, 0x07=x8.
	write_cmos_sensor(0x050e, 0x0100); //DG_B_h.  0x01=x1, 0x07=x8.
	//----------------------------------//


	//-----< Exp.Time >------------------------//
	// Pclk_88Mhz @ Line_length_pclk : 2880 @Exp.Time 33.33ms
	write_cmos_sensor(0x0002, 0x04b0);	//Fine_int : 33.33ms@Pclk88mhz@Line_length2880 
	write_cmos_sensor(0x0004, 0x04b0); //coarse_int : 33.33ms@Pclk88mhz@Line_length2880

	//--- ISP enable Selection ---------------//
	write_cmos_sensor(0x0A04, 0x011A); //isp_en. [9]s-gamma,[8]MIPI_en,[6]compresion10to8,[5]Scaler,[4]window,[3]DG,[2]LSC,[1]adpc,[0]tpg
	//----------------------------------------//

	write_cmos_sensor(0x0118, 0x0100); //sleep Off
}    /*    sensor_init  */


static void preview_setting(void)
{ 
#if 0       // original preview setting
	//////////////////////////////////////////////////////////////////////////
	//			Sensor        	 : Hi-545
	//      Set Name         : Preview
	//      Mode             : D-Binning ( = Scaler 1/2)
	//      Size             : 1296 * 972
	//	set file	 : v0.45
	//	Date		 : 20140724
	//////////////////////////////////////////////////////////////////////////


	write_cmos_sensor(0x0118, 0x0000); //sleep On

	//--- SREG ---//
	write_cmos_sensor(0x0B02, 0x0014); //140718
	write_cmos_sensor(0x0B16, 0x4A8B); //140718 sys_clk 1/2
	write_cmos_sensor(0x0B18, 0x0000); //140718
	write_cmos_sensor(0x0B1A, 0x1044); //140718
	write_cmos_sensor(0x004C, 0x0100); 
	write_cmos_sensor(0x000C, 0x0000); 

	//---< mipi time >---//
	write_cmos_sensor(0x0902, 0x4101); //mipi_value_clk_trail.  
	write_cmos_sensor(0x090A, 0x01E4); //mipi_vblank_delay_h.
	write_cmos_sensor(0x090C, 0x0005); //mipi_hblank_short_delay_h.
	write_cmos_sensor(0x090E, 0x0100); //mipi_hblank_long_delay_h.
	write_cmos_sensor(0x0910, 0x5D04); //05 mipi_LPX
	write_cmos_sensor(0x0912, 0x030f); //05 mipi_CLK_prepare
	write_cmos_sensor(0x0914, 0x0204); //02 mipi_clk_pre
	write_cmos_sensor(0x0916, 0x0707); //09 mipi_data_zero
	write_cmos_sensor(0x0918, 0x0f04); //0c mipi_clk_post

	//---< Pixel Array Address >------//
	write_cmos_sensor(0x0012, 0x00AA); //x_addr_start_hact_h. 170
	write_cmos_sensor(0x0018, 0x0ACB); //x_addr_end_hact_h. 2763 . 2763-170+1=2594
	write_cmos_sensor(0x0026, 0x0018); //y_addr_start_vact_h. 24
	write_cmos_sensor(0x002C, 0x07AF); //y_addr_end_vact_h.   1967. 1967-24+1=1944

	//---< Crop size : 2592x1944 >----//
	write_cmos_sensor(0x0128, 0x0002); // digital_crop_x_offset_l  
	write_cmos_sensor(0x012A, 0x0000); // digital_crop_y_offset_l  
	write_cmos_sensor(0x012C, 0x0A20); // digital_crop_image_width 
	write_cmos_sensor(0x012E, 0x0798); // digital_crop_image_height

	//---< FMT Size : 1296x972 >---------//
	write_cmos_sensor(0x0110, 0x0510); //X_output_size_h 
	write_cmos_sensor(0x0112, 0x03CC); //Y_output_size_h 

	//---< Frame/Line Length >-----//
//	write_cmos_sensor(0x0006, 0x07e1); //frame_length_h 2017
//	write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	write_cmos_sensor(0x000A, 0x0DB0); 

	//---< ETC set >---------------//
//	write_cmos_sensor(0x0000, 0x0100); //orientation. [0]:x-flip, [1]:y-flip.
	write_cmos_sensor(0x0700, 0xA0A8); //140718 scaler 1/2
	write_cmos_sensor(0x001E, 0x0101); //140718
	write_cmos_sensor(0x0032, 0x0101); //140718
	write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
	write_cmos_sensor(0x0A04, 0x0133); //TEST PATTERN

	write_cmos_sensor(0x0118, 0x0100); //sleep Off 
	//mDELAY(30);       // for faster
#else
    capture_setting(300);
#endif
}    /*    preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
    LOG_INF("E! currefps:%d\n",currefps);
    if (currefps == 240) { //24fps for PIP
		//////////////////////////////////////////////////////////////////////////
		//			Sensor        	 : Hi-545
		//      Set Name         : Capture
		//      Mode             : Normal
		//      Size             : 2592 * 1944
		//	set file	 : v0.45
		//	Date		 : 20140724
		//////////////////////////////////////////////////////////////////////////


		write_cmos_sensor(0x0118, 0x0000); //sleep On

		//--- SREG ---//
		write_cmos_sensor(0x0B02, 0x0014); //140718
		write_cmos_sensor(0x0B16, 0x4A0B); 
		write_cmos_sensor(0x0B18, 0x0000); //140718
		write_cmos_sensor(0x0B1A, 0x1044); //140718
		write_cmos_sensor(0x004C, 0x0100); 
		write_cmos_sensor(0x000C, 0x0000); 

		//---< mipi time >---//
		write_cmos_sensor(0x0902, 0x4101); 
		write_cmos_sensor(0x090A, 0x03E4); 
		write_cmos_sensor(0x090C, 0x0020); 
		write_cmos_sensor(0x090E, 0x0020); 
		write_cmos_sensor(0x0910, 0x5D07); 
		write_cmos_sensor(0x0912, 0x061e); 
		write_cmos_sensor(0x0914, 0x0407); 
		write_cmos_sensor(0x0916, 0x0b0a); 
		write_cmos_sensor(0x0918, 0x0e09); 

		//---< Pixel Array Address >------//
		write_cmos_sensor(0x0012, 0x00AA); //x_addr_start_hact_h. 170
		write_cmos_sensor(0x0018, 0x0ACB); //x_addr_end_hact_h. 2763 . 2763-170+1=2594
		write_cmos_sensor(0x0026, 0x0018); //y_addr_start_vact_h. 24
		write_cmos_sensor(0x002C, 0x07AF); //y_addr_end_vact_h.   1967. 1967-24+1=1944

		//---< Crop size : 2592x1944 >----//
		write_cmos_sensor(0x0128, 0x0002); //digital_crop_x_offset_l  
		write_cmos_sensor(0x012A, 0x0000); //digital_crop_y_offset_l  
		write_cmos_sensor(0x012C, 0x0A20); //2592 digital_crop_image_width 
		write_cmos_sensor(0x012E, 0x0798); //1944 digital_crop_image_height 

		//---< FMT Size : 2592x1944 >---------//
		write_cmos_sensor(0x0110, 0x0A20); //X_output_size_h 
		write_cmos_sensor(0x0112, 0x0798); //Y_output_size_h 

		//---< Frame/Line Length >-----//
		write_cmos_sensor(0x0006, 0x07e1); //frame_length_h 2017
		write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
		write_cmos_sensor(0x000A, 0x0DB0); 

		//---< ETC set >---------------//
//		write_cmos_sensor(0x0000, 0x0100); //orientation. [0]:x-flip, [1]:y-flip.
		write_cmos_sensor(0x0700, 0x0590); //140718
		write_cmos_sensor(0x001E, 0x0101); //140718
		write_cmos_sensor(0x0032, 0x0101); //140718
		write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
		write_cmos_sensor(0x0A04, 0x011B); //TEST PATTERN enable

		write_cmos_sensor(0x0118, 0x0100); //sleep Off 		
		//mDELAY(30);       // for faster

    } else {
		//////////////////////////////////////////////////////////////////////////
		//			Sensor        	 : Hi-545
		//      Set Name         : Capture
		//      Mode             : Normal
		//      Size             : 2592 * 1944
		//	set file	 : v0.45
		//	Date		 : 20140724
		//////////////////////////////////////////////////////////////////////////


		write_cmos_sensor(0x0118, 0x0000); //sleep On

		//--- SREG ---//
		write_cmos_sensor(0x0B02, 0x0014); //140718
		write_cmos_sensor(0x0B16, 0x4A0B); 
		write_cmos_sensor(0x0B18, 0x0000); //140718
		write_cmos_sensor(0x0B1A, 0x1044); //140718
		write_cmos_sensor(0x004C, 0x0100); 
		write_cmos_sensor(0x000C, 0x0000); 

		//---< mipi time >---//
		write_cmos_sensor(0x0902, 0x4101); 
		write_cmos_sensor(0x090A, 0x03E4); 
		write_cmos_sensor(0x090C, 0x0020); 
		write_cmos_sensor(0x090E, 0x0020); 
		write_cmos_sensor(0x0910, 0x5D07); 
		write_cmos_sensor(0x0912, 0x061e); 
		write_cmos_sensor(0x0914, 0x0407); 
		write_cmos_sensor(0x0916, 0x0b0a); 
		write_cmos_sensor(0x0918, 0x0e09); 

		//---< Pixel Array Address >------//
		write_cmos_sensor(0x0012, 0x00AA); //x_addr_start_hact_h. 170
		write_cmos_sensor(0x0018, 0x0ACB); //x_addr_end_hact_h. 2763 . 2763-170+1=2594
		write_cmos_sensor(0x0026, 0x0018); //y_addr_start_vact_h. 24
		write_cmos_sensor(0x002C, 0x07AF); //y_addr_end_vact_h.   1967. 1967-24+1=1944

		//---< Crop size : 2592x1944 >----//
		write_cmos_sensor(0x0128, 0x0002); //digital_crop_x_offset_l  
		write_cmos_sensor(0x012A, 0x0000); //digital_crop_y_offset_l  
		write_cmos_sensor(0x012C, 0x0A20); //2592 digital_crop_image_width 
		write_cmos_sensor(0x012E, 0x0798); //1944 digital_crop_image_height 

		//---< FMT Size : 2592x1944 >---------//
		write_cmos_sensor(0x0110, 0x0A20); //X_output_size_h 
		write_cmos_sensor(0x0112, 0x0798); //Y_output_size_h 

		//---< Frame/Line Length >-----//
		write_cmos_sensor(0x0006, 0x07e1); //frame_length_h 2017
		write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
		write_cmos_sensor(0x000A, 0x0DB0); 

		//---< ETC set >---------------//
//		write_cmos_sensor(0x0000, 0x0100); //orientation. [0]:x-flip, [1]:y-flip.
		write_cmos_sensor(0x0700, 0x0590); //140718
		write_cmos_sensor(0x001E, 0x0101); //140718
		write_cmos_sensor(0x0032, 0x0101); //140718
		write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
		write_cmos_sensor(0x0A04, 0x011B); //TEST PATTERN enable

		write_cmos_sensor(0x0118, 0x0100); //sleep Off 		
		//mDELAY(30);       // for faster
	}

}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	
	//////////////////////////////////////////////////////////////////////////
	//			Sensor			 : Hi-545
	//		Set Name		 : Preview
	//		Mode			 : D-Binning ( = Scaler 1/2)
	//		Size			 : 1296 * 972
	//	set file	 : v0.45
	//	Date		 : 20140724
	//////////////////////////////////////////////////////////////////////////


	write_cmos_sensor(0x0118, 0x0000); //sleep On

	//--- SREG ---//
	write_cmos_sensor(0x0B02, 0x0014); //140718
	write_cmos_sensor(0x0B16, 0x4A8B); //140718 sys_clk 1/2
	write_cmos_sensor(0x0B18, 0x0000); //140718
	write_cmos_sensor(0x0B1A, 0x1044); //140718
	write_cmos_sensor(0x004C, 0x0100); 
	write_cmos_sensor(0x000C, 0x0000); 

	//---< mipi time >---//
	write_cmos_sensor(0x0902, 0x4101); //mipi_value_clk_trail.	
	write_cmos_sensor(0x090A, 0x01E4); //mipi_vblank_delay_h.
	write_cmos_sensor(0x090C, 0x0005); //mipi_hblank_short_delay_h.
	write_cmos_sensor(0x090E, 0x0100); //mipi_hblank_long_delay_h.
	write_cmos_sensor(0x0910, 0x5D04); //05 mipi_LPX
	write_cmos_sensor(0x0912, 0x030f); //05 mipi_CLK_prepare
	write_cmos_sensor(0x0914, 0x0204); //02 mipi_clk_pre
	write_cmos_sensor(0x0916, 0x0707); //09 mipi_data_zero
	write_cmos_sensor(0x0918, 0x0f04); //0c mipi_clk_post

	//---< Pixel Array Address >------//
	write_cmos_sensor(0x0012, 0x00AA); //x_addr_start_hact_h. 170
	write_cmos_sensor(0x0018, 0x0ACB); //x_addr_end_hact_h. 2763 . 2763-170+1=2594
	write_cmos_sensor(0x0026, 0x0018); //y_addr_start_vact_h. 24
	write_cmos_sensor(0x002C, 0x07AF); //y_addr_end_vact_h.   1967. 1967-24+1=1944

	//---< Crop size : 2592x1944 >----//
	write_cmos_sensor(0x0128, 0x0002); // digital_crop_x_offset_l  
	write_cmos_sensor(0x012A, 0x0000); // digital_crop_y_offset_l  
	write_cmos_sensor(0x012C, 0x0A20); // digital_crop_image_width 
	write_cmos_sensor(0x012E, 0x0798); // digital_crop_image_height

	//---< FMT Size : 1296x972 >---------//
	write_cmos_sensor(0x0110, 0x0510); //X_output_size_h 
	write_cmos_sensor(0x0112, 0x03CC); //Y_output_size_h 

	//---< Frame/Line Length >-----//
//	write_cmos_sensor(0x0006, 0x07e1); //frame_length_h 2017
//	write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	write_cmos_sensor(0x000A, 0x0DB0); 

	//---< ETC set >---------------//
//	write_cmos_sensor(0x0000, 0x0100); //orientation. [0]:x-flip, [1]:y-flip.
	write_cmos_sensor(0x0700, 0xA0A8); //140718 scaler 1/2
	write_cmos_sensor(0x001E, 0x0101); //140718
	write_cmos_sensor(0x0032, 0x0101); //140718
	write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
	write_cmos_sensor(0x0A04, 0x0133); //TEST PATTERN

	write_cmos_sensor(0x0118, 0x0100); //sleep Off 
	mDELAY(30);
		
}

static void hs_video_setting(void)
{
	LOG_INF("E\n");
	//////////////////////////////////////////////////////////////////////////
	//			Sensor			 : Hi-545
	//		Set Name		 : VGA@120fps
	//		Mode			 : H_scale1/2*H_s2 + V_sclae1/2*V_s2
	//		Size			 : 648 * 488
	//			set file				 : v0.36
	//			Date						 : 20140718
	//////////////////////////////////////////////////////////////////////////
	
	write_cmos_sensor(0x0118, 0x0000); //sleep On
	
	//--- SREG ---//
	write_cmos_sensor(0x0B02, 0x0014); //140718
	write_cmos_sensor(0x0B16, 0x4A0B); 
	write_cmos_sensor(0x0B18, 0x0000); //140718
	write_cmos_sensor(0x0B1A, 0x1044); //140718
	write_cmos_sensor(0x004C, 0x0100); 
	write_cmos_sensor(0x000C, 0x0000);
	
	//---< mipi time >---//
	write_cmos_sensor(0x0902, 0x4101); 
	write_cmos_sensor(0x090A, 0x03E4); 
	write_cmos_sensor(0x090C, 0x0020); 
	write_cmos_sensor(0x090E, 0x0020); 
	write_cmos_sensor(0x0910, 0x5D07); 
	write_cmos_sensor(0x0912, 0x061e); 
	write_cmos_sensor(0x0914, 0x0407); 
	write_cmos_sensor(0x0916, 0x0b0a); 
	write_cmos_sensor(0x0918, 0x0e09); 
	
	//---< Pixel Array Address >------//
	write_cmos_sensor(0x0012, 0x00AA); //x_addr_start_hact_h.	// 164 + 6 = 170
	write_cmos_sensor(0x0018, 0x0ACD); //x_addr_end_hact_h. 	// 2771 - 6 = 2765 
	write_cmos_sensor(0x0026, 0x0012); //y_addr_start_vact_h.	// 16 + 2 = 18 (0x12)
	write_cmos_sensor(0x002C, 0x07B1); //y_addr_end_vact_h. 	// 1971 - 2 = 1969 (0x7B1)
	
	//---< Crop size : 648x488 >----//
	write_cmos_sensor(0x0128, 0x0002); // digital_crop_x_offset_l
	write_cmos_sensor(0x012A, 0x0000); // digital_crop_y_offset_l
	write_cmos_sensor(0x012C, 0x0510); // digital_crop_image_width	// 1296 (0x510)
	write_cmos_sensor(0x012E, 0x01E8); // digital_crop_image_height // VGA = 488 (0x1E8)
	
	//---< FMT Size : 648x488 >---------//
	write_cmos_sensor(0x0110, 0x0288); //X_output_size_h	// 648
	write_cmos_sensor(0x0112, 0x01E8); //Y_output_size_h	// 488
	
	//---< Frame/Line Length >-----//
	write_cmos_sensor(0x0006, 0x01FA); //frame_length_h 1984 - 1956 = 28 --> 488 + 18 = 506(0x1FA) for frame rate
	write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	write_cmos_sensor(0x000A, 0x0DB0); //line_length for binning 3504
	
	//---< ETC set >---------------//
//	write_cmos_sensor(0x0000, 0x0000); 
	write_cmos_sensor(0x0700, 0x215A); // Scaler b[7:6] r_byrscl_y_ratio, b[5:4] r_byrscl_x_ratio [ '01' 3/4 scale down, '10' 1/2 scale down]
	write_cmos_sensor(0x001E, 0x0301); //140718
	write_cmos_sensor(0x0032, 0x0701); //140718
	write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
	write_cmos_sensor(0x0A04, 0x013B); //TEST PATTERN enable
	
	write_cmos_sensor(0x0118, 0x0100); //sleep Off = Streaming On

	mDELAY(30);
}


static void slim_video_setting(void)
{
	LOG_INF("E\n");
	//////////////////////////////////////////////////////////////////////////
	//			Sensor			 : Hi-545
	//		Set Name		 : Preview
	//		Mode			 : D-Binning ( = Scaler 1/2)
	//		Size			 : 1296 * 972
	//	set file	 : v0.45
	//	Date		 : 20140724
	//////////////////////////////////////////////////////////////////////////


	write_cmos_sensor(0x0118, 0x0000); //sleep On

	//--- SREG ---//
	write_cmos_sensor(0x0B02, 0x0014); //140718
	write_cmos_sensor(0x0B16, 0x4A8B); //140718 sys_clk 1/2
	write_cmos_sensor(0x0B18, 0x0000); //140718
	write_cmos_sensor(0x0B1A, 0x1044); //140718
	write_cmos_sensor(0x004C, 0x0100); 
	write_cmos_sensor(0x000C, 0x0000); 

	//---< mipi time >---//
	write_cmos_sensor(0x0902, 0x4101); //mipi_value_clk_trail.	
	write_cmos_sensor(0x090A, 0x01E4); //mipi_vblank_delay_h.
	write_cmos_sensor(0x090C, 0x0005); //mipi_hblank_short_delay_h.
	write_cmos_sensor(0x090E, 0x0100); //mipi_hblank_long_delay_h.
	write_cmos_sensor(0x0910, 0x5D04); //05 mipi_LPX
	write_cmos_sensor(0x0912, 0x030f); //05 mipi_CLK_prepare
	write_cmos_sensor(0x0914, 0x0204); //02 mipi_clk_pre
	write_cmos_sensor(0x0916, 0x0707); //09 mipi_data_zero
	write_cmos_sensor(0x0918, 0x0f04); //0c mipi_clk_post

	//---< Pixel Array Address >------//
	write_cmos_sensor(0x0012, 0x00AA); //x_addr_start_hact_h. 170
	write_cmos_sensor(0x0018, 0x0ACB); //x_addr_end_hact_h. 2763 . 2763-170+1=2594
	write_cmos_sensor(0x0026, 0x0018); //y_addr_start_vact_h. 24
	write_cmos_sensor(0x002C, 0x07AF); //y_addr_end_vact_h.   1967. 1967-24+1=1944

	//---< Crop size : 2592x1944 >----//
	write_cmos_sensor(0x0128, 0x0002); // digital_crop_x_offset_l  
	write_cmos_sensor(0x012A, 0x0000); // digital_crop_y_offset_l  
	write_cmos_sensor(0x012C, 0x0A20); // digital_crop_image_width 
	write_cmos_sensor(0x012E, 0x0798); // digital_crop_image_height

	//---< FMT Size : 1296x972 >---------//
	write_cmos_sensor(0x0110, 0x0510); //X_output_size_h 
	write_cmos_sensor(0x0112, 0x03CC); //Y_output_size_h 

	//---< Frame/Line Length >-----//
//	write_cmos_sensor(0x0006, 0x07e1); //frame_length_h 2017
//	write_cmos_sensor(0x0008, 0x0B40); //line_length_h 2880
	write_cmos_sensor(0x000A, 0x0DB0); 

	//---< ETC set >---------------//
//	write_cmos_sensor(0x0000, 0x0100); //orientation. [0]:x-flip, [1]:y-flip.
	write_cmos_sensor(0x0700, 0xA0A8); //140718 scaler 1/2
	write_cmos_sensor(0x001E, 0x0101); //140718
	write_cmos_sensor(0x0032, 0x0101); //140718
	write_cmos_sensor(0x0A02, 0x0100); //140718 Fast sleep Enable
	write_cmos_sensor(0x0A04, 0x0133); //TEST PATTERN

	write_cmos_sensor(0x0118, 0x0100); //sleep Off 
	mDELAY(30);

}


static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    LOG_INF("enable: %d\n", enable);

    if (enable) {
        write_cmos_sensor(0x020a,0x0200);        

    } else {
        write_cmos_sensor(0x020a,0x0000); 
    }
    spin_lock(&imgsensor_drv_lock);
    imgsensor.test_pattern = enable;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    get_imgsensor_id
*
* DESCRIPTION
*    This function get the sensor ID
*
* PARAMETERS
*    *sensorID : return the sensor ID
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if(*sensor_id == 0x545)
            {
				//*sensor_id = 0x544;
            }
            if (*sensor_id == imgsensor_info.sensor_id) {
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
                return ERROR_NONE;
            }
            LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        retry = 2;
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
*    open
*
* DESCRIPTION
*    This function initialize the registers of CMOS sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
    kal_uint32 sensor_id = 0;
    LOG_1;
    LOG_2; 

#if USE_I2C_400K
    kdSetI2CSpeed(400);
#else
    // do nothing, because default, uses 100K in kd_sensorlist.c
#endif

    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            sensor_id = return_sensor_id();
            if(sensor_id == 0x545)
            {
            	//sensor_id = 0x544;
            }
            if (sensor_id == imgsensor_info.sensor_id) {
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
                break;
            }
            LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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
    imgsensor.ihdr_en = 0;
    imgsensor.test_pattern = KAL_FALSE;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
    spin_unlock(&imgsensor_drv_lock);

    return ERROR_NONE;
}    /*    open  */



/*************************************************************************
* FUNCTION
*    close
*
* DESCRIPTION
*
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
    LOG_INF("E\n");

    /*No Need to implement this function*/

    return ERROR_NONE;
}    /*    close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*    This function start the sensor preview.
*
* PARAMETERS
*    *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
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
	set_mirror_flip(imgsensor.mirror);
    return ERROR_NONE;
}    /*    preview   */

/*************************************************************************
* FUNCTION
*    capture
*
* DESCRIPTION
*    This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*    None
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
    if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
        imgsensor.pclk = imgsensor_info.cap1.pclk;
        imgsensor.line_length = imgsensor_info.cap1.linelength;
        imgsensor.frame_length = imgsensor_info.cap1.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    } else {
        if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
            LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    }
    spin_unlock(&imgsensor_drv_lock);
    capture_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);
    return ERROR_NONE;
}    /* capture() */
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
	set_mirror_flip(imgsensor.mirror);
    return ERROR_NONE;
}    /*    normal_video   */

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
    hs_video_setting();	
	set_mirror_flip(imgsensor.mirror);
    return ERROR_NONE;
}    /*    hs_video   */

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
	set_mirror_flip(imgsensor.mirror);

    return ERROR_NONE;
}    /*    slim_video     */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
    LOG_INF("E %p\n", sensor_resolution);
    sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
    sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

    sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
    sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

    sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
    sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


    sensor_resolution->SensorHighSpeedVideoWidth     = imgsensor_info.hs_video.grabwindow_width;
    sensor_resolution->SensorHighSpeedVideoHeight     = imgsensor_info.hs_video.grabwindow_height;

    sensor_resolution->SensorSlimVideoWidth     = imgsensor_info.slim_video.grabwindow_width;
    sensor_resolution->SensorSlimVideoHeight     = imgsensor_info.slim_video.grabwindow_height;
    return ERROR_NONE;
}    /*    get_resolution    */

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

    sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;          /* The frame of setting shutter default 0 for TG int */
    sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;    /* The frame of setting sensor gain */
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
    sensor_info->SensorHightSampling = 0;    // 0 is default 1x
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
}    /*    get_info  */


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
}    /* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{//
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


static int strcmp1(MUINT8 *s1, MUINT8 *s2)
{
	while(*s1 && *s2)  
	{
		if(*s1 > *s2)
			return 1;
		else if(*s1 < *s2)
			return -1;
		else
		{
			++s1;
			++s2;
			continue;
		}
	}
	if(*s1)
		return 1;
	if(*s2)
		return -1;
	return 0;
}

static void  debug_imgsensor(MSDK_SENSOR_FEATURE_ENUM feature_id,
                             UINT8 *feature_para,UINT32 *feature_para_len)
{
	MSDK_SENSOR_DBG_IMGSENSOR_INFO_STRUCT *debug_info = (MSDK_SENSOR_DBG_IMGSENSOR_INFO_STRUCT *)feature_para;
	if(strcmp1(debug_info->debugStruct, (MUINT8 *)"sensor_output_dataformat") == 0)
	{
		LOG_INF("enter sensor_output_dataformat\n");
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.sensor_output_dataformat;
		else
			imgsensor_info.sensor_output_dataformat = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"sensor_id") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.sensor_id;
		else
			imgsensor_info.sensor_id = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"checksum_value") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.checksum_value;
		else
			imgsensor_info.checksum_value = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"isp_driving_current") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.isp_driving_current;
		else
			imgsensor_info.isp_driving_current = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"sensor_interface_type") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.sensor_interface_type;
		else
			imgsensor_info.sensor_interface_type = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"mclk") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.mclk;
		else
			imgsensor_info.mclk = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"mipi_lane_num") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.mipi_lane_num;
		else
			imgsensor_info.mipi_lane_num = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"i2c_addr_table") == 0)	// just use the first address
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.i2c_addr_table[0];
		else
			imgsensor_info.i2c_addr_table[0] = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"mipi_sensor_type") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.mipi_sensor_type;
		else
			imgsensor_info.mipi_sensor_type = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"mipi_settle_delay_mode") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.mipi_settle_delay_mode;
		else
			imgsensor_info.mipi_settle_delay_mode = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"cap_delay_frame") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.cap_delay_frame;
		else
			imgsensor_info.cap_delay_frame = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"pre_delay_frame") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.pre_delay_frame;
		else
			imgsensor_info.pre_delay_frame = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"video_delay_frame") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.video_delay_frame;
		else
			imgsensor_info.video_delay_frame = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"hs_video_delay_frame") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.hs_video_delay_frame;
		else
			imgsensor_info.hs_video_delay_frame = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"slim_video_delay_frame") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.slim_video_delay_frame;
		else
			imgsensor_info.slim_video_delay_frame = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"sensor_mode_num") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.sensor_mode_num;
		else
			imgsensor_info.sensor_mode_num = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"ihdr_le_firstline") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.ihdr_le_firstline;
		else
			imgsensor_info.ihdr_le_firstline = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"ihdr_support") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.ihdr_support;
		else
			imgsensor_info.ihdr_support = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"ae_ispGain_delay_frame") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.ae_ispGain_delay_frame;
		else
			imgsensor_info.ae_ispGain_delay_frame = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"ae_sensor_gain_delay_frame") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.ae_sensor_gain_delay_frame;
		else
			imgsensor_info.ae_sensor_gain_delay_frame = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"ae_shut_delay_frame") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.ae_shut_delay_frame;
		else
			imgsensor_info.ae_shut_delay_frame = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"max_frame_length") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.max_frame_length;
		else
			imgsensor_info.max_frame_length = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"min_shutter") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.min_shutter;
		else
			imgsensor_info.min_shutter = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"margin") == 0)
	{
		if(debug_info->isGet == 1)
			debug_info->value = imgsensor_info.margin;
		else
			imgsensor_info.margin = debug_info->value;
	}
	else if(strcmp1(debug_info->debugStruct, (MUINT8 *)"pre") == 0)
	{
		if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"pclk") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.pclk;
			else
				imgsensor_info.pre.pclk = debug_info->value;
		}
		else if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"linelength") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.linelength;
			else
				imgsensor_info.pre.linelength = debug_info->value;
		}
		else if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"framelength") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.framelength;
			else
				imgsensor_info.pre.framelength = debug_info->value;
		}
		else if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"startx") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.startx;
			else
				imgsensor_info.pre.startx = debug_info->value;
		}
		else if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"starty") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.starty;
			else
				imgsensor_info.pre.starty = debug_info->value;
		}
		else if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"grabwindow_width") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.grabwindow_width;
			else
				imgsensor_info.pre.grabwindow_width = debug_info->value;
		}
		else if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"grabwindow_height") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.grabwindow_height;
			else
				imgsensor_info.pre.grabwindow_height = debug_info->value;
		}
		else if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"mipi_data_lp2hs_settle_dc") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			else
				imgsensor_info.pre.mipi_data_lp2hs_settle_dc = debug_info->value;
		}
		else if(strcmp1(debug_info->debugSubstruct, (MUINT8 *)"max_framerate") == 0)
		{
			if(debug_info->isGet == 1)
				debug_info->value = imgsensor_info.pre.max_framerate;
			else
				imgsensor_info.pre.max_framerate = debug_info->value;
		}
	}
	else
	{
		LOG_INF("unknown debug\n");
	}
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

    LOG_INF("feature_id = %d\n", feature_id);
    switch (feature_id) {
		case SENSOR_FEATURE_DEBUG_IMGSENSOR:
			debug_imgsensor(feature_id, feature_para, feature_para_len);
			break;
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
            get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
            break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            set_test_pattern_mode((BOOL)*feature_data);
            break;
        case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
            *feature_return_para_32 = imgsensor_info.checksum_value;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_SET_FRAMERATE:
      //      LOG_INF("current fps :%d\n", (UINT32)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case SENSOR_FEATURE_SET_HDR:
      //      LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.ihdr_en = *feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case SENSOR_FEATURE_GET_CROP_INFO:
      //      LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data);
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
      //      LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n", (UINT16)*feature_data, (UINT16)*(feature_data+1), (UINT16)*(feature_data+2));
            ihdr_write_shutter_gain((UINT16)*feature_data, (UINT16)*(feature_data+1), (UINT16)*(feature_data+2));
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

UINT32 HI545_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}    /*    HI545_MIPI_RAW_SensorInit    */
