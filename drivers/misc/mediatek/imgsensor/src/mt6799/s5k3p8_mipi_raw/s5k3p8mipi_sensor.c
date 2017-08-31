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

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>

#include "kd_camera_typedef.h"
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k3p8mipi_sensor.h"


 #define LOG_INF LOG_INF_LOD


/****************************Modify Following Strings for Debug****************************/
#define PFX "s5k3p8sx_camera_primax"
#define LOG_INF_LOD(format, args...)    pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_1 LOG_INF("S5K3P8SX,MIPI 4LANE\n")
#define SENSORDB LOG_INF
/****************************   Modify end    *******************************************/

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K3P8SP_SENSOR_ID,
	.checksum_value =  0xB1F1B3CC,		/* checksum value for Camera Auto Test */
	.pre = {
		.pclk = 560000000,				/* record different mode's pclk */
		.linelength  = 5120,				/* record different mode's linelength */
		.framelength = 3643,			/* record different mode's framelength */
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 2320,		/* record different mode's width of grabwindow */
		.grabwindow_height = 1744,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 560000000,				/* record different mode's pclk */
		.linelength  = 5120,				/* record different mode's linelength */
		.framelength = 3643,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 4640,		/* record different mode's width of grabwindow */
		.grabwindow_height = 3488,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 560000000,				/* record different mode's pclk */
		.linelength  = 6400,				/* record different mode's linelength */
		.framelength = 3643,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 4640,		/* record different mode's width of grabwindow */
		.grabwindow_height = 3488,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 240,
	},
	.cap2 = {
		.pclk = 560000000,				/* record different mode's pclk */
		.linelength  = 10240,				/* record different mode's linelength */
		.framelength = 3643,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 4640,		/* record different mode's width of grabwindow */
		.grabwindow_height = 3488,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 150,
	},
	.normal_video = {
		.pclk = 560000000,				/* record different mode's pclk */
		.linelength  = 5120,				/* record different mode's linelength */
		.framelength = 3643,			/* record different mode's framelength */
		.startx = 0,
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 4640,		/* record different mode's width of grabwindow */
		.grabwindow_height = 3488,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 560000000,				/* record different mode's pclk */
		.linelength  = 4832,				/* record different mode's linelength */
		.framelength = 965,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 1280,		/* record different mode's width of grabwindow */
		.grabwindow_height = 720,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 560000000,				/* record different mode's pclk */
		.linelength  = 5120,				/* record different mode's linelength */
		.framelength = 3644,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 1920,		/* record different mode's width of grabwindow */
		.grabwindow_height = 1080,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.margin = 4,			/* sensor framelength & shutter margin */
	.min_shutter = 4,		/* min shutter */
	.max_frame_length = 0xFFFE,/* REG0x0202 <=REG0x0340-5//max framelength by sensor register's limitation */
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,/* isp gain delay frame for AE cycle */
	.ihdr_support = 0,	  /* 1, support; 0,not support */
	.ihdr_le_firstline = 0,  /* 1,le first ; 0, se first */
	.sensor_mode_num = 5,	  /* support sensor mode num ,don't support Slow motion */
	.cap_delay_frame = 3,		/* enter capture delay frame num */
	.pre_delay_frame = 3,		/* enter preview delay frame num */
	.video_delay_frame = 3,		/* enter video delay frame num */
	.hs_video_delay_frame = 3,	/* enter high speed video  delay frame num */
	.slim_video_delay_frame = 3,/* enter slim video delay frame num */
	.isp_driving_current = ISP_DRIVING_8MA, /* mclk driving current */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,/* sensor_interface_type */
	.mipi_sensor_type = MIPI_OPHY_NCSI2, /* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,/* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,/* sensor output first pixel color */
	.mclk = 24,/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_4_LANE,/* mipi lane num */
	.i2c_addr_table = {0x5A, 0x20, 0xff},/*must end with 0xff */
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_HV_MIRROR,				/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,					/* current shutter */
	.gain = 0x100,						/* current gain */
	.dummy_pixel = 0,					/* current dummypixel */
	.dummy_line = 0,					/* current dummyline */
	.current_fps = 0,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,/* current scenario id */
	.ihdr_en = KAL_FALSE, /* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x5A,/* record current sensor's i2c write id */
};

static int chip_id;
static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {/* Preview mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x0910, 0x06D0, 0x01, 0x00, 0x0000, 0x0000,
	0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x1220, 0x0DA0, 0x01, 0x00, 0x0000, 0x0000,
	0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x1220, 0x0DA0, 0x01, 0x00, 0x0000, 0x0000,
	0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000}
};

static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
	{ 4640, 3488,	  0,	0, 4640, 3488, 2320,  1744, 0000, 0000, 2320, 1744,	    0,	0, 2320, 1744},
	{ 4640, 3488,	  0,	0, 4640, 3488, 4640,  3488, 0000, 0000, 4640, 3488,	    0,	0, 4640, 3488},
	{ 4640, 3488,	  0,	0, 4640, 3488, 4640,  3488, 0000, 0000, 4640, 3488,	    0,	0, 4640, 3488},
	{ 4640, 3488,  400,  664, 3840, 2160, 1280,   720, 0000, 0000, 1280,  720,     0,  0, 1280,  720},
	{ 4640, 3488,  400,  664, 3840, 2160, 1920,  1080, 0000, 0000, 1920, 1080,     0,  0, 1920, 1080},
};

static SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 16,
	.i4OffsetY = 16,
	.i4PitchX  = 64,
	.i4PitchY  = 64,
	.i4PairNum = 16,
	.i4SubBlkW = 16,
	.i4SubBlkH = 16,
	.i4PosL = {{20, 23}, {72, 23}, {36, 27}, {56, 27}, {24, 43}, {68, 43}, {40, 47}, {52, 47}, {40, 55},
				{52, 55}, {24, 59}, {68, 59}, {36, 75}, {56, 75}, {20, 79}, {72, 79} },
	.i4PosR = {{20, 27}, {72, 27}, {36, 31}, {56, 31}, {24, 39}, {68, 39}, {40, 43}, {52, 43}, {40, 59},
				{52, 59}, {24, 63}, {68, 63}, {36, 71}, {56, 71}, {20, 75}, {72, 75} },
	.iMirrorFlip = 0,
	.i4BlockNumX = 72,
	.i4BlockNumY = 54,
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *)&get_byte, 2, imgsensor.i2c_write_id);
	return ((get_byte << 8) & 0xff00) | ((get_byte>>8) & 0x00ff);
}


static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)};

	iWriteRegI2C(pusendcmd, 4, imgsensor.i2c_write_id);
}

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	write_cmos_sensor(0x0342, imgsensor.line_length & 0xFFFF);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{

	kal_uint32 frame_length = imgsensor.frame_length;
	/* unsigned long flags; */

	LOG_INF("framerate = %d, min framelength should enable(%d)\n", framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length)
		? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	/* dummy_line = frame_length - imgsensor.min_frame_length; */
	/* if (dummy_line < 0) */
		/* imgsensor.dummy_line = 0; */
	/* else */
		/* imgsensor.dummy_line = dummy_line; */
	/* imgsensor.frame_length = frame_length + imgsensor.dummy_line; */
	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
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
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length);
		}
	} else {
	/* Extend frame length */
	write_cmos_sensor(0x0340, imgsensor.frame_length);
	LOG_INF("(else)imgsensor.frame_length = %d\n", imgsensor.frame_length);
	}

	/* Update Shutter */
	write_cmos_sensor(0x0202, shutter);
	LOG_INF("shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

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



static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;
	/* gain = 64 = 1x real gain */
	reg_gain = gain / 2;
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

	LOG_INF("set_gain %d\n", gain);
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

static void set_mirror_flip(kal_uint8 image_mirror)
{

	kal_uint8 itemp;

	LOG_INF("image_mirror = %d\n", image_mirror);
	itemp = read_cmos_sensor_8(0x0101);
	itemp &= ~0x03;

	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor_8(0x0101, itemp);
		break;

	case IMAGE_V_MIRROR:
		write_cmos_sensor_8(0x0101, itemp | 0x02);
		break;

	case IMAGE_H_MIRROR:
		write_cmos_sensor_8(0x0101, itemp | 0x01);
		break;

	case IMAGE_HV_MIRROR:
		write_cmos_sensor_8(0x0101, itemp | 0x03);
		break;
	}
}
static void check_stremoff(kal_uint16 fps)
{
	unsigned int i = 0, framecnt = 0;
	int timeout = (10000/fps)+1;

	for (i = 0; i < timeout; i++) {
		framecnt = read_cmos_sensor_8(0x0005);
		if (framecnt != 0xFF)
			mdelay(1);
		else
			return;
	}
	LOG_INF(" Stream Off Fail1!\n");
}

