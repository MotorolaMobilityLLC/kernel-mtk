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
 *	 IMX486mipiraw_Sensor.c
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

/********************Modify Following Strings for Debug***********************/
#define PFX "IMX486_camera_sensor"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__
/****************************   Modify end    *******************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "imx486mipiraw_Sensor.h"



static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {

	/* IMX486MIPI_SENSOR_ID, sensor_id = 0x2680*/
	.sensor_id = IMX486_SENSOR_ID,

	.checksum_value = 0x38ebe79e, /* checksum value for Camera Auto Test */

	.pre = {
		.pclk = 168000000,	/* record different mode's pclk */
		.linelength = 2096,	/* record different mode's linelength */
		.framelength = 2670,  /* record different mode's framelength */
		.startx = 0, /* record different mode's startx of grabwindow */
		.starty = 0, /* record different mode's starty of grabwindow */

		/* record different mode's width of grabwindow */
		.grabwindow_width = 2016,

		/* record different mode's height of grabwindow */
		.grabwindow_height = 1508,

		/* following for MIPIDataLowPwr2HighSpeedSettleDelayCount
		 * by different scenario
		 */
		.mipi_data_lp2hs_settle_dc = 14,	/* unit , ns */
		/*       following for GetDefaultFramerateByScenario()  */
		.max_framerate = 300,
	},
	.cap = {		/*normal capture */
		.pclk = 386000000,
		.linelength = 4192,
		.framelength = 3068,	/* 1332, */
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 3016,
		.mipi_data_lp2hs_settle_dc = 14,	/* unit , ns */
		.max_framerate = 300,	/* 300, */
	},
	.cap1 = {		/*PIP capture */
		 .pclk = 194000000,
		 .linelength = 4192,
		 .framelength = 3068,
		 .startx = 0,
		 .starty = 0,
		 .grabwindow_width = 4032,
		 .grabwindow_height = 3016,
		 .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		 .max_framerate = 150,
	},
	.normal_video = {
			 .pclk = 386000000,
			 .linelength = 4192,
			 .framelength = 3068,
			 .startx = 0,
			 .starty = 0,
			 .grabwindow_width = 4032,
			 .grabwindow_height = 3016,
			 .mipi_data_lp2hs_settle_dc = 14,	/* unit , ns */
			 .max_framerate = 300,	/* modify */
	},
	.hs_video = {		/*slow motion */
		     .pclk = 168000000,	/* 518400000, */
		     .linelength = 2096,
		     .framelength = 2670,	/* 806, */
		     .startx = 0,
		     .starty = 0,
		     .grabwindow_width = 2016,	/* 1400, */
		     .grabwindow_height = 1508,	/* 752, */
		     .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		     .max_framerate = 300,	/* modify */

	},
	.slim_video = {		/*VT Call */
		       .pclk = 168000000,	/* 158400000, */
		       .linelength = 2096,
		       .framelength = 2670,	/* 986,      //1236 */
		       .startx = 0,
		       .starty = 0,
		       .grabwindow_width = 2016,	/* 1400, */
		       .grabwindow_height = 1508,	/* 752, */
		       .mipi_data_lp2hs_settle_dc = 85,	/* unit , ns */
		       .max_framerate = 300,
	},

	.margin = 10,/* sensor framelength & shutter margin */
	.min_shutter = 4,	/* 1,          //min shutter */

	/* max framelength by sensor register's limitation */
	.max_frame_length = 0xff00,
	.ae_shut_delay_frame = 0,

	/* shutter delay frame for AE cycle,
	 * 2 frame with ispGain_delay-shut_delay=2-0=2
	 */
	.ae_sensor_gain_delay_frame = 0,

	/* sensor gain delay frame for AE cycle,
	 * 2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	 */
	.ae_ispGain_delay_frame = 2, /* isp gain delay frame for AE cycle */

	/* The delay frame of setting frame length	*/
	.frame_time_delay_frame = 2,

	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.sensor_mode_num = 5,	/* support sensor mode num */

	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */
	.hs_video_delay_frame = 2, /* enter high speed video  delay frame num */
	.slim_video_delay_frame = 2,	/* enter slim video delay frame num */

	.isp_driving_current = ISP_DRIVING_4MA,	/* mclk driving current */

	/* sensor_interface_type */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,

	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,

	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	/* sensor output first pixel color */
	.mclk = 24,	/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_4_LANE,	/* mipi lane num */

/* record sensor support all write id addr, only supprt 4must end with 0xff */
	.i2c_addr_table = {0x34, 0x20, 0xff},
	.i2c_speed = 400,
};


static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,	/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x14d,	/* current shutter */
	.gain = 0x0000,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */

	/* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,

	/* current scenario id */
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,

	.hdr_mode = 0,	/* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x34,	/* record current sensor's i2c write id */
};

