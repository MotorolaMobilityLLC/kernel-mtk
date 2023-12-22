// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 S5K3P9SPmipi_Sensor.c
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
//#include "../imgsensor_i2c.h"
//#include "imgsensor_common.h"
#include "mot_cancunf_s5k3p9sp04mipiraw_Sensor.h"

#if IS_ENABLED(CONFIG_MTK_CAM_SECURITY_SUPPORT)
#include "imgsensor_ca.h"
#endif

#ifdef VENDOR_EDIT
	#undef VENDOR_EDIT
#endif

#define PFX "S5K3P9SP_camera_sensor"
#define LOG_INF(format, args...) pr_debug(PFX "[%s] " format, __func__, ##args)

#define S5K3P9SP_EEPROM_READ_ID  0xA0
#define S5K3P9SP_EEPROM_WRITE_ID 0xA1

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
		.sensor_id = MOT_CANCUNF_S5K3P9SP04_SENSOR_ID,

		.checksum_value = 0x31e3fbe2,

		.pre = {
			.pclk = 560000000,
			.linelength = 9200,
			.framelength = 2024,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 2320,
			.grabwindow_height = 1740,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 307200000,
			.max_framerate = 300,
		},
		.cap = {
			.pclk = 560000000,
			.linelength = 9200,
			.framelength = 2024,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 2320,
			.grabwindow_height = 1740,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 307200000,
			.max_framerate = 300,
		},
		.normal_video = {
			.pclk = 560000000,
			.linelength = 11520,
			.framelength = 1616,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 2320,
			.grabwindow_height = 1308,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 307200000,
			.max_framerate = 300,
		},
		.hs_video = {
			.pclk = 560000000,
			.linelength = 11520,
			.framelength = 1616,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 2320,
			.grabwindow_height = 1308,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 307200000,
			.max_framerate = 300,
		},
		.slim_video = {
			.pclk = 560000000,
			.linelength = 9200,
			.framelength = 2024,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 2320,
			.grabwindow_height = 1740,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 307200000,
			.max_framerate = 300,
		},
		.margin = 4,
		.min_shutter = 3,
		.min_gain = 64, /*1x gain*/
		.max_gain = 1024, /*16x gain*/
		.min_gain_iso = 100,
		.gain_step = 2,
		.gain_type = 4,
		.max_frame_length = 0xfffb,
		.ae_shut_delay_frame = 0,
		.ae_sensor_gain_delay_frame = 0,
		.ae_ispGain_delay_frame = 2,
		.ihdr_support = 0,/*1, support; 0,not support*/
		.ihdr_le_firstline = 0,/*1,le first; 0, se first*/
		.sensor_mode_num = 5,/*support sensor mode num*/

		.cap_delay_frame = 2,
		.pre_delay_frame = 2,
		.video_delay_frame = 3,
		.hs_video_delay_frame = 3,
		.slim_video_delay_frame = 3,

		.isp_driving_current = ISP_DRIVING_6MA,
		.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
		.mipi_sensor_type = MIPI_OPHY_NCSI2,
		/*0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2*/
		.mipi_settle_delay_mode = 1,
		/*0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL*/
		.sensor_output_dataformat =
			SENSOR_OUTPUT_FORMAT_RAW_Gb,
		.mclk = 24,
		.mipi_lane_num = SENSOR_MIPI_4_LANE,
		.i2c_addr_table = {0x21, 0x20, 0xff},
		.i2c_speed = 1000,
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_V_MIRROR, //mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT,
	/*IMGSENSOR_MODE enum value,record current sensor mode*/
	/*such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video*/
	.shutter = 0x3D0,	/*current shutter*/
	.gain = 0x100,			/*current gain*/
	.dummy_pixel = 0,		/*current dummypixel*/
	.dummy_line = 0,		/*current dummyline*/
	.current_fps = 0,
	/*full size current fps : 24fps for PIP, 30fps for Normal or ZSD*/
	.autoflicker_en = KAL_FALSE,
	/*auto flicker enable:*/
	/*KAL_FALSE for disable auto flicker,KAL_TRUE for enable auto flicker*/
	.test_pattern = 0,
	/*test pattern mode or not.*/
	/*KAL_FALSE for in test pattern mode, KAL_TRUE for normal output*/
	.enable_secure = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	/*current scenario id*/
	.ihdr_mode = 0, /*sensor need support LE, SE with HDR feature*/
	.i2c_write_id = 0x20,
};

/* Sensor output window information */
/*no mirror flip*/
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
	{4640, 3488, 0000, 0004, 4640, 3480, 2320, 1740,
	 0000, 0000, 2320, 1740, 0000, 0000, 2320, 1740},/* Preview */

	{4640, 3488, 0000, 0004, 4640, 3480, 2320, 1740,
	 0000, 0000, 2320, 1740, 0000, 0000, 2320, 1740},/* Capture == Preview*/

	{4640, 3488, 0000, 0436, 4640, 2616, 2320, 1308,
	 0000, 0000, 2320, 1308, 0000, 0000, 2320, 1308},/* Video */

	{4640, 3488, 0000, 0436, 4640, 2616, 2320, 1308,
	 0000, 0000, 2320, 1308, 0000, 0000, 2320, 1308},/* hs_video == Video */

	{4640, 3488, 0000, 0004, 4640, 3480, 2320, 1740,
	 0000, 0000, 2320, 1740, 0000, 0000, 2320, 1740},/* slim_video == Preview */
};

static kal_uint16 read_cmos_sensor_16_16(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};
	/*kdSetI2CSpeed(imgsensor_info.i2c_speed);*/
	/* Add this func to set i2c speed by each sensor*/
	iReadRegI2C(pusendcmd, 2, (u8 *)&get_byte, 2, imgsensor.i2c_write_id);
	return ((get_byte << 8) & 0xff00) | ((get_byte >> 8) & 0x00ff);
}

static void write_cmos_sensor_16_16(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {
		(char)(addr >> 8),
		(char)(addr & 0xFF),
		(char)(para >> 8),
		(char)(para & 0xFF)};
	/* Add this func to set i2c speed by each sensor*/
	iWriteRegI2CTiming(pusendcmd, 4, imgsensor.i2c_write_id,
					imgsensor_info.i2c_speed);
}

static kal_uint16 read_cmos_sensor_16_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};
	/*Add this func to set i2c speed by each sensor*/
	iReadRegI2C(pusendcmd, 2,
		(u8 *)&get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor_16_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[4] = {
		(char)(addr >> 8),
		(char)(addr & 0xFF),
		(char)(para & 0xFF)};
	 /*Add this func to set i2c speed by each sensor*/
	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

#define MULTI_WRITE 1

#if MULTI_WRITE
#define I2C_BUFFER_LEN 225	/* trans# max is 255, each 3 bytes */
#else
#define I2C_BUFFER_LEN 4
#endif

static kal_uint16 table_write_cmos_sensor(kal_uint16 *para, kal_uint32 len)
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
			puSendCmd[tosend++] = (char)(data >> 8);
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;

		}
		#if MULTI_WRITE
/* Write when remain buffer size is less than 4 bytes or reach end of data */
		if ((I2C_BUFFER_LEN - tosend) < 4
				|| IDX == len || addr != addr_last) {
			iBurstWriteReg_multi(puSendCmd,
				tosend, imgsensor.i2c_write_id,
				4, imgsensor_info.i2c_speed);
			tosend = 0;
		}
		#else
		iWriteRegI2CTiming(puSendCmd,
			4, imgsensor.i2c_write_id, imgsensor_info.i2c_speed);
		tosend = 0;

		#endif
	}
	return 0;
}

static kal_uint16 read_cmos_eeprom_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *)&get_byte, 1, S5K3P9SP_EEPROM_READ_ID);
	return get_byte;
}

static void set_dummy(void)
{
	pr_debug("dummyline = %d, dummypixels = %d\n",
		imgsensor.dummy_line, imgsensor.dummy_pixel);
	write_cmos_sensor_16_16(0x0340, imgsensor.frame_length);
	write_cmos_sensor_16_16(0x0342, imgsensor.line_length);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,
	kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	pr_debug("framerate = %d, min framelength should enable %d\n",
		framerate, min_framelength_en);

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

int needSetNormalMode = 0;
/*
 ************************************************************************
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
 ************************************************************************
 */
static void set_shutter(kal_uint32 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_uint32 CintR = 0;
	kal_uint32 Time_Frame = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin) {
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	} else {
		imgsensor.frame_length = imgsensor.min_frame_length;
	}
	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);
	if (shutter < imgsensor_info.min_shutter)
		shutter = imgsensor_info.min_shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
			// Extend frame length
			write_cmos_sensor_16_16(0x0340, imgsensor.frame_length);
		}
	} else {
		// Extend frame length
		write_cmos_sensor_16_16(0x0340, imgsensor.frame_length);
	}

	if (shutter > 0xFFF0) {
		needSetNormalMode = KAL_TRUE;
		if(shutter >= 1947775){
			shutter = 1947775;
		}
		CintR = ((unsigned long long)shutter) / 64;
		Time_Frame = CintR + 0x0004;
		pr_debug("CintR = %d\n", CintR);
		write_cmos_sensor_16_16(0x0340, Time_Frame & 0xFFFF);
		write_cmos_sensor_16_16(0x0202, CintR & 0xFFFF);
		write_cmos_sensor_16_16(0x0702, 0x0600);
		write_cmos_sensor_16_16(0x0704, 0x0600);
		imgsensor.ae_frm_mode.frame_mode_1 = IMGSENSOR_AE_MODE_SE;
		imgsensor.ae_frm_mode.frame_mode_2 = IMGSENSOR_AE_MODE_SE;
		imgsensor.current_ae_effective_frame = 2;
		pr_debug("download long shutter setting shutter = %d\n", shutter);
	} else {
		if (needSetNormalMode == KAL_TRUE) {
			needSetNormalMode = KAL_FALSE;
			write_cmos_sensor_16_16(0x0702, 0x0000);
			write_cmos_sensor_16_16(0x0704, 0x0000);
		}

		pr_debug("return to normal shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);
		write_cmos_sensor_16_16(0x0340, imgsensor.frame_length);
		write_cmos_sensor_16_16(0x0202, imgsensor.shutter);
	}
} /* set_shutter */

