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
 *	 HI841mipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
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
#include <asm/system.h>
#include <linux/xlog.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "hi841mipi_Sensor.h"

#define PFX "HI841_camera_sensor"
#define LOG_INF(format, args...) pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static imgsensor_info_struct imgsensor_info = { 
	.sensor_id = HI841_SENSOR_ID,
	
	.checksum_value = 0x117a7c84,
	
	.pre = {
		.pclk = 153600000,				//record different mode's pclk
		.linelength = 4008,				//record different mode's linelength
		.framelength = 1256,			//record different mode's framelength
		.startx =0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 1632,		//record different mode's width of grabwindow
		.grabwindow_height = 1224,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 304000000,
		.linelength = 4008,	
		.framelength = 2512,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
    .cap1 = {                            //capture for PIP 24fps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
        .pclk = 304000000,
		.linelength = 4008,	
		.framelength = 2512,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
    },
	.normal_video = {
		.pclk = 304000000,//153600000,
		.linelength = 4008,
		.framelength = 1256,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,//2176,
		.grabwindow_height = 1080,//1224,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	 .hs_video = {
        .pclk = 304000000,
        .linelength = 4008,				//record different mode's linelength
		.framelength = 632,			//record different mode's framelength
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 640 ,		//record different mode's width of grabwindow
		.grabwindow_height = 480 ,
		.mipi_data_lp2hs_settle_dc = 14,//unit , ns
		.max_framerate = 1200,
    },
    .slim_video = {
       .pclk = 153600000,
		.linelength = 4008,
		.framelength = 1256,
		.startx = 0,
		.starty = 0,
        .grabwindow_width = 1280,
        .grabwindow_height = 720,
        .mipi_data_lp2hs_settle_dc = 14,//unit , ns
        .max_framerate = 300,
    },
	.margin = 4,
	.min_shutter = 1,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
    .ihdr_support = 0,      //1, support; 0,not support
    .ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num
	
	.cap_delay_frame = 2, 
	.pre_delay_frame = 2, 
	.video_delay_frame = 2,
    .hs_video_delay_frame = 3,    //enter high speed video  delay frame num
    .slim_video_delay_frame = 3,//enter slim video delay frame num
	
	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x40, 0xff},
};



static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x0100,					//current shutter
	.gain = 0xe0,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
 .test_pattern = KAL_FALSE, 
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
 .ihdr_en = 0, 
	.i2c_write_id = 0x40,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{{ 2624, 1956,      8,    2, 2608, 1952, 1296,  972, 0000, 0000, 1296,  972,      2,    2, 1280,  960}, // Preview
 { 2624, 1956,     16,    6, 2592, 1944, 2592, 1944, 0000, 0000, 2592, 1944,      2,    2, 2560, 1920}, // capture
 { 2624, 1956,     16,    6, 2592, 1944, 2592, 1944, 0000, 0000, 2592, 1944,      3,    3, 2560, 1920}, // video
 { 2624, 1956,      2,  250, 2608, 1460, 1280,  720, 0000, 0000, 1280,  720,      0,    0, 1280,  720}, //hight speed video
 { 2624, 1956,      8,  246, 2608, 1460, 1280,  720, 0000, 0000, 1280,  720,      0,    0, 1280,  720}};// slim video


static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy()
{


	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	write_cmos_sensor(0x0104, 0x1);
	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);	  
	write_cmos_sensor(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor(0x0343, imgsensor.line_length & 0xFF);
  write_cmos_sensor(0x0104, 0x0);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	kal_int16 dummy_line;
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
}	/*	set_max_framerate  */

static void write_shutter(kal_uint16 shutter)
{

	LOG_INF("write_shutter");
	kal_uint16 realtime_fps = 0;
	kal_uint32 frame_length = 0;
	   
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
		realtime_fps = imgsensor.pclk * 10 / (imgsensor.line_length * imgsensor.frame_length);
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);	
		else{
			 write_cmos_sensor(0x0104,0x01);
	
		    write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		    write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
	//	write_cmos_sensor(0x0202, (shutter>> 8) & 0xFF);
//	write_cmos_sensor(0x0203, shutter & 0xFF);
	            write_cmos_sensor(0x0104,0x00);
	}
	} else {
		// Extend frame length
		 write_cmos_sensor(0x0104,0x01);
	
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
//	write_cmos_sensor(0x0202, (shutter>> 8) & 0xFF);
//	write_cmos_sensor(0x0203, shutter & 0xFF);
		write_cmos_sensor(0x0104,0x00);
	}

	// Update Shutter
	 write_cmos_sensor(0x0104,0x01);
	
	write_cmos_sensor(0x0202, (shutter>> 8) & 0xFF);
	write_cmos_sensor(0x0203, shutter & 0xFF);	  
	
	write_cmos_sensor(0x0104,0x00);
	LOG_INF("shutter =%d, framelength =%d", shutter,imgsensor.frame_length);

	//LOG_INF("frame_length = %d ", frame_length);
	
}	/*	write_shutter  */



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

	LOG_INF("set_shutter");
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	
	write_shutter(shutter);
}	/*	set_shutter */

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;
	
	  reg_gain = 256*BASEGAIN/gain - 32;
	reg_gain = reg_gain & 0xFFFF;
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
	LOG_INF("set_gain");

	kal_uint16 reg_gain;
	kal_uint16 DigitalGain = 0;

