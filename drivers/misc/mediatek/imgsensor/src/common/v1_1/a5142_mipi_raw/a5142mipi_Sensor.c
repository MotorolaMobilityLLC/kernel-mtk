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
 *     A5142mipi_Sensor.c
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
#include <linux/atomic.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "a5142mipi_Sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "A5142_camera_sensor"
#define LOG_1 LOG_INF("A5142,MIPI 2LANE\n")
#define LOG_2 LOG_INF("preview 1280*960@30fps; video 1280*960@30fps; capture 5M@15fps\n")
/****************************   Modify end    *******************************************/

#define LOG_INF(fmt, args...)    pr_debug(PFX "[%s] " fmt, __func__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = A5142MIPI_SENSOR_ID,

	.checksum_value = 0xc9e24698,	/* checksum value for Camera Auto Test */

	.pre = {
		.pclk = 111800000,	/* record different mode's pclk */
		.linelength = 3694,	/* 3694,            //record different mode's linelength, nick-diff */
		.framelength = 2021,	/* 2021,            //record different mode's framelength */
		.startx = 0,	/* record different mode's startx of grabwindow */
		.starty = 1,	/* record different mode's starty of grabwindow */
		.grabwindow_width = 2560,	/* record different mode's width of grabwindow */
		.grabwindow_height = 1920,	/* record different mode's height of grabwindow */
		/*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
		.mipi_data_lp2hs_settle_dc = 42,	/* unit , ns */
		/*     following for GetDefaultFramerateByScenario()    */
		.mipi_pixel_rate = 168000000,
		.max_framerate = 150,
		},
	.cap = {
		.pclk = 111800000,
		.linelength = 3694,	/* 3151, nick-diff */
		.framelength = 2021,	/* 1100, */
		.startx = 0,
		.starty = 1,
		.grabwindow_width = 2560,
		.grabwindow_height = 1920,
		.mipi_data_lp2hs_settle_dc = 42,	/* unit , ns */
		.mipi_pixel_rate = 168000000,
		.max_framerate = 150,
		},
	.cap1 = {
		 .pclk = 111800000,
		 .linelength = 3694,	/* 3151, nick-diff */
		 .framelength = 2021,	/* 1100, */
		 .startx = 0,
		 .starty = 1,
		 .grabwindow_width = 2560,
		 .grabwindow_height = 1920,
		 .mipi_data_lp2hs_settle_dc = 42,	/* unit , ns */
		 .mipi_pixel_rate = 168000000,
		 .max_framerate = 150,
		 },
	.normal_video = {
			 .pclk = 111800000,
			 .linelength = 3694,	/* 3694, nick-diff */
			 .framelength = 2021,	/* 2021, */
			 .startx = 0,
			 .starty = 1,
			 .grabwindow_width = 2560,
			 .grabwindow_height = 1920,
			 .mipi_data_lp2hs_settle_dc = 42,	/* unit , ns */
			 .mipi_pixel_rate = 168000000,
			 .max_framerate = 150,
			 },
	.hs_video = {
		     .pclk = 111800000,
		     .linelength = 3694,	/* nick-diff */
		     .framelength = 2021,
		     .startx = 0,
		     .starty = 1,
		     .grabwindow_width = 2560,
		     .grabwindow_height = 1920,
		     .mipi_data_lp2hs_settle_dc = 42,	/* unit , ns */
		     .mipi_pixel_rate = 168000000,
		     .max_framerate = 150,
		     },
	.slim_video = {
		       .pclk = 111800000,
		       .linelength = 3694,	/* nick-diff */
		       .framelength = 2021,
		       .startx = 0,
		       .starty = 1,
		       .grabwindow_width = 2560,
		       .grabwindow_height = 1920,
		       .mipi_data_lp2hs_settle_dc = 42,	/* unit , ns */
		       .mipi_pixel_rate = 168000000,
		       .max_framerate = 150,
		       },
	.margin = 8,		/* sensor framelength & shutter margin */
	.min_shutter = 1,	/* min shutter */
	.max_frame_length = 0x7fff,	/* max framelength by sensor register's limitation */
	.ae_shut_delay_frame = 0,
	/* shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2 */
	.ae_sensor_gain_delay_frame = 1,
	/* sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2 */
	.ae_ispGain_delay_frame = 2,	/* isp gain delay frame for AE cycle */
	.frame_time_delay_frame = 1,	/* The delay frame of setting frame length  */
	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.sensor_mode_num = 5,	/* support sensor mode num */

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 5,
	.hs_video_delay_frame = 5,
	.slim_video_delay_frame = 5,

	.isp_driving_current = ISP_DRIVING_2MA,	/* mclk driving current */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,	/* sensor_interface_type */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
	/* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,	/* sensor output first pixel color */
	.mclk = 26,		/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_2_LANE,	/* mipi lane num */
	.i2c_addr_table = {0x6c, 0x6e, 0xff},
	/* .i2c_speed = 200, */
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,	/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,	/* current shutter */
	.gain = 0x100,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	.current_fps = 0,	/* 300 //full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.autoflicker_en = KAL_FALSE,
	/* auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker */
	.test_pattern = KAL_FALSE,
	/* test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output */
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,	/* current scenario id */
	.ihdr_en = 0,		/* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x6c,	/* record current sensor's i2c write id */
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
	{2608, 1960, 8, 8, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944, 0, 1, 2560, 1920},	/* Preview */
	{2608, 1960, 8, 8, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944, 0, 1, 2560, 1920},	/* capture */
	{2608, 1960, 8, 8, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944, 0, 1, 2560, 1920},	/* video */
	{2608, 1960, 8, 8, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944, 0, 1, 2560, 1920},	/* hs video */
	{2608, 1960, 8, 8, 2592, 1944, 2592, 1944, 0, 0, 2592, 1944, 0, 1, 2560, 1920},	/* slim video */
};


static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *) &get_byte, 2, imgsensor.i2c_write_id);

	return ((get_byte << 8) & 0xff00) | ((get_byte >> 8) & 0x00ff);
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[4] = {
	    (char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF) };

	iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
}

