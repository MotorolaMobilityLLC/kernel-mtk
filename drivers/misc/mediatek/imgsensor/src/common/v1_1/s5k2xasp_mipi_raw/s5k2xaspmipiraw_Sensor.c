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
 *	 S5K2XASPmipi_Sensor.c
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
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "s5k2xaspmipiraw_Sensor.h"

#define PFX "S5K2XASP_camera_sensor"
#define LOG_INF(format, args...)  pr_debug(PFX "[%s] " format, __func__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K2XASP_SENSOR_ID,

	.checksum_value = 0xffb1ec31,

	.pre = {
		.pclk = 864000000,
		.linelength = 11744,
		.framelength = 2450,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 223200000,
	},
	.cap = {
		.pclk = 864000000,
		.linelength = 11744,
		.framelength = 2450,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 223200000,
	},
	.normal_video = {
		.pclk = 960000000,
		.linelength = 11520,
		.framelength = 2775,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 223200000,
	},
	.hs_video = {
		.pclk = 800000000,
		.linelength = 7984,
		.framelength = 835,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1344,
		.grabwindow_height = 756,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 960000000,
		.linelength = 11520,
		.framelength = 2775,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},

	.margin = 5,
	.min_shutter = 4,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  /*1, support; 0,not support*/
	.ihdr_le_firstline = 0,  /*1,le first; 0, se first*/
	.sensor_mode_num = 5,	  /*support sensor mode num*/

	.cap_delay_frame = 2,/*3 guanjd modify */
	.pre_delay_frame = 2,/*3 guanjd modify */
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_2MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	/*0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2*/
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = 1, /* 0,MIPI_SETTLEDELAY_AUTO;
				      * 1,MIPI_SETTLEDELAY_MANNUAL
				      */
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R, /*Gr*/
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x20, 0x5A, 0xff},
	.i2c_speed = 385,
};


static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_H_MIRROR, /*mirrorflip information*/
	/* IMGSENSOR_MODE enum value,record current sensor mode,such as:
	 * INIT, Preview, Capture, Video,High Speed Video, Slim Video
	 */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,	/*current shutter*/
	.gain = 0x100,		/*current gain*/
	.dummy_pixel = 0,	/*current dummypixel*/
	.dummy_line = 0,	/*current dummyline*/
	/*full size current fps : 24fps for PIP, 30fps for Normal or ZSD*/
	.current_fps = 0,
	/*auto flicker enable: KAL_FALSE for disable auto flicker,
	 * KAL_TRUE for enable auto flicker
	 */
	.autoflicker_en = KAL_FALSE,
	/*test pattern mode or not.
	 * KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	 */
	.test_pattern = KAL_FALSE,
	/*current scenario id*/
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_mode = 0, /*sensor need support LE, SE with HDR feature*/
	.i2c_write_id = 0x20,

};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
	{ 5664, 4256, 16, 16, 5632, 4224, 2816, 2112,
		0000, 0000, 2816, 2112, 0, 0, 2816, 2112}, /*Preview*/
	{ 5664, 4256, 16, 16, 5632, 4224, 2816, 2112,
		0000, 0000, 2816, 2112, 0, 0, 2816, 2112}, /*capture*/
	{ 5664, 4256, 16, 16, 5632, 4224, 2816, 2112,
		0000, 0000, 2816, 2112, 0, 0, 2816, 2112}, /*video*/
	{ 4032, 3024, 0, 0, 4032, 3024, 1344, 756,
		0000, 0000, 1344, 756, 0, 0, 1344, 756}, /*hs_video,don't use*/
	{ 5664, 4256, 16, 16, 5632, 4224, 2816, 2112,
		0000, 0000, 2816, 2112, 0, 0, 2816, 2112}, /* slim video*/
}; /*cpy from preview*/