static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] = {
	{4032, 3016, 0, 0, 4032, 3016, 2016, 1508,
	 0000, 0000, 2016, 1508, 0, 0, 2016, 1508},	/* Preview */
	{4032, 3016, 0, 0, 4208, 3016, 4032, 3016,
	 0000, 0000, 4032, 3016, 0, 0, 4032, 3016},	/*capture */
	{4032, 3016, 0, 0, 4032, 3016, 4032, 3016,
	 0000, 0000, 4032, 3016, 0, 0, 4032, 3016},	/*video */
	{4032, 3016, 0, 0, 4032, 3016, 2016, 1508,
	 0000, 0000, 2016, 1508, 0, 0, 2016, 1508},	/*hight speed video */
	{4032, 3016, 0, 0, 4200, 3016, 2016, 1508,
	 0000, 0000, 2016, 1508, 0, 0, 2016, 1508},	/*slim video */
	{4032, 3016, 0, 0, 4032, 3016, 4032, 3016,
	 0000, 0000, 4032, 3016, 0, 0, 4032, 3016},	/*custom1 */
	{4032, 3016, 0, 0, 4200, 3016, 2016, 1508,
	 0000, 0000, 2016, 1508, 0, 0, 2016, 1508}	/*custom2 */
};
#if 0
/* #define RAW_TYPE_OVERRIDE     //it's PDO function define */
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 64,
	.i4OffsetY = 16,
	.i4PitchX = 32,
	.i4PitchY = 32,
	.i4PairNum = 4,
	.i4SubBlkW = 16,
	.i4SubBlkH = 16,
	.i4PosL = {{67, 21}, {83, 21}, {75, 21}, {91, 21} },
	.i4PosR = {{67, 33}, {83, 33}, {75, 33}, {91, 33} },


	/* 0:IMAGE_NORMAL,1:IMAGE_H_MIRROR,2:IMAGE_V_MIRROR,3:IMAGE_HV_MIRROR */
	.iMirrorFlip = 0,
};
#endif

#define IMX486MIPI_MaxGainIndex (154)
kal_uint16 IMX486MIPI_sensorGainMapping[IMX486MIPI_MaxGainIndex][2] = {
	{64, 0},
	{65, 8},
	{66, 16},
	{67, 25},
	{68, 30},
	{69, 37},
	{70, 45},
	{71, 51},
	{72, 57},
	{73, 63},
	{74, 67},
	{75, 75},
	{76, 81},
	{77, 85},
	{78, 92},
	{79, 96},
	{80, 103},
	{81, 107},
	{82, 112},
	{83, 118},
	{84, 122},
	{86, 133},
	{88, 140},
	{89, 144},
	{90, 148},
	{93, 159},
	{96, 171},
	{97, 175},
	{99, 182},
	{101, 188},
	{102, 192},
	{104, 197},
	{106, 202},
	{107, 206},
	{109, 211},
	{112, 220},
	{113, 222},
	{115, 228},
	{118, 235},
	{120, 239},
	{125, 250},
	{126, 252},
	{128, 256},
	{129, 258},
	{130, 260},
	{132, 264},
	{133, 266},
	{135, 269},
	{136, 271},
	{138, 274},
	{139, 276},
	{141, 279},
	{142, 282},
	{144, 285},
	{145, 286},
	{147, 290},
	{149, 292},
	{150, 294},
	{155, 300},
	{157, 303},
	{158, 305},
	{161, 309},
	{163, 311},
	{170, 319},
	{172, 322},
	{174, 324},
	{176, 326},
	{179, 329},
	{181, 331},
	{185, 335},
	{189, 339},
	{193, 342},
	{195, 344},
	{196, 345},
	{200, 348},
	{202, 350},
	{205, 352},
	{207, 354},
	{210, 356},
	{211, 357},
	{214, 359},
	{217, 361},
	{218, 362},
	{221, 364},
	{224, 366},
	{231, 370},
	{237, 374},
	{246, 379},
	{250, 381},
	{252, 382},
	{256, 384},
	{260, 386},
	{262, 387},
	{273, 392},
	{275, 393},
	{280, 395},
	{290, 399},
	{306, 405},
	{312, 407},
	{321, 410},
	{331, 413},
	{345, 417},
	{352, 419},
	{360, 421},
	{364, 422},
	{372, 424},
	{386, 427},
	{400, 430},
	{410, 432},
	{420, 434},
	{431, 436},
	{437, 437},
	{449, 439},
	{455, 440},
	{461, 441},
	{468, 442},
	{475, 443},
	{482, 444},
	{489, 445},
	{496, 446},
	{504, 447},
	{512, 448},
	{520, 449},
	{529, 450},
	{537, 451},
	{546, 452},
	{555, 453},
	{565, 454},
	{575, 455},
	{585, 456},
	{596, 457},
	{607, 458},
	{618, 459},
	{630, 460},
	{642, 461},
	{655, 462},
	{669, 463},
	{683, 464},
	{697, 465},
	{713, 466},
	{728, 467},
	{745, 468},
	{762, 469},
	{780, 470},
	{799, 471},
	{819, 472},
	{840, 473},
	{862, 474},
	{886, 475},
	{910, 476},
	{936, 477},
	{964, 478},
	{993, 479},
	{1024, 480}
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(
		pu_send_cmd, 2, (u8 *) &get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static int write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	int ret = 0;
	char pu_send_cmd[3] = {
		(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF) };

	ret = iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);

	return ret;
}

static void set_dummy(void)
{
	/*
	 * pr_info("dummyline = %d, dummypixels = %d\n",
	 * imgsensor.dummy_line, imgsensor.dummy_pixel);
	 */

	write_cmos_sensor(0x0104, 0x01);

	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
	//write_cmos_sensor(0x0342, imgsensor.line_length >> 8);
	//write_cmos_sensor(0x0343, imgsensor.line_length & 0xFF);

	write_cmos_sensor(0x0104, 0x00);
}