#if 0
static kal_uint16 read_cmos_sensor_8(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	kdSetI2CSpeed(imgsensor_info.i2c_speed);	/* Add this func to set i2c speed by each sensor */

	iReadRegI2C(pu_send_cmd, 2, (u8 *) &get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}
#endif
static void write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = { (char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF) };

	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel,*/
	/* or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor_8(0x0104, 0x01);	/* GROUPED_PARAMETER_HOLD */
	write_cmos_sensor(0x0340, imgsensor.frame_length);
	write_cmos_sensor(0x0342, imgsensor.line_length);
	write_cmos_sensor_8(0x0104, 0x00);	/* GROUPED_PARAMETER_HOLD */

}				/*    set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	return read_cmos_sensor(0x0000);
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min_framelength_en = %d\n", framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;

	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length =
	    (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);

	set_dummy();
}				/*    set_max_framerate  */

static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;

	LOG_INF("write_shutter, shutter = %d\n", shutter);
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */


	/* if shutter bigger than frame_length, should extend frame length first */
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter =
	    (shutter >
	     (imgsensor_info.max_frame_length -
	      imgsensor_info.margin)) ? (imgsensor_info.max_frame_length -
					 imgsensor_info.margin) : shutter;

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
	}

	write_cmos_sensor_8(0x0104, 0x01);	/* GROUPED_PARAMETER_HOLD */
	write_cmos_sensor(0x0202, shutter);	/* course_integration_time */
	write_cmos_sensor_8(0x0104, 0x00);	/* GROUPED_PARAMETER_HOLD */

	LOG_INF("[A5142] << write_shutter , shutter =%d, framelength =%d\n", shutter,
		imgsensor.frame_length);
}


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
/* kal_uint16 A5142MIPI_exposure_line=100; */
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);

}

static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{

	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	LOG_INF("Enter set_shutter_frame_length! shutter = %d, frame_length = %d\n", shutter,
		frame_length);


	/* write_shutter(shutter); */
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	/* if shutter bigger than frame_length, should extend frame length first */
	spin_lock(&imgsensor_drv_lock);
	/* Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;

	/*  */
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter =
	    (shutter >
	     (imgsensor_info.max_frame_length -
	      imgsensor_info.margin)) ? (imgsensor_info.max_frame_length -
					 imgsensor_info.margin) : shutter;

	/* frame_length and shutter should be an even number. */


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
	}

	write_cmos_sensor_8(0x0104, 0x01);	/* GROUPED_PARAMETER_HOLD */
	write_cmos_sensor(0x0202, shutter);	/* course_integration_time */
	write_cmos_sensor_8(0x0104, 0x00);	/* GROUPED_PARAMETER_HOLD */

	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

}



static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 iReg = 0x0000;

	iReg = gain >> 1;
	return iReg;
}

static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	reg_gain = gain2reg(gain);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	write_cmos_sensor_8(0x0104, 0x01);	/* parameter_hold */
	reg_gain = gain2reg(gain);
	write_cmos_sensor(0x0204, reg_gain);
	write_cmos_sensor_8(0x0104, 0x00);	/* parameter_hold */

	return gain;
}				/*    set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	/* not support HDR */
	/* LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain); */
}

#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
/* LOG_INF("image_mirror = %d", image_mirror); */
	LOG_INF("[A5142] >> set_mirror_flip  , image_mirror = %d\n", image_mirror);
	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor_8(0x0101, 0x00);
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor_8(0x0101, 0x01);
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor_8(0x0101, 0x02);
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor_8(0x0101, 0x03);
		break;
	}
}
#endif
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
}				/*    night_mode    */

