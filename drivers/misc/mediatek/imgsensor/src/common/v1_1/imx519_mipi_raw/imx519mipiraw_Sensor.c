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
 *	 IMX519mipi_Sensor.c
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

#include "imx519mipiraw_Sensor.h"
/*#include "imx519_eeprom.h"*/

#undef VENDOR_EDIT

/***************Modify Following Strings for Debug**********************/
#define PFX "IMX519_camera_sensor"
#define LOG_1 LOG_INF("IMX519,MIPI 4LANE\n")
/****************************   Modify end	**************************/

#define LOG_INF(format, args...) pr_debug(PFX "[%s] " format, __func__, ##args)

#define USE_BURST_MODE 1
#define FULL_SIZE_60_FPS 1

#ifdef VENDOR_EDIT
/*zhaozhengtao 2016/02/19,modify for different module*/
#define MODULE_ID_OFFSET 0x0000
#endif

BYTE imx519_LRC_data[352] = {0};

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = IMX519_SENSOR_ID,

	.checksum_value = 0x1c0140cc,

	.pre = {
		.pclk = 500000000,
		.linelength = 5920,
		.framelength = 2815,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2328, /* 0x918 */
		.grabwindow_height = 1728, /* 0x6c0 */
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.mipi_pixel_rate = 226290000,
		.max_framerate = 300, /* 30fps */
	},

	#if FULL_SIZE_60_FPS
	.cap = {
		.pclk = 1384000000,
		.linelength = 6400,
		.framelength = 3604,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656, /* 0x1230 */
		.grabwindow_height = 3496, /* 0xda8 */
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 1018110000,
		.max_framerate = 600,
	},
	#else
	.cap = {
		.pclk = 700000000,
		.linelength = 6400,
		.framelength = 3645,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 3496,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 672690000,
		.max_framerate = 300,
	},
	#endif
	.normal_video = {
		.pclk = 528000000,
		.linelength = 6400,
		.framelength = 2750,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 2618,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 417600000,
		.max_framerate = 300,
	},


	.hs_video = {
		.pclk = 1000000000,
		.linelength = 5920,
		.framelength = 1407,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 392910000,
		.max_framerate = 1200,
	},

	.custom1 = {
		.pclk = 556000000,
		.linelength = 6400,
		.framelength = 3619,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 3496,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 440230000,
		.max_framerate = 240,
	},

	.custom2 = {
		.pclk = 508000000,
		.linelength = 8320,
		.framelength = 2544,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2488,
		.mipi_pixel_rate = 221140000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 240,
	},

	.margin = 4,		/* sensor framelength & shutter margin */
	.min_shutter = 1,	/* min shutter */
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,	/* isp gain delay frame for AE cycle */
	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.sensor_mode_num = 6,	/* support sensor mode num */

	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,	/* enter slim video delay frame num */
	.custom1_delay_frame = 2,	/* enter custom1 delay frame num */
	.custom2_delay_frame = 2,	/* enter custom2 delay frame num */

	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	/* .mipi_sensor_type = MIPI_OPHY_NCSI2, */
	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_sensor_type = MIPI_CPHY, /* 0,MIPI_OPHY_NCSI2; 1,MIPI_OPHY_CSI2 */
	.mipi_settle_delay_mode = 0,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	.mclk = 24, /* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	/*.mipi_lane_num = SENSOR_MIPI_4_LANE,*/
	.mipi_lane_num = SENSOR_MIPI_3_LANE,
	.i2c_addr_table = {0x34, 0x34, 0xff},
	/* record sensor support all write id addr,
	 * only supprt 4 must end with 0xff
	 */
	.i2c_speed = 400, /* i2c read/write speed */
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,	/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	/* IMGSENSOR_MODE enum value,record current sensor mode,such as:
	 * INIT, Preview, Capture, Video,High Speed Video, Slim Video
	 */
	.shutter = 0x3D0,	/* current shutter */
	.gain = 0x100,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_mode = 0, /* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x34, /* record current sensor's i2c write id */
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[6] = {
	{4656, 3496, 000, 000, 4656, 3496, 2328, 1728,
	0000, 0000, 2328, 1728, 0, 0, 2328, 1728}, /* Preview */
	{4656, 3496, 000, 000, 4656, 3496, 4656, 3496,
	0000, 0000, 4656, 3496, 0, 0, 4656, 3496}, /* capture */
	{4656, 3496, 000, 440, 4656, 2618, 4656, 2618,
	0000, 0000, 4656, 2618, 0, 0, 4656, 2618}, /* video */
	{4656, 3496, 408, 668, 3840, 2160, 1920, 1080,
	0000, 0000, 1920, 1080, 0, 0, 1920, 1080}, /* hight speed video */
	{4656, 3496, 000, 000, 4656, 3496, 4656, 3496,
	0000, 0000, 4656, 3496, 0, 0, 4656, 3496}, /*  custom1 */
	{4656, 3496, 696, 504, 3264, 2488, 3264, 2488,
	0000, 0000, 3264, 2488, 0, 0, 3264, 2488}, /* custom2 */
};
 /*VC1 for HDR(DT=0X35), VC2 for PDAF(DT=0X36), unit : 10bit */
static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {
	/* Preview mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x0A70, 0x07D8, 0x00, 0x35, 0x0280, 0x0001,
	 0x00, 0x36, 0x0B60, 0x0001, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x14E0, 0x0FB0, 0x00, 0x35, 0x0280, 0x0001,
	 0x00, 0x36, 0x1230, 0x0001, 0x03, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x14E0, 0x0FB0, 0x00, 0x35, 0x0280, 0x0001,
	 0x00, 0x36, 0x1230, 0x0001, 0x03, 0x00, 0x0000, 0x0000}
};

#if 0
/* If mirror flip */
static SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 0,
	.i4OffsetY = 4,
	.i4PitchX = 64,
	.i4PitchY = 64,
	.i4PairNum = 16,
	.i4SubBlkW = 16,
	.i4SubBlkH = 16,
.i4PosL = {{3, 0}, {55, 0}, {19, 4}, {39, 4}, {7, 20}, {51, 20},
	   {23, 24}, {35, 24}, {23, 32}, {35, 32}, {7, 36}, {51, 36},
	   {19, 52}, {39, 52}, {3, 56}, {55, 56} },
.i4PosR = {{3, 4}, {55, 4}, {19, 8}, {39, 8}, {7, 16}, {51, 16},
	   {23, 20}, {35, 20}, {23, 36}, {35, 36}, {7, 40}, {51, 40},
	   {19, 48}, {39, 48}, {3, 52}, {55, 52} },
};
#endif

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};

	iReadRegI2C(pusendcmd, 2, (u8 *)&get_byte, 2, imgsensor.i2c_write_id);
	return ((get_byte<<8)&0xff00) | ((get_byte>>8)&0x00ff);
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
	char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF),
			(char)(para & 0xFF)};

	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void imx519_get_pdaf_reg_setting(MUINT32 regNum, kal_uint16 *regDa)
{
	int i, idx;

	for (i = 0; i < regNum; i++) {
		idx = 2 * i;
		regDa[idx + 1] = read_cmos_sensor_8(regDa[idx]);
		LOG_INF("%x %x", regDa[idx], regDa[idx+1]);
	}
}
static void imx519_set_pdaf_reg_setting(MUINT32 regNum, kal_uint16 *regDa)
{
	int i, idx;

	for (i = 0; i < regNum; i++) {
		idx = 2 * i;
		write_cmos_sensor_8(regDa[idx], regDa[idx + 1]);
		LOG_INF("%x %x", regDa[idx], regDa[idx+1]);
	}
}

static void imx519_apply_LRC(void)
{
	unsigned int Lstart_reg = 0x7520;
	unsigned int Rstart_reg = 0x7568;
	char puSendCmd[144]; /*2byte Laddr + L70 byte + 2byte Raddr + R70 byte*/

	LOG_INF("E");

	/*read_imx519_LRC(imx519_LRC_data);*/

	/* LRC: L:0x7520~0x7565; R:0x7568~0x75AD */
	/* L LRC*/
	puSendCmd[0] = (char)(Lstart_reg >> 8);
	puSendCmd[1] = (char)(Lstart_reg & 0xFF);
	memcpy((void *)&puSendCmd[2], imx519_LRC_data, 70);
/*iBurstWriteReg_multi(puSendCmd, 72, imgsensor.i2c_write_id, 72, 400);*/
	/* imgsensor_info.i2c_speed); */
	/* R LRC*/
	puSendCmd[72] = (char)(Rstart_reg >> 8);
	puSendCmd[73] = (char)(Rstart_reg & 0xFF);
	memcpy((void *)&puSendCmd[74], imx519_LRC_data+70, 70);
/*iBurstWriteReg_multi(puSendCmd, 72, imgsensor.i2c_write_id, 72, 400);*/

	LOG_INF("readback LRC, L1(%d) L70(%d) R1(%d) R70(%d)\n",
		read_cmos_sensor_8(0x7520), read_cmos_sensor_8(0x7565),
		read_cmos_sensor_8(0x7568), read_cmos_sensor_8(0x75AD));
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n",
		imgsensor.dummy_line, imgsensor.dummy_pixel);
	/* return;*/ /* for test */
	write_cmos_sensor_8(0x0104, 0x01);

	write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
	write_cmos_sensor_8(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor_8(0x0343, imgsensor.line_length & 0xFF);

	write_cmos_sensor_8(0x0104, 0x00);

}	/*	set_dummy  */

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

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	/*kal_int16 dummy_line;*/
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable %d\n", framerate,
		min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	if (frame_length >= imgsensor.min_frame_length)
		imgsensor.frame_length = frame_length;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
		imgsensor.dummy_line =
			imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line =
			imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */

static void write_shutter(kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;
	#ifdef VENDOR_EDIT
	/*Yijun.Tan@camera.driver,20180116,add for slow shutter */
	int longexposure_times = 0;
	static int long_exposure_status;
	#endif

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
		write_cmos_sensor_8(0x0104, 0x01);
		write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x0104, 0x00);
		}
	} else {
		/* Extend frame length*/
		write_cmos_sensor_8(0x0104, 0x01);
		write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x0104, 0x00);

		LOG_INF("(else)imgsensor.frame_length = %d\n",
			imgsensor.frame_length);

	}
	#ifdef VENDOR_EDIT
	/*Yijun.Tan@camera.driver,20180116,add for slow shutter */
	while (shutter >= 65535) {
		shutter = shutter / 2;
		longexposure_times += 1;
	}

	if (longexposure_times > 0) {
		LOG_INF("enter long exposure mode, time is %d",
			longexposure_times);
		long_exposure_status = 1;
		imgsensor.frame_length = shutter + 32;
		write_cmos_sensor_8(0x0104, 0x01);
		write_cmos_sensor_8(0x3100, longexposure_times & 0x07);
		write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x0104, 0x00);
	} else if (long_exposure_status == 1) {
		long_exposure_status = 0;
		write_cmos_sensor_8(0x0104, 0x01);
		write_cmos_sensor_8(0x3100, 0x00);
		write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x0104, 0x00);

		LOG_INF("exit long exposure mode");
	}
	#endif
	/* Update Shutter */
	write_cmos_sensor_8(0x0104, 0x01);
	write_cmos_sensor_8(0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(0x0203, shutter  & 0xFF);
	write_cmos_sensor_8(0x0104, 0x00);

	LOG_INF("shutter =%d, framelength =%d\n",
		shutter, imgsensor.frame_length);

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
static void set_shutter(kal_uint32 shutter)
{
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
} /* set_shutter */


/*************************************************************************
 * FUNCTION
 *	set_shutter_frame_length
 *
 * DESCRIPTION
 *	for frame & 3A sync
 *
 *************************************************************************/
static void set_shutter_frame_length(kal_uint16 shutter,
				     kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	/*0x3500, 0x3501, 0x3502 will increase VBLANK to
	 *get exposure larger than frame exposure
	 *AE doesn't update sensor gain at capture mode,
	 *thus extra exposure lines must be updated here.
	 */

	/* OV Recommend Solution */
	/*if shutter bigger than frame_length,
	 *should extend frame length first
	 */
	spin_lock(&imgsensor_drv_lock);
	/* Change frame time */
	dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;

	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter)
			? imgsensor_info.min_shutter : shutter;
	shutter =
	(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin)
		: shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 /
				imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor_8(0x0104, 0x01);
			write_cmos_sensor_8(0x0340,
					imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x0341,
					imgsensor.frame_length & 0xFF);
			write_cmos_sensor_8(0x0104, 0x00);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor_8(0x0104, 0x01);
		write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor_8(0x0104, 0x00);
	}

	/* Update Shutter */
	write_cmos_sensor_8(0x0104, 0x01);
	write_cmos_sensor_8(0x0350, 0x00); /* Disable auto extend */
	write_cmos_sensor_8(0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(0x0203, shutter  & 0xFF);
	write_cmos_sensor_8(0x0104, 0x00);
	LOG_INF(
		"Exit! shutter =%d, framelength =%d/%d, dummy_line=%d, auto_extend=%d\n",
		shutter, imgsensor.frame_length, frame_length,
		dummy_line, read_cmos_sensor(0x0350));

}	/* set_shutter_frame_length */

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	 kal_uint16 reg_gain = 0x0;

	reg_gain = 1024 - (1024*64)/gain;
	return (kal_uint16) reg_gain;
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
	LOG_INF("gain = %d, reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor_8(0x0104, 0x01);
	write_cmos_sensor_8(0x0204, (reg_gain>>8) & 0xFF);
	write_cmos_sensor_8(0x0205, reg_gain & 0xFF);
	write_cmos_sensor_8(0x0104, 0x00);

	return gain;
} /* set_gain */

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
static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n",
		enable);
	if (enable)
		write_cmos_sensor_8(0x0100, 0X01);
	else
		write_cmos_sensor_8(0x0100, 0x00);
	return ERROR_NONE;
}

#if USE_BURST_MODE
#define MULTI_WRITE 1

#if MULTI_WRITE
#define I2C_BUFFER_LEN 765 /* trans# max is 255, each 3 bytes */
#else
#define I2C_BUFFER_LEN 3
#endif
static kal_uint16 imx519_table_write_cmos_sensor(kal_uint16 *para,
						 kal_uint32 len)
{
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	tosend = 0;
	IDX = 0;

	while (len > IDX) {
		addr = para[IDX];

		{
			puSendCmd[tosend++] = (char)(addr >> 8);
			puSendCmd[tosend++] = (char)(addr & 0xFF);
			data = para[IDX + 1];
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;

		}
#if MULTI_WRITE
		/* Write when remain buffer size is less than 3 bytes
		 * or reach end of data
		 */
		if ((I2C_BUFFER_LEN - tosend) < 3
			|| IDX == len || addr != addr_last) {
			iBurstWriteReg_multi(puSendCmd,
						tosend,
						imgsensor.i2c_write_id,
						3,
						imgsensor_info.i2c_speed);
			tosend = 0;
		}
#else
		iWriteRegI2C(puSendCmd, 3, imgsensor.i2c_write_id);
		tosend = 0;

#endif
	}
	return 0;
}