static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}	/*	night_mode	*/
static void sensor_init(void)
{
	LOG_INF("sensor_init() E\n");
	chip_id = read_cmos_sensor_8(0x0002);
	LOG_INF("chip id:%d E\n", chip_id);

	if (chip_id == 0xA0) {
	LOG_INF("EVT0 E\n");
	write_cmos_sensor(0x6028, 0x0001);
	write_cmos_sensor(0x6010, 0x0001);
	mdelay(3);         /* Wait value must be at least 20000 MCLKs */
	write_cmos_sensor(0x6214, 0x7971);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x303C);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0449);
	write_cmos_sensor(0x6F12, 0x0348);
	write_cmos_sensor(0x6F12, 0x0860);
	write_cmos_sensor(0x6F12, 0xCA8D);
	write_cmos_sensor(0x6F12, 0x101A);
	write_cmos_sensor(0x6F12, 0x8880);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x6CBE);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3FAC);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1E80);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0xFF4C);
	write_cmos_sensor(0x6F12, 0x0E46);
	write_cmos_sensor(0x6F12, 0xA4F8);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0xFE4D);
	write_cmos_sensor(0x6F12, 0x15F8);
	write_cmos_sensor(0x6F12, 0x940F);
	write_cmos_sensor(0x6F12, 0xA4F8);
	write_cmos_sensor(0x6F12, 0x0601);
	write_cmos_sensor(0x6F12, 0x6878);
	write_cmos_sensor(0x6F12, 0xA4F8);
	write_cmos_sensor(0x6F12, 0x0801);
	write_cmos_sensor(0x6F12, 0x1046);
	write_cmos_sensor(0x6F12, 0x04F5);
	write_cmos_sensor(0x6F12, 0xE874);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x12FF);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x14FF);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0x9A01);
	write_cmos_sensor(0x6F12, 0x6086);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0x9E01);
	write_cmos_sensor(0x6F12, 0xE086);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0xA001);
	write_cmos_sensor(0x6F12, 0x2087);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0xA601);
	write_cmos_sensor(0x6F12, 0xE087);
	write_cmos_sensor(0x6F12, 0x70BD);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF05F);
	write_cmos_sensor(0x6F12, 0xEF4A);
	write_cmos_sensor(0x6F12, 0xF049);
	write_cmos_sensor(0x6F12, 0xED4C);
	write_cmos_sensor(0x6F12, 0x0646);
	write_cmos_sensor(0x6F12, 0x1378);
	write_cmos_sensor(0x6F12, 0x0F89);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0xBE00);
	write_cmos_sensor(0x6F12, 0x4FF4);
	write_cmos_sensor(0x6F12, 0x8079);
	write_cmos_sensor(0x6F12, 0x13B1);
	write_cmos_sensor(0x6F12, 0xA0B9);
	write_cmos_sensor(0x6F12, 0x4F46);
	write_cmos_sensor(0x6F12, 0x12E0);
	write_cmos_sensor(0x6F12, 0xEA4B);
	write_cmos_sensor(0x6F12, 0x0968);
	write_cmos_sensor(0x6F12, 0x5278);
	write_cmos_sensor(0x6F12, 0x1B6D);
	write_cmos_sensor(0x6F12, 0xB1FB);
	write_cmos_sensor(0x6F12, 0xF3F1);
	write_cmos_sensor(0x6F12, 0x7943);
	write_cmos_sensor(0x6F12, 0xD140);
	write_cmos_sensor(0x6F12, 0x8BB2);
	write_cmos_sensor(0x6F12, 0x30B1);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x4FF4);
	write_cmos_sensor(0x6F12, 0x8051);
	write_cmos_sensor(0x6F12, 0x1846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xECFE);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x4846);
	write_cmos_sensor(0x6F12, 0x87B2);
	write_cmos_sensor(0x6F12, 0x3088);
	write_cmos_sensor(0x6F12, 0x18B1);
	write_cmos_sensor(0x6F12, 0x0121);
	write_cmos_sensor(0x6F12, 0xE048);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xE8FE);
	write_cmos_sensor(0x6F12, 0x0021);
	write_cmos_sensor(0x6F12, 0xDF48);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xE4FE);
	write_cmos_sensor(0x6F12, 0xDFF8);
	write_cmos_sensor(0x6F12, 0x7CA3);
	write_cmos_sensor(0x6F12, 0x0025);
	write_cmos_sensor(0x6F12, 0xA046);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x010B);
	write_cmos_sensor(0x6F12, 0x3088);
	write_cmos_sensor(0x6F12, 0x70B1);
	write_cmos_sensor(0x6F12, 0x7088);
	write_cmos_sensor(0x6F12, 0x38B1);
	write_cmos_sensor(0x6F12, 0xF089);
	write_cmos_sensor(0x6F12, 0x06E0);
	write_cmos_sensor(0x6F12, 0x2A46);
	write_cmos_sensor(0x6F12, 0xD649);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xD9FE);
	write_cmos_sensor(0x6F12, 0x0446);
	write_cmos_sensor(0x6F12, 0x05E0);
	write_cmos_sensor(0x6F12, 0xB089);
	write_cmos_sensor(0x6F12, 0x0028);
	write_cmos_sensor(0x6F12, 0xF6D1);
	write_cmos_sensor(0x6F12, 0x3846);
	write_cmos_sensor(0x6F12, 0xF4E7);
	write_cmos_sensor(0x6F12, 0x4C46);
	write_cmos_sensor(0x6F12, 0x98F8);
	write_cmos_sensor(0x6F12, 0xBE00);
	write_cmos_sensor(0x6F12, 0x28B1);
	write_cmos_sensor(0x6F12, 0x2A46);
	write_cmos_sensor(0x6F12, 0xD049);
	write_cmos_sensor(0x6F12, 0x3846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xC9FE);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x4846);
	write_cmos_sensor(0x6F12, 0xAAF8);
	write_cmos_sensor(0x6F12, 0x5802);
	write_cmos_sensor(0x6F12, 0x4443);
	write_cmos_sensor(0x6F12, 0x98F8);
	write_cmos_sensor(0x6F12, 0x2B02);
	write_cmos_sensor(0x6F12, 0x98F8);
	write_cmos_sensor(0x6F12, 0x2922);
	write_cmos_sensor(0x6F12, 0xC440);
	write_cmos_sensor(0x6F12, 0x0244);
	write_cmos_sensor(0x6F12, 0x0BFA);
	write_cmos_sensor(0x6F12, 0x02F0);
	write_cmos_sensor(0x6F12, 0x421E);
	write_cmos_sensor(0x6F12, 0xA242);
	write_cmos_sensor(0x6F12, 0x01DA);
	write_cmos_sensor(0x6F12, 0x1346);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x2346);
	write_cmos_sensor(0x6F12, 0x002B);
	write_cmos_sensor(0x6F12, 0x01DA);
	write_cmos_sensor(0x6F12, 0x0024);
	write_cmos_sensor(0x6F12, 0x02E0);
	write_cmos_sensor(0x6F12, 0xA242);
	write_cmos_sensor(0x6F12, 0x00DA);
	write_cmos_sensor(0x6F12, 0x1446);
	write_cmos_sensor(0x6F12, 0x0AEB);
	write_cmos_sensor(0x6F12, 0x4500);
	write_cmos_sensor(0x6F12, 0x6D1C);
	write_cmos_sensor(0x6F12, 0xA0F8);
	write_cmos_sensor(0x6F12, 0x5A42);
	write_cmos_sensor(0x6F12, 0x042D);
	write_cmos_sensor(0x6F12, 0xC4DB);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF09F);
	write_cmos_sensor(0x6F12, 0xBA4A);
	write_cmos_sensor(0x6F12, 0x92F8);
	write_cmos_sensor(0x6F12, 0xB330);
	write_cmos_sensor(0x6F12, 0x53B1);
	write_cmos_sensor(0x6F12, 0x92F8);
	write_cmos_sensor(0x6F12, 0xFD20);
	write_cmos_sensor(0x6F12, 0x0420);
	write_cmos_sensor(0x6F12, 0x52B1);
	write_cmos_sensor(0x6F12, 0xBA4A);
	write_cmos_sensor(0x6F12, 0x92F8);
	write_cmos_sensor(0x6F12, 0x3221);
	write_cmos_sensor(0x6F12, 0x012A);
	write_cmos_sensor(0x6F12, 0x05D1);
	write_cmos_sensor(0x6F12, 0x0220);
	write_cmos_sensor(0x6F12, 0x03E0);
	write_cmos_sensor(0x6F12, 0x5379);
	write_cmos_sensor(0x6F12, 0x2BB9);
	write_cmos_sensor(0x6F12, 0x20B1);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x8842);
	write_cmos_sensor(0x6F12, 0x00D8);
	write_cmos_sensor(0x6F12, 0x0846);
	write_cmos_sensor(0x6F12, 0x7047);
	write_cmos_sensor(0x6F12, 0x1078);
	write_cmos_sensor(0x6F12, 0xF9E7);
	write_cmos_sensor(0x6F12, 0xFEB5);
	write_cmos_sensor(0x6F12, 0x0646);
	write_cmos_sensor(0x6F12, 0xB248);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xC068);
	write_cmos_sensor(0x6F12, 0x84B2);
	write_cmos_sensor(0x6F12, 0x050C);
	write_cmos_sensor(0x6F12, 0x2146);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x87FE);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x89FE);
	write_cmos_sensor(0x6F12, 0xA74E);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x8DF8);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0xFD00);
	write_cmos_sensor(0x6F12, 0x8DF8);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0xB310);
	write_cmos_sensor(0x6F12, 0xA64F);
	write_cmos_sensor(0x6F12, 0x19B1);
	write_cmos_sensor(0x6F12, 0x97F8);
	write_cmos_sensor(0x6F12, 0x3221);
	write_cmos_sensor(0x6F12, 0x012A);
	write_cmos_sensor(0x6F12, 0x0FD0);
	write_cmos_sensor(0x6F12, 0x0023);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x0292);
	write_cmos_sensor(0x6F12, 0x0192);
	write_cmos_sensor(0x6F12, 0x21B1);
	write_cmos_sensor(0x6F12, 0x18B1);
	write_cmos_sensor(0x6F12, 0x97F8);
	write_cmos_sensor(0x6F12, 0x3201);
	write_cmos_sensor(0x6F12, 0x0128);
	write_cmos_sensor(0x6F12, 0x07D0);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0x6801);
	write_cmos_sensor(0x6F12, 0x4208);
	write_cmos_sensor(0x6F12, 0x33B1);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x06E0);
	write_cmos_sensor(0x6F12, 0x0123);
	write_cmos_sensor(0x6F12, 0xEEE7);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0x6821);
	write_cmos_sensor(0x6F12, 0xF7E7);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0xA400);
	write_cmos_sensor(0x6F12, 0x4100);
	write_cmos_sensor(0x6F12, 0x002A);
	write_cmos_sensor(0x6F12, 0x02D0);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x0200);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x41EA);
	write_cmos_sensor(0x6F12, 0x4020);
	write_cmos_sensor(0x6F12, 0x9449);
	write_cmos_sensor(0x6F12, 0xA1F8);
	write_cmos_sensor(0x6F12, 0x5201);
	write_cmos_sensor(0x6F12, 0x07D0);
	write_cmos_sensor(0x6F12, 0x9348);
	write_cmos_sensor(0x6F12, 0x6B46);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x4810);
	write_cmos_sensor(0x6F12, 0x4FF2);
	write_cmos_sensor(0x6F12, 0x5010);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x51FE);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x2146);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x42FE);
	write_cmos_sensor(0x6F12, 0xFEBD);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF047);
	write_cmos_sensor(0x6F12, 0x0546);
	write_cmos_sensor(0x6F12, 0x8948);
	write_cmos_sensor(0x6F12, 0x9046);
	write_cmos_sensor(0x6F12, 0x8946);
	write_cmos_sensor(0x6F12, 0x8068);
	write_cmos_sensor(0x6F12, 0x1C46);
	write_cmos_sensor(0x6F12, 0x86B2);
	write_cmos_sensor(0x6F12, 0x070C);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x3146);
	write_cmos_sensor(0x6F12, 0x3846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x32FE);
	write_cmos_sensor(0x6F12, 0x2346);
	write_cmos_sensor(0x6F12, 0x4246);
	write_cmos_sensor(0x6F12, 0x4946);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x36FE);
	write_cmos_sensor(0x6F12, 0x7B48);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xB310);
	write_cmos_sensor(0x6F12, 0xD1B1);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xFD00);
	write_cmos_sensor(0x6F12, 0xB8B1);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0xA5F5);
	write_cmos_sensor(0x6F12, 0x7141);
	write_cmos_sensor(0x6F12, 0x05F1);
	write_cmos_sensor(0x6F12, 0x8044);
	write_cmos_sensor(0x6F12, 0x9039);
	write_cmos_sensor(0x6F12, 0x02D0);
	write_cmos_sensor(0x6F12, 0x4031);
	write_cmos_sensor(0x6F12, 0x02D0);
	write_cmos_sensor(0x6F12, 0x0DE0);
	write_cmos_sensor(0x6F12, 0xA081);
	write_cmos_sensor(0x6F12, 0x0BE0);
	write_cmos_sensor(0x6F12, 0xA081);
	write_cmos_sensor(0x6F12, 0x7548);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0x3201);
	write_cmos_sensor(0x6F12, 0x0128);
	write_cmos_sensor(0x6F12, 0x05D1);
	write_cmos_sensor(0x6F12, 0x0220);
	write_cmos_sensor(0x6F12, 0xA080);
	write_cmos_sensor(0x6F12, 0x2D20);
	write_cmos_sensor(0x6F12, 0x2081);
	write_cmos_sensor(0x6F12, 0x2E20);
	write_cmos_sensor(0x6F12, 0x6081);
	write_cmos_sensor(0x6F12, 0x3146);
	write_cmos_sensor(0x6F12, 0x3846);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF047);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x06BE);
	write_cmos_sensor(0x6F12, 0xF0B5);
	write_cmos_sensor(0x6F12, 0x30F8);
	write_cmos_sensor(0x6F12, 0x4A3F);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0xC488);
	write_cmos_sensor(0x6F12, 0x6746);
	write_cmos_sensor(0x6F12, 0x1B1B);
	write_cmos_sensor(0x6F12, 0x0489);
	write_cmos_sensor(0x6F12, 0x30F8);
	write_cmos_sensor(0x6F12, 0x0A0C);
	write_cmos_sensor(0x6F12, 0x1B1B);
	write_cmos_sensor(0x6F12, 0x9DB2);
	write_cmos_sensor(0x6F12, 0x32F8);
	write_cmos_sensor(0x6F12, 0x5A3F);
	write_cmos_sensor(0x6F12, 0x8C78);
	write_cmos_sensor(0x6F12, 0x117C);
	write_cmos_sensor(0x6F12, 0x92F8);
	write_cmos_sensor(0x6F12, 0x58E0);
	write_cmos_sensor(0x6F12, 0x04FB);
	write_cmos_sensor(0x6F12, 0x1133);
	write_cmos_sensor(0x6F12, 0x9BB2);
	write_cmos_sensor(0x6F12, 0x9489);
	write_cmos_sensor(0x6F12, 0xA0EB);
	write_cmos_sensor(0x6F12, 0x0E00);
	write_cmos_sensor(0x6F12, 0x04FB);
	write_cmos_sensor(0x6F12, 0x0134);
	write_cmos_sensor(0x6F12, 0x12F8);
	write_cmos_sensor(0x6F12, 0x3F6C);
	write_cmos_sensor(0x6F12, 0x80B2);
	write_cmos_sensor(0x6F12, 0x00FB);
	write_cmos_sensor(0x6F12, 0x013E);
	write_cmos_sensor(0x6F12, 0xA41B);
	write_cmos_sensor(0x6F12, 0xD27C);
	write_cmos_sensor(0x6F12, 0xA4B2);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x8EFE);
	write_cmos_sensor(0x6F12, 0x22B1);
	write_cmos_sensor(0x6F12, 0x3444);
	write_cmos_sensor(0x6F12, 0x00FB);
	write_cmos_sensor(0x6F12, 0x1144);
	write_cmos_sensor(0x6F12, 0x641E);
	write_cmos_sensor(0x6F12, 0x07E0);
	write_cmos_sensor(0x6F12, 0x20B1);
	write_cmos_sensor(0x6F12, 0xAEF1);
	write_cmos_sensor(0x6F12, 0x0104);
	write_cmos_sensor(0x6F12, 0x05FB);
	write_cmos_sensor(0x6F12, 0x0144);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0x641E);
	write_cmos_sensor(0x6F12, 0x3444);
	write_cmos_sensor(0x6F12, 0xA4B2);
	write_cmos_sensor(0x6F12, 0x2AB1);
	write_cmos_sensor(0x6F12, 0x10B1);
	write_cmos_sensor(0x6F12, 0x05FB);
	write_cmos_sensor(0x6F12, 0x1143);
	write_cmos_sensor(0x6F12, 0x5B1C);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x83FE);
	write_cmos_sensor(0x6F12, 0xBEF1);
	write_cmos_sensor(0x6F12, 0x680F);
	write_cmos_sensor(0x6F12, 0x02D2);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x010E);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0xAEF1);
	write_cmos_sensor(0x6F12, 0x670E);
	write_cmos_sensor(0x6F12, 0x4FF4);
	write_cmos_sensor(0x6F12, 0x5861);
	write_cmos_sensor(0x6F12, 0x673C);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x8EF0);
	write_cmos_sensor(0x6F12, 0x8C42);
	write_cmos_sensor(0x6F12, 0x00DD);
	write_cmos_sensor(0x6F12, 0x0C46);
	write_cmos_sensor(0x6F12, 0x211A);
	write_cmos_sensor(0x6F12, 0x491C);
	write_cmos_sensor(0x6F12, 0x8BB2);
	write_cmos_sensor(0x6F12, 0x401E);
	write_cmos_sensor(0x6F12, 0xC117);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x9161);
	write_cmos_sensor(0x6F12, 0x21F0);
	write_cmos_sensor(0x6F12, 0x3F01);
	write_cmos_sensor(0x6F12, 0x401A);
	write_cmos_sensor(0x6F12, 0xC117);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x1171);
	write_cmos_sensor(0x6F12, 0x0911);
	write_cmos_sensor(0x6F12, 0xA0EB);
	write_cmos_sensor(0x6F12, 0x0110);
	write_cmos_sensor(0x6F12, 0xC0F1);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x6F12, 0x81B2);
	write_cmos_sensor(0x6F12, 0x04F0);
	write_cmos_sensor(0x6F12, 0x0F00);
	write_cmos_sensor(0x6F12, 0xCC06);
	write_cmos_sensor(0x6F12, 0x00D5);
	write_cmos_sensor(0x6F12, 0x0021);
	write_cmos_sensor(0x6F12, 0xC406);
	write_cmos_sensor(0x6F12, 0x00D5);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x0C18);
	write_cmos_sensor(0x6F12, 0x1B1B);
	write_cmos_sensor(0x6F12, 0x9BB2);
	write_cmos_sensor(0x6F12, 0x89B1);
	write_cmos_sensor(0x6F12, 0xB2B1);
	write_cmos_sensor(0x6F12, 0x0829);
	write_cmos_sensor(0x6F12, 0x02D9);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x040C);
	write_cmos_sensor(0x6F12, 0x0BE0);
	write_cmos_sensor(0x6F12, 0x0529);
	write_cmos_sensor(0x6F12, 0x02D9);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x030C);
	write_cmos_sensor(0x6F12, 0x06E0);
	write_cmos_sensor(0x6F12, 0x0429);
	write_cmos_sensor(0x6F12, 0x02D9);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x020C);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x010C);
	write_cmos_sensor(0x6F12, 0xB0B1);
	write_cmos_sensor(0x6F12, 0x72B1);
	write_cmos_sensor(0x6F12, 0x0828);
	write_cmos_sensor(0x6F12, 0x06D2);
	write_cmos_sensor(0x6F12, 0x0027);
	write_cmos_sensor(0x6F12, 0x11E0);
	write_cmos_sensor(0x6F12, 0x0829);
	write_cmos_sensor(0x6F12, 0xE8D8);
	write_cmos_sensor(0x6F12, 0x0429);
	write_cmos_sensor(0x6F12, 0xF3D9);
	write_cmos_sensor(0x6F12, 0xEAE7);
	write_cmos_sensor(0x6F12, 0x0C28);
	write_cmos_sensor(0x6F12, 0x01D2);
	write_cmos_sensor(0x6F12, 0x0127);
	write_cmos_sensor(0x6F12, 0x08E0);
	write_cmos_sensor(0x6F12, 0x0327);
	write_cmos_sensor(0x6F12, 0x06E0);
	write_cmos_sensor(0x6F12, 0x0828);
	write_cmos_sensor(0x6F12, 0xF0D3);
	write_cmos_sensor(0x6F12, 0x0C28);
	write_cmos_sensor(0x6F12, 0xF7D3);
	write_cmos_sensor(0x6F12, 0x0D28);
	write_cmos_sensor(0x6F12, 0xF7D2);
	write_cmos_sensor(0x6F12, 0x0227);
	write_cmos_sensor(0x6F12, 0x9809);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0xC3F3);
	write_cmos_sensor(0x6F12, 0x0111);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x8100);
	write_cmos_sensor(0x6F12, 0x6044);
	write_cmos_sensor(0x6F12, 0x3844);
	write_cmos_sensor(0x6F12, 0x80B2);
	write_cmos_sensor(0x6F12, 0xF0BD);
	write_cmos_sensor(0x6F12, 0x0346);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x0029);
	write_cmos_sensor(0x6F12, 0x0BD0);
	write_cmos_sensor(0x6F12, 0x1749);
	write_cmos_sensor(0x6F12, 0x0978);
	write_cmos_sensor(0x6F12, 0x0029);
	write_cmos_sensor(0x6F12, 0x07D0);
	write_cmos_sensor(0x6F12, 0x002A);
	write_cmos_sensor(0x6F12, 0x05D0);
	write_cmos_sensor(0x6F12, 0x2BB1);
	write_cmos_sensor(0x6F12, 0x02F0);
	write_cmos_sensor(0x6F12, 0x0300);
	write_cmos_sensor(0x6F12, 0x0128);
	write_cmos_sensor(0x6F12, 0x03D1);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x7047);
	write_cmos_sensor(0x6F12, 0x9007);
	write_cmos_sensor(0x6F12, 0xFCD0);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x6F12, 0x7047);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF05F);
	write_cmos_sensor(0x6F12, 0x0C46);
	write_cmos_sensor(0x6F12, 0x0546);
	write_cmos_sensor(0x6F12, 0x9089);
	write_cmos_sensor(0x6F12, 0x518A);
	write_cmos_sensor(0x6F12, 0x1646);
	write_cmos_sensor(0x6F12, 0x401A);
	write_cmos_sensor(0x6F12, 0x918A);
	write_cmos_sensor(0x6F12, 0x401A);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x80FA);
	write_cmos_sensor(0x6F12, 0x0848);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0x5A10);
	write_cmos_sensor(0x6F12, 0x8278);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6A00);
	write_cmos_sensor(0x6F12, 0x02FB);
	write_cmos_sensor(0x6F12, 0x1011);
	write_cmos_sensor(0x6F12, 0x89B2);
	write_cmos_sensor(0x6F12, 0x17E0);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0xE000);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1350);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x4A00);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2970);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1EB0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1BAA);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1B58);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2530);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1CC0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3F80);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0xF000);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x05C0);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0x6620);
	write_cmos_sensor(0x6F12, 0x02FB);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0xE97E);
	write_cmos_sensor(0x6F12, 0x401A);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x80F8);
	write_cmos_sensor(0x6F12, 0x7088);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0xB210);
	write_cmos_sensor(0x6F12, 0x401A);
	write_cmos_sensor(0x6F12, 0x87B2);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x2200);
	write_cmos_sensor(0x6F12, 0x30B1);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x21FD);
	write_cmos_sensor(0x6F12, 0x10B1);
	write_cmos_sensor(0x6F12, 0xFC48);
	write_cmos_sensor(0x6F12, 0x0088);
	write_cmos_sensor(0x6F12, 0x00B1);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x6F12, 0x2070);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6900);
	write_cmos_sensor(0x6F12, 0x0228);
	write_cmos_sensor(0x6F12, 0x03D1);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6B00);
	write_cmos_sensor(0x6F12, 0x0228);
	write_cmos_sensor(0x6F12, 0x14D0);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0x9E00);
	write_cmos_sensor(0x6F12, 0x0221);
	write_cmos_sensor(0x6F12, 0xB1EB);
	write_cmos_sensor(0x6F12, 0x101F);
	write_cmos_sensor(0x6F12, 0x03D1);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6B00);
	write_cmos_sensor(0x6F12, 0x0228);
	write_cmos_sensor(0x6F12, 0x0AD0);
	write_cmos_sensor(0x6F12, 0x0023);
	write_cmos_sensor(0x6F12, 0x6370);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6D00);
	write_cmos_sensor(0x6F12, 0x591C);
	write_cmos_sensor(0x6F12, 0x30B1);
	write_cmos_sensor(0x6F12, 0xEA7E);
	write_cmos_sensor(0x6F12, 0xC8F5);
	write_cmos_sensor(0x6F12, 0x6060);
	write_cmos_sensor(0x6F12, 0x801A);
	write_cmos_sensor(0x6F12, 0x02E0);
	write_cmos_sensor(0x6F12, 0x0123);
	write_cmos_sensor(0x6F12, 0xF3E7);
	write_cmos_sensor(0x6F12, 0x6888);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6B20);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x80F9);
	write_cmos_sensor(0x6F12, 0xB9FB);
	write_cmos_sensor(0x6F12, 0xF2F0);
	write_cmos_sensor(0x6F12, 0x3844);
	write_cmos_sensor(0x6F12, 0x80B2);
	write_cmos_sensor(0x6F12, 0x6080);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6B20);
	write_cmos_sensor(0x6F12, 0x40F6);
	write_cmos_sensor(0x6F12, 0x985E);
	write_cmos_sensor(0x6F12, 0xBEFB);
	write_cmos_sensor(0x6F12, 0xF2F8);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x0A0C);
	write_cmos_sensor(0x6F12, 0xE045);
	write_cmos_sensor(0x6F12, 0x06D9);
	write_cmos_sensor(0x6F12, 0x2FB1);
	write_cmos_sensor(0x6F12, 0x728A);
	write_cmos_sensor(0x6F12, 0x02EB);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x0CEB);
	write_cmos_sensor(0x6F12, 0x0A02);
	write_cmos_sensor(0x6F12, 0x04E0);
	write_cmos_sensor(0x6F12, 0xBEFB);
	write_cmos_sensor(0x6F12, 0xF2F2);
	write_cmos_sensor(0x6F12, 0xB6F8);
	write_cmos_sensor(0x6F12, 0x12C0);
	write_cmos_sensor(0x6F12, 0x6244);
	write_cmos_sensor(0x6F12, 0xA280);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6B20);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x180C);
	write_cmos_sensor(0x6F12, 0xBCFB);
	write_cmos_sensor(0x6F12, 0xF2F8);
	write_cmos_sensor(0x6F12, 0x8045);
	write_cmos_sensor(0x6F12, 0x14D2);
	write_cmos_sensor(0x6F12, 0x728A);
	write_cmos_sensor(0x6F12, 0x0AB1);
	write_cmos_sensor(0x6F12, 0x0246);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0xE280);
	write_cmos_sensor(0x6F12, 0xB6F8);
	write_cmos_sensor(0x6F12, 0x12C0);
	write_cmos_sensor(0x6F12, 0x92B2);
	write_cmos_sensor(0x6F12, 0x6244);
	write_cmos_sensor(0x6F12, 0x92B2);
	write_cmos_sensor(0x6F12, 0xE280);
	write_cmos_sensor(0x6F12, 0x9346);
	write_cmos_sensor(0x6F12, 0x7289);
	write_cmos_sensor(0x6F12, 0x02FB);
	write_cmos_sensor(0x6F12, 0x01FC);
	write_cmos_sensor(0x6F12, 0xBCF5);
	write_cmos_sensor(0x6F12, 0x905F);
	write_cmos_sensor(0x6F12, 0x04DD);
	write_cmos_sensor(0x6F12, 0x9022);
	write_cmos_sensor(0x6F12, 0x1AE0);
	write_cmos_sensor(0x6F12, 0xBCFB);
	write_cmos_sensor(0x6F12, 0xF2F2);
	write_cmos_sensor(0x6F12, 0xECE7);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x400C);
	write_cmos_sensor(0x6F12, 0xBCFB);
	write_cmos_sensor(0x6F12, 0xF1FC);
	write_cmos_sensor(0x6F12, 0xB2FB);
	write_cmos_sensor(0x6F12, 0xFCF8);
	write_cmos_sensor(0x6F12, 0x0CFB);
	write_cmos_sensor(0x6F12, 0x182C);
	write_cmos_sensor(0x6F12, 0xBCF1);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x400C);
	write_cmos_sensor(0x6F12, 0xBCFB);
	write_cmos_sensor(0x6F12, 0xF1FC);
	write_cmos_sensor(0x6F12, 0xB2FB);
	write_cmos_sensor(0x6F12, 0xFCF2);
	write_cmos_sensor(0x6F12, 0x02D0);
	write_cmos_sensor(0x6F12, 0x5200);
	write_cmos_sensor(0x6F12, 0x921C);
	write_cmos_sensor(0x6F12, 0x03E0);
	write_cmos_sensor(0x6F12, 0x4FF6);
	write_cmos_sensor(0x6F12, 0xFF7C);
	write_cmos_sensor(0x6F12, 0x0CEA);
	write_cmos_sensor(0x6F12, 0x4202);
	write_cmos_sensor(0x6F12, 0x2281);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x82FC);
	write_cmos_sensor(0x6F12, 0x0C22);
	write_cmos_sensor(0x6F12, 0xBCFB);
	write_cmos_sensor(0x6F12, 0xF2F8);
	write_cmos_sensor(0x6F12, 0xA4F8);
	write_cmos_sensor(0x6F12, 0x0A80);
	write_cmos_sensor(0x6F12, 0xBCFB);
	write_cmos_sensor(0x6F12, 0xF2F8);
	write_cmos_sensor(0x6F12, 0x02FB);
	write_cmos_sensor(0x6F12, 0x18C2);
	write_cmos_sensor(0x6F12, 0xA281);
	write_cmos_sensor(0x6F12, 0xB74A);
	write_cmos_sensor(0x6F12, 0x5288);
	write_cmos_sensor(0x6F12, 0xE281);
	write_cmos_sensor(0x6F12, 0xB6F8);
	write_cmos_sensor(0x6F12, 0x0AC0);
	write_cmos_sensor(0x6F12, 0x6FF0);
	write_cmos_sensor(0x6F12, 0x0A02);
	write_cmos_sensor(0x6F12, 0x02EB);
	write_cmos_sensor(0x6F12, 0x9C02);
	write_cmos_sensor(0x6F12, 0x2282);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6D80);
	write_cmos_sensor(0x6F12, 0xB8F1);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x6F12, 0x02D0);
	write_cmos_sensor(0x6F12, 0x09F1);
	write_cmos_sensor(0x6F12, 0x0702);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x4A46);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x82FC);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6B20);
	write_cmos_sensor(0x6F12, 0x4843);
	write_cmos_sensor(0x6F12, 0xBCFB);
	write_cmos_sensor(0x6F12, 0xF2FC);
	write_cmos_sensor(0x6F12, 0xBC44);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x8CFC);
	write_cmos_sensor(0x6F12, 0x1828);
	write_cmos_sensor(0x6F12, 0x3ADC);
	write_cmos_sensor(0x6F12, 0xB8F1);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x6F12, 0x03D0);
	write_cmos_sensor(0x6F12, 0x0720);
	write_cmos_sensor(0x6F12, 0xB0FB);
	write_cmos_sensor(0x6F12, 0xF1F0);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0xA449);
	write_cmos_sensor(0x6F12, 0x6082);
	write_cmos_sensor(0x6F12, 0x8888);
	write_cmos_sensor(0x6F12, 0xA082);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x9200);
	write_cmos_sensor(0x6F12, 0x3844);
	write_cmos_sensor(0x6F12, 0xE082);
	write_cmos_sensor(0x6F12, 0x3FB1);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6B20);
	write_cmos_sensor(0x6F12, 0x0AEB);
	write_cmos_sensor(0x6F12, 0x0700);
	write_cmos_sensor(0x6F12, 0xBEFB);
	write_cmos_sensor(0x6F12, 0xF2F2);
	write_cmos_sensor(0x6F12, 0x8242);
	write_cmos_sensor(0x6F12, 0x03D8);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6B20);
	write_cmos_sensor(0x6F12, 0xBEFB);
	write_cmos_sensor(0x6F12, 0xF2F0);
	write_cmos_sensor(0x6F12, 0x2083);
	write_cmos_sensor(0x6F12, 0x82B2);
	write_cmos_sensor(0x6F12, 0x708A);
	write_cmos_sensor(0x6F12, 0x08B1);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x9200);
	write_cmos_sensor(0x6F12, 0x1044);
	write_cmos_sensor(0x6F12, 0x2083);
	write_cmos_sensor(0x6F12, 0xC888);
	write_cmos_sensor(0x6F12, 0x6083);
	write_cmos_sensor(0x6F12, 0x3088);
	write_cmos_sensor(0x6F12, 0xA083);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0x9E20);
	write_cmos_sensor(0x6F12, 0x3088);
	write_cmos_sensor(0x6F12, 0x7189);
	write_cmos_sensor(0x6F12, 0x1209);
	write_cmos_sensor(0x6F12, 0x01FB);
	write_cmos_sensor(0x6F12, 0x0200);
	write_cmos_sensor(0x6F12, 0xE083);
	write_cmos_sensor(0x6F12, 0x708A);
	write_cmos_sensor(0x6F12, 0x40B3);
	write_cmos_sensor(0x6F12, 0x608A);
	write_cmos_sensor(0x6F12, 0x0828);
	write_cmos_sensor(0x6F12, 0x16D0);
	write_cmos_sensor(0x6F12, 0x0C28);
	write_cmos_sensor(0x6F12, 0x14D0);
	write_cmos_sensor(0x6F12, 0x0D28);
	write_cmos_sensor(0x6F12, 0x12D0);
	write_cmos_sensor(0x6F12, 0x12E0);
	write_cmos_sensor(0x6F12, 0x02FB);
	write_cmos_sensor(0x6F12, 0x0CF2);
	write_cmos_sensor(0x6F12, 0x183A);
	write_cmos_sensor(0x6F12, 0xD017);
	write_cmos_sensor(0x6F12, 0x02EB);
	write_cmos_sensor(0x6F12, 0x9060);
	write_cmos_sensor(0x6F12, 0x20F0);
	write_cmos_sensor(0x6F12, 0x3F00);
	write_cmos_sensor(0x6F12, 0x101A);
	write_cmos_sensor(0x6F12, 0xC217);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x1272);
	write_cmos_sensor(0x6F12, 0x1211);
	write_cmos_sensor(0x6F12, 0xA0EB);
	write_cmos_sensor(0x6F12, 0x0210);
	write_cmos_sensor(0x6F12, 0x90FB);
	write_cmos_sensor(0x6F12, 0xF1F0);
	write_cmos_sensor(0x6F12, 0xBAE7);
	write_cmos_sensor(0x6F12, 0x13B1);
	write_cmos_sensor(0x6F12, 0x0428);
	write_cmos_sensor(0x6F12, 0x05D0);
	write_cmos_sensor(0x6F12, 0x0AE0);
	write_cmos_sensor(0x6F12, 0x801C);
	write_cmos_sensor(0x6F12, 0x6082);
	write_cmos_sensor(0x6F12, 0x0BF1);
	write_cmos_sensor(0x6F12, 0x0200);
	write_cmos_sensor(0x6F12, 0x04E0);
	write_cmos_sensor(0x6F12, 0x23B1);
	write_cmos_sensor(0x6F12, 0x0520);
	write_cmos_sensor(0x6F12, 0x6082);
	write_cmos_sensor(0x6F12, 0x0BF1);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0xE080);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6D00);
	write_cmos_sensor(0x6F12, 0x3A46);
	write_cmos_sensor(0x6F12, 0x1946);
	write_cmos_sensor(0x6F12, 0xFFF7);
	write_cmos_sensor(0x6F12, 0x9EFE);
	write_cmos_sensor(0x6F12, 0x7849);
	write_cmos_sensor(0x6F12, 0x4880);
	write_cmos_sensor(0x6F12, 0x55E5);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0x0123);
	write_cmos_sensor(0x6F12, 0x11E0);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2350);
	write_cmos_sensor(0x6F12, 0x5A1E);
	write_cmos_sensor(0x6F12, 0x03E0);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x8206);
	write_cmos_sensor(0x6F12, 0x521E);
	write_cmos_sensor(0x6F12, 0x7460);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2240);
	write_cmos_sensor(0x6F12, 0xAC42);
	write_cmos_sensor(0x6F12, 0x01D9);
	write_cmos_sensor(0x6F12, 0x002A);
	write_cmos_sensor(0x6F12, 0xF5DA);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x8202);
	write_cmos_sensor(0x6F12, 0x5B1C);
	write_cmos_sensor(0x6F12, 0x5560);
	write_cmos_sensor(0x6F12, 0x8B42);
	write_cmos_sensor(0x6F12, 0xEBDB);
	write_cmos_sensor(0x6F12, 0x70BD);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0x0123);
	write_cmos_sensor(0x6F12, 0x11E0);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2350);
	write_cmos_sensor(0x6F12, 0x5A1E);
	write_cmos_sensor(0x6F12, 0x03E0);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x8206);
	write_cmos_sensor(0x6F12, 0x521E);
	write_cmos_sensor(0x6F12, 0x7460);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2240);
	write_cmos_sensor(0x6F12, 0xAC42);
	write_cmos_sensor(0x6F12, 0x01D2);
	write_cmos_sensor(0x6F12, 0x002A);
	write_cmos_sensor(0x6F12, 0xF5DA);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x8202);
	write_cmos_sensor(0x6F12, 0x5B1C);
	write_cmos_sensor(0x6F12, 0x5560);
	write_cmos_sensor(0x6F12, 0x8B42);
	write_cmos_sensor(0x6F12, 0xEBDB);
	write_cmos_sensor(0x6F12, 0x70BD);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF34F);
	write_cmos_sensor(0x6F12, 0x5E4F);
	write_cmos_sensor(0x6F12, 0x0646);
	write_cmos_sensor(0x6F12, 0x91B0);
	write_cmos_sensor(0x6F12, 0xB87F);
	write_cmos_sensor(0x6F12, 0x0028);
	write_cmos_sensor(0x6F12, 0x79D0);
	write_cmos_sensor(0x6F12, 0x307E);
	write_cmos_sensor(0x6F12, 0x717E);
	write_cmos_sensor(0x6F12, 0x129C);
	write_cmos_sensor(0x6F12, 0x0844);
	write_cmos_sensor(0x6F12, 0x0690);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0x6B10);
	write_cmos_sensor(0x6F12, 0xB07E);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xD5FB);
	write_cmos_sensor(0x6F12, 0x1090);
	write_cmos_sensor(0x6F12, 0xB07E);
	write_cmos_sensor(0x6F12, 0xF17E);
	write_cmos_sensor(0x6F12, 0x0844);
	write_cmos_sensor(0x6F12, 0x0590);
	write_cmos_sensor(0x6F12, 0x0090);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0x6D00);
	write_cmos_sensor(0x6F12, 0x08B1);
	write_cmos_sensor(0x6F12, 0x0125);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x0025);
	write_cmos_sensor(0x6F12, 0xB6F8);
	write_cmos_sensor(0x6F12, 0x5800);
	write_cmos_sensor(0x6F12, 0x0699);
	write_cmos_sensor(0x6F12, 0xC5F1);
	write_cmos_sensor(0x6F12, 0x0108);
	write_cmos_sensor(0x6F12, 0xB0FB);
	write_cmos_sensor(0x6F12, 0xF1F2);
	write_cmos_sensor(0x6F12, 0x01FB);
	write_cmos_sensor(0x6F12, 0x1201);
	write_cmos_sensor(0x6F12, 0x069A);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x6F12, 0xB0FB);
	write_cmos_sensor(0x6F12, 0xF2F0);
	write_cmos_sensor(0x6F12, 0xB6F8);
	write_cmos_sensor(0x6F12, 0x8E20);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x5200);
	write_cmos_sensor(0x6F12, 0x0990);
	write_cmos_sensor(0x6F12, 0x4848);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x8000);
	write_cmos_sensor(0x6F12, 0x401A);
	write_cmos_sensor(0x6F12, 0x0F90);
	write_cmos_sensor(0x6F12, 0x45B1);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0xFF30);
	write_cmos_sensor(0x6F12, 0x0A90);
	write_cmos_sensor(0x6F12, 0xD7E9);
	write_cmos_sensor(0x6F12, 0x0419);
	write_cmos_sensor(0x6F12, 0x091F);
	write_cmos_sensor(0x6F12, 0x0B91);
	write_cmos_sensor(0x6F12, 0x0146);
	write_cmos_sensor(0x6F12, 0x07E0);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x6F12, 0x0A90);
	write_cmos_sensor(0x6F12, 0xD7E9);
	write_cmos_sensor(0x6F12, 0x0490);
	write_cmos_sensor(0x6F12, 0x001D);
	write_cmos_sensor(0x6F12, 0x0B90);
	write_cmos_sensor(0x6F12, 0x0099);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0x6C20);
	write_cmos_sensor(0x6F12, 0x12B1);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0xC702);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0xFF02);
	write_cmos_sensor(0x6F12, 0x394B);
	write_cmos_sensor(0x6F12, 0x9A61);
	write_cmos_sensor(0x6F12, 0x364B);
	write_cmos_sensor(0x6F12, 0x9A68);
	write_cmos_sensor(0x6F12, 0x0392);
	write_cmos_sensor(0x6F12, 0xDB68);
	write_cmos_sensor(0x6F12, 0x0493);
	write_cmos_sensor(0x6F12, 0xCDE9);
	write_cmos_sensor(0x6F12, 0x0123);
	write_cmos_sensor(0x6F12, 0x7288);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0x6A30);
	write_cmos_sensor(0x6F12, 0x368E);
	write_cmos_sensor(0x6F12, 0x03FB);
	write_cmos_sensor(0x6F12, 0x0622);
	write_cmos_sensor(0x6F12, 0x0892);
	write_cmos_sensor(0x6F12, 0xDCE0);
	write_cmos_sensor(0x6F12, 0x0A9E);
	write_cmos_sensor(0x6F12, 0xD9F8);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x09EB);
	write_cmos_sensor(0x6F12, 0x8609);
	write_cmos_sensor(0x6F12, 0x089E);
	write_cmos_sensor(0x6F12, 0x130C);
	write_cmos_sensor(0x6F12, 0xB342);
	write_cmos_sensor(0x6F12, 0xF5D3);
	write_cmos_sensor(0x6F12, 0x9E1B);
	write_cmos_sensor(0x6F12, 0x0F9B);
	write_cmos_sensor(0x6F12, 0x92B2);
	write_cmos_sensor(0x6F12, 0x069F);
	write_cmos_sensor(0x6F12, 0x1344);
	write_cmos_sensor(0x6F12, 0xB3FB);
	write_cmos_sensor(0x6F12, 0xF7F3);
	write_cmos_sensor(0x6F12, 0x099F);
	write_cmos_sensor(0x6F12, 0xBB42);
	write_cmos_sensor(0x6F12, 0x76D3);
	write_cmos_sensor(0x6F12, 0xDB1B);
	write_cmos_sensor(0x6F12, 0x63F3);
	write_cmos_sensor(0x6F12, 0x5F02);
	write_cmos_sensor(0x6F12, 0x0E92);
	write_cmos_sensor(0x6F12, 0xB8F1);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x6F12, 0x01D0);
	write_cmos_sensor(0x6F12, 0x8E42);
	write_cmos_sensor(0x6F12, 0x02D2);
	write_cmos_sensor(0x6F12, 0x25B1);
	write_cmos_sensor(0x6F12, 0x8642);
	write_cmos_sensor(0x6F12, 0x02D2);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0x45E1);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x002A);
	write_cmos_sensor(0x6F12, 0x7DD0);
	write_cmos_sensor(0x6F12, 0x01A8);
	write_cmos_sensor(0x6F12, 0x4AEA);
	write_cmos_sensor(0x6F12, 0x0547);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2500);
	write_cmos_sensor(0x6F12, 0x0D90);
	write_cmos_sensor(0x6F12, 0x03A8);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x25B0);
	write_cmos_sensor(0x6F12, 0x0D98);
	write_cmos_sensor(0x6F12, 0xA0EB);
	write_cmos_sensor(0x6F12, 0x0B00);
	write_cmos_sensor(0x6F12, 0x8010);
	write_cmos_sensor(0x6F12, 0x0790);
	write_cmos_sensor(0x6F12, 0x1648);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xFC00);
	write_cmos_sensor(0x6F12, 0x08B1);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x02E0);
	write_cmos_sensor(0x6F12, 0x1348);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x2C01);
	write_cmos_sensor(0x6F12, 0x0C90);
	write_cmos_sensor(0x6F12, 0x1348);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x4E15);
	write_cmos_sensor(0x6F12, 0x0798);
	write_cmos_sensor(0x6F12, 0x8142);
	write_cmos_sensor(0x6F12, 0x03D2);
	write_cmos_sensor(0x6F12, 0x390C);
	write_cmos_sensor(0x6F12, 0x3620);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x44FB);
	write_cmos_sensor(0x6F12, 0x0C48);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xFD00);
	write_cmos_sensor(0x6F12, 0x20B9);
	write_cmos_sensor(0x6F12, 0x0A48);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x2801);
	write_cmos_sensor(0x6F12, 0x07EB);
	write_cmos_sensor(0x6F12, 0x0047);
	write_cmos_sensor(0x6F12, 0x0798);
	write_cmos_sensor(0x6F12, 0x0128);
	write_cmos_sensor(0x6F12, 0x13D0);
	write_cmos_sensor(0x6F12, 0x21D9);
	write_cmos_sensor(0x6F12, 0x074A);
	write_cmos_sensor(0x6F12, 0x0146);
	write_cmos_sensor(0x6F12, 0x5846);
	write_cmos_sensor(0x6F12, 0x9269);
	write_cmos_sensor(0x6F12, 0x9047);
	write_cmos_sensor(0x6F12, 0x5846);
	write_cmos_sensor(0x6F12, 0x17E0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x19E0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x4A00);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1C20);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1EB0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3F80);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1350);
	write_cmos_sensor(0x6F12, 0xDBF8);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0x0C98);
	write_cmos_sensor(0x6F12, 0x0844);
	write_cmos_sensor(0x6F12, 0x3843);
	write_cmos_sensor(0x6F12, 0x01C4);
	write_cmos_sensor(0x6F12, 0x07E0);
	write_cmos_sensor(0x6F12, 0x04C8);
	write_cmos_sensor(0x6F12, 0x0C99);
	write_cmos_sensor(0x6F12, 0x1144);
	write_cmos_sensor(0x6F12, 0x3943);
	write_cmos_sensor(0x6F12, 0x02C4);
	write_cmos_sensor(0x6F12, 0x0D99);
	write_cmos_sensor(0x6F12, 0x8842);
	write_cmos_sensor(0x6F12, 0xF7D1);
	write_cmos_sensor(0x6F12, 0x01A8);
	write_cmos_sensor(0x6F12, 0x4AEA);
	write_cmos_sensor(0x6F12, 0x0847);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2800);
	write_cmos_sensor(0x6F12, 0x0C90);
	write_cmos_sensor(0x6F12, 0x03A8);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x28B0);
	write_cmos_sensor(0x6F12, 0x0C98);
	write_cmos_sensor(0x6F12, 0xA0EB);
	write_cmos_sensor(0x6F12, 0x0B00);
	write_cmos_sensor(0x6F12, 0x8010);
	write_cmos_sensor(0x6F12, 0x0790);
	write_cmos_sensor(0x6F12, 0xFE48);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xFC00);
	write_cmos_sensor(0x6F12, 0x10B1);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x03E0);
	write_cmos_sensor(0x6F12, 0x51E0);
	write_cmos_sensor(0x6F12, 0xFA48);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x2C01);
	write_cmos_sensor(0x6F12, 0x8246);
	write_cmos_sensor(0x6F12, 0xF948);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x4E15);
	write_cmos_sensor(0x6F12, 0x0798);
	write_cmos_sensor(0x6F12, 0x8142);
	write_cmos_sensor(0x6F12, 0x03D2);
	write_cmos_sensor(0x6F12, 0x390C);
	write_cmos_sensor(0x6F12, 0x3620);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xF2FA);
	write_cmos_sensor(0x6F12, 0xF348);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xFD10);
	write_cmos_sensor(0x6F12, 0x19B9);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x2801);
	write_cmos_sensor(0x6F12, 0x07EB);
	write_cmos_sensor(0x6F12, 0x0047);
	write_cmos_sensor(0x6F12, 0x0798);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x2AE0);
	write_cmos_sensor(0x6F12, 0x0128);
	write_cmos_sensor(0x6F12, 0x07D0);
	write_cmos_sensor(0x6F12, 0x15D9);
	write_cmos_sensor(0x6F12, 0xEE4A);
	write_cmos_sensor(0x6F12, 0x0146);
	write_cmos_sensor(0x6F12, 0x5846);
	write_cmos_sensor(0x6F12, 0x9269);
	write_cmos_sensor(0x6F12, 0x9047);
	write_cmos_sensor(0x6F12, 0x5846);
	write_cmos_sensor(0x6F12, 0x0BE0);
	write_cmos_sensor(0x6F12, 0xDBF8);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0x01EB);
	write_cmos_sensor(0x6F12, 0x0A00);
	write_cmos_sensor(0x6F12, 0x3843);
	write_cmos_sensor(0x6F12, 0x01C4);
	write_cmos_sensor(0x6F12, 0x07E0);
	write_cmos_sensor(0x6F12, 0x04C8);
	write_cmos_sensor(0x6F12, 0x02EB);
	write_cmos_sensor(0x6F12, 0x0A01);
	write_cmos_sensor(0x6F12, 0x3943);
	write_cmos_sensor(0x6F12, 0x02C4);
	write_cmos_sensor(0x6F12, 0x0C99);
	write_cmos_sensor(0x6F12, 0x8842);
	write_cmos_sensor(0x6F12, 0xF7D1);
	write_cmos_sensor(0x6F12, 0x0398);
	write_cmos_sensor(0x6F12, 0x0190);
	write_cmos_sensor(0x6F12, 0x0498);
	write_cmos_sensor(0x6F12, 0x0290);
	write_cmos_sensor(0x6F12, 0x0598);
	write_cmos_sensor(0x6F12, 0xB6FB);
	write_cmos_sensor(0x6F12, 0xF0F1);
	write_cmos_sensor(0x6F12, 0xB6FB);
	write_cmos_sensor(0x6F12, 0xF0F2);
	write_cmos_sensor(0x6F12, 0x00FB);
	write_cmos_sensor(0x6F12, 0x1163);
	write_cmos_sensor(0x6F12, 0x0098);
	write_cmos_sensor(0x6F12, 0x0099);
	write_cmos_sensor(0x6F12, 0x5043);
	write_cmos_sensor(0x6F12, 0x0144);
	write_cmos_sensor(0x6F12, 0x4FEA);
	write_cmos_sensor(0x6F12, 0x424A);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x331A);
	write_cmos_sensor(0x6F12, 0x109A);
	write_cmos_sensor(0x6F12, 0xDA40);
	write_cmos_sensor(0x6F12, 0xD207);
	write_cmos_sensor(0x6F12, 0x08D0);
	write_cmos_sensor(0x6F12, 0x06F0);
	write_cmos_sensor(0x6F12, 0x0103);
	write_cmos_sensor(0x6F12, 0x01AE);
	write_cmos_sensor(0x6F12, 0x0E9F);
	write_cmos_sensor(0x6F12, 0x56F8);
	write_cmos_sensor(0x6F12, 0x2320);
	write_cmos_sensor(0x6F12, 0x80C2);
	write_cmos_sensor(0x6F12, 0x46F8);
	write_cmos_sensor(0x6F12, 0x2320);
	write_cmos_sensor(0x6F12, 0x0B9B);
	write_cmos_sensor(0x6F12, 0x9945);
	write_cmos_sensor(0x6F12, 0x7FF4);
	write_cmos_sensor(0x6F12, 0x1FAF);
	write_cmos_sensor(0x6F12, 0x01A8);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2590);
	write_cmos_sensor(0x6F12, 0x03A8);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2560);
	write_cmos_sensor(0x6F12, 0x4AEA);
	write_cmos_sensor(0x6F12, 0x0545);
	write_cmos_sensor(0x6F12, 0xA9EB);
	write_cmos_sensor(0x6F12, 0x0600);
	write_cmos_sensor(0x6F12, 0x4FEA);
	write_cmos_sensor(0x6F12, 0xA00B);
	write_cmos_sensor(0x6F12, 0xC948);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xFC00);
	write_cmos_sensor(0x6F12, 0x08B1);
	write_cmos_sensor(0x6F12, 0x0027);
	write_cmos_sensor(0x6F12, 0x02E0);
	write_cmos_sensor(0x6F12, 0xC648);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x2C71);
	write_cmos_sensor(0x6F12, 0xC648);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x4E15);
	write_cmos_sensor(0x6F12, 0x5945);
	write_cmos_sensor(0x6F12, 0x03D2);
	write_cmos_sensor(0x6F12, 0x290C);
	write_cmos_sensor(0x6F12, 0x3620);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x8CFA);
	write_cmos_sensor(0x6F12, 0xC048);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xFD00);
	write_cmos_sensor(0x6F12, 0x20B9);
	write_cmos_sensor(0x6F12, 0xBE48);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x2801);
	write_cmos_sensor(0x6F12, 0x05EB);
	write_cmos_sensor(0x6F12, 0x0045);
	write_cmos_sensor(0x6F12, 0x5846);
	write_cmos_sensor(0x6F12, 0xBBF1);
	write_cmos_sensor(0x6F12, 0x010F);
	write_cmos_sensor(0x6F12, 0x06D0);
	write_cmos_sensor(0x6F12, 0x10D9);
	write_cmos_sensor(0x6F12, 0xBB4A);
	write_cmos_sensor(0x6F12, 0x0146);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x9269);
	write_cmos_sensor(0x6F12, 0x9047);
	write_cmos_sensor(0x6F12, 0x08E0);
	write_cmos_sensor(0x6F12, 0x3068);
	write_cmos_sensor(0x6F12, 0x3844);
	write_cmos_sensor(0x6F12, 0x2843);
	write_cmos_sensor(0x6F12, 0x01C4);
	write_cmos_sensor(0x6F12, 0x05E0);
	write_cmos_sensor(0x6F12, 0x01CE);
	write_cmos_sensor(0x6F12, 0x3844);
	write_cmos_sensor(0x6F12, 0x2843);
	write_cmos_sensor(0x6F12, 0x01C4);
	write_cmos_sensor(0x6F12, 0x4E45);
	write_cmos_sensor(0x6F12, 0xF9D1);
	write_cmos_sensor(0x6F12, 0x01A8);
	write_cmos_sensor(0x6F12, 0x4AEA);
	write_cmos_sensor(0x6F12, 0x0845);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2890);
	write_cmos_sensor(0x6F12, 0x03A8);
	write_cmos_sensor(0x6F12, 0xDFF8);
	write_cmos_sensor(0x6F12, 0xB8A2);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2860);
	write_cmos_sensor(0x6F12, 0xA9EB);
	write_cmos_sensor(0x6F12, 0x0600);
	write_cmos_sensor(0x6F12, 0x4FEA);
	write_cmos_sensor(0x6F12, 0xA008);
	write_cmos_sensor(0x6F12, 0x9AF8);
	write_cmos_sensor(0x6F12, 0xFC00);
	write_cmos_sensor(0x6F12, 0x08B1);
	write_cmos_sensor(0x6F12, 0x0027);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0xBAF8);
	write_cmos_sensor(0x6F12, 0x2C71);
	write_cmos_sensor(0x6F12, 0xA748);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x4E15);
	write_cmos_sensor(0x6F12, 0x4145);
	write_cmos_sensor(0x6F12, 0x03D2);
	write_cmos_sensor(0x6F12, 0x290C);
	write_cmos_sensor(0x6F12, 0x3620);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x4FFA);
	write_cmos_sensor(0x6F12, 0x9AF8);
	write_cmos_sensor(0x6F12, 0xFD10);
	write_cmos_sensor(0x6F12, 0x19B9);
	write_cmos_sensor(0x6F12, 0xBAF8);
	write_cmos_sensor(0x6F12, 0x2801);
	write_cmos_sensor(0x6F12, 0x05EB);
	write_cmos_sensor(0x6F12, 0x0045);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0xB8F1);
	write_cmos_sensor(0x6F12, 0x010F);
	write_cmos_sensor(0x6F12, 0x06D0);
	write_cmos_sensor(0x6F12, 0x10D9);
	write_cmos_sensor(0x6F12, 0x9E4A);
	write_cmos_sensor(0x6F12, 0x0146);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x9269);
	write_cmos_sensor(0x6F12, 0x9047);
	write_cmos_sensor(0x6F12, 0x08E0);
	write_cmos_sensor(0x6F12, 0x3068);
	write_cmos_sensor(0x6F12, 0x3844);
	write_cmos_sensor(0x6F12, 0x2843);
	write_cmos_sensor(0x6F12, 0x01C4);
	write_cmos_sensor(0x6F12, 0x05E0);
	write_cmos_sensor(0x6F12, 0x01CE);
	write_cmos_sensor(0x6F12, 0x3844);
	write_cmos_sensor(0x6F12, 0x2843);
	write_cmos_sensor(0x6F12, 0x01C4);
	write_cmos_sensor(0x6F12, 0x4E45);
	write_cmos_sensor(0x6F12, 0xF9D1);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0xFF30);
	write_cmos_sensor(0x6F12, 0x2060);
	write_cmos_sensor(0x6F12, 0x1298);
	write_cmos_sensor(0x6F12, 0x8442);
	write_cmos_sensor(0x6F12, 0x01D0);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x9249);
	write_cmos_sensor(0x6F12, 0x0870);
	write_cmos_sensor(0x6F12, 0x1298);
	write_cmos_sensor(0x6F12, 0x201A);
	write_cmos_sensor(0x6F12, 0x8008);
	write_cmos_sensor(0x6F12, 0x0884);
	write_cmos_sensor(0x6F12, 0x13B0);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF08F);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0x6920);
	write_cmos_sensor(0x6F12, 0x0D46);
	write_cmos_sensor(0x6F12, 0x0446);
	write_cmos_sensor(0x6F12, 0x032A);
	write_cmos_sensor(0x6F12, 0x15D0);
	write_cmos_sensor(0x6F12, 0x062A);
	write_cmos_sensor(0x6F12, 0x13D0);
	write_cmos_sensor(0x6F12, 0x8748);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xC069);
	write_cmos_sensor(0x6F12, 0x87B2);
	write_cmos_sensor(0x6F12, 0x060C);
	write_cmos_sensor(0x6F12, 0x3946);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xF4F9);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x0EFA);
	write_cmos_sensor(0x6F12, 0x3946);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xE9B9);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x12E6);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x0446);
	write_cmos_sensor(0x6F12, 0x7A48);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x006A);
	write_cmos_sensor(0x6F12, 0x87B2);
	write_cmos_sensor(0x6F12, 0x4FEA);
	write_cmos_sensor(0x6F12, 0x1048);
	write_cmos_sensor(0x6F12, 0x3946);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xD9F9);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xF9F9);
	write_cmos_sensor(0x6F12, 0x764E);
	write_cmos_sensor(0x6F12, 0x04F1);
	write_cmos_sensor(0x6F12, 0x5805);
	write_cmos_sensor(0x6F12, 0xB6F8);
	write_cmos_sensor(0x6F12, 0xB400);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xF7F9);
	write_cmos_sensor(0x6F12, 0x80B2);
	write_cmos_sensor(0x6F12, 0xE880);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0xB810);
	write_cmos_sensor(0x6F12, 0x0129);
	write_cmos_sensor(0x6F12, 0x03D0);
	write_cmos_sensor(0x6F12, 0xB6F8);
	write_cmos_sensor(0x6F12, 0xB600);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xEDF9);
	write_cmos_sensor(0x6F12, 0xA881);
	write_cmos_sensor(0x6F12, 0x96F8);
	write_cmos_sensor(0x6F12, 0xB800);
	write_cmos_sensor(0x6F12, 0x0128);
	write_cmos_sensor(0x6F12, 0x0ED0);
	write_cmos_sensor(0x6F12, 0x6C48);
	write_cmos_sensor(0x6F12, 0x0068);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x4800);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xE2F9);
	write_cmos_sensor(0x6F12, 0xA4F8);
	write_cmos_sensor(0x6F12, 0x4800);
	write_cmos_sensor(0x6F12, 0x3946);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xB1B9);
	write_cmos_sensor(0x6F12, 0x34F8);
	write_cmos_sensor(0x6F12, 0x420F);
	write_cmos_sensor(0x6F12, 0xE080);
	write_cmos_sensor(0x6F12, 0xF4E7);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x5C4C);
	write_cmos_sensor(0x6F12, 0x624D);
	write_cmos_sensor(0x6F12, 0x0F46);
	write_cmos_sensor(0x6F12, 0x8046);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x4430);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0xFD20);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0x5C11);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x23B1);
	write_cmos_sensor(0x6F12, 0x09B1);
	write_cmos_sensor(0x6F12, 0x0220);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0x02B1);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x6F12, 0x5B4E);
	write_cmos_sensor(0x6F12, 0xA6F8);
	write_cmos_sensor(0x6F12, 0x4602);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x2030);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x23B1);
	write_cmos_sensor(0x6F12, 0x09B1);
	write_cmos_sensor(0x6F12, 0x0220);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0x02B1);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x6F12, 0x70B1);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x0221);
	write_cmos_sensor(0x6F12, 0x47F2);
	write_cmos_sensor(0x6F12, 0xC600);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x87F9);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0xFD10);
	write_cmos_sensor(0x6F12, 0x39B1);
	write_cmos_sensor(0x6F12, 0x5148);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0x5D00);
	write_cmos_sensor(0x6F12, 0x18B1);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x6F12, 0x02E0);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0xEFE7);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0xB120);
	write_cmos_sensor(0x6F12, 0x5040);
	write_cmos_sensor(0x6F12, 0x84F8);
	write_cmos_sensor(0x6F12, 0x5C01);
	write_cmos_sensor(0x6F12, 0x84F8);
	write_cmos_sensor(0x6F12, 0x5D01);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x2020);
	write_cmos_sensor(0x6F12, 0x022A);
	write_cmos_sensor(0x6F12, 0x04D1);
	write_cmos_sensor(0x6F12, 0x19B1);
	write_cmos_sensor(0x6F12, 0x80EA);
	write_cmos_sensor(0x6F12, 0x0102);
	write_cmos_sensor(0x6F12, 0x84F8);
	write_cmos_sensor(0x6F12, 0x5C21);
	write_cmos_sensor(0x6F12, 0xA6F8);
	write_cmos_sensor(0x6F12, 0x6A02);
	write_cmos_sensor(0x6F12, 0x3B48);
	write_cmos_sensor(0x6F12, 0x9030);
	write_cmos_sensor(0x6F12, 0x0646);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x91F9);
	write_cmos_sensor(0x6F12, 0xC4F8);
	write_cmos_sensor(0x6F12, 0x6001);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x91F9);
	write_cmos_sensor(0x6F12, 0xC4F8);
	write_cmos_sensor(0x6F12, 0x6401);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x2020);
	write_cmos_sensor(0x6F12, 0x354D);
	write_cmos_sensor(0x6F12, 0x022A);
	write_cmos_sensor(0x6F12, 0x3FD0);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0x5C01);
	write_cmos_sensor(0x6F12, 0xE8B3);
	write_cmos_sensor(0x6F12, 0x0021);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0xAB32);
	write_cmos_sensor(0x6F12, 0x5940);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0xFC30);
	write_cmos_sensor(0x6F12, 0x5840);
	write_cmos_sensor(0x6F12, 0x14F8);
	write_cmos_sensor(0x6F12, 0xEB3F);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0x3640);
	write_cmos_sensor(0x6F12, 0x2344);
	write_cmos_sensor(0x6F12, 0x9B07);
	write_cmos_sensor(0x6F12, 0x00D0);
	write_cmos_sensor(0x6F12, 0x0123);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0xAA42);
	write_cmos_sensor(0x6F12, 0x6340);
	write_cmos_sensor(0x6F12, 0x5840);
	write_cmos_sensor(0x6F12, 0xA2B1);
	write_cmos_sensor(0x6F12, 0x40EA);
	write_cmos_sensor(0x6F12, 0x4100);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0xC213);
	write_cmos_sensor(0x6F12, 0x40EA);
	write_cmos_sensor(0x6F12, 0x8100);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0xC313);
	write_cmos_sensor(0x6F12, 0x40EA);
	write_cmos_sensor(0x6F12, 0xC100);
	write_cmos_sensor(0x6F12, 0x2B49);
	write_cmos_sensor(0x6F12, 0xA1F8);
	write_cmos_sensor(0x6F12, 0x4A03);
	write_cmos_sensor(0x6F12, 0x8915);
	write_cmos_sensor(0x6F12, 0x4AF2);
	write_cmos_sensor(0x6F12, 0x4A30);
	write_cmos_sensor(0x6F12, 0x022A);
	write_cmos_sensor(0x6F12, 0x1ED0);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x2CF9);
	write_cmos_sensor(0x6F12, 0x1F48);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x416A);
	write_cmos_sensor(0x6F12, 0x0C0C);
	write_cmos_sensor(0x6F12, 0x8DB2);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x23F9);
	write_cmos_sensor(0x6F12, 0x3946);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x56F9);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x18B9);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x03E0);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0x5C11);
	write_cmos_sensor(0x6F12, 0x0846);
	write_cmos_sensor(0x6F12, 0xD0E7);
	write_cmos_sensor(0x6F12, 0x0121);
	write_cmos_sensor(0x6F12, 0xBBE7);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0xDFE7);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0x0646);
	write_cmos_sensor(0x6F12, 0x0F48);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x816A);
	write_cmos_sensor(0x6F12, 0x0C0C);
	write_cmos_sensor(0x6F12, 0x8DB2);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x03F9);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x3CF9);
	write_cmos_sensor(0x6F12, 0x1148);
	write_cmos_sensor(0x6F12, 0x114A);
	write_cmos_sensor(0x6F12, 0x8188);
	write_cmos_sensor(0x6F12, 0x1180);
	write_cmos_sensor(0x6F12, 0x911C);
	write_cmos_sensor(0x6F12, 0xC088);
	write_cmos_sensor(0x6F12, 0x0880);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0x7040);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xF2B8);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1EB0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1350);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3F80);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1C20);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x0520);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2970);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0xF000);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x05C0);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0xA000);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x4A00);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0x950C);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF6);
	write_cmos_sensor(0x6F12, 0xAB31);
	write_cmos_sensor(0x6F12, 0x4848);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x13F9);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF6);
	write_cmos_sensor(0x6F12, 0x8531);
	write_cmos_sensor(0x6F12, 0x4648);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x0DF9);
	write_cmos_sensor(0x6F12, 0x464D);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF6);
	write_cmos_sensor(0x6F12, 0xEF21);
	write_cmos_sensor(0x6F12, 0xE860);
	write_cmos_sensor(0x6F12, 0x4448);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x05F9);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF6);
	write_cmos_sensor(0x6F12, 0x8921);
	write_cmos_sensor(0x6F12, 0xA860);
	write_cmos_sensor(0x6F12, 0x4248);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xFEF8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF6);
	write_cmos_sensor(0x6F12, 0x0351);
	write_cmos_sensor(0x6F12, 0x2861);
	write_cmos_sensor(0x6F12, 0x3F48);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xF7F8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF6);
	write_cmos_sensor(0x6F12, 0xCF41);
	write_cmos_sensor(0x6F12, 0x2860);
	write_cmos_sensor(0x6F12, 0x3D48);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xF0F8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0xB321);
	write_cmos_sensor(0x6F12, 0x6860);
	write_cmos_sensor(0x6F12, 0x3A48);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xE9F8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x8121);
	write_cmos_sensor(0x6F12, 0xE861);
	write_cmos_sensor(0x6F12, 0x3848);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xE2F8);
	write_cmos_sensor(0x6F12, 0x3749);
	write_cmos_sensor(0x6F12, 0x2862);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0x4880);
	write_cmos_sensor(0x6F12, 0x3648);
	write_cmos_sensor(0x6F12, 0x4FF4);
	write_cmos_sensor(0x6F12, 0xA051);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x0182);
	write_cmos_sensor(0x6F12, 0x41F2);
	write_cmos_sensor(0x6F12, 0xA471);
	write_cmos_sensor(0x6F12, 0x4182);
	write_cmos_sensor(0x6F12, 0x41F2);
	write_cmos_sensor(0x6F12, 0xAB61);
	write_cmos_sensor(0x6F12, 0x8182);
	write_cmos_sensor(0x6F12, 0xAFF6);
	write_cmos_sensor(0x6F12, 0x8311);
	write_cmos_sensor(0x6F12, 0x3148);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xCEF8);
	write_cmos_sensor(0x6F12, 0x304C);
	write_cmos_sensor(0x6F12, 0x6861);
	write_cmos_sensor(0x6F12, 0x44F6);
	write_cmos_sensor(0x6F12, 0xC860);
	write_cmos_sensor(0x6F12, 0xE18C);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xCCF8);
	write_cmos_sensor(0x6F12, 0xE08C);
	write_cmos_sensor(0x6F12, 0x2E4E);
	write_cmos_sensor(0x6F12, 0x2D49);
	write_cmos_sensor(0x6F12, 0x46F8);
	write_cmos_sensor(0x6F12, 0x2010);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0x81B2);
	write_cmos_sensor(0x6F12, 0xE184);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0xD800);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xC0F8);
	write_cmos_sensor(0x6F12, 0xE08C);
	write_cmos_sensor(0x6F12, 0x2949);
	write_cmos_sensor(0x6F12, 0x46F8);
	write_cmos_sensor(0x6F12, 0x2010);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0x81B2);
	write_cmos_sensor(0x6F12, 0xE184);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0xDC00);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0xE08C);
	write_cmos_sensor(0x6F12, 0x2549);
	write_cmos_sensor(0x6F12, 0x46F8);
	write_cmos_sensor(0x6F12, 0x2010);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0x81B2);
	write_cmos_sensor(0x6F12, 0xE184);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0x4440);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xAAF8);
	write_cmos_sensor(0x6F12, 0xE08C);
	write_cmos_sensor(0x6F12, 0x2049);
	write_cmos_sensor(0x6F12, 0x46F8);
	write_cmos_sensor(0x6F12, 0x2010);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0x81B2);
	write_cmos_sensor(0x6F12, 0xE184);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0x4840);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x9FF8);
	write_cmos_sensor(0x6F12, 0xE08C);
	write_cmos_sensor(0x6F12, 0x1C49);
	write_cmos_sensor(0x6F12, 0x46F8);
	write_cmos_sensor(0x6F12, 0x2010);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0xE084);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0xB721);
	write_cmos_sensor(0x6F12, 0x1948);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x8EF8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x8511);
	write_cmos_sensor(0x6F12, 0x6862);
	write_cmos_sensor(0x6F12, 0x1648);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x87F8);
	write_cmos_sensor(0x6F12, 0xA862);
	write_cmos_sensor(0x6F12, 0x70BD);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x7C2D);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xACA1);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3F80);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xA871);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x9E41);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x3367);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x3783);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x3CA7);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x69B9);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x4A00);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x0570);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0A91);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1E80);
	write_cmos_sensor(0x6F12, 0x9E04);
	write_cmos_sensor(0x6F12, 0x0BE0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2E10);
	write_cmos_sensor(0x6F12, 0x18BF);
	write_cmos_sensor(0x6F12, 0x06F2);
	write_cmos_sensor(0x6F12, 0x0144);
	write_cmos_sensor(0x6F12, 0x04F5);
	write_cmos_sensor(0x6F12, 0x18BF);
	write_cmos_sensor(0x6F12, 0x0AF2);
	write_cmos_sensor(0x6F12, 0x0148);
	write_cmos_sensor(0x6F12, 0x1FFA);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x7A19);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x1209);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0x473C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0xEF2C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F2);
	write_cmos_sensor(0x6F12, 0x237C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0xE96C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x43F2);
	write_cmos_sensor(0x6F12, 0x736C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F2);
	write_cmos_sensor(0x6F12, 0xA37C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4AF6);
	write_cmos_sensor(0x6F12, 0xA14C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4AF6);
	write_cmos_sensor(0x6F12, 0x710C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F6);
	write_cmos_sensor(0x6F12, 0x592C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x43F6);
	write_cmos_sensor(0x6F12, 0xE13C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F2);
	write_cmos_sensor(0x6F12, 0xA90C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x43F6);
	write_cmos_sensor(0x6F12, 0xA74C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x46F6);
	write_cmos_sensor(0x6F12, 0xB91C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x46F6);
	write_cmos_sensor(0x6F12, 0xAD1C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x47F6);
	write_cmos_sensor(0x6F12, 0x1B3C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x47F6);
	write_cmos_sensor(0x6F12, 0x073C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x47F6);
	write_cmos_sensor(0x6F12, 0x192C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x41F2);
	write_cmos_sensor(0x6F12, 0x092C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4DF2);
	write_cmos_sensor(0x6F12, 0x936C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4DF2);
	write_cmos_sensor(0x6F12, 0x336C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x3108);
	write_cmos_sensor(0x6F12, 0x01FC);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0FFF);
	write_cmos_sensor(0x602A, 0x4A04);
	write_cmos_sensor(0x6F12, 0x0088);
	write_cmos_sensor(0x6F12, 0x0D70);
	write_cmos_sensor(0x602A, 0x4A00);
	write_cmos_sensor(0x6F12, 0x0005);
	write_cmos_sensor(0x021A, 0x0100);
	write_cmos_sensor(0x0204, 0x0020);
	write_cmos_sensor(0x0206, 0x0200);
	write_cmos_sensor(0x0208, 0x0100);
	write_cmos_sensor(0xF496, 0x0050);
	write_cmos_sensor(0xF470, 0x0008);
	write_cmos_sensor(0xF43A, 0x0015);
	write_cmos_sensor(0x3676, 0x0008);
	write_cmos_sensor(0x3678, 0x0008);
	write_cmos_sensor(0x362A, 0x0404);
	write_cmos_sensor(0x362C, 0x0404);
	write_cmos_sensor(0x362E, 0x0404);
	write_cmos_sensor(0x3630, 0x0404);
	write_cmos_sensor(0x3622, 0x0707);
	write_cmos_sensor(0x3624, 0x0707);
	write_cmos_sensor(0x3626, 0x0707);
	write_cmos_sensor(0x3628, 0x0707);
	write_cmos_sensor(0xF484, 0x0006);
	write_cmos_sensor(0xF440, 0x002F);
	write_cmos_sensor(0xF442, 0x44C6);
	write_cmos_sensor(0xF408, 0xFFF7);
	write_cmos_sensor(0x3666, 0x030B);
	write_cmos_sensor(0xF494, 0x1010);
	write_cmos_sensor(0x322E, 0x0003);
	write_cmos_sensor(0x3236, 0x0002);
	write_cmos_sensor(0x32A6, 0x0004);
	write_cmos_sensor(0x32EE, 0x0001);
	write_cmos_sensor(0x32F6, 0x0003);
	write_cmos_sensor(0xF4D4, 0x0038);
	write_cmos_sensor(0xF4D6, 0x0039);
	write_cmos_sensor(0xF4D8, 0x0034);
	write_cmos_sensor(0xF4DA, 0x0035);
	write_cmos_sensor(0xF4DC, 0x0038);
	write_cmos_sensor(0xF4DE, 0x0039);
	write_cmos_sensor(0x3616, 0x0606);
	write_cmos_sensor(0x3618, 0x0606);
	write_cmos_sensor(0xF47C, 0x001F);
	write_cmos_sensor(0x3476, 0x00BD);
	write_cmos_sensor(0x347A, 0x00D5);
	write_cmos_sensor(0x347E, 0x00ED);
	write_cmos_sensor(0x3482, 0x0105);
	write_cmos_sensor(0x3486, 0x011D);
	write_cmos_sensor(0x348A, 0x0135);
	write_cmos_sensor(0xF62E, 0x00C5);
	write_cmos_sensor(0xF630, 0x00CD);
	write_cmos_sensor(0xF632, 0x00DD);
	write_cmos_sensor(0xF634, 0x00E5);
	write_cmos_sensor(0xF636, 0x00F5);
	write_cmos_sensor(0xF638, 0x00FD);
	write_cmos_sensor(0xF63a, 0x010D);
	write_cmos_sensor(0xF63C, 0x0115);
	write_cmos_sensor(0xF63E, 0x0125);
	write_cmos_sensor(0xF640, 0x012D);
	write_cmos_sensor(0x361A, 0x0E0E);
	write_cmos_sensor(0x361C, 0x0E0E);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1704);
	write_cmos_sensor(0x6F12, 0x8011);
	write_cmos_sensor(0x6F12, 0x8110);
	write_cmos_sensor(0x3D34, 0x0010);
	write_cmos_sensor(0x3D64, 0x0105);
	write_cmos_sensor(0x3D66, 0x0105);
	write_cmos_sensor(0x3D6C, 0x00F0);
	write_cmos_sensor(0x31B4, 0x0006);
	write_cmos_sensor(0x31B6, 0x0008);
	write_cmos_sensor(0x3D50, 0x0107);
	write_cmos_sensor(0x3680, 0x0101);
	write_cmos_sensor(0x3AC8, 0x0A04);
	write_cmos_sensor(0x0202, 0x0006);
	write_cmos_sensor(0x0200, 0x0618);
	write_cmos_sensor(0x021E, 0x0300);
	write_cmos_sensor(0x021C, 0x0000);
	write_cmos_sensor(0x3606, 0x0103);
	write_cmos_sensor(0x0110, 0x0002);
	write_cmos_sensor(0x0114, 0x0300);
	write_cmos_sensor(0x30A0, 0x0008);
	write_cmos_sensor(0x0112, 0x0a0a);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x157C);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x15F0);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x0B00, 0x0080);
	write_cmos_sensor(0x3070, 0x00FF);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1898);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x0B04, 0x0101);
	write_cmos_sensor(0x0B08, 0x0000);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x1710);
	write_cmos_sensor(0x6F12, 0x03D9);
	write_cmos_sensor(0x3090, 0x0904);
	write_cmos_sensor(0x31A4, 0x0102);
	} else{
	LOG_INF("EVT1 E\n");
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6010, 0x0001);
	mdelay(3);         /* Wait value must be at least 20000 MCLKs */
	write_cmos_sensor(0x6214, 0x7971);
	write_cmos_sensor(0x6218, 0x7150);

	write_cmos_sensor(0x6028, 0x2000);  /* TnP Start */
	write_cmos_sensor(0x602A, 0x3074);

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
	write_cmos_sensor(0x6F12, 0x52B8);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x31E8);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1E80);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x4B00);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0x0646);
	write_cmos_sensor(0x6F12, 0x3248);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x0168);
	write_cmos_sensor(0x6F12, 0x0C0C);
	write_cmos_sensor(0x6F12, 0x8DB2);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x73F8);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x75F8);
	write_cmos_sensor(0x6F12, 0x2D48);
	write_cmos_sensor(0x6F12, 0x2E4A);
	write_cmos_sensor(0x6F12, 0x0188);
	write_cmos_sensor(0x6F12, 0x1180);
	write_cmos_sensor(0x6F12, 0x911C);
	write_cmos_sensor(0x6F12, 0x4088);
	write_cmos_sensor(0x6F12, 0x0880);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0x7040);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x62B8);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x8046);
	write_cmos_sensor(0x6F12, 0x2448);
	write_cmos_sensor(0x6F12, 0x1446);
	write_cmos_sensor(0x6F12, 0x0F46);
	write_cmos_sensor(0x6F12, 0x4068);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x85B2);
	write_cmos_sensor(0x6F12, 0x060C);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x54F8);
	write_cmos_sensor(0x6F12, 0x2148);
	write_cmos_sensor(0x6F12, 0x408C);
	write_cmos_sensor(0x6F12, 0x70B1);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0x6B00);
	write_cmos_sensor(0x6F12, 0x4021);
	write_cmos_sensor(0x6F12, 0xB1FB);
	write_cmos_sensor(0x6F12, 0xF0F0);
	write_cmos_sensor(0x6F12, 0x1E49);
	write_cmos_sensor(0x6F12, 0x1F4A);
	write_cmos_sensor(0x6F12, 0x098B);
	write_cmos_sensor(0x6F12, 0x5288);
	write_cmos_sensor(0x6F12, 0x891A);
	write_cmos_sensor(0x6F12, 0x0901);
	write_cmos_sensor(0x6F12, 0x91FB);
	write_cmos_sensor(0x6F12, 0xF0F0);
	write_cmos_sensor(0x6F12, 0x84B2);
	write_cmos_sensor(0x6F12, 0x05E0);
	write_cmos_sensor(0x6F12, 0x2246);
	write_cmos_sensor(0x6F12, 0x3946);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x47F8);
	write_cmos_sensor(0x6F12, 0x0446);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x37F8);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF081);
	write_cmos_sensor(0x6F12, 0x10B5);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x9B01);
	write_cmos_sensor(0x6F12, 0x1248);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x3CF8);
	write_cmos_sensor(0x6F12, 0x0B4C);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x6F01);
	write_cmos_sensor(0x6F12, 0x2060);
	write_cmos_sensor(0x6F12, 0x0F48);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x34F8);
	write_cmos_sensor(0x6F12, 0x6060);
	write_cmos_sensor(0x6F12, 0x0E4C);
	write_cmos_sensor(0x6F12, 0x40F6);
	write_cmos_sensor(0x6F12, 0x8C30);
	write_cmos_sensor(0x6F12, 0xE18C);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x32F8);
	write_cmos_sensor(0x6F12, 0xE08C);
	write_cmos_sensor(0x6F12, 0x0D4A);
	write_cmos_sensor(0x6F12, 0x0B49);
	write_cmos_sensor(0x6F12, 0x42F8);
	write_cmos_sensor(0x6F12, 0x2010);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0xE084);
	write_cmos_sensor(0x6F12, 0x10BD);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x31E0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x4B00);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0x950C);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2F10);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1B10);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2F40);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x15E9);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x9ECD);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x1E80);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6830);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2E10);
	write_cmos_sensor(0x6F12, 0x40F2);
	write_cmos_sensor(0x6F12, 0xA37C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x41F2);
	write_cmos_sensor(0x6F12, 0xE95C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x49F6);
	write_cmos_sensor(0x6F12, 0xCD6C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4DF6);
	write_cmos_sensor(0x6F12, 0x1B0C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4DF2);
	write_cmos_sensor(0x6F12, 0xBB7C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x3108);
	write_cmos_sensor(0x6F12, 0x029F);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0015);  /* TnP End */

	write_cmos_sensor(0x6028, 0x4000);  /* Global Start */
	write_cmos_sensor(0x0202, 0x0006);
	write_cmos_sensor(0x0200, 0x0618);
	write_cmos_sensor(0x021E, 0x0300);
	write_cmos_sensor(0x021C, 0x0000);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x162C);
	write_cmos_sensor(0x6F12, 0x8080);
	write_cmos_sensor(0x602A, 0x164C);
	write_cmos_sensor(0x6F12, 0x5555);
	write_cmos_sensor(0x6F12, 0x5500);
	write_cmos_sensor(0x602A, 0x166C);
	write_cmos_sensor(0x6F12, 0x4040);
	write_cmos_sensor(0x6F12, 0x4040);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x3606, 0x0103);
	write_cmos_sensor(0xF496, 0x004D);
	/* write_cmos_sensor(0xF496, 0x0048); //@2016.07.19 revised by dj for v0.03 */
	write_cmos_sensor(0xF470, 0x0008);
	write_cmos_sensor(0xF43A, 0x0015);
	write_cmos_sensor(0xF484, 0x0006);
	write_cmos_sensor(0xF442, 0x44C6);
	write_cmos_sensor(0xF408, 0xFFF7);
	write_cmos_sensor(0xF494, 0x1010);
	write_cmos_sensor(0xF4D4, 0x0038);
	write_cmos_sensor(0xF4D6, 0x0039);
	write_cmos_sensor(0xF4D8, 0x0034);
	write_cmos_sensor(0xF4DA, 0x0035);
	write_cmos_sensor(0xF4DC, 0x0038);
	write_cmos_sensor(0xF4DE, 0x0039);
	write_cmos_sensor(0xF47C, 0x001F);
	write_cmos_sensor(0xF62E, 0x00C5);
	write_cmos_sensor(0xF630, 0x00CD);
	write_cmos_sensor(0xF632, 0x00DD);
	write_cmos_sensor(0xF634, 0x00E5);
	write_cmos_sensor(0xF636, 0x00F5);
	write_cmos_sensor(0xF638, 0x00FD);
	write_cmos_sensor(0xF63A, 0x010D);
	write_cmos_sensor(0xF63C, 0x0115);
	write_cmos_sensor(0xF63E, 0x0125);
	write_cmos_sensor(0xF640, 0x012D);
	write_cmos_sensor(0x3070, 0x0100);
	write_cmos_sensor(0x3090, 0x0904);
	write_cmos_sensor(0x31C0, 0x00C8);

	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x18F0);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x18F6);
	write_cmos_sensor(0x6F12, 0x002F);
	write_cmos_sensor(0x6F12, 0x002F);
	write_cmos_sensor(0x6F12, 0xF440);  /* Gloabl End */
	}

}


