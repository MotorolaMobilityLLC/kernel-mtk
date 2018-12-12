/*
 * Copyright (C) 2018 MediaTek Inc.
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

/*
 * NOTICE: SENSOR LIMITATION
 *
 * 1. command_update could only be issued once in a frame,
 *    or randomly not apply new value.
 * 2. set_gain will follow set_shutter in a frame.
 * 3. AE and flashlight control couldn't be applied on the same frame.
 * 4. For dual cam case, command_update could only apply on master sensor.
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
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "hm1062_2mipimono_sensor.h"

/************************* Modify Following Strings for Debug ****************/
#define PFX "hm1062_2_camera_primax mono"
#define LOG_INF(format, args...) pr_info(PFX "[%s] " format, __func__, ##args)
#define LOG_1 LOG_INF("HM1062,MIPI 1LANE\n")
/************************* Modify end ****************************************/

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	/* Sensor ID Value, record sensor id defined in kd_imgsensor.h */
	.sensor_id = HM1062_MONO_SENSOR_ID,

	.checksum_value =  0xa353fed, /*checksum value for Camera Auto Test */

	.pre = {
		.pclk = 79750000, /* record different mode's pclk */
		.linelength  = 1414, /* record different mode's linelength */
		.framelength = 1880, /* record different mode's framelength */
		.startx = 8,
		.starty = 8, /* record different mode's starty of grabwindow */
		/* record different mode's width of grabwindow */
		.grabwindow_width  = 1280,
		/* record different mode's height of grabwindow */
		.grabwindow_height = 720,
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.max_framerate = 300,
		.mipi_pixel_rate = 85700000,
	},
	.pre1 = {
		.pclk = 79750000, /* record different mode's pclk */
		.linelength  = 1414, /* record different mode's linelength */
		.framelength = 3760, /* record different mode's framelength */
		.startx = 8,
		.starty = 8, /* record different mode's starty of grabwindow */
		/* record different mode's width of grabwindow */
		.grabwindow_width  = 1280,
		/* record different mode's height of grabwindow */
		.grabwindow_height = 720,
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.max_framerate = 150,
		.mipi_pixel_rate = 85700000,
	},
	.cap = {
		.pclk = 79750000, /* record different mode's pclk */
		.linelength  = 1414, /* record different mode's linelength */
		.framelength = 1880, /* record different mode's framelength */
		.startx = 8, /* record different mode's startx of grabwindow */
		.starty = 8, /* record different mode's starty of grabwindow */
		/* record different mode's width of grabwindow */
		.grabwindow_width  = 1280,
		/* record different mode's height of grabwindow */
		.grabwindow_height = 720,
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.max_framerate = 300,
		.mipi_pixel_rate = 85700000,
	},
	.cap1 = {
		.pclk = 79750000, /* record different mode's pclk */
		.linelength  = 1414, /* record different mode's linelength */
		.framelength = 3760, /* record different mode's framelength */
		.startx = 8, /* record different mode's startx of grabwindow */
		.starty = 8, /* record different mode's starty of grabwindow */
		/* record different mode's width of grabwindow */
		.grabwindow_width  = 1280,
		/* record different mode's height of grabwindow */
		.grabwindow_height = 720,
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.max_framerate = 150,
		.mipi_pixel_rate = 85700000,
	},
	.normal_video = {
		.pclk = 79750000, /*record different mode's pclk*/
		.linelength  = 1414, /*record different mode's linelength*/
		.framelength = 1880, /*record different mode's framelength*/
		.startx = 8, /*record different mode's startx of grabwindow*/
		.starty = 8, /*record different mode's starty of grabwindow*/
		/*record different mode's width of grabwindow*/
		.grabwindow_width  = 1280,
		/*record different mode's height of grabwindow*/
		.grabwindow_height = 720,
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.max_framerate = 300,
		.mipi_pixel_rate = 85700000,
	},
	.hs_video = {
		.pclk = 79750000, /* record different mode's pclk */
		.linelength  = 1414, /* record different mode's linelength */
		.framelength = 940, /* record different mode's framelength */
		.startx = 8, /* record different mode's startx of grabwindow */
		.starty = 8, /* record different mode's starty of grabwindow */
		/* record different mode's width of grabwindow */
		.grabwindow_width  = 1280,
		/* record different mode's height of grabwindow */
		.grabwindow_height = 720,
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.max_framerate = 600,
		.mipi_pixel_rate = 85700000,
	},
	.slim_video = {
		.pclk = 79750000, /* record different mode's pclk */
		.linelength  = 1414, /* record different mode's linelength */
		.framelength = 1880, /* record different mode's framelength */
		.startx = 8, /* record different mode's startx of grabwindow */
		.starty = 8, /* record different mode's starty of grabwindow */
		/* record different mode's width of grabwindow */
		.grabwindow_width  = 1280,
		/* record different mode's height of grabwindow */
		.grabwindow_height = 720,
		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 85,
		/* following for GetDefaultFramerateByScenario() */
		.max_framerate = 300,
		.mipi_pixel_rate = 85700000,
	},

	.margin = 10,		/* sensor framelength & shutter margin */
	.min_shutter = 2,	/* min shutter */
	/* REG0x0202 <= REG0x0340-5
	 * //max framelength by sensor register's limitation
	 */
	.max_frame_length = 0xFFFE,
	/* shutter delay frame for AE cycle,
	 * 2 frame with ispGain_delay-shut_delay=2-0=2
	 */
	.ae_shut_delay_frame = 0,
	/* sensor gain delay frame for AE cycle,
	 * 2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	 */
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,/* isp gain delay frame for AE cycle */
	.frame_time_delay_frame = 2,
	.ihdr_support = 0,	  /* 1, support; 0,not support */
	.ihdr_le_firstline = 0,  /* 1,le first ; 0, se first */
	.temperature_support = 0, /* 1, support; 0,not support */
	/* support sensor mode num ,don't support Slow motion */
	.sensor_mode_num = 5,

	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */
	.hs_video_delay_frame = 2,/* enter high speed video  delay frame num */
	.slim_video_delay_frame = 2,/* enter slim video delay frame num */

	.isp_driving_current = ISP_DRIVING_8MA, /* mclk driving current */
	/* sensor_interface_type */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	/* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,
	/* sensor output first pixel color */
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_IR,
	.mclk = 24,/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_1_LANE,/* mipi lane num */
	/* record sensor support all write id addr,
	 * only supprt 4must end with 0xff
	 */
	.i2c_addr_table = {0x68, 0x48, 0xff},
	.i2c_speed = 400,	/* i2c read/write speed */
	.sync_mode_capacity = SENSOR_MASTER_SYNC_MODE | SENSOR_SLAVE_SYNC_MODE,
};


