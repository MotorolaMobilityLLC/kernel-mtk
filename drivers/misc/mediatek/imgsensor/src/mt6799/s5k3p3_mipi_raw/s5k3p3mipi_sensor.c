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

#include "s5k3p3mipi_sensor.h"

/*===FEATURE SWITH===*/
 /* #define FANPENGTAO   //for debug log */
 #define LOG_INF LOG_INF_LOD
 /* #define NONCONTINUEMODE */
/*===FEATURE SWITH===*/

/****************************Modify Following Strings for Debug****************************/
#define PFX "s5k3p3_camera_primax"
#define LOG_INF_LOD(format, args...)    pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_1 LOG_INF("S5K3P3,MIPI 4LANE\n")
#define SENSORDB LOG_INF
/****************************   Modify end    *******************************************/

static DEFINE_SPINLOCK(imgsensor_drv_lock);

u8 *s5k3p3_primax_otp_buf;
#define	MAX_READ_WRITE_SIZE	8
#define	OTP_DATA_SIZE	0x891
#define	OTP_START_ADDR	0x0000
#define	E2PROM_WRITE_ID	0xA0

struct s5k3p3_write_buffer {
	u8 addr[2];
	u8 data[MAX_READ_WRITE_SIZE];
};

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K3P3SX_SENSOR_ID,
	.checksum_value =  0xe1b26f6c,		/* checksum value for Camera Auto Test */
	.pre = {
		.pclk = 280000000,				/* record different mode's pclk */
		.linelength  = 5148,				/* record different mode's linelength */
		.framelength = 1800,			/* record different mode's framelength */
		.startx = 0,
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 2320,		/* record different mode's width of grabwindow */
		.grabwindow_height = 1748,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 280000000,				/* record different mode's pclk */
		.linelength  = 5148,				/* record different mode's linelength */
		.framelength = 1800,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 2320,		/* record different mode's width of grabwindow */
		.grabwindow_height = 1748,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk = 400000000,				/* record different mode's pclk */
		.linelength  = 4704,				/* record different mode's linelength */
		.framelength = 3536,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 4208,		/* record different mode's width of grabwindow */
		.grabwindow_height = 3120,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 240,
	},
	.custom2 = {
		.pclk = 300000000,				/* record different mode's pclk */
		.linelength  = 4296,				/* record different mode's linelength */
		.framelength = 2302,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 4032,		/* record different mode's width of grabwindow */
		.grabwindow_height = 2256,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 233300000,				/* record different mode's pclk */
		.linelength  = 4296,				/* record different mode's linelength */
		.framelength = 1780,			/* record different mode's framelength */
		.startx = 0,
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 2016,		/* record different mode's width of grabwindow */
		.grabwindow_height = 1508,		/* record different mode's height of grabwindow */
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 600000000,				/* record different mode's pclk */
		.linelength  = 4296,				/* record different mode's linelength */
		.framelength = 1160,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 1296,		/* record different mode's width of grabwindow */
		.grabwindow_height = 736,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 600000000,				/* record different mode's pclk */
		.linelength  = 4296,				/* record different mode's linelength */
		.framelength = 4648,			/* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width  = 1296,		/* record different mode's width of grabwindow */
		.grabwindow_height = 736,		/* record different mode's height of grabwindow */
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},

	.margin = 10,			/* sensor framelength & shutter margin */
	.min_shutter = 2,		/* min shutter */
	.max_frame_length = 0xFFFE,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,/* isp gain delay frame for AE cycle */
	.ihdr_support = 0,	  /* 1, support; 0,not support */
	.ihdr_le_firstline = 0,  /* 1,le first ; 0, se first */
	.sensor_mode_num = 6,	  /* support sensor mode num ,don't support Slow motion */
	.cap_delay_frame = 2,		/* enter capture delay frame num */
	.pre_delay_frame = 2,		/* enter preview delay frame num */
	.custom2_delay_frame = 2,		/* enter capture delay frame num */
	.video_delay_frame = 2,		/* enter video delay frame num */
	.hs_video_delay_frame = 2,	/* enter high speed video  delay frame num */
	.slim_video_delay_frame = 2,/* enter slim video delay frame num */

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
	.mirror = IMAGE_NORMAL,				/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x200,					/* current shutter */
	.gain = 0x200,						/* current gain */
	.dummy_pixel = 0,					/* current dummypixel */
	.dummy_line = 0,					/* current dummyline */
	.current_fps = 0,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,/* current scenario id */
	.ihdr_en = KAL_FALSE, /* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x5A,/* record current sensor's i2c write id */
};

/* Sensor output window information*/
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[6] = {
{2320, 1748, 0, 0, 2320, 1748, 2320, 1748, 0, 0, 2320, 1748, 0, 0, 2320, 1748}, /* Preview */
{2320, 1748, 0, 0, 2320, 1748, 2320, 1748, 0, 0, 2320, 1748, 0, 0, 2320, 1748}, /* capture */
{2320, 1748,	0, 0,	2320, 1748, 2320, 1748,	0, 0,	2320, 1748, 0, 0,	2320, 1748}, /* Preview */
{2320, 1748,	0, 0,	2320, 1748, 2320, 1748,	0, 0,	2320, 1748, 0, 0,	2320, 1748}, /* Preview */
{2320, 1748,	0, 0,	2320, 1748, 2320, 1748,	0, 0,	2320, 1748, 0, 0,	2320, 1748}, /* Preview */
{2320, 1748,	0, 0,	2320, 1748, 2320, 1748,	0, 0,	2320, 1748, 0, 0,	2320, 1748}, /* Preview */
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}
#if 0
static void write_cmos_sensor_burst(kal_uint32 addr, u8 *reg_buf, kal_uint32 size)
{
	struct s5k3p3_write_buffer buf;
	int i;
	int ret;

	for (i = 0; i < size; i += MAX_READ_WRITE_SIZE) {
		buf.addr[0] = (u8)(addr >> 8);
		buf.addr[1] = (u8)(addr & 0xFF);
		if ((i + MAX_READ_WRITE_SIZE) > size) {
			memcpy(buf.data, (reg_buf + i), (size - i));
			ret = iBurstWriteReg((u8 *)&buf, (size - i + 2), imgsensor.i2c_write_id);
		} else {
			memcpy(buf.data, (reg_buf + i), MAX_READ_WRITE_SIZE);
			ret = iBurstWriteReg((u8 *)&buf, (MAX_READ_WRITE_SIZE + 2), imgsensor.i2c_write_id);
		}

		if (ret < 0)
			LOG_INF("write burst reg into sensor failed!\n");

		addr += MAX_READ_WRITE_SIZE;
	}
}
#endif
static int s5k3p3_read_otp(u16 addr, u8 *buf)
{
	int ret = 0;
	u8 pu_send_cmd[2] = {(u8)(addr >> 8), (u8)(addr & 0xFF)};

	ret = iReadRegI2C(pu_send_cmd, 2, (u8 *)buf, 1, E2PROM_WRITE_ID);
	if (ret < 0)
		LOG_INF("read data from s5k3p3 primax otp e2prom failed!\n");

	return ret;
}