static kal_uint32 return_sensor_id(void)
{
/* kal_uint32 tmp = 0; */
	int retry = 10;

	if (write_cmos_sensor(0x0A02, 0x0b) == 0) {
		write_cmos_sensor(0x0A00, 0x01);
		while (retry--) {
			if (read_cmos_sensor(0x0A01) == 0x01)
			return (kal_uint16) ((read_cmos_sensor(0x0A29) << 4) |
				(read_cmos_sensor(0x0A2A) >> 4));
		}
	}

	return 0x00;
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	/* kal_int16 dummy_line; */
	kal_uint32 frame_length = imgsensor.frame_length;
	/* unsigned long flags; */

	pr_info("framerate = %d, min framelength should enable %d\n",
		framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length =
	    (frame_length > imgsensor.min_frame_length)
	    ? frame_length : imgsensor.min_frame_length;

	imgsensor.dummy_line =
		imgsensor.frame_length - imgsensor.min_frame_length;

	/* dummy_line = frame_length - imgsensor.min_frame_length; */
	/* if (dummy_line < 0) */
	/* imgsensor.dummy_line = 0; */
	/* else */
	/* imgsensor.dummy_line = dummy_line; */
	/* imgsensor.frame_length = frame_length + imgsensor.dummy_line; */
	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;

		imgsensor.dummy_line =
			imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}				/*      set_max_framerate  */


static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	/* write_shutter(shutter); */
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK
	 * to get exposure larger than frame exposure
	 */
	/* AE doesn't update sensor gain at capture mode,
	 * thus extra exposure lines must be updated here.
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

	shutter =
(shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;

	shutter =
	(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
	? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk
			/ imgsensor.line_length * 10 / imgsensor.frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x0104, 0x01);
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		      write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor(0x0104, 0x00);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0104, 0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x0104, 0x00);
	}

	/* Update Shutter */
	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor(0x0350, 0x01);	/* Enable auto extend */
	write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor(0x0203, shutter & 0xFF);
	write_cmos_sensor(0x0104, 0x00);
	pr_info("Exit! shutter =%d, framelength =%d, auto_extend=%d\n",
		shutter, imgsensor.frame_length, read_cmos_sensor(0x0350));
}				/*    set_shutter */


static void set_shutter_frame_length(
				kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	/* pr_info("shutter =%d, frame_time =%d\n", shutter, frame_time); */

	/* 0x3500, 0x3501, 0x3502 will increase VBLANK
	 * to get exposure larger than frame exposure
	 */
	/* AE doesn't update sensor gain at capture mode,
	 * thus extra exposure lines must be updated here.
	 */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length,
	 * should extend frame length first
	 */
	spin_lock(&imgsensor_drv_lock);
	/*Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;

	/*  */
	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);

	shutter =
(shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;

	shutter =
	(shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
	? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk
			/ imgsensor.line_length * 10 / imgsensor.frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x0104, 0x01);
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		      write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor(0x0104, 0x00);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0104, 0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x0104, 0x00);
	}

	/* Update Shutter */
	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor(0x0350, 0x00);/* Disable auto extend */
	write_cmos_sensor(0x0202,
		(shutter >> 8) & 0xFF);
	write_cmos_sensor(0x0203, shutter & 0xFF);
	write_cmos_sensor(0x0104, 0x00);

	pr_info("shutter =%d,framelen =%d/%d,dummy_line=%d,autoextend=%d\n",
		shutter, imgsensor.frame_length, frame_length,
		dummy_line, read_cmos_sensor(0x0350));
}				/* set_shutter_frame_length */