// AG = 256/(B[7:0] + 32)
	// hi841離댕連넣8굡친콰gain,(7 * 255 / 256)굡鑒俚gain
	if(gain > 4080)  
	{
		gain= 4080;
	}
	if(gain < 58)  // gain돨reg離댕令角255
	{
		gain = 58;
	}

	if(gain <= 8 * BASEGAIN)
	{
		if(imgsensor.gain > 512)
		{
			write_cmos_sensor(0x020e, 0x1);
			write_cmos_sensor(0x020f, 0x0);
			write_cmos_sensor(0x0210, 0x1);
			write_cmos_sensor(0x0211, 0x0);
			write_cmos_sensor(0x0212, 0x1);
			write_cmos_sensor(0x0213, 0x0);
			write_cmos_sensor(0x0214, 0x1);
			write_cmos_sensor(0x0215, 0x0);
		}
		reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain; 
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x ", gain, reg_gain);
	
		write_cmos_sensor(0x0104, 0x1);
		write_cmos_sensor(0x0205, reg_gain & 0xFF); 
		write_cmos_sensor(0x0104, 0x0);
		
	}
	else
	{
		spin_lock(&imgsensor_drv_lock);
		imgsensor.gain = gain;
		spin_unlock(&imgsensor_drv_lock);
		DigitalGain = gain / 2 ;  // DigitalGain = iGain * 256 / 64 / 8
		write_cmos_sensor(0x0104, 0x1);
		
		write_cmos_sensor(0x0205, 0);   // 8굡친콰gain
		write_cmos_sensor(0x020e,(DigitalGain >> 8 ) & 0x7);
		write_cmos_sensor(0x020f, DigitalGain & 0xff);
		write_cmos_sensor(0x0210,(DigitalGain >> 8 ) & 0x7);
		write_cmos_sensor(0x0211, DigitalGain & 0xff);
		write_cmos_sensor(0x0212,(DigitalGain >> 8 ) & 0x7);
		write_cmos_sensor(0x0213, DigitalGain & 0xff);	
		write_cmos_sensor(0x0214,(DigitalGain >> 8 ) & 0x7);
		write_cmos_sensor(0x0215, DigitalGain & 0xff);	

		write_cmos_sensor(0x0104, 0x0);

	}
	
return;
}	/*	set_gain  */
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

        write_cmos_sensor(0x3508, (se << 4) & 0xFF);
        write_cmos_sensor(0x3507, (se >> 4) & 0xFF);
        write_cmos_sensor(0x3506, (se >> 12) & 0x0F);

        set_gain(gain);
    }

}




static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d", image_mirror);

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
	
	switch (image_mirror) {
		case IMAGE_NORMAL:
			write_cmos_sensor(0x0101,0x00);
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0101,0x01);
			
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0101,0x02);
			
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x0101,0x03);

			break;
		default:
			LOG_INF("Error image_mirror setting");
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
/////// Sensor Information  /////////////////////////// 
//	Sensor        	: Hi-841                        
//	Date		: 2014-06-26                    
//	Image size      : -                             
//	MCLK/PCLK       : 24MHz / -Mhz                  
//	MIPI speed(Mbps): -Mbps (each lane)             
//	Frame Length    : -                             
//	Line Length	: 4008                          
//	Max Fps         : -fps	                        
//	Pixel order     : Green 1st (=GB)               
//	X/Y-flip        : X flip                        
//	I2C Address     : 0x40(Write), 0x41(Read)       
////////////////////////////////////////////////////////


// Initial setting ********
write_cmos_sensor(0x8400, 0x03);	
//write_cmos_sensor(0x0101, 0x01); // org
write_cmos_sensor(0x0101,0x02); // V flip
write_cmos_sensor(0x0200, 0x00); 
write_cmos_sensor(0x0201, 0xA8); 
write_cmos_sensor(0x0202, 0x09);   
write_cmos_sensor(0x0203, 0x8D);
write_cmos_sensor(0x0205, 0xac); 
write_cmos_sensor(0x0344, 0x00); 
write_cmos_sensor(0x0345, 0x28); 
write_cmos_sensor(0x0346, 0x00); 
write_cmos_sensor(0x0347, 0x28); 
write_cmos_sensor(0x0348, 0x0C); 
write_cmos_sensor(0x0349, 0xE7); 
write_cmos_sensor(0x034A, 0x09); 
write_cmos_sensor(0x034B, 0xB7);
  
write_cmos_sensor(0x0342, 0x0F); 
write_cmos_sensor(0x0343, 0xA8); 
 
write_cmos_sensor(0x0901, 0x20); 
   
write_cmos_sensor(0x0B04, 0x01); 
     
write_cmos_sensor(0x4098, 0x80); 
write_cmos_sensor(0x4099, 0x08); 
write_cmos_sensor(0x409A, 0x08); 
write_cmos_sensor(0x400A, 0x00); 
write_cmos_sensor(0x400B, 0x07); 
		
write_cmos_sensor(0x40A0, 0x01); 
     
write_cmos_sensor(0x40A1, 0x3D); 
write_cmos_sensor(0x40A2, 0x80); 
write_cmos_sensor(0x40A6, 0x16); 
write_cmos_sensor(0x40AC, 0x03); 
write_cmos_sensor(0x40AD, 0x1A); 
write_cmos_sensor(0x40AE, 0x06); 
write_cmos_sensor(0x40BC, 0x10); 
write_cmos_sensor(0x40BD, 0x4C); 
write_cmos_sensor(0x40BE, 0x18); 

write_cmos_sensor(0x4001, 0x0A); 
write_cmos_sensor(0x4002, 0x10); 
write_cmos_sensor(0x4003, 0x40); 
write_cmos_sensor(0x4004, 0x00); 
write_cmos_sensor(0x4005, 0x00); 
write_cmos_sensor(0x4007, 0x00); 

write_cmos_sensor(0x400C, 0x43); 
write_cmos_sensor(0x400D, 0x88); 
write_cmos_sensor(0x400E, 0x37); 
write_cmos_sensor(0x407B, 0x00); 

write_cmos_sensor(0x4100, 0x01); 
write_cmos_sensor(0x4101, 0x22); 
write_cmos_sensor(0x4103, 0x20); 

write_cmos_sensor(0x4048, 0x08); 
write_cmos_sensor(0x4078, 0x07); 

write_cmos_sensor(0x4049, 0xC2); 
write_cmos_sensor(0x404A, 0x01); 
write_cmos_sensor(0x404B, 0x03); 
write_cmos_sensor(0x404E, 0x22); 
write_cmos_sensor(0x40CB, 0x00); 