static kal_uint16 imx519_init_setting[] = {
	0x0136, 0x18,
	0x0137, 0x00,
	0x3C7E, 0x01,
	0x3C7F, 0x07,
	0x3020, 0x00,
	0x3E35, 0x01,
	0x3F7F, 0x01,
	0x5609, 0x57,
	0x5613, 0x51,
	0x561F, 0x5E,
	0x5623, 0xD2,
	0x5637, 0x11,
	0x5657, 0x11,
	0x5659, 0x12,
	0x5733, 0x60,
	0x5905, 0x57,
	0x590F, 0x51,
	0x591B, 0x5E,
	0x591F, 0xD2,
	0x5933, 0x11,
	0x5953, 0x11,
	0x5955, 0x12,
	0x5A2F, 0x60,
	0x5A85, 0x57,
	0x5A8F, 0x51,
	0x5A9B, 0x5E,
	0x5A9F, 0xD2,
	0x5AB3, 0x11,
	0x5AD3, 0x11,
	0x5AD5, 0x12,
	0x5BAF, 0x60,
	0x5C15, 0x2A,
	0x5C17, 0x80,
	0x5C19, 0x31,
	0x5C1B, 0x87,
	0x5C25, 0x25,
	0x5C27, 0x7B,
	0x5C29, 0x2A,
	0x5C2B, 0x80,
	0x5C2D, 0x31,
	0x5C2F, 0x87,
	0x5C35, 0x2B,
	0x5C37, 0x81,
	0x5C39, 0x31,
	0x5C3B, 0x87,
	0x5C45, 0x25,
	0x5C47, 0x7B,
	0x5C49, 0x2A,
	0x5C4B, 0x80,
	0x5C4D, 0x31,
	0x5C4F, 0x87,
	0x5C55, 0x2D,
	0x5C57, 0x83,
	0x5C59, 0x32,
	0x5C5B, 0x88,
	0x5C65, 0x29,
	0x5C67, 0x7F,
	0x5C69, 0x2E,
	0x5C6B, 0x84,
	0x5C6D, 0x32,
	0x5C6F, 0x88,
	0x5E69, 0x04,
	0x5E9D, 0x00,
	0x5F18, 0x10,
	0x5F1A, 0x0E,
	0x5F20, 0x12,
	0x5F22, 0x10,
	0x5F24, 0x0E,
	0x5F28, 0x10,
	0x5F2A, 0x0E,
	0x5F30, 0x12,
	0x5F32, 0x10,
	0x5F34, 0x0E,
	0x5F38, 0x0F,
	0x5F39, 0x0D,
	0x5F3C, 0x11,
	0x5F3D, 0x0F,
	0x5F3E, 0x0D,
	0x5F61, 0x07,
	0x5F64, 0x05,
	0x5F67, 0x03,
	0x5F6A, 0x03,
	0x5F6D, 0x07,
	0x5F70, 0x07,
	0x5F73, 0x05,
	0x5F76, 0x02,
	0x5F79, 0x07,
	0x5F7C, 0x07,
	0x5F7F, 0x07,
	0x5F82, 0x07,
	0x5F85, 0x03,
	0x5F88, 0x02,
	0x5F8B, 0x01,
	0x5F8E, 0x01,
	0x5F91, 0x04,
	0x5F94, 0x05,
	0x5F97, 0x02,
	0x5F9D, 0x07,
	0x5FA0, 0x07,
	0x5FA3, 0x07,
	0x5FA6, 0x07,
	0x5FA9, 0x03,
	0x5FAC, 0x01,
	0x5FAF, 0x01,
	0x5FB5, 0x03,
	0x5FB8, 0x02,
	0x5FBB, 0x01,
	0x5FC1, 0x07,
	0x5FC4, 0x07,
	0x5FC7, 0x07,
	0x5FD1, 0x00,
	0x6302, 0x79,
	0x6305, 0x78,
	0x6306, 0xA5,
	0x6308, 0x03,
	0x6309, 0x20,
	0x630B, 0x0A,
	0x630D, 0x48,
	0x630F, 0x06,
	0x6311, 0xA4,
	0x6313, 0x03,
	0x6314, 0x20,
	0x6316, 0x0A,
	0x6317, 0x31,
	0x6318, 0x4A,
	0x631A, 0x06,
	0x631B, 0x40,
	0x631C, 0xA4,
	0x631E, 0x03,
	0x631F, 0x20,
	0x6321, 0x0A,
	0x6323, 0x4A,
	0x6328, 0x80,
	0x6329, 0x01,
	0x632A, 0x30,
	0x632B, 0x02,
	0x632C, 0x20,
	0x632D, 0x02,
	0x632E, 0x30,
	0x6330, 0x60,
	0x6332, 0x90,
	0x6333, 0x01,
	0x6334, 0x30,
	0x6335, 0x02,
	0x6336, 0x20,
	0x6338, 0x80,
	0x633A, 0xA0,
	0x633B, 0x01,
	0x633C, 0x60,
	0x633D, 0x02,
	0x633E, 0x60,
	0x633F, 0x01,
	0x6340, 0x30,
	0x6341, 0x02,
	0x6342, 0x20,
	0x6343, 0x03,
	0x6344, 0x80,
	0x6345, 0x03,
	0x6346, 0x90,
	0x6348, 0xF0,
	0x6349, 0x01,
	0x634A, 0x20,
	0x634B, 0x02,
	0x634C, 0x10,
	0x634D, 0x03,
	0x634E, 0x60,
	0x6350, 0xA0,
	0x6351, 0x01,
	0x6352, 0x60,
	0x6353, 0x02,
	0x6354, 0x50,
	0x6355, 0x02,
	0x6356, 0x60,
	0x6357, 0x01,
	0x6358, 0x30,
	0x6359, 0x02,
	0x635A, 0x30,
	0x635B, 0x03,
	0x635C, 0x90,
	0x635F, 0x01,
	0x6360, 0x10,
	0x6361, 0x01,
	0x6362, 0x40,
	0x6363, 0x02,
	0x6364, 0x50,
	0x6368, 0x70,
	0x636A, 0xA0,
	0x636B, 0x01,
	0x636C, 0x50,
	0x637D, 0xE4,
	0x637E, 0xB4,
	0x638C, 0x8E,
	0x638D, 0x38,
	0x638E, 0xE3,
	0x638F, 0x4C,
	0x6390, 0x30,
	0x6391, 0xC3,
	0x6392, 0xAE,
	0x6393, 0xBA,
	0x6394, 0xEB,
	0x6395, 0x6E,
	0x6396, 0x34,
	0x6397, 0xE3,
	0x6398, 0xCF,
	0x6399, 0x3C,
	0x639A, 0xF3,
	0x639B, 0x0C,
	0x639C, 0x30,
	0x639D, 0xC1,
	0x63B9, 0xA3,
	0x63BA, 0xFE,
	0x7600, 0x01,
	0x79A0, 0x01,
	0x79A1, 0x01,
	0x79A2, 0x01,
	0x79A3, 0x01,
	0x79A4, 0x01,
	0x79A5, 0x20,
	0x79A9, 0x00,
	0x79AA, 0x01,
	0x79AD, 0x00,
	0x79AF, 0x00,
	0x8173, 0x01,
	0x835C, 0x01,
	0x8A74, 0x01,
	0x8C1F, 0x00,
	0x8C27, 0x00,
	0x8C3B, 0x03,
	0x9004, 0x0B,
	0x920C, 0x6A,
	0x920D, 0x22,
	0x920E, 0x6A,
	0x920F, 0x23,
	0x9214, 0x6A,
	0x9215, 0x20,
	0x9216, 0x6A,
	0x9217, 0x21,
	0x9385, 0x3E,
	0x9387, 0x1B,
	0x938D, 0x4D,
	0x938F, 0x43,
	0x9391, 0x1B,
	0x9395, 0x4D,
	0x9397, 0x43,
	0x9399, 0x1B,
	0x939D, 0x3E,
	0x939F, 0x2F,
	0x93A5, 0x43,
	0x93A7, 0x2F,
	0x93A9, 0x2F,
	0x93AD, 0x34,
	0x93AF, 0x2F,
	0x93B5, 0x3E,
	0x93B7, 0x2F,
	0x93BD, 0x4D,
	0x93BF, 0x43,
	0x93C1, 0x2F,
	0x93C5, 0x4D,
	0x93C7, 0x43,
	0x93C9, 0x2F,
	0x974B, 0x02,
	0x995C, 0x8C,
	0x995D, 0x00,
	0x995E, 0x00,
	0x9963, 0x64,
	0x9964, 0x50,
	0xAA0A, 0x26,
	0xAE03, 0x04,
	0xAE04, 0x03,
	0xAE05, 0x03,
	0xBC1C, 0x08,
	0xAA06, 0x3F,
	0xAA07, 0x05,
	0xAA08, 0x04,
	0xAA12, 0x3F,
	0xAA13, 0x04,
	0xAA14, 0x03,
	0xAA1E, 0x12,
	0xAA1F, 0x05,
	0xAA20, 0x04,
	0xAA2A, 0x0D,
	0xAA2B, 0x04,
	0xAA2C, 0x03,
	0xAC19, 0x02,
	0xAC1B, 0x01,
	0xAC1D, 0x01,
	0xAC3C, 0x00,
	0xAC3D, 0x01,
	0xAC3E, 0x00,
	0xAC3F, 0x01,
	0xAC40, 0x00,
	0xAC41, 0x01,
	0xAC61, 0x02,
	0xAC63, 0x01,
	0xAC65, 0x01,
	0xAC84, 0x00,
	0xAC85, 0x01,
	0xAC86, 0x00,
	0xAC87, 0x01,
	0xAC88, 0x00,
	0xAC89, 0x01,
	0x38AC, 0x01,
	0x38AD, 0x01,
	0x38AE, 0x01,
	0x38AF, 0x01,
	0x38B0, 0x01,
	0x38B1, 0x01,
	0x38B2, 0x01,
	0x38B3, 0x01,
};
#if FULL_SIZE_60_FPS
static kal_uint16 imx519_capture_60_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x19,
	0x0343, 0x00,
	0x0340, 0x0E,
	0x0341, 0x14,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0xA7,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0D,
	0x040F, 0xA8,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0D,
	0x034F, 0xA8,
	/*PDAF Area Config Begin*/
	0x38A3, 0x02,
	0x38B4, 0x05,
	0x38B5, 0xBB,
	0x38B6, 0x04,
	0x38B7, 0x2B,
	0x38B8, 0x0C,
	0x38B9, 0x74,
	0x38BA, 0x09,
	0x38BB, 0x7C,
	0x38AC, 0x01,
	0x38AD, 0x00,
	0x38AE, 0x00,
	0x38AF, 0x00,
	0x38B0, 0x00,
	0x38B1, 0x00,
	0x38B2, 0x00,
	0x38B3, 0x00,
	/*PDAF Area Config End*/
	0x0301, 0x06,
	0x0303, 0x02,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x5A,
	0x0309, 0x0A,
	0x030B, 0x01,
	0x030D, 0x04,
	0x030E, 0x01,
	0x030F, 0x19,
	0x0310, 0x01,
	0x0820, 0x13,
	0x0821, 0xC2,
	0x0822, 0x00,
	0x0823, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
	0x3E3B, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3FBC, 0x00,
	0x3C06, 0x00,
	0x3C07, 0x80,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x01,
	0x3F79, 0x54,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3614, 0x00,
	0x3616, 0x0D,
	0x3617, 0x56,
	0xB612, 0x20,
	0xB613, 0x20,
	0xB614, 0x20,
	0xB615, 0x20,
	0xB616, 0x0A,
	0xB617, 0x0A,
	0xB618, 0x20,
	0xB619, 0x20,
	0xB61A, 0x20,
	0xB61B, 0x20,
	0xB61C, 0x0A,
	0xB61D, 0x0A,
	0xB666, 0x30,
	0xB667, 0x30,
	0xB668, 0x30,
	0xB669, 0x30,
	0xB66A, 0x14,
	0xB66B, 0x14,
	0xB66C, 0x20,
	0xB66D, 0x20,
	0xB66E, 0x20,
	0xB66F, 0x20,
	0xB670, 0x10,
	0xB671, 0x10,
	0x3900, 0x00,
	0x3901, 0x00,
	0x3237, 0x00,
	0x30AC, 0x00,
};
#else
static kal_uint16 imx519_capture_30_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x19,
	0x0343, 0x00,
	0x0340, 0x0E,
	0x0341, 0x3D,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0xA7,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0D,
	0x040F, 0xA8,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0D,
	0x034F, 0xA8,
	/*PDAF Area Config Begin*/
	0x38A3, 0x02,
	0x38B4, 0x05,
	0x38B5, 0xBB,
	0x38B6, 0x04,
	0x38B7, 0x2B,
	0x38B8, 0x0C,
	0x38B9, 0x74,
	0x38BA, 0x09,
	0x38BB, 0x7C,
	0x38AC, 0x01,
	0x38AD, 0x00,
	0x38AE, 0x00,
	0x38AF, 0x00,
	0x38B0, 0x00,
	0x38B1, 0x00,
	0x38B2, 0x00,
	0x38B3, 0x00,
	/*PDAF Area Config End*/
	0x0301, 0x06,
	0x0303, 0x04,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x5E,
	0x0309, 0x0A,
	0x030B, 0x02,
	0x030D, 0x04,
	0x030E, 0x01,
	0x030F, 0x47,
	0x0310, 0x01,
	0x0820, 0x0B,
	0x0821, 0x7F,
	0x0822, 0x00,
	0x0823, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
	0x3E3B, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3FBC, 0x00,
	0x3C06, 0x00,
	0x3C07, 0x80,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x01,
	0x3F79, 0x54,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3614, 0x00,
	0x3616, 0x0D,
	0x3617, 0x56,
	0xB612, 0x20,
	0xB613, 0x20,
	0xB614, 0x20,
	0xB615, 0x20,
	0xB616, 0x0A,
	0xB617, 0x0A,
	0xB618, 0x20,
	0xB619, 0x20,
	0xB61A, 0x20,
	0xB61B, 0x20,
	0xB61C, 0x0A,
	0xB61D, 0x0A,
	0xB666, 0x30,
	0xB667, 0x30,
	0xB668, 0x30,
	0xB669, 0x30,
	0xB66A, 0x14,
	0xB66B, 0x14,
	0xB66C, 0x20,
	0xB66D, 0x20,
	0xB66E, 0x20,
	0xB66F, 0x20,
	0xB670, 0x10,
	0xB671, 0x10,
	0x3900, 0x00,
	0x3901, 0x00,
	0x3237, 0x00,
	0x30AC, 0x00,
};