static void preview_setting(void)
{
	/* Preview 2320*1744 30fps 24M MCLK 4lane 1464Mbps/lane */
	/* preview 30.01fps */
	LOG_INF("preview_setting() E\n");
	write_cmos_sensor_8(0x0100, 0x00);
	check_stremoff(imgsensor_info.pre.max_framerate);
	if (chip_id == 0xA0) {
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x6214, 0x7971);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x3004, 0x0003);
	write_cmos_sensor(0x3000, 0x0001);
	write_cmos_sensor(0xF444, 0x8000);
	write_cmos_sensor(0x3604, 0x0002);
	write_cmos_sensor(0x3058, 0x0001);
	write_cmos_sensor(0x0306, 0x0069);
	write_cmos_sensor(0x0300, 0x0003);
	write_cmos_sensor(0x030E, 0x007A);
	write_cmos_sensor(0x0344, 0x0008);
	write_cmos_sensor(0x0348, 0x1227);
	write_cmos_sensor(0x0346, 0x0008);
	write_cmos_sensor(0x034A, 0x0DA7);
	write_cmos_sensor(0x034C, 0x0910);
	write_cmos_sensor(0x034E, 0x06D0);
	write_cmos_sensor(0x0342, 0x1400);
	write_cmos_sensor(0x0340, 0x0E3B);
	write_cmos_sensor(0x0216, 0x0000);
	write_cmos_sensor(0x3664, 0x0019);
	write_cmos_sensor(0x1006, 0x0002);
	write_cmos_sensor(0x0900, 0x0122);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0003);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0003);
	write_cmos_sensor(0x0400, 0x0000);
	write_cmos_sensor(0x0404, 0x0010);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x19E0);
	if (imgsensor.pdaf_mode == 1) {
	 /* enable PDAF Tail Mode */
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x30E2, 0x0002);
	write_cmos_sensor(0x30E4, 0x0030);
	} else {
	write_cmos_sensor(0x6F12, 0x0001);
	}
	write_cmos_sensor(0x317A, 0xFFFF);
	write_cmos_sensor(0x0B0E, 0x0100);
	write_cmos_sensor(0x0100, 0x0100);
	} else {
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x0069);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0300, 0x0003);
	write_cmos_sensor(0x030C, 0x0004);
	write_cmos_sensor(0x030E, 0x007A);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x0344, 0x0008);
	write_cmos_sensor(0x0346, 0x0008);
	write_cmos_sensor(0x0348, 0x1227);
	write_cmos_sensor(0x034A, 0x0DA7);
	write_cmos_sensor(0x034C, 0x0910);
	write_cmos_sensor(0x034E, 0x06D0);
	write_cmos_sensor(0x0408, 0x0000);
	write_cmos_sensor(0x0900, 0x0112);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0003);
	write_cmos_sensor(0x0400, 0x0001);
	write_cmos_sensor(0x0404, 0x0020);
	write_cmos_sensor(0x0342, 0x1400);
	write_cmos_sensor(0x0340, 0x0E3B);
	write_cmos_sensor(0x0B0E, 0x0100);
	write_cmos_sensor(0x0216, 0x0000);
	write_cmos_sensor(0x3604, 0x0002);
	write_cmos_sensor(0x3664, 0x0019);
	write_cmos_sensor(0x3004, 0x0003);
	write_cmos_sensor(0x3000, 0x0001);
	write_cmos_sensor(0x317A, 0x00A0);
	write_cmos_sensor(0xF440, 0x002F);
	write_cmos_sensor(0x1006, 0x0002);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x19E0);
	if (imgsensor.pdaf_mode == 1) {
	/* enable PDAF Tail Mode */
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x30E2, 0x0002);
	write_cmos_sensor(0x30E4, 0x0030);
	} else {
	write_cmos_sensor(0x6F12, 0x0001);
	}
	write_cmos_sensor(0x602A, 0x18F6);
	write_cmos_sensor(0x6F12, 0x002F);
	write_cmos_sensor(0x6F12, 0x002F);

	write_cmos_sensor(0x31A4, 0x0102);
	write_cmos_sensor_8(0x0100, 0x01);
	}

	}	/* preview_setting */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("capture_setting() E! currefps:%d\n", currefps);

	write_cmos_sensor_8(0x0100, 0x00);
	check_stremoff(currefps);
	if (chip_id == 0xA0) {
		if (currefps == 300) {
			write_cmos_sensor(0x6028, 0x2000);
			write_cmos_sensor(0x6214, 0x7971);
			write_cmos_sensor(0x6218, 0x7150);
			write_cmos_sensor(0x6214, 0x7971);
			write_cmos_sensor(0x6218, 0x7150);
			write_cmos_sensor(0x3004, 0x0003);
			write_cmos_sensor(0x3000, 0x0001);
			write_cmos_sensor(0xF444, 0x8000);
			write_cmos_sensor(0x3604, 0x0002);
			write_cmos_sensor(0x3058, 0x0001);
			write_cmos_sensor(0x0306, 0x0069);
			write_cmos_sensor(0x0300, 0x0003);
			write_cmos_sensor(0x030E, 0x007A);
			write_cmos_sensor(0x0344, 0x0008);
			write_cmos_sensor(0x0348, 0x1227);
			write_cmos_sensor(0x0346, 0x0008);
			write_cmos_sensor(0x034A, 0x0DA7);
			write_cmos_sensor(0x034C, 0x1220);
			write_cmos_sensor(0x034E, 0x0DA0);
			write_cmos_sensor(0x0342, 0x1400);
			write_cmos_sensor(0x0340, 0x0E3B);
			write_cmos_sensor(0x0216, 0x0000);
			write_cmos_sensor(0x3664, 0x0019);
			write_cmos_sensor(0x1006, 0x0002);
			write_cmos_sensor(0x0900, 0x0011);
			write_cmos_sensor(0x0380, 0x0001);
			write_cmos_sensor(0x0382, 0x0001);
			write_cmos_sensor(0x0384, 0x0001);
			write_cmos_sensor(0x0386, 0x0001);
			write_cmos_sensor(0x0400, 0x0000);
			write_cmos_sensor(0x0404, 0x0010);
			write_cmos_sensor(0x0B0E, 0x0100);

		} else if (currefps == 240) {
			write_cmos_sensor(0x6028, 0x2000);
			write_cmos_sensor(0x3004, 0x0003);
			write_cmos_sensor(0x3000, 0x0001);
			write_cmos_sensor(0xF444, 0x8000);
			write_cmos_sensor(0x3604, 0x0002);
			write_cmos_sensor(0x3058, 0x0001);
			write_cmos_sensor(0x0306, 0x0069);
			write_cmos_sensor(0x0300, 0x0003);
			write_cmos_sensor(0x030E, 0x0062);
			write_cmos_sensor(0x0344, 0x0008);
			write_cmos_sensor(0x0348, 0x1227);
			write_cmos_sensor(0x0346, 0x0008);
			write_cmos_sensor(0x034A, 0x0DA7);
			write_cmos_sensor(0x034C, 0x1220);
			write_cmos_sensor(0x034E, 0x0DA0);
			write_cmos_sensor(0x0342, 0x1900);
			write_cmos_sensor(0x0340, 0x0E3B);
			write_cmos_sensor(0x0216, 0x0000);
			write_cmos_sensor(0x3664, 0x0019);
			write_cmos_sensor(0x1006, 0x0002);
			write_cmos_sensor(0x0900, 0x0011);
			write_cmos_sensor(0x0380, 0x0001);
			write_cmos_sensor(0x0382, 0x0001);
			write_cmos_sensor(0x0384, 0x0001);
			write_cmos_sensor(0x0386, 0x0001);
			write_cmos_sensor(0x0400, 0x0000);
			write_cmos_sensor(0x0404, 0x0010);
			write_cmos_sensor(0x0B0E, 0x0100);
		} else{
			write_cmos_sensor(0x6028, 0x2000);
			write_cmos_sensor(0x6214, 0x7971);
			write_cmos_sensor(0x6218, 0x7150);
			write_cmos_sensor(0x3004, 0x0003);
			write_cmos_sensor(0x3000, 0x0001);
			write_cmos_sensor(0xF444, 0x8000);
			write_cmos_sensor(0x3604, 0x0002);
			write_cmos_sensor(0x3058, 0x0001);
			write_cmos_sensor(0x0306, 0x0069);
			write_cmos_sensor(0x0300, 0x0003);
			write_cmos_sensor(0x030E, 0x003D);
			write_cmos_sensor(0x0344, 0x0008);
			write_cmos_sensor(0x0348, 0x1227);
			write_cmos_sensor(0x0346, 0x0008);
			write_cmos_sensor(0x034A, 0x0DA7);
			write_cmos_sensor(0x034C, 0x1220);
			write_cmos_sensor(0x034E, 0x0DA0);
			write_cmos_sensor(0x0342, 0x2800);
			write_cmos_sensor(0x0340, 0x0E3B);
			write_cmos_sensor(0x0216, 0x0000);
			write_cmos_sensor(0x3664, 0x0019);
			write_cmos_sensor(0x1006, 0x0002);
			write_cmos_sensor(0x0900, 0x0011);
			write_cmos_sensor(0x0380, 0x0001);
			write_cmos_sensor(0x0382, 0x0001);
			write_cmos_sensor(0x0384, 0x0001);
			write_cmos_sensor(0x0386, 0x0001);
			write_cmos_sensor(0x0400, 0x0000);
			write_cmos_sensor(0x0404, 0x0010);
			write_cmos_sensor(0x0B0E, 0x0100);
			}
			write_cmos_sensor(0x6028, 0x2000);
			write_cmos_sensor(0x602A, 0x19E0);
			if (imgsensor.pdaf_mode == 1) {
			/* enable PDAF Tail Mode */
			write_cmos_sensor(0x6F12, 0x0000);
			write_cmos_sensor(0x30E2, 0x0002);
			write_cmos_sensor(0x30E4, 0x0030);
			} else {
			write_cmos_sensor(0x6F12, 0x0001);
			}
			write_cmos_sensor(0x317A, 0x0007);
			write_cmos_sensor(0x0100, 0x0100);
			} else{
				/* full size 29.76fps */
				/* capture setting 4640*3488  24MHz 560MHz 1464Mbps/lane */
		    if (currefps == 300) {
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0136, 0x1800);
			write_cmos_sensor(0x0304, 0x0006);
			write_cmos_sensor(0x0306, 0x0069);
			write_cmos_sensor(0x0302, 0x0001);
			write_cmos_sensor(0x0300, 0x0003);
			write_cmos_sensor(0x030C, 0x0004);
			write_cmos_sensor(0x030E, 0x007A);
			write_cmos_sensor(0x030A, 0x0001);
			write_cmos_sensor(0x0308, 0x0008);
			write_cmos_sensor(0x0344, 0x0008);
			write_cmos_sensor(0x0346, 0x0008);
			write_cmos_sensor(0x0348, 0x1227);
			write_cmos_sensor(0x034A, 0x0DA7);
			write_cmos_sensor(0x034C, 0x1220);
			write_cmos_sensor(0x034E, 0x0DA0);
			write_cmos_sensor(0x0408, 0x0000);
			write_cmos_sensor(0x0900, 0x0011);
			write_cmos_sensor(0x0380, 0x0001);
			write_cmos_sensor(0x0382, 0x0001);
			write_cmos_sensor(0x0384, 0x0001);
			write_cmos_sensor(0x0386, 0x0001);
			write_cmos_sensor(0x0400, 0x0000);
			write_cmos_sensor(0x0404, 0x0010);
			write_cmos_sensor(0x0342, 0x1400);
			write_cmos_sensor(0x0340, 0x0E3B);
			write_cmos_sensor(0x0B0E, 0x0100);
			write_cmos_sensor(0x0216, 0x0000);
			write_cmos_sensor(0x3604, 0x0002);
			write_cmos_sensor(0x3664, 0x0019);
			write_cmos_sensor(0x3004, 0x0003);
			write_cmos_sensor(0x3000, 0x0001);
			write_cmos_sensor(0x317A, 0x0130);
			write_cmos_sensor(0x1006, 0x0002);
		    } else if (currefps == 240) {
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0136, 0x1800);
			write_cmos_sensor(0x0304, 0x0006);
			write_cmos_sensor(0x0306, 0x0069);
			write_cmos_sensor(0x0302, 0x0001);
			write_cmos_sensor(0x0300, 0x0003);
			write_cmos_sensor(0x030C, 0x0004);
			write_cmos_sensor(0x030E, 0x0062);
			write_cmos_sensor(0x030A, 0x0001);
			write_cmos_sensor(0x0308, 0x0008);
			write_cmos_sensor(0x0344, 0x0008);
			write_cmos_sensor(0x0346, 0x0008);
			write_cmos_sensor(0x0348, 0x1227);
			write_cmos_sensor(0x034A, 0x0DA7);
			write_cmos_sensor(0x034C, 0x1220);
			write_cmos_sensor(0x034E, 0x0DA0);
			write_cmos_sensor(0x0408, 0x0000);
			write_cmos_sensor(0x0900, 0x0011);
			write_cmos_sensor(0x0380, 0x0001);
			write_cmos_sensor(0x0382, 0x0001);
			write_cmos_sensor(0x0384, 0x0001);
			write_cmos_sensor(0x0386, 0x0001);
			write_cmos_sensor(0x0400, 0x0000);
			write_cmos_sensor(0x0404, 0x0010);
			write_cmos_sensor(0x0342, 0x1900);
			write_cmos_sensor(0x0340, 0x0E3B);
			write_cmos_sensor(0x0B0E, 0x0100);
			write_cmos_sensor(0x0216, 0x0000);
			write_cmos_sensor(0x3604, 0x0002);
			write_cmos_sensor(0x3664, 0x0019);
			write_cmos_sensor(0x3004, 0x0003);
			write_cmos_sensor(0x3000, 0x0001);
			write_cmos_sensor(0x317A, 0x0130);
			write_cmos_sensor(0x1006, 0x0002);
		    } else{
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0136, 0x1800);
			write_cmos_sensor(0x0304, 0x0006);
			write_cmos_sensor(0x0306, 0x0069);
			write_cmos_sensor(0x0302, 0x0001);
			write_cmos_sensor(0x0300, 0x0003);
			write_cmos_sensor(0x030C, 0x0004);
			write_cmos_sensor(0x030E, 0x003D);
			write_cmos_sensor(0x030A, 0x0001);
			write_cmos_sensor(0x0308, 0x0008);
			write_cmos_sensor(0x0344, 0x0008);
			write_cmos_sensor(0x0346, 0x0008);
			write_cmos_sensor(0x0348, 0x1227);
			write_cmos_sensor(0x034A, 0x0DA7);
			write_cmos_sensor(0x034C, 0x1220);
			write_cmos_sensor(0x034E, 0x0DA0);
			write_cmos_sensor(0x0408, 0x0000);
			write_cmos_sensor(0x0900, 0x0011);
			write_cmos_sensor(0x0380, 0x0001);
			write_cmos_sensor(0x0382, 0x0001);
			write_cmos_sensor(0x0384, 0x0001);
			write_cmos_sensor(0x0386, 0x0001);
			write_cmos_sensor(0x0400, 0x0000);
			write_cmos_sensor(0x0404, 0x0010);
			write_cmos_sensor(0x0342, 0x2800);
			write_cmos_sensor(0x0340, 0x0E3B);
			write_cmos_sensor(0x0B0E, 0x0100);
			write_cmos_sensor(0x0216, 0x0000);
			write_cmos_sensor(0x3604, 0x0002);
			write_cmos_sensor(0x3664, 0x0019);
			write_cmos_sensor(0x3004, 0x0003);
			write_cmos_sensor(0x3000, 0x0001);
			write_cmos_sensor(0x317A, 0x0130);
			write_cmos_sensor(0x1006, 0x0002);
		    }
			write_cmos_sensor(0x6028, 0x2000);
			write_cmos_sensor(0x602A, 0x19E0);
			if (imgsensor.pdaf_mode == 1)   {
			/* enable PDAF Tail Mode */
			write_cmos_sensor(0x6F12, 0x0000);
			write_cmos_sensor(0x30E2, 0x0002);
			write_cmos_sensor(0x30E4, 0x0030);
			} else {
			write_cmos_sensor(0x6F12, 0x0001);
			}
			write_cmos_sensor(0x602A, 0x18F6);
			write_cmos_sensor(0x6F12, 0x002F);
			write_cmos_sensor(0x6F12, 0x002F);

			write_cmos_sensor(0x31A4, 0x0102);
			write_cmos_sensor_8(0x0100, 0x01);
			}

}	/* capture setting */