static void sensor_init(void)
{
	LOG_INF("E\n");
	LOG_INF("[A5142] >> sensor_init\n");
	write_cmos_sensor_8(0x0103, 0x01);	/* SOFTWARE_RESET (clears itself) */
	mDELAY(100);		/* Initialization Time */

	/* [Demo Initialization 1296 x 972 MCLK= 26MHz, PCLK=104MHz] */
	/* stop_streaming */
	write_cmos_sensor_8(0x0100, 0x00);	/* MODE_SELECT */

	write_cmos_sensor(0x301A, 0x0218);	/* RESET_REGISTER enable mipi interface  bit[9] mask bad frame */
	write_cmos_sensor(0x3064, 0xB800);	/* SMIA_TEST */
	write_cmos_sensor(0x31AE, 0x0202);	/* two lane */
	write_cmos_sensor(0x0112, 0x0A0A);	/* 10bit raw output */

	/* REV4_recommended_settings */
	write_cmos_sensor(0x316A, 0x8400);	/* DAC_FBIAS */
	write_cmos_sensor(0x316C, 0x8400);	/* DAC_TXLO */
	write_cmos_sensor(0x316E, 0x8400);	/* DAC_ECL */
	write_cmos_sensor(0x3EFA, 0x1A1F);	/* DAC_LD_ECL */
	write_cmos_sensor(0x3ED2, 0xD965);	/* DAC_LD_6_7 */
	write_cmos_sensor(0x3ED8, 0x7F1B);	/* DAC_LD_12_13 */
	write_cmos_sensor(0x3EDA, 0x2F11);	/* DAC_LD_14_15 */
	write_cmos_sensor(0x3EE2, 0x0060);	/* DAC_LD_22_23 */
	write_cmos_sensor(0x3EF2, 0xD965);	/* DAC_LP_6_7 */
	write_cmos_sensor(0x3EF8, 0x797F);	/* DAC_LD_TXHI */
	write_cmos_sensor(0x3EFC, 0x286F);	/* DAC_LD_FBIAS */
	write_cmos_sensor(0x3EFE, 0x2C01);	/* DAC_LD_TXLO */

	/* REV1_pixel_timing */
	write_cmos_sensor(0x3E00, 0x042F);	/* DYNAMIC_SEQRAM_00 */
	write_cmos_sensor(0x3E02, 0xFFFF);	/* DYNAMIC_SEQRAM_02 */
	write_cmos_sensor(0x3E04, 0xFFFF);	/* DYNAMIC_SEQRAM_04 */
	write_cmos_sensor(0x3E06, 0xFFFF);	/* DYNAMIC_SEQRAM_06 */
	write_cmos_sensor(0x3E08, 0x8071);	/* DYNAMIC_SEQRAM_08 */
	write_cmos_sensor(0x3E0A, 0x7281);	/* DYNAMIC_SEQRAM_0A */
	write_cmos_sensor(0x3E0C, 0x4011);	/* DYNAMIC_SEQRAM_0C */
	write_cmos_sensor(0x3E0E, 0x8010);	/* DYNAMIC_SEQRAM_0E */
	write_cmos_sensor(0x3E10, 0x60A5);	/* DYNAMIC_SEQRAM_10 */
	write_cmos_sensor(0x3E12, 0x4080);	/* DYNAMIC_SEQRAM_12 */
	write_cmos_sensor(0x3E14, 0x4180);	/* DYNAMIC_SEQRAM_14 */
	write_cmos_sensor(0x3E16, 0x0018);	/* DYNAMIC_SEQRAM_16 */
	write_cmos_sensor(0x3E18, 0x46B7);	/* DYNAMIC_SEQRAM_18 */
	write_cmos_sensor(0x3E1A, 0x4994);	/* DYNAMIC_SEQRAM_1A */
	write_cmos_sensor(0x3E1C, 0x4997);	/* DYNAMIC_SEQRAM_1C */
	write_cmos_sensor(0x3E1E, 0x4682);	/* DYNAMIC_SEQRAM_1E */
	write_cmos_sensor(0x3E20, 0x0018);	/* DYNAMIC_SEQRAM_20 */
	write_cmos_sensor(0x3E22, 0x4241);	/* DYNAMIC_SEQRAM_22 */
	write_cmos_sensor(0x3E24, 0x8000);	/* DYNAMIC_SEQRAM_24 */
	write_cmos_sensor(0x3E26, 0x1880);	/* DYNAMIC_SEQRAM_26 */
	write_cmos_sensor(0x3E28, 0x4785);	/* DYNAMIC_SEQRAM_28 */
	write_cmos_sensor(0x3E2A, 0x4992);	/* DYNAMIC_SEQRAM_2A */
	write_cmos_sensor(0x3E2C, 0x4997);	/* DYNAMIC_SEQRAM_2C */
	write_cmos_sensor(0x3E2E, 0x4780);	/* DYNAMIC_SEQRAM_2E */
	write_cmos_sensor(0x3E30, 0x4D80);	/* DYNAMIC_SEQRAM_30 */
	write_cmos_sensor(0x3E32, 0x100C);	/* DYNAMIC_SEQRAM_32 */
	write_cmos_sensor(0x3E34, 0x8000);	/* DYNAMIC_SEQRAM_34 */
	write_cmos_sensor(0x3E36, 0x184A);	/* DYNAMIC_SEQRAM_36 */
	write_cmos_sensor(0x3E38, 0x8042);	/* DYNAMIC_SEQRAM_38 */
	write_cmos_sensor(0x3E3A, 0x001A);	/* DYNAMIC_SEQRAM_3A */
	write_cmos_sensor(0x3E3C, 0x9610);	/* DYNAMIC_SEQRAM_3C */
	write_cmos_sensor(0x3E3E, 0x0C80);	/* DYNAMIC_SEQRAM_3E */
	write_cmos_sensor(0x3E40, 0x4DC6);	/* DYNAMIC_SEQRAM_40 */
	write_cmos_sensor(0x3E42, 0x4A80);	/* DYNAMIC_SEQRAM_42 */
	write_cmos_sensor(0x3E44, 0x0018);	/* DYNAMIC_SEQRAM_44 */
	write_cmos_sensor(0x3E46, 0x8042);	/* DYNAMIC_SEQRAM_46 */
	write_cmos_sensor(0x3E48, 0x8041);	/* DYNAMIC_SEQRAM_48 */
	write_cmos_sensor(0x3E4A, 0x0018);	/* DYNAMIC_SEQRAM_4A */
	write_cmos_sensor(0x3E4C, 0x804B);	/* DYNAMIC_SEQRAM_4C */
	write_cmos_sensor(0x3E4E, 0xB74B);	/* DYNAMIC_SEQRAM_4E */
	write_cmos_sensor(0x3E50, 0x8010);	/* DYNAMIC_SEQRAM_50 */
	write_cmos_sensor(0x3E52, 0x6056);	/* DYNAMIC_SEQRAM_52 */
	write_cmos_sensor(0x3E54, 0x001C);	/* DYNAMIC_SEQRAM_54 */
	write_cmos_sensor(0x3E56, 0x8211);	/* DYNAMIC_SEQRAM_56 */
	write_cmos_sensor(0x3E58, 0x8056);	/* DYNAMIC_SEQRAM_58 */
	write_cmos_sensor(0x3E5A, 0x827C);	/* DYNAMIC_SEQRAM_5A */
	write_cmos_sensor(0x3E5C, 0x0970);	/* DYNAMIC_SEQRAM_5C */
	write_cmos_sensor(0x3E5E, 0x8082);	/* DYNAMIC_SEQRAM_5E */
	write_cmos_sensor(0x3E60, 0x7281);	/* DYNAMIC_SEQRAM_60 */
	write_cmos_sensor(0x3E62, 0x4C40);	/* DYNAMIC_SEQRAM_62 */
	write_cmos_sensor(0x3E64, 0x8E4D);	/* DYNAMIC_SEQRAM_64 */
	write_cmos_sensor(0x3E66, 0x8110);	/* DYNAMIC_SEQRAM_66 */
	write_cmos_sensor(0x3E68, 0x0CAF);	/* DYNAMIC_SEQRAM_68 */
	write_cmos_sensor(0x3E6A, 0x4D80);	/* DYNAMIC_SEQRAM_6A */
	write_cmos_sensor(0x3E6C, 0x100C);	/* DYNAMIC_SEQRAM_6C */
	write_cmos_sensor(0x3E6E, 0x8440);	/* DYNAMIC_SEQRAM_6E */
	write_cmos_sensor(0x3E70, 0x4C81);	/* DYNAMIC_SEQRAM_70 */
	write_cmos_sensor(0x3E72, 0x7C5F);	/* DYNAMIC_SEQRAM_72 */
	write_cmos_sensor(0x3E74, 0x7000);	/* DYNAMIC_SEQRAM_74 */
	write_cmos_sensor(0x3E76, 0x0000);	/* DYNAMIC_SEQRAM_76 */
	write_cmos_sensor(0x3E78, 0x0000);	/* DYNAMIC_SEQRAM_78 */
	write_cmos_sensor(0x3E7A, 0x0000);	/* DYNAMIC_SEQRAM_7A */
	write_cmos_sensor(0x3E7C, 0x0000);	/* DYNAMIC_SEQRAM_7C */
	write_cmos_sensor(0x3E7E, 0x0000);	/* DYNAMIC_SEQRAM_7E */
	write_cmos_sensor(0x3E80, 0x0000);	/* DYNAMIC_SEQRAM_80 */
	write_cmos_sensor(0x3E82, 0x0000);	/* DYNAMIC_SEQRAM_82 */
	write_cmos_sensor(0x3E84, 0x0000);	/* DYNAMIC_SEQRAM_84 */
	write_cmos_sensor(0x3E86, 0x0000);	/* DYNAMIC_SEQRAM_86 */
	write_cmos_sensor(0x3E88, 0x0000);	/* DYNAMIC_SEQRAM_88 */
	write_cmos_sensor(0x3E8A, 0x0000);	/* DYNAMIC_SEQRAM_8A */
	write_cmos_sensor(0x3E8C, 0x0000);	/* DYNAMIC_SEQRAM_8C */
	write_cmos_sensor(0x3E8E, 0x0000);	/* DYNAMIC_SEQRAM_8E */
	write_cmos_sensor(0x3E90, 0x0000);	/* DYNAMIC_SEQRAM_90 */
	write_cmos_sensor(0x3E92, 0x0000);	/* DYNAMIC_SEQRAM_92 */
	write_cmos_sensor(0x3E94, 0x0000);	/* DYNAMIC_SEQRAM_94 */
	write_cmos_sensor(0x3E96, 0x0000);	/* DYNAMIC_SEQRAM_96 */
	write_cmos_sensor(0x3E98, 0x0000);	/* DYNAMIC_SEQRAM_98 */
	write_cmos_sensor(0x3E9A, 0x0000);	/* DYNAMIC_SEQRAM_9A */
	write_cmos_sensor(0x3E9C, 0x0000);	/* DYNAMIC_SEQRAM_9C */
	write_cmos_sensor(0x3E9E, 0x0000);	/* DYNAMIC_SEQRAM_9E */
	write_cmos_sensor(0x3EA0, 0x0000);	/* DYNAMIC_SEQRAM_A0 */
	write_cmos_sensor(0x3EA2, 0x0000);	/* DYNAMIC_SEQRAM_A2 */
	write_cmos_sensor(0x3EA4, 0x0000);	/* DYNAMIC_SEQRAM_A4 */
	write_cmos_sensor(0x3EA6, 0x0000);	/* DYNAMIC_SEQRAM_A6 */
	write_cmos_sensor(0x3EA8, 0x0000);	/* DYNAMIC_SEQRAM_A8 */
	write_cmos_sensor(0x3EAA, 0x0000);	/* DYNAMIC_SEQRAM_AA */
	write_cmos_sensor(0x3EAC, 0x0000);	/* DYNAMIC_SEQRAM_AC */
	write_cmos_sensor(0x3EAE, 0x0000);	/* DYNAMIC_SEQRAM_AE */
	write_cmos_sensor(0x3EB0, 0x0000);	/* DYNAMIC_SEQRAM_B0 */
	write_cmos_sensor(0x3EB2, 0x0000);	/* DYNAMIC_SEQRAM_B2 */
	write_cmos_sensor(0x3EB4, 0x0000);	/* DYNAMIC_SEQRAM_B4 */
	write_cmos_sensor(0x3EB6, 0x0000);	/* DYNAMIC_SEQRAM_B6 */
	write_cmos_sensor(0x3EB8, 0x0000);	/* DYNAMIC_SEQRAM_B8 */
	write_cmos_sensor(0x3EBA, 0x0000);	/* DYNAMIC_SEQRAM_BA */
	write_cmos_sensor(0x3EBC, 0x0000);	/* DYNAMIC_SEQRAM_BC */
	write_cmos_sensor(0x3EBE, 0x0000);	/* DYNAMIC_SEQRAM_BE */
	write_cmos_sensor(0x3EC0, 0x0000);	/* DYNAMIC_SEQRAM_C0 */
	write_cmos_sensor(0x3EC2, 0x0000);	/* DYNAMIC_SEQRAM_C2 */
	write_cmos_sensor(0x3EC4, 0x0000);	/* DYNAMIC_SEQRAM_C4 */
	write_cmos_sensor(0x3EC6, 0x0000);	/* DYNAMIC_SEQRAM_C6 */
	write_cmos_sensor(0x3EC8, 0x0000);	/* DYNAMIC_SEQRAM_C8 */
	write_cmos_sensor(0x3ECA, 0x0000);	/* DYNAMIC_SEQRAM_CA */

	/* dynamic power disable */
	write_cmos_sensor(0x3170, 0x2150);	/* ANALOG_CONTROL */
	write_cmos_sensor(0x317A, 0x0150);	/* ANALOG_CONTROL6 */
	write_cmos_sensor(0x3ECC, 0x2200);	/* DAC_LD_0_1 */
	write_cmos_sensor(0x3174, 0x0000);	/* ANALOG_CONTROL3 */
	write_cmos_sensor(0x3176, 0x0000);	/* ANALOG_CONTROL4 */

	write_cmos_sensor(0x30BC, 0x0384);
	write_cmos_sensor(0x30C0, 0x1220);
	write_cmos_sensor(0x30D4, 0x9200);
	write_cmos_sensor(0x30B2, 0xC000);
	write_cmos_sensor(0x3100, 0x0000);
	write_cmos_sensor(0x3102, 0x0060);

	write_cmos_sensor(0x31B0, 0x00C4);
	write_cmos_sensor(0x31B2, 0x0064);
	write_cmos_sensor(0x31B4, 0x0E77);
	write_cmos_sensor(0x31B6, 0x0D24);
	write_cmos_sensor(0x31B8, 0x020E);
	write_cmos_sensor(0x31BA, 0x0710);
	write_cmos_sensor(0x31BC, 0x2A0D);
	write_cmos_sensor(0x31BE, 0xC007);
	/* write_cmos_sensor(0x31BE, 0xC003); */

	/* write_cmos_sensor(0x3ECE, 0x0000);  // DAC_LD_2_3 */
	/* write_cmos_sensor(0x0400, 0x0000);  // SCALING_MODE disable scale */
	/* write_cmos_sensor(0x0404, 0x0010);  // SCALE_M */

	write_cmos_sensor(0x305E, 0x112E);	/* global gain */
	write_cmos_sensor(0x30F0, 0x0000);	/* disable AF  A5142 have not internal AF IC */

	/* PLL MCLK = 26MHZ, PCLK = 104MHZ, VT = 104MHZ */
	write_cmos_sensor(0x0300, 0x05);	/* vt_pix_clk_div = 5 */
	write_cmos_sensor(0x0302, 0x01);	/* vt_sys_clk_div = 1 */
	write_cmos_sensor(0x0304, 0x02);	/* pre_pll_clk_div = 2 */
	write_cmos_sensor(0x0306, 0x28);	/* pll_multiplier    =  40 */
	write_cmos_sensor(0x0308, 0x0A);	/* op_pix_clk_div =  10 */
	write_cmos_sensor(0x030A, 0x01);	/* op_sys_clk_div = 1 */

	/* write_cmos_sensor(0x306E, 0xbc00);  // slew rate for color issue */
	/* write_cmos_sensor(0x3040, 0x04C3); */
	/* write_cmos_sensor(0x3010, 0x0184);  // FINE_CORRECTION */
	/* write_cmos_sensor(0x3012, 0x029C);  // COARSE_INTEGRATION_TIME_ */
	/* write_cmos_sensor(0x3014, 0x0908);  // FINE_INTEGRATION_TIME_ */
	/* write_cmos_sensor_8(0x0100, 0x01);  // MODE_SELECT */

	mDELAY(5);		/* Allow PLL to lock */
	LOG_INF("[A5142] <<  sensor_init\n");
}				/*    MIPI_sensor_Init  */