static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_H_MIRROR,	/* mirrorflip information */
	/* IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT,
	 * Preview, Capture, Video,High Speed Video, Slim Video
	 */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x200,	/* current shutter */
	.gain = 0x200,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	/* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.current_fps = 0,
	/* auto flicker enable: KAL_FALSE for disable auto flicker,
	 * KAL_TRUE for enable auto flicker
	 */
	.autoflicker_en = KAL_FALSE,
	/* test pattern mode or not. KAL_FALSE for in test pattern mode,
	 * KAL_TRUE for normal output
	 */
	.test_pattern = KAL_FALSE,
	/* current scenario id */
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = KAL_FALSE, /* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x20,/* record current sensor's i2c write id */
	.sync_mode = SENSOR_NO_SYNC_MODE,
};

/* Sensor output window information*/
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
	{ 1296, 736, 0,	0, 1296, 736, 1296, 736,
		0, 0, 1296, 736, 8, 8, 1280, 720}, /* Preview */
	{ 1296, 736, 0,	0, 1296, 736, 1296, 736,
		0, 0, 1296, 736, 8, 8, 1280, 720}, /* Capture */
	{ 1296, 736, 0,	0, 1296, 736, 1296, 736,
		0, 0, 1296, 736, 8, 8, 1280, 720}, /* Normal video */
	{ 1296, 736, 0,	0, 1296, 736, 1296, 736,
		0, 0, 1296, 736, 8, 8, 1280, 720}, /* Hs video */
	{ 1296, 736, 0,	0, 1296, 736, 1296, 736,
		0, 0, 1296, 736, 8, 8, 1280, 720}, /* Slim video */
};

static IMGSENSOR_CTRL_PIN_CAPABILITY ctrl_pin_capability = {
	.flash = 1,
	.strobe = 1,
};

static struct IMGSENSOR_I2C_CFG *get_i2c_cfg(void)
{
	return &(((struct IMGSENSOR_SENSOR_INST *)
		  (imgsensor.psensor_func->psensor_inst))->i2c_cfg);
}

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	imgsensor_i2c_read(
		get_i2c_cfg(),
		pu_send_cmd,
		2,
		(u8 *)&get_byte,
		1,
		imgsensor.i2c_write_id,
		imgsensor_info.i2c_speed);
	return get_byte;
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[3] = {
		(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	imgsensor_i2c_write(
		get_i2c_cfg(),
		pusendcmd,
		3,
		3,
		imgsensor.i2c_write_id,
		imgsensor_info.i2c_speed);
}

#define MULTI_WRITE 1
#if MULTI_WRITE
#define I2C_BUFFER_LEN 765	/* trans# max is 255, each 3 bytes */
#else
#define I2C_BUFFER_LEN 3
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
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;

		}