#endif
static kal_uint16 imx519_preview_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x17,
	0x0343, 0x20,
	0x0340, 0x0A,
	0x0341, 0xFF,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x12,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0x95,
	0x0220, 0x00,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x01,
	0x0901, 0x22,
	0x0902, 0x84,
	0x3F4C, 0x05,
	0x3F4D, 0x03,
	/*PDAF Area Config Begin*/
	0x38A3, 0x02,/*0:Fixed (16x12), 1:Fixed (8x6), 2:Flexible*/
	0x38B4, 0x02,
	0x38B5, 0xE2,/*738*/
	0x38B6, 0x02,
	0x38B7, 0x10,/*528*/
	0x38B8, 0x06,
	0x38B9, 0x36,/*1590*/
	0x38BA, 0x04,
	0x38BB, 0xB0,/*1200*/
	0x38AC, 0x01,/* enable and disable Flexible window,total 8*/
	0x38AD, 0x00,
	0x38AE, 0x00,
	0x38AF, 0x00,
	0x38B0, 0x00,
	0x38B1, 0x00,
	0x38B2, 0x00,
	0x38B3, 0x00,
	/*PDAF Area Config End*/
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x09,
	0x040D, 0x18,
	0x040E, 0x06,
	0x040F, 0xC0,
	0x034C, 0x09,
	0x034D, 0x18,
	0x034E, 0x06,
	0x034F, 0xC0,
	0x0301, 0x06,
	0x0303, 0x04,
	0x0305, 0x04,
	0x0306, 0x00,
	0x0307, 0xFA,
	0x0309, 0x0A,
	0x030B, 0x04,
	0x030D, 0x04,
	0x030E, 0x00,
	0x030F, 0xE6,
	0x0310, 0x01,
	0x0820, 0x04,
	0x0821, 0x0B,
	0x0822, 0x00,
	0x0823, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
	0x3E3B, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3FBC, 0x00,
	0x3C06, 0x00,
	0x3C07, 0x00,
	0x3C0A, 0x00,
	0x3C0B, 0xF0,
	0x3F78, 0x01,
	0x3F79, 0x38,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x01,
	0x0225, 0xF4,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
};

static kal_uint16 imx519_custom1_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x19,
	0x0343, 0x00,
	0x0340, 0x0E,
	0x0341, 0x23,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x00,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0D,
	0x034B, 0xA7,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x00,
	0x0409, 0x00,
	0x040A, 0x00,
	0x040B, 0x00,
	0x040C, 0x12,
	0x040D, 0x30,
	0x040E, 0x0D,
	0x040F, 0xA8,
	0x034C, 0x12,
	0x034D, 0x30,
	0x034E, 0x0D,
	0x034F, 0xA8,
	0x38A3, 0x02,
	0x38B4, 0x05,
	0x38B5, 0xBB,
	0x38B6, 0x04,
	0x38B7, 0x2A,
	0x38B8, 0x0C,
	0x38B9, 0x75,
	0x38BA, 0x09,
	0x38BB, 0x7D,
	0x38AC, 0x01,
	0x38AD, 0x00,
	0x38AE, 0x00,
	0x38AF, 0x00,
	0x38B0, 0x00,
	0x38B1, 0x00,
	0x38B2, 0x00,
	0x38B3, 0x00,
	0x0301, 0x06,
	0x0303, 0x04,
	0x0305, 0x04,
	0x0306, 0x01,
	0x0307, 0x16,
	0x0309, 0x0A,
	0x030B, 0x02,
	0x030D, 0x04,
	0x030E, 0x00,
	0x030F, 0xD6,
	0x0310, 0x01,
	0x0820, 0x07,
	0x0821, 0x86,
	0x0822, 0x00,
	0x0823, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
	0x3E3B, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3FBC, 0x00,
	0x3C06, 0x00,
	0x3C07, 0x80,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x01,
	0x3F79, 0x54,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3614, 0x00,
	0x3616, 0x0D,
	0x3617, 0x56,
	0xB612, 0x20,
	0xB613, 0x20,
	0xB614, 0x20,
	0xB615, 0x20,
	0xB616, 0x0A,
	0xB617, 0x0A,
	0xB618, 0x20,
	0xB619, 0x20,
	0xB61A, 0x20,
	0xB61B, 0x20,
	0xB61C, 0x0A,
	0xB61D, 0x0A,
	0xB666, 0x30,
	0xB667, 0x30,
	0xB668, 0x30,
	0xB669, 0x30,
	0xB66A, 0x14,
	0xB66B, 0x14,
	0xB66C, 0x20,
	0xB66D, 0x20,
	0xB66E, 0x20,
	0xB66F, 0x20,
	0xB670, 0x10,
	0xB671, 0x10,
	0x3900, 0x00,
	0x3901, 0x00,
	0x3237, 0x00,
	0x30AC, 0x00,
};

static kal_uint16 imx519_custom2_setting[] = {
	0x0111, 0x03,
	0x0112, 0x0A,
	0x0113, 0x0A,
	0x0114, 0x02,
	0x0342, 0x20,
	0x0343, 0x80,
	0x0340, 0x09,
	0x0341, 0xF0,
	0x0344, 0x00,
	0x0345, 0x00,
	0x0346, 0x02,
	0x0347, 0x00,
	0x0348, 0x12,
	0x0349, 0x2F,
	0x034A, 0x0B,
	0x034B, 0x97,
	0x0220, 0x01,
	0x0221, 0x11,
	0x0222, 0x01,
	0x0900, 0x00,
	0x0901, 0x11,
	0x0902, 0x0A,
	0x3F4C, 0x01,
	0x3F4D, 0x01,
	0x4254, 0x7F,
	0x0401, 0x00,
	0x0404, 0x00,
	0x0405, 0x10,
	0x0408, 0x02,
	0x0409, 0xB8,
	0x040A, 0x00,
	0x040B, 0x04,
	0x040C, 0x0C,
	0x040D, 0xC0,
	0x040E, 0x09,
	0x040F, 0x90,
	0x034C, 0x0C,
	0x034D, 0xC0,
	0x034E, 0x09,
	0x034F, 0x90,
	0x38A3, 0x02,
	0x38B4, 0x05,
	0x38B5, 0xBB,
	0x38B6, 0x04,
	0x38B7, 0x2A,
	0x38B8, 0x0C,
	0x38B9, 0x75,
	0x38BA, 0x09,
	0x38BB, 0x7D,
	0x38AC, 0x01,
	0x38AD, 0x00,
	0x38AE, 0x00,
	0x38AF, 0x00,
	0x38B0, 0x00,
	0x38B1, 0x00,
	0x38B2, 0x00,
	0x38B3, 0x00,
	0x0301, 0x06,
	0x0303, 0x04,
	0x0305, 0x04,
	0x0306, 0x00,
	0x0307, 0xFE,
	0x0309, 0x0A,
	0x030B, 0x04,
	0x030D, 0x04,
	0x030E, 0x00,
	0x030F, 0xD7,
	0x0310, 0x01,
	0x0820, 0x03,
	0x0821, 0xC7,
	0x0822, 0x80,
	0x0823, 0x00,
	0x3E20, 0x01,
	0x3E37, 0x01,
	0x3E3B, 0x00,
	0x0106, 0x00,
	0x0B00, 0x00,
	0x3230, 0x00,
	0x3F14, 0x00,
	0x3F3C, 0x03,
	0x3F0D, 0x0A,
	0x3FBC, 0x00,
	0x3C06, 0x01,
	0x3C07, 0xA1,
	0x3C0A, 0x00,
	0x3C0B, 0x00,
	0x3F78, 0x00,
	0x3F79, 0x00,
	0x3F7C, 0x00,
	0x3F7D, 0x00,
	0x0202, 0x03,
	0x0203, 0xE8,
	0x0224, 0x03,
	0x0225, 0xE8,
	0x0204, 0x00,
	0x0205, 0x00,
	0x0216, 0x00,
	0x0217, 0x00,
	0x020E, 0x01,
	0x020F, 0x00,
	0x0218, 0x01,
	0x0219, 0x00,
	0x3614, 0x00,
	0x3616, 0x0D,
	0x3617, 0x56,
	0xB612, 0x20,
	0xB613, 0x20,
	0xB614, 0x20,
	0xB615, 0x20,
	0xB616, 0x0A,
	0xB617, 0x0A,
	0xB618, 0x20,
	0xB619, 0x20,
	0xB61A, 0x20,
	0xB61B, 0x20,
	0xB61C, 0x0A,
	0xB61D, 0x0A,
	0xB666, 0x30,
	0xB667, 0x30,
	0xB668, 0x30,
	0xB669, 0x30,
	0xB66A, 0x14,
	0xB66B, 0x14,
	0xB66C, 0x20,
	0xB66D, 0x20,
	0xB66E, 0x20,
	0xB66F, 0x20,
	0xB670, 0x10,
	0xB671, 0x10,
	0x3900, 0x01,
	0x3901, 0x01,
	0x3237, 0x00,
	0x30AC, 0x00,
};
#endif