static void preview_setting(void)
{
    /********************************************************
       *
       *   1296x972 30fps 2 lane MIPI 420Mbps/lane
       *
       ********************************************************/
	kal_uint16 temp;

#if 0
	kal_uint16 temp;

	/* stop_streaming */
	write_cmos_sensor_8(0x0100, 0x0);	/* MODE_SELECT */

	mDELAY(20);

	write_cmos_sensor_8(0x0104, 0x01);	/* GROUPED_PARAMETER_HOLD = 0x1 */

	/* 1296 x 972  Timing settings 30fps */

	/* write_cmos_sensor(0x301A, 0x0018);  // enable mipi interface */
	write_cmos_sensor(0x3064, 0xB800);	/* SMIA_TEST */
	write_cmos_sensor(0x31AE, 0x0202);	/* two lane 201 tow 202 */
	write_cmos_sensor(0x0112, 0x0A0A);	/* 10bit raw output */

	/* PLL MCLK=26MHZ, PCLK = 104MHZ, VT = 104MHZ */
	write_cmos_sensor(0x0300, 0x05);	/* vt_pix_clk_div = 5 */
	write_cmos_sensor(0x0302, 0x01);	/* vt_sys_clk_div = 1 */
	write_cmos_sensor(0x0304, 0x02);	/* pre_pll_clk_div = 2 */
	write_cmos_sensor(0x0306, 0x28);	/* pll_multiplier    =  40 */
	write_cmos_sensor(0x0308, 0x0A);	/* op_pix_clk_div =  10 */
	write_cmos_sensor(0x030A, 0x01);	/* op_sys_clk_div = 1 */

	mDELAY(10);

	write_cmos_sensor(0x0344, 0x0008);	/* X_ADDR_START   =  8 */
	write_cmos_sensor(0x0346, 0x0008);	/* Y_ADDR_START   =  8 */
	write_cmos_sensor(0x0348, 0x0A25);	/* X_ADDR_END      = 2597 */
	write_cmos_sensor(0x034A, 0x079D);	/* Y_ADDR_END       =  1949 */
	temp = read_cmos_sensor(0x3040);
	temp = temp & 0xF000;
	temp = temp | 0x04C3;
	write_cmos_sensor(0x3040, temp);	/* READ_MODE  10 011 000011 xy binning enable xodd=3, yodd=3 */
	write_cmos_sensor(0x034C, 0x0510);	/* X_OUTPUT_SIZE    = 1296 */
	write_cmos_sensor(0x034E, 0x03CC);	/* Y_OUTPUT_SIZE    =  972 */

	write_cmos_sensor(0x300C, 0x0C4F);	/* LINE_LENGTH  3151 */
	write_cmos_sensor(0x300A, 0x044C);	/* FRAME_LINEs  1100 */

	/* write_cmos_sensor(0x3012, 0x0414);    // coarse_integration_time, nick-differnce */
	write_cmos_sensor(0x3014, 0x0908);	/* fine_integration_time */
	write_cmos_sensor(0x3010, 0x0184);	/* fine_correction */

	write_cmos_sensor_8(0x0104, 0x00);	/* GROUPED_PARAMETER_HOLD */

	/* start_streaming */
	write_cmos_sensor_8(0x0100, 0x01);	/* MODE_SELECT */

	mDELAY(50);
#else

	/* stop_streaming */
	write_cmos_sensor_8(0x0100, 0x00);	/* MODE_SELECT */

	write_cmos_sensor_8(0x0104, 0x01);	/* Grouped Parameter Hold = 0x1 */

	/* write_cmos_sensor(0x301A, 0x0018);        // enable mipi interface */
	write_cmos_sensor(0x3064, 0xB800);	/* SMIA_TEST */
	write_cmos_sensor(0x31AE, 0x0202);	/* two lane 201 tow 202 */
	write_cmos_sensor(0x0112, 0x0A0A);	/* 10bit raw output */


	/* PLL MCLK=26MHZ, PCLK = 111.8MHZ, VT = 111.8MHZ */
	write_cmos_sensor(0x0300, 0x05);	/* vt_pix_clk_div = 5 */
	write_cmos_sensor(0x0302, 0x01);	/* vt_sys_clk_div = 1 */
	write_cmos_sensor(0x0304, 0x04);	/* pre_pll_clk_div = 4 */
	write_cmos_sensor(0x0306, 0x56);	/* pll_multiplier    =  86 */
	write_cmos_sensor(0x0308, 0x0A);	/* op_pix_clk_div =  10 */
	write_cmos_sensor(0x030A, 0x01);	/* op_sys_clk_div = 1 */

	mDELAY(10);

	write_cmos_sensor(0x0344, 0x0008);	/* X_ADDR_START   = 8 */
	write_cmos_sensor(0x0346, 0x0008);	/* Y_ADDR_START    = 8 */
	write_cmos_sensor(0x0348, 0x0A27);	/* X_ADDR_END =  2599 */
	write_cmos_sensor(0x034A, 0x079F);	/* Y_ADDR_END = 1951 */

	temp = read_cmos_sensor(0x3040);
	temp = temp & 0xF000;
	temp = temp | 0x0041;
	write_cmos_sensor(0x3040, temp);	/* Read Mode = 0x41   1 000001 binning disable */

	write_cmos_sensor(0x034C, 0x0A20);	/* X_OUTPUT_SIZE= 2592 */
	write_cmos_sensor(0x034E, 0x0798);	/* Y_OUTPUT_SIZE = 1944 */
	write_cmos_sensor(0x300A, 0x07E5);	/* Frame Lines = 0x7E5  2021 */
	write_cmos_sensor(0x300C, 0x0E6E);	/* Line Length = 0xE6E  3694 */
	write_cmos_sensor(0x3010, 0x00A0);	/* Fine Correction = 0xA0 */
	/* write_cmos_sensor(0x3012, 0x07E4);  //Coarse integration Time = 0x7E4 */
	write_cmos_sensor(0x3014, 0x0C8C);	/* Fine Integration Time = 0xC8C */

	write_cmos_sensor_8(0x0104, 0x00);	/* Grouped Parameter Hold */

	/* start_streaming */
	/* write_cmos_sensor_8(0x0100, 0x01);    // MODE_SELECT */

	mDELAY(20);
#endif
	LOG_INF("[A5142] <<  preview_setting\n");
}				/*    preview_setting  */


