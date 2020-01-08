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

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>
#include <mt-plat/mtk_boot.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "blackjack_hlt_ov16a10mipiraw_Sensor.h"

#define PFX "ov16a10mipiraw_Sensor"

//Gionee <GN_BSP_CAMERA> <xuxiaojiang> <20170826> add for CR179907 ov16a10 capture size and setting change from 16m to 4m begin
//static  kal_uint16 ROTATION=01; //00 for normal, 01 for ratation 180degree.
//Gionee <GN_BSP_CAMERA> <xuxiaojiang> <20170826> add for CR179907 ov16a10 capture size and setting change from 16m to 4m end
/****************************   Modify end    *******************************************/

#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __func__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = BLACKJACK_HLT_OV16A10_SENSOR_ID,

	.checksum_value = 0x4ccd0a9,	/* checksum value for Camera Auto Test */

	.pre = {                    //check
		.pclk = 100000000,
		.linelength = 1700,	
		.framelength = 1960,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2308,
		.grabwindow_height = 1728,
		.mipi_data_lp2hs_settle_dc = 65,
		.max_framerate = 300,
		.mipi_pixel_rate = 302400000,
	},
	.cap = {		/* 95:line 5312, 52/35:line 5336 */
		.pclk = 100000000,
		.linelength = 850,
		.framelength = 3920,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4608,
		.grabwindow_height = 3456,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
		.mipi_pixel_rate = 628800000,
	},
	.cap1 = {
		.pclk = 100000000,
		.linelength = 850,
		.framelength = 3920,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4608,
		.grabwindow_height = 3456,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
		.mipi_pixel_rate = 628800000,
	},
	.normal_video = {
		.pclk = 100000000,
		.linelength = 1700,
		.framelength = 1960,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2304,
		.grabwindow_height = 1728,
		.mipi_data_lp2hs_settle_dc = 65,
		.max_framerate = 300,
		.mipi_pixel_rate = 302400000,

	},
	.hs_video = {
		.pclk = 100000000,
		.linelength = 1700,
		.framelength = 1960,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2328,
		.grabwindow_height = 1748,
		.mipi_data_lp2hs_settle_dc = 65,
		.max_framerate = 300,
		.mipi_pixel_rate = 30240000,
	},
	.slim_video = {
		.pclk = 100000000,
		.linelength = 1700,
		.framelength = 1960,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2308,
		.grabwindow_height = 1728,
		.mipi_data_lp2hs_settle_dc = 65,
		.max_framerate = 300,
		.mipi_pixel_rate = 302400000,
	},
	.custom1 = {
		.pclk = 100000000,
		.linelength = 1100,
		.framelength = 3788,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4608,
		.grabwindow_height = 3456,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 240,
		.mipi_pixel_rate = 480000000,
	},
	.custom2 = {
		.pclk = 100000000,
		.linelength = 1100,
		.framelength = 3788,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4608,
		.grabwindow_height = 3456,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 240,
		.mipi_pixel_rate = 480000000,
	},
	.margin = 8,		/* sensor framelength & shutter margin */
	.min_shutter = 8,	/* min shutter */
	.max_frame_length = 0x7ffe,	/* max framelength by sensor register's limitation */
	.ae_shut_delay_frame = 0,	/* shutter delay frame for AE cycle,
					 * 2 frame with ispGain_delay-shut_delay=2-0=2
					 */
	.ae_sensor_gain_delay_frame = 0,	/* sensor gain delay frame for AE cycle,
						 * 2 frame with ispGain_delay-sensor_gain_delay=2-0=2
						 */
	.ae_ispGain_delay_frame = 2,	/* isp gain delay frame for AE cycle */
	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.sensor_mode_num = 8,	/* support sensor mode num ,don't support Slow motion */

	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */
	.hs_video_delay_frame = 2,	/* enter high speed video  delay frame num */
	.slim_video_delay_frame = 2,	/* enter slim video delay frame num */
	.custom1_delay_frame = 1,
    .custom2_delay_frame = 1,
	.isp_driving_current = ISP_DRIVING_8MA,	/* mclk driving current */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,	/* sensor_interface_type */
	.mipi_sensor_type = MIPI_OPHY_CSI2,	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_settle_delay_mode = 1,	/* 0,MIPI_SETTLEDELAY_AUTO;
					 * 1,MIPI_SETTLEDELAY_MANNUAL
					 */
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,//SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,//sensor output first pixel color
	.mclk = 24,		/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_4_LANE,	/* mipi lane num */
	.i2c_addr_table = {0x6C, 0xff},	/* record sensor support all write id addr,
							 * only supprt 4must end with 0xff*/
};


static struct imgsensor_struct imgsensor = {
//Gionee:malp 20150514 modify for mirrorflip start
#ifdef ORIGINAL_VERSION
	.mirror = IMAGE_NORMAL, 			//mirrorflip information
#else 
	.mirror = IMAGE_HV_MIRROR,			   //mirrorflip information
#endif
//Gionee:malp 20150514 modify for mirrorflip end

	.sensor_mode = IMGSENSOR_MODE_INIT,	/* IMGSENSOR_MODE enum value,
						 * record current sensor mode,such as:
						 * INIT, Preview, Capture, Video,High Speed Video, Slim Video
						 */
	.shutter = 0x4C00,	/* current shutter */
	.gain = 0x200,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	.current_fps = 30,	/* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.autoflicker_en = KAL_FALSE,	/* auto flicker enable:
					 * KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
					 */
	.test_pattern = KAL_FALSE,	/* test pattern mode or not.
					 * KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
					 */
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,	/* current scenario id */
	.ihdr_en = 0,		/* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x20,	/* record current sensor's i2c write id */
};

/* Sensor output window information*/
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[7] = {
	{4656, 3496,    0,    0,   4656, 3496, 2328, 1748,  12,  10,  2304, 1728, 0, 0, 2304, 1728,}, /* Preview check*/
	{4656, 3496,    0,    0,   4656, 3496, 4656, 3496,  24,  20,  4608, 3456, 0, 0, 4608, 3456,},/* capture */
	{4656, 3496,    0,    0,   4656, 3496, 2328, 1748,  12,  10,  2304, 1728, 0, 0, 2304, 1728,}, /* video */
	{4656, 3496,    0,    0,   4656, 3496, 2328, 1748,  12,  10,  2304, 1728, 0, 0, 2304, 1728,}, /* hs vedio */
	{4656, 3496,    0,    0,   4656, 3496, 2328, 1748,  12,  10,  2304, 1728, 0, 0, 2304, 1728,},/* slim vedio */
	{4656, 3496,    0,    0,   4656, 3496, 4656, 3496,  24,  20,  4608, 3456, 0, 0, 4608, 3456,},//custom1
	{4656, 3496,    0,    0,   4656, 3496, 4656, 3496,  24,  20,  4608, 3456, 0, 0, 4608, 3456,},//custom2
};

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {
    /* Preview mode setting 496(pxiel)*1496 NOT USED*/
    {
        0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
        0x00, 0x2b, 0x0FA0, 0x0BB8, 0x00, 0x00, 0x0280, 0x0001,
        0x01, 0x2b, 0x026c, 0x05d8, 0x03, 0x00, 0x0000, 0x0000
    },
    /* Capture mode setting  288(Pixel)*1728*/
    {
        0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
        0x00, 0x2b, 0x1F40, 0x1770, 0x00, 0x00, 0x0280, 0x0001,
        0x01, 0x2b, 0x0168, 0x06c0, 0x03, 0x00, 0x0000, 0x0000
    },
    /* Video mode setting 496(pxiel)*1496 NOT USED*/
    {
        0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
        0x00, 0x2b, 0x0FA0, 0x0BB8, 0x00, 0x00, 0x0280, 0x0001,
        0x01, 0x2b, 0x026c, 0x05d8, 0x03, 0x00, 0x0000, 0x0000
    },
};

/*PDAF START*/
#if 1
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX =  0,
	.i4OffsetY =  0,
	.i4PitchX  = 32,
	.i4PitchY  = 32,
	.i4PairNum =  16,
	.i4SubBlkW =  8,
	.i4SubBlkH =  8,
	.i4BlockNumX =144,
	.i4BlockNumY =108,
	.i4PosL = {{7,6}, {15,6}, {23,6}, {31,6}, {3,14}, {11,14}, {19,14},{27,14},{7,22}, {15,22}, {23,22}, {31,22}, {3,30}, {11,30}, {19,30},{27,30}},
	.i4PosR = {{6,6}, {14,6}, {22,6}, {30,6}, {2,14}, {10,14}, {18,14},{26,14},{6,22}, {14,22}, {22,22}, {30,22}, {2,30}, {10,30}, {18,30},{26,30}},
	.iMirrorFlip = 0,
};
#endif

typedef struct SET_PD_BLOCK_INFO_T SET_PD_BLOCK_INFO_T;
static SET_PD_BLOCK_INFO_T *PDAFinfo;
/*PDAF END*/

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *) &get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

/*#ifdef CONFIG_HQ_HARDWARE_INFO
static kal_uint16 read_eeprom_module(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *) &get_byte, 1, EEPROM_ADDR);

	return get_byte;
}
#endif*/

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pusendcmd[4] = { (char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF) };

	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	/* check */
	/* LOG_INF("dummyline = %d, dummypixels = %d\n", imgsensor.dummy_line, imgsensor.dummy_pixel); */

	write_cmos_sensor(0x380c, imgsensor.line_length >> 8);
	write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);
	write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
}


static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;
	/* unsigned long flags; */

	LOG_INF("framerate = %d, min framelength should enable? %d\n", framerate,
		min_framelength_en);

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
}				/*      set_max_framerate  */