static void s5k3p3_read_otp_burst(u16 addr, u8 *otp_buf)
{
	int i;
	int ret;
	u8 pu_send_cmd[2];

	for (i = 0; i < OTP_DATA_SIZE; i += MAX_READ_WRITE_SIZE) {
		pu_send_cmd[0] = (u8)(addr >> 8);
		pu_send_cmd[1] = (u8)(addr & 0xFF);

		if (i + MAX_READ_WRITE_SIZE > OTP_DATA_SIZE)
			ret = iReadRegI2C(pu_send_cmd, 2, (u8 *)(otp_buf + i), (OTP_DATA_SIZE - i), E2PROM_WRITE_ID);
		else
			ret = iReadRegI2C(pu_send_cmd, 2, (u8 *)(otp_buf + i), MAX_READ_WRITE_SIZE, E2PROM_WRITE_ID);

		if (ret < 0)
			LOG_INF("read lsc table from s5k3p3 primax otp e2prom failed!\n");

		addr += MAX_READ_WRITE_SIZE;
	}
}

static void write_cmos_sensor_w(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)};

	iWriteRegI2C(pusendcmd, 4, imgsensor.i2c_write_id);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor_w(0x0340, imgsensor.frame_length & 0xFFFF);
/* write_cmos_sensor_w(0x0342, imgsensor.line_length & 0xFFFF); */
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
#define MAX_SHUTTER	12103350		/* 120s long exposure time */
static void set_shutter(kal_uint32 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_uint32 line_length = 0;
	kal_uint16 long_exp_times = 0;
	kal_uint16 long_exp_shift = 0;

	/* limit max exposure time to be 120s */
	if (shutter > MAX_SHUTTER)
		shutter = MAX_SHUTTER;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	LOG_INF("enter shutter =%d\n", shutter);
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);

	/* Just should be called in capture case with long exposure */
	if (shutter == 6) {
		/*
		 * return to normal mode from long exposure mode.
		 */
		write_cmos_sensor(0x0100, 0x00);
		write_cmos_sensor(0x3004, 0x00);
		write_cmos_sensor_w(0x0342, imgsensor.line_length & 0xFFFF);
		write_cmos_sensor(0x0100, 0x01);
	}
	if (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) {
		long_exp_times = shutter / (imgsensor_info.max_frame_length - imgsensor_info.margin);
		if (shutter % (imgsensor_info.max_frame_length - imgsensor_info.margin))
			long_exp_times++;
		if (long_exp_times > 128)
			long_exp_times = 128;
		long_exp_shift = fls(long_exp_times) - 1;
		if (long_exp_times & (~(1 << long_exp_shift)))
			long_exp_shift++;

		long_exp_times = 1 << long_exp_shift;
		write_cmos_sensor(0x3004, long_exp_shift);
		shutter = shutter / long_exp_times;
		if (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) {
			line_length = shutter * 4296 / (imgsensor_info.max_frame_length - imgsensor_info.margin);
			line_length = (line_length + 1) / 2 * 2;
		}

		spin_lock(&imgsensor_drv_lock);
		if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
			imgsensor.frame_length = shutter + imgsensor_info.margin;
		else
			imgsensor.frame_length = imgsensor.min_frame_length;
		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
			imgsensor.frame_length = imgsensor_info.max_frame_length;
		spin_unlock(&imgsensor_drv_lock);

		/* line_length range is 4296 <-> 32766 */
		if (line_length > 32766)
			line_length = 32766;
		if (line_length < 4296)
			line_length = 4296;
		write_cmos_sensor_w(0x0342, line_length & 0xFFFF);
	}

	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
			? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 237 && realtime_fps <= 245) {
			set_max_framerate(236, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
			/* Extend frame length */
			write_cmos_sensor_w(0x0340, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor_w(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	/* Update Shutter */
	write_cmos_sensor_w(0x0202, shutter & 0xFFFF);
	LOG_INF("Exit! shutter=%d, framelength=%d, long_exp_line_length=%d, long_exp_shift:%d\n",
			shutter, imgsensor.frame_length, line_length, long_exp_shift);
}

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

	write_cmos_sensor_w(0x0204, (reg_gain&0xFFFF));

	return gain;
}	/*	set_gain  */

/* ihdr_write_shutter_gain not support for s5k2M8 */
static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n", le, se, gain);
	if (imgsensor.ihdr_en) {

		spin_lock(&imgsensor_drv_lock);
			if (le > imgsensor.min_frame_length - imgsensor_info.margin)
				imgsensor.frame_length = le + imgsensor_info.margin;
			else
				imgsensor.frame_length = imgsensor.min_frame_length;
			if (imgsensor.frame_length > imgsensor_info.max_frame_length)
				imgsensor.frame_length = imgsensor_info.max_frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (le < imgsensor_info.min_shutter)
				le = imgsensor_info.min_shutter;
			if (se < imgsensor_info.min_shutter)
				se = imgsensor_info.min_shutter;


		/* Extend frame length first */
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


#if 0
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
	imgsensor.mirror = image_mirror;
	spin_unlock(&imgsensor_drv_lock);
	switch (image_mirror) {
	case IMAGE_NORMAL:
	write_cmos_sensor(0x0101, 0X00);
	break;
	case IMAGE_H_MIRROR:
	write_cmos_sensor(0x0101, 0X01);
	break;
	case IMAGE_V_MIRROR:
	write_cmos_sensor(0x0101, 0X02);
	break;
	case IMAGE_HV_MIRROR:
	write_cmos_sensor(0x0101, 0X03);
	break;
	default:
	LOG_INF("Error image_mirror setting\n");
}

}
#endif
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
	LOG_INF("%s.\n", __func__);
	write_cmos_sensor_w(0x6028, 0x4000);
	write_cmos_sensor_w(0x6010, 0x0001);
	write_cmos_sensor_w(0x6028, 0x4000);
	write_cmos_sensor_w(0x6214, 0x7971);
	write_cmos_sensor_w(0x6218, 0x0100);
	write_cmos_sensor(0x0101, 0x03);
	/* Global */
	write_cmos_sensor_w(0x602A, 0xF408);
	write_cmos_sensor_w(0x6F12, 0x0048);
	write_cmos_sensor_w(0x602A, 0xF40C);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x602A, 0xF4AA);
	write_cmos_sensor_w(0x6F12, 0x0060);
	write_cmos_sensor_w(0x602A, 0xF442);
	write_cmos_sensor_w(0x6F12, 0x0800);
	write_cmos_sensor_w(0x602A, 0xF43E);
	write_cmos_sensor_w(0x6F12, 0x0400);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x602A, 0xF4A4);
	write_cmos_sensor_w(0x6F12, 0x0010);
	write_cmos_sensor_w(0x602A, 0xF4AC);
	write_cmos_sensor_w(0x6F12, 0x0056);
	write_cmos_sensor_w(0x602A, 0xF480);
	write_cmos_sensor_w(0x6F12, 0x0008);
	write_cmos_sensor_w(0x602A, 0xF492);
	write_cmos_sensor_w(0x6F12, 0x0016);
	write_cmos_sensor_w(0x602A, 0x3E58);
	write_cmos_sensor_w(0x6F12, 0x0056);
	write_cmos_sensor_w(0x602A, 0x39EE);
	write_cmos_sensor_w(0x6F12, 0x0206);
	write_cmos_sensor_w(0x602A, 0x39E8);
	write_cmos_sensor_w(0x6F12, 0x0205);
	write_cmos_sensor_w(0x602A, 0x3A36);
	write_cmos_sensor_w(0x6F12, 0xB3F0);
	write_cmos_sensor_w(0x602A, 0x32B2);
	write_cmos_sensor_w(0x6F12, 0x0132);
	write_cmos_sensor_w(0x602A, 0x3A38);
	write_cmos_sensor_w(0x6F12, 0x006C);
	write_cmos_sensor_w(0x602A, 0x3552);
	write_cmos_sensor_w(0x6F12, 0x00D0);
	write_cmos_sensor_w(0x602A, 0x3194);
	write_cmos_sensor_w(0x6F12, 0x1001);
	write_cmos_sensor_w(0x6028, 0x2000);
	write_cmos_sensor_w(0x602A, 0x13EC);
	write_cmos_sensor_w(0x6F12, 0x8011);
	write_cmos_sensor_w(0x6F12, 0x8011);
	write_cmos_sensor_w(0x6028, 0x4000);
	write_cmos_sensor_w(0x602A, 0x39BA);
	write_cmos_sensor_w(0x6F12, 0x0001);
	write_cmos_sensor_w(0x602A, 0x3004);
	write_cmos_sensor_w(0x6F12, 0x0008);
	write_cmos_sensor_w(0x602A, 0x39AA);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6028, 0x2000);
	write_cmos_sensor_w(0x602A, 0x026C);
	write_cmos_sensor_w(0x6F12, 0x41F0);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6028, 0x4000);
	write_cmos_sensor_w(0x602A, 0x37D4);
	write_cmos_sensor_w(0x6F12, 0x002D);
	write_cmos_sensor_w(0x602A, 0x37DA);
	write_cmos_sensor_w(0x6F12, 0x005D);
	write_cmos_sensor_w(0x602A, 0x37E0);
	write_cmos_sensor_w(0x6F12, 0x008D);
	write_cmos_sensor_w(0x602A, 0x37E6);
	write_cmos_sensor_w(0x6F12, 0x00BD);
	write_cmos_sensor_w(0x602A, 0x37EC);
	write_cmos_sensor_w(0x6F12, 0x00ED);
	write_cmos_sensor_w(0x602A, 0x37F2);
	write_cmos_sensor_w(0x6F12, 0x011D);
	write_cmos_sensor_w(0x602A, 0x37F8);
	write_cmos_sensor_w(0x6F12, 0x014D);
	write_cmos_sensor_w(0x602A, 0x37FE);
	write_cmos_sensor_w(0x6F12, 0x017D);
	write_cmos_sensor_w(0x602A, 0x3804);
	write_cmos_sensor_w(0x6F12, 0x01AD);
	write_cmos_sensor_w(0x602A, 0x380A);
	write_cmos_sensor_w(0x6F12, 0x01DD);
	write_cmos_sensor_w(0x602A, 0x3810);
	write_cmos_sensor_w(0x6F12, 0x020D);
	write_cmos_sensor_w(0x602A, 0x32A6);
	write_cmos_sensor_w(0x6F12, 0x0006);
	write_cmos_sensor_w(0x602A, 0x32BE);
	write_cmos_sensor_w(0x6F12, 0x0006);
	write_cmos_sensor_w(0x602A, 0x3210);
	write_cmos_sensor_w(0x6F12, 0x0006);
	/* TnP */
	write_cmos_sensor_w(0x6028, 0x2000);
	write_cmos_sensor_w(0x602A, 0x2EF8);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x0448);
	write_cmos_sensor_w(0x6F12, 0x0349);
	write_cmos_sensor_w(0x6F12, 0x0160);
	write_cmos_sensor_w(0x6F12, 0xC26A);
	write_cmos_sensor_w(0x6F12, 0x511A);
	write_cmos_sensor_w(0x6F12, 0x8180);
	write_cmos_sensor_w(0x6F12, 0x00F0);
	write_cmos_sensor_w(0x6F12, 0x21B8);
	write_cmos_sensor_w(0x6F12, 0x2000);
	write_cmos_sensor_w(0x6F12, 0x2F78);
	write_cmos_sensor_w(0x6F12, 0x2000);
	write_cmos_sensor_w(0x6F12, 0x18E0);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x10B5);
	write_cmos_sensor_w(0x6F12, 0x0E4C);
	write_cmos_sensor_w(0x6F12, 0xB4F8);
	write_cmos_sensor_w(0x6F12, 0x5820);
	write_cmos_sensor_w(0x6F12, 0x2388);
	write_cmos_sensor_w(0x6F12, 0xA2EB);
	write_cmos_sensor_w(0x6F12, 0x5302);
	write_cmos_sensor_w(0x6F12, 0xA4F8);
	write_cmos_sensor_w(0x6F12, 0x5820);
	write_cmos_sensor_w(0x6F12, 0x6288);
	write_cmos_sensor_w(0x6F12, 0x5208);
	write_cmos_sensor_w(0x6F12, 0x6280);
	write_cmos_sensor_w(0x6F12, 0x00F0);
	write_cmos_sensor_w(0x6F12, 0x14F8);
	write_cmos_sensor_w(0x6F12, 0xB4F8);
	write_cmos_sensor_w(0x6F12, 0x5800);
	write_cmos_sensor_w(0x6F12, 0x2188);
	write_cmos_sensor_w(0x6F12, 0x00EB);
	write_cmos_sensor_w(0x6F12, 0x5100);
	write_cmos_sensor_w(0x6F12, 0xA4F8);
	write_cmos_sensor_w(0x6F12, 0x5800);
	write_cmos_sensor_w(0x6F12, 0x6088);
	write_cmos_sensor_w(0x6F12, 0x4000);
	write_cmos_sensor_w(0x6F12, 0x6080);
	write_cmos_sensor_w(0x6F12, 0x10BD);
	write_cmos_sensor_w(0x6F12, 0xAFF2);
	write_cmos_sensor_w(0x6F12, 0x3300);
	write_cmos_sensor_w(0x6F12, 0x0249);
	write_cmos_sensor_w(0x6F12, 0x0863);
	write_cmos_sensor_w(0x6F12, 0x7047);
	write_cmos_sensor_w(0x6F12, 0x2000);
	write_cmos_sensor_w(0x6F12, 0x1998);
	write_cmos_sensor_w(0x6F12, 0x2000);
	write_cmos_sensor_w(0x6F12, 0x0460);
	write_cmos_sensor_w(0x6F12, 0x43F2);
	write_cmos_sensor_w(0x6F12, 0x290C);
	write_cmos_sensor_w(0x6F12, 0xC0F2);
	write_cmos_sensor_w(0x6F12, 0x000C);
	write_cmos_sensor_w(0x6F12, 0x6047);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x3103);
	write_cmos_sensor_w(0x6F12, 0x0022);
	write_cmos_sensor_w(0x6F12, 0x0000);
	write_cmos_sensor_w(0x6F12, 0x0001);
}