static void sensor_init(void)
{
	LOG_INF("E\n");
	#if USE_BURST_MODE
	imx519_table_write_cmos_sensor(imx519_init_setting,
		sizeof(imx519_init_setting)/sizeof(kal_uint16));
	#else
	/* external clock setting */
	write_cmos_sensor_8(0x0136, 0x18);
	write_cmos_sensor_8(0x0137, 0x00);
	write_cmos_sensor_8(0x3C7E, 0x01);
	write_cmos_sensor_8(0x3C7F, 0x07);
	write_cmos_sensor_8(0x3020, 0x00);
	write_cmos_sensor_8(0x3E35, 0x01);
	write_cmos_sensor_8(0x3F7F, 0x01);
	write_cmos_sensor_8(0x5609, 0x57);
	write_cmos_sensor_8(0x5613, 0x51);
	write_cmos_sensor_8(0x561F, 0x5E);
	write_cmos_sensor_8(0x5623, 0xD2);
	write_cmos_sensor_8(0x5637, 0x11);
	write_cmos_sensor_8(0x5657, 0x11);
	write_cmos_sensor_8(0x5659, 0x12);
	write_cmos_sensor_8(0x5733, 0x60);
	write_cmos_sensor_8(0x5905, 0x57);
	write_cmos_sensor_8(0x590F, 0x51);
	write_cmos_sensor_8(0x591B, 0x5E);
	write_cmos_sensor_8(0x591F, 0xD2);
	write_cmos_sensor_8(0x5933, 0x11);
	write_cmos_sensor_8(0x5953, 0x11);
	write_cmos_sensor_8(0x5955, 0x12);
	write_cmos_sensor_8(0x5A2F, 0x60);
	write_cmos_sensor_8(0x5A85, 0x57);
	write_cmos_sensor_8(0x5A8F, 0x51);
	write_cmos_sensor_8(0x5A9B, 0x5E);
	write_cmos_sensor_8(0x5A9F, 0xD2);
	write_cmos_sensor_8(0x5AB3, 0x11);
	write_cmos_sensor_8(0x5AD3, 0x11);
	write_cmos_sensor_8(0x5AD5, 0x12);
	write_cmos_sensor_8(0x5BAF, 0x60);
	write_cmos_sensor_8(0x5C15, 0x2A);
	write_cmos_sensor_8(0x5C17, 0x80);
	write_cmos_sensor_8(0x5C19, 0x31);
	write_cmos_sensor_8(0x5C1B, 0x87);
	write_cmos_sensor_8(0x5C25, 0x25);
	write_cmos_sensor_8(0x5C27, 0x7B);
	write_cmos_sensor_8(0x5C29, 0x2A);
	write_cmos_sensor_8(0x5C2B, 0x80);
	write_cmos_sensor_8(0x5C2D, 0x31);
	write_cmos_sensor_8(0x5C2F, 0x87);
	write_cmos_sensor_8(0x5C35, 0x2B);
	write_cmos_sensor_8(0x5C37, 0x81);
	write_cmos_sensor_8(0x5C39, 0x31);
	write_cmos_sensor_8(0x5C3B, 0x87);
	write_cmos_sensor_8(0x5C45, 0x25);
	write_cmos_sensor_8(0x5C47, 0x7B);
	write_cmos_sensor_8(0x5C49, 0x2A);
	write_cmos_sensor_8(0x5C4B, 0x80);
	write_cmos_sensor_8(0x5C4D, 0x31);
	write_cmos_sensor_8(0x5C4F, 0x87);
	write_cmos_sensor_8(0x5C55, 0x2D);
	write_cmos_sensor_8(0x5C57, 0x83);
	write_cmos_sensor_8(0x5C59, 0x32);
	write_cmos_sensor_8(0x5C5B, 0x88);
	write_cmos_sensor_8(0x5C65, 0x29);
	write_cmos_sensor_8(0x5C67, 0x7F);
	write_cmos_sensor_8(0x5C69, 0x2E);
	write_cmos_sensor_8(0x5C6B, 0x84);
	write_cmos_sensor_8(0x5C6D, 0x32);
	write_cmos_sensor_8(0x5C6F, 0x88);
	write_cmos_sensor_8(0x5E69, 0x04);
	write_cmos_sensor_8(0x5E9D, 0x00);
	write_cmos_sensor_8(0x5F18, 0x10);
	write_cmos_sensor_8(0x5F1A, 0x0E);
	write_cmos_sensor_8(0x5F20, 0x12);
	write_cmos_sensor_8(0x5F22, 0x10);
	write_cmos_sensor_8(0x5F24, 0x0E);
	write_cmos_sensor_8(0x5F28, 0x10);
	write_cmos_sensor_8(0x5F2A, 0x0E);
	write_cmos_sensor_8(0x5F30, 0x12);
	write_cmos_sensor_8(0x5F32, 0x10);
	write_cmos_sensor_8(0x5F34, 0x0E);
	write_cmos_sensor_8(0x5F38, 0x0F);
	write_cmos_sensor_8(0x5F39, 0x0D);
	write_cmos_sensor_8(0x5F3C, 0x11);
	write_cmos_sensor_8(0x5F3D, 0x0F);
	write_cmos_sensor_8(0x5F3E, 0x0D);
	write_cmos_sensor_8(0x5F61, 0x07);
	write_cmos_sensor_8(0x5F64, 0x05);
	write_cmos_sensor_8(0x5F67, 0x03);
	write_cmos_sensor_8(0x5F6A, 0x03);
	write_cmos_sensor_8(0x5F6D, 0x07);
	write_cmos_sensor_8(0x5F70, 0x07);
	write_cmos_sensor_8(0x5F73, 0x05);
	write_cmos_sensor_8(0x5F76, 0x02);
	write_cmos_sensor_8(0x5F79, 0x07);
	write_cmos_sensor_8(0x5F7C, 0x07);
	write_cmos_sensor_8(0x5F7F, 0x07);
	write_cmos_sensor_8(0x5F82, 0x07);
	write_cmos_sensor_8(0x5F85, 0x03);
	write_cmos_sensor_8(0x5F88, 0x02);
	write_cmos_sensor_8(0x5F8B, 0x01);
	write_cmos_sensor_8(0x5F8E, 0x01);
	write_cmos_sensor_8(0x5F91, 0x04);
	write_cmos_sensor_8(0x5F94, 0x05);
	write_cmos_sensor_8(0x5F97, 0x02);
	write_cmos_sensor_8(0x5F9D, 0x07);
	write_cmos_sensor_8(0x5FA0, 0x07);
	write_cmos_sensor_8(0x5FA3, 0x07);
	write_cmos_sensor_8(0x5FA6, 0x07);
	write_cmos_sensor_8(0x5FA9, 0x03);
	write_cmos_sensor_8(0x5FAC, 0x01);
	write_cmos_sensor_8(0x5FAF, 0x01);
	write_cmos_sensor_8(0x5FB5, 0x03);
	write_cmos_sensor_8(0x5FB8, 0x02);
	write_cmos_sensor_8(0x5FBB, 0x01);
	write_cmos_sensor_8(0x5FC1, 0x07);
	write_cmos_sensor_8(0x5FC4, 0x07);
	write_cmos_sensor_8(0x5FC7, 0x07);
	write_cmos_sensor_8(0x5FD1, 0x00);
	write_cmos_sensor_8(0x6302, 0x79);
	write_cmos_sensor_8(0x6305, 0x78);
	write_cmos_sensor_8(0x6306, 0xA5);
	write_cmos_sensor_8(0x6308, 0x03);
	write_cmos_sensor_8(0x6309, 0x20);
	write_cmos_sensor_8(0x630B, 0x0A);
	write_cmos_sensor_8(0x630D, 0x48);
	write_cmos_sensor_8(0x630F, 0x06);
	write_cmos_sensor_8(0x6311, 0xA4);
	write_cmos_sensor_8(0x6313, 0x03);
	write_cmos_sensor_8(0x6314, 0x20);
	write_cmos_sensor_8(0x6316, 0x0A);
	write_cmos_sensor_8(0x6317, 0x31);
	write_cmos_sensor_8(0x6318, 0x4A);
	write_cmos_sensor_8(0x631A, 0x06);
	write_cmos_sensor_8(0x631B, 0x40);
	write_cmos_sensor_8(0x631C, 0xA4);
	write_cmos_sensor_8(0x631E, 0x03);
	write_cmos_sensor_8(0x631F, 0x20);
	write_cmos_sensor_8(0x6321, 0x0A);
	write_cmos_sensor_8(0x6323, 0x4A);
	write_cmos_sensor_8(0x6328, 0x80);
	write_cmos_sensor_8(0x6329, 0x01);
	write_cmos_sensor_8(0x632A, 0x30);
	write_cmos_sensor_8(0x632B, 0x02);
	write_cmos_sensor_8(0x632C, 0x20);
	write_cmos_sensor_8(0x632D, 0x02);
	write_cmos_sensor_8(0x632E, 0x30);
	write_cmos_sensor_8(0x6330, 0x60);
	write_cmos_sensor_8(0x6332, 0x90);
	write_cmos_sensor_8(0x6333, 0x01);
	write_cmos_sensor_8(0x6334, 0x30);
	write_cmos_sensor_8(0x6335, 0x02);
	write_cmos_sensor_8(0x6336, 0x20);
	write_cmos_sensor_8(0x6338, 0x80);
	write_cmos_sensor_8(0x633A, 0xA0);
	write_cmos_sensor_8(0x633B, 0x01);
	write_cmos_sensor_8(0x633C, 0x60);
	write_cmos_sensor_8(0x633D, 0x02);
	write_cmos_sensor_8(0x633E, 0x60);
	write_cmos_sensor_8(0x633F, 0x01);
	write_cmos_sensor_8(0x6340, 0x30);
	write_cmos_sensor_8(0x6341, 0x02);
	write_cmos_sensor_8(0x6342, 0x20);
	write_cmos_sensor_8(0x6343, 0x03);
	write_cmos_sensor_8(0x6344, 0x80);
	write_cmos_sensor_8(0x6345, 0x03);
	write_cmos_sensor_8(0x6346, 0x90);
	write_cmos_sensor_8(0x6348, 0xF0);
	write_cmos_sensor_8(0x6349, 0x01);
	write_cmos_sensor_8(0x634A, 0x20);
	write_cmos_sensor_8(0x634B, 0x02);
	write_cmos_sensor_8(0x634C, 0x10);
	write_cmos_sensor_8(0x634D, 0x03);
	write_cmos_sensor_8(0x634E, 0x60);
	write_cmos_sensor_8(0x6350, 0xA0);
	write_cmos_sensor_8(0x6351, 0x01);
	write_cmos_sensor_8(0x6352, 0x60);
	write_cmos_sensor_8(0x6353, 0x02);
	write_cmos_sensor_8(0x6354, 0x50);
	write_cmos_sensor_8(0x6355, 0x02);
	write_cmos_sensor_8(0x6356, 0x60);
	write_cmos_sensor_8(0x6357, 0x01);
	write_cmos_sensor_8(0x6358, 0x30);
	write_cmos_sensor_8(0x6359, 0x02);
	write_cmos_sensor_8(0x635A, 0x30);
	write_cmos_sensor_8(0x635B, 0x03);
	write_cmos_sensor_8(0x635C, 0x90);
	write_cmos_sensor_8(0x635F, 0x01);
	write_cmos_sensor_8(0x6360, 0x10);
	write_cmos_sensor_8(0x6361, 0x01);
	write_cmos_sensor_8(0x6362, 0x40);
	write_cmos_sensor_8(0x6363, 0x02);
	write_cmos_sensor_8(0x6364, 0x50);
	write_cmos_sensor_8(0x6368, 0x70);
	write_cmos_sensor_8(0x636A, 0xA0);
	write_cmos_sensor_8(0x636B, 0x01);
	write_cmos_sensor_8(0x636C, 0x50);
	write_cmos_sensor_8(0x637D, 0xE4);
	write_cmos_sensor_8(0x637E, 0xB4);
	write_cmos_sensor_8(0x638C, 0x8E);
	write_cmos_sensor_8(0x638D, 0x38);
	write_cmos_sensor_8(0x638E, 0xE3);
	write_cmos_sensor_8(0x638F, 0x4C);
	write_cmos_sensor_8(0x6390, 0x30);
	write_cmos_sensor_8(0x6391, 0xC3);
	write_cmos_sensor_8(0x6392, 0xAE);
	write_cmos_sensor_8(0x6393, 0xBA);
	write_cmos_sensor_8(0x6394, 0xEB);
	write_cmos_sensor_8(0x6395, 0x6E);
	write_cmos_sensor_8(0x6396, 0x34);
	write_cmos_sensor_8(0x6397, 0xE3);
	write_cmos_sensor_8(0x6398, 0xCF);
	write_cmos_sensor_8(0x6399, 0x3C);
	write_cmos_sensor_8(0x639A, 0xF3);
	write_cmos_sensor_8(0x639B, 0x0C);
	write_cmos_sensor_8(0x639C, 0x30);
	write_cmos_sensor_8(0x639D, 0xC1);
	write_cmos_sensor_8(0x63B9, 0xA3);
	write_cmos_sensor_8(0x63BA, 0xFE);
	write_cmos_sensor_8(0x7600, 0x01);
	write_cmos_sensor_8(0x79A0, 0x01);
	write_cmos_sensor_8(0x79A1, 0x01);
	write_cmos_sensor_8(0x79A2, 0x01);
	write_cmos_sensor_8(0x79A3, 0x01);
	write_cmos_sensor_8(0x79A4, 0x01);
	write_cmos_sensor_8(0x79A5, 0x20);
	write_cmos_sensor_8(0x79A9, 0x00);
	write_cmos_sensor_8(0x79AA, 0x01);
	write_cmos_sensor_8(0x79AD, 0x00);
	write_cmos_sensor_8(0x79AF, 0x00);
	write_cmos_sensor_8(0x8173, 0x01);
	write_cmos_sensor_8(0x835C, 0x01);
	write_cmos_sensor_8(0x8A74, 0x01);
	write_cmos_sensor_8(0x8C1F, 0x00);
	write_cmos_sensor_8(0x8C27, 0x00);
	write_cmos_sensor_8(0x8C3B, 0x03);
	write_cmos_sensor_8(0x9004, 0x0B);
	write_cmos_sensor_8(0x920C, 0x6A);
	write_cmos_sensor_8(0x920D, 0x22);
	write_cmos_sensor_8(0x920E, 0x6A);
	write_cmos_sensor_8(0x920F, 0x23);
	write_cmos_sensor_8(0x9214, 0x6A);
	write_cmos_sensor_8(0x9215, 0x20);
	write_cmos_sensor_8(0x9216, 0x6A);
	write_cmos_sensor_8(0x9217, 0x21);
	write_cmos_sensor_8(0x9385, 0x3E);
	write_cmos_sensor_8(0x9387, 0x1B);
	write_cmos_sensor_8(0x938D, 0x4D);
	write_cmos_sensor_8(0x938F, 0x43);
	write_cmos_sensor_8(0x9391, 0x1B);
	write_cmos_sensor_8(0x9395, 0x4D);
	write_cmos_sensor_8(0x9397, 0x43);
	write_cmos_sensor_8(0x9399, 0x1B);
	write_cmos_sensor_8(0x939D, 0x3E);
	write_cmos_sensor_8(0x939F, 0x2F);
	write_cmos_sensor_8(0x93A5, 0x43);
	write_cmos_sensor_8(0x93A7, 0x2F);
	write_cmos_sensor_8(0x93A9, 0x2F);
	write_cmos_sensor_8(0x93AD, 0x34);
	write_cmos_sensor_8(0x93AF, 0x2F);
	write_cmos_sensor_8(0x93B5, 0x3E);
	write_cmos_sensor_8(0x93B7, 0x2F);
	write_cmos_sensor_8(0x93BD, 0x4D);
	write_cmos_sensor_8(0x93BF, 0x43);
	write_cmos_sensor_8(0x93C1, 0x2F);
	write_cmos_sensor_8(0x93C5, 0x4D);
	write_cmos_sensor_8(0x93C7, 0x43);
	write_cmos_sensor_8(0x93C9, 0x2F);
	write_cmos_sensor_8(0x974B, 0x02);
	write_cmos_sensor_8(0x995C, 0x8C);
	write_cmos_sensor_8(0x995D, 0x00);
	write_cmos_sensor_8(0x995E, 0x00);
	write_cmos_sensor_8(0x9963, 0x64);
	write_cmos_sensor_8(0x9964, 0x50);
	write_cmos_sensor_8(0xAA0A, 0x26);
	write_cmos_sensor_8(0xAE03, 0x04);
	write_cmos_sensor_8(0xAE04, 0x03);
	write_cmos_sensor_8(0xAE05, 0x03);
	write_cmos_sensor_8(0xBC1C, 0x08);
	write_cmos_sensor_8(0xAA06, 0x3F);
	write_cmos_sensor_8(0xAA07, 0x05);
	write_cmos_sensor_8(0xAA08, 0x04);
	write_cmos_sensor_8(0xAA12, 0x3F);
	write_cmos_sensor_8(0xAA13, 0x04);
	write_cmos_sensor_8(0xAA14, 0x03);
	write_cmos_sensor_8(0xAA1E, 0x12);
	write_cmos_sensor_8(0xAA1F, 0x05);
	write_cmos_sensor_8(0xAA20, 0x04);
	write_cmos_sensor_8(0xAA2A, 0x0D);
	write_cmos_sensor_8(0xAA2B, 0x04);
	write_cmos_sensor_8(0xAA2C, 0x03);
	write_cmos_sensor_8(0xAC19, 0x02);
	write_cmos_sensor_8(0xAC1B, 0x01);
	write_cmos_sensor_8(0xAC1D, 0x01);
	write_cmos_sensor_8(0xAC3C, 0x00);
	write_cmos_sensor_8(0xAC3D, 0x01);
	write_cmos_sensor_8(0xAC3E, 0x00);
	write_cmos_sensor_8(0xAC3F, 0x01);
	write_cmos_sensor_8(0xAC40, 0x00);
	write_cmos_sensor_8(0xAC41, 0x01);
	write_cmos_sensor_8(0xAC61, 0x02);
	write_cmos_sensor_8(0xAC63, 0x01);
	write_cmos_sensor_8(0xAC65, 0x01);
	write_cmos_sensor_8(0xAC84, 0x00);
	write_cmos_sensor_8(0xAC85, 0x01);
	write_cmos_sensor_8(0xAC86, 0x00);
	write_cmos_sensor_8(0xAC87, 0x01);
	write_cmos_sensor_8(0xAC88, 0x00);
	write_cmos_sensor_8(0xAC89, 0x01);

	write_cmos_sensor_8(0x38AC, 0x01);
	write_cmos_sensor_8(0x38AD, 0x01);
	write_cmos_sensor_8(0x38AE, 0x01);
	write_cmos_sensor_8(0x38AF, 0x01);
	write_cmos_sensor_8(0x38B0, 0x01);
	write_cmos_sensor_8(0x38B1, 0x01);
	write_cmos_sensor_8(0x38B2, 0x01);
	write_cmos_sensor_8(0x38B3, 0x01);
	#endif
	LOG_INF("X");
}	/*	  sensor_init  */