write_cmos_sensor(0x4058, 0x10); 
write_cmos_sensor(0x4059, 0x13); 
write_cmos_sensor(0x405A, 0xAA); 
write_cmos_sensor(0x405B, 0xFF); 
write_cmos_sensor(0x405C, 0x00); 
write_cmos_sensor(0x405D, 0xFF); 
write_cmos_sensor(0x405E, 0x55); 
write_cmos_sensor(0x407C, 0xF1); 
		
write_cmos_sensor(0x4051, 0x03); 
write_cmos_sensor(0x4053, 0x55); 
write_cmos_sensor(0x4054, 0x54); 
write_cmos_sensor(0x407D, 0x05); 
			
write_cmos_sensor(0x4041, 0x27); 
write_cmos_sensor(0x4042, 0x09);
write_cmos_sensor(0x4043, 0x7F); 
write_cmos_sensor(0x4079, 0x0F); 
write_cmos_sensor(0x40CA, 0x00); 
			
write_cmos_sensor(0x40C2, 0x01); 
write_cmos_sensor(0x40C3, 0x4F); 
write_cmos_sensor(0x40C6, 0x01); 
write_cmos_sensor(0x40C7, 0x4F); 
write_cmos_sensor(0x40C8, 0x01); 
write_cmos_sensor(0x40C9, 0x4F); 

write_cmos_sensor(0x0301, 0x03); 
write_cmos_sensor(0x0305, 0x05); 
write_cmos_sensor(0x0309, 0x03); 
			
write_cmos_sensor(0x8404, 0x3A); 
write_cmos_sensor(0x8405, 0x08); 

write_cmos_sensor(0x7400, 0x60); 
write_cmos_sensor(0x5810, 0x0C); 
write_cmos_sensor(0x8410, 0x00); 
write_cmos_sensor(0x8411, 0x18); 
write_cmos_sensor(0x8412, 0x18); 
write_cmos_sensor(0x8413, 0x18); 
write_cmos_sensor(0x8414, 0x18); 
write_cmos_sensor(0x8416, 0x3F); 
write_cmos_sensor(0x8417, 0x03); 
			
write_cmos_sensor(0x4003, 0x42); 
		
write_cmos_sensor(0x4130, 0x00); 
write_cmos_sensor(0x4131, 0x58); 
write_cmos_sensor(0x4132, 0x01); 
write_cmos_sensor(0x4133, 0x1A); 
write_cmos_sensor(0x4134, 0x03); 
write_cmos_sensor(0x4135, 0xDE); 
			
write_cmos_sensor(0x4136, 0x00); 
write_cmos_sensor(0x4137, 0x58); 
write_cmos_sensor(0x4138, 0x01); 
write_cmos_sensor(0x4139, 0x1A); 
			
write_cmos_sensor(0x410E, 0x00); 
write_cmos_sensor(0x410F, 0x76); 
write_cmos_sensor(0x410C, 0x01); 
write_cmos_sensor(0x410D, 0x17); 
			
write_cmos_sensor(0x4112, 0x01); 
write_cmos_sensor(0x4113, 0x74); 
write_cmos_sensor(0x4110, 0x03); 
write_cmos_sensor(0x4111, 0xDD); 
			
write_cmos_sensor(0x4106, 0x00); 
write_cmos_sensor(0x4107, 0x76); 
write_cmos_sensor(0x4104, 0x01); 
write_cmos_sensor(0x4105, 0x1A); 
			
write_cmos_sensor(0x410A, 0x01); 
write_cmos_sensor(0x410B, 0x74); 
write_cmos_sensor(0x4108, 0x03); 
write_cmos_sensor(0x4109, 0xDE); 
				
write_cmos_sensor(0x41D0, 0x00); 
write_cmos_sensor(0x41D1, 0xC7); 
write_cmos_sensor(0x41D2, 0x00); 
write_cmos_sensor(0x41D3, 0x9E); 
				
write_cmos_sensor(0x41D6, 0x00); 
write_cmos_sensor(0x41D7, 0xEF); 
write_cmos_sensor(0x41D4, 0x01); 
write_cmos_sensor(0x41D5, 0x1A); 
				
write_cmos_sensor(0x41D8, 0x02); 
write_cmos_sensor(0x41D9, 0xA9); 
write_cmos_sensor(0x41DA, 0x02); 
write_cmos_sensor(0x41DB, 0x0E); 
				
write_cmos_sensor(0x41DC, 0x03); 
write_cmos_sensor(0x41DD, 0xDE); 
write_cmos_sensor(0x41DE, 0x03); 
write_cmos_sensor(0x41DF, 0x43); 
	 			
write_cmos_sensor(0x41E0, 0x00); 
write_cmos_sensor(0x41E1, 0x9E); 
write_cmos_sensor(0x41E2, 0x01); 
write_cmos_sensor(0x41E3, 0x1A); 
	 			
write_cmos_sensor(0x41E4, 0x02); 
write_cmos_sensor(0x41E5, 0x0E); 
write_cmos_sensor(0x41E6, 0x03); 
write_cmos_sensor(0x41E7, 0xDE); 
	 			
write_cmos_sensor(0x41E8, 0x00); 
write_cmos_sensor(0x41E9, 0xC7); 
write_cmos_sensor(0x41EA, 0x01); 
write_cmos_sensor(0x41EB, 0x1A); 
	 			
write_cmos_sensor(0x41EC, 0x02); 
write_cmos_sensor(0x41ED, 0xA9); 
write_cmos_sensor(0x41EE, 0x03); 
write_cmos_sensor(0x41EF, 0xDE); 
	 			
write_cmos_sensor(0x41F0, 0x00); 
write_cmos_sensor(0x41F1, 0xEF); 
write_cmos_sensor(0x41F2, 0x01); 
write_cmos_sensor(0x41F3, 0x1A); 
	 			