#if 0
static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;

	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	/* OV Recommend Solution */
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
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
	    ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	shutter = (shutter >> 1) << 1;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;
			write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
			write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		}
	} else {
		/* Extend frame length */
		imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;
		write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	}

	/* Update Shutter */
	//write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);
	write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
	write_cmos_sensor(0x3502, (shutter & 0xFF);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

	/* LOG_INF("frame_length = %d ", frame_length); */

}				/*      write_shutter  */
#endif

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
	kal_uint16 realtime_fps = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	/* write_shutter(shutter); */
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length, should extend frame length first */
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
	    ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
	shutter = (shutter >> 1) << 1;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
			write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	}

	/* Update Shutter */
	//write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);
	write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
	write_cmos_sensor(0x3502, (shutter & 0xFF));
	LOG_INF("Exit! shutter =%d, framelength =%d, for flicker realtime_fps=%d\n", shutter,
		imgsensor.frame_length, realtime_fps);

}

static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	/*Change frame time*/
	dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	//
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
		 write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		 write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		}
	} else {
		// Extend frame length
		 write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		 write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	}

	// Update Shutter
	//write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);
	write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
	write_cmos_sensor(0x3502, (shutter & 0xFF));

	LOG_INF("Add for N3D! shutterlzl =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 iReg = 0x0000;

	/* platform 1xgain = 64, sensor driver 1*gain = 0x80 */
	iReg = gain * 256 / BASEGAIN;

	if (iReg < 0x100)	/* sensor 1xGain */
		iReg = 0X100;

	if (iReg > 0xf80)	/* sensor 15.5xGain */
		iReg = 0Xf80;

	return iReg;
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

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x3508, (reg_gain >> 8));
	write_cmos_sensor(0x3509, (reg_gain & 0xFF));
	//write_cmos_sensor(0x350c, (reg_gain >> 8));
	//write_cmos_sensor(0x350d, (reg_gain & 0xFF));
	return gain;
}				/*      set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n", le, se, gain);
}


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
	/*No Need to implement this function */
}				/*      night_mode      */