static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("normal_video_setting() E! currefps:%d\n", currefps);
	write_cmos_sensor_8(0x0100, 0x00);
	check_stremoff(currefps);
	if (chip_id == 0xA0) {
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x6214, 0x7971);
		write_cmos_sensor(0x6218, 0x7150);
		write_cmos_sensor(0x6214, 0x7971);
		write_cmos_sensor(0x6218, 0x7150);
		write_cmos_sensor(0x3004, 0x0003);
		write_cmos_sensor(0x3000, 0x0001);
		write_cmos_sensor(0xF444, 0x8000);
		write_cmos_sensor(0x3604, 0x0002);
		write_cmos_sensor(0x3058, 0x0001);
		write_cmos_sensor(0x0306, 0x0069);
		write_cmos_sensor(0x0300, 0x0003);
		write_cmos_sensor(0x030E, 0x007A);
		write_cmos_sensor(0x0344, 0x0008);
		write_cmos_sensor(0x0348, 0x1227);
		write_cmos_sensor(0x0346, 0x0008);
		write_cmos_sensor(0x034A, 0x0DA7);
		write_cmos_sensor(0x034C, 0x1220);
		write_cmos_sensor(0x034E, 0x0DA0);
		write_cmos_sensor(0x0342, 0x1400);
		write_cmos_sensor(0x0340, 0x0E3B);
		write_cmos_sensor(0x0216, 0x0000);
		write_cmos_sensor(0x3664, 0x0019);
		write_cmos_sensor(0x1006, 0x0002);
		write_cmos_sensor(0x0900, 0x0011);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0001);
		write_cmos_sensor(0x0400, 0x0000);
		write_cmos_sensor(0x0404, 0x0010);
		write_cmos_sensor(0x0B0E, 0x0100);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x19E0);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x317A, 0x0007);
		write_cmos_sensor(0x0100, 0x0100);
		} else{
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x0136, 0x1800);
		write_cmos_sensor(0x0304, 0x0006);
		write_cmos_sensor(0x0306, 0x0069);
		write_cmos_sensor(0x0302, 0x0001);
		write_cmos_sensor(0x0300, 0x0003);
		write_cmos_sensor(0x030C, 0x0004);
		write_cmos_sensor(0x030E, 0x007A);
		write_cmos_sensor(0x030A, 0x0001);
		write_cmos_sensor(0x0308, 0x0008);
		write_cmos_sensor(0x0344, 0x0008);
		write_cmos_sensor(0x0346, 0x0008);
		write_cmos_sensor(0x0348, 0x1227);
		write_cmos_sensor(0x034A, 0x0DA7);
		write_cmos_sensor(0x034C, 0x1220);
		write_cmos_sensor(0x034E, 0x0DA0);
		write_cmos_sensor(0x0408, 0x0000);
		write_cmos_sensor(0x0900, 0x0011);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0001);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0001);
		write_cmos_sensor(0x0400, 0x0000);
		write_cmos_sensor(0x0404, 0x0010);
		write_cmos_sensor(0x0342, 0x1400);
		write_cmos_sensor(0x0340, 0x0E3B);
		write_cmos_sensor(0x0B0E, 0x0100);
		write_cmos_sensor(0x0216, 0x0000);
		write_cmos_sensor(0x3604, 0x0002);
		write_cmos_sensor(0x3664, 0x0019);
		write_cmos_sensor(0x3004, 0x0003);
		write_cmos_sensor(0x3000, 0x0001);
		write_cmos_sensor(0x317A, 0x0130);
		write_cmos_sensor(0x1006, 0x0002);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x19E0);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x602A, 0x18F6);
		write_cmos_sensor(0x6F12, 0x002F);
		write_cmos_sensor(0x6F12, 0x002F);

		write_cmos_sensor(0x31A4, 0x0102);
		write_cmos_sensor_8(0x0100, 0x01);
		}
}