write_cmos_sensor(0x41F4, 0x03); 
write_cmos_sensor(0x41F5, 0x43); 
write_cmos_sensor(0x41F6, 0x03); 
write_cmos_sensor(0x41F7, 0xDE); 
	 			
write_cmos_sensor(0x41B0, 0x00); 
write_cmos_sensor(0x41B1, 0x0A); 
write_cmos_sensor(0x41B2, 0x00); 
write_cmos_sensor(0x41B3, 0x58); 
	 			
write_cmos_sensor(0x413E, 0x00); 
write_cmos_sensor(0x413F, 0x08); 
write_cmos_sensor(0x4140, 0x00); 
write_cmos_sensor(0x4141, 0x2C); 
	 			
write_cmos_sensor(0x4142, 0x00); 
write_cmos_sensor(0x4143, 0x0A); 
write_cmos_sensor(0x4144, 0x00); 
write_cmos_sensor(0x4145, 0x26); 
	 			
write_cmos_sensor(0x414A, 0x00); 
write_cmos_sensor(0x414B, 0x0E); 
write_cmos_sensor(0x414C, 0x00); 
write_cmos_sensor(0x414D, 0x28); 
	 			
write_cmos_sensor(0x4156, 0x00); 
write_cmos_sensor(0x4157, 0x12); 
write_cmos_sensor(0x4158, 0x00); 
write_cmos_sensor(0x4159, 0x22); 
	 			
write_cmos_sensor(0x415E, 0x00); 
write_cmos_sensor(0x415F, 0x02); 
write_cmos_sensor(0x4160, 0x03); 
write_cmos_sensor(0x4161, 0xE6); 
	 			
write_cmos_sensor(0x4166, 0x00); 
write_cmos_sensor(0x4167, 0x04); 
write_cmos_sensor(0x4168, 0x03); 
write_cmos_sensor(0x4169, 0xE2); 
	 			
write_cmos_sensor(0x416A, 0x00); 
write_cmos_sensor(0x416B, 0x02); 
write_cmos_sensor(0x416C, 0x03); 
write_cmos_sensor(0x416D, 0xDE); 
	 			
write_cmos_sensor(0x41BA, 0x00); 
write_cmos_sensor(0x41BB, 0x06); 
write_cmos_sensor(0x41B8, 0x03); 
write_cmos_sensor(0x41B9, 0xDE); 
	 			
write_cmos_sensor(0x4162, 0x00); 
write_cmos_sensor(0x4163, 0x2c); 
write_cmos_sensor(0x4164, 0x00); 
write_cmos_sensor(0x4165, 0x2c); 
	 			
write_cmos_sensor(0x417A, 0x00); 
write_cmos_sensor(0x417B, 0x06); 
write_cmos_sensor(0x417C, 0x00); 
write_cmos_sensor(0x417D, 0x10); 
	 			
write_cmos_sensor(0x4182, 0x03); 
write_cmos_sensor(0x4183, 0xE0); 
write_cmos_sensor(0x4184, 0x03); 
write_cmos_sensor(0x4185, 0xE4); 
	 			
write_cmos_sensor(0x416E, 0x01); 
write_cmos_sensor(0x416F, 0x1A); 
write_cmos_sensor(0x4170, 0x01); 
write_cmos_sensor(0x4171, 0x2A); 
	 			
write_cmos_sensor(0x4176, 0x03); 
write_cmos_sensor(0x4177, 0xE0); 
write_cmos_sensor(0x4178, 0x03); 
write_cmos_sensor(0x4179, 0xE4); 
	 			
write_cmos_sensor(0x418A, 0x00); 
write_cmos_sensor(0x418B, 0x08); 
write_cmos_sensor(0x418C, 0x01); 
write_cmos_sensor(0x418D, 0x0A); 
	 			
write_cmos_sensor(0x418E, 0x01); 
write_cmos_sensor(0x418F, 0x10); 
write_cmos_sensor(0x4190, 0x03); 
write_cmos_sensor(0x4191, 0xDE); 
	 			
write_cmos_sensor(0x4192, 0x00); 
write_cmos_sensor(0x4193, 0x12); 
write_cmos_sensor(0x4194, 0x00); 
write_cmos_sensor(0x4195, 0x56); 
	 			
write_cmos_sensor(0x41AA, 0x01); 
write_cmos_sensor(0x41AB, 0x1C); 
write_cmos_sensor(0x41AC, 0x01); 
write_cmos_sensor(0x41AD, 0x72); 
	 			
write_cmos_sensor(0x4196, 0x00); 
write_cmos_sensor(0x4197, 0x30); 
write_cmos_sensor(0x4198, 0x00); 
write_cmos_sensor(0x4199, 0x50); 
	 			
write_cmos_sensor(0x419A, 0x00); 
write_cmos_sensor(0x419B, 0x30); 
write_cmos_sensor(0x419C, 0x00); 
write_cmos_sensor(0x419D, 0x58); 
	 			
write_cmos_sensor(0x4186, 0x00); 
write_cmos_sensor(0x4187, 0x11); 
write_cmos_sensor(0x4188, 0x00); 
write_cmos_sensor(0x4189, 0x58); 
	 			
write_cmos_sensor(0x419E, 0x00); 
write_cmos_sensor(0x419F, 0x60); 
write_cmos_sensor(0x41A0, 0x03); 
write_cmos_sensor(0x41A1, 0xDE); 
	 			
write_cmos_sensor(0x41C2, 0x00); 
write_cmos_sensor(0x41C3, 0x02); 
write_cmos_sensor(0x41C0, 0x00); 
write_cmos_sensor(0x41C1, 0x0C); 
	 			
write_cmos_sensor(0x41A4, 0x00); 
write_cmos_sensor(0x41A5, 0x02); 
write_cmos_sensor(0x41A2, 0x00); 
write_cmos_sensor(0x41A3, 0x0C); 
	 			