static kal_uint16 gain2reg(const kal_uint16 gain)
{
	//kal_uint8 i;
#if 1
	int reg_gain = 0;

	reg_gain = 512-(100*512*64/gain)/100;
	if (reg_gain < 0)
		reg_gain = 0;
	if (reg_gain > 480)
		reg_gain = 480;
	return reg_gain;
#else
	for (i = 0; i < IMX486MIPI_MaxGainIndex; i++) {
		if (gain <= IMX486MIPI_sensorGainMapping[i][0])
			break;
	}
	if (gain != IMX486MIPI_sensorGainMapping[i][0])
		pr_info("Gain mapping don't correctly:%d %d\n", gain,
			IMX486MIPI_sensorGainMapping[i][0]);
	return IMX486MIPI_sensorGainMapping[i][1];
#endif
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

	/* 0x350A[0:1], 0x350B[0:7] AGC real gain */
	/* [0:3] = N meams N /16 X        */
	/* [4:9] = M meams M X             */
	/* Total gain = M + N /16 X   */

	/*  */
	if (gain < BASEGAIN || gain > 16 * BASEGAIN) {
		pr_info("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 16 * BASEGAIN)
			gain = 16 * BASEGAIN;
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	pr_info("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor(0x0204, (reg_gain >> 8) & 0xFF);
	write_cmos_sensor(0x0205, reg_gain & 0xFF);
	write_cmos_sensor(0x0104, 0x00);

	return gain;
}				/*      set_gain  */

static void ihdr_write_shutter_gain(
				kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{

	kal_uint16 realtime_fps = 0;
	kal_uint16 reg_gain;

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
	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk
			/ imgsensor.line_length * 10 / imgsensor.frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			write_cmos_sensor(0x0104, 0x01);
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		      write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor(0x0104, 0x00);
		}
	} else {
		write_cmos_sensor(0x0104, 0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x0104, 0x00);
	}
	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor(0x0350, 0x01);	/* Enable auto extend */
	/* Long exposure */
	write_cmos_sensor(0x0202, (le >> 8) & 0xFF);
	write_cmos_sensor(0x0203, le & 0xFF);
	/* Short exposure */
	write_cmos_sensor(0x0224, (se >> 8) & 0xFF);
	write_cmos_sensor(0x0225, se & 0xFF);
	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	/* Global analog Gain for Long expo */
	write_cmos_sensor(0x0204, (reg_gain >> 8) & 0xFF);
	write_cmos_sensor(0x0205, reg_gain & 0xFF);
	/* Global analog Gain for Short expo */
	/* write_cmos_sensor(0x0216, (reg_gain>>8)& 0xFF); */
	/* write_cmos_sensor(0x0217, reg_gain & 0xFF); */
	write_cmos_sensor(0x0104, 0x00);
	pr_info("le:0x%x, se:0x%x, gain:0x%x, auto_extend=%d\n", le, se, gain,
		read_cmos_sensor(0x0350));
}
#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
	kal_uint8 iTemp;

	pr_info("image_mirror = %d\n", image_mirror);
	iTemp = read_cmos_sensor(0x0101);
	iTemp &= ~0x03;		/* Clear the mirror and flip bits. */

	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor(0x0101, iTemp | 0x00);
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor(0x0101, iTemp | 0x01);
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor(0x0101, iTemp | 0x02);
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor(0x0101, iTemp | 0x03);
		break;
	default:
		pr_info("Error image_mirror setting\n");
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
}				/*      night_mode      */
#if 0
	/*      preview_setting  */
static void imx486_ImageQuality_Setting(void)
{

}
#endif

static void sensor_init(void)
{
//From reg tool at 2018-1-6 16:54
write_cmos_sensor(0x0136, 0x18);
write_cmos_sensor(0x0137, 0x00);
write_cmos_sensor(0x4979, 0x0d);
write_cmos_sensor(0x3040, 0x03);
write_cmos_sensor(0x3041, 0x04);
write_cmos_sensor(0x4340, 0x01);
write_cmos_sensor(0x4341, 0x02);
write_cmos_sensor(0x4342, 0x04);
write_cmos_sensor(0x4344, 0x00);
write_cmos_sensor(0x4345, 0x2C);
write_cmos_sensor(0x4346, 0x00);
write_cmos_sensor(0x4347, 0x26);
write_cmos_sensor(0x434C, 0x00);
write_cmos_sensor(0x434D, 0x31);
write_cmos_sensor(0x434E, 0x00);
write_cmos_sensor(0x434F, 0x2C);
write_cmos_sensor(0x4350, 0x00);
write_cmos_sensor(0x4351, 0x0A);
write_cmos_sensor(0x4354, 0x00);
write_cmos_sensor(0x4355, 0x2A);
write_cmos_sensor(0x4356, 0x00);
write_cmos_sensor(0x4357, 0x1F);
write_cmos_sensor(0x435C, 0x00);
write_cmos_sensor(0x435D, 0x2F);
write_cmos_sensor(0x435E, 0x00);
write_cmos_sensor(0x435F, 0x2A);
write_cmos_sensor(0x4360, 0x00);
write_cmos_sensor(0x4361, 0x0A);
write_cmos_sensor(0x4600, 0x2A);
write_cmos_sensor(0x4601, 0x30);
write_cmos_sensor(0x4920, 0x0A);
write_cmos_sensor(0x4926, 0x01);
write_cmos_sensor(0x4929, 0x01);
write_cmos_sensor(0x4938, 0x01);
write_cmos_sensor(0x4959, 0x15);
write_cmos_sensor(0x4A38, 0x0A);
write_cmos_sensor(0x4A39, 0x05);
write_cmos_sensor(0x4A3C, 0x0F);
write_cmos_sensor(0x4A3D, 0x05);
write_cmos_sensor(0x4A3E, 0x05);
write_cmos_sensor(0x4A40, 0x0F);
write_cmos_sensor(0x4A4A, 0x01);
write_cmos_sensor(0x4A4B, 0x01);
write_cmos_sensor(0x4A4C, 0x01);
write_cmos_sensor(0x4A4D, 0x01);
write_cmos_sensor(0x4A4E, 0x01);
write_cmos_sensor(0x4A4F, 0x01);
write_cmos_sensor(0x4A50, 0x01);
write_cmos_sensor(0x4A51, 0x01);
write_cmos_sensor(0x4A76, 0x16);
write_cmos_sensor(0x4A77, 0x16);
write_cmos_sensor(0x4A78, 0x16);
write_cmos_sensor(0x4A79, 0x16);
write_cmos_sensor(0x4A7A, 0x16);
write_cmos_sensor(0x4A7B, 0x16);
write_cmos_sensor(0x4A7C, 0x16);
write_cmos_sensor(0x4A7D, 0x16);
write_cmos_sensor(0x555E, 0x01);
write_cmos_sensor(0x5563, 0x01);
write_cmos_sensor(0x70DA, 0x02);

}				/*      sensor_init  */

static void preview_setting(void)
{
	pr_info("preview E\n");

//From reg tool at 2018-1-6 17:01
//write_cmos_sensor(0x0100, 0x00);
write_cmos_sensor(0x0112, 0x0A);
write_cmos_sensor(0x0113, 0x0A);
write_cmos_sensor(0x0114, 0x03);
write_cmos_sensor(0x0342, 0x08);
write_cmos_sensor(0x0343, 0x30);
write_cmos_sensor(0x0340, 0x0A);
write_cmos_sensor(0x0341, 0x6E);
write_cmos_sensor(0x0344, 0x00);
write_cmos_sensor(0x0345, 0x00);
write_cmos_sensor(0x0346, 0x00);
write_cmos_sensor(0x0347, 0x00);
write_cmos_sensor(0x0348, 0x0F);
write_cmos_sensor(0x0349, 0xBF);
write_cmos_sensor(0x034A, 0x0B);
write_cmos_sensor(0x034B, 0xC7);
write_cmos_sensor(0x0220, 0x00);
write_cmos_sensor(0x0222, 0x01);
write_cmos_sensor(0x0900, 0x01);
write_cmos_sensor(0x0901, 0x22);
write_cmos_sensor(0x0902, 0x00);
write_cmos_sensor(0x3130, 0x01);
write_cmos_sensor(0x034C, 0x07);
write_cmos_sensor(0x034D, 0xE0);
write_cmos_sensor(0x034E, 0x05);
write_cmos_sensor(0x034F, 0xE4);
write_cmos_sensor(0x0301, 0x06);
write_cmos_sensor(0x0303, 0x02);
write_cmos_sensor(0x0305, 0x04);
write_cmos_sensor(0x0306, 0x00);
write_cmos_sensor(0x0307, 0x54);
write_cmos_sensor(0x030B, 0x01);
write_cmos_sensor(0x030D, 0x02);
write_cmos_sensor(0x030E, 0x00);
write_cmos_sensor(0x030F, 0x2A);
write_cmos_sensor(0x0310, 0x01);
write_cmos_sensor(0x0700, 0x01);
write_cmos_sensor(0x0701, 0x50);
write_cmos_sensor(0x0820, 0x07);
write_cmos_sensor(0x0821, 0xE0);
write_cmos_sensor(0x3100, 0x06);
write_cmos_sensor(0x7036, 0x02);
write_cmos_sensor(0x704E, 0x02);
write_cmos_sensor(0x0202, 0x0A);
write_cmos_sensor(0x0203, 0x64);
write_cmos_sensor(0x0224, 0x01);
write_cmos_sensor(0x0225, 0xF4);
write_cmos_sensor(0x0204, 0x00);
write_cmos_sensor(0x0205, 0x00);
write_cmos_sensor(0x020E, 0x01);
write_cmos_sensor(0x020F, 0x00);
write_cmos_sensor(0x0216, 0x00);
write_cmos_sensor(0x0217, 0x00);
write_cmos_sensor(0x0226, 0x01);
write_cmos_sensor(0x0227, 0x00);
//write_cmos_sensor(0x0100, 0x01);

}

static void capture_setting(kal_uint16 curretfps, kal_uint8 pdaf_mode)
{
	pr_info("capture E\n");
	pr_info("E! currefps:%d\n", curretfps);
	if (curretfps == 150) {
		pr_info("PIP15fps capture E\n");

//From reg tool at 2018-1-6 17:01
//write_cmos_sensor(0x0100, 0x00);
write_cmos_sensor(0x0112, 0x0A);
write_cmos_sensor(0x0113, 0x0A);
write_cmos_sensor(0x0114, 0x03);
write_cmos_sensor(0x0342, 0x10);
write_cmos_sensor(0x0343, 0x60);
write_cmos_sensor(0x0340, 0x0B);
write_cmos_sensor(0x0341, 0xFC);
write_cmos_sensor(0x0344, 0x00);
write_cmos_sensor(0x0345, 0x00);
write_cmos_sensor(0x0346, 0x00);
write_cmos_sensor(0x0347, 0x00);
write_cmos_sensor(0x0348, 0x0F);
write_cmos_sensor(0x0349, 0xBF);
write_cmos_sensor(0x034A, 0x0B);
write_cmos_sensor(0x034B, 0xC7);
write_cmos_sensor(0x0220, 0x00);
write_cmos_sensor(0x0222, 0x01);
write_cmos_sensor(0x0900, 0x00);
write_cmos_sensor(0x0901, 0x11);
write_cmos_sensor(0x0902, 0x00);
write_cmos_sensor(0x3130, 0x01);
write_cmos_sensor(0x034C, 0x0F);
write_cmos_sensor(0x034D, 0xC0);
write_cmos_sensor(0x034E, 0x0B);
write_cmos_sensor(0x034F, 0xC8);
write_cmos_sensor(0x0301, 0x06);
write_cmos_sensor(0x0303, 0x02);
write_cmos_sensor(0x0305, 0x04);
write_cmos_sensor(0x0306, 0x00);
write_cmos_sensor(0x0307, 0x61);
write_cmos_sensor(0x030B, 0x01);
write_cmos_sensor(0x030D, 0x02);
write_cmos_sensor(0x030E, 0x00);
write_cmos_sensor(0x030F, 0x2A);
write_cmos_sensor(0x0310, 0x01);
write_cmos_sensor(0x0700, 0x00);
write_cmos_sensor(0x0701, 0x98);
write_cmos_sensor(0x0820, 0x07);
write_cmos_sensor(0x0821, 0xE0);
write_cmos_sensor(0x3100, 0x06);
write_cmos_sensor(0x7036, 0x02);
write_cmos_sensor(0x704E, 0x02);
write_cmos_sensor(0x0202, 0x0B);
write_cmos_sensor(0x0203, 0xF2);
write_cmos_sensor(0x0224, 0x01);
write_cmos_sensor(0x0225, 0xF4);
write_cmos_sensor(0x0204, 0x00);
write_cmos_sensor(0x0205, 0x00);
//write_cmos_sensor(0x020E,0x01);
write_cmos_sensor(0x020F, 0x00);
write_cmos_sensor(0x0216, 0x00);
write_cmos_sensor(0x0217, 0x00);
write_cmos_sensor(0x0226, 0x01);
write_cmos_sensor(0x0227, 0x00);
//write_cmos_sensor(0x0100, 0x01);
	} else if (curretfps == 240) {
	pr_info("PIP24fps capture E\n");
//From reg tool at 2018-1-6 17:01
//write_cmos_sensor(0x0100, 0x00);
write_cmos_sensor(0x0112, 0x0A);
write_cmos_sensor(0x0113, 0x0A);
write_cmos_sensor(0x0114, 0x03);
write_cmos_sensor(0x0342, 0x10);
write_cmos_sensor(0x0343, 0x60);
write_cmos_sensor(0x0340, 0x0B);
write_cmos_sensor(0x0341, 0xFC);
write_cmos_sensor(0x0344, 0x00);
write_cmos_sensor(0x0345, 0x00);
write_cmos_sensor(0x0346, 0x00);
write_cmos_sensor(0x0347, 0x00);
write_cmos_sensor(0x0348, 0x0F);
write_cmos_sensor(0x0349, 0xBF);
write_cmos_sensor(0x034A, 0x0B);
write_cmos_sensor(0x034B, 0xC7);
write_cmos_sensor(0x0220, 0x00);
write_cmos_sensor(0x0222, 0x01);
write_cmos_sensor(0x0900, 0x00);
write_cmos_sensor(0x0901, 0x11);
write_cmos_sensor(0x0902, 0x00);
write_cmos_sensor(0x3130, 0x01);
write_cmos_sensor(0x034C, 0x0F);
write_cmos_sensor(0x034D, 0xC0);
write_cmos_sensor(0x034E, 0x0B);
write_cmos_sensor(0x034F, 0xC8);
write_cmos_sensor(0x0301, 0x06);
write_cmos_sensor(0x0303, 0x02);
write_cmos_sensor(0x0305, 0x04);
write_cmos_sensor(0x0306, 0x00);
write_cmos_sensor(0x0307, 0x9C);
write_cmos_sensor(0x030B, 0x01);
write_cmos_sensor(0x030D, 0x02);
write_cmos_sensor(0x030E, 0x00);
write_cmos_sensor(0x030F, 0x42);
write_cmos_sensor(0x0310, 0x01);
write_cmos_sensor(0x0700, 0x00);
write_cmos_sensor(0x0701, 0x40);
write_cmos_sensor(0x0820, 0x0C);
write_cmos_sensor(0x0821, 0x60);
write_cmos_sensor(0x3100, 0x06);
write_cmos_sensor(0x7036, 0x00);
write_cmos_sensor(0x704E, 0x02);
write_cmos_sensor(0x0202, 0x0B);
write_cmos_sensor(0x0203, 0xF2);
write_cmos_sensor(0x0224, 0x01);
write_cmos_sensor(0x0225, 0xF4);
write_cmos_sensor(0x0204, 0x00);
write_cmos_sensor(0x0205, 0x00);
//write_cmos_sensor(0x020E,0x01);
write_cmos_sensor(0x020F, 0x00);
write_cmos_sensor(0x0216, 0x00);
write_cmos_sensor(0x0217, 0x00);
write_cmos_sensor(0x0226, 0x01);
write_cmos_sensor(0x0227, 0x00);
//write_cmos_sensor(0x0100, 0x01);
	} else {
//From reg tool at 2018-1-6 17:01
//write_cmos_sensor(0x0100, 0x00);
write_cmos_sensor(0x0112, 0x0A);
write_cmos_sensor(0x0113, 0x0A);
write_cmos_sensor(0x0114, 0x03);
write_cmos_sensor(0x0342, 0x10);
write_cmos_sensor(0x0343, 0x60);
write_cmos_sensor(0x0340, 0x0B);
write_cmos_sensor(0x0341, 0xFC);
write_cmos_sensor(0x0344, 0x00);
write_cmos_sensor(0x0345, 0x00);
write_cmos_sensor(0x0346, 0x00);
write_cmos_sensor(0x0347, 0x00);
write_cmos_sensor(0x0348, 0x0F);
write_cmos_sensor(0x0349, 0xBF);
write_cmos_sensor(0x034A, 0x0B);
write_cmos_sensor(0x034B, 0xC7);
write_cmos_sensor(0x0220, 0x00);
write_cmos_sensor(0x0222, 0x01);
write_cmos_sensor(0x0900, 0x00);
write_cmos_sensor(0x0901, 0x11);
write_cmos_sensor(0x0902, 0x00);
write_cmos_sensor(0x3130, 0x01);
write_cmos_sensor(0x034C, 0x0F);
write_cmos_sensor(0x034D, 0xC0);
write_cmos_sensor(0x034E, 0x0B);
write_cmos_sensor(0x034F, 0xC8);
write_cmos_sensor(0x0301, 0x06);
write_cmos_sensor(0x0303, 0x02);
write_cmos_sensor(0x0305, 0x04);
write_cmos_sensor(0x0306, 0x00);
write_cmos_sensor(0x0307, 0xC1);
write_cmos_sensor(0x030B, 0x01);
write_cmos_sensor(0x030D, 0x02);
write_cmos_sensor(0x030E, 0x00);
write_cmos_sensor(0x030F, 0x52);
write_cmos_sensor(0x0310, 0x01);
write_cmos_sensor(0x0700, 0x00);
write_cmos_sensor(0x0701, 0x50);
write_cmos_sensor(0x0820, 0x0F);
write_cmos_sensor(0x0821, 0x60);
write_cmos_sensor(0x3100, 0x06);
write_cmos_sensor(0x7036, 0x00);
write_cmos_sensor(0x704E, 0x00);
write_cmos_sensor(0x0202, 0x0B);
write_cmos_sensor(0x0203, 0xF2);
write_cmos_sensor(0x0224, 0x01);
write_cmos_sensor(0x0225, 0xF4);
write_cmos_sensor(0x0204, 0x00);
write_cmos_sensor(0x0205, 0x00);
//write_cmos_sensor(0x020E,0x01);
write_cmos_sensor(0x020F, 0x00);
write_cmos_sensor(0x0216, 0x00);
write_cmos_sensor(0x0217, 0x00);
write_cmos_sensor(0x0226, 0x01);
write_cmos_sensor(0x0227, 0x00);
//write_cmos_sensor(0x0100, 0x01);

	}
}

static void normal_video_setting(kal_uint16 currefps, kal_uint8 pdaf_mode)
{
	pr_info("normal video E\n");
	pr_info("E! currefps:%d\n", currefps);
	capture_setting(currefps, pdaf_mode);
}

static void hs_video_setting(void)
{
	pr_info("hs_video E\n");
	preview_setting();
}

static void slim_video_setting(void)
{
	pr_info("slim video E\n");
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
/* #define SLT_DEVINFO_CMM */
#ifdef SLT_DEVINFO_CMM
#include  <linux/dev_info.h>
static struct devinfo_struct *s_DEVINFO_ccm;	/* suppose 10 max lcm device */
#endif
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry_total = 1;
	kal_uint8 retry_cnt = retry_total;
#ifdef SLT_DEVINFO_CMM
	s_DEVINFO_ccm = kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);
	s_DEVINFO_ccm->device_type = "CCM";

	/* can change if got module id */
	s_DEVINFO_ccm->device_module = "PC0FB0002B";

	s_DEVINFO_ccm->device_vendor = "Sunrise";
	s_DEVINFO_ccm->device_ic = "IMX486";
	s_DEVINFO_ccm->device_version = "HI";
	s_DEVINFO_ccm->device_info = "200W";
#endif
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				pr_info("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
#ifdef SLT_DEVINFO_CMM
				s_DEVINFO_ccm->device_used = DEVINFO_USED;
				devinfo_check_add_device(s_DEVINFO_ccm);
#endif
				return ERROR_NONE;
			}

		      pr_info("Read sensor id fail, write id: 0x%x, id: 0x%x\n",
				imgsensor.i2c_write_id, *sensor_id);

			retry_cnt--;
		} while (retry_cnt > 0);
		i++;
		retry_cnt = retry_total;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {

	/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
		*sensor_id = 0xFFFFFFFF;
#ifdef SLT_DEVINFO_CMM
		s_DEVINFO_ccm->device_used = DEVINFO_UNUSED;
		devinfo_check_add_device(s_DEVINFO_ccm);
#endif
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
	/* const kal_uint8 i2c_addr[] = {
	 * IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	 */
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;

	pr_info("IMX486,MIPI 4LANE\n");
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				pr_info("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
				break;
			}

		      pr_info("Read sensor id fail, write id: 0x%x, id: 0x%x\n",
				imgsensor.i2c_write_id, sensor_id);

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
	imgsensor.gain = 0xe000;	/* 0x100; */
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.hdr_mode = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}				/*      open  */



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
	pr_info("E\n");

	/*No Need to implement this function */

	return ERROR_NONE;
}				/*      close  */

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
	pr_info("E\n");

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
}				/*      preview   */

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
	pr_info("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
#if 0
	else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
#endif
	else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			pr_info(
	  "current_fps %d fps is not support, so use cap's setting: %d fps!\n",
		imgsensor.current_fps, imgsensor_info.cap.max_framerate / 10);

		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps, imgsensor.pdaf_mode);

	mdelay(100);
	return ERROR_NONE;
}				/* capture() */