static void capture_setting(kal_uint16 currefps)
{
	kal_uint16 temp;

	LOG_INF("E! currefps:%d\n", currefps);

    /********************************************************
       *
       *   2592x1944 15fps 2 lane MIPI 420Mbps/lane
       *
       ********************************************************/

	/* stop_streaming */
	write_cmos_sensor_8(0x0100, 0x00);	/* MODE_SELECT */

	write_cmos_sensor_8(0x0104, 0x01);	/* Grouped Parameter Hold = 0x1 */

	/* write_cmos_sensor(0x301A, 0x0018);        // enable mipi interface */
	write_cmos_sensor(0x3064, 0xB800);	/* SMIA_TEST */
	write_cmos_sensor(0x31AE, 0x0202);	/* two lane 201 tow 202 */
	write_cmos_sensor(0x0112, 0x0A0A);	/* 10bit raw output */


	/* PLL MCLK=26MHZ, PCLK = 111.8MHZ, VT = 111.8MHZ */
	write_cmos_sensor(0x0300, 0x05);	/* vt_pix_clk_div = 5 */
	write_cmos_sensor(0x0302, 0x01);	/* vt_sys_clk_div = 1 */
	write_cmos_sensor(0x0304, 0x04);	/* pre_pll_clk_div = 4 */
	write_cmos_sensor(0x0306, 0x56);	/* pll_multiplier    =  86 */
	write_cmos_sensor(0x0308, 0x0A);	/* op_pix_clk_div =  10 */
	write_cmos_sensor(0x030A, 0x01);	/* op_sys_clk_div = 1 */

	mDELAY(10);

	write_cmos_sensor(0x0344, 0x0008);	/* X_ADDR_START   = 8 */
	write_cmos_sensor(0x0346, 0x0008);	/* Y_ADDR_START    = 8 */
	write_cmos_sensor(0x0348, 0x0A27);	/* X_ADDR_END =  2599 */
	write_cmos_sensor(0x034A, 0x079F);	/* Y_ADDR_END = 1951 */

	temp = read_cmos_sensor(0x3040);
	temp = temp & 0xF000;
	temp = temp | 0x0041;
	write_cmos_sensor(0x3040, temp);	/* Read Mode = 0x41   1 000001 binning disable */

	write_cmos_sensor(0x034C, 0x0A20);	/* X_OUTPUT_SIZE= 2592 */
	write_cmos_sensor(0x034E, 0x0798);	/* Y_OUTPUT_SIZE = 1944 */
	write_cmos_sensor(0x300A, 0x07E5);	/* Frame Lines = 0x7E5  2021 */
	write_cmos_sensor(0x300C, 0x0E6E);	/* Line Length = 0xE6E  3694 */
	write_cmos_sensor(0x3010, 0x00A0);	/* Fine Correction = 0xA0 */
	/* write_cmos_sensor(0x3012, 0x07E4);  //Coarse integration Time = 0x7E4 */
	write_cmos_sensor(0x3014, 0x0C8C);	/* Fine Integration Time = 0xC8C */

	write_cmos_sensor_8(0x0104, 0x00);	/* Grouped Parameter Hold */

	/* start_streaming */
	/* write_cmos_sensor_8(0x0100, 0x01);    // MODE_SELECT */

	mDELAY(20);
	LOG_INF("[A5142] <<  capture_setting\n");
}				/*    capture_setting  */