write_cmos_sensor(0x41A8, 0x00); 
write_cmos_sensor(0x41A9, 0x02); 
write_cmos_sensor(0x41A6, 0x00); 
write_cmos_sensor(0x41A7, 0x0C); 
	 			
write_cmos_sensor(0x41B4, 0x03); 
write_cmos_sensor(0x41B5, 0xDE); 
write_cmos_sensor(0x41B6, 0x03); 
write_cmos_sensor(0x41B7, 0xE6); 
	 			
write_cmos_sensor(0x9c02, 0x03);
write_cmos_sensor(0x9c03, 0xff);
write_cmos_sensor(0x9c04, 0x00);
write_cmos_sensor(0x9c05, 0x20);
write_cmos_sensor(0x9c06, 0x00);
write_cmos_sensor(0x9c07, 0x00);
write_cmos_sensor(0x9c08, 0x00);
write_cmos_sensor(0x9c00, 0x17);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03);
write_cmos_sensor(0x8400, 0x03); 
write_cmos_sensor(0x5400, 0x01); 
write_cmos_sensor(0x5810, 0x5b); 
write_cmos_sensor(0x5811, 0x12); 
write_cmos_sensor(0x5812, 0x07); 
write_cmos_sensor(0x5813, 0x0C); 
write_cmos_sensor(0x5814, 0x20); 
write_cmos_sensor(0x5815, 0x07); 
write_cmos_sensor(0x5820, 0x00); 
write_cmos_sensor(0x5821, 0x80); 
write_cmos_sensor(0x5822, 0x00); 
write_cmos_sensor(0x5823, 0x00); 
write_cmos_sensor(0x5824, 0x00); 
write_cmos_sensor(0x5825, 0x80); 
write_cmos_sensor(0x5826, 0x01); 
write_cmos_sensor(0x5827, 0x0f); 
write_cmos_sensor(0x5830, 0x88); 
write_cmos_sensor(0x5831, 0x8c); 
write_cmos_sensor(0x5832, 0x0c); 
write_cmos_sensor(0x5833, 0x20); 
write_cmos_sensor(0x5834, 0x3f); 
write_cmos_sensor(0x5835, 0x7f); 
write_cmos_sensor(0x5840, 0x0c); 
write_cmos_sensor(0x5841, 0x0c); 
write_cmos_sensor(0x5842, 0x10); 
write_cmos_sensor(0x5843, 0x1a); 
write_cmos_sensor(0x5844, 0x0f); 
write_cmos_sensor(0x5845, 0x0e); 
write_cmos_sensor(0x5846, 0x08); 
write_cmos_sensor(0x5847, 0x0f); 
write_cmos_sensor(0x5848, 0x0f); 
write_cmos_sensor(0x5850, 0x07); 
write_cmos_sensor(0x5851, 0x32); 
write_cmos_sensor(0x5852, 0xff); 
write_cmos_sensor(0x5853, 0x80); 
write_cmos_sensor(0x5854, 0x01); 
write_cmos_sensor(0x5855, 0x00); 
write_cmos_sensor(0x5856, 0x10); 
write_cmos_sensor(0x0114, 0x03); 
write_cmos_sensor(0x8006, 0x07);
write_cmos_sensor(0x8041, 0x00);
write_cmos_sensor(0x8042, 0x00);
write_cmos_sensor(0x8043, 0xA0);
write_cmos_sensor(0x8005, 0x64);	
write_cmos_sensor(0x8401, 0x11);

}
static void preview_setting(void)
{
// Preview Mode Setting                                                                                 
// MIPI Setting                                                                               
                                                                                              
write_cmos_sensor(0x0800, 0x57);  // tclk_post 234ns                                              
write_cmos_sensor(0x0801, 0x1F);  // ths_prepare 85ns                                             
write_cmos_sensor(0x0802, 0x09);  // ths prepare + ths_zero_min 360ns                             
write_cmos_sensor(0x0803, 0x26);  // ths_trail 80ns                                               
write_cmos_sensor(0x0804, 0x1F);  // tclk_trail_min 70ns                                          
write_cmos_sensor(0x0805, 0x17);  // tclk_prepare 64ns                                            
write_cmos_sensor(0x0806, 0x70);  // tclk prepare + tclk_zero 370ns                               
write_cmos_sensor(0x0807, 0x1F);  // tlpx 84ns                                                    
			// clk pre 90ns                                          
                                                                                             
write_cmos_sensor(0x0344, 0x00); // x_addr_start_Hi                                               
write_cmos_sensor(0x0345, 0x28); // x_addr_start_Lo (40, Display시에 20-23 은 잘리고 24 부터 출력)
write_cmos_sensor(0x0346, 0x00); // y_addr_start_Hi                                               
write_cmos_sensor(0x0347, 0x28); // y_addr_start_Lo (40, Display시에 22-23 은 잘리고 24 부터 출력)
write_cmos_sensor(0x0348, 0x0C); // x_addr_end_Hi                                                 
write_cmos_sensor(0x0349, 0xE7); // x_addr_end_Lo (3303, Display시에 3320-3323 은 잘림)           
write_cmos_sensor(0x034A, 0x09); // y_addr_end_Hi                                                 
write_cmos_sensor(0x034B, 0xB7); // y_addr_end_Lo (2487, Display시에 2504-2505 은 잘림)           
                                                                                        
write_cmos_sensor(0x0307, 0x30);  // pll multiplier                                               
write_cmos_sensor(0x0900, 0x01);  //  Binning mode enable                                         
write_cmos_sensor(0x0340, 0x04);  //    frame_length_lines_Hi                                     
write_cmos_sensor(0x0341, 0xE8);  //    frame_length_lines_Lo                                     
write_cmos_sensor(0x034C, 0x06);  //    X_output_size_Hi (1632)                                   
write_cmos_sensor(0x034D, 0x60);  //    X_output_size_Lo                                          
write_cmos_sensor(0x034E, 0x04);  //    Y_output_size_Hi (1224)                                   
write_cmos_sensor(0x034F, 0xC8);  //    Y_output_size_Lo                                          
write_cmos_sensor(0x0387, 0x03);  //    y_odd_inc                                                 
                                                                                             
write_cmos_sensor(0x6400, 0x00);    //digital crop enable                                         
write_cmos_sensor(0x0401, 0x00);    //scaling_mode                                                
write_cmos_sensor(0x6C00, 0x00);    //fifo_enable                                                 
write_cmos_sensor(0x6C08, 0x00);    //fifo_op_line_length_offset                                  
write_cmos_sensor(0x7407, 0x00);    

write_cmos_sensor(0x0100, 0x01);  // sleep on   
}
static void capture_setting(kal_uint16 currefps)
{
// Capture Mode Setting                                                                                       
// MIPI Setting                                                                               
write_cmos_sensor(0x0800, 0x77);  // tclk_post 165ns                                              
write_cmos_sensor(0x0801, 0x2F);  // ths_prepare 64ns                                             
write_cmos_sensor(0x0802, 0x18);  // ths prepare + ths_zero_min 223ns                             
write_cmos_sensor(0x0803, 0x3f);  // ths_trail 81ns                                               
write_cmos_sensor(0x0804, 0x3c);  // tclk_trail_min 67ns                                          
write_cmos_sensor(0x0805, 0x2f);  // tclk_prepare 65ns                                            
write_cmos_sensor(0x0806, 0xc0);  // tclk prepare + tclk_zero 325ns                               
write_cmos_sensor(0x0807, 0x2F);  // tlpx 62ns                                                    
                        // tclk_pre 45ns                                                     
                                                                                             
write_cmos_sensor(0x0344, 0x00); // x_addr_start_Hi                                               
write_cmos_sensor(0x0345, 0x28); // x_addr_start_Lo (40, Display시에 20-23 은 잘리고 24 부터 출력)
write_cmos_sensor(0x0346, 0x00); // y_addr_start_Hi                                               
write_cmos_sensor(0x0347, 0x28); // y_addr_start_Lo (40, Display시에 22-23 은 잘리고 24 부터 출력)
write_cmos_sensor(0x0348, 0x0C); // x_addr_end_Hi                                                 
write_cmos_sensor(0x0349, 0xE7); // x_addr_end_Lo (3303, Display시에 3320-3323 은 잘림)           
write_cmos_sensor(0x034A, 0x09); // y_addr_end_Hi                                                 
write_cmos_sensor(0x034B, 0xB7); // y_addr_end_Lo (2487, Display시에 2504-2505 은 잘림)           
                                                                                             
write_cmos_sensor(0x0307, 0x5F);  // pll multiplier                                               
write_cmos_sensor(0x0900, 0x00);  //  Binning mode enable                                         
write_cmos_sensor(0x0340, 0x09);  //	frame_length_lines_Hi                                 
write_cmos_sensor(0x0341, 0xD4);  //	frame_length_lines_Lo                                 
write_cmos_sensor(0x034C, 0x0C);  //	X_output_size_Hi (3264)                               
write_cmos_sensor(0x034D, 0xC0);  //	X_output_size_Lo                                      
write_cmos_sensor(0x034E, 0x09);  //	Y_output_size_Hi (2448)                               
write_cmos_sensor(0x034F, 0x90);  //	Y_output_size_Lo                                      
write_cmos_sensor(0x0387, 0x01);  //	y_odd_inc                                             
                                                                                             
write_cmos_sensor(0x6400, 0x00);	//digital crop enable                                         
write_cmos_sensor(0x0401, 0x00);	//scaling_mode                                                
write_cmos_sensor(0x6C00, 0x00);	//fifo_enable                                                 
write_cmos_sensor(0x6C08, 0x00);	//fifo_op_line_length_offset                                  
write_cmos_sensor(0x7407, 0x00);                                                                  

write_cmos_sensor(0x0100, 0x01);  // sleep on            

}
static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	
		// Video Mode Setting
	// Video Mode Setting    