/*	write_shutter  */
static void set_shutter_frame_length(
	kal_uint16 shutter, kal_uint16 frame_length)
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

	imgsensor.frame_length =
		imgsensor.frame_length + dummy_line;


	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;


	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;


	spin_unlock(&imgsensor_drv_lock);
	shutter =
		(shutter < imgsensor_info.min_shutter)
		? imgsensor_info.min_shutter
		: shutter;
	shutter =
		(shutter >
		(imgsensor_info.max_frame_length - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin)
		: shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps =
			imgsensor.pclk / imgsensor.line_length *
			10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor_16_16(0x0340,
				imgsensor.frame_length & 0xFFFF);
		}
	} else
		/* Extend frame length */
		write_cmos_sensor_16_16(0x0340,
			imgsensor.frame_length & 0xFFFF);

	/* Update Shutter */
	write_cmos_sensor_16_16(0X0202, shutter & 0xFFFF);

	pr_debug("shutter = %d, framelength = %d/%d, dummy_line= %d\n",
		shutter, imgsensor.frame_length,
		frame_length, dummy_line);

} /*write_shutter*/


static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0;

	reg_gain = gain/4;
	return (kal_uint16)reg_gain;
}

/*
 ************************************************************************
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
 ************************************************************************
 */
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	if (gain < (BASEGAIN * 2) || gain > 32 * (BASEGAIN * 2)) {
		pr_debug("Error gain setting");

		if (gain < (BASEGAIN * 2))
			gain = (BASEGAIN * 2);
		else if (gain > 32 * (BASEGAIN * 2))
			gain = 32 * (BASEGAIN * 2);
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	pr_debug("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor_16_16(0x0204, reg_gain);

	return gain;
}	/*	set_gain  */

/*
 ************************************************************************
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
 ************************************************************************
 */

static kal_uint32 streaming_control(kal_bool enable)
{
	int timeout = (10000 / imgsensor.current_fps) + 1;
	int i = 0;
	int framecnt = 0;

	pr_debug("streaming_enable(0= Sw Standby,1= streaming): %d\n", enable);
	if (enable) {
		//set orientation
		write_cmos_sensor_16_8(0x0101, 0x03);

		write_cmos_sensor_16_8(0x0100, 0x01);
		mDELAY(10);
	} else {
		write_cmos_sensor_16_8(0x0100, 0x00);
		for (i = 0; i < timeout; i++) {
			mDELAY(5);
			framecnt = read_cmos_sensor_16_8(0x0005);
			if (framecnt == 0xFF) {
				pr_debug(" Stream Off OK at i=%d.\n", i);
				return ERROR_NONE;
			}
		}
		pr_debug("Stream Off Fail! framecnt= %d.\n", framecnt);
	}
	return ERROR_NONE;
}

static kal_uint16 addr_data_pair_init[] = {

	0x6214, 0x7970,
	0x6218, 0x7150,
	0x0A02, 0x007E,
	0x6028, 0x2000,
	0x602A, 0x3F4C,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0649,
	0x6F12, 0x0548,
	0x6F12, 0xC1F8,
	0x6F12, 0xC405,
	0x6F12, 0x0549,
	0x6F12, 0x081A,
	0x6F12, 0x0349,
	0x6F12, 0xA1F8,
	0x6F12, 0xC805,
	0x6F12, 0x00F0,
	0x6F12, 0x65BC,
	0x6F12, 0x0000,
	0x6F12, 0x2000,
	0x6F12, 0x4A84,
	0x6F12, 0x2000,
	0x6F12, 0x2ED0,
	0x6F12, 0x2000,
	0x6F12, 0x6C00,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x40BA,
	0x6F12, 0x7047,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0xC0BA,
	0x6F12, 0x7047,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x2DE9,
	0x6F12, 0xF047,
	0x6F12, 0x1C46,
	0x6F12, 0x9046,
	0x6F12, 0x8946,
	0x6F12, 0x0746,
	0x6F12, 0xFE48,
	0x6F12, 0x0022,
	0x6F12, 0x0068,
	0x6F12, 0x86B2,
	0x6F12, 0x050C,
	0x6F12, 0x3146,
	0x6F12, 0x2846,
	0x6F12, 0x00F0,
	0x6F12, 0xC5FC,
	0x6F12, 0x2346,
	0x6F12, 0x4246,
	0x6F12, 0x4946,
	0x6F12, 0x3846,
	0x6F12, 0x00F0,
	0x6F12, 0xC4FC,
	0x6F12, 0xF848,
	0x6F12, 0x90F8,
	0x6F12, 0x8B02,
	0x6F12, 0x88B1,
	0x6F12, 0x788A,
	0x6F12, 0x04F1,
	0x6F12, 0x0054,
	0x6F12, 0x04EB,
	0x6F12, 0x8001,
	0x6F12, 0x09E0,
	0x6F12, 0x2268,
	0x6F12, 0xC2F3,
	0x6F12, 0xC360,
	0x6F12, 0x90FA,
	0x6F12, 0xA0F0,
	0x6F12, 0x22F0,
	0x6F12, 0x7842,
	0x6F12, 0x42EA,
	0x6F12, 0x5000,
	0x6F12, 0x01C4,
	0x6F12, 0x8C42,
	0x6F12, 0xF3D1,
	0x6F12, 0x3146,
	0x6F12, 0x2846,
	0x6F12, 0xBDE8,
	0x6F12, 0xF047,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0xA2BC,
	0x6F12, 0x2DE9,
	0x6F12, 0xFC5F,
	0x6F12, 0x8346,
	0x6F12, 0xE748,
	0x6F12, 0x8A46,
	0x6F12, 0x0022,
	0x6F12, 0x4068,
	0x6F12, 0x010C,
	0x6F12, 0x80B2,
	0x6F12, 0xCDE9,
	0x6F12, 0x0001,
	0x6F12, 0x0146,
	0x6F12, 0x0198,
	0x6F12, 0x00F0,
	0x6F12, 0x93FC,
	0x6F12, 0xABFB,
	0x6F12, 0x0A10,
	0x6F12, 0xE24B,
	0x6F12, 0xE04D,
	0x6F12, 0xE04A,
	0x6F12, 0x93F8,
	0x6F12, 0x9160,
	0x6F12, 0x05F5,
	0x6F12, 0xAA69,
	0x6F12, 0x06FB,
	0x6F12, 0x0BF6,
	0x6F12, 0x0023,
	0x6F12, 0x891B,
	0x6F12, 0x4D46,
	0x6F12, 0x60EB,
	0x6F12, 0x0300,
	0x6F12, 0x03C5,
	0x6F12, 0x1D46,
	0x6F12, 0xEBFB,
	0x6F12, 0x0A65,
	0x6F12, 0x02F5,
	0x6F12, 0xAB67,
	0x6F12, 0x3A46,
	0x6F12, 0xD64C,
	0x6F12, 0x60C2,
	0x6F12, 0xA4F8,
	0x6F12, 0x4835,
	0x6F12, 0x04F5,
	0x6F12, 0xA962,
	0x6F12, 0x94F8,
	0x6F12, 0xA0C4,
	0x6F12, 0x94F8,
	0x6F12, 0xA144,
	0x6F12, 0x4FF4,
	0x6F12, 0xF858,
	0x6F12, 0xBCF1,
	0x6F12, 0x010F,
	0x6F12, 0x03D0,
	0x6F12, 0xBCF1,
	0x6F12, 0x020F,
	0x6F12, 0x14D0,
	0x6F12, 0x29E0,
	0x6F12, 0x08EA,
	0x6F12, 0x0423,
	0x6F12, 0x43F0,
	0x6F12, 0x1103,
	0x6F12, 0x1380,
	0x6F12, 0x2346,
	0x6F12, 0x0022,
	0x6F12, 0x00F0,
	0x6F12, 0x6AFC,
	0x6F12, 0xC9E9,
	0x6F12, 0x0001,
	0x6F12, 0x2346,
	0x6F12, 0x0022,
	0x6F12, 0x2846,
	0x6F12, 0x3146,
	0x6F12, 0x00F0,
	0x6F12, 0x62FC,
	0x6F12, 0xC7E9,
	0x6F12, 0x0001,
	0x6F12, 0x15E0,
	0x6F12, 0x08EA,
	0x6F12, 0x042C,
	0x6F12, 0x4CF0,
	0x6F12, 0x010C,
	0x6F12, 0xA2F8,
	0x6F12, 0x00C0,
	0x6F12, 0xA1FB,
	0x6F12, 0x042C,
	0x6F12, 0x00FB,
	0x6F12, 0x04C0,
	0x6F12, 0x01FB,
	0x6F12, 0x0301,
	0x6F12, 0xC9E9,
	0x6F12, 0x0012,
	0x6F12, 0xA6FB,
	0x6F12, 0x0401,
	0x6F12, 0x05FB,
	0x6F12, 0x0411,
	0x6F12, 0x06FB,
	0x6F12, 0x0311,
	0x6F12, 0xC7E9,
	0x6F12, 0x0010,
	0x6F12, 0xB848,
	0x6F12, 0xB949,
	0x6F12, 0xB0F8,
	0x6F12, 0x4805,
	0x6F12, 0x0880,
	0x6F12, 0xB848,
	0x6F12, 0x0CC8,
	0x6F12, 0x48F6,
	0x6F12, 0x2200,
	0x6F12, 0x00F0,
	0x6F12, 0x43FC,
	0x6F12, 0xB548,
	0x6F12, 0x0830,
	0x6F12, 0x0CC8,
	0x6F12, 0x48F6,
	0x6F12, 0x2A00,
	0x6F12, 0x00F0,
	0x6F12, 0x3CFC,
	0x6F12, 0x5846,
	0x6F12, 0x00F0,
	0x6F12, 0x3EFC,
	0x6F12, 0xAD49,
	0x6F12, 0x0122,
	0x6F12, 0xC1F8,
	0x6F12, 0x68A5,
	0x6F12, 0xDDE9,
	0x6F12, 0x0010,
	0x6F12, 0x02B0,
	0x6F12, 0xBDE8,
	0x6F12, 0xF05F,
	0x6F12, 0x00F0,
	0x6F12, 0x1FBC,
	0x6F12, 0xA84A,
	0x6F12, 0x92F8,
	0x6F12, 0xD525,
	0x6F12, 0x2AB1,
	0x6F12, 0xA64A,
	0x6F12, 0xA54B,
	0x6F12, 0xD2F8,
	0x6F12, 0x6825,
	0x6F12, 0xC3F8,
	0x6F12, 0x3024,
	0x6F12, 0xA34A,
	0x6F12, 0xD2F8,
	0x6F12, 0x3024,
	0x6F12, 0x00F0,
	0x6F12, 0x29BC,
	0x6F12, 0x10B5,
	0x6F12, 0xA049,
	0x6F12, 0xA34A,
	0x6F12, 0xA44B,
	0x6F12, 0xD1F8,
	0x6F12, 0x3C14,
	0x6F12, 0x947C,
	0x6F12, 0x0CB1,
	0x6F12, 0x908A,
	0x6F12, 0x1BE0,
	0x6F12, 0x9B4A,
	0x6F12, 0x92F8,
	0x6F12, 0xA220,
	0x6F12, 0xC2F1,
	0x6F12, 0x0C02,
	0x6F12, 0xD140,
	0x6F12, 0x4843,
	0x6F12, 0x010A,
	0x6F12, 0x9D48,
	0x6F12, 0xD0F8,
	0x6F12, 0x8400,
	0x6F12, 0x0279,
	0x6F12, 0x4A43,
	0x6F12, 0x4179,
	0x6F12, 0xC088,
	0x6F12, 0xCA40,
	0x6F12, 0x00EB,
	0x6F12, 0x1210,
	0x6F12, 0x4FF4,
	0x6F12, 0x8021,
	0x6F12, 0xB1FB,
	0x6F12, 0xF0F0,
	0x6F12, 0x0911,
	0x6F12, 0x8842,
	0x6F12, 0x04D2,
	0x6F12, 0x4028,
	0x6F12, 0x00D8,
	0x6F12, 0x4020,
	0x6F12, 0x5880,
	0x6F12, 0x10BD,
	0x6F12, 0x0846,
	0x6F12, 0xFBE7,
	0x6F12, 0x4168,
	0x6F12, 0x4A7B,
	0x6F12, 0x9149,
	0x6F12, 0xA1F8,
	0x6F12, 0x8223,
	0x6F12, 0x4268,
	0x6F12, 0x537B,
	0x6F12, 0x002B,
	0x6F12, 0x15D0,
	0x6F12, 0x01F5,
	0x6F12, 0x6171,
	0x6F12, 0x927B,
	0x6F12, 0x0A80,
	0x6F12, 0x4068,
	0x6F12, 0xC07B,
	0x6F12, 0x4880,
	0x6F12, 0x8B48,
	0x6F12, 0xB0F8,
	0x6F12, 0xC220,
	0x6F12, 0x8A80,
	0x6F12, 0xB0F8,
	0x6F12, 0xC420,
	0x6F12, 0xCA80,
	0x6F12, 0x10F8,
	0x6F12, 0xC72F,
	0x6F12, 0xC078,
	0x6F12, 0x5208,
	0x6F12, 0x4008,
	0x6F12, 0x42EA,
	0x6F12, 0x8000,
	0x6F12, 0x0881,
	0x6F12, 0x7047,
	0x6F12, 0x2DE9,
	0x6F12, 0xFF4F,
	0x6F12, 0x8348,
	0x6F12, 0x83B0,
	0x6F12, 0x1D46,
	0x6F12, 0xC079,
	0x6F12, 0xDDF8,
	0x6F12, 0x44B0,
	0x6F12, 0x1646,
	0x6F12, 0x0F46,
	0x6F12, 0x0028,
	0x6F12, 0x6ED0,
	0x6F12, 0xDFF8,
	0x6F12, 0xF4A1,
	0x6F12, 0x0AF1,
	0x6F12, 0xBA0A,
	0x6F12, 0xAAF1,
	0x6F12, 0x1C00,
	0x6F12, 0xB0F8,
	0x6F12, 0x0090,
	0x6F12, 0xB0F8,
	0x6F12, 0x0480,
	0x6F12, 0x00F0,
	0x6F12, 0xCCFB,
	0x6F12, 0x0399,
	0x6F12, 0x109C,
	0x6F12, 0x0843,
	0x6F12, 0x04F1,
	0x6F12, 0x8044,
	0x6F12, 0x07D0,
	0x6F12, 0xA780,
	0x6F12, 0xE680,
	0x6F12, 0xAAF1,
	0x6F12, 0x1C00,
	0x6F12, 0x0188,
	0x6F12, 0x2181,
	0x6F12, 0x8088,
	0x6F12, 0x20E0,
	0x6F12, 0x6848,
	0x6F12, 0x9AF8,
	0x6F12, 0x0C10,
	0x6F12, 0xB0F8,
	0x6F12, 0xD801,
	0x6F12, 0x4843,
	0x6F12, 0x0290,
	0x6F12, 0x00F0,
	0x6F12, 0xBAFB,
	0x6F12, 0x0028,
	0x6F12, 0x0298,
	0x6F12, 0x01D0,
	0x6F12, 0x361A,
	0x6F12, 0x00E0,
	0x6F12, 0x0744,
	0x6F12, 0xA780,
	0x6F12, 0xE680,
	0x6F12, 0x6048,
	0x6F12, 0xB0F8,
	0x6F12, 0xDA61,
	0x6F12, 0x90F8,
	0x6F12, 0x8A02,
	0x6F12, 0x4643,
	0x6F12, 0x00F0,
	0x6F12, 0xAFFB,
	0x6F12, 0x10B1,
	0x6F12, 0xA8EB,
	0x6F12, 0x0608,
	0x6F12, 0x00E0,
	0x6F12, 0xB144,
	0x6F12, 0xA4F8,
	0x6F12, 0x0890,
	0x6F12, 0x4046,
	0x6F12, 0x6081,
	0x6F12, 0x0398,
	0x6F12, 0x28B1,
	0x6F12, 0x5648,
	0x6F12, 0x90F8,
	0x6F12, 0x4F11,
	0x6F12, 0x90F8,
	0x6F12, 0x8902,
	0x6F12, 0x03E0,
	0x6F12, 0x9AF8,
	0x6F12, 0x0D10,
	0x6F12, 0x9AF8,
	0x6F12, 0x0C00,
	0x6F12, 0x0A01,
	0x6F12, 0x5149,
	0x6F12, 0x91F8,
	0x6F12, 0x4E11,
	0x6F12, 0x42EA,
	0x6F12, 0x8121,
	0x6F12, 0x41F0,
	0x6F12, 0x0301,
	0x6F12, 0xA181,
	0x6F12, 0x0121,
	0x6F12, 0xFF22,
	0x6F12, 0x02EB,
	0x6F12, 0x4000,
	0x6F12, 0x41EA,
	0x6F12, 0x0020,
	0x6F12, 0xE081,
	0x6F12, 0x01A9,
	0x6F12, 0x6846,
	0x6F12, 0x00F0,
	0x6F12, 0x8BFB,
	0x6F12, 0x9DF8,
	0x6F12, 0x0000,
	0x6F12, 0x9DF8,
	0x6F12, 0x0410,
	0x6F12, 0x40EA,
	0x6F12, 0x0120,
	0x6F12, 0x2082,
	0x6F12, 0x5F46,
	0x6F12, 0x3E46,
	0x6F12, 0x00F0,
	0x6F12, 0x7BFB,
	0x6F12, 0x791E,
	0x6F12, 0x0028,
	0x6F12, 0xA889,
	0x6F12, 0x04D0,
	0x6F12, 0x4718,
	0x6F12, 0x46F6,
	0x6F12, 0xA410,
	0x6F12, 0x03E0,
	0x6F12, 0x3FE0,
	0x6F12, 0x4618,
	0x6F12, 0x46F6,
	0x6F12, 0x2410,
	0x6F12, 0xA880,
	0x6F12, 0x6782,
	0x6F12, 0xE682,
	0x6F12, 0x0020,
	0x6F12, 0xA082,
	0x6F12, 0xA888,
	0x6F12, 0x2080,
	0x6F12, 0x00F0,
	0x6F12, 0x70FB,
	0x6F12, 0x0128,
	0x6F12, 0x0CD1,
	0x6F12, 0x00F0,
	0x6F12, 0x71FB,
	0x6F12, 0x48B1,
	0x6F12, 0x00F0,
	0x6F12, 0x73FB,
	0x6F12, 0x30B1,
	0x6F12, 0x40F2,
	0x6F12, 0x1340,
	0x6F12, 0xA081,
	0x6F12, 0x40F2,
	0x6F12, 0x0110,
	0x6F12, 0xE081,
	0x6F12, 0x2082,
	0x6F12, 0x2B6A,
	0x6F12, 0x0021,
	0x6F12, 0x8320,
	0x6F12, 0x109A,
	0x6F12, 0x00F0,
	0x6F12, 0x6AFB,
	0x6F12, 0xE881,
	0x6F12, 0x00F0,
	0x6F12, 0x58FB,
	0x6F12, 0x0126,
	0x6F12, 0x0128,
	0x6F12, 0x12D1,
	0x6F12, 0x00F0,
	0x6F12, 0x58FB,
	0x6F12, 0x78B1,
	0x6F12, 0x00F0,
	0x6F12, 0x5AFB,
	0x6F12, 0x60B1,
	0x6F12, 0x2680,
	0x6F12, 0x3048,
	0x6F12, 0x0021,
	0x6F12, 0x04E0,
	0x6F12, 0x0288,
	0x6F12, 0x5208,
	0x6F12, 0x20F8,
	0x6F12, 0x022B,
	0x6F12, 0x491C,
	0x6F12, 0xEA89,
	0x6F12, 0xB1EB,
	0x6F12, 0x420F,
	0x6F12, 0xF6DB,
	0x6F12, 0xE989,
	0x6F12, 0xA889,
	0x6F12, 0x8142,
	0x6F12, 0x00D9,
	0x6F12, 0xE881,
	0x6F12, 0x2680,
	0x6F12, 0x07B0,
	0x6F12, 0xBDE8,
	0x6F12, 0xF08F,
	0x6F12, 0x2DE9,
	0x6F12, 0xF843,
	0x6F12, 0x1A48,
	0x6F12, 0x0022,
	0x6F12, 0x4069,
	0x6F12, 0x85B2,
	0x6F12, 0x4FEA,
	0x6F12, 0x1048,
	0x6F12, 0x2946,
	0x6F12, 0x4046,
	0x6F12, 0x00F0,
	0x6F12, 0xFBFA,
	0x6F12, 0x00F0,
	0x6F12, 0x3FFB,
	0x6F12, 0x204F,
	0x6F12, 0x97F8,
	0x6F12, 0x7300,
	0x6F12, 0x30B1,
	0x6F12, 0x1348,
	0x6F12, 0x90F8,
	0x6F12, 0x8B02,
	0x6F12, 0x10B1,
	0x6F12, 0x1D49,
	0x6F12, 0x1B20,
	0x6F12, 0x0880,
	0x6F12, 0x1C48,
	0x6F12, 0x0E4E,
	0x6F12, 0x3436,
	0x6F12, 0x90F8,
	0x6F12, 0xC046,
	0x6F12, 0xB089,
	0x6F12, 0x98B9,
	0x6F12, 0x0020,
	0x6F12, 0xADF8,
	0x6F12, 0x0000,
	0x6F12, 0x0A48,
	0x6F12, 0x0222,
	0x6F12, 0x6946,
	0x6F12, 0xB0F8,
	0x6F12, 0x0006,
	0x6F12, 0x2E30,
	0x6F12, 0x00F0,
	0x6F12, 0x27FB,
	0x6F12, 0x10B1,
	0x6F12, 0xBDF8,
	0x6F12, 0x0000,
	0x6F12, 0xB081,
	0x6F12, 0xB089,
	0x6F12, 0x10B9,
	0x6F12, 0x4FF4,
	0x6F12, 0x8060,
	0x6F12, 0xB081,
	0x6F12, 0x97F8,
	0x6F12, 0x7500,
	0x6F12, 0x1DE0,
	0x6F12, 0x2000,
	0x6F12, 0x4A40,
	0x6F12, 0x2000,
	0x6F12, 0x2ED0,
	0x6F12, 0x2000,
	0x6F12, 0x0E20,
	0x6F12, 0x4000,
	0x6F12, 0x8832,
	0x6F12, 0x2000,
	0x6F12, 0x3420,
	0x6F12, 0x2000,
	0x6F12, 0x21A0,
	0x6F12, 0x2000,
	0x6F12, 0x3F40,
	0x6F12, 0x2000,
	0x6F12, 0x3E70,
	0x6F12, 0x4000,
	0x6F12, 0xA000,
	0x6F12, 0x2000,
	0x6F12, 0x38C0,
	0x6F12, 0x2000,
	0x6F12, 0x2210,
	0x6F12, 0x2000,
	0x6F12, 0x8000,
	0x6F12, 0x2000,
	0x6F12, 0x2850,
	0x6F12, 0x4000,
	0x6F12, 0xF47E,
	0x6F12, 0x2000,
	0x6F12, 0x0FE0,
	0x6F12, 0x28B1,
	0x6F12, 0xB089,
	0x6F12, 0x18B1,
	0x6F12, 0x6043,
	0x6F12, 0x00F5,
	0x6F12, 0x0070,
	0x6F12, 0x840A,
	0x6F12, 0xFE48,
	0x6F12, 0x4FF4,
	0x6F12, 0x8072,
	0x6F12, 0xB0F8,
	0x6F12, 0x7C07,
	0x6F12, 0x9042,
	0x6F12, 0x01D9,
	0x6F12, 0x0146,
	0x6F12, 0x00E0,
	0x6F12, 0x1146,
	0x6F12, 0x8B01,
	0x6F12, 0xA3F5,
	0x6F12, 0x8043,
	0x6F12, 0x9042,
	0x6F12, 0x01D9,
	0x6F12, 0x0146,
	0x6F12, 0x00E0,
	0x6F12, 0x1146,
	0x6F12, 0x01FB,
	0x6F12, 0x0431,
	0x6F12, 0xFF23,
	0x6F12, 0xB3EB,
	0x6F12, 0x112F,
	0x6F12, 0x0ED9,
	0x6F12, 0x9042,
	0x6F12, 0x01D9,
	0x6F12, 0x0146,
	0x6F12, 0x00E0,
	0x6F12, 0x1146,
	0x6F12, 0x8901,
	0x6F12, 0xA1F5,
	0x6F12, 0x8041,
	0x6F12, 0x9042,
	0x6F12, 0x00D8,
	0x6F12, 0x1046,
	0x6F12, 0x00FB,
	0x6F12, 0x0410,
	0x6F12, 0x000A,
	0x6F12, 0x00E0,
	0x6F12, 0xFF20,
	0x6F12, 0xEB49,
	0x6F12, 0x0880,
	0x6F12, 0x2946,
	0x6F12, 0x4046,
	0x6F12, 0xBDE8,
	0x6F12, 0xF843,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0x7ABA,
	0x6F12, 0x70B5,
	0x6F12, 0xE748,
	0x6F12, 0x0022,
	0x6F12, 0x8169,
	0x6F12, 0x0C0C,
	0x6F12, 0x8DB2,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0x00F0,
	0x6F12, 0x70FA,
	0x6F12, 0x00F0,
	0x6F12, 0xBEFA,
	0x6F12, 0xE248,
	0x6F12, 0x90F8,
	0x6F12, 0x7410,
	0x6F12, 0x11B1,
	0x6F12, 0x0021,
	0x6F12, 0x80F8,
	0x6F12, 0x7010,
	0x6F12, 0xE048,
	0x6F12, 0x4FF4,
	0x6F12, 0x8071,
	0x6F12, 0x90F8,
	0x6F12, 0x6F20,
	0x6F12, 0x4FF4,
	0x6F12, 0x3040,
	0x6F12, 0x00F0,
	0x6F12, 0x5EFA,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0xBDE8,
	0x6F12, 0x7040,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0x57BA,
	0x6F12, 0x70B5,
	0x6F12, 0x0446,
	0x6F12, 0xD648,
	0x6F12, 0xD74D,
	0x6F12, 0x90F8,
	0x6F12, 0x0804,
	0x6F12, 0xC8B1,
	0x6F12, 0x2846,
	0x6F12, 0x90F8,
	0x6F12, 0x0906,
	0x6F12, 0xA8B1,
	0x6F12, 0x2846,
	0x6F12, 0xD5F8,
	0x6F12, 0x8423,
	0x6F12, 0xC0F8,
	0x6F12, 0x1424,
	0x6F12, 0x00F2,
	0x6F12, 0x1441,
	0x6F12, 0x2A46,
	0x6F12, 0xD5F8,
	0x6F12, 0x9003,
	0x6F12, 0xC2F8,
	0x6F12, 0x2004,
	0x6F12, 0xD5F8,
	0x6F12, 0xC043,
	0x6F12, 0x1046,
	0x6F12, 0xC5F8,
	0x6F12, 0xE442,
	0x6F12, 0xC0F8,
	0x6F12, 0x3044,
	0x6F12, 0x0846,
	0x6F12, 0x00F0,
	0x6F12, 0x8BFA,
	0x6F12, 0xC749,
	0x6F12, 0xB5F8,
	0x6F12, 0xB022,
	0x6F12, 0x088F,
	0x6F12, 0x498F,
	0x6F12, 0x201A,
	0x6F12, 0x401E,
	0x6F12, 0x1144,
	0x6F12, 0x8142,
	0x6F12, 0x00D9,
	0x6F12, 0x0846,
	0x6F12, 0xA5F8,
	0x6F12, 0xB202,
	0x6F12, 0x70BD,
	0x6F12, 0x2DE9,
	0x6F12, 0xF041,
	0x6F12, 0x0646,
	0x6F12, 0xBD48,
	0x6F12, 0x0022,
	0x6F12, 0x006A,
	0x6F12, 0x85B2,
	0x6F12, 0x040C,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0x00F0,
	0x6F12, 0x1CFA,
	0x6F12, 0x3046,
	0x6F12, 0x00F0,
	0x6F12, 0x73FA,
	0x6F12, 0xBB48,
	0x6F12, 0xBB4F,
	0x6F12, 0x0068,
	0x6F12, 0x3B68,
	0x6F12, 0x418B,
	0x6F12, 0x090A,
	0x6F12, 0x83F8,
	0x6F12, 0x3610,
	0x6F12, 0xC17E,
	0x6F12, 0x83F8,
	0x6F12, 0x3810,
	0x6F12, 0xB449,
	0x6F12, 0x91F8,
	0x6F12, 0x4C21,
	0x6F12, 0x002A,
	0x6F12, 0xD1F8,
	0x6F12, 0x3421,
	0x6F12, 0x01D0,
	0x6F12, 0x521C,
	0x6F12, 0x5208,
	0x6F12, 0xCE33,
	0x6F12, 0x160A,
	0x6F12, 0x1E71,
	0x6F12, 0x9A71,
	0x6F12, 0xB1F8,
	0x6F12, 0x3C21,
	0x6F12, 0xC2F3,
	0x6F12, 0x5712,
	0x6F12, 0x1A70,
	0x6F12, 0x91F8,
	0x6F12, 0x3D21,
	0x6F12, 0xD200,
	0x6F12, 0x9A70,
	0x6F12, 0x91F8,
	0x6F12, 0x4D21,
	0x6F12, 0xCE3B,
	0x6F12, 0x22B1,
	0x6F12, 0xD1F8,
	0x6F12, 0x3821,
	0x6F12, 0x521C,
	0x6F12, 0x5608,
	0x6F12, 0x01E0,
	0x6F12, 0xD1F8,
	0x6F12, 0x3861,
	0x6F12, 0x7A68,
	0x6F12, 0x4FEA,
	0x6F12, 0x162C,
	0x6F12, 0x01F5,
	0x6F12, 0x9071,
	0x6F12, 0x82F8,
	0x6F12, 0x16C0,
	0x6F12, 0x1676,
	0x6F12, 0xCE8B,
	0x6F12, 0x00F5,
	0x6F12, 0xBA70,
	0x6F12, 0xC6F3,
	0x6F12, 0x5716,
	0x6F12, 0x9674,
	0x6F12, 0xCE7F,
	0x6F12, 0xF600,
	0x6F12, 0x1675,
	0x6F12, 0x0E8C,
	0x6F12, 0xCF68,
	0x6F12, 0xF608,
	0x6F12, 0x7E43,
	0x6F12, 0x360B,
	0x6F12, 0x370A,
	0x6F12, 0x03F8,
	0x6F12, 0xD67F,
	0x6F12, 0x7732,
	0x6F12, 0x9E70,
	0x6F12, 0x0688,
	0x6F12, 0x360A,
	0x6F12, 0x02F8,
	0x6F12, 0x276C,
	0x6F12, 0x4678,
	0x6F12, 0x02F8,
	0x6F12, 0x256C,
	0x6F12, 0x8688,
	0x6F12, 0x360A,
	0x6F12, 0x02F8,
	0x6F12, 0x1F6C,
	0x6F12, 0x4679,
	0x6F12, 0x02F8,
	0x6F12, 0x1D6C,
	0x6F12, 0x8F4E,
	0x6F12, 0x96F8,
	0x6F12, 0x1064,
	0x6F12, 0xD671,
	0x6F12, 0x8D4E,
	0x6F12, 0x96F8,
	0x6F12, 0x1164,
	0x6F12, 0x5672,
	0x6F12, 0x8B4E,
	0x6F12, 0x96F8,
	0x6F12, 0x0B64,
	0x6F12, 0xD672,
	0x6F12, 0x894E,
	0x6F12, 0x96F8,
	0x6F12, 0x0964,
	0x6F12, 0x5673,
	0x6F12, 0x90F8,
	0x6F12, 0x3060,
	0x6F12, 0xD673,
	0x6F12, 0x90F8,
	0x6F12, 0xDE00,
	0x6F12, 0x02F8,
	0x6F12, 0x1F0F,
	0x6F12, 0x8448,
	0x6F12, 0x00F2,
	0x6F12, 0x7246,
	0x6F12, 0x90F8,
	0x6F12, 0x7204,
	0x6F12, 0x9074,
	0x6F12, 0x3078,
	0x6F12, 0x1075,
	0x6F12, 0xA522,
	0x6F12, 0xDA70,
	0x6F12, 0x0E20,
	0x6F12, 0x1871,
	0x6F12, 0x11F8,
	0x6F12, 0x7E0C,
	0x6F12, 0xC0F1,
	0x6F12, 0x0C01,
	0x6F12, 0x7C48,
	0x6F12, 0xD0F8,
	0x6F12, 0x3C04,
	0x6F12, 0xC840,
	0x6F12, 0x060A,
	0x6F12, 0x9E71,
	0x6F12, 0x1872,
	0x6F12, 0x0120,
	0x6F12, 0x03F8,
	0x6F12, 0x2C0C,
	0x6F12, 0x7748,
	0x6F12, 0xD0F8,
	0x6F12, 0x4C04,
	0x6F12, 0xC840,
	0x6F12, 0xAA21,
	0x6F12, 0x03F8,
	0x6F12, 0x571D,
	0x6F12, 0x0226,
	0x6F12, 0x5E70,
	0x6F12, 0x9A70,
	0x6F12, 0x3022,
	0x6F12, 0xDA70,
	0x6F12, 0x5A22,
	0x6F12, 0x1A71,
	0x6F12, 0x060A,
	0x6F12, 0x5E71,
	0x6F12, 0x9A71,
	0x6F12, 0xD871,
	0x6F12, 0x1972,
	0x6F12, 0x0020,
	0x6F12, 0x5872,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0xBDE8,
	0x6F12, 0xF041,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0x77B9,
	0x6F12, 0x2DE9,
	0x6F12, 0xF041,
	0x6F12, 0x0746,
	0x6F12, 0x6448,
	0x6F12, 0x0C46,
	0x6F12, 0x0022,
	0x6F12, 0x406A,
	0x6F12, 0x86B2,
	0x6F12, 0x050C,
	0x6F12, 0x3146,
	0x6F12, 0x2846,
	0x6F12, 0x00F0,
	0x6F12, 0x6AF9,
	0x6F12, 0x2146,
	0x6F12, 0x3846,
	0x6F12, 0x00F0,
	0x6F12, 0xC5F9,
	0x6F12, 0x6048,
	0x6F12, 0x90F8,
	0x6F12, 0x9702,
	0x6F12, 0x10B9,
	0x6F12, 0x00F0,
	0x6F12, 0x97F9,
	0x6F12, 0x20B1,
	0x6F12, 0x04F1,
	0x6F12, 0x8044,
	0x6F12, 0xA08A,
	0x6F12, 0x401C,
	0x6F12, 0xA082,
	0x6F12, 0x3146,
	0x6F12, 0x2846,
	0x6F12, 0xBDE8,
	0x6F12, 0xF041,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0x53B9,
	0x6F12, 0x2DE9,
	0x6F12, 0xF041,
	0x6F12, 0x0746,
	0x6F12, 0x5248,
	0x6F12, 0x0E46,
	0x6F12, 0x0022,
	0x6F12, 0x806A,
	0x6F12, 0x85B2,
	0x6F12, 0x040C,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0x00F0,
	0x6F12, 0x46F9,
	0x6F12, 0x3146,
	0x6F12, 0x3846,
	0x6F12, 0x00F0,
	0x6F12, 0xA6F9,
	0x6F12, 0x4B4F,
	0x6F12, 0x4DF2,
	0x6F12, 0x0C26,
	0x6F12, 0x3437,
	0x6F12, 0x4FF4,
	0x6F12, 0x8061,
	0x6F12, 0x3A78,
	0x6F12, 0x3046,
	0x6F12, 0x00F0,
	0x6F12, 0x38F9,
	0x6F12, 0x7878,
	0x6F12, 0xC8B3,
	0x6F12, 0x0022,
	0x6F12, 0x4FF4,
	0x6F12, 0x0071,
	0x6F12, 0x3046,
	0x6F12, 0x00F0,
	0x6F12, 0x30F9,
	0x6F12, 0x4848,
	0x6F12, 0x0088,
	0x6F12, 0x484B,
	0x6F12, 0xA3F8,
	0x6F12, 0x4402,
	0x6F12, 0x4648,
	0x6F12, 0x001D,
	0x6F12, 0x0088,
	0x6F12, 0xA3F8,
	0x6F12, 0x4602,
	0x6F12, 0xB3F8,
	0x6F12, 0x4402,
	0x6F12, 0xB3F8,
	0x6F12, 0x4612,
	0x6F12, 0x4218,
	0x6F12, 0x02D0,
	0x6F12, 0x8002,
	0x6F12, 0xB0FB,
	0x6F12, 0xF2F2,
	0x6F12, 0x91B2,
	0x6F12, 0x404A,
	0x6F12, 0xA3F8,
	0x6F12, 0x4812,
	0x6F12, 0x5088,
	0x6F12, 0x1288,
	0x6F12, 0x3D4B,
	0x6F12, 0xA3F8,
	0x6F12, 0xA605,
	0x6F12, 0xA3F8,
	0x6F12, 0xA825,
	0x6F12, 0x8018,
	0x6F12, 0x05D0,
	0x6F12, 0x9202,
	0x6F12, 0xB2FB,
	0x6F12, 0xF0F0,
	0x6F12, 0x1A46,
	0x6F12, 0xA2F8,
	0x6F12, 0xAA05,
	0x6F12, 0x3648,
	0x6F12, 0xB0F8,
	0x6F12, 0xAA05,
	0x6F12, 0x0A18,
	0x6F12, 0x01FB,
	0x6F12, 0x1020,
	0x6F12, 0x40F3,
	0x6F12, 0x9510,
	0x6F12, 0x1028,
	0x6F12, 0x06DC,
	0x6F12, 0x0028,
	0x6F12, 0x05DA,
	0x6F12, 0x0020,
	0x6F12, 0x03E0,
	0x6F12, 0xFFE7,
	0x6F12, 0x0122,
	0x6F12, 0xC3E7,
	0x6F12, 0x1020,
	0x6F12, 0x2F49,
	0x6F12, 0x0880,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0xBDE8,
	0x6F12, 0xF041,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0xEFB8,
	0x6F12, 0x70B5,
	0x6F12, 0x2148,
	0x6F12, 0x0022,
	0x6F12, 0xC16A,
	0x6F12, 0x0C0C,
	0x6F12, 0x8DB2,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0x00F0,
	0x6F12, 0xE5F8,
	0x6F12, 0x2148,
	0x6F12, 0x0268,
	0x6F12, 0xB2F8,
	0x6F12, 0x6202,
	0x6F12, 0x8301,
	0x6F12, 0x92F8,
	0x6F12, 0x6002,
	0x6F12, 0x10F0,
	0x6F12, 0x020F,
	0x6F12, 0x09D0,
	0x6F12, 0x1848,
	0x6F12, 0x3430,
	0x6F12, 0x8188,
	0x6F12, 0x9942,
	0x6F12, 0x06D8,
	0x6F12, 0x4088,
	0x6F12, 0xA0F5,
	0x6F12, 0x5141,
	0x6F12, 0x2339,
	0x6F12, 0x01D1,
	0x6F12, 0x00F0,
	0x6F12, 0x38F9,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0xBDE8,
	0x6F12, 0x7040,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0xC8B8,
	0x6F12, 0x70B5,
	0x6F12, 0x0646,
	0x6F12, 0x0D48,
	0x6F12, 0x0022,
	0x6F12, 0x016B,
	0x6F12, 0x0C0C,
	0x6F12, 0x8DB2,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0x00F0,
	0x6F12, 0xBDF8,
	0x6F12, 0x3046,
	0x6F12, 0x00F0,
	0x6F12, 0x28F9,
	0x6F12, 0x0749,
	0x6F12, 0x114A,
	0x6F12, 0x3431,
	0x6F12, 0xCB79,
	0x6F12, 0xD068,
	0x6F12, 0x9840,
	0x6F12, 0xD060,
	0x6F12, 0x1068,
	0x6F12, 0x9840,
	0x6F12, 0x1060,
	0x6F12, 0x8868,
	0x6F12, 0x19E0,
	0x6F12, 0x2000,
	0x6F12, 0x0FE0,
	0x6F12, 0x4000,
	0x6F12, 0xF474,
	0x6F12, 0x2000,
	0x6F12, 0x4A40,
	0x6F12, 0x2000,
	0x6F12, 0x2850,
	0x6F12, 0x2000,
	0x6F12, 0x0E20,
	0x6F12, 0x2000,
	0x6F12, 0x2ED0,
	0x6F12, 0x2000,
	0x6F12, 0x08D0,
	0x6F12, 0x2000,
	0x6F12, 0x36E0,
	0x6F12, 0x4000,
	0x6F12, 0x9404,
	0x6F12, 0x2000,
	0x6F12, 0x38C0,
	0x6F12, 0x4000,
	0x6F12, 0xD214,
	0x6F12, 0x4000,
	0x6F12, 0xA410,
	0x6F12, 0x2000,
	0x6F12, 0x3254,
	0x6F12, 0xD063,
	0x6F12, 0x2946,
	0x6F12, 0x2046,
	0x6F12, 0xBDE8,
	0x6F12, 0x7040,
	0x6F12, 0x0122,
	0x6F12, 0x00F0,
	0x6F12, 0x8CB8,
	0x6F12, 0x10B5,
	0x6F12, 0x0022,
	0x6F12, 0xAFF6,
	0x6F12, 0x9701,
	0x6F12, 0x3348,
	0x6F12, 0x00F0,
	0x6F12, 0xF8F8,
	0x6F12, 0x334C,
	0x6F12, 0x0022,
	0x6F12, 0xAFF6,
	0x6F12, 0x3F01,
	0x6F12, 0x2060,
	0x6F12, 0x3148,
	0x6F12, 0x00F0,
	0x6F12, 0xF0F8,
	0x6F12, 0x6060,
	0x6F12, 0xAFF2,
	0x6F12, 0x4970,
	0x6F12, 0x2F49,
	0x6F12, 0x0022,
	0x6F12, 0xC861,
	0x6F12, 0xAFF2,
	0x6F12, 0x9F61,
	0x6F12, 0x2E48,
	0x6F12, 0x00F0,
	0x6F12, 0xE5F8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x2D51,
	0x6F12, 0x2061,
	0x6F12, 0x2B48,
	0x6F12, 0x00F0,
	0x6F12, 0xDEF8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x2341,
	0x6F12, 0x6061,
	0x6F12, 0x2948,
	0x6F12, 0x00F0,
	0x6F12, 0xD7F8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0xE931,
	0x6F12, 0xA061,
	0x6F12, 0x2648,
	0x6F12, 0x00F0,
	0x6F12, 0xD0F8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x6B71,
	0x6F12, 0xE061,
	0x6F12, 0x2448,
	0x6F12, 0x00F0,
	0x6F12, 0xC9F8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x2371,
	0x6F12, 0xA060,
	0x6F12, 0x2148,
	0x6F12, 0x00F0,
	0x6F12, 0xC2F8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0xB731,
	0x6F12, 0xE060,
	0x6F12, 0x1F48,
	0x6F12, 0x00F0,
	0x6F12, 0xBBF8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x6121,
	0x6F12, 0x2062,
	0x6F12, 0x1C48,
	0x6F12, 0x00F0,
	0x6F12, 0xB4F8,
	0x6F12, 0x6062,
	0x6F12, 0x0020,
	0x6F12, 0x04F1,
	0x6F12, 0x3401,
	0x6F12, 0x0246,
	0x6F12, 0x8881,
	0x6F12, 0xAFF2,
	0x6F12, 0x3121,
	0x6F12, 0x1848,
	0x6F12, 0x00F0,
	0x6F12, 0xA9F8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x7511,
	0x6F12, 0xA062,
	0x6F12, 0x1548,
	0x6F12, 0x00F0,
	0x6F12, 0xA2F8,
	0x6F12, 0x0022,
	0x6F12, 0xAFF2,
	0x6F12, 0x3711,
	0x6F12, 0xE062,
	0x6F12, 0x1348,
	0x6F12, 0x00F0,
	0x6F12, 0x9BF8,
	0x6F12, 0x1249,
	0x6F12, 0x2063,
	0x6F12, 0x40F6,
	0x6F12, 0xF100,
	0x6F12, 0x0968,
	0x6F12, 0x4883,
	0x6F12, 0x10BD,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0xDE1F,
	0x6F12, 0x2000,
	0x6F12, 0x4A40,
	0x6F12, 0x0000,
	0x6F12, 0x5F3B,
	0x6F12, 0x2000,
	0x6F12, 0x0850,
	0x6F12, 0x0000,
	0x6F12, 0xD719,
	0x6F12, 0x0000,
	0x6F12, 0x27FF,
	0x6F12, 0x0000,
	0x6F12, 0x39E3,
	0x6F12, 0x0001,
	0x6F12, 0x32CF,
	0x6F12, 0x0001,
	0x6F12, 0x1E3B,
	0x6F12, 0x0000,
	0x6F12, 0xEC45,
	0x6F12, 0x0000,
	0x6F12, 0x67B9,
	0x6F12, 0x0000,
	0x6F12, 0xE62B,
	0x6F12, 0x0001,
	0x6F12, 0x2265,
	0x6F12, 0x0000,
	0x6F12, 0x8C83,
	0x6F12, 0x0000,
	0x6F12, 0x5449,
	0x6F12, 0x2000,
	0x6F12, 0x08D0,
	0x6F12, 0x4AF2,
	0x6F12, 0x2B1C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x4DF6,
	0x6F12, 0x1F6C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x44F6,
	0x6F12, 0x655C,
	0x6F12, 0xC0F2,
	0x6F12, 0x010C,
	0x6F12, 0x6047,
	0x6F12, 0x45F6,
	0x6F12, 0x433C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x45F6,
	0x6F12, 0xE36C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x46F2,
	0x6F12, 0x7B1C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x44F6,
	0x6F12, 0xD90C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x44F6,
	0x6F12, 0x791C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x44F6,
	0x6F12, 0x811C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x44F2,
	0x6F12, 0xB50C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x44F6,
	0x6F12, 0xE90C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x43F6,
	0x6F12, 0x155C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x43F6,
	0x6F12, 0x1D5C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x4DF2,
	0x6F12, 0xC95C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x42F2,
	0x6F12, 0xFF7C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x48F2,
	0x6F12, 0x712C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x43F6,
	0x6F12, 0xE31C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x43F2,
	0x6F12, 0x374C,
	0x6F12, 0xC0F2,
	0x6F12, 0x010C,
	0x6F12, 0x6047,
	0x6F12, 0x46F2,
	0x6F12, 0xB97C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x4EF2,
	0x6F12, 0x2B6C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x42F2,
	0x6F12, 0x652C,
	0x6F12, 0xC0F2,
	0x6F12, 0x010C,
	0x6F12, 0x6047,
	0x6F12, 0x48F6,
	0x6F12, 0x834C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x45F2,
	0x6F12, 0x494C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6F12, 0x4CF2,
	0x6F12, 0x2D1C,
	0x6F12, 0xC0F2,
	0x6F12, 0x000C,
	0x6F12, 0x6047,
	0x6028, 0x2000,
	0x602A, 0x16F0,
	0x6F12, 0x2929,
	0x602A, 0x16F2,
	0x6F12, 0x2929,
	0x602A, 0x16FA,
	0x6F12, 0x0029,
	0x602A, 0x16FC,
	0x6F12, 0x0029,
	0x602A, 0x1708,
	0x6F12, 0x0029,
	0x602A, 0x170A,
	0x6F12, 0x0029,
	0x602A, 0x1712,
	0x6F12, 0x2929,
	0x602A, 0x1714,
	0x6F12, 0x2929,
	0x602A, 0x1716,
	0x6F12, 0x2929,
	0x602A, 0x1722,
	0x6F12, 0x152A,
	0x602A, 0x1724,
	0x6F12, 0x152A,
	0x602A, 0x172C,
	0x6F12, 0x002A,
	0x602A, 0x172E,
	0x6F12, 0x002A,
	0x602A, 0x1736,
	0x6F12, 0x1500,
	0x602A, 0x1738,
	0x6F12, 0x1500,
	0x602A, 0x1740,
	0x6F12, 0x152A,
	0x602A, 0x1742,
	0x6F12, 0x152A,
	0x602A, 0x16BE,
	0x6F12, 0x1515,
	0x6F12, 0x1515,
	0x602A, 0x16C8,
	0x6F12, 0x0029,
	0x6F12, 0x0029,
	0x602A, 0x16D6,
	0x6F12, 0x0015,
	0x6F12, 0x0015,
	0x602A, 0x16E0,
	0x6F12, 0x2929,
	0x6F12, 0x2929,
	0x6F12, 0x2929,
	0x602A, 0x19B8,
	0x6F12, 0x0100,
	0x602A, 0x2224,
	0x6F12, 0x0100,
	0x602A, 0x0DF8,
	0x6F12, 0x1001,
	0x602A, 0x1EDA,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x6F12, 0x000E,
	0x602A, 0x16A0,
	0x6F12, 0x3D09,
	0x602A, 0x10A8,
	0x6F12, 0x000E,
	0x602A, 0x1198,
	0x6F12, 0x002B,
	0x602A, 0x1002,
	0x6F12, 0x0001,
	0x602A, 0x0F70,
	0x6F12, 0x0101,
	0x6F12, 0x002F,
	0x6F12, 0x007F,
	0x6F12, 0x0030,
	0x6F12, 0x0080,
	0x6F12, 0x000B,
	0x6F12, 0x0009,
	0x6F12, 0xF46E,
	0x602A, 0x0FAA,
	0x6F12, 0x000D,
	0x6F12, 0x0003,
	0x6F12, 0xF464,
	0x602A, 0x1698,
	0x6F12, 0x0D05,
	0x602A, 0x20A0,
	0x6F12, 0x0001,
	0x6F12, 0x0203,
	0x602A, 0x4A74,
	0x6F12, 0x0101,
	0x6F12, 0x0000,
	0x6F12, 0x1F80,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x0FF4,
	0x6F12, 0x0100,
	0x6F12, 0x1800,
	0x6028, 0x4000,
	0x0FEA, 0x1440,
	0x0B06, 0x0101,
	0xF44A, 0x0007,
	0xF456, 0x000A,
	0xF46A, 0xBFA0,
	0x0D80, 0x1388,
	0xB134, 0x0000,
	0xB136, 0x0000,
	0xB138, 0x0000,
	0x0B09, 0x0100,
	//0x0101, 0X0300,
};

static kal_uint16 addr_data_pair_preview[] = {
	0x6028, 0x4000,
	0x6214, 0x7970,
	0x6218, 0x7150,
	0x6028, 0x2000,
	0x602A, 0x0ED6,
	0x6F12, 0x0000,
	0x602A, 0x1CF0,
	0x6F12, 0x0200,
	0x602A, 0x0E58,
	0x6F12, 0x0023,
	0x602A, 0x1694,
	0x6F12, 0x170F,
	0x602A, 0x16AA,
	0x6F12, 0x009D,
	0x6F12, 0x000F,
	0x602A, 0x1098,
	0x6F12, 0x0012,
	0x602A, 0x2690,
	0x6F12, 0x0100,
	0x6F12, 0x0000,
	0x602A, 0x16A8,
	0x6F12, 0x38C0,
	0x602A, 0x108C,
	0x6F12, 0x0002,
	0x602A, 0x10CC,
	0x6F12, 0x0001,
	0x602A, 0x10D0,
	0x6F12, 0x000F,
	0x602A, 0x0F50,
	0x6F12, 0x0200,
	0x602A, 0x1758,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0x0344, 0x0000,
	0x0346, 0x000C,
	0x0348, 0x122F,
	0x034A, 0x0DA3,
	0x034C, 0x0910,
	0x034E, 0x06CC,
	0x0350, 0x0004,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0404, 0x1000,
	0x0402, 0x1010,
	0x0400, 0x1010,
	0x0114, 0x0300,
	0x0110, 0x1002,
	0x0136, 0x1800,
	0x0300, 0x0007,
	0x0302, 0x0001,
	0x0304, 0x0006,
	0x0306, 0x00F5,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x0080,
	0x0312, 0x0001,
	0x6028, 0x2000,
	0x602A, 0x16A6,
	0x6F12, 0x006C,
	0x6028, 0x4000,
	0x0340, 0x07E8,
	0x0342, 0x23F0,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x021E, 0x0000,
	0x0D00, 0x0000,
	0x0D02, 0x0001,
	0x0112, 0x0A0A,
};

static kal_uint16 addr_data_pair_capture[] = {
	0x6028, 0x4000,
	0x6214, 0x7970,
	0x6218, 0x7150,
	0x6028, 0x2000,
	0x602A, 0x0ED6,
	0x6F12, 0x0000,
	0x602A, 0x1CF0,
	0x6F12, 0x0200,
	0x602A, 0x0E58,
	0x6F12, 0x0023,
	0x602A, 0x1694,
	0x6F12, 0x170F,
	0x602A, 0x16AA,
	0x6F12, 0x009D,
	0x6F12, 0x000F,
	0x602A, 0x1098,
	0x6F12, 0x0012,
	0x602A, 0x2690,
	0x6F12, 0x0100,
	0x6F12, 0x0000,
	0x602A, 0x16A8,
	0x6F12, 0x38C0,
	0x602A, 0x108C,
	0x6F12, 0x0002,
	0x602A, 0x10CC,
	0x6F12, 0x0001,
	0x602A, 0x10D0,
	0x6F12, 0x000F,
	0x602A, 0x0F50,
	0x6F12, 0x0200,
	0x602A, 0x1758,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0x0344, 0x0000,
	0x0346, 0x000C,
	0x0348, 0x122F,
	0x034A, 0x0DA3,
	0x034C, 0x0910,
	0x034E, 0x06CC,
	0x0350, 0x0004,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0404, 0x1000,
	0x0402, 0x1010,
	0x0400, 0x1010,
	0x0114, 0x0300,
	0x0110, 0x1002,
	0x0136, 0x1800,
	0x0300, 0x0007,
	0x0302, 0x0001,
	0x0304, 0x0006,
	0x0306, 0x00F5,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x0080,
	0x0312, 0x0001,
	0x6028, 0x2000,
	0x602A, 0x16A6,
	0x6F12, 0x006C,
	0x6028, 0x4000,
	0x0340, 0x07E8,
	0x0342, 0x23F0,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x021E, 0x0000,
	0x0D00, 0x0000,
	0x0D02, 0x0001,
	0x0112, 0x0A0A,
};

static kal_uint16 addr_data_pair_normal_video[] = {
	0x6028, 0x4000,
	0x6214, 0x7970,
	0x6218, 0x7150,
	0x6028, 0x2000,
	0x602A, 0x0ED6,
	0x6F12, 0x0000,
	0x602A, 0x1CF0,
	0x6F12, 0x0200,
	0x602A, 0x0E58,
	0x6F12, 0x0023,
	0x602A, 0x1694,
	0x6F12, 0x170F,
	0x602A, 0x16AA,
	0x6F12, 0x009D,
	0x6F12, 0x000F,
	0x602A, 0x1098,
	0x6F12, 0x0012,
	0x602A, 0x2690,
	0x6F12, 0x0100,
	0x6F12, 0x0000,
	0x602A, 0x16A8,
	0x6F12, 0x38C0,
	0x602A, 0x108C,
	0x6F12, 0x0002,
	0x602A, 0x10CC,
	0x6F12, 0x0001,
	0x602A, 0x10D0,
	0x6F12, 0x000F,
	0x602A, 0x0F50,
	0x6F12, 0x0200,
	0x602A, 0x1758,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0x0344, 0x0000,
	0x0346, 0x01BC,
	0x0348, 0x122F,
	0x034A, 0x0BF3,
	0x034C, 0x0910,
	0x034E, 0x051C,
	0x0350, 0x0004,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0404, 0x1000,
	0x0402, 0x1010,
	0x0400, 0x1010,
	0x0114, 0x0300,
	0x0110, 0x1002,
	0x0136, 0x1800,
	0x0300, 0x0007,
	0x0302, 0x0001,
	0x0304, 0x0006,
	0x0306, 0x00F5,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x0080,
	0x0312, 0x0001,
	0x6028, 0x2000,
	0x602A, 0x16A6,
	0x6F12, 0x006C,
	0x6028, 0x4000,
	0x0340, 0x0650,
	0x0342, 0x2D00,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x021E, 0x0000,
	0x0D00, 0x0000,
	0x0D02, 0X0001,
	0x0112, 0x0A0A,
};

static kal_uint16 addr_data_pair_hs_video[] = {
	0x6028, 0x4000,
	0x6214, 0x7970,
	0x6218, 0x7150,
	0x6028, 0x2000,
	0x602A, 0x0ED6,
	0x6F12, 0x0000,
	0x602A, 0x1CF0,
	0x6F12, 0x0200,
	0x602A, 0x0E58,
	0x6F12, 0x0023,
	0x602A, 0x1694,
	0x6F12, 0x170F,
	0x602A, 0x16AA,
	0x6F12, 0x009D,
	0x6F12, 0x000F,
	0x602A, 0x1098,
	0x6F12, 0x0012,
	0x602A, 0x2690,
	0x6F12, 0x0100,
	0x6F12, 0x0000,
	0x602A, 0x16A8,
	0x6F12, 0x38C0,
	0x602A, 0x108C,
	0x6F12, 0x0002,
	0x602A, 0x10CC,
	0x6F12, 0x0001,
	0x602A, 0x10D0,
	0x6F12, 0x000F,
	0x602A, 0x0F50,
	0x6F12, 0x0200,
	0x602A, 0x1758,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0x0344, 0x0000,
	0x0346, 0x01BC,
	0x0348, 0x122F,
	0x034A, 0x0BF3,
	0x034C, 0x0910,
	0x034E, 0x051C,
	0x0350, 0x0004,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0404, 0x1000,
	0x0402, 0x1010,
	0x0400, 0x1010,
	0x0114, 0x0300,
	0x0110, 0x1002,
	0x0136, 0x1800,
	0x0300, 0x0007,
	0x0302, 0x0001,
	0x0304, 0x0006,
	0x0306, 0x00F5,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x0080,
	0x0312, 0x0001,
	0x6028, 0x2000,
	0x602A, 0x16A6,
	0x6F12, 0x006C,
	0x6028, 0x4000,
	0x0340, 0x0650,
	0x0342, 0x2D00,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x021E, 0x0000,
	0x0D00, 0x0000,
	0x0D02, 0X0001,
	0x0112, 0x0A0A,
};

static kal_uint16 addr_data_pair_slim_video[] = {
	0x6028, 0x4000,
	0x6214, 0x7970,
	0x6218, 0x7150,
	0x6028, 0x2000,
	0x602A, 0x0ED6,
	0x6F12, 0x0000,
	0x602A, 0x1CF0,
	0x6F12, 0x0200,
	0x602A, 0x0E58,
	0x6F12, 0x0023,
	0x602A, 0x1694,
	0x6F12, 0x170F,
	0x602A, 0x16AA,
	0x6F12, 0x009D,
	0x6F12, 0x000F,
	0x602A, 0x1098,
	0x6F12, 0x0012,
	0x602A, 0x2690,
	0x6F12, 0x0100,
	0x6F12, 0x0000,
	0x602A, 0x16A8,
	0x6F12, 0x38C0,
	0x602A, 0x108C,
	0x6F12, 0x0002,
	0x602A, 0x10CC,
	0x6F12, 0x0001,
	0x602A, 0x10D0,
	0x6F12, 0x000F,
	0x602A, 0x0F50,
	0x6F12, 0x0200,
	0x602A, 0x1758,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0x0344, 0x0000,
	0x0346, 0x000C,
	0x0348, 0x122F,
	0x034A, 0x0DA3,
	0x034C, 0x0910,
	0x034E, 0x06CC,
	0x0350, 0x0004,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0404, 0x1000,
	0x0402, 0x1010,
	0x0400, 0x1010,
	0x0114, 0x0300,
	0x0110, 0x1002,
	0x0136, 0x1800,
	0x0300, 0x0007,
	0x0302, 0x0001,
	0x0304, 0x0006,
	0x0306, 0x00F5,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x0080,
	0x0312, 0x0001,
	0x6028, 0x2000,
	0x602A, 0x16A6,
	0x6F12, 0x006C,
	0x6028, 0x4000,
	0x0340, 0x07E8,
	0x0342, 0x23F0,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x021E, 0x0000,
	0x0D00, 0x0000,
	0x0D02, 0x0001,
	0x0112, 0x0A0A,
};

static void sensor_init(void)
{
	/*Global setting */
	pr_debug("E\n");
	write_cmos_sensor_16_16(0x6028, 0x4000);
	write_cmos_sensor_16_16(0x0000, 0x101F);
	write_cmos_sensor_16_16(0x0000, 0x3109);
	write_cmos_sensor_16_16(0x6010, 0x0001);
	mdelay(3);
	table_write_cmos_sensor(addr_data_pair_init,
		   sizeof(addr_data_pair_init) / sizeof(kal_uint16));
	pr_debug("X\n");

}	/*	sensor_init  */

static void capture_setting(void)
{
	table_write_cmos_sensor(addr_data_pair_capture,
		   sizeof(addr_data_pair_capture) / sizeof(kal_uint16));
}	/*	preview_setting  */

static void preview_setting(void)
{
	table_write_cmos_sensor(addr_data_pair_preview,
		   sizeof(addr_data_pair_preview) / sizeof(kal_uint16));
}	/*	preview_setting  */

static void normal_video_setting(void)
{
	table_write_cmos_sensor(addr_data_pair_normal_video,
		sizeof(addr_data_pair_normal_video) / sizeof(kal_uint16));
}

static void hs_video_setting(void)
{
	table_write_cmos_sensor(addr_data_pair_hs_video,
		sizeof(addr_data_pair_hs_video) / sizeof(kal_uint16));
}

static void slim_video_setting(void)
{
	table_write_cmos_sensor(addr_data_pair_slim_video,
		sizeof(addr_data_pair_slim_video) / sizeof(kal_uint16));
}

#define FOUR_CELL_SIZE 2048
#define FOUR_CELL_ADDR 0x150F
static u32 is_read_four_cell;
static char four_cell_data[FOUR_CELL_SIZE + 2];
static void read_four_cell_from_eeprom(char *data)
{
	int i;

	if (is_read_four_cell != 1) {
		LOG_INF("need to read from EEPROM\n");
		four_cell_data[0] = (FOUR_CELL_SIZE & 0xFF);/*Low*/
		four_cell_data[1] = ((FOUR_CELL_SIZE >> 8) & 0xFF);/*High*/
		/*Multi-Read*/
		for (i = 0; i < FOUR_CELL_SIZE; i++)
			four_cell_data[i+2] = read_cmos_eeprom_8(FOUR_CELL_ADDR + i);
		is_read_four_cell = 1;
	}
	if (data != NULL) {
		LOG_INF("return data\n");
		memcpy(data, four_cell_data, FOUR_CELL_SIZE + 2);
	}
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
 ************************************************************************
 */
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id =
				((read_cmos_sensor_16_8(0x0000) << 8)
				| read_cmos_sensor_16_8(0x0001));
			printk("s5k3p9read out sensor id 0x%x\n",
				*sensor_id);
			if (*sensor_id == imgsensor_info.sensor_id) {
				pr_info("[%s] i2c write id: 0x%x, sensor id: 0x%x\n",
					__func__, imgsensor.i2c_write_id, *sensor_id);
				*sensor_id = MOT_CANCUNF_S5K3P9SP04_SENSOR_ID;
				read_four_cell_from_eeprom(NULL);
				return ERROR_NONE;
			}
			pr_debug("Read sensor id fail, id: 0x%x\n",
				imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id !=  imgsensor_info.sensor_id) {
/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF*/
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
 ************************************************************************
 */
static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;

	pr_debug("PLATFORM:MT6750,MIPI 4LANE\n");
	pr_debug("preview 1280*960@30fps,864Mbps/lane;");
	pr_debug("video 1280*960@30fps,864Mbps/lane;");
	pr_debug("capture 5M@30fps,864Mbps/lane\n");

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = ((read_cmos_sensor_16_8(0x0000) << 8) |
				read_cmos_sensor_16_8(0x0001));
			if (sensor_id == imgsensor_info.sensor_id) {
				pr_debug("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
				break;
			}
			pr_debug("Read sensor id fail, id: 0x%x\n",
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
	imgsensor.test_pattern = 0;
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
	streaming_control(KAL_FALSE);

	pr_debug("E\n");

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
	pr_debug("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
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
	pr_debug("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps != imgsensor_info.cap.max_framerate) {
		pr_debug("Warning: current_fps %d fps is not support",
			imgsensor.current_fps);
		pr_debug("so use cap's setting: %d fps!\n",
			imgsensor_info.cap.max_framerate / 10);
	}
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	capture_setting();

	return ERROR_NONE;
} /* capture() */

static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	normal_video_setting();

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("HS_Video E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	hs_video_setting();

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("E\n");

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

	return ERROR_NONE;
}	/*	slim_video	 */

static kal_uint32 get_resolution(
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT(*sensor_resolution))
{
	pr_debug("E\n");
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

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_INFO_STRUCT *sensor_info,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity =
		SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity =
		SENSOR_CLOCK_POLARITY_LOW;
	/* not use */
	sensor_info->SensorHsyncPolarity =
		SENSOR_CLOCK_POLARITY_LOW;
	/* inverse with datasheet*/
	sensor_info->SensorVsyncPolarity =
		SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType =
		imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType =
		imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode =
		imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame =
		imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame =
		imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame =
		imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame =
		imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0;
	/* not use */
	sensor_info->SensorDrivingCurrent =
		imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame =
		imgsensor_info.ae_shut_delay_frame;
	/* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	/* The frame of setting sensor gain */
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
	sensor_info->SensorWidthSampling = 0;	/* 0 is default 1x*/
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x*/
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		sensor_info->SensorGrabStartX =
			imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		sensor_info->SensorGrabStartX =
			imgsensor_info.cap.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.cap.starty;

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
		sensor_info->SensorGrabStartX =
			imgsensor_info.hs_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.hs_video.starty;

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
		sensor_info->SensorGrabStartX =
			imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
		imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}	/*	get_info  */

static kal_uint32 control(
		enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("scenario_id = %d\n", scenario_id);
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
		pr_debug("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */

static kal_uint32 set_video_mode(UINT16 framerate)
{
	pr_debug("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate*/
	if (framerate == 0) {
		/* Dynamic frame rate*/
		return ERROR_NONE;
	}
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) &&
			(imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) &&
			(imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	/*LOG_INF("enable = %d, framerate = %d\n", enable, framerate);*/
	spin_lock(&imgsensor_drv_lock);
	if (enable) {/*enable auto flicker*/
		imgsensor.autoflicker_en = KAL_TRUE;
	} else {/*Cancel Auto flick*/
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	pr_debug("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk /
			framerate * 10 /
			imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
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
			(frame_length >
				imgsensor_info.normal_video.framelength) ?
			(frame_length -
				imgsensor_info.normal_video.framelength)
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
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate) {
			pr_debug("Warning: current_fps %d fps is not support",
				framerate);
			pr_debug("so use cap's setting: %d fps!\n",
				imgsensor_info.cap.max_framerate / 10);
		}
		frame_length = imgsensor_info.cap.pclk /
			framerate * 10 /
			imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap.framelength) ?
			(frame_length - imgsensor_info.cap.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.cap.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk /
			framerate * 10 /
			imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.hs_video.framelength) ?
			(frame_length - imgsensor_info.hs_video.framelength) :
			0;
		imgsensor.frame_length =
			imgsensor_info.hs_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk /
			framerate * 10 /
			imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.slim_video.framelength) ?
			(frame_length - imgsensor_info.slim_video.framelength) :
			0;
		imgsensor.frame_length =
			imgsensor_info.slim_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	default:/*coding with  preview scenario by default*/
		frame_length = imgsensor_info.pre.pclk /
			framerate * 10 /
			imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		pr_debug("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	pr_debug("scenario_id = %d\n", scenario_id);

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
	pr_debug("set_test_pattern_mode enable: %d", enable);

	if (enable) {
/* 0 : Normal, 1 : Solid Color, 2 : Color Bar, 3 : Shade Color Bar, 4 : PN9 */
		write_cmos_sensor_16_16(0x0600, 0x0001);
	} else {
		write_cmos_sensor_16_16(0x0600, 0x0000);
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

	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;


	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/*pr_debug("feature_id = %d\n", feature_id);*/
	switch (feature_id) {

	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		*(feature_data + 2) = imgsensor_info.max_gain;
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_shutter;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
					= imgsensor_info.cap.pclk;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.pclk;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.pclk;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.pclk;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_OFFSET_TO_START_OF_EXPOSURE:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= 2500000;
		break;
	case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.cap.framelength << 16)
				+ imgsensor_info.cap.linelength;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.normal_video.framelength << 16)
				+ imgsensor_info.normal_video.linelength;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.hs_video.linelength;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.slim_video.framelength << 16)
				+ imgsensor_info.slim_video.linelength;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			 *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
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
		write_cmos_sensor_16_16(sensor_reg_data->RegAddr,
			sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor_16_16(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM */
		/* or just return LENS_DRIVER_ID_DO_NOT_CARE */
		/* if EEPROM does not exist in camera module.*/
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
			(enum MSDK_SCENARIO_ID_ENUM)*feature_data,
			*(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL) (*feature_data));
		break;
	/*for factory mode auto testing*/
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		/*pr_debug("SENSOR_FEATURE_GET_CROP_INFO:");*/
		/*pr_debug("scenarioId:%d\n", *feature_data_32);*/
		wininfo =
			(struct SENSOR_WINSIZE_INFO_STRUCT *)
			(uintptr_t)(*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[3],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[4],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		/*pr_debug("SENSOR_FEATURE_GET_PDAF_INFO");*/
		/*pr_debug("scenarioId:%lld\n", *feature_data);*/
		PDAFinfo =
			(struct SET_PD_BLOCK_INFO_T *)
			(uintptr_t)(*(feature_data + 1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		/*pr_debug("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY");*/
		/*pr_debug("scenarioId:%lld\n", *feature_data);*/
		/*PDAF capacity enable or not, 2p8 only full size support PDAF*/
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= 0;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= 0; /* video & capture use same setting*/
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= 0;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= 0;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= 0;
			break;
		}
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) *feature_data,
			(UINT16) *(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_4CELL_DATA: {
		char *data = (char *)(uintptr_t)(*(feature_data + 1));
		UINT16 type = (UINT16)(*feature_data);

		/*get 4 cell data from eeprom*/
		if (type == FOUR_CELL_CAL_TYPE_XTALK_CAL) {
			LOG_INF("Read Cross Talk Start");
			read_four_cell_from_eeprom(data);
			LOG_INF("Read Cross Talk = %02x %02x %02x %02x %02x %02x\n",
				(UINT16)data[0], (UINT16)data[1],
				(UINT16)data[2], (UINT16)data[3],
				(UINT16)data[4], (UINT16)data[5]);
		}
		break;
	}
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		pr_debug("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		pr_debug("SENSOR_FEATURE_SET_STREAMING_RESUME");
		pr_debug("shutter:%llu\n", *feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*feature_return_para_32 = 1000;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*feature_return_para_32 = 1000;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		default:
			*feature_return_para_32 = 1000;
			break;
		}
		pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
		{
			kal_uint32 rate;

			switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				rate =
				imgsensor_info.cap.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				rate =
				imgsensor_info.normal_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				rate =
				imgsensor_info.hs_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
				rate =
				imgsensor_info.slim_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				rate =
					imgsensor_info.pre.mipi_pixel_rate;
				break;
			default:
					rate = 0;
					break;
			}
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
		}
		break;
	case SENSOR_FEATURE_GET_PIXEL_RATE:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.cap.pclk /
			(imgsensor_info.cap.linelength - 80))*
			imgsensor_info.cap.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.normal_video.pclk /
			(imgsensor_info.normal_video.linelength - 80))*
			imgsensor_info.normal_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.hs_video.pclk /
			(imgsensor_info.hs_video.linelength - 80))*
			imgsensor_info.hs_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.slim_video.pclk /
			(imgsensor_info.slim_video.linelength - 80))*
			imgsensor_info.slim_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.pre.pclk /
			(imgsensor_info.pre.linelength - 80))*
			imgsensor_info.pre.grabwindow_width;
			break;
		}
		break;
	default:
		break;
	}
	return ERROR_NONE;
}	/*	feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 MOT_CANCUNF_S5K3P9SP04_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;

	return ERROR_NONE;
}	/*	S5K3P9SP_MIPI_RAW_SensorInit	*/