static void preview_setting(void)
{
	LOG_INF("E\n");
	#if USE_BURST_MODE
	imx519_table_write_cmos_sensor(imx519_preview_setting,
		sizeof(imx519_preview_setting)/sizeof(kal_uint16));
	#else
	write_cmos_sensor_8(0x0111, 0x03);
	write_cmos_sensor_8(0x0112, 0x0A);
	write_cmos_sensor_8(0x0113, 0x0A);
	write_cmos_sensor_8(0x0114, 0x02);
	write_cmos_sensor_8(0x0342, 0x17);
	write_cmos_sensor_8(0x0343, 0x20);

	write_cmos_sensor_8(0x0340, 0x0A);
	write_cmos_sensor_8(0x0341, 0xFF);

	write_cmos_sensor_8(0x0344, 0x00);
	write_cmos_sensor_8(0x0345, 0x00);
	write_cmos_sensor_8(0x0346, 0x00);
	write_cmos_sensor_8(0x0347, 0x12);
	write_cmos_sensor_8(0x0348, 0x12);
	write_cmos_sensor_8(0x0349, 0x2F);
	write_cmos_sensor_8(0x034A, 0x0D);
	write_cmos_sensor_8(0x034B, 0x95);

	write_cmos_sensor_8(0x0220, 0x00);
	write_cmos_sensor_8(0x0221, 0x11);
	write_cmos_sensor_8(0x0222, 0x01);
	write_cmos_sensor_8(0x0900, 0x01);
	write_cmos_sensor_8(0x0901, 0x22);
	write_cmos_sensor_8(0x0902, 0x84);
	write_cmos_sensor_8(0x3F4C, 0x05);
	write_cmos_sensor_8(0x3F4D, 0x03);
	write_cmos_sensor_8(0x4254, 0x7F);

	write_cmos_sensor_8(0x0401, 0x00);
	write_cmos_sensor_8(0x0404, 0x00);
	write_cmos_sensor_8(0x0405, 0x10);
	write_cmos_sensor_8(0x0408, 0x00);
	write_cmos_sensor_8(0x0409, 0x00);
	write_cmos_sensor_8(0x040A, 0x00);
	write_cmos_sensor_8(0x040B, 0x00);
	write_cmos_sensor_8(0x040C, 0x09);
	write_cmos_sensor_8(0x040D, 0x18);
	write_cmos_sensor_8(0x040E, 0x06);
	write_cmos_sensor_8(0x040F, 0xC0);

	write_cmos_sensor_8(0x034C, 0x09);
	write_cmos_sensor_8(0x034D, 0x18);
	write_cmos_sensor_8(0x034E, 0x06);
	write_cmos_sensor_8(0x034F, 0xC0);

	/* PDAF Area Config Begin */
	write_cmos_sensor_8(0x38A3, 0x02);
	/* 0:Fixed (16x12), 1:Fixed (8x6), 2:Flexible */

	write_cmos_sensor_8(0x38B4, 0x02);
	write_cmos_sensor_8(0x38B5, 0xE2);/* 738 */
	write_cmos_sensor_8(0x38B6, 0x02);
	write_cmos_sensor_8(0x38B7, 0x10);/* 528 */
	write_cmos_sensor_8(0x38B8, 0x06);
	write_cmos_sensor_8(0x38B9, 0x36);/* 1590 */
	write_cmos_sensor_8(0x38BA, 0x04);
	write_cmos_sensor_8(0x38BB, 0xB0);/* 1200 */

	write_cmos_sensor_8(0x38AC, 0x01);
	/* enable and disable Flexible window,total 8 */

	write_cmos_sensor_8(0x38AD, 0x00);
	write_cmos_sensor_8(0x38AE, 0x00);
	write_cmos_sensor_8(0x38AF, 0x00);
	write_cmos_sensor_8(0x38B0, 0x00);
	write_cmos_sensor_8(0x38B1, 0x00);
	write_cmos_sensor_8(0x38B2, 0x00);
	write_cmos_sensor_8(0x38B3, 0x00);
	/*PDAF Area Config End*/

	write_cmos_sensor_8(0x0301, 0x06);
	write_cmos_sensor_8(0x0303, 0x04);
	write_cmos_sensor_8(0x0305, 0x04);
	write_cmos_sensor_8(0x0306, 0x00);
	write_cmos_sensor_8(0x0307, 0xFA);
	write_cmos_sensor_8(0x0309, 0x0A);
	write_cmos_sensor_8(0x030B, 0x04);
	write_cmos_sensor_8(0x030D, 0x04);
	write_cmos_sensor_8(0x030E, 0x00);
	write_cmos_sensor_8(0x030F, 0xE6);
	write_cmos_sensor_8(0x0310, 0x01);
	write_cmos_sensor_8(0x0820, 0x04);
	write_cmos_sensor_8(0x0821, 0x0B);
	write_cmos_sensor_8(0x0822, 0x00);
	write_cmos_sensor_8(0x0823, 0x00);

	write_cmos_sensor_8(0x3E20, 0x01);
	write_cmos_sensor_8(0x3E37, 0x01);
	write_cmos_sensor_8(0x3E3B, 0x00);

	write_cmos_sensor_8(0x0106, 0x00);
	write_cmos_sensor_8(0x0B00, 0x00);
	write_cmos_sensor_8(0x3230, 0x00);
	write_cmos_sensor_8(0x3F14, 0x00);
	write_cmos_sensor_8(0x3F3C, 0x03);
	write_cmos_sensor_8(0x3F0D, 0x0A);
	write_cmos_sensor_8(0x3FBC, 0x00);
	write_cmos_sensor_8(0x3C06, 0x00);
	write_cmos_sensor_8(0x3C07, 0x00);
	write_cmos_sensor_8(0x3C0A, 0x00);
	write_cmos_sensor_8(0x3C0B, 0xF0);
	write_cmos_sensor_8(0x3F78, 0x01);
	write_cmos_sensor_8(0x3F79, 0x38);
	write_cmos_sensor_8(0x3F7C, 0x00);
	write_cmos_sensor_8(0x3F7D, 0x00);

	write_cmos_sensor_8(0x0202, 0x03);
	write_cmos_sensor_8(0x0203, 0xE8);
	write_cmos_sensor_8(0x0224, 0x01);
	write_cmos_sensor_8(0x0225, 0xF4);

	write_cmos_sensor_8(0x0204, 0x00);
	write_cmos_sensor_8(0x0205, 0x00);
	write_cmos_sensor_8(0x0216, 0x00);
	write_cmos_sensor_8(0x0217, 0x00);
	write_cmos_sensor_8(0x020E, 0x01);
	write_cmos_sensor_8(0x020F, 0x00);
	write_cmos_sensor_8(0x0218, 0x01);
	write_cmos_sensor_8(0x0219, 0x00);
	#endif
	LOG_INF("X");
} /* preview_setting */

#if FULL_SIZE_60_FPS
/* full size 60fps */
static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("%s 60 fps E! currefps:%d\n", __func__, currefps);
	#if USE_BURST_MODE
	imx519_table_write_cmos_sensor(imx519_capture_60_setting,
		sizeof(imx519_capture_60_setting)/sizeof(kal_uint16));
	#else
	write_cmos_sensor_8(0x0111, 0x03);
	write_cmos_sensor_8(0x0112, 0x0A);
	write_cmos_sensor_8(0x0113, 0x0A);
	write_cmos_sensor_8(0x0114, 0x02);
	write_cmos_sensor_8(0x0342, 0x19);
	write_cmos_sensor_8(0x0343, 0x00);
	write_cmos_sensor_8(0x0340, 0x0E);
	write_cmos_sensor_8(0x0341, 0x14);
	write_cmos_sensor_8(0x0344, 0x00);
	write_cmos_sensor_8(0x0345, 0x00);
	write_cmos_sensor_8(0x0346, 0x00);
	write_cmos_sensor_8(0x0347, 0x00);
	write_cmos_sensor_8(0x0348, 0x12);
	write_cmos_sensor_8(0x0349, 0x2F);
	write_cmos_sensor_8(0x034A, 0x0D);
	write_cmos_sensor_8(0x034B, 0xA7);
	write_cmos_sensor_8(0x0220, 0x01);
	write_cmos_sensor_8(0x0221, 0x11);
	write_cmos_sensor_8(0x0222, 0x01);
	write_cmos_sensor_8(0x0900, 0x00);
	write_cmos_sensor_8(0x0901, 0x11);
	write_cmos_sensor_8(0x0902, 0x0A);
	write_cmos_sensor_8(0x3F4C, 0x01);
	write_cmos_sensor_8(0x3F4D, 0x01);
	write_cmos_sensor_8(0x4254, 0x7F);
	write_cmos_sensor_8(0x0401, 0x00);
	write_cmos_sensor_8(0x0404, 0x00);
	write_cmos_sensor_8(0x0405, 0x10);
	write_cmos_sensor_8(0x0408, 0x00);
	write_cmos_sensor_8(0x0409, 0x00);
	write_cmos_sensor_8(0x040A, 0x00);
	write_cmos_sensor_8(0x040B, 0x00);
	write_cmos_sensor_8(0x040C, 0x12);
	write_cmos_sensor_8(0x040D, 0x30);
	write_cmos_sensor_8(0x040E, 0x0D);
	write_cmos_sensor_8(0x040F, 0xA8);
	write_cmos_sensor_8(0x034C, 0x12);
	write_cmos_sensor_8(0x034D, 0x30);
	write_cmos_sensor_8(0x034E, 0x0D);
	write_cmos_sensor_8(0x034F, 0xA8);

	write_cmos_sensor_8(0x38A3, 0x02);
	write_cmos_sensor_8(0x38B4, 0x05);
	write_cmos_sensor_8(0x38B5, 0xBB);
	write_cmos_sensor_8(0x38B6, 0x04);
	write_cmos_sensor_8(0x38B7, 0x2B);
	write_cmos_sensor_8(0x38B8, 0x0C);
	write_cmos_sensor_8(0x38B9, 0x74);
	write_cmos_sensor_8(0x38BA, 0x09);
	write_cmos_sensor_8(0x38BB, 0x7C);
	write_cmos_sensor_8(0x38AC, 0x01);
	write_cmos_sensor_8(0x38AD, 0x00);
	write_cmos_sensor_8(0x38AE, 0x00);
	write_cmos_sensor_8(0x38AF, 0x00);
	write_cmos_sensor_8(0x38B0, 0x00);
	write_cmos_sensor_8(0x38B1, 0x00);
	write_cmos_sensor_8(0x38B2, 0x00);
	write_cmos_sensor_8(0x38B3, 0x00);

	write_cmos_sensor_8(0x0301, 0x06);
	write_cmos_sensor_8(0x0303, 0x02);
	write_cmos_sensor_8(0x0305, 0x04);
	write_cmos_sensor_8(0x0306, 0x01);
	write_cmos_sensor_8(0x0307, 0x5A);
	write_cmos_sensor_8(0x0309, 0x0A);
	write_cmos_sensor_8(0x030B, 0x01);
	write_cmos_sensor_8(0x030D, 0x04);
	write_cmos_sensor_8(0x030E, 0x01);
	write_cmos_sensor_8(0x030F, 0x19);
	write_cmos_sensor_8(0x0310, 0x01);
	write_cmos_sensor_8(0x0820, 0x13);
	write_cmos_sensor_8(0x0821, 0xC2);
	write_cmos_sensor_8(0x0822, 0x00);
	write_cmos_sensor_8(0x0823, 0x00);
	write_cmos_sensor_8(0x3E20, 0x01);
	write_cmos_sensor_8(0x3E37, 0x01);
	write_cmos_sensor_8(0x3E3B, 0x00);
	write_cmos_sensor_8(0x0106, 0x00);
	write_cmos_sensor_8(0x0B00, 0x00);
	write_cmos_sensor_8(0x3230, 0x00);
	write_cmos_sensor_8(0x3F14, 0x00);
	write_cmos_sensor_8(0x3F3C, 0x03);
	write_cmos_sensor_8(0x3F0D, 0x0A);
	write_cmos_sensor_8(0x3FBC, 0x00);
	write_cmos_sensor_8(0x3C06, 0x00);
	write_cmos_sensor_8(0x3C07, 0x80);
	write_cmos_sensor_8(0x3C0A, 0x00);
	write_cmos_sensor_8(0x3C0B, 0x00);
	write_cmos_sensor_8(0x3F78, 0x01);
	write_cmos_sensor_8(0x3F79, 0x54);
	write_cmos_sensor_8(0x3F7C, 0x00);
	write_cmos_sensor_8(0x3F7D, 0x00);
	write_cmos_sensor_8(0x0202, 0x03);
	write_cmos_sensor_8(0x0203, 0xE8);
	write_cmos_sensor_8(0x0224, 0x03);
	write_cmos_sensor_8(0x0225, 0xE8);
	write_cmos_sensor_8(0x0204, 0x00);
	write_cmos_sensor_8(0x0205, 0x00);
	write_cmos_sensor_8(0x0216, 0x00);
	write_cmos_sensor_8(0x0217, 0x00);
	write_cmos_sensor_8(0x020E, 0x01);
	write_cmos_sensor_8(0x020F, 0x00);
	write_cmos_sensor_8(0x0218, 0x01);
	write_cmos_sensor_8(0x0219, 0x00);
	write_cmos_sensor_8(0x3614, 0x00);
	write_cmos_sensor_8(0x3616, 0x0D);
	write_cmos_sensor_8(0x3617, 0x56);
	write_cmos_sensor_8(0xB612, 0x20);
	write_cmos_sensor_8(0xB613, 0x20);
	write_cmos_sensor_8(0xB614, 0x20);
	write_cmos_sensor_8(0xB615, 0x20);
	write_cmos_sensor_8(0xB616, 0x0A);
	write_cmos_sensor_8(0xB617, 0x0A);
	write_cmos_sensor_8(0xB618, 0x20);
	write_cmos_sensor_8(0xB619, 0x20);
	write_cmos_sensor_8(0xB61A, 0x20);
	write_cmos_sensor_8(0xB61B, 0x20);
	write_cmos_sensor_8(0xB61C, 0x0A);
	write_cmos_sensor_8(0xB61D, 0x0A);
	write_cmos_sensor_8(0xB666, 0x30);
	write_cmos_sensor_8(0xB667, 0x30);
	write_cmos_sensor_8(0xB668, 0x30);
	write_cmos_sensor_8(0xB669, 0x30);
	write_cmos_sensor_8(0xB66A, 0x14);
	write_cmos_sensor_8(0xB66B, 0x14);
	write_cmos_sensor_8(0xB66C, 0x20);
	write_cmos_sensor_8(0xB66D, 0x20);
	write_cmos_sensor_8(0xB66E, 0x20);
	write_cmos_sensor_8(0xB66F, 0x20);
	write_cmos_sensor_8(0xB670, 0x10);
	write_cmos_sensor_8(0xB671, 0x10);
	write_cmos_sensor_8(0x3900, 0x00);
	write_cmos_sensor_8(0x3901, 0x00);
	write_cmos_sensor_8(0x3237, 0x00);
	write_cmos_sensor_8(0x30AC, 0x00);
	#endif
	LOG_INF("%s(PD 012515) 60 fpsX\n", __func__);
}
#else