static struct IMGSENSOR_I2C_CFG *get_i2c_cfg(void)
{
	return &(((struct IMGSENSOR_SENSOR_INST *)
		  (imgsensor.psensor_func->psensor_inst))->i2c_cfg);
}

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	imgsensor_i2c_read(
			   get_i2c_cfg(),
			   pusendcmd,
			   2,
			   (u8 *)&get_byte,
			   2,
			   imgsensor.i2c_write_id,
			   imgsensor_info.i2c_speed);

	return ((get_byte << 8) & 0xff00) | ((get_byte >> 8) & 0x00ff);
}


static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF),
		(char)(para >> 8), (char)(para & 0xFF)};

	imgsensor_i2c_write(
			    get_i2c_cfg(),
			    pusendcmd,
			    4,
			    4,
			    imgsensor.i2c_write_id,
			    imgsensor_info.i2c_speed);
}

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	imgsensor_i2c_read(
			   get_i2c_cfg(),
			   pusendcmd,
			   2,
			   (u8 *)&get_byte,
			   1,
			   imgsensor.i2c_write_id,
			   imgsensor_info.i2c_speed);

	return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[4] = {(char)(addr >> 8),
		(char)(addr & 0xFF), (char)(para & 0xFF)};

	imgsensor_i2c_write(
			    get_i2c_cfg(),
			    pusendcmd,
			    3,
			    3,
			    imgsensor.i2c_write_id,
			    imgsensor_info.i2c_speed);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n",
		imgsensor.dummy_line, imgsensor.dummy_pixel);
	write_cmos_sensor(0x0340, imgsensor.frame_length);
	write_cmos_sensor(0x0342, imgsensor.line_length);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{

	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable %d\n",
		framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	if (frame_length >= imgsensor.min_frame_length)
		imgsensor.frame_length = frame_length;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length
		- imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length -
			imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */

static void write_shutter(kal_uint16 shutter)
{

	kal_uint16 realtime_fps = 0;


	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	if (shutter < imgsensor_info.min_shutter)
		shutter = imgsensor_info.min_shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10
			/ imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length*/
			write_cmos_sensor(0x0340, imgsensor.frame_length);

		}
	} else {
		/* Extend frame length*/
		write_cmos_sensor(0x0340, imgsensor.frame_length);

	}
	/* Update Shutter*/
	write_cmos_sensor(0x0202, shutter);
	LOG_INF("shutter = %d, framelength = %d\n", shutter,
		imgsensor.frame_length);

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
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}	/*	set_shutter */

/*	write_shutter  */
static void set_shutter_frame_length(kal_uint16 shutter,
				     kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	/* Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;

	/*  */
	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;

	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter)
		? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length
			      - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin)
		: shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10
			/ imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x0340,
					  imgsensor.frame_length & 0xFFFF);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	/* Update Shutter */
	write_cmos_sensor(0X0202, shutter & 0xFFFF);

	LOG_INF("shutter = %d, framelength = %d/%d, dummy_line= %d\n",
		shutter, imgsensor.frame_length,
		frame_length, dummy_line);

}				/*      write_shutter  */


static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0;

	reg_gain = gain/2;
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

	/*gain= 1024;for test*/
	/*return; for test*/

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

	write_cmos_sensor(0x0204, reg_gain);
	/*write_cmos_sensor_8(0x0204,(reg_gain>>8));*/
	/*write_cmos_sensor_8(0x0205,(reg_gain&0xff));*/

	return gain;
}	/*	set_gain  */