static void preview_setting(void)
{
	LOG_INF("%s.\n", __func__);
	/*
	 * H: 2320
	 * V: 1748
	 */
	/* mode */
	write_cmos_sensor_w(0x6028, 0x4000);
	write_cmos_sensor(0x0100, 0x00);
	write_cmos_sensor_w(0x0344, 0x0000);
	write_cmos_sensor_w(0x0346, 0x0000);
	write_cmos_sensor_w(0x0348, 0x090F);
	write_cmos_sensor_w(0x034A, 0x06D3);
	write_cmos_sensor_w(0x034C, 0x0910);
	write_cmos_sensor_w(0x034E, 0x06D4);
	write_cmos_sensor_w(0x3002, 0x0001);
	write_cmos_sensor_w(0x0136, 0x1800);	/* ext MCLK = 24Mhz */
	write_cmos_sensor_w(0x0304, 0x0006);	/* PLL_P */
	write_cmos_sensor_w(0x0306, 0x008C);	/* PLL_M */
	write_cmos_sensor_w(0x0302, 0x0001);
	write_cmos_sensor_w(0x0300, 0x0008);
	write_cmos_sensor_w(0x030C, 0x0004);
	write_cmos_sensor_w(0x030E, 0x006E);
	write_cmos_sensor_w(0x030A, 0x0001);
	write_cmos_sensor_w(0x0308, 0x0008);
	write_cmos_sensor_w(0x3008, 0x0001);
	write_cmos_sensor_w(0x0800, 0x0000);
	write_cmos_sensor_w(0x0200, 0x0200);
	write_cmos_sensor_w(0x0202, 0x0100);
	write_cmos_sensor_w(0x021C, 0x0200);
	write_cmos_sensor_w(0x021E, 0x0100);
	write_cmos_sensor_w(0x0342, 0x141C);	/* line length in pixel. */
	write_cmos_sensor_w(0x0340, 0x0708);	/* frame length in lines */
	write_cmos_sensor_w(0x3072, 0x03C0);
	write_cmos_sensor_w(0x6214, 0x7970);
	write_cmos_sensor(0x0100, 0x01);
}	/* preview_setting */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("%s.\n", __func__);
	/*
	 * H: 2320
	 * V: 1748
	 */
	/* mode */
	write_cmos_sensor_w(0x6028, 0x4000);
	write_cmos_sensor(0x0100, 0x00);
	write_cmos_sensor_w(0x0344, 0x0000);
	write_cmos_sensor_w(0x0346, 0x0000);
	write_cmos_sensor_w(0x0348, 0x090F);
	write_cmos_sensor_w(0x034A, 0x06D3);
	write_cmos_sensor_w(0x034C, 0x0910);
	write_cmos_sensor_w(0x034E, 0x06D4);
	write_cmos_sensor_w(0x3002, 0x0001);
	write_cmos_sensor_w(0x0136, 0x1800);
	write_cmos_sensor_w(0x0304, 0x0006);
	write_cmos_sensor_w(0x0306, 0x008C);
	write_cmos_sensor_w(0x0302, 0x0001);
	write_cmos_sensor_w(0x0300, 0x0008);
	write_cmos_sensor_w(0x030C, 0x0004);
	write_cmos_sensor_w(0x030E, 0x006E);
	write_cmos_sensor_w(0x030A, 0x0001);
	write_cmos_sensor_w(0x0308, 0x0008);
	write_cmos_sensor_w(0x3008, 0x0001);
	write_cmos_sensor_w(0x0800, 0x0000);
	write_cmos_sensor_w(0x0200, 0x0200);
	write_cmos_sensor_w(0x0202, 0x0100);
	write_cmos_sensor_w(0x021C, 0x0200);
	write_cmos_sensor_w(0x021E, 0x0100);
	write_cmos_sensor_w(0x0342, 0x141C);
	write_cmos_sensor_w(0x0340, 0x0708);
	write_cmos_sensor_w(0x3072, 0x03C0);
	write_cmos_sensor_w(0x6214, 0x7970);
	write_cmos_sensor(0x0100, 0x01);
}	/* capture setting */