/*full size 30fps*/
static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("%s(PD 012515) 30 fps E! currefps:%d\n", __func__, currefps);
	/*************MIPI output setting************/
	#if USE_BURST_MODE
	imx519_table_write_cmos_sensor(imx519_capture_30_setting,
		sizeof(imx519_capture_30_setting)/sizeof(kal_uint16));
	#else
	write_cmos_sensor_8(0x0111, 0x03);
	write_cmos_sensor_8(0x0112, 0x0A);
	write_cmos_sensor_8(0x0113, 0x0A);
	write_cmos_sensor_8(0x0114, 0x02);
	write_cmos_sensor_8(0x0342, 0x19);
	write_cmos_sensor_8(0x0343, 0x00);
	write_cmos_sensor_8(0x0340, 0x0E);
	write_cmos_sensor_8(0x0341, 0x3D);
	write_cmos_sensor_8(0x0344, 0x00);
	write_cmos_sensor_8(0x0345, 0x00);
	write_cmos_sensor_8(0x0346, 0x00);
	write_cmos_sensor_8(0x0347, 0x00);
	write_cmos_sensor_8(0x0348, 0x12);
	write_cmos_sensor_8(0x0349, 0x2F);
	write_cmos_sensor_8(0x034A, 0x0D);
	write_cmos_sensor_8(0x034B, 0xA7);
	write_cmos_sensor_8(0x0220, 0x01);
	write_cmos_sensor_8(0x0221, 0x11);
	write_cmos_sensor_8(0x0222, 0x01);
	write_cmos_sensor_8(0x0900, 0x00);
	write_cmos_sensor_8(0x0901, 0x11);
	write_cmos_sensor_8(0x0902, 0x0A);
	write_cmos_sensor_8(0x3F4C, 0x01);
	write_cmos_sensor_8(0x3F4D, 0x01);
	write_cmos_sensor_8(0x4254, 0x7F);
	write_cmos_sensor_8(0x0401, 0x00);
	write_cmos_sensor_8(0x0404, 0x00);
	write_cmos_sensor_8(0x0405, 0x10);
	write_cmos_sensor_8(0x0408, 0x00);
	write_cmos_sensor_8(0x0409, 0x00);
	write_cmos_sensor_8(0x040A, 0x00);
	write_cmos_sensor_8(0x040B, 0x00);
	write_cmos_sensor_8(0x040C, 0x12);
	write_cmos_sensor_8(0x040D, 0x30);
	write_cmos_sensor_8(0x040E, 0x0D);
	write_cmos_sensor_8(0x040F, 0xA8);
	write_cmos_sensor_8(0x034C, 0x12);
	write_cmos_sensor_8(0x034D, 0x30);
	write_cmos_sensor_8(0x034E, 0x0D);
	write_cmos_sensor_8(0x034F, 0xA8);

	/* PDAF Area Config Begin */
	/* 0:Fixed (16x12), 1:Fixed (8x6), 2:Flexible */
	write_cmos_sensor_8(0x38A3, 0x02);

	write_cmos_sensor_8(0x38B4, 0x05);
	write_cmos_sensor_8(0x38B5, 0xBB); /* 1467 */
	write_cmos_sensor_8(0x38B6, 0x04);
	write_cmos_sensor_8(0x38B7, 0x2B); /* 1067 */
	write_cmos_sensor_8(0x38B8, 0x0C);
	write_cmos_sensor_8(0x38B9, 0x74); /* 3188 */
	write_cmos_sensor_8(0x38BA, 0x09);
	write_cmos_sensor_8(0x38BB, 0x7C); /* 2428 */

	 /* enable and disable Flexible window,total 8 */
	write_cmos_sensor_8(0x38AC, 0x01);
	write_cmos_sensor_8(0x38AD, 0x00);
	write_cmos_sensor_8(0x38AE, 0x00);
	write_cmos_sensor_8(0x38AF, 0x00);
	write_cmos_sensor_8(0x38B0, 0x00);
	write_cmos_sensor_8(0x38B1, 0x00);
	write_cmos_sensor_8(0x38B2, 0x00);
	write_cmos_sensor_8(0x38B3, 0x00);
	/* PDAF Area Config End */

	write_cmos_sensor_8(0x0301, 0x06);
	write_cmos_sensor_8(0x0303, 0x04);
	write_cmos_sensor_8(0x0305, 0x04);
	write_cmos_sensor_8(0x0306, 0x01);
	write_cmos_sensor_8(0x0307, 0x5E);
	write_cmos_sensor_8(0x0309, 0x0A);
	write_cmos_sensor_8(0x030B, 0x02);
	write_cmos_sensor_8(0x030D, 0x04);
	write_cmos_sensor_8(0x030E, 0x01);
	write_cmos_sensor_8(0x030F, 0x47);
	write_cmos_sensor_8(0x0310, 0x01);
	write_cmos_sensor_8(0x0820, 0x0B);
	write_cmos_sensor_8(0x0821, 0x7F);
	write_cmos_sensor_8(0x0822, 0x00);
	write_cmos_sensor_8(0x0823, 0x00);
	write_cmos_sensor_8(0x3E20, 0x01);
	write_cmos_sensor_8(0x3E37, 0x01);
	write_cmos_sensor_8(0x3E3B, 0x00);
	write_cmos_sensor_8(0x0106, 0x00);
	write_cmos_sensor_8(0x0B00, 0x00);
	write_cmos_sensor_8(0x3230, 0x00);
	write_cmos_sensor_8(0x3F14, 0x00);
	write_cmos_sensor_8(0x3F3C, 0x03);
	write_cmos_sensor_8(0x3F0D, 0x0A);
	write_cmos_sensor_8(0x3FBC, 0x00);
	write_cmos_sensor_8(0x3C06, 0x00);
	write_cmos_sensor_8(0x3C07, 0x80);
	write_cmos_sensor_8(0x3C0A, 0x00);
	write_cmos_sensor_8(0x3C0B, 0x00);
	write_cmos_sensor_8(0x3F78, 0x01);
	write_cmos_sensor_8(0x3F79, 0x54);
	write_cmos_sensor_8(0x3F7C, 0x00);
	write_cmos_sensor_8(0x3F7D, 0x00);
	write_cmos_sensor_8(0x0202, 0x03);
	write_cmos_sensor_8(0x0203, 0xE8);
	write_cmos_sensor_8(0x0224, 0x03);
	write_cmos_sensor_8(0x0225, 0xE8);
	write_cmos_sensor_8(0x0204, 0x00);
	write_cmos_sensor_8(0x0205, 0x00);
	write_cmos_sensor_8(0x0216, 0x00);
	write_cmos_sensor_8(0x0217, 0x00);
	write_cmos_sensor_8(0x020E, 0x01);
	write_cmos_sensor_8(0x020F, 0x00);
	write_cmos_sensor_8(0x0218, 0x01);
	write_cmos_sensor_8(0x0219, 0x00);
	write_cmos_sensor_8(0x3614, 0x00);
	write_cmos_sensor_8(0x3616, 0x0D);
	write_cmos_sensor_8(0x3617, 0x56);
	write_cmos_sensor_8(0xB612, 0x20);
	write_cmos_sensor_8(0xB613, 0x20);
	write_cmos_sensor_8(0xB614, 0x20);
	write_cmos_sensor_8(0xB615, 0x20);
	write_cmos_sensor_8(0xB616, 0x0A);
	write_cmos_sensor_8(0xB617, 0x0A);
	write_cmos_sensor_8(0xB618, 0x20);
	write_cmos_sensor_8(0xB619, 0x20);
	write_cmos_sensor_8(0xB61A, 0x20);
	write_cmos_sensor_8(0xB61B, 0x20);
	write_cmos_sensor_8(0xB61C, 0x0A);
	write_cmos_sensor_8(0xB61D, 0x0A);
	write_cmos_sensor_8(0xB666, 0x30);
	write_cmos_sensor_8(0xB667, 0x30);
	write_cmos_sensor_8(0xB668, 0x30);
	write_cmos_sensor_8(0xB669, 0x30);
	write_cmos_sensor_8(0xB66A, 0x14);
	write_cmos_sensor_8(0xB66B, 0x14);
	write_cmos_sensor_8(0xB66C, 0x20);
	write_cmos_sensor_8(0xB66D, 0x20);
	write_cmos_sensor_8(0xB66E, 0x20);
	write_cmos_sensor_8(0xB66F, 0x20);
	write_cmos_sensor_8(0xB670, 0x10);
	write_cmos_sensor_8(0xB671, 0x10);
	write_cmos_sensor_8(0x3900, 0x00);
	write_cmos_sensor_8(0x3901, 0x00);
	write_cmos_sensor_8(0x3237, 0x00);
	write_cmos_sensor_8(0x30AC, 0x00);
	#endif
	LOG_INF("%s(PD 012515) 30 fpsX\n", __func__);
}
#endif

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("%s E! currefps:%d\n", __func__, currefps);
	/*************MIPI output setting************/
	write_cmos_sensor_8(0x0111, 0x03);
	write_cmos_sensor_8(0x0112, 0x0A);
	write_cmos_sensor_8(0x0113, 0x0A);
	write_cmos_sensor_8(0x0114, 0x02);
	write_cmos_sensor_8(0x0342, 0x19);
	write_cmos_sensor_8(0x0343, 0x00);
	write_cmos_sensor_8(0x0340, 0x0A);
	write_cmos_sensor_8(0x0341, 0xBE);
	write_cmos_sensor_8(0x0344, 0x00);
	write_cmos_sensor_8(0x0345, 0x00);
	write_cmos_sensor_8(0x0346, 0x01);
	write_cmos_sensor_8(0x0347, 0xB0);
	write_cmos_sensor_8(0x0348, 0x12);
	write_cmos_sensor_8(0x0349, 0x2F);
	write_cmos_sensor_8(0x034A, 0x0B);
	write_cmos_sensor_8(0x034B, 0xF7);
	write_cmos_sensor_8(0x0220, 0x01);
	write_cmos_sensor_8(0x0221, 0x11);
	write_cmos_sensor_8(0x0222, 0x01);
	write_cmos_sensor_8(0x0900, 0x00);
	write_cmos_sensor_8(0x0901, 0x11);
	write_cmos_sensor_8(0x0902, 0x0A);
	write_cmos_sensor_8(0x3F4C, 0x01);
	write_cmos_sensor_8(0x3F4D, 0x01);
	write_cmos_sensor_8(0x4254, 0x7F);
	write_cmos_sensor_8(0x0401, 0x00);
	write_cmos_sensor_8(0x0404, 0x00);
	write_cmos_sensor_8(0x0405, 0x10);
	write_cmos_sensor_8(0x0408, 0x00);
	write_cmos_sensor_8(0x0409, 0x00);
	write_cmos_sensor_8(0x040A, 0x00);
	write_cmos_sensor_8(0x040B, 0x06);
	write_cmos_sensor_8(0x040C, 0x12);
	write_cmos_sensor_8(0x040D, 0x30);
	write_cmos_sensor_8(0x040E, 0x0A);
	write_cmos_sensor_8(0x040F, 0x3A);
	write_cmos_sensor_8(0x034C, 0x12);
	write_cmos_sensor_8(0x034D, 0x30);
	write_cmos_sensor_8(0x034E, 0x0A);
	write_cmos_sensor_8(0x034F, 0x3A);

	/* PDAF Area Config Begin */
	write_cmos_sensor_8(0x38A3, 0x02);

	write_cmos_sensor_8(0x38B4, 0x05);
	write_cmos_sensor_8(0x38B5, 0xBB);
	write_cmos_sensor_8(0x38B6, 0x03);
	write_cmos_sensor_8(0x38B7, 0x1E);
	write_cmos_sensor_8(0x38B8, 0x0C);
	write_cmos_sensor_8(0x38B9, 0x75);
	write_cmos_sensor_8(0x38BA, 0x07);
	write_cmos_sensor_8(0x38BB, 0x1B);

	write_cmos_sensor_8(0x38AC, 0x01);
	write_cmos_sensor_8(0x38AD, 0x00);
	write_cmos_sensor_8(0x38AE, 0x00);
	write_cmos_sensor_8(0x38AF, 0x00);
	write_cmos_sensor_8(0x38B0, 0x00);
	write_cmos_sensor_8(0x38B1, 0x00);
	write_cmos_sensor_8(0x38B2, 0x00);
	write_cmos_sensor_8(0x38B3, 0x00);
	/* PDAF Area Config End */

	write_cmos_sensor_8(0x0301, 0x06);
	write_cmos_sensor_8(0x0303, 0x04);
	write_cmos_sensor_8(0x0305, 0x04);
	write_cmos_sensor_8(0x0306, 0x01);
	write_cmos_sensor_8(0x0307, 0x08);
	write_cmos_sensor_8(0x0309, 0x0A);
	write_cmos_sensor_8(0x030B, 0x04);
	write_cmos_sensor_8(0x030D, 0x04);
	write_cmos_sensor_8(0x030E, 0x01);
	write_cmos_sensor_8(0x030F, 0x96);
	write_cmos_sensor_8(0x0310, 0x01);
	write_cmos_sensor_8(0x0820, 0x07);
	write_cmos_sensor_8(0x0821, 0x23);
	write_cmos_sensor_8(0x0822, 0x00);
	write_cmos_sensor_8(0x0823, 0x00);
	write_cmos_sensor_8(0x3E20, 0x01);
	write_cmos_sensor_8(0x3E37, 0x01);
	write_cmos_sensor_8(0x3E3B, 0x00);
	write_cmos_sensor_8(0x0106, 0x00);
	write_cmos_sensor_8(0x0B00, 0x00);
	write_cmos_sensor_8(0x3230, 0x00);
	write_cmos_sensor_8(0x3F14, 0x00);
	write_cmos_sensor_8(0x3F3C, 0x03);
	write_cmos_sensor_8(0x3F0D, 0x0A);
	write_cmos_sensor_8(0x3FBC, 0x00);
	write_cmos_sensor_8(0x3C06, 0x00);
	write_cmos_sensor_8(0x3C07, 0x80);
	write_cmos_sensor_8(0x3C0A, 0x00);
	write_cmos_sensor_8(0x3C0B, 0x00);
	write_cmos_sensor_8(0x3F78, 0x01);
	write_cmos_sensor_8(0x3F79, 0x54);
	write_cmos_sensor_8(0x3F7C, 0x00);
	write_cmos_sensor_8(0x3F7D, 0x00);
	write_cmos_sensor_8(0x0202, 0x03);
	write_cmos_sensor_8(0x0203, 0xE8);
	write_cmos_sensor_8(0x0224, 0x03);
	write_cmos_sensor_8(0x0225, 0xE8);
	write_cmos_sensor_8(0x0204, 0x00);
	write_cmos_sensor_8(0x0205, 0x00);
	write_cmos_sensor_8(0x0216, 0x00);
	write_cmos_sensor_8(0x0217, 0x00);
	write_cmos_sensor_8(0x020E, 0x01);
	write_cmos_sensor_8(0x020F, 0x00);
	write_cmos_sensor_8(0x0218, 0x01);
	write_cmos_sensor_8(0x0219, 0x00);
	LOG_INF("X\n");
}