#if MULTI_WRITE
		/* Write when remain buffer size is less than 3 bytes
		 * or reach end of data
		 */
		if ((I2C_BUFFER_LEN - tosend) < 3 || IDX == len
		    || addr != addr_last) {
			LOG_INF("burst write with addr = 0x%x\n", addr);
			imgsensor_i2c_write(get_i2c_cfg(),
					    puSendCmd,
					    tosend,
					    3,
					    imgsensor.i2c_write_id,
					    imgsensor_info.i2c_speed);
			tosend = 0;
		}
#else
		imgsensor_i2c_write(get_i2c_cfg(),
				    puSendCmd,
				    3,
				    3,
				    imgsensor.i2c_write_id,
				    imgsensor_info.i2c_speed);

		tosend = 0;

#endif
	}
	return 0;
}

static void command_update(void)
{
	if (imgsensor.sync_mode == SENSOR_SLAVE_SYNC_MODE)
		hm1062_cmu();
	else
		hm1062_2_cmu();
}

static void ae_command_update(void)
{
	if (imgsensor.sync_mode == SENSOR_SLAVE_SYNC_MODE)
		hm1062_ae_cmu(imgsensor.sync_mode);
	else
		hm1062_2_ae_cmu(imgsensor.sync_mode);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n",
		imgsensor.dummy_line, imgsensor.dummy_pixel);
	LOG_INF("frame_length = %d, line_length = %d\n",
		imgsensor.frame_length,
		imgsensor.line_length);
	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel
	 * , or you can set dummy by imgsensor.frame_length and
	 * imgsensor.line_length
	 */
	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFE);
	write_cmos_sensor(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor(0x0343, imgsensor.line_length & 0xFE);

	/* ae_command_update(); // update by set_gain */
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{

	kal_uint32 frame_length = imgsensor.frame_length;
	/* unsigned long flags; */

	LOG_INF("framerate = %d, min framelength should enable(%d)\n",
		framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ?
		frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line =
		imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length
			- imgsensor.min_frame_length;
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

	LOG_INF("Enter! shutter =%d\n", shutter);

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	/* write_shutter(shutter); */
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure
	 * larger than frame exposure AE doesn't update sensor gain
	 * at capture mode, thus extra exposure lines must be updated here.
	 */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length,
	 * should extend frame length first
	 */
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ?
		imgsensor_info.min_shutter : shutter;
	shutter =
	(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
	? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10
			/ imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
			write_cmos_sensor(0x0341,
					  imgsensor.frame_length & 0xFE);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFE);
	}

	/* Update Shutter */
	write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor(0x0203, shutter & 0xFE);

	/* Update LED Pulse Length */
	write_cmos_sensor(0x3321, (shutter >> 8) & 0xFF);
	write_cmos_sensor(0x3322, shutter & 0xFE);
	/* ae_command_update(); // update by set_gain */
	LOG_INF("Exit! shutter =%d, framelength =%d\n",
		shutter, imgsensor.frame_length);
}

#define HII_GAIN_CODE

#ifdef HII_GAIN_CODE
#define HM1062MIPI_MaxGainIndex (65)
static const kal_uint16 HM1062_sensorGainMapping[HM1062MIPI_MaxGainIndex][2] = {
	{64, 0},
	{68, 1},
	{72, 2},
	{76, 3},
	{80, 4},
	{84, 5},
	{88, 6},
	{92, 7},
	{96, 8},
	{100, 9},
	{104, 10},
	{108, 11},
	{112, 12},
	{116, 13},
	{120, 14},
	{124, 15},
	{128, 16},
	{136, 17},
	{144, 18},
	{152, 19},
	{160, 20},
	{168, 21},
	{176, 22},
	{184, 23},
	{192, 24},
	{200, 25},
	{208, 26},
	{216, 27},
	{224, 28},
	{232, 29},
	{240, 30},
	{248, 31},
	{256, 32},
	{272, 33},
	{288, 34},
	{304, 35},
	{320, 36},
	{336, 37},
	{352, 38},
	{368, 39},
	{384, 40},
	{400, 41},
	{416, 42},
	{432, 43},
	{448, 44},
	{464, 45},
	{480, 46},
	{496, 47},
	{512, 48},
	{544, 49},
	{576, 50},
	{608, 51},
	{640, 52},
	{672, 53},
	{704, 54},
	{736, 55},
	{768, 56},
	{800, 57},
	{832, 58},
	{864, 59},
	{896, 60},
	{928, 61},
	{960, 62},
	{992, 63},
	{1024, 64},
};
#else /* SMIA GAIN CODE */
#define HM1062MIPI_MaxGainIndex (65)
static const kal_uint16 HM1062_sensorGainMapping[HM1062MIPI_MaxGainIndex][2] = {
	{64, 0},
	{68, 1},
	{72, 2},
	{76, 3},
	{80, 4},
	{84, 5},
	{88, 6},
	{92, 7},
	{96, 8},
	{100, 9},
	{104, 10},
	{108, 11},
	{112, 12},
	{116, 13},
	{120, 14},
	{124, 15},
	{128, 16},
	{136, 18},
	{144, 20},
	{152, 22},
	{160, 24},
	{168, 26},
	{176, 28},
	{184, 30},
	{192, 32},
	{200, 34},
	{208, 36},
	{216, 38},
	{224, 40},
	{232, 42},
	{240, 44},
	{248, 46},
	{256, 48},
	{272, 52},
	{288, 56},
	{304, 60},
	{320, 64},
	{336, 68},
	{352, 72},
	{368, 76},
	{384, 80},
	{400, 84},
	{416, 88},
	{432, 92},
	{448, 96},
	{464, 100},
	{480, 104},
	{496, 108},
	{512, 112},
	{544, 120},
	{576, 128},
	{608, 136},
	{640, 144},
	{672, 152},
	{704, 160},
	{736, 168},
	{768, 176},
	{800, 184},
	{832, 192},
	{864, 200},
	{896, 208},
	{928, 216},
	{960, 224},
	{992, 232},
	{1024, 240},
};
#endif
static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint8 iI = 0;

	for (iI = 0; iI < HM1062MIPI_MaxGainIndex; iI++) {
		if (gain <= HM1062_sensorGainMapping[iI][0])
			return HM1062_sensorGainMapping[iI][1];
	}
	LOG_INF("exit HM1062_sensorGainMapping function\n");
	return HM1062_sensorGainMapping[iI-1][1];
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
	kal_uint16 reg_gain = 0x0000;

	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = gain;
	spin_unlock(&imgsensor_drv_lock);

	LOG_INF("set_gain %d\n", gain);
	if (gain < BASEGAIN || gain > 16 * BASEGAIN) {
		LOG_INF("Error gain setting");
		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 16 * BASEGAIN)
			gain = 16 * BASEGAIN;
	}

	reg_gain = gain2reg(gain);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x0205, (reg_gain & 0xFF));

	ae_command_update();

	return gain;
}	/*	set_gain  */