static void hd_4k_setting(void)
{
	LOG_INF("%s.\n", __func__);
	write_cmos_sensor(0x0100, 0x00);
	/*
	 * Full-reso (16:9)@30fps
	 * H: 4032
	 * V: 2256
	 */
	/* Mode Setting */
	write_cmos_sensor(0x0112, 0x0A);
	write_cmos_sensor(0x0113, 0x0A);
	/* Clock Setting */
	write_cmos_sensor(0x0301, 0x06);
	write_cmos_sensor(0x0303, 0x02);
	write_cmos_sensor(0x0305, 0x0C);
	write_cmos_sensor(0x0306, 0x01);
	write_cmos_sensor(0x0307, 0xC2);
	write_cmos_sensor(0x0309, 0x0A);
	write_cmos_sensor(0x030B, 0x01);
	write_cmos_sensor(0x030D, 0x0C);
	write_cmos_sensor(0x030E, 0x01);
	write_cmos_sensor(0x030F, 0xC2);
	write_cmos_sensor(0x0310, 0x00);
	/* Output Size Setting */
	write_cmos_sensor(0x0342, 0x10);
	write_cmos_sensor(0x0343, 0xC8);
	write_cmos_sensor(0x0340, 0x08);
	write_cmos_sensor(0x0341, 0xFE);
	write_cmos_sensor(0x0344, 0x00);
	write_cmos_sensor(0x0345, 0x00);
	write_cmos_sensor(0x0346, 0x01);
	write_cmos_sensor(0x0347, 0x7C);
	write_cmos_sensor(0x0348, 0x0F);
	write_cmos_sensor(0x0349, 0xBF);
	write_cmos_sensor(0x034A, 0x0A);
	write_cmos_sensor(0x034B, 0x4B);
	write_cmos_sensor(0x0385, 0x01);
	write_cmos_sensor(0x0387, 0x01);
	write_cmos_sensor(0x0900, 0x00);
	write_cmos_sensor(0x0901, 0x11);
	write_cmos_sensor(0x300D, 0x00);
	write_cmos_sensor(0x302E, 0x00);
	write_cmos_sensor(0x0401, 0x00);
	write_cmos_sensor(0x0404, 0x00);
	write_cmos_sensor(0x0405, 0x10);
	write_cmos_sensor(0x040C, 0x0F);
	write_cmos_sensor(0x040D, 0xC0);
	write_cmos_sensor(0x040E, 0x08);
	write_cmos_sensor(0x040F, 0xD0);
	write_cmos_sensor(0x034C, 0x0F);
	write_cmos_sensor(0x034D, 0xC0);
	write_cmos_sensor(0x034E, 0x08);
	write_cmos_sensor(0x034F, 0xD0);
	/* Other Setting */
	write_cmos_sensor(0x0114, 0x03);
	write_cmos_sensor(0x0408, 0x00);
	write_cmos_sensor(0x0409, 0x00);
	write_cmos_sensor(0x040A, 0x00);
	write_cmos_sensor(0x040B, 0x00);
	write_cmos_sensor(0x0902, 0x00);
	write_cmos_sensor(0x3030, 0x00);
	write_cmos_sensor(0x3031, 0x01);
	write_cmos_sensor(0x3032, 0x00);
	write_cmos_sensor(0x3047, 0x01);
	write_cmos_sensor(0x3049, 0x01);
	write_cmos_sensor(0x30E6, 0x02);
	write_cmos_sensor(0x30E7, 0x59);
	write_cmos_sensor(0x4E25, 0x80);
	write_cmos_sensor(0x663A, 0x02);
	write_cmos_sensor(0x9311, 0x00);
	write_cmos_sensor(0xA0CD, 0x19);
	write_cmos_sensor(0xA0CE, 0x19);
	write_cmos_sensor(0xA0CF, 0x19);
	/* Integration Time Setting */
	write_cmos_sensor(0x0202, 0x08);
	write_cmos_sensor(0x0203, 0xF4);
	/* Gain Setting */
	write_cmos_sensor(0x0204, 0x00);
	write_cmos_sensor(0x0205, 0x00);
	write_cmos_sensor(0x020E, 0x01);
	write_cmos_sensor(0x020F, 0x00);
	write_cmos_sensor(0x0210, 0x01);
	write_cmos_sensor(0x0211, 0x00);
	write_cmos_sensor(0x0212, 0x01);
	write_cmos_sensor(0x0213, 0x00);
	write_cmos_sensor(0x0214, 0x01);
	write_cmos_sensor(0x0215, 0x00);

	write_cmos_sensor(0x0100, 0x01);
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("%s.\n", __func__);
	write_cmos_sensor(0x0100, 0x00);
	/*
	 * 1/2Binning@30fps
	 * H: 2016
	 * V: 1508
	 */
	/* Mode Setting */
	write_cmos_sensor(0x0112, 0x0A);
	write_cmos_sensor(0x0113, 0x0A);
	/* Clock Setting */
	write_cmos_sensor(0x0301, 0x06);
	write_cmos_sensor(0x0303, 0x02);
	write_cmos_sensor(0x0305, 0x0C);
	write_cmos_sensor(0x0306, 0x01);
	write_cmos_sensor(0x0307, 0x5E);
	write_cmos_sensor(0x0309, 0x0A);
	write_cmos_sensor(0x030B, 0x01);
	write_cmos_sensor(0x030D, 0x0C);
	write_cmos_sensor(0x030E, 0x01);
	write_cmos_sensor(0x030F, 0x5E);
	write_cmos_sensor(0x0310, 0x00);
	write_cmos_sensor(0x315D, 0x01);
	/* Output Size Setting */
	write_cmos_sensor(0x0342, 0x10);
	write_cmos_sensor(0x0343, 0xC8);
	/* Output Size Setting */
	write_cmos_sensor(0x0340, 0x06);
	write_cmos_sensor(0x0341, 0xF4);
	/* Output Size Setting */
	write_cmos_sensor(0x0344, 0x00);
	write_cmos_sensor(0x0345, 0x00);
	write_cmos_sensor(0x0346, 0x00);
	write_cmos_sensor(0x0347, 0x00);
	write_cmos_sensor(0x0348, 0x0F);
	write_cmos_sensor(0x0349, 0xBF);
	write_cmos_sensor(0x034A, 0x0B);
	write_cmos_sensor(0x034B, 0xC7);
	/* Output Size Setting */
	write_cmos_sensor(0x0385, 0x01);
	write_cmos_sensor(0x0387, 0x01);
	write_cmos_sensor(0x0900, 0x01);
	write_cmos_sensor(0x0901, 0x12);
	write_cmos_sensor(0x300D, 0x00);
	write_cmos_sensor(0x302E, 0x00);
	write_cmos_sensor(0x0401, 0x01);
	write_cmos_sensor(0x0404, 0x00);
	write_cmos_sensor(0x0405, 0x20);
	write_cmos_sensor(0x040C, 0x0F);
	write_cmos_sensor(0x040D, 0xC0);
	write_cmos_sensor(0x040E, 0x05);
	write_cmos_sensor(0x040F, 0xE4);
	write_cmos_sensor(0x034C, 0x07);
	write_cmos_sensor(0x034D, 0xE0);
	write_cmos_sensor(0x034E, 0x05);
	write_cmos_sensor(0x034F, 0xE4);
	/* Other Setting */
	write_cmos_sensor(0x0114, 0x03);
	write_cmos_sensor(0x0408, 0x00);
	write_cmos_sensor(0x0409, 0x00);
	write_cmos_sensor(0x040A, 0x00);
	write_cmos_sensor(0x040B, 0x00);
	write_cmos_sensor(0x0902, 0x00);
	write_cmos_sensor(0x3030, 0x00);
	write_cmos_sensor(0x3031, 0x01);
	write_cmos_sensor(0x3032, 0x00);
	write_cmos_sensor(0x3047, 0x01);
	write_cmos_sensor(0x3049, 0x01);
	write_cmos_sensor(0x30E6, 0x00);
	write_cmos_sensor(0x30E7, 0x00);
	write_cmos_sensor(0x4E25, 0x80);
	write_cmos_sensor(0x663A, 0x01);
	write_cmos_sensor(0x9311, 0x40);
	write_cmos_sensor(0xA0CD, 0x19);
	write_cmos_sensor(0xA0CE, 0x19);
	write_cmos_sensor(0xA0CF, 0x19);
	/* Integration Time Setting */
	write_cmos_sensor(0x0202, 0x06);
	write_cmos_sensor(0x0203, 0xEA);
	/* Gain Setting */
	write_cmos_sensor(0x0204, 0x00);
	write_cmos_sensor(0x0205, 0x00);
	write_cmos_sensor(0x020E, 0x01);
	write_cmos_sensor(0x020F, 0x00);
	write_cmos_sensor(0x0210, 0x01);
	write_cmos_sensor(0x0211, 0x00);
	write_cmos_sensor(0x0212, 0x01);
	write_cmos_sensor(0x0213, 0x00);
	write_cmos_sensor(0x0214, 0x01);
	write_cmos_sensor(0x0215, 0x00);

	write_cmos_sensor(0x0100, 0x01);
}