static void sensor_init(void)
{
	LOG_INF("E\n");

	write_cmos_sensor(0x0103, 0x01);
	mdelay(10);
	write_cmos_sensor(0x0102, 0x00);
	write_cmos_sensor(0x0301, 0x48);
	write_cmos_sensor(0x0302, 0x31);
	write_cmos_sensor(0x0303, 0x04);
	write_cmos_sensor(0x0305, 0xc2);
	write_cmos_sensor(0x0306, 0x00);
	write_cmos_sensor(0x0320, 0x02);
	write_cmos_sensor(0x0323, 0x04);
	write_cmos_sensor(0x0326, 0xd8);
	write_cmos_sensor(0x0327, 0x0b);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0343, 0x04);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x0346, 0xc0);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x300e, 0x22);
	write_cmos_sensor(0x3012, 0x41);
	write_cmos_sensor(0x3016, 0xd2);
	write_cmos_sensor(0x3018, 0x70);
	write_cmos_sensor(0x301e, 0x98);
	write_cmos_sensor(0x3025, 0x03);
	write_cmos_sensor(0x3026, 0x10);
	write_cmos_sensor(0x3027, 0x08);
	write_cmos_sensor(0x3102, 0x00);
	write_cmos_sensor(0x3400, 0x04);
	write_cmos_sensor(0x3406, 0x04);
	write_cmos_sensor(0x3408, 0x04);
	write_cmos_sensor(0x3421, 0x09);
	write_cmos_sensor(0x3422, 0x20);
	write_cmos_sensor(0x3423, 0x15);
	write_cmos_sensor(0x3424, 0x40);
	write_cmos_sensor(0x3425, 0x14);
	write_cmos_sensor(0x3426, 0x04);
	write_cmos_sensor(0x3504, 0x08);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);
	write_cmos_sensor(0x350a, 0x01);
	write_cmos_sensor(0x350b, 0x00);
	write_cmos_sensor(0x350c, 0x00);
	write_cmos_sensor(0x3548, 0x01);
	write_cmos_sensor(0x3549, 0x00);
	write_cmos_sensor(0x354a, 0x01);
	write_cmos_sensor(0x354b, 0x00);
	write_cmos_sensor(0x354c, 0x00);
	write_cmos_sensor(0x3600, 0xff);
	write_cmos_sensor(0x3602, 0x42);
	write_cmos_sensor(0x3603, 0x7b);
	write_cmos_sensor(0x3608, 0x9b);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x360b, 0x53);
	write_cmos_sensor(0x3618, 0xc0);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x361d, 0x20);
	write_cmos_sensor(0x361e, 0x10);
	write_cmos_sensor(0x361f, 0x01);
	write_cmos_sensor(0x3620, 0x89);
	write_cmos_sensor(0x3624, 0x8f);
	write_cmos_sensor(0x3629, 0x09);
	write_cmos_sensor(0x362e, 0x50);
	write_cmos_sensor(0x3631, 0xe2);
	write_cmos_sensor(0x3632, 0xe2);
	write_cmos_sensor(0x3634, 0x10);
	write_cmos_sensor(0x3635, 0x10);
	write_cmos_sensor(0x3636, 0x10);
	write_cmos_sensor(0x3639, 0xa6);
	write_cmos_sensor(0x363a, 0xaa);
	write_cmos_sensor(0x363b, 0x0c);
	write_cmos_sensor(0x363c, 0x16);
	write_cmos_sensor(0x363d, 0x29);
	write_cmos_sensor(0x363e, 0x4f);
	write_cmos_sensor(0x3642, 0xa8);
	write_cmos_sensor(0x3652, 0x00);
	write_cmos_sensor(0x3653, 0x00);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x3657, 0x8e);
	write_cmos_sensor(0x3660, 0x80);
	write_cmos_sensor(0x3663, 0x00);
	write_cmos_sensor(0x3664, 0x00);
	write_cmos_sensor(0x3668, 0x05);
	write_cmos_sensor(0x3669, 0x05);
	write_cmos_sensor(0x370d, 0x10);
	write_cmos_sensor(0x370e, 0x05);
	write_cmos_sensor(0x370f, 0x10);
	write_cmos_sensor(0x3711, 0x01);
	write_cmos_sensor(0x3712, 0x09);
	write_cmos_sensor(0x3713, 0x40);
	write_cmos_sensor(0x3714, 0xe4);
	write_cmos_sensor(0x3716, 0x04);
	write_cmos_sensor(0x3717, 0x01);
	write_cmos_sensor(0x3718, 0x02);
	write_cmos_sensor(0x3719, 0x01);
	write_cmos_sensor(0x371a, 0x02);
	write_cmos_sensor(0x371b, 0x02);
	write_cmos_sensor(0x371c, 0x01);
	write_cmos_sensor(0x371d, 0x02);
	write_cmos_sensor(0x371e, 0x12);
	write_cmos_sensor(0x371f, 0x02);
	write_cmos_sensor(0x3720, 0x14);
	write_cmos_sensor(0x3721, 0x12);
	write_cmos_sensor(0x3722, 0x44);
	write_cmos_sensor(0x3723, 0x60);
	write_cmos_sensor(0x372f, 0x34);
	write_cmos_sensor(0x3726, 0x21);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x08);
	write_cmos_sensor(0x3808, 0x12);
	write_cmos_sensor(0x3809, 0x30);
	write_cmos_sensor(0x380a, 0x0d);
	write_cmos_sensor(0x380b, 0xa8);
	write_cmos_sensor(0x380c, 0x03);
	write_cmos_sensor(0x380d, 0x52);
	write_cmos_sensor(0x380e, 0x0f);
	write_cmos_sensor(0x380f, 0x50);
	write_cmos_sensor(0x3814, 0x11);
	write_cmos_sensor(0x3815, 0x11);
	write_cmos_sensor(0x3820, 0x00);
	write_cmos_sensor(0x3821, 0x06);
	write_cmos_sensor(0x3822, 0x00);
	write_cmos_sensor(0x3823, 0x00);
	write_cmos_sensor(0x3837, 0x10);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x383d, 0xff);
	write_cmos_sensor(0x383e, 0x0d);
	write_cmos_sensor(0x383f, 0x33);
	write_cmos_sensor(0x3857, 0x01);//for mirror[on]& flip[on]
	write_cmos_sensor(0x388f, 0x00);
	write_cmos_sensor(0x3890, 0x00);
	write_cmos_sensor(0x3891, 0x00);
	write_cmos_sensor(0x3d81, 0x10);
	write_cmos_sensor(0x3d83, 0x0c);
	write_cmos_sensor(0x3d84, 0x00);
	write_cmos_sensor(0x3d85, 0x1b);
	write_cmos_sensor(0x3d88, 0x00);
	write_cmos_sensor(0x3d89, 0x00);
	write_cmos_sensor(0x3d8a, 0x00);
	write_cmos_sensor(0x3d8b, 0x01);
	write_cmos_sensor(0x3d8c, 0x77);
	write_cmos_sensor(0x3d8d, 0xa0);
	write_cmos_sensor(0x3f00, 0x02);
	write_cmos_sensor(0x3f0c, 0x07);
	write_cmos_sensor(0x3f0d, 0x2f);
	write_cmos_sensor(0x4012, 0x0d);
	write_cmos_sensor(0x4015, 0x04);
	write_cmos_sensor(0x4016, 0x1b);
	write_cmos_sensor(0x4017, 0x04);
	write_cmos_sensor(0x4018, 0x0b);
	write_cmos_sensor(0x401b, 0x10);
	write_cmos_sensor(0x401e, 0x01);
	write_cmos_sensor(0x401f, 0x38);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x6a);
	write_cmos_sensor(0x4502, 0xb4);
	write_cmos_sensor(0x4586, 0x00);
	write_cmos_sensor(0x4588, 0x02);
	write_cmos_sensor(0x4640, 0x00);
	write_cmos_sensor(0x4641, 0x24);
	write_cmos_sensor(0x4643, 0x08);
	write_cmos_sensor(0x4645, 0x04);
	write_cmos_sensor(0x4806, 0x40);
	write_cmos_sensor(0x480e, 0x00);
	write_cmos_sensor(0x4815, 0x2b);
	write_cmos_sensor(0x481b, 0x3c);
	write_cmos_sensor(0x4833, 0x18);
	write_cmos_sensor(0x4837, 0x08);
	write_cmos_sensor(0x484b, 0x07);
	write_cmos_sensor(0x4850, 0x41);
	write_cmos_sensor(0x4860, 0x00);
	write_cmos_sensor(0x4861, 0xec);
	write_cmos_sensor(0x4864, 0x00);
	write_cmos_sensor(0x4883, 0x00);
	write_cmos_sensor(0x4888, 0x10);
	write_cmos_sensor(0x4a00, 0x10);
	write_cmos_sensor(0x4e00, 0x00);
	write_cmos_sensor(0x4e01, 0x04);
	write_cmos_sensor(0x4e02, 0x01);
	write_cmos_sensor(0x4e03, 0x00);
	write_cmos_sensor(0x4e04, 0x08);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e06, 0x00);
	write_cmos_sensor(0x4e07, 0x13);
	write_cmos_sensor(0x4e08, 0x01);
	write_cmos_sensor(0x4e09, 0x00);
	write_cmos_sensor(0x4e0a, 0x15);
	write_cmos_sensor(0x4e0b, 0x0e);
	write_cmos_sensor(0x4e0c, 0x00);
	write_cmos_sensor(0x4e0d, 0x17);
	write_cmos_sensor(0x4e0e, 0x07);
	write_cmos_sensor(0x4e0f, 0x00);
	write_cmos_sensor(0x4e10, 0x19);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e12, 0x00);
	write_cmos_sensor(0x4e13, 0x1b);
	write_cmos_sensor(0x4e14, 0x08);
	write_cmos_sensor(0x4e15, 0x00);
	write_cmos_sensor(0x4e16, 0x1f);
	write_cmos_sensor(0x4e17, 0x08);
	write_cmos_sensor(0x4e18, 0x00);
	write_cmos_sensor(0x4e19, 0x21);
	write_cmos_sensor(0x4e1a, 0x0e);
	write_cmos_sensor(0x4e1b, 0x00);
	write_cmos_sensor(0x4e1c, 0x2d);
	write_cmos_sensor(0x4e1d, 0x30);
	write_cmos_sensor(0x4e1e, 0x00);
	write_cmos_sensor(0x4e1f, 0x6a);
	write_cmos_sensor(0x4e20, 0x05);
	write_cmos_sensor(0x4e21, 0x00);
	write_cmos_sensor(0x4e22, 0x6c);
	write_cmos_sensor(0x4e23, 0x05);
	write_cmos_sensor(0x4e24, 0x00);
	write_cmos_sensor(0x4e25, 0x6e);
	write_cmos_sensor(0x4e26, 0x39);
	write_cmos_sensor(0x4e27, 0x00);
	write_cmos_sensor(0x4e28, 0x7a);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x4e2a, 0x00);
	write_cmos_sensor(0x4e2b, 0x00);
	write_cmos_sensor(0x4e2c, 0x00);
	write_cmos_sensor(0x4e2d, 0x00);
	write_cmos_sensor(0x4e2e, 0x00);
	write_cmos_sensor(0x4e2f, 0x00);
	write_cmos_sensor(0x4e30, 0x00);
	write_cmos_sensor(0x4e31, 0x00);
	write_cmos_sensor(0x4e32, 0x00);
	write_cmos_sensor(0x4e33, 0x00);
	write_cmos_sensor(0x4e34, 0x00);
	write_cmos_sensor(0x4e35, 0x00);
	write_cmos_sensor(0x4e36, 0x00);
	write_cmos_sensor(0x4e37, 0x00);
	write_cmos_sensor(0x4e38, 0x00);
	write_cmos_sensor(0x4e39, 0x00);
	write_cmos_sensor(0x4e3a, 0x00);
	write_cmos_sensor(0x4e3b, 0x00);
	write_cmos_sensor(0x4e3c, 0x00);
	write_cmos_sensor(0x4e3d, 0x00);
	write_cmos_sensor(0x4e3e, 0x00);
	write_cmos_sensor(0x4e3f, 0x00);
	write_cmos_sensor(0x4e40, 0x00);
	write_cmos_sensor(0x4e41, 0x00);
	write_cmos_sensor(0x4e42, 0x00);
	write_cmos_sensor(0x4e43, 0x00);
	write_cmos_sensor(0x4e44, 0x00);
	write_cmos_sensor(0x4e45, 0x00);
	write_cmos_sensor(0x4e46, 0x00);
	write_cmos_sensor(0x4e47, 0x00);
	write_cmos_sensor(0x4e48, 0x00);
	write_cmos_sensor(0x4e49, 0x00);
	write_cmos_sensor(0x4e4a, 0x00);
	write_cmos_sensor(0x4e4b, 0x00);
	write_cmos_sensor(0x4e4c, 0x00);
	write_cmos_sensor(0x4e4d, 0x00);
	write_cmos_sensor(0x4e4e, 0x00);
	write_cmos_sensor(0x4e4f, 0x00);
	write_cmos_sensor(0x4e50, 0x00);
	write_cmos_sensor(0x4e51, 0x00);
	write_cmos_sensor(0x4e52, 0x00);
	write_cmos_sensor(0x4e53, 0x00);
	write_cmos_sensor(0x4e54, 0x00);
	write_cmos_sensor(0x4e55, 0x00);
	write_cmos_sensor(0x4e56, 0x00);
	write_cmos_sensor(0x4e57, 0x00);
	write_cmos_sensor(0x4e58, 0x00);
	write_cmos_sensor(0x4e59, 0x00);
	write_cmos_sensor(0x4e5a, 0x00);
	write_cmos_sensor(0x4e5b, 0x00);
	write_cmos_sensor(0x4e5c, 0x00);
	write_cmos_sensor(0x4e5d, 0x00);
	write_cmos_sensor(0x4e5e, 0x00);
	write_cmos_sensor(0x4e5f, 0x00);
	write_cmos_sensor(0x4e60, 0x00);
	write_cmos_sensor(0x4e61, 0x00);
	write_cmos_sensor(0x4e62, 0x00);
	write_cmos_sensor(0x4e63, 0x00);
	write_cmos_sensor(0x4e64, 0x00);
	write_cmos_sensor(0x4e65, 0x00);
	write_cmos_sensor(0x4e66, 0x00);
	write_cmos_sensor(0x4e67, 0x00);
	write_cmos_sensor(0x4e68, 0x00);
	write_cmos_sensor(0x4e69, 0x00);
	write_cmos_sensor(0x4e6a, 0x00);
	write_cmos_sensor(0x4e6b, 0x00);
	write_cmos_sensor(0x4e6c, 0x00);
	write_cmos_sensor(0x4e6d, 0x00);
	write_cmos_sensor(0x4e6e, 0x00);
	write_cmos_sensor(0x4e6f, 0x00);
	write_cmos_sensor(0x4e70, 0x00);
	write_cmos_sensor(0x4e71, 0x00);
	write_cmos_sensor(0x4e72, 0x00);
	write_cmos_sensor(0x4e73, 0x00);
	write_cmos_sensor(0x4e74, 0x00);
	write_cmos_sensor(0x4e75, 0x00);
	write_cmos_sensor(0x4e76, 0x00);
	write_cmos_sensor(0x4e77, 0x00);
	write_cmos_sensor(0x4e78, 0x1c);
	write_cmos_sensor(0x4e79, 0x1e);
	write_cmos_sensor(0x4e7a, 0x00);
	write_cmos_sensor(0x4e7b, 0x00);
	write_cmos_sensor(0x4e7c, 0x2c);
	write_cmos_sensor(0x4e7d, 0x2f);
	write_cmos_sensor(0x4e7e, 0x79);
	write_cmos_sensor(0x4e7f, 0x7b);
	write_cmos_sensor(0x4e80, 0x0a);
	write_cmos_sensor(0x4e81, 0x31);
	write_cmos_sensor(0x4e82, 0x66);
	write_cmos_sensor(0x4e83, 0x81);
	write_cmos_sensor(0x4e84, 0x03);
	write_cmos_sensor(0x4e85, 0x40);
	write_cmos_sensor(0x4e86, 0x02);
	write_cmos_sensor(0x4e87, 0x09);
	write_cmos_sensor(0x4e88, 0x43);
	write_cmos_sensor(0x4e89, 0x53);
	write_cmos_sensor(0x4e8a, 0x32);
	write_cmos_sensor(0x4e8b, 0x67);
	write_cmos_sensor(0x4e8c, 0x05);
	write_cmos_sensor(0x4e8d, 0x83);
	write_cmos_sensor(0x4e8e, 0x00);
	write_cmos_sensor(0x4e8f, 0x00);
	write_cmos_sensor(0x4e90, 0x00);
	write_cmos_sensor(0x4e91, 0x00);
	write_cmos_sensor(0x4e92, 0x00);
	write_cmos_sensor(0x4e93, 0x00);
	write_cmos_sensor(0x4e94, 0x00);
	write_cmos_sensor(0x4e95, 0x00);
	write_cmos_sensor(0x4e96, 0x00);
	write_cmos_sensor(0x4e97, 0x00);
	write_cmos_sensor(0x4e98, 0x00);
	write_cmos_sensor(0x4e99, 0x00);
	write_cmos_sensor(0x4e9a, 0x00);
	write_cmos_sensor(0x4e9b, 0x00);
	write_cmos_sensor(0x4e9c, 0x00);
	write_cmos_sensor(0x4e9d, 0x00);
	write_cmos_sensor(0x4e9e, 0x00);
	write_cmos_sensor(0x4e9f, 0x00);
	write_cmos_sensor(0x4ea0, 0x00);
	write_cmos_sensor(0x4ea1, 0x00);
	write_cmos_sensor(0x4ea2, 0x00);
	write_cmos_sensor(0x4ea3, 0x00);
	write_cmos_sensor(0x4ea4, 0x00);
	write_cmos_sensor(0x4ea5, 0x00);
	write_cmos_sensor(0x4ea6, 0x1e);
	write_cmos_sensor(0x4ea7, 0x20);
	write_cmos_sensor(0x4ea8, 0x32);
	write_cmos_sensor(0x4ea9, 0x6d);
	write_cmos_sensor(0x4eaa, 0x18);
	write_cmos_sensor(0x4eab, 0x7f);
	write_cmos_sensor(0x4eac, 0x00);
	write_cmos_sensor(0x4ead, 0x00);
	write_cmos_sensor(0x4eae, 0x7c);
	write_cmos_sensor(0x4eaf, 0x07);
	write_cmos_sensor(0x4eb0, 0x7c);
	write_cmos_sensor(0x4eb1, 0x07);
	write_cmos_sensor(0x4eb2, 0x07);
	write_cmos_sensor(0x4eb3, 0x1c);
	write_cmos_sensor(0x4eb4, 0x07);
	write_cmos_sensor(0x4eb5, 0x1c);
	write_cmos_sensor(0x4eb6, 0x07);
	write_cmos_sensor(0x4eb7, 0x1c);
	write_cmos_sensor(0x4eb8, 0x07);
	write_cmos_sensor(0x4eb9, 0x1c);
	write_cmos_sensor(0x4eba, 0x07);
	write_cmos_sensor(0x4ebb, 0x14);
	write_cmos_sensor(0x4ebc, 0x07);
	write_cmos_sensor(0x4ebd, 0x1c);
	write_cmos_sensor(0x4ebe, 0x07);
	write_cmos_sensor(0x4ebf, 0x1c);
	write_cmos_sensor(0x4ec0, 0x07);
	write_cmos_sensor(0x4ec1, 0x1c);
	write_cmos_sensor(0x4ec2, 0x07);
	write_cmos_sensor(0x4ec3, 0x1c);
	write_cmos_sensor(0x4ec4, 0x2c);
	write_cmos_sensor(0x4ec5, 0x2f);
	write_cmos_sensor(0x4ec6, 0x79);
	write_cmos_sensor(0x4ec7, 0x7b);
	write_cmos_sensor(0x4ec8, 0x7c);
	write_cmos_sensor(0x4ec9, 0x07);
	write_cmos_sensor(0x4eca, 0x7c);
	write_cmos_sensor(0x4ecb, 0x07);
	write_cmos_sensor(0x4ecc, 0x00);
	write_cmos_sensor(0x4ecd, 0x00);
	write_cmos_sensor(0x4ece, 0x07);
	write_cmos_sensor(0x4ecf, 0x31);
	write_cmos_sensor(0x4ed0, 0x69);
	write_cmos_sensor(0x4ed1, 0x7f);
	write_cmos_sensor(0x4ed2, 0x67);
	write_cmos_sensor(0x4ed3, 0x00);
	write_cmos_sensor(0x4ed4, 0x00);
	write_cmos_sensor(0x4ed5, 0x00);
	write_cmos_sensor(0x4ed6, 0x7c);
	write_cmos_sensor(0x4ed7, 0x07);
	write_cmos_sensor(0x4ed8, 0x7c);
	write_cmos_sensor(0x4ed9, 0x07);
	write_cmos_sensor(0x4eda, 0x33);
	write_cmos_sensor(0x4edb, 0x7f);
	write_cmos_sensor(0x4edc, 0x00);
	write_cmos_sensor(0x4edd, 0x16);
	write_cmos_sensor(0x4ede, 0x00);
	write_cmos_sensor(0x4edf, 0x00);
	write_cmos_sensor(0x4ee0, 0x32);
	write_cmos_sensor(0x4ee1, 0x70);
	write_cmos_sensor(0x4ee2, 0x01);
	write_cmos_sensor(0x4ee3, 0x30);
	write_cmos_sensor(0x4ee4, 0x22);
	write_cmos_sensor(0x4ee5, 0x28);
	write_cmos_sensor(0x4ee6, 0x6f);
	write_cmos_sensor(0x4ee7, 0x75);
	write_cmos_sensor(0x4ee8, 0x00);
	write_cmos_sensor(0x4ee9, 0x00);
	write_cmos_sensor(0x4eea, 0x30);
	write_cmos_sensor(0x4eeb, 0x7f);
	write_cmos_sensor(0x4eec, 0x00);
	write_cmos_sensor(0x4eed, 0x00);
	write_cmos_sensor(0x4eee, 0x00);
	write_cmos_sensor(0x4eef, 0x00);
	write_cmos_sensor(0x4ef0, 0x69);
	write_cmos_sensor(0x4ef1, 0x7f);
	write_cmos_sensor(0x4ef2, 0x07);
	write_cmos_sensor(0x4ef3, 0x30);
	write_cmos_sensor(0x4ef4, 0x32);
	write_cmos_sensor(0x4ef5, 0x09);
	write_cmos_sensor(0x4ef6, 0x7d);
	write_cmos_sensor(0x4ef7, 0x65);
	write_cmos_sensor(0x4ef8, 0x00);
	write_cmos_sensor(0x4ef9, 0x00);
	write_cmos_sensor(0x4efa, 0x00);
	write_cmos_sensor(0x4efb, 0x00);
	write_cmos_sensor(0x4efc, 0x7f);
	write_cmos_sensor(0x4efd, 0x09);
	write_cmos_sensor(0x4efe, 0x7f);
	write_cmos_sensor(0x4eff, 0x09);
	write_cmos_sensor(0x4f00, 0x1e);
	write_cmos_sensor(0x4f01, 0x7c);
	write_cmos_sensor(0x4f02, 0x7f);
	write_cmos_sensor(0x4f03, 0x09);
	write_cmos_sensor(0x4f04, 0x7f);
	write_cmos_sensor(0x4f05, 0x0b);
	write_cmos_sensor(0x4f06, 0x7c);
	write_cmos_sensor(0x4f07, 0x02);
	write_cmos_sensor(0x4f08, 0x7c);
	write_cmos_sensor(0x4f09, 0x02);
	write_cmos_sensor(0x4f0a, 0x32);
	write_cmos_sensor(0x4f0b, 0x64);
	write_cmos_sensor(0x4f0c, 0x32);
	write_cmos_sensor(0x4f0d, 0x64);
	write_cmos_sensor(0x4f0e, 0x32);
	write_cmos_sensor(0x4f0f, 0x64);
	write_cmos_sensor(0x4f10, 0x32);
	write_cmos_sensor(0x4f11, 0x64);
	write_cmos_sensor(0x4f12, 0x31);
	write_cmos_sensor(0x4f13, 0x4f);
	write_cmos_sensor(0x4f14, 0x83);
	write_cmos_sensor(0x4f15, 0x84);
	write_cmos_sensor(0x4f16, 0x63);
	write_cmos_sensor(0x4f17, 0x64);
	write_cmos_sensor(0x4f18, 0x83);
	write_cmos_sensor(0x4f19, 0x84);
	write_cmos_sensor(0x4f1a, 0x31);
	write_cmos_sensor(0x4f1b, 0x32);
	write_cmos_sensor(0x4f1c, 0x7b);
	write_cmos_sensor(0x4f1d, 0x7c);
	write_cmos_sensor(0x4f1e, 0x2f);
	write_cmos_sensor(0x4f1f, 0x30);
	write_cmos_sensor(0x4f20, 0x30);
	write_cmos_sensor(0x4f21, 0x69);
	write_cmos_sensor(0x4d06, 0x08);
	write_cmos_sensor(0x5000, 0x0b);
	write_cmos_sensor(0x5001, 0x4b);
	write_cmos_sensor(0x5002, 0x57);
	write_cmos_sensor(0x5003, 0x42);
	write_cmos_sensor(0x5005, 0x00);
	write_cmos_sensor(0x5038, 0x00);
	write_cmos_sensor(0x5081, 0x00);
	write_cmos_sensor(0x5180, 0x00);
	write_cmos_sensor(0x5181, 0x10);
	write_cmos_sensor(0x5182, 0x07);
	write_cmos_sensor(0x5183, 0x8f);
	write_cmos_sensor(0x5820, 0xc5);
	write_cmos_sensor(0x5854, 0x00);
	write_cmos_sensor(0x58cb, 0x03);
	write_cmos_sensor(0x5bd0, 0x01);
	write_cmos_sensor(0x5bd1, 0x06);	//for mirror[on]& flip[on]
	write_cmos_sensor(0x5c0e, 0x11);
	write_cmos_sensor(0x5c11, 0x01);
	write_cmos_sensor(0x5c16, 0x02);
	write_cmos_sensor(0x5c17, 0x00);
	write_cmos_sensor(0x5c1a, 0x00);
	write_cmos_sensor(0x5c1b, 0x00);
	write_cmos_sensor(0x5c21, 0x10);
	write_cmos_sensor(0x5c22, 0x10);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x0a);
	write_cmos_sensor(0x5c25, 0x06);
	write_cmos_sensor(0x5c26, 0x0e);
	write_cmos_sensor(0x5c27, 0x02);
	write_cmos_sensor(0x5c28, 0x02);
	write_cmos_sensor(0x5c29, 0x0a);
	write_cmos_sensor(0x5c2a, 0x0a);
	write_cmos_sensor(0x5c2b, 0x01);
	write_cmos_sensor(0x5c2c, 0x00);
	write_cmos_sensor(0x5c2e, 0x08);
	write_cmos_sensor(0x5c30, 0x04);
	write_cmos_sensor(0x5c35, 0x03);
	write_cmos_sensor(0x5c36, 0x03);
	write_cmos_sensor(0x5c37, 0x03);
	write_cmos_sensor(0x5c38, 0x03);
	write_cmos_sensor(0x5d00, 0xff);
	write_cmos_sensor(0x5d01, 0x07);
	write_cmos_sensor(0x5d02, 0x80);
	write_cmos_sensor(0x5d03, 0x44);
	write_cmos_sensor(0x5d05, 0xfc);
	write_cmos_sensor(0x5d06, 0x0b);
	write_cmos_sensor(0x5d08, 0x10);
	write_cmos_sensor(0x5d09, 0x10);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x0a);
	write_cmos_sensor(0x5d0c, 0x06);
	write_cmos_sensor(0x5d0d, 0x0e);
	write_cmos_sensor(0x5d0e, 0x02);
	write_cmos_sensor(0x5d0f, 0x02);
	write_cmos_sensor(0x5d10, 0x0a);
	write_cmos_sensor(0x5d11, 0x0a);
	write_cmos_sensor(0x5d12, 0x01);
	write_cmos_sensor(0x5d13, 0x00);
	write_cmos_sensor(0x5d15, 0x10);
	write_cmos_sensor(0x5d16, 0x10);
	write_cmos_sensor(0x5d17, 0x10);
	write_cmos_sensor(0x5d18, 0x10);
	write_cmos_sensor(0x5d1a, 0x10);
	write_cmos_sensor(0x5d1b, 0x10);
	write_cmos_sensor(0x5d1c, 0x10);
	write_cmos_sensor(0x5d1d, 0x10);
	write_cmos_sensor(0x5d1e, 0x04);
	write_cmos_sensor(0x5d1f, 0x04);
	write_cmos_sensor(0x5d20, 0x04);
	write_cmos_sensor(0x5d27, 0x64);
	write_cmos_sensor(0x5d28, 0xc8);
	write_cmos_sensor(0x5d29, 0x96);
	write_cmos_sensor(0x5d2a, 0xff);
	write_cmos_sensor(0x5d2b, 0xc8);
	write_cmos_sensor(0x5d2c, 0xff);
	write_cmos_sensor(0x5d2d, 0x04);
	write_cmos_sensor(0x5d34, 0x00);
	write_cmos_sensor(0x5d35, 0x08);
	write_cmos_sensor(0x5d36, 0x00);
	write_cmos_sensor(0x5d37, 0x04);
	write_cmos_sensor(0x5d4a, 0x00);
	write_cmos_sensor(0x5d4c, 0x00);
	write_cmos_sensor(0x5900, 0x40);
	write_cmos_sensor(0x5901, 0x40);
	write_cmos_sensor(0x5902, 0x40);
	write_cmos_sensor(0x5903, 0x40);
	write_cmos_sensor(0x5904, 0x40);
	write_cmos_sensor(0x5905, 0x40);
	write_cmos_sensor(0x5906, 0x40);
	write_cmos_sensor(0x5907, 0x40);
	write_cmos_sensor(0x5908, 0x40);
	write_cmos_sensor(0x5909, 0x40);
	write_cmos_sensor(0x590a, 0x40);
	write_cmos_sensor(0x590b, 0x40);
	write_cmos_sensor(0x590c, 0x40);
	write_cmos_sensor(0x590d, 0x40);
	write_cmos_sensor(0x590e, 0x40);
	write_cmos_sensor(0x590f, 0x40);
	write_cmos_sensor(0x5910, 0x40);
	write_cmos_sensor(0x5911, 0x40);
	write_cmos_sensor(0x5912, 0x40);
	write_cmos_sensor(0x5913, 0x40);
	write_cmos_sensor(0x5914, 0x40);
	write_cmos_sensor(0x5915, 0x40);
	write_cmos_sensor(0x5916, 0x40);
	write_cmos_sensor(0x5917, 0x40);
	write_cmos_sensor(0x5918, 0x40);
	write_cmos_sensor(0x5919, 0x40);
	write_cmos_sensor(0x591a, 0x40);
	write_cmos_sensor(0x591b, 0x40);
	write_cmos_sensor(0x591c, 0x40);
	write_cmos_sensor(0x591d, 0x40);
	write_cmos_sensor(0x591e, 0x40);
	write_cmos_sensor(0x591f, 0x40);
	write_cmos_sensor(0x5920, 0x40);
	write_cmos_sensor(0x5921, 0x40);
	write_cmos_sensor(0x5922, 0x40);
	write_cmos_sensor(0x5923, 0x40);
	write_cmos_sensor(0x5924, 0x40);
	write_cmos_sensor(0x5925, 0x40);
	write_cmos_sensor(0x5926, 0x40);
	write_cmos_sensor(0x5927, 0x40);
	write_cmos_sensor(0x5928, 0x40);
	write_cmos_sensor(0x5929, 0x40);
	write_cmos_sensor(0x592a, 0x40);
	write_cmos_sensor(0x592b, 0x40);
	write_cmos_sensor(0x592c, 0x40);
	write_cmos_sensor(0x592d, 0x40);
	write_cmos_sensor(0x592e, 0x40);
	write_cmos_sensor(0x592f, 0x40);
	write_cmos_sensor(0x5930, 0x40);
	write_cmos_sensor(0x5931, 0x40);
	write_cmos_sensor(0x5932, 0x40);
	write_cmos_sensor(0x5933, 0x40);
	write_cmos_sensor(0x5934, 0x40);
	write_cmos_sensor(0x5935, 0x40);
	write_cmos_sensor(0x5936, 0x40);
	write_cmos_sensor(0x5937, 0x40);
	write_cmos_sensor(0x5938, 0x40);
	write_cmos_sensor(0x5939, 0x40);
	write_cmos_sensor(0x593a, 0x40);
	write_cmos_sensor(0x593b, 0x40);
	write_cmos_sensor(0x593c, 0x40);
	write_cmos_sensor(0x593d, 0x40);
	write_cmos_sensor(0x593e, 0x40);
	write_cmos_sensor(0x593f, 0x40);
	write_cmos_sensor(0x5940, 0x40);
	write_cmos_sensor(0x5941, 0x40);
	write_cmos_sensor(0x5942, 0x40);
	write_cmos_sensor(0x5943, 0x40);
	write_cmos_sensor(0x5944, 0x40);
	write_cmos_sensor(0x5945, 0x40);
	write_cmos_sensor(0x5946, 0x40);
	write_cmos_sensor(0x5947, 0x40);
	write_cmos_sensor(0x5948, 0x40);
	write_cmos_sensor(0x5949, 0x40);
	write_cmos_sensor(0x594a, 0x40);
	write_cmos_sensor(0x594b, 0x40);
	write_cmos_sensor(0x594c, 0x40);
	write_cmos_sensor(0x594d, 0x40);
	write_cmos_sensor(0x594e, 0x40);
	write_cmos_sensor(0x594f, 0x40);
	write_cmos_sensor(0x5950, 0x40);
	write_cmos_sensor(0x5951, 0x40);
	write_cmos_sensor(0x5952, 0x40);
	write_cmos_sensor(0x5953, 0x40);
	write_cmos_sensor(0x5954, 0x40);
	write_cmos_sensor(0x5955, 0x40);
	write_cmos_sensor(0x5956, 0x40);
	write_cmos_sensor(0x5957, 0x40);
	write_cmos_sensor(0x5958, 0x40);
	write_cmos_sensor(0x5959, 0x40);

}