static void hs_video_setting(void)
{
	LOG_INF("%s E! currefps 120\n", __func__);
	/*************MIPI output setting************/
	write_cmos_sensor_8(0x0111, 0x03);
	write_cmos_sensor_8(0x0112, 0x0A);
	write_cmos_sensor_8(0x0113, 0x0A);
	write_cmos_sensor_8(0x0114, 0x02);
	write_cmos_sensor_8(0x0342, 0x17);
	write_cmos_sensor_8(0x0343, 0x20);
	write_cmos_sensor_8(0x0340, 0x05);
	write_cmos_sensor_8(0x0341, 0x7F);
	write_cmos_sensor_8(0x0344, 0x00);
	write_cmos_sensor_8(0x0345, 0x00);
	write_cmos_sensor_8(0x0346, 0x02);
	write_cmos_sensor_8(0x0347, 0x92);
	write_cmos_sensor_8(0x0348, 0x12);
	write_cmos_sensor_8(0x0349, 0x2F);
	write_cmos_sensor_8(0x034A, 0x0B);
	write_cmos_sensor_8(0x034B, 0x05);
	write_cmos_sensor_8(0x0220, 0x00);
	write_cmos_sensor_8(0x0221, 0x11);
	write_cmos_sensor_8(0x0222, 0x01);
	write_cmos_sensor_8(0x0900, 0x01);
	write_cmos_sensor_8(0x0901, 0x22);
	write_cmos_sensor_8(0x0902, 0x08);
	write_cmos_sensor_8(0x3F4C, 0x05);
	write_cmos_sensor_8(0x3F4D, 0x03);
	write_cmos_sensor_8(0x4254, 0x7F);
	write_cmos_sensor_8(0x0401, 0x00);
	write_cmos_sensor_8(0x0404, 0x00);
	write_cmos_sensor_8(0x0405, 0x10);
	write_cmos_sensor_8(0x0408, 0x00);
	write_cmos_sensor_8(0x0409, 0xCC);
	write_cmos_sensor_8(0x040A, 0x00);
	write_cmos_sensor_8(0x040B, 0x00);
	write_cmos_sensor_8(0x040C, 0x07);
	write_cmos_sensor_8(0x040D, 0x80);
	write_cmos_sensor_8(0x040E, 0x04);
	write_cmos_sensor_8(0x040F, 0x38);
	write_cmos_sensor_8(0x034C, 0x07);
	write_cmos_sensor_8(0x034D, 0x80);
	write_cmos_sensor_8(0x034E, 0x04);
	write_cmos_sensor_8(0x034F, 0x38);
	write_cmos_sensor_8(0x0301, 0x06);
	write_cmos_sensor_8(0x0303, 0x02);
	write_cmos_sensor_8(0x0305, 0x04);
	write_cmos_sensor_8(0x0306, 0x00);
	write_cmos_sensor_8(0x0307, 0xFA);
	write_cmos_sensor_8(0x0309, 0x0A);
	write_cmos_sensor_8(0x030B, 0x04);
	write_cmos_sensor_8(0x030D, 0x04);
	write_cmos_sensor_8(0x030E, 0x01);
	write_cmos_sensor_8(0x030F, 0x7E);
	write_cmos_sensor_8(0x0310, 0x01);
	write_cmos_sensor_8(0x0820, 0x06);
	write_cmos_sensor_8(0x0821, 0xB7);
	write_cmos_sensor_8(0x0822, 0x00);
	write_cmos_sensor_8(0x0823, 0x00);
	write_cmos_sensor_8(0x3E20, 0x01);
	write_cmos_sensor_8(0x3E37, 0x01);
	write_cmos_sensor_8(0x3E3B, 0x00);
	write_cmos_sensor_8(0x0106, 0x00);
	write_cmos_sensor_8(0x0B00, 0x00);
	write_cmos_sensor_8(0x3230, 0x00);
	write_cmos_sensor_8(0x3F14, 0x00);
	write_cmos_sensor_8(0x3F3C, 0x03);
	write_cmos_sensor_8(0x3F0D, 0x0A);
	write_cmos_sensor_8(0x3FBC, 0x00);
	write_cmos_sensor_8(0x3C06, 0x00);
	write_cmos_sensor_8(0x3C07, 0x00);
	write_cmos_sensor_8(0x3C0A, 0x00);
	write_cmos_sensor_8(0x3C0B, 0xF0);
	write_cmos_sensor_8(0x3F78, 0x01);
	write_cmos_sensor_8(0x3F79, 0x38);
	write_cmos_sensor_8(0x3F7C, 0x00);
	write_cmos_sensor_8(0x3F7D, 0x00);
	write_cmos_sensor_8(0x0202, 0x03);
	write_cmos_sensor_8(0x0203, 0xE8);
	write_cmos_sensor_8(0x0224, 0x01);
	write_cmos_sensor_8(0x0225, 0xF4);
	write_cmos_sensor_8(0x0204, 0x00);
	write_cmos_sensor_8(0x0205, 0x00);
	write_cmos_sensor_8(0x0216, 0x00);
	write_cmos_sensor_8(0x0217, 0x00);
	write_cmos_sensor_8(0x020E, 0x01);
	write_cmos_sensor_8(0x020F, 0x00);
	write_cmos_sensor_8(0x0218, 0x01);
	write_cmos_sensor_8(0x0219, 0x00);
	LOG_INF("X\n");
}

static void slim_video_setting(void)
{
	LOG_INF("%s E\n", __func__);
}

/*full size 16M@24fps*/
static void custom1_setting(void)
{
	LOG_INF("%s 30 fps E! currefps\n", __func__);
	/*************MIPI output setting************/
	#if USE_BURST_MODE
	imx519_table_write_cmos_sensor(imx519_custom1_setting,
		sizeof(imx519_custom1_setting)/sizeof(kal_uint16));
	#else
	write_cmos_sensor_8(0x0111, 0x03);
	write_cmos_sensor_8(0x0112, 0x0A);
	write_cmos_sensor_8(0x0113, 0x0A);
	write_cmos_sensor_8(0x0114, 0x02);
	write_cmos_sensor_8(0x0342, 0x19);
	write_cmos_sensor_8(0x0343, 0x00);
	write_cmos_sensor_8(0x0340, 0x0E);
	write_cmos_sensor_8(0x0341, 0x23);
	write_cmos_sensor_8(0x0344, 0x00);
	write_cmos_sensor_8(0x0345, 0x00);
	write_cmos_sensor_8(0x0346, 0x00);
	write_cmos_sensor_8(0x0347, 0x00);
	write_cmos_sensor_8(0x0348, 0x12);
	write_cmos_sensor_8(0x0349, 0x2F);
	write_cmos_sensor_8(0x034A, 0x0D);
	write_cmos_sensor_8(0x034B, 0xA7);
	write_cmos_sensor_8(0x0220, 0x01);
	write_cmos_sensor_8(0x0221, 0x11);
	write_cmos_sensor_8(0x0222, 0x01);
	write_cmos_sensor_8(0x0900, 0x00);
	write_cmos_sensor_8(0x0901, 0x11);
	write_cmos_sensor_8(0x0902, 0x0A);
	write_cmos_sensor_8(0x3F4C, 0x01);
	write_cmos_sensor_8(0x3F4D, 0x01);
	write_cmos_sensor_8(0x4254, 0x7F);
	write_cmos_sensor_8(0x0401, 0x00);
	write_cmos_sensor_8(0x0404, 0x00);
	write_cmos_sensor_8(0x0405, 0x10);
	write_cmos_sensor_8(0x0408, 0x00);
	write_cmos_sensor_8(0x0409, 0x00);
	write_cmos_sensor_8(0x040A, 0x00);
	write_cmos_sensor_8(0x040B, 0x00);
	write_cmos_sensor_8(0x040C, 0x12);
	write_cmos_sensor_8(0x040D, 0x30);
	write_cmos_sensor_8(0x040E, 0x0D);
	write_cmos_sensor_8(0x040F, 0xA8);
	write_cmos_sensor_8(0x034C, 0x12);
	write_cmos_sensor_8(0x034D, 0x30);
	write_cmos_sensor_8(0x034E, 0x0D);
	write_cmos_sensor_8(0x034F, 0xA8);

	write_cmos_sensor_8(0x38A3, 0x02); /* 2:PD Flexible Window */
	write_cmos_sensor_8(0x38B4, 0x05);
	write_cmos_sensor_8(0x38B5, 0xBB); /* 1467 */
	write_cmos_sensor_8(0x38B6, 0x04);
	write_cmos_sensor_8(0x38B7, 0x2A); /* 1067 */
	write_cmos_sensor_8(0x38B8, 0x0C);
	write_cmos_sensor_8(0x38B9, 0x75); /* 3189 */
	write_cmos_sensor_8(0x38BA, 0x09);
	write_cmos_sensor_8(0x38BB, 0x7D); /* 2429 */

	write_cmos_sensor_8(0x38AC, 0x01); /* enable and disable windows */
	write_cmos_sensor_8(0x38AD, 0x00);
	write_cmos_sensor_8(0x38AE, 0x00);
	write_cmos_sensor_8(0x38AF, 0x00);
	write_cmos_sensor_8(0x38B0, 0x00);
	write_cmos_sensor_8(0x38B1, 0x00);
	write_cmos_sensor_8(0x38B2, 0x00);
	write_cmos_sensor_8(0x38B3, 0x00);

	write_cmos_sensor_8(0x0301, 0x06);
	write_cmos_sensor_8(0x0303, 0x04);
	write_cmos_sensor_8(0x0305, 0x04);
	write_cmos_sensor_8(0x0306, 0x01);
	write_cmos_sensor_8(0x0307, 0x16);
	write_cmos_sensor_8(0x0309, 0x0A);
	write_cmos_sensor_8(0x030B, 0x02);
	write_cmos_sensor_8(0x030D, 0x04);
	write_cmos_sensor_8(0x030E, 0x00);
	write_cmos_sensor_8(0x030F, 0xD6);
	write_cmos_sensor_8(0x0310, 0x01);
	write_cmos_sensor_8(0x0820, 0x07);
	write_cmos_sensor_8(0x0821, 0x86);
	write_cmos_sensor_8(0x0822, 0x00);
	write_cmos_sensor_8(0x0823, 0x00);
	write_cmos_sensor_8(0x3E20, 0x01);
	write_cmos_sensor_8(0x3E37, 0x01);
	write_cmos_sensor_8(0x3E3B, 0x00);
	write_cmos_sensor_8(0x0106, 0x00);
	write_cmos_sensor_8(0x0B00, 0x00);
	write_cmos_sensor_8(0x3230, 0x00);
	write_cmos_sensor_8(0x3F14, 0x00);
	write_cmos_sensor_8(0x3F3C, 0x03);
	write_cmos_sensor_8(0x3F0D, 0x0A);
	write_cmos_sensor_8(0x3FBC, 0x00);
	write_cmos_sensor_8(0x3C06, 0x00);
	write_cmos_sensor_8(0x3C07, 0x80);
	write_cmos_sensor_8(0x3C0A, 0x00);
	write_cmos_sensor_8(0x3C0B, 0x00);
	write_cmos_sensor_8(0x3F78, 0x01);
	write_cmos_sensor_8(0x3F79, 0x54);
	write_cmos_sensor_8(0x3F7C, 0x00);
	write_cmos_sensor_8(0x3F7D, 0x00);
	write_cmos_sensor_8(0x0202, 0x03);
	write_cmos_sensor_8(0x0203, 0xE8);
	write_cmos_sensor_8(0x0224, 0x03);
	write_cmos_sensor_8(0x0225, 0xE8);
	write_cmos_sensor_8(0x0204, 0x00);
	write_cmos_sensor_8(0x0205, 0x00);
	write_cmos_sensor_8(0x0216, 0x00);
	write_cmos_sensor_8(0x0217, 0x00);
	write_cmos_sensor_8(0x020E, 0x01);
	write_cmos_sensor_8(0x020F, 0x00);
	write_cmos_sensor_8(0x0218, 0x01);
	write_cmos_sensor_8(0x0219, 0x00);
	write_cmos_sensor_8(0x3614, 0x00);
	write_cmos_sensor_8(0x3616, 0x0D);
	write_cmos_sensor_8(0x3617, 0x56);
	write_cmos_sensor_8(0xB612, 0x20);
	write_cmos_sensor_8(0xB613, 0x20);
	write_cmos_sensor_8(0xB614, 0x20);
	write_cmos_sensor_8(0xB615, 0x20);
	write_cmos_sensor_8(0xB616, 0x0A);
	write_cmos_sensor_8(0xB617, 0x0A);
	write_cmos_sensor_8(0xB618, 0x20);
	write_cmos_sensor_8(0xB619, 0x20);
	write_cmos_sensor_8(0xB61A, 0x20);
	write_cmos_sensor_8(0xB61B, 0x20);
	write_cmos_sensor_8(0xB61C, 0x0A);
	write_cmos_sensor_8(0xB61D, 0x0A);
	write_cmos_sensor_8(0xB666, 0x30);
	write_cmos_sensor_8(0xB667, 0x30);
	write_cmos_sensor_8(0xB668, 0x30);
	write_cmos_sensor_8(0xB669, 0x30);
	write_cmos_sensor_8(0xB66A, 0x14);
	write_cmos_sensor_8(0xB66B, 0x14);
	write_cmos_sensor_8(0xB66C, 0x20);
	write_cmos_sensor_8(0xB66D, 0x20);
	write_cmos_sensor_8(0xB66E, 0x20);
	write_cmos_sensor_8(0xB66F, 0x20);
	write_cmos_sensor_8(0xB670, 0x10);
	write_cmos_sensor_8(0xB671, 0x10);
	write_cmos_sensor_8(0x3900, 0x00);
	write_cmos_sensor_8(0x3901, 0x00);
	write_cmos_sensor_8(0x3237, 0x00);
	write_cmos_sensor_8(0x30AC, 0x00);
	#endif
	LOG_INF("X");
}