static void hs_video_setting(void)
{
	LOG_INF("hs_video_setting() E\n");

	write_cmos_sensor_8(0x0100, 0x00);
	check_stremoff(imgsensor_info.hs_video.max_framerate);
	if (chip_id == 0xA0) {
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x6214, 0x7971);
		write_cmos_sensor(0x6218, 0x7150);
		write_cmos_sensor(0x3004, 0x0004);
		write_cmos_sensor(0x3000, 0x0000);
		write_cmos_sensor(0xF444, 0x0000);
		write_cmos_sensor(0x3604, 0x0001);
		write_cmos_sensor(0x3058, 0x0000);
		write_cmos_sensor(0x0306, 0x008C);
		write_cmos_sensor(0x0300, 0x0004);
		write_cmos_sensor(0x030E, 0x007A);
		write_cmos_sensor(0x0344, 0x0198);
		write_cmos_sensor(0x0348, 0x1097);
		write_cmos_sensor(0x0346, 0x02A0);
		write_cmos_sensor(0x034A, 0x0B0F);
		write_cmos_sensor(0x034C, 0x0500);
		write_cmos_sensor(0x034E, 0x02D0);
		write_cmos_sensor(0x0342, 0x12E0);
		write_cmos_sensor(0x0340, 0x03C5);
		write_cmos_sensor(0x0216, 0x0000);
		write_cmos_sensor(0x3664, 0x1019);
		write_cmos_sensor(0x1006, 0x0003);
		write_cmos_sensor(0x0900, 0x0133);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0005);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0005);
		write_cmos_sensor(0x0400, 0x0000);
		write_cmos_sensor(0x0404, 0x0010);
		write_cmos_sensor(0x0B0E, 0x0100);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x19E0);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x317A, 0xFFFF);
		write_cmos_sensor(0x0100, 0x0100);
	} else{
	    write_cmos_sensor(0x6028, 0x4000);
	    write_cmos_sensor(0x0136, 0x1800);
	    write_cmos_sensor(0x0304, 0x0006);
	    write_cmos_sensor(0x0306, 0x008C);
	    write_cmos_sensor(0x0302, 0x0001);
	    write_cmos_sensor(0x0300, 0x0004);
	    write_cmos_sensor(0x030C, 0x0004);
	    write_cmos_sensor(0x030E, 0x007A);
	    write_cmos_sensor(0x030A, 0x0001);
	    write_cmos_sensor(0x0308, 0x0008);
	    write_cmos_sensor(0x0344, 0x0198);
	    write_cmos_sensor(0x0346, 0x02A0);
	    write_cmos_sensor(0x0348, 0x1097);
	    write_cmos_sensor(0x034A, 0x0B0F);
	    write_cmos_sensor(0x034C, 0x0500);
	    write_cmos_sensor(0x034E, 0x02D0);
	    write_cmos_sensor(0x0408, 0x0000);
	    write_cmos_sensor(0x0900, 0x0113);
	    write_cmos_sensor(0x0380, 0x0001);
	    write_cmos_sensor(0x0382, 0x0001);
	    write_cmos_sensor(0x0384, 0x0001);
	    write_cmos_sensor(0x0386, 0x0005);
	    write_cmos_sensor(0x0400, 0x0001);
	    write_cmos_sensor(0x0404, 0x0030);
	    write_cmos_sensor(0x0342, 0x12E0);
	    write_cmos_sensor(0x0340, 0x03C5);
	    write_cmos_sensor(0x0B0E, 0x0100);
	    write_cmos_sensor(0x0216, 0x0000);
	    write_cmos_sensor(0x3604, 0x0001);
	    write_cmos_sensor(0x3664, 0x0011);
	    write_cmos_sensor(0x3004, 0x0004);
	    write_cmos_sensor(0x3000, 0x0000);
	    write_cmos_sensor(0x317A, 0x0007);
	    write_cmos_sensor(0x1006, 0x0003);
	    write_cmos_sensor(0x6028, 0x2000);
	    write_cmos_sensor(0x602A, 0x19E0);
	    write_cmos_sensor(0x6F12, 0x0001);

	    write_cmos_sensor(0x602A, 0x18F6);
	    write_cmos_sensor(0x6F12, 0x00AF);
	    write_cmos_sensor(0x6F12, 0x00AF);

	    write_cmos_sensor(0x31A4, 0x0102);
	    write_cmos_sensor_8(0x0100, 0x01);
	}
}