write_cmos_sensor(0x0800, 0x77);  // tclk_post 165ns                                              
write_cmos_sensor(0x0801, 0x2F);  // ths_prepare 64ns                                             
write_cmos_sensor(0x0802, 0x3f);  // ths prepare + ths_zero_min 257ns                             
write_cmos_sensor(0x0803, 0x3f);  // ths_trail 81ns                                               
write_cmos_sensor(0x0804, 0x3c);  // tclk_trail_min 67ns                                          
write_cmos_sensor(0x0805, 0x2f);  // tclk_prepare 65ns                                            
write_cmos_sensor(0x0806, 0xc0);  // tclk prepare + tclk_zero 325ns                               
write_cmos_sensor(0x0807, 0x2F);  // tlpx 62ns                                                    
                      // tclk_pre 46ns                                                   
                                                                                          
write_cmos_sensor(0x0344, 0x02); // x_addr_start_Hi                                               
write_cmos_sensor(0x0345, 0xC8); // x_addr_start_Lo (40, Display시에 20-23 은 잘리고 24 부터 출력)
write_cmos_sensor(0x0346, 0x02); // y_addr_start_Hi                                               
write_cmos_sensor(0x0347, 0xD4); // y_addr_start_Lo (40, Display시에 22-23 은 잘리고 24 부터 출력)
write_cmos_sensor(0x0348, 0x0A); // x_addr_end_Hi                                                 
write_cmos_sensor(0x0349, 0x47); // x_addr_end_Lo (3303, Display시에 3320-3323 은 잘림)           
write_cmos_sensor(0x034A, 0x07); // y_addr_end_Hi                                                 
write_cmos_sensor(0x034B, 0x0B); // y_addr_end_Lo (2487, Display시에 2504-2505 은 잘림)           
                                                                                        