static void set_mirror_flip(kal_uint8 image_mirror)
{
	switch (image_mirror) {

	case IMAGE_NORMAL:
		write_cmos_sensor_8(0x0101, 0x00);   /* Gr*/
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor_8(0x0101, 0x01);
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor_8(0x0101, 0x02);
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor_8(0x0101, 0x03);/*Gb*/
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
#if 0
static void night_mode(kal_bool enable)
{
	/*No Need to implement this function*/
}	/*	night_mode	*/
#endif

static kal_uint32 streaming_control(kal_bool enable)
{
	int timeout = (10000 / imgsensor.current_fps) + 1;
	int i = 0;
	int framecnt = 0;

	LOG_INF("streaming_enable(0= Sw Standby,1= streaming): %d\n", enable);
	if (enable) {
		write_cmos_sensor_8(0x0100, 0X01);
		mDELAY(10);
	} else {
		write_cmos_sensor_8(0x0100, 0x00);
		mDELAY(60);
		for (i = 0; i < timeout; i++) {
			mDELAY(5);
			framecnt = read_cmos_sensor_8(0x0005);
			if (framecnt == 0xFF) {
				LOG_INF(" Stream Off OK at i=%d.\n", i);
				return ERROR_NONE;
			}
		}
		LOG_INF("Stream Off Fail! framecnt= %d.\n", framecnt);
	}
	return ERROR_NONE;
}

static void sensor_init(void)
{
	/*Global setting */
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7971);
	write_cmos_sensor(0x6218, 0x7150);
	mdelay(3);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x3DA4);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0449);
	write_cmos_sensor(0x6F12, 0x0348);
	write_cmos_sensor(0x6F12, 0x044A);
	write_cmos_sensor(0x6F12, 0x0860);
	write_cmos_sensor(0x6F12, 0x101A);
	write_cmos_sensor(0x6F12, 0x8880);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x7AB8);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3F84);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3220);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x6FD4);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF05F);
	write_cmos_sensor(0x6F12, 0x9146);
	write_cmos_sensor(0x6F12, 0x0E46);
	write_cmos_sensor(0x6F12, 0x0446);
	write_cmos_sensor(0x6F12, 0xDFF8);
	write_cmos_sensor(0x6F12, 0x04A1);
	write_cmos_sensor(0x6F12, 0x30E0);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x8CF8);
	write_cmos_sensor(0x6F12, 0x8346);
	write_cmos_sensor(0x6F12, 0x4FEA);
	write_cmos_sensor(0x6F12, 0x4835);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0x3D4F);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xB021);
	write_cmos_sensor(0x6F12, 0x05F5);
	write_cmos_sensor(0x6F12, 0x0055);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x86F8);
	write_cmos_sensor(0x6F12, 0x388D);
	write_cmos_sensor(0x6F12, 0xF98C);
	write_cmos_sensor(0x6F12, 0x411A);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x85F8);
	write_cmos_sensor(0x6F12, 0x788D);
	write_cmos_sensor(0x6F12, 0xB989);
	write_cmos_sensor(0x6F12, 0x411A);
	write_cmos_sensor(0x6F12, 0x5046);
	write_cmos_sensor(0x6F12, 0xAAF8);
	write_cmos_sensor(0x6F12, 0x1A13);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x1A13);
	write_cmos_sensor(0x6F12, 0x0029);
	write_cmos_sensor(0x6F12, 0xFBD1);
	write_cmos_sensor(0x6F12, 0x7021);
	write_cmos_sensor(0x6F12, 0xA0F8);
	write_cmos_sensor(0x6F12, 0x0E13);
	write_cmos_sensor(0x6F12, 0xC4F3);
	write_cmos_sensor(0x6F12, 0x0C01);
	write_cmos_sensor(0x6F12, 0xAE42);
	write_cmos_sensor(0x6F12, 0x00D2);
	write_cmos_sensor(0x6F12, 0x3546);
	write_cmos_sensor(0x6F12, 0x2D1B);
	write_cmos_sensor(0x6F12, 0x2B46);
	write_cmos_sensor(0x6F12, 0x4A46);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x72F8);
	write_cmos_sensor(0x6F12, 0x2C44);
	write_cmos_sensor(0x6F12, 0xA944);
	write_cmos_sensor(0x6F12, 0x5946);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x71F8);
	write_cmos_sensor(0x6F12, 0x600B);
	write_cmos_sensor(0x6F12, 0x5FEA);
	write_cmos_sensor(0x6F12, 0x0008);
	write_cmos_sensor(0x6F12, 0x01D1);
	write_cmos_sensor(0x6F12, 0xB442);
	write_cmos_sensor(0x6F12, 0xC8D3);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF09F);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0x4FF4);
	write_cmos_sensor(0x6F12, 0x0141);
	write_cmos_sensor(0x6F12, 0x0C20);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x68F8);
	write_cmos_sensor(0x6F12, 0x0446);
	write_cmos_sensor(0x6F12, 0x214D);
	write_cmos_sensor(0x6F12, 0xC005);
	write_cmos_sensor(0x6F12, 0x04D5);
	write_cmos_sensor(0x6F12, 0x2888);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0x2880);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x64F8);
	write_cmos_sensor(0x6F12, 0x2004);
	write_cmos_sensor(0x6F12, 0x06D5);
	write_cmos_sensor(0x6F12, 0x288C);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0x2884);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0x7040);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x60B8);
	write_cmos_sensor(0x6F12, 0x70BD);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0x0446);
	write_cmos_sensor(0x6F12, 0x1848);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x0168);
	write_cmos_sensor(0x6F12, 0x0D0C);
	write_cmos_sensor(0x6F12, 0x8EB2);
	write_cmos_sensor(0x6F12, 0x3146);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x59F8);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x5BF8);
	write_cmos_sensor(0x6F12, 0x34F8);
	write_cmos_sensor(0x6F12, 0x6A0F);
	write_cmos_sensor(0x6F12, 0x3146);
	write_cmos_sensor(0x6F12, 0xA080);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0x7040);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x4CB8);
	write_cmos_sensor(0x6F12, 0x10B5);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x6901);
	write_cmos_sensor(0x6F12, 0x0C48);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x4FF8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0xF701);
	write_cmos_sensor(0x6F12, 0x0A48);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x49F8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x4F01);
	write_cmos_sensor(0x6F12, 0x0848);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x43F8);
	write_cmos_sensor(0x6F12, 0x0449);
	write_cmos_sensor(0x6F12, 0x0860);
	write_cmos_sensor(0x6F12, 0x10BD);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0x8000);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3C50);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2730);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3F80);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x047B);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xE891);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x8655);
	write_cmos_sensor(0x6F12, 0x4EF2);
	write_cmos_sensor(0x6F12, 0x634C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4EF2);
	write_cmos_sensor(0x6F12, 0xC14C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4EF2);
	write_cmos_sensor(0x6F12, 0x555C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4EF2);
	write_cmos_sensor(0x6F12, 0x7D6C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4EF2);
	write_cmos_sensor(0x6F12, 0xB74C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F2);
	write_cmos_sensor(0x6F12, 0xC91C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4BF2);
	write_cmos_sensor(0x6F12, 0xD76C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x46F6);
	write_cmos_sensor(0x6F12, 0x251C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F6);
	write_cmos_sensor(0x6F12, 0x931C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x48F2);
	write_cmos_sensor(0x6F12, 0x556C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0x471C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x010C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1C84);
	write_cmos_sensor(0x6F12, 0x0A01);
	write_cmos_sensor(0x602A, 0x1C20);
	write_cmos_sensor(0x6F12, 0x0030);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1C86);
	write_cmos_sensor(0x6F12, 0x0250);
	write_cmos_sensor(0x602A, 0x1C88);
	write_cmos_sensor(0x6F12, 0x8060);
	write_cmos_sensor(0x602A, 0x1C5E);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x2112);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0100);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x010C);
	write_cmos_sensor(0x6F12, 0xFF08);
	write_cmos_sensor(0x602A, 0x0118);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0x8000);
	write_cmos_sensor(0x6F12, 0xC000);
	write_cmos_sensor(0x6F12, 0xFFFF);
	write_cmos_sensor(0x602A, 0x004A);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x00B4);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x00B0);
	write_cmos_sensor(0x6F12, 0x0359);
	write_cmos_sensor(0x602A, 0x13E0);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1D12);
	write_cmos_sensor(0x6F12, 0x1010);
	write_cmos_sensor(0x602A, 0x1D10);
	write_cmos_sensor(0x6F12, 0x0011);
	write_cmos_sensor(0x602A, 0x1D22);
	write_cmos_sensor(0x6F12, 0x7700);
	write_cmos_sensor(0x602A, 0x037A);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x03A8);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x03D6);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x0404);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x0432);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x320A, 0x0801);
	write_cmos_sensor(0x3206, 0x0100);
	write_cmos_sensor(0x112C, 0x4220);
	write_cmos_sensor(0x112E, 0x0000);
	write_cmos_sensor(0x3842, 0x340F);
	write_cmos_sensor(0xF404, 0x0FF3);
	write_cmos_sensor(0xF458, 0x0009);
	write_cmos_sensor(0xF45A, 0x000E);
	write_cmos_sensor(0xF45C, 0x000E);
	write_cmos_sensor(0xF45E, 0x0018);
	write_cmos_sensor(0xF460, 0x0010);
	write_cmos_sensor(0xF462, 0x0018);
	write_cmos_sensor(0x3850, 0x006B);
	write_cmos_sensor(0x382E, 0x0909);
	write_cmos_sensor(0x3830, 0x0B0F);
	write_cmos_sensor(0x3832, 0x0000);
	write_cmos_sensor(0xF424, 0x0060);
	write_cmos_sensor(0xF428, 0xC4C2);
	write_cmos_sensor(0x3834, 0x0808);
	write_cmos_sensor(0x3260, 0x0036);
	write_cmos_sensor(0x3270, 0x0030);
	write_cmos_sensor(0x3288, 0x0024);
	write_cmos_sensor(0x32A0, 0x0018);
	write_cmos_sensor(0x32C4, 0x0035);
	write_cmos_sensor(0x337C, 0x005A);
	write_cmos_sensor(0xF4B0, 0x1124);
	write_cmos_sensor(0xF4B4, 0x1125);
	write_cmos_sensor(0xF4B8, 0x112C);
	write_cmos_sensor(0xF4BC, 0x112D);
	write_cmos_sensor(0xF4C0, 0x1134);
	write_cmos_sensor(0xF4C4, 0x1135);
	write_cmos_sensor(0xF486, 0x0480);
	write_cmos_sensor(0x31E4, 0x0060);
	write_cmos_sensor(0x31E2, 0x0060);
	write_cmos_sensor(0x31E0, 0x0000);
	write_cmos_sensor(0xF46C, 0x002D);
	write_cmos_sensor(0xF47A, 0x1718);
	write_cmos_sensor(0xF47C, 0x0000);
	write_cmos_sensor(0x0B04, 0x0101);
	write_cmos_sensor(0x3050, 0x0000);
	write_cmos_sensor(0x3052, 0x0400);
	write_cmos_sensor(0xA83C, 0x0001);
	write_cmos_sensor(0x0B00, 0x0080);
	write_cmos_sensor(0x305E, 0x0000);
	write_cmos_sensor(0x3076, 0x0000);
	write_cmos_sensor(0x3054, 0x0000);
	write_cmos_sensor(0x30D0, 0x0000);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x306A, 0x0340);
	write_cmos_sensor(0x0B0E, 0x0000);
	write_cmos_sensor(0xB134, 0x0180);
	write_cmos_sensor(0xB13C, 0x0600);
	write_cmos_sensor(0x655E, 0x03E8);
	write_cmos_sensor(0x0200, 0x0000);
	write_cmos_sensor(0x021E, 0x0040);
	write_cmos_sensor(0x021C, 0x0000);
	write_cmos_sensor(0x6592, 0x0010);
	write_cmos_sensor(0x31F8, 0x1158);
	write_cmos_sensor(0x31FA, 0x0008);
	write_cmos_sensor(0xF160, 0x000B);
}	/*	sensor_init  */