static kal_uint32 streaming_control(kal_bool enable)
{
#if 1
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(0x0100, 0X01);
	else
		write_cmos_sensor(0x0100, 0x00);
#endif
	mdelay(30);
	return ERROR_NONE;
}

static void preview_setting(void)
{
	LOG_INF("E\n");
	
	write_cmos_sensor(0x0305, 0x7a);
	write_cmos_sensor(0x0307, 0x01);
	write_cmos_sensor(0x4837, 0x15);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x3639, 0xa6);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x04);
	write_cmos_sensor(0x3808, 0x09);
	write_cmos_sensor(0x3809, 0x00);
	write_cmos_sensor(0x380a, 0x06);
	write_cmos_sensor(0x380b, 0xc0);
	write_cmos_sensor(0x380c, 0x06);
	write_cmos_sensor(0x380d, 0xa4);
	write_cmos_sensor(0x380e, 0x07);
	write_cmos_sensor(0x380f, 0xa8);
	write_cmos_sensor(0x3814, 0x11);
	write_cmos_sensor(0x3815, 0x31);
	write_cmos_sensor(0x3820, 0x05); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3821, 0x02); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3822, 0x01);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x4015, 0x02);
	write_cmos_sensor(0x4016, 0x0d);
	write_cmos_sensor(0x4017, 0x00);
	write_cmos_sensor(0x4018, 0x03);
	write_cmos_sensor(0x401e, 0x01);
	write_cmos_sensor(0x401f, 0x38);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x67);
	write_cmos_sensor(0x4586, 0x00);
	write_cmos_sensor(0x4588, 0x02);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e1d, 0x30);
	write_cmos_sensor(0x4e26, 0x39);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x5000, 0x2b);
	write_cmos_sensor(0x5001, 0xcb);
	write_cmos_sensor(0x5002, 0x17);
	write_cmos_sensor(0x5820, 0xc1);
	write_cmos_sensor(0x5bd0, 0x09);
	write_cmos_sensor(0x5c0e, 0x11);
	write_cmos_sensor(0x5c21, 0x10);
	write_cmos_sensor(0x5c22, 0x08);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x0a);
	write_cmos_sensor(0x5c25, 0x06);
	write_cmos_sensor(0x5c26, 0x0e);
	write_cmos_sensor(0x5c27, 0x00);
	write_cmos_sensor(0x5c28, 0x00);
	write_cmos_sensor(0x5c29, 0x04);
	write_cmos_sensor(0x5c2a, 0x04);
	write_cmos_sensor(0x5d08, 0x10);
	write_cmos_sensor(0x5d09, 0x08);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x0a);
	write_cmos_sensor(0x5d0c, 0x06);
	write_cmos_sensor(0x5d0d, 0x0e);
	write_cmos_sensor(0x5d0e, 0x00);
	write_cmos_sensor(0x5d0f, 0x00);
	write_cmos_sensor(0x5d10, 0x04);
	write_cmos_sensor(0x5d11, 0x04);
	write_cmos_sensor(0x3501, 0x07);
	write_cmos_sensor(0x3502, 0xa0);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);
	

}				/*      preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
		if (0) {
	write_cmos_sensor(0x0305, 0x7a);
	write_cmos_sensor(0x0307, 0x01);
	write_cmos_sensor(0x4837, 0x15);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x3639, 0xa6);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x04);
	write_cmos_sensor(0x3808, 0x09);
	write_cmos_sensor(0x3809, 0x18);
	write_cmos_sensor(0x380a, 0x06);
	write_cmos_sensor(0x380b, 0xd4);
	write_cmos_sensor(0x380c, 0x06);
	write_cmos_sensor(0x380d, 0xa4);
	write_cmos_sensor(0x380e, 0x07);
	write_cmos_sensor(0x380f, 0xa8);
	write_cmos_sensor(0x3814, 0x11);
	write_cmos_sensor(0x3815, 0x31);
	write_cmos_sensor(0x3820, 0x05); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3821, 0x02); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3822, 0x01);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x4015, 0x02);
	write_cmos_sensor(0x4016, 0x0d);
	write_cmos_sensor(0x4017, 0x00);
	write_cmos_sensor(0x4018, 0x03);
	write_cmos_sensor(0x401e, 0x01);
	write_cmos_sensor(0x401f, 0x38);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x67);
	write_cmos_sensor(0x4586, 0x00);
	write_cmos_sensor(0x4588, 0x02);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e1d, 0x30);
	write_cmos_sensor(0x4e26, 0x39);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x5000, 0x2b);
	write_cmos_sensor(0x5001, 0xcb);
	write_cmos_sensor(0x5002, 0x17);
	write_cmos_sensor(0x5820, 0xc1);
	write_cmos_sensor(0x5bd0, 0x09);
	write_cmos_sensor(0x5c0e, 0x11);
	write_cmos_sensor(0x5c21, 0x10);
	write_cmos_sensor(0x5c22, 0x08);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x0a);
	write_cmos_sensor(0x5c25, 0x06);
	write_cmos_sensor(0x5c26, 0x0e);
	write_cmos_sensor(0x5c27, 0x00);
	write_cmos_sensor(0x5c28, 0x00);
	write_cmos_sensor(0x5c29, 0x04);
	write_cmos_sensor(0x5c2a, 0x04);
	write_cmos_sensor(0x5d08, 0x10);
	write_cmos_sensor(0x5d09, 0x08);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x0a);
	write_cmos_sensor(0x5d0c, 0x06);
	write_cmos_sensor(0x5d0d, 0x0e);
	write_cmos_sensor(0x5d0e, 0x00);
	write_cmos_sensor(0x5d0f, 0x00);
	write_cmos_sensor(0x5d10, 0x04);
	write_cmos_sensor(0x5d11, 0x04);
	write_cmos_sensor(0x3501, 0x07);
	write_cmos_sensor(0x3502, 0xa0);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);	
	return;
	}else if (currefps == 300) 
	{
		write_cmos_sensor(0x0305, 0x89);
	write_cmos_sensor(0x0307, 0x00);
	write_cmos_sensor(0x4837, 0x0a);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x3639, 0xa6);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x08);
	write_cmos_sensor(0x3808, 0x12);
	write_cmos_sensor(0x3809, 0x00);
	write_cmos_sensor(0x380a, 0x0d);
	write_cmos_sensor(0x380b, 0x80);
	write_cmos_sensor(0x380c, 0x03);
	write_cmos_sensor(0x380d, 0x52);
	write_cmos_sensor(0x380e, 0x0f);
	write_cmos_sensor(0x380f, 0x50);
	write_cmos_sensor(0x3814, 0x11);
	write_cmos_sensor(0x3815, 0x11);
	write_cmos_sensor(0x3820, 0x04); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3821, 0x02); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3822, 0x00);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x4015, 0x04);
	write_cmos_sensor(0x4016, 0x1b);
	write_cmos_sensor(0x4017, 0x04);
	write_cmos_sensor(0x4018, 0x0b);
	write_cmos_sensor(0x401e, 0x01);
	write_cmos_sensor(0x401f, 0x38);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x6f);
	write_cmos_sensor(0x4586, 0x00);
	write_cmos_sensor(0x4588, 0x02);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e1d, 0x30);
	write_cmos_sensor(0x4e26, 0x39);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x5000, 0x0b);
	write_cmos_sensor(0x5001, 0x4b);
	write_cmos_sensor(0x5002, 0x57);
	write_cmos_sensor(0x5820, 0xc5);
	write_cmos_sensor(0x5bd0, 0x01);
	write_cmos_sensor(0x5c0e, 0x11);
	write_cmos_sensor(0x5c21, 0x10);
	write_cmos_sensor(0x5c22, 0x10);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x0a);
	write_cmos_sensor(0x5c25, 0x06);
	write_cmos_sensor(0x5c26, 0x0e);
	write_cmos_sensor(0x5c27, 0x02);
	write_cmos_sensor(0x5c28, 0x02);
	write_cmos_sensor(0x5c29, 0x0a);
	write_cmos_sensor(0x5c2a, 0x0a);
	write_cmos_sensor(0x5d08, 0x10);
	write_cmos_sensor(0x5d09, 0x10);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x0a);
	write_cmos_sensor(0x5d0c, 0x06);
	write_cmos_sensor(0x5d0d, 0x0e);
	write_cmos_sensor(0x5d0e, 0x02);
	write_cmos_sensor(0x5d0f, 0x02);
	write_cmos_sensor(0x5d10, 0x0a);
	write_cmos_sensor(0x5d11, 0x0a);
	write_cmos_sensor(0x3501, 0x0f);
	write_cmos_sensor(0x3502, 0x48);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);
	return;
	}
	return;
}
static void custom1_setting(void)
{
	LOG_INF("E\n");
	write_cmos_sensor(0x0305, 0x2c);
	write_cmos_sensor(0x0307, 0x00);
	write_cmos_sensor(0x4837, 0x0d);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x3639, 0xa6);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x08);
	write_cmos_sensor(0x3808, 0x12);
	write_cmos_sensor(0x3809, 0x00);
	write_cmos_sensor(0x380a, 0x0d);
	write_cmos_sensor(0x380b, 0x80);
	write_cmos_sensor(0x380c, 0x04); 
	write_cmos_sensor(0x380d, 0x4c);
	write_cmos_sensor(0x380e, 0x0e);
	write_cmos_sensor(0x380f, 0xcc);
	write_cmos_sensor(0x3814, 0x11);
	write_cmos_sensor(0x3815, 0x11);
	write_cmos_sensor(0x3820, 0x04); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3821, 0x02); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3822, 0x00);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x4015, 0x04);
	write_cmos_sensor(0x4016, 0x1b);
	write_cmos_sensor(0x4017, 0x04);
	write_cmos_sensor(0x4018, 0x0b);
	write_cmos_sensor(0x401e, 0x01);
	write_cmos_sensor(0x401f, 0x38);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x6f);
	write_cmos_sensor(0x4586, 0x00);
	write_cmos_sensor(0x4588, 0x02);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e1d, 0x30);
	write_cmos_sensor(0x4e26, 0x39);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x5000, 0x0b);
	write_cmos_sensor(0x5001, 0x4b);
	write_cmos_sensor(0x5002, 0x57);
	write_cmos_sensor(0x5820, 0xc5);
	write_cmos_sensor(0x5bd0, 0x01);
	write_cmos_sensor(0x5c0e, 0x11);
	write_cmos_sensor(0x5c21, 0x10);
	write_cmos_sensor(0x5c22, 0x10);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x0a);
	write_cmos_sensor(0x5c25, 0x06);
	write_cmos_sensor(0x5c26, 0x0e);
	write_cmos_sensor(0x5c27, 0x02);
	write_cmos_sensor(0x5c28, 0x02);
	write_cmos_sensor(0x5c29, 0x0a);
	write_cmos_sensor(0x5c2a, 0x0a);
	write_cmos_sensor(0x5d08, 0x10);
	write_cmos_sensor(0x5d09, 0x10);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x0a);
	write_cmos_sensor(0x5d0c, 0x06);
	write_cmos_sensor(0x5d0d, 0x0e);
	write_cmos_sensor(0x5d0e, 0x02);
	write_cmos_sensor(0x5d0f, 0x02);
	write_cmos_sensor(0x5d10, 0x0a);
	write_cmos_sensor(0x5d11, 0x0a);
	write_cmos_sensor(0x3501, 0x0e); 
	write_cmos_sensor(0x3502, 0xc8);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);
}

static void custom2_setting(void)
{
	LOG_INF("E\n");
	write_cmos_sensor(0x0305, 0x2c);
	write_cmos_sensor(0x0307, 0x00);
	write_cmos_sensor(0x4837, 0x0d);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x3639, 0xa6);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x08);
	write_cmos_sensor(0x3808, 0x12);
	write_cmos_sensor(0x3809, 0x00);
	write_cmos_sensor(0x380a, 0x0d);
	write_cmos_sensor(0x380b, 0x80);
	write_cmos_sensor(0x380c, 0x04); 
	write_cmos_sensor(0x380d, 0x4c);
	write_cmos_sensor(0x380e, 0x0e);
	write_cmos_sensor(0x380f, 0xcc);
	write_cmos_sensor(0x3814, 0x11);
	write_cmos_sensor(0x3815, 0x11);
	write_cmos_sensor(0x3820, 0x00);
	write_cmos_sensor(0x3821, 0x06);
	write_cmos_sensor(0x3822, 0x00);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x4015, 0x04);
	write_cmos_sensor(0x4016, 0x1b);
	write_cmos_sensor(0x4017, 0x04);
	write_cmos_sensor(0x4018, 0x0b);
	write_cmos_sensor(0x401e, 0x01);
	write_cmos_sensor(0x401f, 0x38);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x6f);
	write_cmos_sensor(0x4586, 0x00);
	write_cmos_sensor(0x4588, 0x02);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e1d, 0x30);
	write_cmos_sensor(0x4e26, 0x39);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x5000, 0x0b);
	write_cmos_sensor(0x5001, 0x4b);
	write_cmos_sensor(0x5002, 0x57);
	write_cmos_sensor(0x5820, 0xc5);
	write_cmos_sensor(0x5bd0, 0x01);
	write_cmos_sensor(0x5c0e, 0x11);
	write_cmos_sensor(0x5c21, 0x10);
	write_cmos_sensor(0x5c22, 0x10);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x0a);
	write_cmos_sensor(0x5c25, 0x06);
	write_cmos_sensor(0x5c26, 0x0e);
	write_cmos_sensor(0x5c27, 0x02);
	write_cmos_sensor(0x5c28, 0x02);
	write_cmos_sensor(0x5c29, 0x0a);
	write_cmos_sensor(0x5c2a, 0x0a);
	write_cmos_sensor(0x5d08, 0x10);
	write_cmos_sensor(0x5d09, 0x10);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x0a);
	write_cmos_sensor(0x5d0c, 0x06);
	write_cmos_sensor(0x5d0d, 0x0e);
	write_cmos_sensor(0x5d0e, 0x02);
	write_cmos_sensor(0x5d0f, 0x02);
	write_cmos_sensor(0x5d10, 0x0a);
	write_cmos_sensor(0x5d11, 0x0a);
	write_cmos_sensor(0x3501, 0x0e); 
	write_cmos_sensor(0x3502, 0xc8);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);
}
static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
    imgsensor.pclk = imgsensor_info.custom1.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom1.linelength;
    imgsensor.frame_length = imgsensor_info.custom1.framelength; 
    imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    custom1_setting();
    return ERROR_NONE;
}   /*  Custom1   */
static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
   LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
    imgsensor.pclk = imgsensor_info.custom2.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom2.linelength;
    imgsensor.frame_length = imgsensor_info.custom2.framelength; 
    imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    custom2_setting();
    return ERROR_NONE;
}   /*  Custom2   */

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n", currefps);
	LOG_INF("E! video just has 30fps preview size setting ,NOT HAS 24FPS SETTING!\n");
	preview_setting();

}