write_cmos_sensor(0x0307, 0x5f);  // pll multiplier                                               
write_cmos_sensor(0x0900, 0x00);  //  Binning mode enable                                         
write_cmos_sensor(0x0340, 0x04);  //    frame_length_lines_Hi                                     
write_cmos_sensor(0x0341, 0x58);  //    frame_length_lines_Lo                                     
write_cmos_sensor(0x034C, 0x07);  //    X_output_size_Hi (1920)                                   
write_cmos_sensor(0x034D, 0x80);  //    X_output_size_Lo                                          
write_cmos_sensor(0x034E, 0x04);  //    Y_output_size_Hi (1080)                                   
write_cmos_sensor(0x034F, 0x38);  //    Y_output_size_Lo                                          
write_cmos_sensor(0x0387, 0x01);  //    y_odd_inc                                                 
                                                                                             
write_cmos_sensor(0x6400, 0x00);    //digital crop enable                                         
write_cmos_sensor(0x0401, 0x00);    //scaling_mode                                                
write_cmos_sensor(0x6C00, 0x00);    //fifo_enable                                                 
write_cmos_sensor(0x6C08, 0x00);    //fifo_op_line_length_offset                                  
write_cmos_sensor(0x7407, 0x00); 

write_cmos_sensor(0x0100, 0x01);  // sleep on        
}
static void hs_video_setting()
{
    LOG_INF("E\n");
//Preview Mode VGA Setting                                                       
// MIPI Setting                                                                
write_cmos_sensor(0x0800, 0x77);  // tclk_post 158ns                                                                  
write_cmos_sensor(0x0801, 0x28);  // ths_prepare 56ns                                                                 
write_cmos_sensor(0x0802, 0x20);  // ths prepare + ths_zero_min 234ns                                                 
write_cmos_sensor(0x0803, 0x3a);  // ths_trail 76ns                                                                   
write_cmos_sensor(0x0804, 0x3a);  // tclk_trail_min 66ns                                                              
write_cmos_sensor(0x0805, 0x28);  // tclk_prepare 54ns                                                                
write_cmos_sensor(0x0806, 0xBF);  // tclk prepare + tclk_zero 316ns                                                   
write_cmos_sensor(0x0807, 0x30);  // tlpx 66ns                                                                        
                                                                            
write_cmos_sensor(0x0344, 0x01); // x_addr_start_Hi                                
write_cmos_sensor(0x0345, 0x60); //68); // x_addr_start_Lo (360)                   
write_cmos_sensor(0x0346, 0x01); // y_addr_start_Hi                                
write_cmos_sensor(0x0347, 0x28); // y_addr_start_Lo (296)                          
write_cmos_sensor(0x0348, 0x0B); // x_addr_end_Hi                                  
write_cmos_sensor(0x0349, 0xAF); //A7); // x_addr_end_Lo (2983)                    
write_cmos_sensor(0x034A, 0x08); // y_addr_end_Hi                                  
write_cmos_sensor(0x034B, 0xB7); // y_addr_end_Lo (2231)                           
	                                                                           
write_cmos_sensor(0x0307, 0x5f);  // pll multiplier                                
write_cmos_sensor(0x0900, 0x00);  //  Binning mode enable                          
write_cmos_sensor(0x0340, 0x02);  //    frame_length_lines_Hi                      
write_cmos_sensor(0x0341, 0x74);  //    frame_length_lines_Lo                      
write_cmos_sensor(0x034C, 0x02);  //    X_output_size_Hi (640)                     
write_cmos_sensor(0x034D, 0x80);  //    X_output_size_Lo                           
write_cmos_sensor(0x034E, 0x01);  //    Y_output_size_Hi (480)                     
write_cmos_sensor(0x034F, 0xE0);  //    Y_output_size_Lo                           
write_cmos_sensor(0x0387, 0x07);  //    y_odd_inc                                  
                                                                              
// 0x624 x 484                                                                  
write_cmos_sensor(0x6400, 0x01);  //digital crop enable		                  
write_cmos_sensor(0x040C, 0x0A);  //digital_crop_image_width_h  (2624)             
write_cmos_sensor(0x040D, 0x50);  //digital_crop_image_width_l 0x40                
write_cmos_sensor(0x040E, 0x01);  //digital_crop_image_height_h (484)              
write_cmos_sensor(0x040F, 0xE4);  //digital_crop_image_height_1                    
write_cmos_sensor(0x0401, 0x01);  //scaling_mode 			          
write_cmos_sensor(0x0403, 0x01);  //spatial_sampling                               
write_cmos_sensor(0x0404, 0x00);  //scale_m			                  
write_cmos_sensor(0x0405, 0x40);  //scale_m					  
write_cmos_sensor(0x6C00, 0x01);  //fifo_enable				          
write_cmos_sensor(0x0700, 0x00);  //fifo_water_mark_pixels_h                       
write_cmos_sensor(0x0701, 0x08);  //fifo_water_mark_pixels_l                       
write_cmos_sensor(0x6C04, 0x02);  //fifo_x_out_size_h (668)			  
write_cmos_sensor(0x6C05, 0x98);  //fifo_x_out_size_l		                  
write_cmos_sensor(0x6C06, 0x01);  //fifo_y_out_size_h		                  
write_cmos_sensor(0x6C07, 0xE2);  //fifo_y_out_size_l		                  
write_cmos_sensor(0x6C08, 0x00);  //fifo_op_line_length_offset                     
write_cmos_sensor(0x6C01, 0x00);  // 0x00                                          
write_cmos_sensor(0x6C09, 0x00);  // 0x00                                          
write_cmos_sensor(0x6C0A, 0x00);  // 0x00                                          
write_cmos_sensor(0x7407, 0x04);                                                   
	                                                                           
write_cmos_sensor(0x0100, 0x01);   // sleep on                
}