static void slim_video_setting(void)
{
	LOG_INF("slim_video_setting() E\n");

	write_cmos_sensor_8(0x0100, 0x00);
	check_stremoff(imgsensor_info.slim_video.max_framerate);
	if (chip_id == 0xA0)	{
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x6214, 0x7971);
		write_cmos_sensor(0x6218, 0x7150);
		write_cmos_sensor(0x3004, 0x0003);
		write_cmos_sensor(0x3000, 0x0001);
		write_cmos_sensor(0xF444, 0x8000);
		write_cmos_sensor(0x3604, 0x0002);
		write_cmos_sensor(0x3058, 0x0001);
		write_cmos_sensor(0x0306, 0x0069);
		write_cmos_sensor(0x0300, 0x0003);
		write_cmos_sensor(0x030E, 0x007A);
		write_cmos_sensor(0x0344, 0x0198);
		write_cmos_sensor(0x0348, 0x1097);
		write_cmos_sensor(0x0346, 0x02A0);
		write_cmos_sensor(0x034A, 0x0B0F);
		write_cmos_sensor(0x034C, 0x0780);
		write_cmos_sensor(0x034E, 0x0438);
		write_cmos_sensor(0x0342, 0x1400);
		write_cmos_sensor(0x0340, 0x0E3C);
		write_cmos_sensor(0x0216, 0x0000);
		write_cmos_sensor(0x3664, 0x0019);
		write_cmos_sensor(0x1006, 0x0002);
		write_cmos_sensor(0x0900, 0x0122);
		write_cmos_sensor(0x0380, 0x0001);
		write_cmos_sensor(0x0382, 0x0003);
		write_cmos_sensor(0x0384, 0x0001);
		write_cmos_sensor(0x0386, 0x0003);
		write_cmos_sensor(0x0400, 0x0000);
		write_cmos_sensor(0x0404, 0x0010);
		write_cmos_sensor(0x0B0E, 0x0100);
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x19E0);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x317A, 0xFFFF);
		write_cmos_sensor(0x0100, 0x0100);
	} else{
	    write_cmos_sensor(0x6028, 0x4000);
	    write_cmos_sensor(0x0136, 0x1800);
	    write_cmos_sensor(0x0304, 0x0006);
	    write_cmos_sensor(0x0306, 0x0069);
	    write_cmos_sensor(0x0302, 0x0001);
	    write_cmos_sensor(0x0300, 0x0003);
	    write_cmos_sensor(0x030C, 0x0004);
	    write_cmos_sensor(0x030E, 0x007A);
	    write_cmos_sensor(0x030A, 0x0001);
	    write_cmos_sensor(0x0308, 0x0008);
	    write_cmos_sensor(0x0344, 0x0198);
	    write_cmos_sensor(0x0346, 0x02A0);
	    write_cmos_sensor(0x0348, 0x1097);
	    write_cmos_sensor(0x034A, 0x0B0F);
	    write_cmos_sensor(0x034C, 0x0780);
	    write_cmos_sensor(0x034E, 0x0438);
	    write_cmos_sensor(0x0408, 0x0000);
	    write_cmos_sensor(0x0900, 0x0112);
	    write_cmos_sensor(0x0380, 0x0001);
	    write_cmos_sensor(0x0382, 0x0001);
	    write_cmos_sensor(0x0384, 0x0001);
	    write_cmos_sensor(0x0386, 0x0003);
	    write_cmos_sensor(0x0400, 0x0001);
	    write_cmos_sensor(0x0404, 0x0020);
	    write_cmos_sensor(0x0342, 0x1400);
	    write_cmos_sensor(0x0340, 0x0E3C);
	    write_cmos_sensor(0x0B0E, 0x0100);
	    write_cmos_sensor(0x0216, 0x0000);
	    write_cmos_sensor(0x3604, 0x0002);
	    write_cmos_sensor(0x3664, 0x0019);
	    write_cmos_sensor(0x3004, 0x0003);
	    write_cmos_sensor(0x3000, 0x0001);
	    write_cmos_sensor(0x317A, 0x00A0);
	    write_cmos_sensor(0x1006, 0x0002);
	    write_cmos_sensor(0x6028, 0x2000);
	    write_cmos_sensor(0x602A, 0x19E0);
	    write_cmos_sensor(0x6F12, 0x0001);

	    write_cmos_sensor(0x602A, 0x18F6);
	    write_cmos_sensor(0x6F12, 0x002F);
	    write_cmos_sensor(0x6F12, 0x002F);

	    write_cmos_sensor(0x31A4, 0x0102);
	    write_cmos_sensor_8(0x0100, 0x01);
	}


}