static void hs_video_setting(void)
{
  	write_cmos_sensor(0x0305, 0x7a);
	write_cmos_sensor(0x0307, 0x01);
	write_cmos_sensor(0x4837, 0x15);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x3639, 0xa6);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x04);
	write_cmos_sensor(0x3808, 0x09);
	write_cmos_sensor(0x3809, 0x00);
	write_cmos_sensor(0x380a, 0x06);
	write_cmos_sensor(0x380b, 0xc0);
	write_cmos_sensor(0x380c, 0x06);
	write_cmos_sensor(0x380d, 0xa4);
	write_cmos_sensor(0x380e, 0x07);
	write_cmos_sensor(0x380f, 0xa8);
	write_cmos_sensor(0x3814, 0x11);
	write_cmos_sensor(0x3815, 0x31);
	write_cmos_sensor(0x3820, 0x05); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3821, 0x02); //for mirror[on]& flip[on]
	write_cmos_sensor(0x3822, 0x01);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x4015, 0x02);
	write_cmos_sensor(0x4016, 0x0d);
	write_cmos_sensor(0x4017, 0x00);
	write_cmos_sensor(0x4018, 0x03);
	write_cmos_sensor(0x401e, 0x01);
	write_cmos_sensor(0x401f, 0x38);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x67);
	write_cmos_sensor(0x4586, 0x00);
	write_cmos_sensor(0x4588, 0x02);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e1d, 0x30);
	write_cmos_sensor(0x4e26, 0x39);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x5000, 0x2b);
	write_cmos_sensor(0x5001, 0xcb);
	write_cmos_sensor(0x5002, 0x17);
	write_cmos_sensor(0x5820, 0xc1);
	write_cmos_sensor(0x5bd0, 0x09);
	write_cmos_sensor(0x5c0e, 0x11);
	write_cmos_sensor(0x5c21, 0x10);
	write_cmos_sensor(0x5c22, 0x08);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x0a);
	write_cmos_sensor(0x5c25, 0x06);
	write_cmos_sensor(0x5c26, 0x0e);
	write_cmos_sensor(0x5c27, 0x00);
	write_cmos_sensor(0x5c28, 0x00);
	write_cmos_sensor(0x5c29, 0x04);
	write_cmos_sensor(0x5c2a, 0x04);
	write_cmos_sensor(0x5d08, 0x10);
	write_cmos_sensor(0x5d09, 0x08);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x0a);
	write_cmos_sensor(0x5d0c, 0x06);
	write_cmos_sensor(0x5d0d, 0x0e);
	write_cmos_sensor(0x5d0e, 0x00);
	write_cmos_sensor(0x5d0f, 0x00);
	write_cmos_sensor(0x5d10, 0x04);
	write_cmos_sensor(0x5d11, 0x04);
	write_cmos_sensor(0x3501, 0x07);
	write_cmos_sensor(0x3502, 0xa0);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);

}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
	preview_setting();
}