static void normal_video_setting(kal_uint16 currefps)
{
/* LOG_INF("E! currefps:%d\n", currefps); */

    /********************************************************
       *
       *   1296x972 30fps 2 lane MIPI 420Mbps/lane
       *
       ********************************************************/

#if 0
	kal_uint16 temp;

	/* stop_streaming */
	write_cmos_sensor_8(0x0100, 0x0);	/* MODE_SELECT */

	mDELAY(20);

	write_cmos_sensor_8(0x0104, 0x01);	/* GROUPED_PARAMETER_HOLD = 0x1 */

	/* 1296 x 972  Timing settings 30fps */

	/* write_cmos_sensor(0x301A, 0x0018);  // enable mipi interface */
	write_cmos_sensor(0x3064, 0xB800);	/* SMIA_TEST */
	write_cmos_sensor(0x31AE, 0x0202);	/* two lane 201 tow 202 */
	write_cmos_sensor(0x0112, 0x0A0A);	/* 10bit raw output */

	/* PLL MCLK=26MHZ, PCLK = 104MHZ, VT = 104MHZ */
	write_cmos_sensor(0x0300, 0x05);	/* vt_pix_clk_div = 5 */
	write_cmos_sensor(0x0302, 0x01);	/* vt_sys_clk_div = 1 */
	write_cmos_sensor(0x0304, 0x02);	/* pre_pll_clk_div = 2 */
	write_cmos_sensor(0x0306, 0x28);	/* pll_multiplier    =  40 */
	write_cmos_sensor(0x0308, 0x0A);	/* op_pix_clk_div =  10 */
	write_cmos_sensor(0x030A, 0x01);	/* op_sys_clk_div = 1 */

	mDELAY(10);

	write_cmos_sensor(0x0344, 0x0008);	/* X_ADDR_START   =  8 */
	write_cmos_sensor(0x0346, 0x0008);	/* Y_ADDR_START   =  8 */
	write_cmos_sensor(0x0348, 0x0A25);	/* X_ADDR_END      = 2597 */
	write_cmos_sensor(0x034A, 0x079D);	/* Y_ADDR_END       =  1949 */
	temp = read_cmos_sensor(0x3040);
	temp = temp & 0xF000;
	temp = temp | 0x04C3;
	write_cmos_sensor(0x3040, temp);	/* READ_MODE  10 011 000011 xy binning enable xodd=3, yodd=3 */
	write_cmos_sensor(0x034C, 0x0510);	/* X_OUTPUT_SIZE    = 1296 */
	write_cmos_sensor(0x034E, 0x03CC);	/* Y_OUTPUT_SIZE    =  972 */

	write_cmos_sensor(0x300C, 0x0C4F);	/* LINE_LENGTH  3151 */
	write_cmos_sensor(0x300A, 0x044C);	/* FRAME_LINEs  1100 */

	/* write_cmos_sensor(0x3012, 0x0414);    // coarse_integration_time */
	write_cmos_sensor(0x3014, 0x0908);	/* fine_integration_time */
	write_cmos_sensor(0x3010, 0x0184);	/* fine_correction */

	write_cmos_sensor_8(0x0104, 0x00);	/* GROUPED_PARAMETER_HOLD */

	/* start_streaming */
	write_cmos_sensor_8(0x0100, 0x01);	/* MODE_SELECT */

	mDELAY(100);
#else
	preview_setting();
#endif
	LOG_INF("normal_video_setting\n");
}				/*    preview_setting  */