static void hs_video_setting(void)
{
	LOG_INF("%s.\n", __func__);
	write_cmos_sensor(0x0100, 0x00);
	/*
	 * 1296X736@120fps
	 * H: 1296
	 * V: 736
	 */
	/* Mode Setting */
	write_cmos_sensor(0x0112, 0x0A);
	write_cmos_sensor(0x0113, 0x0A);
	/* Clock Setting */
	write_cmos_sensor(0x0301, 0x04);
	write_cmos_sensor(0x0303, 0x02);
	write_cmos_sensor(0x0305, 0x0C);
	write_cmos_sensor(0x0306, 0x02);
	write_cmos_sensor(0x0307, 0x58);
	write_cmos_sensor(0x0309, 0x0A);
	write_cmos_sensor(0x030B, 0x01);
	write_cmos_sensor(0x030D, 0x0C);
	write_cmos_sensor(0x030E, 0x02);
	write_cmos_sensor(0x030F, 0x58);
	write_cmos_sensor(0x0310, 0x00);
	/* Output Size Setting */
	write_cmos_sensor(0x0342, 0x10);
	write_cmos_sensor(0x0343, 0xC8);
	/* Output Size Setting */
	write_cmos_sensor(0x0340, 0x04);
	write_cmos_sensor(0x0341, 0x88);
	/* Output Size Setting */
	write_cmos_sensor(0x0344, 0x02);
	write_cmos_sensor(0x0345, 0xD0);
	write_cmos_sensor(0x0346, 0x03);
	write_cmos_sensor(0x0347, 0x04);
	write_cmos_sensor(0x0348, 0x0C);
	write_cmos_sensor(0x0349, 0xEF);
	write_cmos_sensor(0x034A, 0x08);
	write_cmos_sensor(0x034B, 0xC3);
	write_cmos_sensor(0x0385, 0x01);
	write_cmos_sensor(0x0387, 0x01);
	write_cmos_sensor(0x0900, 0x01);
	write_cmos_sensor(0x0901, 0x12);
	write_cmos_sensor(0x300D, 0x00);
	write_cmos_sensor(0x302E, 0x01);
	write_cmos_sensor(0x0401, 0x01);
	write_cmos_sensor(0x0404, 0x00);
	write_cmos_sensor(0x0405, 0x20);
	write_cmos_sensor(0x040C, 0x0A);
	write_cmos_sensor(0x040D, 0x20);
	write_cmos_sensor(0x040E, 0x02);
	write_cmos_sensor(0x040F, 0xE0);
	write_cmos_sensor(0x034C, 0x05);
	write_cmos_sensor(0x034D, 0x10);
	write_cmos_sensor(0x034E, 0x02);
	write_cmos_sensor(0x034F, 0xE0);
	/* Other Setting */
	write_cmos_sensor(0x0114, 0x03);
	write_cmos_sensor(0x0408, 0x00);
	write_cmos_sensor(0x0409, 0x00);
	write_cmos_sensor(0x040A, 0x00);
	write_cmos_sensor(0x040B, 0x00);
	write_cmos_sensor(0x0902, 0x00);
	write_cmos_sensor(0x3030, 0x00);
	write_cmos_sensor(0x3031, 0x01);
	write_cmos_sensor(0x3032, 0x00);
	write_cmos_sensor(0x3047, 0x01);
	write_cmos_sensor(0x3049, 0x01);
	write_cmos_sensor(0x30E6, 0x00);
	write_cmos_sensor(0x30E7, 0x00);
	write_cmos_sensor(0x4E25, 0x80);
	write_cmos_sensor(0x663A, 0x01);
	write_cmos_sensor(0x9311, 0x64);
	write_cmos_sensor(0xA0CD, 0x19);
	write_cmos_sensor(0xA0CE, 0x19);
	write_cmos_sensor(0xA0CF, 0x19);
	/* Integration Time Setting */
	write_cmos_sensor(0x0202, 0x04);
	write_cmos_sensor(0x0203, 0x7E);
	/* Gain Setting */
	write_cmos_sensor(0x0204, 0x00);
	write_cmos_sensor(0x0205, 0x00);
	write_cmos_sensor(0x020E, 0x01);
	write_cmos_sensor(0x020F, 0x00);
	write_cmos_sensor(0x0210, 0x01);
	write_cmos_sensor(0x0211, 0x00);
	write_cmos_sensor(0x0212, 0x01);
	write_cmos_sensor(0x0213, 0x00);
	write_cmos_sensor(0x0214, 0x01);
	write_cmos_sensor(0x0215, 0x00);

	write_cmos_sensor(0x0100, 0x01);
}