static void slim_video_setting()
{
    LOG_INF("E\n");
    //@@video_720p_30fps_800Mbps
    // Preview Mode VGA Setting                                                       
// MIPI Setting                                                                
write_cmos_sensor(0x0800, 0x57); // tclk_post 244ns                         
write_cmos_sensor(0x0801, 0x1F); // ths_prepare 85ns                        
write_cmos_sensor(0x0802, 0x00); // ths prepare + ths_zero_min 342ns
write_cmos_sensor(0x0803, 0x26); // ths_trail 99ns                          
write_cmos_sensor(0x0804, 0x1F); // tclk_trail_min 70ns                     
write_cmos_sensor(0x0805, 0x17); // tclk_prepare 64ns                       
write_cmos_sensor(0x0806, 0x70); // tclk prepare + tclk_zero 370ns          
write_cmos_sensor(0x0807, 0x1F); // tlpx 84ns                               
   			// clk pre 90ns                     
	                                                                     
write_cmos_sensor(0x0344, 0x01);// x_addr_start_Hi                          
write_cmos_sensor(0x0345, 0x88);// x_addr_start_Lo (392 )                   
write_cmos_sensor(0x0346, 0x02);// y_addr_start_Hi                          
write_cmos_sensor(0x0347, 0x20);// y_addr_start_Lo (220 )                   
write_cmos_sensor(0x0348, 0x0B);// x_addr_end_Hi                            
write_cmos_sensor(0x0349, 0x87);// x_addr_end_Lo (2951 )                    
write_cmos_sensor(0x034A, 0x07);// y_addr_end_Hi                            
write_cmos_sensor(0x034B, 0xBF);// y_addr_end_Lo (1983 )                    
	                                                                     
write_cmos_sensor(0x0307, 0x30); // pll multiplier                          
write_cmos_sensor(0x0900, 0x01); //  Binning mode enable                    
write_cmos_sensor(0x0340, 0x04); //    frame_length_lines_Hi                
write_cmos_sensor(0x0341, 0xE8); //    frame_length_lines_Lo                
write_cmos_sensor(0x034C, 0x05); //    X_output_size_Hi (1280)              
write_cmos_sensor(0x034D, 0x00); //    X_output_size_Lo                     
write_cmos_sensor(0x034E, 0x02); //    Y_output_size_Hi (720)               
write_cmos_sensor(0x034F, 0xD0); //    Y_output_size_Lo                     
write_cmos_sensor(0x0387, 0x03); //    y_odd_inc                            
                                                                       
write_cmos_sensor(0x6400, 0x00);   //digital crop enable                    
write_cmos_sensor(0x0401, 0x00);   //scaling_mode                           
write_cmos_sensor(0x6C00, 0x00);   //fifo_enable                            
write_cmos_sensor(0x6C08, 0x00);   //fifo_op_line_length_offset             
write_cmos_sensor(0x7407, 0x00);        

write_cmos_sensor(0x0100, 0x01);  // sleep on     
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

LOG_INF("[get_imgsensor_id] ");
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address

	write_cmos_sensor(0x8408,0x0a);
	write_cmos_sensor(0x0103,0x01) ;
	write_cmos_sensor(0x0103,0x00);
	write_cmos_sensor(0x8400 ,0x03 );// system_enable (Parallel mode enable, System clock enable)

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
			spin_lock(&imgsensor_drv_lock);
			imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
			spin_unlock(&imgsensor_drv_lock);
			do {
				*sensor_id = ((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
				if (*sensor_id == imgsensor_info.sensor_id) {				
					LOG_INF("i2c write id  : 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);	  
					return ERROR_NONE;
				}	
				LOG_INF("get_imgsensor_id Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
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
	LOG_INF("[open] ");
	//const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0; 
	LOG_INF("[open]: PLATFORM:MT6571,MIPI 2LANE\n");
	LOG_INF("preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n");

	write_cmos_sensor(0x8408,0x0a);
	write_cmos_sensor(0x0103,0x01) ;
	write_cmos_sensor(0x0103,0x00);
	write_cmos_sensor(0x8400 ,0x03 );
	LOG_INF("WR\n");
	
	
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		LOG_INF("SP\n");
		do {
			sensor_id = ((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
			if (sensor_id == imgsensor_info.sensor_id) {				
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);	  
				break;
			}	
			LOG_INF("open:Read sensor id fail open w, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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
	LOG_INF("A\n");

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x100;
	imgsensor.gain = 0xe0;
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
       LOG_INF("B\n");
	return ERROR_NONE;
}	/*	open  */



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
	LOG_INF("close E");

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
	LOG_INF("E");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength; 
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
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
	LOG_INF("E");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	imgsensor.current_fps = 300;
	
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.current_fps = imgsensor_info.cap.max_framerate;
		//imgsensor.current_fps = imgsensor_info.cap.max_framerate;
		imgsensor.autoflicker_en = KAL_FALSE;
	
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps); 
	
	
	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");
	
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;  
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	
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
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    hs_video_setting();	
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
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
	//set_mirror_flip(sensor_config_data->SensorImageMirror);

    return ERROR_NONE;
}    /*    slim_video     */






static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
    LOG_INF("E\n");
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
	LOG_INF("[contrlo]scenario_id = %d", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		
			LOG_INF("preview\n");
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			case MSDK_SCENARIO_ID_CAMERA_ZSD:
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
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	
	if ((framerate == 30) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 15) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = 10 * framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps,1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{

	
	LOG_INF("enable = %d, framerate = %d ", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) 	  
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
         //   set_dummy();
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
          //  set_dummy();
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
          //  set_dummy();
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
            imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
          //  set_dummy();
            break;
        default:  //coding with  preview scenario by default
            frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
          //  set_dummy();
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

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d", enable);

	if (enable) {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor(0x0600, 0x00);
		write_cmos_sensor(0x0601,0x02);		
	} else {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor(0x0600, 0x00);
		write_cmos_sensor(0x0601,0x00);	
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
    unsigned long long *feature_return_para=(unsigned long long *) feature_para;

    SENSOR_WINSIZE_INFO_STRUCT *wininfo;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

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
        case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
            LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            break;
        default:
            break;
    }

    return ERROR_NONE;
}    /*    feature_control()  */

SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 HI841_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	HI841_MIPI_RAW_SensorInit	*/