static kal_uint32 normal_video(
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_info("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	/* imgsensor.current_fps = 300; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	pr_info("ihdr enable :%d\n", imgsensor.hdr_mode);
	normal_video_setting(imgsensor.current_fps, imgsensor.pdaf_mode);
	return ERROR_NONE;
}				/*      normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_info("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/* imgsensor.current_fps = 600; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();

	return ERROR_NONE;
}				/*      hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			     MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_info("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/* imgsensor.current_fps = 1200; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();

	return ERROR_NONE;
}				/*      slim_video       */



static kal_uint32 get_resolution(
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT(*sensor_resolution))
{
	pr_info("E\n");
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
/*add end*/

	return ERROR_NONE;
}				/*      get_resolution  */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_info("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* not use */
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* inverse with datasheet */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4;	/* not use */
	sensor_info->SensorResetActiveHigh = FALSE;	/* not use */
	sensor_info->SensorResetDelayCount = 5;	/* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;

	sensor_info->SettleDelayMode =
		imgsensor_info.mipi_settle_delay_mode;

	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;

	sensor_info->HighSpeedVideoDelayFrame =
		imgsensor_info.hs_video_delay_frame;

	sensor_info->SlimVideoDelayFrame =
		imgsensor_info.slim_video_delay_frame;


	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;

	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;

	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;

	sensor_info->FrameTimeDelayFrame =
		imgsensor_info.frame_time_delay_frame;

	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->TEMPERATURE_SUPPORT = 0;

	//sensor_info->PDAF_Support = PDAF_SUPPORT_RAW;
	sensor_info->PDAF_Support = PDAF_SUPPORT_NA;

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
}				/*      get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_info("scenario_id = %d\n", scenario_id);
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
		pr_info("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}				/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	pr_info("framerate = %d\n ", framerate);
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
	pr_info("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)		/* enable auto flicker */
		imgsensor.autoflicker_en = KAL_TRUE;
	else			/* Cancel Auto flick */
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	pr_info("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk
			/ framerate * 10 / imgsensor_info.pre.linelength;

		spin_lock(&imgsensor_drv_lock);

		imgsensor.dummy_line =
		  (frame_length > imgsensor_info.pre.framelength)
		? (frame_length - imgsensor_info.pre.framelength) : 0;

		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;

		frame_length = imgsensor_info.normal_video.pclk
		    / framerate * 10 / imgsensor_info.normal_video.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		  (frame_length > imgsensor_info.normal_video.framelength)
		? (frame_length - imgsensor_info.normal_video.framelength) : 0;

		imgsensor.frame_length =
		 imgsensor_info.normal_video.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		frame_length = imgsensor_info.cap1.pclk
			/ framerate * 10 / imgsensor_info.cap1.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		  (frame_length > imgsensor_info.cap1.framelength)
		? (frame_length - imgsensor_info.cap1.framelength) : 0;

		imgsensor.frame_length =
		    imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			pr_info(
			    "current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			    framerate,
			    imgsensor_info.cap.max_framerate / 10);

		frame_length = imgsensor_info.cap.pclk /
			framerate * 10 / imgsensor_info.cap.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		  (frame_length > imgsensor_info.cap.framelength)
		? (frame_length - imgsensor_info.cap.framelength) : 0;

		imgsensor.frame_length =
		    imgsensor_info.cap.framelength + imgsensor.dummy_line;

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
		    (frame_length > imgsensor_info.hs_video.framelength)
		    ? (frame_length - imgsensor_info.hs_video.framelength) : 0;

		imgsensor.frame_length =
		    imgsensor_info.hs_video.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk
			/ framerate * 10 / imgsensor_info.slim_video.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		  (frame_length > imgsensor_info.slim_video.framelength)
		? (frame_length - imgsensor_info.slim_video.framelength) : 0;

		imgsensor.frame_length =
		  imgsensor_info.slim_video.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	default:		/* coding with  preview scenario by default */
		frame_length = imgsensor_info.pre.pclk
		    / framerate * 10 / imgsensor_info.pre.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		  (frame_length > imgsensor_info.pre.framelength)
		? (frame_length - imgsensor_info.pre.framelength) : 0;

		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		pr_info("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	pr_info("scenario_id = %d\n", scenario_id);

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
	pr_info("set_test_pattern_mode enable: %d\n", enable);

	if (enable)
		write_cmos_sensor(0x0601, 0x02);
	else
		write_cmos_sensor(0x0601, 0x00);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 get_sensor_temperature(void)
{
	UINT8 temperature;
	INT32 temperature_convert;

	/*TEMP_SEN_CTL */
	write_cmos_sensor(0x0138, 0x01);
	temperature = read_cmos_sensor(0x013a);

	if (temperature >= 0x0 && temperature <= 0x4F)
		temperature_convert = temperature;
	else if (temperature >= 0x50 && temperature <= 0x7F)
		temperature_convert = 80;
	else if (temperature >= 0x80 && temperature <= 0xEC)
		temperature_convert = -20;
	else
		temperature_convert = (INT8) temperature;

	pr_info("temp_c(%d), read_reg(%d)\n",
		temperature_convert, temperature);

	return temperature_convert;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	pr_info("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(0x0100, 0X01);
	else
		write_cmos_sensor(0x0100, 0x00);
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
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;

	/* unsigned long long *feature_return_para =
	 * (unsigned long long *) feature_para;
	 */

	struct SET_PD_BLOCK_INFO_T *PDAFinfo;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	//struct SENSOR_VC_INFO_STRUCT *pvcinfo;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/* pr_info("feature_id = %d\n", feature_id); */

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
		set_shutter((UINT16) *feature_data);
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
		write_cmos_sensor(
			sensor_reg_data->RegAddr, sensor_reg_data->RegData);
		break;

	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(sensor_reg_data->RegAddr);
		break;

	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM
		 * or just return LENS_DRIVER_ID_DO_NOT_CARE
		 */
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
		set_auto_flicker_mode(
			(BOOL) * feature_data_16, *(feature_data_16 + 1));
		break;

	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM) *feature_data,
					      *(feature_data + 1));
		break;

	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM) *feature_data,
			  (MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;

	case SENSOR_FEATURE_GET_PDAF_DATA:
		pr_info("SENSOR_FEATURE_GET_PDAF_DATA\n");
		#if 0
		read_imx486_pdaf((kal_uint16) (*feature_data),
				 (char *)(uintptr_t) (*(feature_data + 1)),
				 (kal_uint32) (*(feature_data + 2)));
		#endif
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		pr_info("SENSOR_FEATURE_SET_TEST_PATTERN\n");
		set_test_pattern_mode((BOOL) * feature_data);
		break;

	/* for factory mode auto testing */
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;

	case SENSOR_FEATURE_SET_FRAMERATE:
		pr_info("current fps :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = (UINT16)*feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		/* HDR mODE : 0: disable HDR, 1:IHDR, 2:HDR, 9:ZHDR */
		pr_info("ihdr enable :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.hdr_mode = (UINT8)*feature_data_32;
		pr_info("ihdr enable :%d\n", imgsensor.hdr_mode);
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		pr_info("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			(UINT32) *feature_data);
		wininfo =
	(struct SENSOR_WINSIZE_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[3],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[4],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy(
				(void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
#if 1
	case SENSOR_FEATURE_GET_PDAF_INFO:
		pr_info("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%llu\n",
			*feature_data);

		PDAFinfo =
	      (struct SET_PD_BLOCK_INFO_T *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		#if 0
			if (imx486_type != IMX486_BINNING_TYPE)
				memcpy(
				(void *)PDAFinfo,
				(void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			else
				memcpy(
				(void *)PDAFinfo,
				(void *)&imgsensor_pd_info_Binning,
				sizeof(struct SET_PD_BLOCK_INFO_T));
		#endif
			break;

		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			break;
		}
		break;
#endif
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		pr_info("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16) *feature_data,
			(UINT16) *(feature_data + 1),
			(UINT16) *(feature_data + 2));

		ihdr_write_shutter_gain((UINT16) *feature_data,
					(UINT16) *(feature_data + 1),
					(UINT16) *(feature_data + 2));
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		break;
		/*PDAF CMD */
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		pr_info(
		    "SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n",
			*feature_data);

	/* PDAF capacity enable or not, 2p8 only full size support PDAF */
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_SET_PDAF:
		pr_info("PDAF mode :%d\n", *feature_data_16);
		imgsensor.pdaf_mode = *feature_data_16;
		break;

	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length(
			(UINT16) *feature_data, (UINT16) *(feature_data + 1));
		break;

	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = get_sensor_temperature();
		*feature_para_len = 4;
		break;

	case SENSOR_FEATURE_GET_PDAF_TYPE:
		break;
	case SENSOR_FEATURE_SET_PDAF_TYPE:
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		pr_info("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;

	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		pr_info("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	default:
		break;
	}

	return ERROR_NONE;
}				/*      feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 IMX486_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}				/*      IMX486_MIPI_RAW_SensorInit      */