static void slim_video_setting(void)
{
	LOG_INF("%s.\n", __func__);
	write_cmos_sensor(0x0100, 0x00);
	/*
	 * 1296X736@30fps
	 * H: 1296
	 * V: 736
	 */
	/* Mode Setting */
	write_cmos_sensor(0x0112, 0x0A);
	write_cmos_sensor(0x0113, 0x0A);
	/* Clock Setting */
	write_cmos_sensor(0x0301, 0x04);
	write_cmos_sensor(0x0303, 0x02);
	write_cmos_sensor(0x0305, 0x0C);
	write_cmos_sensor(0x0306, 0x02);
	write_cmos_sensor(0x0307, 0x58);
	write_cmos_sensor(0x0309, 0x0A);
	write_cmos_sensor(0x030B, 0x01);
	write_cmos_sensor(0x030D, 0x0C);
	write_cmos_sensor(0x030E, 0x02);
	write_cmos_sensor(0x030F, 0x58);
	write_cmos_sensor(0x0310, 0x00);
	/* Output Size Setting */
	write_cmos_sensor(0x0342, 0x10);
	write_cmos_sensor(0x0343, 0xC8);
	/* Output Size Setting */
	write_cmos_sensor(0x0340, 0x12);
	write_cmos_sensor(0x0341, 0x28);
	/* Output Size Setting */
	write_cmos_sensor(0x0344, 0x02);
	write_cmos_sensor(0x0345, 0xD0);
	write_cmos_sensor(0x0346, 0x03);
	write_cmos_sensor(0x0347, 0x04);
	write_cmos_sensor(0x0348, 0x0C);
	write_cmos_sensor(0x0349, 0xEF);
	write_cmos_sensor(0x034A, 0x08);
	write_cmos_sensor(0x034B, 0xC3);
	write_cmos_sensor(0x0385, 0x01);
	write_cmos_sensor(0x0387, 0x01);
	write_cmos_sensor(0x0900, 0x01);
	write_cmos_sensor(0x0901, 0x12);
	write_cmos_sensor(0x300D, 0x00);
	write_cmos_sensor(0x302E, 0x01);
	write_cmos_sensor(0x0401, 0x01);
	write_cmos_sensor(0x0404, 0x00);
	write_cmos_sensor(0x0405, 0x20);
	write_cmos_sensor(0x040C, 0x0A);
	write_cmos_sensor(0x040D, 0x20);
	write_cmos_sensor(0x040E, 0x02);
	write_cmos_sensor(0x040F, 0xE0);
	write_cmos_sensor(0x034C, 0x05);
	write_cmos_sensor(0x034D, 0x10);
	write_cmos_sensor(0x034E, 0x02);
	write_cmos_sensor(0x034F, 0xE0);
	/* Other Setting */
	write_cmos_sensor(0x0114, 0x03);
	write_cmos_sensor(0x0408, 0x00);
	write_cmos_sensor(0x0409, 0x00);
	write_cmos_sensor(0x040A, 0x00);
	write_cmos_sensor(0x040B, 0x00);
	write_cmos_sensor(0x0902, 0x00);
	write_cmos_sensor(0x3030, 0x00);
	write_cmos_sensor(0x3031, 0x01);
	write_cmos_sensor(0x3032, 0x00);
	write_cmos_sensor(0x3047, 0x01);
	write_cmos_sensor(0x3049, 0x01);
	write_cmos_sensor(0x30E6, 0x00);
	write_cmos_sensor(0x30E7, 0x00);
	write_cmos_sensor(0x4E25, 0x80);
	write_cmos_sensor(0x663A, 0x01);
	write_cmos_sensor(0x9311, 0x64);
	write_cmos_sensor(0xA0CD, 0x19);
	write_cmos_sensor(0xA0CE, 0x19);
	write_cmos_sensor(0xA0CF, 0x19);
	/* Integration Time Setting */
	write_cmos_sensor(0x0202, 0x12);
	write_cmos_sensor(0x0203, 0x1E);
	/* Gain Setting */
	write_cmos_sensor(0x0204, 0x00);
	write_cmos_sensor(0x0205, 0x00);
	write_cmos_sensor(0x020E, 0x01);
	write_cmos_sensor(0x020F, 0x00);
	write_cmos_sensor(0x0210, 0x01);
	write_cmos_sensor(0x0211, 0x00);
	write_cmos_sensor(0x0212, 0x01);
	write_cmos_sensor(0x0213, 0x00);
	write_cmos_sensor(0x0214, 0x01);
	write_cmos_sensor(0x0215, 0x00);

	write_cmos_sensor(0x0100, 0x01);
}