static void hs_video_setting(void)
{
	LOG_INF("E\n");

	preview_setting();
}

static void slim_video_setting(void)
{
	LOG_INF("E\n");

	preview_setting();
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

	LOG_INF("[A5142] >> get_imgsensor_id\n");
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("A5142 i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			LOG_INF("A5142 Read sensor id fail, id: 0x%x\n", *sensor_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	LOG_INF("[A5142] << get_imgsensor_id\n");
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
	LOG_INF("[A5142] >> open\n");
	/* sensor have two i2c address 0x6c 0x6e, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x\n", sensor_id);
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
	LOG_INF("[A5142] << open\n");

	return ERROR_NONE;
}				/*    open  */



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
	LOG_INF("[A5142] >> close\n");

	/*No Need to implement this function */

	return ERROR_NONE;
}				/*    close  */


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
/* LOG_INF("E\n"); */
	LOG_INF("[A5142] >> preview\n");

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
	/* set_mirror_flip(sensor_config_data->SensorImageMirror); */

	LOG_INF("[A5142] << preview\n");
	return ERROR_NONE;
}				/*    preview   */

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
	LOG_INF("[A5142] >> capture\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		/* PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M */
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			/* LOG_INF("Warning: current_fps %d fps is not support,*/
			/* so use cap1's setting: %d fps!\n",imgsensor_info.cap1.max_framerate/10); */
			imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);
	/* set_mirror_flip(sensor_config_data->SensorImageMirror); */

	LOG_INF("[A5142] << capture\n");
	return ERROR_NONE;
}				/* capture() */