/*full size 8M@24fps*/
static void custom2_setting(void)
{
	LOG_INF("%s 30 fps E! currefps\n", __func__);
	/*************MIPI output setting************/
	#if USE_BURST_MODE
	imx519_table_write_cmos_sensor(imx519_custom2_setting,
		sizeof(imx519_custom2_setting)/sizeof(kal_uint16));
	#else
	write_cmos_sensor_8(0x0111, 0x03);
	write_cmos_sensor_8(0x0112, 0x0A);
	write_cmos_sensor_8(0x0113, 0x0A);
	write_cmos_sensor_8(0x0114, 0x02);
	write_cmos_sensor_8(0x0342, 0x20);
	write_cmos_sensor_8(0x0343, 0x80);
	write_cmos_sensor_8(0x0340, 0x09);
	write_cmos_sensor_8(0x0341, 0xF0);
	write_cmos_sensor_8(0x0344, 0x00);
	write_cmos_sensor_8(0x0345, 0x00);
	write_cmos_sensor_8(0x0346, 0x02);
	write_cmos_sensor_8(0x0347, 0x00);
	write_cmos_sensor_8(0x0348, 0x12);
	write_cmos_sensor_8(0x0349, 0x2F);
	write_cmos_sensor_8(0x034A, 0x0B);
	write_cmos_sensor_8(0x034B, 0x97);
	write_cmos_sensor_8(0x0220, 0x01);
	write_cmos_sensor_8(0x0221, 0x11);
	write_cmos_sensor_8(0x0222, 0x01);
	write_cmos_sensor_8(0x0900, 0x00);
	write_cmos_sensor_8(0x0901, 0x11);
	write_cmos_sensor_8(0x0902, 0x0A);
	write_cmos_sensor_8(0x3F4C, 0x01);
	write_cmos_sensor_8(0x3F4D, 0x01);
	write_cmos_sensor_8(0x4254, 0x7F);
	write_cmos_sensor_8(0x0401, 0x00);
	write_cmos_sensor_8(0x0404, 0x00);
	write_cmos_sensor_8(0x0405, 0x10);
	write_cmos_sensor_8(0x0408, 0x02);
	write_cmos_sensor_8(0x0409, 0xB8);
	write_cmos_sensor_8(0x040A, 0x00);
	write_cmos_sensor_8(0x040B, 0x04);
	write_cmos_sensor_8(0x040C, 0x0C);
	write_cmos_sensor_8(0x040D, 0xC0);
	write_cmos_sensor_8(0x040E, 0x09);
	write_cmos_sensor_8(0x040F, 0x90);
	write_cmos_sensor_8(0x034C, 0x0C);
	write_cmos_sensor_8(0x034D, 0xC0);
	write_cmos_sensor_8(0x034E, 0x09);
	write_cmos_sensor_8(0x034F, 0x90);

	write_cmos_sensor_8(0x38A3, 0x02);
	write_cmos_sensor_8(0x38B4, 0x05);
	write_cmos_sensor_8(0x38B5, 0xBB);
	write_cmos_sensor_8(0x38B6, 0x04);
	write_cmos_sensor_8(0x38B7, 0x2A);
	write_cmos_sensor_8(0x38B8, 0x0C);
	write_cmos_sensor_8(0x38B9, 0x75);
	write_cmos_sensor_8(0x38BA, 0x09);
	write_cmos_sensor_8(0x38BB, 0x7D);
	write_cmos_sensor_8(0x38AC, 0x01);
	write_cmos_sensor_8(0x38AD, 0x00);
	write_cmos_sensor_8(0x38AE, 0x00);
	write_cmos_sensor_8(0x38AF, 0x00);
	write_cmos_sensor_8(0x38B0, 0x00);
	write_cmos_sensor_8(0x38B1, 0x00);
	write_cmos_sensor_8(0x38B2, 0x00);
	write_cmos_sensor_8(0x38B3, 0x00);

	write_cmos_sensor_8(0x0301, 0x06);
	write_cmos_sensor_8(0x0303, 0x04);
	write_cmos_sensor_8(0x0305, 0x04);
	write_cmos_sensor_8(0x0306, 0x00);
	write_cmos_sensor_8(0x0307, 0xFE);
	write_cmos_sensor_8(0x0309, 0x0A);
	write_cmos_sensor_8(0x030B, 0x04);
	write_cmos_sensor_8(0x030D, 0x04);
	write_cmos_sensor_8(0x030E, 0x00);
	write_cmos_sensor_8(0x030F, 0xD7);
	write_cmos_sensor_8(0x0310, 0x01);
	write_cmos_sensor_8(0x0820, 0x03);
	write_cmos_sensor_8(0x0821, 0xC7);
	write_cmos_sensor_8(0x0822, 0x80);
	write_cmos_sensor_8(0x0823, 0x00);
	write_cmos_sensor_8(0x3E20, 0x01);
	write_cmos_sensor_8(0x3E37, 0x01);
	write_cmos_sensor_8(0x3E3B, 0x00);
	write_cmos_sensor_8(0x0106, 0x00);
	write_cmos_sensor_8(0x0B00, 0x00);
	write_cmos_sensor_8(0x3230, 0x00);
	write_cmos_sensor_8(0x3F14, 0x00);
	write_cmos_sensor_8(0x3F3C, 0x03);
	write_cmos_sensor_8(0x3F0D, 0x0A);
	write_cmos_sensor_8(0x3FBC, 0x00);
	write_cmos_sensor_8(0x3C06, 0x01);
	write_cmos_sensor_8(0x3C07, 0xA1);
	write_cmos_sensor_8(0x3C0A, 0x00);
	write_cmos_sensor_8(0x3C0B, 0x00);
	write_cmos_sensor_8(0x3F78, 0x00);
	write_cmos_sensor_8(0x3F79, 0x00);
	write_cmos_sensor_8(0x3F7C, 0x00);
	write_cmos_sensor_8(0x3F7D, 0x00);
	write_cmos_sensor_8(0x0202, 0x03);
	write_cmos_sensor_8(0x0203, 0xE8);
	write_cmos_sensor_8(0x0224, 0x03);
	write_cmos_sensor_8(0x0225, 0xE8);
	write_cmos_sensor_8(0x0204, 0x00);
	write_cmos_sensor_8(0x0205, 0x00);
	write_cmos_sensor_8(0x0216, 0x00);
	write_cmos_sensor_8(0x0217, 0x00);
	write_cmos_sensor_8(0x020E, 0x01);
	write_cmos_sensor_8(0x020F, 0x00);
	write_cmos_sensor_8(0x0218, 0x01);
	write_cmos_sensor_8(0x0219, 0x00);
	write_cmos_sensor_8(0x3614, 0x00);
	write_cmos_sensor_8(0x3616, 0x0D);
	write_cmos_sensor_8(0x3617, 0x56);
	write_cmos_sensor_8(0xB612, 0x20);
	write_cmos_sensor_8(0xB613, 0x20);
	write_cmos_sensor_8(0xB614, 0x20);
	write_cmos_sensor_8(0xB615, 0x20);
	write_cmos_sensor_8(0xB616, 0x0A);
	write_cmos_sensor_8(0xB617, 0x0A);
	write_cmos_sensor_8(0xB618, 0x20);
	write_cmos_sensor_8(0xB619, 0x20);
	write_cmos_sensor_8(0xB61A, 0x20);
	write_cmos_sensor_8(0xB61B, 0x20);
	write_cmos_sensor_8(0xB61C, 0x0A);
	write_cmos_sensor_8(0xB61D, 0x0A);
	write_cmos_sensor_8(0xB666, 0x30);
	write_cmos_sensor_8(0xB667, 0x30);
	write_cmos_sensor_8(0xB668, 0x30);
	write_cmos_sensor_8(0xB669, 0x30);
	write_cmos_sensor_8(0xB66A, 0x14);
	write_cmos_sensor_8(0xB66B, 0x14);
	write_cmos_sensor_8(0xB66C, 0x20);
	write_cmos_sensor_8(0xB66D, 0x20);
	write_cmos_sensor_8(0xB66E, 0x20);
	write_cmos_sensor_8(0xB66F, 0x20);
	write_cmos_sensor_8(0xB670, 0x10);
	write_cmos_sensor_8(0xB671, 0x10);
	write_cmos_sensor_8(0x3900, 0x01);
	write_cmos_sensor_8(0x3901, 0x01);
	write_cmos_sensor_8(0x3237, 0x00);
	write_cmos_sensor_8(0x30AC, 0x00);
	#endif
	LOG_INF("X");
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
	/*sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 *we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = ((read_cmos_sensor_8(0x0016) << 8)
					| read_cmos_sensor_8(0x0017));
			LOG_INF(
				"read_0x0000=0x%x, 0x0001=0x%x,0x0000_0001=0x%x\n",
				read_cmos_sensor_8(0x0016),
				read_cmos_sensor_8(0x0017),
				read_cmos_sensor(0x0000));
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
	if (*sensor_id != imgsensor_info.sensor_id) {
		/*if Sensor ID is not correct,
		 *Must set *sensor_id to 0xFFFFFFFF
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

	LOG_INF("PLATFORM:MT6595, MIPI 2LANE\n");
	LOG_INF(
	"preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n");

	/*sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 *we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = ((read_cmos_sensor_8(0x0016) << 8)
					| read_cmos_sensor_8(0x0017));
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
	if (imgsensor_info.sensor_id != sensor_id)
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
} /* open */

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
} /* close */


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
	LOG_INF("%s E\n", __func__);

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
} /* preview */

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
#if 0
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
	/* PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M */
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF(
			"Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",
		imgsensor.current_fps, imgsensor_info.cap1.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
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
	}
#else
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
#endif

	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);

	if (imgsensor.pdaf_mode)
		imx519_apply_LRC();

	/* set_mirror_flip(imgsensor.mirror); */

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
	normal_video_setting(imgsensor.current_fps);
	/*set_mirror_flip(imgsensor.mirror);*/

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

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
	/*set_mirror_flip(imgsensor.mirror);*/

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
	/*set_mirror_flip(imgsensor.mirror);*/

	return ERROR_NONE;
}	/* slim_video */


static kal_uint32 custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s.\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom1_setting();

	if (imgsensor.pdaf_mode)
		imx519_apply_LRC();

	return ERROR_NONE;
}	/* custom1 */

static kal_uint32 custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s.\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	imgsensor.pclk = imgsensor_info.custom2.pclk;
	imgsensor.line_length = imgsensor_info.custom2.linelength;
	imgsensor.frame_length = imgsensor_info.custom2.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom2_setting();

	if (imgsensor.pdaf_mode)
		imx519_apply_LRC();

	return ERROR_NONE;
}	/* custom2 */

static kal_uint32 get_resolution(
		(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *) sensor_resolution)
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

	sensor_resolution->SensorCustom1Width =
		imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height =
		imgsensor_info.custom1.grabwindow_height;

	sensor_resolution->SensorCustom2Width =
		imgsensor_info.custom2.grabwindow_width;
	sensor_resolution->SensorCustom2Height =
		imgsensor_info.custom2.grabwindow_height;

	return ERROR_NONE;
} /* get_resolution */

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
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
	sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;
	sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 2;
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0; /* 0 is default 1x */
	sensor_info->SensorHightSampling = 0; /* 0 is default 1x */
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

	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
		break;

	case MSDK_SCENARIO_ID_CUSTOM2:
		sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom2.mipi_data_lp2hs_settle_dc;
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
	case MSDK_SCENARIO_ID_CUSTOM1:
		custom1(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		custom2(image_window, sensor_config_data);
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
		? (frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk /
				framerate * 10 /
				imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.normal_video.framelength)
		? (frame_length - imgsensor_info.normal_video.framelength)
		: 0;
		imgsensor.frame_length =
			imgsensor_info.normal_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
#if 0
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		frame_length = imgsensor_info.cap1.pclk / framerate * 10
				/ imgsensor_info.cap1.linelength;
		spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap1.framelength)
			? (frame_length - imgsensor_info.cap1.framelength) : 0;
			imgsensor.frame_length =
				imgsensor_info.cap1.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
	} else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
		frame_length = imgsensor_info.cap2.pclk / framerate * 10
					/ imgsensor_info.cap2.linelength;
		spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			  (frame_length > imgsensor_info.cap2.framelength)
			  ? (frame_length - imgsensor_info.cap2.framelength)
			  : 0;
			imgsensor.frame_length =
				imgsensor_info.cap2.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
	} else {
			if (imgsensor.current_fps
				!= imgsensor_info.cap.max_framerate)
			LOG_INF(
			"Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n"
			, framerate, imgsensor_info.cap.max_framerate/10);
		frame_length = imgsensor_info.cap.pclk / framerate * 10
				/ imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap.framelength)
			  ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length =
				imgsensor_info.cap.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
	}
#else
	if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
		LOG_INF(
			"Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n"
			, framerate, imgsensor_info.cap.max_framerate/10);
		frame_length = imgsensor_info.cap.pclk / framerate * 10
				/ imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap.framelength)
			  ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length =
				imgsensor_info.cap.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
#endif
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10
				/ imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.hs_video.framelength)
			  ? (frame_length - imgsensor_info.hs_video.framelength)
			  : 0;
		imgsensor.frame_length =
			imgsensor_info.hs_video.framelength
				+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10
			/ imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.slim_video.framelength)
			? (frame_length - imgsensor_info.slim_video.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.slim_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10
				/ imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom1.framelength)
			? (frame_length - imgsensor_info.custom1.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.custom1.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		frame_length = imgsensor_info.custom2.pclk / framerate * 10
				/ imgsensor_info.custom2.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom2.framelength)
			? (frame_length - imgsensor_info.custom2.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.custom2.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	default:  /*coding with  preview scenario by default*/
		frame_length = imgsensor_info.pre.pclk / framerate * 10
			/ imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
			? (frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
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
	case MSDK_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		*framerate = imgsensor_info.custom2.max_framerate;
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
		/* 0x5E00[8]: 1 enable,  0 disable
		 * 0x5E00[1:0];
		 * 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		 */
		write_cmos_sensor_8(0x0600, 0x0002);
	} else {
		/*0x5E00[8]: 1 enable,  0 disable
		 *0x5E00[1:0];
		 *00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		 */
		write_cmos_sensor_8(0x0600, 0x0000);
	}
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
	/* unsigned long long *feature_return_para
	 *  = (unsigned long long *) feature_para;
	 */
	SET_PD_BLOCK_INFO_T *PDAFinfo;
	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	SENSOR_VC_INFO_STRUCT *pvcinfo;
	/* SET_SENSOR_AWB_GAIN *pSetSensorAWB
	 *  = (SET_SENSOR_AWB_GAIN *)feature_para;
	 */
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data
		= (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/*LOG_INF("feature_id = %d\n", feature_id);*/
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
		 /* night_mode((BOOL) *feature_data); */
		break;
	#ifdef VENDOR_EDIT
	case SENSOR_FEATURE_CHECK_MODULE_ID:
		*feature_return_para_32 = imgsensor_info.module_id;
		break;
	#endif
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor_8(sensor_reg_data->RegAddr,
				    sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/*get the lens driver ID from EEPROM
		 * or just return LENS_DRIVER_ID_DO_NOT_CARE
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
		set_auto_flicker_mode((BOOL)*feature_data_16,
				      *(feature_data_16+1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		 set_max_framerate_by_scenario(
				(MSDK_SCENARIO_ID_ENUM)*feature_data,
				*(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		 get_default_framerate_by_scenario(
				(MSDK_SCENARIO_ID_ENUM)*(feature_data),
				(MUINT32 *)(uintptr_t)(*(feature_data+1)));
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
		#if 0
		read_3P8_eeprom((kal_uint16)(*feature_data),
				(char *)(uintptr_t)(*(feature_data+1)),
				(kal_uint32)(*(feature_data+2)));
		#endif
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		/* for factory mode auto testing */
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		LOG_INF("current fps :%d\n", (UINT32)*feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_mode = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			(UINT32)*feature_data);
		wininfo =
		  (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

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
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo,
			(void *)&imgsensor_winsize_info[4],
			sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy((void *)wininfo,
			(void *)&imgsensor_winsize_info[5],
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
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n",
			(UINT16) *feature_data);
		PDAFinfo =
			(SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			#if 0
			memcpy((void *)PDAFinfo,
				(void *)&imgsensor_pd_info,
				sizeof(SET_PD_BLOCK_INFO_T));
			#endif
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			#if 0
			memcpy((void *)PDAFinfo,
				(void *)&imgsensor_pd_info_16_9,
				sizeof(SET_PD_BLOCK_INFO_T));
			#endif
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:

		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF(
		"SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n",
			(UINT16) *feature_data);
		/*PDAF capacity enable or not, 2p8 only full size support PDAF*/
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			/* video & capture use same setting */
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_REG_SETTING:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_REG_SETTING %d",
			(*feature_para_len));
		imx519_get_pdaf_reg_setting((*feature_para_len) / sizeof(UINT32)
					   , feature_data_16);
		break;
	case SENSOR_FEATURE_SET_PDAF_REG_SETTING:
		LOG_INF("SENSOR_FEATURE_SET_PDAF_REG_SETTING %d",
			(*feature_para_len));
		imx519_set_pdaf_reg_setting((*feature_para_len) / sizeof(UINT32)
					   , feature_data_16);
		break;
	case SENSOR_FEATURE_SET_PDAF:
		LOG_INF("PDAF mode :%d\n", *feature_data_16);
		imgsensor.pdaf_mode = *feature_data_16;
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data,
			(UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) (*feature_data),
					(UINT16) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_AWB_GAIN:
		break;
	case SENSOR_FEATURE_SET_HDR_SHUTTER:
		LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1));
		#if 0
		ihdr_write_shutter((UINT16)*feature_data,
				   (UINT16)*(feature_data+1));
		#endif
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
			/* rate = imgsensor_info.hs_video.mipi_pixel_rate; */
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			rate = imgsensor_info.custom1.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			rate = imgsensor_info.custom2.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			rate = imgsensor_info.pre.mipi_pixel_rate;
			break;
		}
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
	}
break;

	case SENSOR_FEATURE_GET_VC_INFO:
		LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16)*feature_data);
		pvcinfo =
		  (SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
				sizeof(SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2],
				sizeof(SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
				sizeof(SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
				sizeof(SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
				sizeof(SENSOR_VC_INFO_STRUCT));
			break;
		default:
			#if 0
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
				sizeof(SENSOR_VC_INFO_STRUCT));
			#endif
			break;
		}
	default:
		break;
	}

	return ERROR_NONE;
} /* feature_control() */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 IMX519_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
} /* IMX519_MIPI_RAW_SensorInit */