#if 1
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
	/* command_update(); */

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

static kal_uint16 addr_data_pair_init_hm1062[] = {
	0x0100, 0x00,
	0x0103, 0x00,
	0x0304, 0x09,
	0x0305, 0x0C,
	0x0307, 0x78,
	0x0303, 0x03,
	0x0309, 0x10,
	0x030A, 0x09,
	0x030D, 0x02,
	0x030F, 0x24,
	0x3020, 0x24,
	0x3021, 0x92,
	0x3022, 0x16,
	0x3023, 0x82,
	0x3024, 0x12,
	0x0100, 0x02,
	0x4265, 0x02,
	0x4001, 0x00,
	0x4002, 0x23,
	0x0101, 0x00,
	0x4024, 0x40,
	0x0202, 0x02,
	0x0203, 0xEC,
	0x0340, 0x07,
	0x0341, 0x58,
	0x0342, 0x05,
	0x0343, 0x86,
	0x0350, 0x03,
	0x4274, 0xB3,
	0x0350, 0x13,
	0x4131, 0x01,
	0x4142, 0x00,
	0x4149, 0x41,
	0x417E, 0x35,
	0x417F, 0xFF,
	0x426C, 0x03,
	0x4275, 0x40,
	0x4276, 0x2F,
	0x427E, 0x77,
#ifdef HII_GAIN_CODE
	0x427F, 0x01,
#else /* SMIA GAIN CODE */
	0x427F, 0x00,
#endif
	0x4380, 0x90,
	0x4381, 0x25,
	0x4382, 0x00,
	0x4383, 0xC0,
	0x4385, 0xC2,
	0x4386, 0x15,
	0x4387, 0xF7,
	0x4389, 0x00,
	0x438A, 0x55,
	0x438C, 0x57,
	0x438D, 0x00,
	0x438E, 0x80,
	0x438F, 0x00,
	0x4400, 0x00,
	0x4401, 0xC0,
	0x4402, 0x08,
	0x4403, 0x64,
	0x44D0, 0x3C,
	0x44E1, 0xA0,
	0x4C00, 0x08,
	0x45FC, 0x0B,
	0x45FD, 0x08,
	0x45FE, 0x00,
	0x45FF, 0x00,
	0x4B3B, 0x02,
	0x4B45, 0x01,
	0x4B47, 0x00,
	0x4490, 0x30,
	0x4491, 0x31,
	0x4492, 0x33,
	0x4493, 0x37,
	0x4494, 0x3F,
	0x4495, 0x7F,
	0x4496, 0x6F,
	0x4498, 0xB0,
	0x4499, 0x30,
	0x449A, 0x31,
	0x449B, 0x33,
	0x449C, 0x37,
	0x449D, 0x3F,
	0x449E, 0x7F,
	0x44A0, 0x30,
	0x44A1, 0x31,
	0x44A2, 0x33,
	0x44A3, 0x37,
	0x44A4, 0x3F,
	0x44A5, 0x7F,
	0x44A6, 0x6F,
	0x44A8, 0xB0,
	0x44A9, 0x30,
	0x44AA, 0x31,
	0x44AB, 0x33,
	0x44AC, 0x37,
	0x44AD, 0x3F,
	0x44AE, 0x7F,
	0x44B0, 0x30,
	0x44B1, 0x31,
	0x44B2, 0x33,
	0x44B3, 0x37,
	0x44B4, 0x3F,
	0x44B5, 0x7F,
	0x44B6, 0x6F,
	0x44B8, 0x30,
	0x44B9, 0x31,
	0x44BA, 0x33,
	0x44BB, 0x37,
	0x44BC, 0x3F,
	0x44BD, 0x7F,
	0x44BE, 0x6F,
	0x44C0, 0x30,
	0x44C1, 0x70,
	0x44C2, 0x60,
	0x44C3, 0x50,
	0x44C4, 0x40,
	0x44C5, 0x40,
	0x44C8, 0x30,
	0x44C9, 0x31,
	0x44CA, 0x33,
	0x3006, 0x10,
	0x3007, 0x20,
	0x4680, 0x0F,
	0x4681, 0x5F,
	0x4682, 0x00,
	0x4683, 0xFF,
	0x4684, 0x35,
	0x4685, 0x08,
	0x4686, 0x66,
	0x4687, 0x30,
	0x4688, 0x0F,
	0x4689, 0x5F,
	0x468A, 0x00,
	0x468B, 0xFF,
	0x468C, 0x35,
	0x468D, 0x08,
	0x468E, 0x64,
	0x468F, 0x31,
	0x4690, 0x0F,
	0x4691, 0x5F,
	0x4692, 0x00,
	0x4693, 0xFF,
	0x4694, 0x35,
	0x4695, 0x08,
	0x4696, 0x64,
	0x4697, 0x33,
	0x4698, 0x0F,
	0x4699, 0x5F,
	0x469A, 0x00,
	0x469B, 0xFF,
	0x469C, 0x35,
	0x469D, 0x08,
	0x469E, 0x64,
	0x469F, 0x37,
	0x46A0, 0x0F,
	0x46A1, 0x5F,
	0x46A2, 0x00,
	0x46A3, 0xFF,
	0x46A4, 0x35,
	0x46A5, 0x08,
	0x46A6, 0x64,
	0x46A7, 0x3F,
	0x46A8, 0x0F,
	0x46A9, 0x5F,
	0x46AA, 0x00,
	0x46AB, 0xFF,
	0x46AC, 0x35,
	0x46AD, 0x08,
	0x46AE, 0x64,
	0x46AF, 0x7F,
	0x46B0, 0x0F,
	0x46B1, 0x5F,
	0x46B2, 0x00,
	0x46B3, 0xFF,
	0x46B4, 0x35,
	0x46B5, 0x08,
	0x46B6, 0x64,
	0x46B7, 0x6F,
	0x46C0, 0x0F,
	0x46C1, 0x5F,
	0x46C2, 0x00,
	0x46C3, 0xFF,
	0x46C4, 0x35,
	0x46C5, 0x08,
	0x46C6, 0x66,
	0x46C7, 0xB0,
	0x46C8, 0x0F,
	0x46C9, 0x5F,
	0x46CA, 0x00,
	0x46CB, 0xFF,
	0x46CC, 0x35,
	0x46CD, 0x08,
	0x46CE, 0x66,
	0x46CF, 0x30,
	0x46D0, 0x0F,
	0x46D1, 0x5F,
	0x46D2, 0x00,
	0x46D3, 0xFF,
	0x46D4, 0x35,
	0x46D5, 0x08,
	0x46D6, 0x64,
	0x46D7, 0x31,
	0x46D8, 0x0F,
	0x46D9, 0x5F,
	0x46DA, 0x00,
	0x46DB, 0xFF,
	0x46DC, 0x35,
	0x46DD, 0x08,
	0x46DE, 0x64,
	0x46DF, 0x33,
	0x46E0, 0x0F,
	0x46E1, 0x5F,
	0x46E2, 0x00,
	0x46E3, 0xFF,
	0x46E4, 0x35,
	0x46E5, 0x08,
	0x46E6, 0x64,
	0x46E7, 0x37,
	0x46E8, 0x0F,
	0x46E9, 0x5F,
	0x46EA, 0x00,
	0x46EB, 0xFF,
	0x46EC, 0x35,
	0x46ED, 0x08,
	0x46EE, 0x64,
	0x46EF, 0x3F,
	0x46F0, 0x0F,
	0x46F1, 0x5F,
	0x46F2, 0x00,
	0x46F3, 0xFF,
	0x46F4, 0x35,
	0x46F5, 0x08,
	0x46F6, 0x64,
	0x46F7, 0x7F,
	0x4A00, 0x20,
	0x4A01, 0x30,
	0x4A02, 0x00,
	0x4A03, 0x88,
	0x4A04, 0x7F,
	0x4A05, 0x7F,
	0x4A06, 0x52,
	0x4A07, 0x07,
	0x4A08, 0x85,
	0x4A09, 0x00,
	0x4A0A, 0x03,
	0x4A0B, 0x13,
	0x4A0C, 0x18,
	0x4A0D, 0x33,
	0x4A0E, 0x41,
	0x4A0F, 0x06,
	0x4A10, 0x22,
	0x4A11, 0x08,
	0x3110, 0x00,
	0xBAA2, 0xC0,
	0xBAA2, 0x40,
	0xBA90, 0x01,
	0xBA93, 0x02,
	0x373E, 0x8A,
	0x373F, 0x8A,
	0x4B20, 0x9E,
	0x4B03, 0x0A,
	0x4B05, 0x19,
	0x4B18, 0x10,
	0x4800, 0xAC,
	0x0104, 0x01,
	0x0104, 0x00,
	0x4801, 0xAE,
	0x330E, 0X01,
};