static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));
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
	kal_uint16 sp8spFlag = 0;

    /* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
	spin_unlock(&imgsensor_drv_lock);
	do {
		*sensor_id = return_sensor_id();
		if (*sensor_id == imgsensor_info.sensor_id) {
			LOG_INF("Get s5k3p8 sensor success!i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);

			sp8spFlag = (((read_cmos_sensor(0x000C) & 0xFF) << 8)
						|((read_cmos_sensor(0x000E) >> 8) & 0xFF));
			LOG_INF("sp8Flag(0x%x),0x5003 used by s5k3p8sp\n", sp8spFlag);

			if (sp8spFlag == 0x5003) {
				LOG_INF("it is s5k3p8sp\n");
				*sensor_id = 0xFFFFFFFF;
				return ERROR_SENSOR_CONNECT_FAIL;
			}
			LOG_INF("3p8 type is 0x(%x),0x000C(0x%x),0x000E(0x%x)\n", sp8spFlag,
				read_cmos_sensor(0x000C), read_cmos_sensor(0x000E));
			*sensor_id = 0x3109;
			return ERROR_NONE;

		}
		LOG_INF("Read sensor id fail, addr: 0x%x, sensor_id:0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
		retry--;
	} while (retry > 0);
	i++;
	retry = 1;
	}
	if  (*sensor_id != imgsensor_info.sensor_id) {
	/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
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
    /* const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2}; */
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;

	LOG_1;