static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			       MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
/* LOG_INF("E\n"); */
	LOG_INF("[A5142] >>  normal_video\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	/* imgsensor.current_fps = 300; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	/* set_mirror_flip(sensor_config_data->SensorImageMirror); */

	LOG_INF("[A5142] <<  normal_video\n");
	return ERROR_NONE;
}				/*    normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
/* LOG_INF("E\n"); */
	LOG_INF("[A5142] >>  hs_video\n");
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
	/* set_mirror_flip(sensor_config_data->SensorImageMirror); */
	LOG_INF("[A5142] <<  hs_video\n");
	return ERROR_NONE;
}				/*    hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			     MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	/* LOG_INF("E\n"); */
	LOG_INF("[A5142] >> slim_video\n");
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
	/* set_mirror_flip(sensor_config_data->SensorImageMirror); */
	LOG_INF("[A5142] << slim_video\n");
	return ERROR_NONE;
}				/*    slim_video     */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
/* LOG_INF("E\n"); */
	LOG_INF("[A5142] >> get_resolution\n");

	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight = imgsensor_info.slim_video.grabwindow_height;
	LOG_INF("[A5142] <<  get_resolution\n");

	return ERROR_NONE;
}				/*    get_resolution    */

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("[A5142] >>  get_info, scenario_id = %d\n", scenario_id);


	/* sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; */
	/* sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10;  */
	/* imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate;  */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;	/* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;	/* inverse with datasheet */
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4;	/* not use */
	sensor_info->SensorResetActiveHigh = FALSE;	/* not use */
	sensor_info->SensorResetDelayCount = 5;	/* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	/* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;
	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3;	/* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2;	/* not use */
	sensor_info->SensorPixelClockCount = 3;	/* not use */
	sensor_info->SensorDataLatchCount = 2;	/* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x */
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

		sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

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
		sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

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
	LOG_INF("[A5142] << get_info\n");
	return ERROR_NONE;
}				/*    get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("[A5142] >> control, scenario_id = %d\n", scenario_id);
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
		/* LOG_INF("Error ScenarioId setting"); */
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}

	LOG_INF("[A5142] << control\n");
	return ERROR_NONE;
}				/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{				/* This Function not used after ROME */
	LOG_INF("[A5142] >>  set_video_mode, framerate = %d\n", framerate);
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
	LOG_INF("[A5142] << set_video_mode\n");
	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("[A5142] >> set_auto_flicker_mode, enable = %d, framerate = %d\n", enable,
		framerate);

	spin_lock(&imgsensor_drv_lock);
	if (enable)		/* enable auto flicker */
		imgsensor.autoflicker_en = KAL_TRUE;
	else			/* Cancel Auto flick */
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	/* when enable Test Pattern output, it can not set frame length */
	/* register, and Test Pattern do not need auto flicker function, so return */
	if (imgsensor.test_pattern == KAL_TRUE)
		return ERROR_NONE;

	if (enable) {
		/* enable auto flicker */
		write_cmos_sensor_8(0x0104, 1);
		write_cmos_sensor(0x0340, imgsensor.frame_length + 10);
		write_cmos_sensor_8(0x0104, 0);
	} else {
		/* disable auto flicker */
		write_cmos_sensor_8(0x0104, 1);
		write_cmos_sensor(0x0340, imgsensor.frame_length);
		write_cmos_sensor_8(0x0104, 0);
	}
	LOG_INF("[A5142] <<  set_auto_flicker_mode\n");
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id,
						MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("[A5142] >> set_max_framerate_by_scenario, scenario_id = %d, framerate = %d\n",
		scenario_id, framerate);
	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length =
		    imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		    (frame_length >
		     imgsensor_info.pre.framelength) ? (frame_length -
							imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length =
		    imgsensor_info.normal_video.pclk / framerate * 10 /
		    imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		    (frame_length >
		     imgsensor_info.normal_video.framelength) ? (frame_length -
								 imgsensor_info.normal_video.
								 framelength) : 0;
		imgsensor.frame_length =
		    imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		frame_length =
		    imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		    (frame_length >
		     imgsensor_info.cap.framelength) ? (frame_length -
							imgsensor_info.cap.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length =
		    imgsensor_info.hs_video.pclk / framerate * 10 /
		    imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		    (frame_length >
		     imgsensor_info.hs_video.framelength) ? (frame_length -
							     imgsensor_info.hs_video.
							     framelength) : 0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length =
		    imgsensor_info.slim_video.pclk / framerate * 10 /
		    imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		    (frame_length >
		     imgsensor_info.slim_video.framelength) ? (frame_length -
							       imgsensor_info.slim_video.
							       framelength) : 0;
		imgsensor.frame_length =
		    imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	default:		/* coding with  preview scenario by default */
		frame_length =
		    imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		    (frame_length >
		     imgsensor_info.pre.framelength) ? (frame_length -
							imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		LOG_INF("error scenario_id = %d, we use preview scenario\n", scenario_id);
		break;
	}
	LOG_INF("[A5142] << set_max_framerate_by_scenario\n");
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id,
						    MUINT32 *framerate)
{
	LOG_INF("[A5142] >> get_default_framerate_by_scenario, scenario_id = %d\n", scenario_id);
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
	LOG_INF("[A5142] << get_default_framerate_by_scenario\n");
	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("[A5142] >> set_test_pattern_mode , enable = %d\n", enable);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);

	if (enable)
		write_cmos_sensor(0x0600, 0x0002);
	else
		write_cmos_sensor(0x0600, 0x0000);

	LOG_INF("[A5142] <<  set_test_pattern_mode\n");
	return ERROR_NONE;
}


static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor_8(0x0100, 0x01);
	else
		write_cmos_sensor_8(0x0100, 0x00);

	mdelay(10);
	return ERROR_NONE;
}


static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				  UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;

	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("[A5142] >> feature_control ,feature_id = %d\n", feature_id);
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
		night_mode((BOOL) * feature_data);
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
		set_auto_flicker_mode((BOOL) * feature_data_16, *(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM) *feature_data,
					      *(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM) *feature_data,
						  (MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL) * feature_data_16);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:	/* for factory mode auto testing */
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		LOG_INF("current fps :%d\n", *feature_data_16);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_16;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		LOG_INF("ihdr enable :%d\n", *feature_data_16);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_en = *feature_data_16;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) (*feature_data), (UINT16) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", *feature_data_32);
		wininfo = (SENSOR_WINSIZE_INFO_STRUCT *) (feature_data_32 + 1);

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[1],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[2],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[3],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[4],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0],
			       sizeof(SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n", *feature_data_32,
			*(feature_data_32 + 1), *(feature_data_32 + 2));
		ihdr_write_shutter_gain(*feature_data_32, *(feature_data_32 + 1),
					*(feature_data_32 + 2));
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
			default:
				rate = imgsensor_info.pre.mipi_pixel_rate;
				break;
			}
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = rate;
		}
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
	default:
		break;
	}
	LOG_INF("[A5142] << feature_control\n");
	return ERROR_NONE;
}				/*    feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 A5142_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}				/*    A5142_MIPI_RAW_SensorInit    */