static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
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
	int ret = 0;
	u8 module_id = 0;

    /* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
	spin_unlock(&imgsensor_drv_lock);
	do {
		*sensor_id = return_sensor_id();
	    ret = s5k3p3_read_otp(0x0001, &module_id);
	    if (*sensor_id == imgsensor_info.sensor_id) {
		LOG_INF("Get s5k3p3 sensor success!i2c write id: 0x%x, sensor id: 0x%x, module_id:0x%x\n",
			imgsensor.i2c_write_id, *sensor_id, module_id);
		*sensor_id = S5K3P3SX_SENSOR_ID;
		goto otp_read;
	    }
	    LOG_INF("Read sensor id fail, addr: 0x%x, sensor_id:0x%x, module_id:0x%x\n",
			imgsensor.i2c_write_id, *sensor_id, module_id);
	    retry--;
	} while (retry > 0);
	i++;
	retry = 1;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
	/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
	*sensor_id = 0xFFFFFFFF;
	return ERROR_SENSOR_CONNECT_FAIL;
		}

otp_read:
	/*
	 * read lsc calibration from OTP E2PROM.
	 */
	s5k3p3_primax_otp_buf = kzalloc(OTP_DATA_SIZE, GFP_KERNEL);

	/* read lsc calibration from E2PROM */
	s5k3p3_read_otp_burst(OTP_START_ADDR, s5k3p3_primax_otp_buf);


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
		LOG_INF("2015_12_24 i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
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
	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate)
		LOG_INF("capture30fps: use cap30FPS's setting: %d fps!\n", imgsensor.current_fps/10);
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);

	return ERROR_NONE;
}	/* capture() */