/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF(" i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 1;
	}
	if (imgsensor_info.sensor_id != sensor_id)
	return ERROR_SENSOR_CONNECT_FAIL;

/* initail sequence write in  */
	sensor_init();

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en = KAL_FALSE;
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
	LOG_INF("%s.\n", __func__);

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
	LOG_INF("%s.\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	/* imgsensor.video_mode = KAL_FALSE; */
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
	LOG_INF("%s.\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}  else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
					imgsensor.current_fps, imgsensor_info.cap.max_framerate/10);
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
}	/* capture() */

static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s.\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s.\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
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
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s.\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
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
}

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("%s.\n", __func__);
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

	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);


	/* sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; // not use */
	/* sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; // not use */
	/* imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate; // not use */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; /* inverse with datasheet */
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

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;
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
	sensor_info->SensorWidthSampling = 0;  /* 0 is default 1x */
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorPacketECCOrder = 1;
	sensor_info->PDAF_Support = 0;

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
}	/*	get_info  */

static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(0x0100, 0X01);
	else
		write_cmos_sensor(0x0100, 0x00);
	return ERROR_NONE;
}

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
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)
		/* Dynamic frame rate */
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
	if (enable) /* enable auto flicker */
		imgsensor.autoflicker_en = KAL_TRUE;
	else /* Cancel Auto flick */
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
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength)
						? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);

		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;

	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
		return ERROR_NONE;
	frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength)
						? (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;

	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
			frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength)
						? (frame_length - imgsensor_info.cap1.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		} else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
			frame_length = imgsensor_info.cap2.pclk / framerate * 10 / imgsensor_info.cap2.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap2.framelength)
							? (frame_length - imgsensor_info.cap2.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap2.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		} else {
			if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
					framerate, imgsensor_info.cap.max_framerate/10);
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength)
							? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		}
		break;

	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength)
					? (frame_length - imgsensor_info.hs_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;

	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength)
					? (frame_length - imgsensor_info.slim_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;

	default:
		/* coding with  preview scenario by default */
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength)
					? (frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		LOG_INF("error scenario_id = %d, we use preview scenario\n", scenario_id);
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
	LOG_INF("%s, enable: %d\n", __func__, enable);

	if (enable)
		write_cmos_sensor(0x0600, 0x02);
	else
		write_cmos_sensor(0x0600, 0x00);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
	UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	SET_PD_BLOCK_INFO_T *PDAFinfo;
	SENSOR_VC_INFO_STRUCT *pvcinfo;
	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

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
		night_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16)*feature_data);
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
		/* get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE */
		/* if EEPROM does not exist in camera module. */
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
		set_auto_flicker_mode((BOOL)*feature_data_16, *(feature_data_16+1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*(feature_data),
		(MUINT32 *)(uintptr_t)(*(feature_data+1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: /* for factory mode auto testing */
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
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
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[1], sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[2], sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[3], sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[4], sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;

		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0], sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;

	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		break;
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
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n", (UINT16)*feature_data);
		PDAFinfo = (SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG: /* full */
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW: /* 2x2 binning */
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info, sizeof(SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
		pvcinfo = (SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2], sizeof(SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1], sizeof(SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0], sizeof(SENSOR_VC_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n", (UINT16)*feature_data);
		/* PDAF capacity enable or not */
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0; /* need to check */
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
		/*read_3P8_eeprom((kal_uint16 )(*feature_data), (char*)(uintptr_t)(*(feature_data+1)),*/
		/*(kal_uint32)(*(feature_data+2)));*/
		LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA success\n");
		break;
	case SENSOR_FEATURE_SET_PDAF:
		LOG_INF("PDAF mode :%d\n", *feature_data_16);
		imgsensor.pdaf_mode = *feature_data_16;
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


UINT32 S5K3P8_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}	/*	S5K3P8_MIPI_RAW_SensorInit	*/