static void sensor_init(void)
{
	LOG_INF("%s.\n", __func__);
}

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("%s.\n", __func__);

	table_write_cmos_sensor(addr_data_pair_init_hm1062,
				sizeof(addr_data_pair_init_hm1062)
				/ sizeof(kal_uint16));

	set_mirror_flip(imgsensor.mirror);

	if (currefps == imgsensor_info.cap1.max_framerate) {
		/* 15fps */
		LOG_INF("%s 15fps\n", __func__);
		write_cmos_sensor(0x0340, 0x0E);
		write_cmos_sensor(0x0341, 0xB0);
	}

	if (imgsensor.is_config_done == KAL_FALSE) {
		LOG_INF("%s mode config\n", __func__);
		if (imgsensor.sync_mode == SENSOR_MASTER_SYNC_MODE) {
			/* Master mode */
			write_cmos_sensor(0x0100, 0x02);
			write_cmos_sensor(0x3300, 0x11);
			LOG_INF("%s master mode\n", __func__);
		} else if (imgsensor.sync_mode == SENSOR_SLAVE_SYNC_MODE) {
			/* Slave mode */
			write_cmos_sensor(0x0100, 0x01);
			mdelay(1);
			write_cmos_sensor(0x0100, 0x02);
			write_cmos_sensor(0x3300, 0x63);
			LOG_INF("%s slave mode\n", __func__);
		}
		spin_lock(&imgsensor_drv_lock);
		imgsensor.is_config_done = KAL_TRUE;
		spin_unlock(&imgsensor_drv_lock);
	}

	command_update();
}	/* capture setting */