static void preview_setting(void)
{
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7971);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x0344, 0x0010);
	write_cmos_sensor(0x0346, 0x0010);
	write_cmos_sensor(0x0348, 0x160F);
	write_cmos_sensor(0x034A, 0x108F);
	write_cmos_sensor(0x034C, 0x0B00);
	write_cmos_sensor(0x034E, 0x0840);
	write_cmos_sensor(0x0340, 0x0992);
	write_cmos_sensor(0x0342, 0x2DE0);
	write_cmos_sensor(0x3000, 0x0001);
	write_cmos_sensor(0x3002, 0x0100);
	write_cmos_sensor(0x0900, 0x0111);
	write_cmos_sensor(0x0380, 0x0002);
	write_cmos_sensor(0x0382, 0x0002);
	write_cmos_sensor(0x0384, 0x0002);
	write_cmos_sensor(0x0386, 0x0002);
	write_cmos_sensor(0x3070, 0x0000);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x0460);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x0462);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0464);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x3072, 0x0000);
	write_cmos_sensor(0x0B04, 0x0101);
	write_cmos_sensor(0x0B08, 0x0000);
	write_cmos_sensor(0x306E, 0x0000);
	write_cmos_sensor(0x3074, 0x0000);
	write_cmos_sensor(0x3004, 0x0001);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x013E, 0x0000);
	write_cmos_sensor(0x0300, 0x0003);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0004);
	write_cmos_sensor(0x0306, 0x00D8);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0004);
	write_cmos_sensor(0x030E, 0x00BA);
	write_cmos_sensor(0x300A, 0x0001);
	write_cmos_sensor(0x300C, 0x0001);
	write_cmos_sensor(0x324A, 0x0300);
	write_cmos_sensor(0x31C0, 0x0004);
	write_cmos_sensor(0x31A0, 0x0100);
	write_cmos_sensor(0x31A2, 0x001C);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1C22);
	write_cmos_sensor(0x6F12, 0x0081);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0202, 0x0820);
	write_cmos_sensor(0x0204, 0x0020);
	write_cmos_sensor(0x0216, 0x0000);
	write_cmos_sensor(0xF426, 0x00A0);
	write_cmos_sensor(0xF150, 0x000B);
	write_cmos_sensor(0x0612, 0x0000);
	write_cmos_sensor(0x3840, 0x006C);
	write_cmos_sensor(0xF41E, 0x0000);
	write_cmos_sensor(0x3844, 0x00BF);
}	/*	preview_setting  */