static kal_uint32 hd_4k_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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
	hd_4k_setting();

	return ERROR_NONE;
}	/*	hd_4k_video   */

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

	sensor_resolution->SensorCustom2Width = imgsensor_info.custom2.grabwindow_width;
	sensor_resolution->SensorCustom2Height = imgsensor_info.custom2.grabwindow_height;

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
	sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame;
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
	case MSDK_SCENARIO_ID_CUSTOM2:
	sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx;
	sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;
	sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom2.mipi_data_lp2hs_settle_dc;
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
	case MSDK_SCENARIO_ID_CUSTOM2:
	hd_4k_video(image_window, sensor_config_data);
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
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
	if (framerate == 0)
	return ERROR_NONE;
	frame_length = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.dummy_line = (frame_length > imgsensor_info.custom2.framelength)
				? (frame_length - imgsensor_info.custom2.framelength) : 0;
	imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength)
				? (frame_length - imgsensor_info.cap.framelength) : 0;
	imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
	frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength)
				? (frame_length - imgsensor_info.hs_video.framelength) : 0;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
	frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength)
				? (frame_length - imgsensor_info.slim_video.framelength) : 0;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
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
	case MSDK_SCENARIO_ID_CUSTOM2:
	*framerate = imgsensor_info.custom2.max_framerate;
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
		write_cmos_sensor(0x0601, 0x02);
	else
		write_cmos_sensor(0x0601, 0x00);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(0x0100, 0X01);
	else
		write_cmos_sensor(0x0100, 0x00);
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
	case MSDK_SCENARIO_ID_CUSTOM2:
	memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[5], sizeof(SENSOR_WINSIZE_INFO_STRUCT));
	break;
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	default:
	memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0], sizeof(SENSOR_WINSIZE_INFO_STRUCT));
	break;
	}
	break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
	ihdr_write_shutter_gain((UINT16)*feature_data, (UINT16)*(feature_data+1),
	(UINT16)*(feature_data+2));
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


UINT32 S5K3P3_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc =  &sensor_func;
	return ERROR_NONE;
}	/*	S5K3P3_MIPI_RAW_SensorInit	*/