/*#ifdef CONFIG_HQ_HARDWARE_INFO
#define DEBUG 0
static void get_eeprom_data(EEPROM_DATA *data)
{
	kal_uint8 i =0x0;
	u8 *otp_data = (u8 *)data;

	for (;i <= 0xE; i++, otp_data++)
		*otp_data = read_eeprom_module(i);

#if DEBUG
	otp_data = (u8 *)data;
	for (i=0;i<=0xE;i++)
		pr_err(" otpdata[0x%x]=0x%x    ", i, *(otp_data + i));
#endif
	return ;
}
#endif*/

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

static kal_uint32 return_sensor_id(void)
{
return ((read_cmos_sensor(0x300b) << 8) | read_cmos_sensor(0x300c));
}

extern char back_cam_name[64];
extern char back_cam_efuse_id[64];
extern u32 dual_main_sensorid;

static  void get_back_cam_efuse_id(void)
{
	int ret, i = 0;
	kal_uint8 efuse_id;

	ret = read_cmos_sensor(0x5000);
	write_cmos_sensor(0x5000, (0x00 & 0x08) | (ret & (~0x08)));

	write_cmos_sensor(0x0100, 0x01);
	msleep(5);
	write_cmos_sensor(0x3D84, 0x40);
	write_cmos_sensor(0x3D88, 0x70);
	write_cmos_sensor(0x3D89, 0x00);
	write_cmos_sensor(0x3D8A, 0x70);
	write_cmos_sensor(0x3D8B, 0x0f);
	write_cmos_sensor(0x0100, 0x01);

	for(i=0;i<16;i++)
	{
		efuse_id = read_cmos_sensor(0x7000+i);
		sprintf(back_cam_efuse_id+2*i,"%02x",efuse_id);
		msleep(1);
	}
}