/* Pll Setting - VCO = 280Mhz*/
#if 0
static void capture_setting(kal_uint16 currefps)
{
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7971);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x0344, 0x0000);
	write_cmos_sensor(0x0346, 0x0000);
	write_cmos_sensor(0x0348, 0x161F);
	write_cmos_sensor(0x034A, 0x109F);
	write_cmos_sensor(0x034C, 0x1620);
	write_cmos_sensor(0x034E, 0x10A0);
	write_cmos_sensor(0x0340, 0x12A0);
	write_cmos_sensor(0x0342, 0x2358);
	write_cmos_sensor(0x3000, 0x0001);
	write_cmos_sensor(0x3002, 0x0000);
	write_cmos_sensor(0x0900, 0x0111);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0001);
	write_cmos_sensor(0x3070, 0x0000);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x0460);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x0462);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x602A, 0x0464);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x3072, 0x0001);
	write_cmos_sensor(0x0B04, 0x0101);
	write_cmos_sensor(0x0B08, 0x0000);
	write_cmos_sensor(0x306E, 0x0000);
	write_cmos_sensor(0x3074, 0x0000);
	write_cmos_sensor(0x3004, 0x0001);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x013E, 0x0000);
	write_cmos_sensor(0x0300, 0x0003);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0004);
	write_cmos_sensor(0x0306, 0x00D8);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0002);
	write_cmos_sensor(0x030E, 0x007D);
	write_cmos_sensor(0x300A, 0x0001);
	write_cmos_sensor(0x300C, 0x0000);
	write_cmos_sensor(0x324A, 0x0100);
	write_cmos_sensor(0x31C0, 0x0008);
	write_cmos_sensor(0x31A0, 0x0100);
	write_cmos_sensor(0x31A2, 0x0020);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1C22);
	write_cmos_sensor(0x6F12, 0x0081);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0202, 0x0820);
	write_cmos_sensor(0x0204, 0x0020);
	write_cmos_sensor(0x0216, 0x0000);
	write_cmos_sensor(0xF426, 0x00A1);
	write_cmos_sensor(0xF150, 0x000A);
	write_cmos_sensor(0x0612, 0x0001);
	write_cmos_sensor(0x3840, 0x0096);
}
#endif
#if 0
static void normal_video_setting(kal_uint16 currefps)
{

}
#endif
static void hs_video_setting(void)
{

}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
	preview_setting();
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
	kal_uint8 retry = 2;

	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = ((read_cmos_sensor_8(0x0000) << 8)
				      | read_cmos_sensor_8(0x0001));
			LOG_INF("read out sensor id 0x%x\n", *sensor_id);
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail, id: 0x%x\n",
				imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id !=  imgsensor_info.sensor_id) {
		/* if Sensor ID is not correct,
		 * Must set *sensor_id to 0xFFFFFFFF
		 */
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
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;

	LOG_INF("PLATFORM:MT6750,MIPI 4LANE\n");
	LOG_INF(
	"preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n");

	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = ((read_cmos_sensor_8(0x0000) << 8)
				     | read_cmos_sensor_8(0x0001));
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x\n",
				imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id !=  sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

	/* initail sequence write in  */
	sensor_init();

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_mode = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

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
 *  *image_window : address pointer of pixel numbers in one period of HSYNC
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
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	preview_setting();
	set_mirror_flip(imgsensor.mirror);

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
	if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
		LOG_INF(
		"Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
		imgsensor.current_fps,
		imgsensor_info.cap.max_framerate / 10);
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	/*	 capture_setting(imgsensor.current_fps);*/
	preview_setting();
	set_mirror_flip(imgsensor.mirror);

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
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	/* normal_video_setting(imgsensor.current_fps);*/
	preview_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Don't use, no setting E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	/*imgsensor.video_mode = KAL_TRUE;*/
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/*imgsensor.current_fps = 300;*/
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			     MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	/*imgsensor.video_mode = KAL_TRUE;*/
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/*imgsensor.current_fps = 300;*/
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	slim_video	 */



static kal_uint32 get_resolution(
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT * sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth =
		imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight =
		imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth =
		imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight =
		imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth =
		imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight =
		imgsensor_info.normal_video.grabwindow_height;

	sensor_resolution->SensorHighSpeedVideoWidth =
		imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight =
		imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth =
		imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight =
		imgsensor_info.slim_video.grabwindow_height;

	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* not use */
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* inverse with datasheet*/
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame =
		imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	/* The frame of setting shutter default 0 for TG int */
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	/* The frame of setting sensor gain */
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 0;
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  /* 0 is default 1x*/
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x*/
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

		sensor_info->SensorGrabStartX =
			imgsensor_info.normal_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.normal_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		sensor_info->SensorGrabStartX =
			imgsensor_info.slim_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.slim_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

		break;
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate*/
	if (framerate == 0)
		/* Dynamic frame rate*/
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) /*enable auto flicker*/
		imgsensor.autoflicker_en = KAL_TRUE;
	else /*Cancel Auto flick*/
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(
	MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk / framerate * 10
			/ imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
			? (frame_length - imgsensor_info.pre.framelength)
			: 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		/*set_dummy();*/
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk / framerate * 10
			/ imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.normal_video.framelength)
			? (frame_length -
			   imgsensor_info.normal_video.framelength)
			: 0;
		imgsensor.frame_length = imgsensor_info.normal_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		/*set_dummy();*/
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF(
			"Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			framerate,
			imgsensor_info.cap.max_framerate / 10);
		frame_length = imgsensor_info.cap.pclk / framerate * 10
			/ imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap.framelength)
			? (frame_length - imgsensor_info.cap.framelength)
			: 0;
		imgsensor.frame_length = imgsensor_info.cap.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		/*set_dummy();*/
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10
			/ imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.hs_video.framelength)
			? (frame_length - imgsensor_info.hs_video.framelength)
			: 0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		/*set_dummy();*/
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10
			/ imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.slim_video.framelength)
			? (frame_length - imgsensor_info.slim_video.framelength)
			: 0;
		imgsensor.frame_length = imgsensor_info.slim_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		/*set_dummy();*/
		break;
	default:  /*coding with  preview scenario by default*/
		frame_length = imgsensor_info.pre.pclk / framerate * 10
			/ imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
			? (frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		/*set_dummy();*/
		LOG_INF("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(
	MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
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
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		/* 0x5E00[8]: 1 enable,  0 disable*/
		/* 0x5E00[1:0]; 00 Color bar,
		 * 01 Random Data, 10 Square, 11 BLACK
		 */
		write_cmos_sensor(0x0600, 0x0002);
	} else {
		/* 0x5E00[8]: 1 enable,  0 disable*/
		/* 0x5E00[1:0]; 00 Color bar, 01 Random Data,
		 * 10 Square, 11 BLACK
		 */
		write_cmos_sensor(0x0600, 0x0000);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}
static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				  UINT8 *feature_para,
				  UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	SET_PD_BLOCK_INFO_T *PDAFinfo;
	SENSOR_WINSIZE_INFO_STRUCT *wininfo;


	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = imgsensor.line_length;
		*feature_return_para_16 = imgsensor.frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_return_para_32 = imgsensor.pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(*feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		/*night_mode((BOOL) *feature_data);*/
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor(sensor_reg_data->RegAddr,
				  sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM or
		 * just return LENS_DRIVER_ID_DO_NOT_CARE
		 * if EEPROM does not exist in camera module.
		 */
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(*feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode((BOOL) (*feature_data_16),
				      *(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(
			(MSDK_SCENARIO_ID_ENUM) *feature_data,
			*(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(MSDK_SCENARIO_ID_ENUM) *(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data + 1)));
		break;
	/* case SENSOR_FEATURE_GET_PDAF_DATA:
	 * LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
	 * read_2L9_eeprom((kal_uint16 )(*feature_data),
	 * (char*)(uintptr_t)(*(feature_data + 1)),
	 * (kal_uint32)(*(feature_data + 2)));
	 * break;
	 */

	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL) (*feature_data));
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		/*for factory mode auto testing*/
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			*feature_data_32);
		wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)
			(*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[1],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[2],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[3],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[4],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,
			       (void *)&imgsensor_winsize_info[0],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld\n",
			*feature_data);
		PDAFinfo = (SET_PD_BLOCK_INFO_T *)(uintptr_t)
			(*(feature_data + 1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			/* memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,
			 * sizeof(SET_PD_BLOCK_INFO_T));
			 */
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF(
		"SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n",
		*feature_data);
		/*PDAF capacity enable or not, 2p8 only full size support PDAF*/
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			/* video & capture use same setting*/
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
			break;
		}
		break;


	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) *feature_data,
					 (UINT16) *(feature_data + 1));
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
		kal_uint32 rate;

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			rate = imgsensor_info.cap.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			rate = imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			rate = imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			rate = imgsensor_info.slim_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			rate = imgsensor_info.pre.mipi_pixel_rate;
			break;
		default:
			rate = 0;
			break;
		}
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
	}
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


UINT32 S5K2XASP_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	sensor_func.arch = IMGSENSOR_ARCH_V2;
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	if (imgsensor.psensor_func == NULL)
		imgsensor.psensor_func = &sensor_func;
	return ERROR_NONE;
}	/*	OV5693_MIPI_RAW_SensorInit	*/