static void preview_setting(kal_uint16 currefps)
{
	LOG_INF("%s.\n", __func__);
	capture_setting(currefps);

}	/* preview_setting */

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("%s.\n", __func__);
	capture_setting(300);
}

static void hs_video_setting(void)
{
	LOG_INF("%s.\n", __func__);
	capture_setting(300);

	/* 60 fps */
	LOG_INF("%s 60fps\n", __func__);
	write_cmos_sensor(0x0340, 0x03);
	write_cmos_sensor(0x0341, 0xAC);
	/*command_update();*/
}

static void slim_video_setting(void)
{
	LOG_INF("%s.\n", __func__);
	capture_setting(300);
}

static void set_sync_mode(kal_uint32 sync_mode)
{
	LOG_INF("mode = %d\n", sync_mode);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sync_mode = sync_mode;
	spin_unlock(&imgsensor_drv_lock);
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

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if  (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF(
				"hm1062 i2c write id: 0x%x, sensor id: 0x%x\n",
				imgsensor.i2c_write_id, *sensor_id);
				*sensor_id = HM1062_2_MONO_SENSOR_ID;
				return ERROR_NONE;

			}
			LOG_INF(
			    "Read sensor id fail, addr: 0x%x, sensor_id:0x%x\n",
			    imgsensor.i2c_write_id, *sensor_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 1;
	}
	if  (*sensor_id != imgsensor_info.sensor_id) {
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
	kal_uint32 sensor_id = 0;

	LOG_1;

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF(
				"hm1062 i2c write id: 0x%x, sensor id: 0x%x\n",
				imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF(
			    "Read sensor id fail, id: 0x%x, sensor id: 0x%x\n",
			    imgsensor.i2c_write_id, sensor_id);
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
	imgsensor.sync_mode = SENSOR_NO_SYNC_MODE,
	imgsensor.is_config_done = KAL_FALSE;
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
*  *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
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
	if (imgsensor.current_fps == imgsensor_info.pre1.max_framerate) {
		imgsensor.pclk = imgsensor_info.pre1.pclk;
		imgsensor.line_length = imgsensor_info.pre1.linelength;
		imgsensor.frame_length = imgsensor_info.pre1.framelength;
		imgsensor.min_frame_length = imgsensor_info.pre1.framelength;
	} else {
		imgsensor.pclk = imgsensor_info.pre.pclk;
		imgsensor.line_length = imgsensor_info.pre.linelength;
		imgsensor.frame_length = imgsensor_info.pre.framelength;
		imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	}
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting(imgsensor.current_fps);

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
	} else {
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	}
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);

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

static kal_uint32 get_resolution(
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT * sensor_resolution)
{
	LOG_INF("%s.\n", __func__);
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

	sensor_resolution->SensorSlimVideoWidth	=
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

	/* sensor_info->SensorVideoFrameRate =
	 * imgsensor_info.normal_video.max_framerate/10;  //not use
	 * sensor_info->SensorStillCaptureFrameRate=
	 * imgsensor_info.cap.max_framerate/10; // not use
	 * imgsensor_info->SensorWebCamCaptureFrameRate=
	 * imgsensor_info.v.max_framerate;// not use
	 */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* not use */
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* inverse with datasheet */
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
	sensor_info->FrameTimeDelayFrame =
		imgsensor_info.frame_time_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->TEMPERATURE_SUPPORT = imgsensor_info.temperature_support;
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

	sensor_info->SensorHorFOV = 67;

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
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 control(
	MSDK_SCENARIO_ID_ENUM scenario_id,
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
}

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


static kal_uint32 set_max_framerate_by_scenario(
	MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		if (imgsensor.current_fps ==
		    imgsensor_info.pre1.max_framerate) {
			frame_length = imgsensor_info.pre1.pclk
				/ framerate * 10
				/ imgsensor_info.pre1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			    (frame_length > imgsensor_info.pre1.framelength) ?
			    (frame_length - imgsensor_info.pre1.framelength) :
			    0;
			imgsensor.frame_length = imgsensor_info.pre1.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
		} else {
			frame_length = imgsensor_info.pre.pclk / framerate * 10
				/ imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			    (frame_length > imgsensor_info.pre.framelength) ?
			    (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
		}
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length =
			imgsensor_info.normal_video.pclk / framerate * 10
			/ imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		    (frame_length > imgsensor_info.normal_video.framelength) ?
		    (frame_length - imgsensor_info.normal_video.framelength) :
		    0;
		imgsensor.frame_length =
			imgsensor_info.normal_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps ==
		    imgsensor_info.cap1.max_framerate) {
			frame_length = imgsensor_info.cap1.pclk
				/ framerate * 10
				/ imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			    (frame_length > imgsensor_info.cap1.framelength) ?
			    (frame_length - imgsensor_info.cap1.framelength) :
			    0;
			imgsensor.frame_length = imgsensor_info.cap1.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
		} else {
			frame_length = imgsensor_info.cap.pclk
				/ framerate * 10
				/ imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			    (frame_length > imgsensor_info.cap.framelength) ?
			    (frame_length - imgsensor_info.cap.framelength) :
			    0;
			imgsensor.frame_length = imgsensor_info.cap.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
		}
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk
			/ framerate * 10 / imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.hs_video.framelength) ?
			(frame_length - imgsensor_info.hs_video.framelength) :
			0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk
			/ framerate * 10 / imgsensor_info.slim_video.linelength;
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
		set_dummy();
		break;
	default:
		frame_length = imgsensor_info.pre.pclk
			/ framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		LOG_INF("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		set_dummy();
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
	LOG_INF("%s, enable: %d\n", __func__, enable);

	write_cmos_sensor(0x0601, enable ? 0x02 : 0x00);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	if (imgsensor.sync_mode == SENSOR_SLAVE_SYNC_MODE) {
		hm1062_stream_on(enable, imgsensor.shutter, imgsensor.gain);
	} else {
		/* master mode or no sync mode */
		if (enable)
			write_cmos_sensor(0x0100, 0X01);
		else
			write_cmos_sensor(0x0100, 0x02);
		LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	}
	return ERROR_NONE;
}

static kal_uint32 set_flashlight(MUINT16 mode)
{

	LOG_INF("set_flashlight hm1062_2 %d", mode);
	switch (mode) {
	case 0:
		LOG_INF("strobe trigger mode 0 (flash off)\n");
		write_cmos_sensor(0x3310, 0X10);
		command_update();
		break;
	case 1:
		LOG_INF("strobe trigger mode 1\n");
		write_cmos_sensor(0x3310, 0X11);
		command_update();
		break;
	case 2:
		LOG_INF("strobe trigger mode 2\n");
		write_cmos_sensor(0x3320, 0X31);
		write_cmos_sensor(0x3310, 0X35);
		command_update();
		break;
	case 3:
		LOG_INF("strobe trigger mode 3\n");
		write_cmos_sensor(0x3320, 0X12);
		write_cmos_sensor(0x3323, 0x00);
		write_cmos_sensor(0x3324, 0x02);
		command_update();
		break;
	case 4:
		LOG_INF("strobe trigger mode 4 (strobe off)\n");
		write_cmos_sensor(0x3320, 0X10);
		command_update();
		break;
	case 5:
		LOG_INF("strobe trigger mode 5 (interleave off)\n");
		write_cmos_sensor(0x3310, 0X10);
		write_cmos_sensor(0x3320, 0X10);
		command_update();
		break;
	default:
		break;
	}
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
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

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
		set_gain((UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		set_flashlight((MUINT16) *feature_data);
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
		/* get the lens driver ID from EEPROM or just
		 * return LENS_DRIVER_ID_DO_NOT_CARE
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
			(MSDK_SCENARIO_ID_ENUM)*feature_data,
			*(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data + 1)));
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
		LOG_INF("current fps :%d\n", (UINT32)*feature_data);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			(UINT32)*feature_data);

		wininfo =
		(SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data + 1));

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
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.cap.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.slim_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.pre.mipi_pixel_rate;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_SYNC_MODE_CAPACITY:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_SYNC_MODE_CAPACITY\n");
		*feature_return_para_32 = imgsensor_info.sync_mode_capacity;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_SENSOR_SYNC_MODE:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_SYNC_MODE\n");
		*feature_return_para_32 = imgsensor.sync_mode;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_SENSOR_SYNC_MODE:
		LOG_INF("SENSOR_FEATURE_SET_SENSOR_SYNC_MODE\n");
		set_sync_mode((MUINT32) (*feature_data_32));
		break;
	case SENSOR_FEATURE_GET_CTRL_PIN_CAPABILITY:
		LOG_INF("SENSOR_FEATURE_GET_CTRL_PIN_CAPABILITY\n");
		memcpy((void *)feature_return_para_32,
		       (void *)&ctrl_pin_capability,
		       sizeof(kal_uint32));
		*feature_para_len = 4;
		break;
	default:
		break;
	}

	return ERROR_NONE;
}

void hm1062_2_stream_on(kal_bool enable, kal_uint32 shutter, kal_uint16 gain)
{
	LOG_INF("hm1062_2_stream_on\n");
	if (imgsensor.is_config_done) {
		if (enable) {
			/* workaround: set shutter and gain again */
			set_shutter(shutter);
			set_gain(gain);
			write_cmos_sensor(0x0100, 0X01);
		} else {
			write_cmos_sensor(0x0100, 0x02);
		}
		LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	}
}

void hm1062_2_cmu(void)
{
	LOG_INF("hm1062_2 cmu\n");
	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor(0x0104, 0x00);
}

void hm1062_2_ae_cmu(kal_uint32 mode)
{
	static kal_uint32 status;

	LOG_INF("hm1062_2 ae cmu (%d)\n", mode);

	status |= mode;
	if (mode == SENSOR_NO_SYNC_MODE ||
	    status == (SENSOR_MASTER_SYNC_MODE | SENSOR_SLAVE_SYNC_MODE)) {
		/* update when both master and slave operating done */
		status = 0;
		hm1062_2_cmu();
	}
}

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 HM1062_2_MIPI_MONO_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	sensor_func.arch = IMGSENSOR_ARCH_V2;
	if (pfFunc != NULL)
		*pfFunc =  &sensor_func;
	if (imgsensor.psensor_func == NULL)
		imgsensor.psensor_func = &sensor_func;
	return ERROR_NONE;
}