static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			/* lx_revised */
			LOG_INF("[ov16a10]Read sensor id OK, write id:0x%x ,sensor Id:0x%x\n",
				imgsensor.i2c_write_id, *sensor_id);
			if (*sensor_id == imgsensor_info.sensor_id) {
                                memset(back_cam_name, 0x00, sizeof(back_cam_name));
                                memcpy(back_cam_name, "0_ov16a10_HLT", 64);
				ontim_get_otp_data(*sensor_id, NULL, 0);
				get_back_cam_efuse_id();
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
				/*#ifdef CONFIG_HQ_HARDWARE_INFO
					get_eeprom_data(&pOtp_data);
					hw_info_main_otp.otp_valid = pOtp_data.vaild_flag;
					hw_info_main_otp.vendor_id = pOtp_data.vendor_id;
					hw_info_main_otp.module_code = pOtp_data.module_code;
					hw_info_main_otp.module_ver = pOtp_data.module_ver;
					hw_info_main_otp.sw_ver = pOtp_data.sw_ver;
					hw_info_main_otp.year = pOtp_data.year;
					hw_info_main_otp.month = pOtp_data.month;
					hw_info_main_otp.day = pOtp_data.day;
					hw_info_main_otp.vcm_vendorid = pOtp_data.vcm_id;
					hw_info_main_otp.vcm_moduleid = pOtp_data.vcm_id;
				#endif*/

				return ERROR_NONE;
			}
			LOG_INF("Read sensor id OK:0x%x, id: 0x%x\n", imgsensor.i2c_write_id,
				*sensor_id);
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

	LOG_INF("park Read sensor sensor_id fail, id: 0x%x\n", *sensor_id);
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
	kal_uint8 retry = 1;
	kal_uint32 sensor_id = 0;

	LOG_INF("PLATFORM:MIPI 4LANE ov16a10 open+++++ ++++\n");

	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("[16885]i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x,sensor_id =0x%x\n",
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

//	write_cmos_sensor(0x0100, 0x00);
//	mdelay(10);
//	preview_setting();
//	write_cmos_sensor(0x0100, 0x01);

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x2D00;
	imgsensor.gain = 0x100;
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
	LOG_INF("E\n");

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
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	/* imgsensor.video_mode = KAL_FALSE; */
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.current_fps = imgsensor.current_fps;
/* imgsensor.autoflicker_en = KAL_FALSE; */
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
	
 	//mdelay(10); 
	set_mirror_flip(imgsensor.mirror); ////GIONEE:malp modify
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
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		/* PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 16m */
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		/* imgsensor.autoflicker_en = KAL_FALSE; */

	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF
			    ("Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",
			     imgsensor_info.cap1.max_framerate / 10,
			     imgsensor_info.cap.max_framerate);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		/* imgsensor.autoflicker_en = KAL_FALSE; */
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);
	
	//set_mirror_flip(imgsensor.mirror); ////GIONEE:malp modify

	return ERROR_NONE;
}				/* capture() */

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
	/* imgsensor.current_fps = 300; */
	/* imgsensor.autoflicker_en = KAL_FALSE; */
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(30);
	
	set_mirror_flip(imgsensor.mirror); ////GIONEE:malp modify

	return ERROR_NONE;
}				/*      normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/* imgsensor.current_fps = 300; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();

	//mdelay(10); 
	set_mirror_flip(imgsensor.mirror); ////GIONEE:malp modify

	return ERROR_NONE;
}				/*      hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			     MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/* imgsensor.current_fps = 300; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();

	//mdelay(10); 
	set_mirror_flip(imgsensor.mirror); ////GIONEE:malp modify

	return ERROR_NONE;
}				/*      slim_video       */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
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
	
	sensor_resolution->SensorCustom1Width  = imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height = imgsensor_info.custom1.grabwindow_height;

	sensor_resolution->SensorCustom2Width  = imgsensor_info.custom2.grabwindow_width;
	sensor_resolution->SensorCustom2Height = imgsensor_info.custom2.grabwindow_height;
	return ERROR_NONE;
}				/*      get_resolution  */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d %d\n", scenario_id, sensor_info->SensorOutputDataFormat);

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

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;	/* The frame of setting
										 * shutter default 0 for TG int
										 */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting
												 * sensor gain
												 */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
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
	sensor_info->PDAF_Support = 2;	//0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode

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
	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
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
		Custom1(image_window, sensor_config_data); // Custom1
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		Custom2(image_window, sensor_config_data); // Custom1
	break;
	default:
		LOG_INF("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}				/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)	/* Dynamic frame rate */
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
	if (enable)		/* enable auto flicker */
		imgsensor.autoflicker_en = KAL_TRUE;
	else			/* Cancel Auto flick */
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id,
						MUINT32 framerate)
{
	/* kal_int16 dummyLine; */
	kal_uint32 frameHeight;
	kal_uint32 frame_length;
	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	if (framerate == 0)
		return ERROR_NONE;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frameHeight =
		    imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		LOG_INF("frameHeight = %d\n", frameHeight);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight > imgsensor_info.pre.framelength)
		    ? (frameHeight - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frameHeight =
		    imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >  imgsensor_info.normal_video.framelength)
			? (frameHeight - imgsensor_info.normal_video.framelength) : 0;
		imgsensor.frame_length =
		    imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);

		set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		frameHeight =
		    imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);

		imgsensor.dummy_line = (frameHeight > imgsensor_info.cap.framelength)
		    ? (frameHeight - imgsensor_info.cap.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frameHeight =
		    imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >  imgsensor_info.hs_video.framelength)
		    ? (frameHeight - imgsensor_info.hs_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);

		set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frameHeight =
		    imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight > imgsensor_info.slim_video.framelength)
			? (frameHeight - imgsensor_info.slim_video.framelength) : 0;
		imgsensor.frame_length =
		    imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
		if (imgsensor.dummy_line < 0)
			imgsensor.dummy_line = 0;
		imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if(imgsensor.frame_length > imgsensor.shutter){
					set_dummy();
		}
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		frame_length = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.custom2.framelength) ? (frame_length - imgsensor_info.custom2.framelength) : 0;
			if (imgsensor.dummy_line < 0)
				imgsensor.dummy_line = 0;
		imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if(imgsensor.frame_length > imgsensor.shutter){
		set_dummy();
		}
		break;
	default:		/* coding with  preview scenario by default */
		frameHeight =
		    imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight > imgsensor_info.pre.framelength)
			? (frameHeight - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);

		set_dummy();
		LOG_INF("error scenario_id = %d, we use preview scenario\n", scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id,
						    MUINT32 *framerate)
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
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor(0x5081, 0x01);
		write_cmos_sensor(0x5000,(read_cmos_sensor(0x5000)&0xa1)|0x00 );
	} else {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor(0x5081, 0x00);
		write_cmos_sensor(0x5000,(read_cmos_sensor(0x5000)&0xa1)|0x040 );
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

#define EEPROM_READ_ID  0xA0
static void read_eeprom(int offset, char *data, kal_uint32 size)
{
	int i = 0, addr = offset;
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	for (i = 0; i < size; i++) {
		pu_send_cmd[0] = (char)(addr >> 8);
		pu_send_cmd[1] = (char)(addr & 0xFF);
		iReadRegI2C(pu_send_cmd, 2, &data[i], 1, EEPROM_READ_ID);

		addr++;
	}
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				  UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;
	/* unsigned long long *feature_return_para=(unsigned long long *) feature_para; */

	kal_uint32 rate;
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/* LOG_INF("feature_id = %d\n", feature_id); */
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
		set_max_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM) *feature_data,
					      *(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM) *(feature_data),
						  (MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL) * feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:	/* for factory mode auto testing */
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		LOG_INF("current fps :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = (UINT16) *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		LOG_INF("ihdr enable :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_en = (UINT8) *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
		case SENSOR_FEATURE_GET_VC_INFO:
		pr_debug("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16) *feature_data);

		pvcinfo =
	    (struct SENSOR_VC_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n", (UINT32) *feature_data);
		PDAFinfo = (SET_PD_BLOCK_INFO_T *) (uintptr_t) (*(feature_data + 1));
	
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				   sizeof(SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n", *feature_data);
		/* PDAF capacity enable or not, OV16885 only full size support PDAF */
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		default:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
		break;
	case SENSOR_FEATURE_SET_PDAF:
		LOG_INF("PDAF mode :%d\n", *feature_data_16);
		imgsensor.pdaf_mode = *feature_data_16;
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
		LOG_INF("SENSOR_FEATURE_GET_MIPI_PIXEL_RATE\n");
		switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				 rate =	imgsensor_info.cap.mipi_pixel_rate;
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
			default:
				rate = 0;
				break;
			}
		LOG_INF("SENSOR_FEATURE_GET_MIPI_PIXEL_RATE wangweifeng:rate:%d\n",rate);
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;	
	}
	break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32) *feature_data);
		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[1],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[2],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[3],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[4],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n", (UINT16) *feature_data,
			(UINT16) *(feature_data + 1), (UINT16) *(feature_data + 2));
		ihdr_write_shutter_gain((UINT16) *feature_data, (UINT16) *(feature_data + 1),
					(UINT16) *(feature_data + 2));
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
	case SENSOR_FEATURE_GET_4CELL_DATA:/*get 4 cell data from eeprom*/
	{
		int type = (kal_uint16)(*feature_data);
		char *data = (char *)((feature_data+1));

		if (type == FOUR_CELL_CAL_TYPE_XTALK_CAL) {/*only copy Cross Talk calibration data*/
			read_eeprom(0x763, data, 600+2);
			LOG_INF("read Cross Talk calibration data size= %d %d\n", data[0], data[1]);
		} else if (type == FOUR_CELL_CAL_TYPE_DPC) {
			read_eeprom(0x9BE, data, 832+2);
			LOG_INF("read DPC calibration data size= %d %d\n", data[0], data[1]);
		}
		break;
	}
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME://lzl
		set_shutter_frame_length((UINT16)*feature_data,(UINT16)*(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_PIXEL_RATE: {
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
		}
	}
	break;
	default:
		break;
	}

	return ERROR_NONE;
}				/*    feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 BLACKJACK_HLT_OV16A10_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}				/*      OV5693_MIPI_RAW_SensorInit      */
