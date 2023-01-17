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
 *	 mot_lyriq_ov50amipi_Sensor.c
 *
 * Project:
 * --------
 *	 MTK SOP
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
#define PFX "MOT_LYRIQ_OV50A_camera_sensor"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


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

#include "mot_lyriq_ov50amipiraw_Sensor.h"
#include "mot_lyriq_ov50a_Sensor_setting.h"
#include "mot_lyriq_ov50a_ana_gain_table.h"

extern mot_calibration_status_t *LYRIQ_OV50A_eeprom_get_calibration_status(void);
extern mot_calibration_mnf_t *LYRIQ_OV50A_eeprom_get_mnf_info(void);
extern void LYRIQ_OV50A_eeprom_format_calibration_data(struct imgsensor_struct *pImgsensor);

static DEFINE_SPINLOCK(imgsensor_drv_lock);
#define PD_PIX_2_EN 0

#define FPT_PDAF_SUPPORT 1
static int full_remosaic_mode =0;
static int crop_remosaic_mode =0;

extern void write_cross_talk_data(void);
static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_LYRIQ_OV50A_SENSOR_ID,

	.checksum_value = 0x388c7147,
	.pre = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4164,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1316016000,
	},
	.cap = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4164,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1316016000,
	},
	.normal_video = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4164,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 2304,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1316016000,
	},
	.hs_video = {
		.pclk = 75000000,
		.linelength = 275,
		.framelength = 2272,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2048,
		.grabwindow_height = 1536,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
		.mipi_pixel_rate = 1316016000,
	},
	.slim_video = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4164,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1316016000,
	},
	.custom1 = {
		.pclk = 75000000,
		.linelength = 390,
		.framelength = 6408,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 8192,
		.grabwindow_height = 6144,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1778400000,
	},
	.custom2 = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 2080,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2048,
		.grabwindow_height = 1536,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 600,
		.mipi_pixel_rate = 1316016000,
	},
	.custom3 = {
		.pclk = 75000000,
		.linelength = 225,
		.framelength = 1388,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1316016000,
	},
	.custom4 = {
		.pclk = 75000000,
		.linelength = 450,
		.framelength = 2776,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 2304,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1316016000,
	},
	.custom5 = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4164,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1316016000,
	},
	.margin = 32,					/* sensor framelength & shutter margin */
	.min_shutter = 16,				/* min shutter */
	.min_gain = BASEGAIN, /*1x gain*/
	.max_gain = 255*BASEGAIN, /*255x gain*/
	.min_gain_iso = 100,
	.gain_step = 1, /*minimum step = 4 in 1x~2x gain*/
	.gain_type = 1,/*to be modify,no gain table for sony*/
	.max_frame_length = 0xffffff,     /* max framelength by sensor register's limitation */
	.ae_shut_delay_frame = 0,		//check
	.ae_sensor_gain_delay_frame = 0,//check
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 10,			//support sensor mode num

	.cap_delay_frame = 2,			//enter capture delay frame num
	.pre_delay_frame = 2,			//enter preview delay frame num
	.video_delay_frame = 2,			//enter video delay frame num
	.hs_video_delay_frame = 2,		//enter high speed video  delay frame num
	.slim_video_delay_frame = 2,	//enter slim video delay frame num
	.custom1_delay_frame = 2,		//enter custom1 delay frame num
	.custom2_delay_frame = 2,		//enter custom2 delay frame num
	.custom3_delay_frame = 2,		//enter custom3 delay frame num
	.custom4_delay_frame = 2,		//enter custom4 delay frame num
	.custom5_delay_frame = 2,		//enter custom5 delay frame num
	.frame_time_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_CPHY,
	.mipi_settle_delay_mode = 0,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_3_LANE,//mipi lane num
	.i2c_addr_table = {0x20, 0xff},
	.i2c_speed = 1000,
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_HV_MIRROR,	/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT,
	/* IMGSENSOR_MODE enum value,record current sensor mode,such as:
	 * INIT, Preview, Capture, Video,High Speed Video, Slim Video
	 */
	.shutter = 0x100,	/* current shutter */
	.gain = 0xe0,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_mode = 0, /* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x20, /* record current sensor's i2c write id */
	.extend_frame_length_en = KAL_FALSE,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] = {
	/* Preview*/
	{8192, 6144,    0,    0, 8192, 6144, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
	/* capture */
	{8192, 6144,    0,    0, 8192, 6144, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
	/* video */
	{8192, 6144,    0,  768, 8192, 4608, 4096, 2304,  0,  0, 4096, 2304, 0, 0, 4096, 2304},
	/* hs_video */
	{8192, 6144,    0,    0, 8192, 6144, 2048, 1536,  0,   0, 2048, 1536, 0, 0, 2048, 1536},
	/* slim_video */
	{8192, 6144,    0,    0, 8192, 6144, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
	/* custom1 */
	{8192, 6144,    0,    0, 8192, 6144, 8192, 6144,  0,   0, 8192, 6144, 0, 0, 8192, 6144},
	/* custom2 */
	{8192, 6144,    0,    0, 8192, 6144, 2048, 1536,  0,   0, 2048, 1536, 0, 0, 2048, 1536},
	/* custom3 */
	{8192, 6144,    256, 912, 7680, 4320, 1920, 1080,  0,   0, 1920, 1080, 0, 0, 1920, 1080},
	/* custom4 */
	{8192, 6144,    0,   768, 8192, 4608, 4096, 2304,  0,  0, 4096, 2304, 0, 0, 4096, 2304},
	/* custom5 */
	{8192, 6144,  2048,  1536, 4096, 3072, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
};

//the index order of VC_STAGGER_NE/ME/SE in array identify the order of readout in MIPI transfer
static struct SENSOR_VC_INFO2_STRUCT SENSOR_VC_INFO2[4] = {
	{
		0x03, 0x0a, 0x00, 0x08, 0x40, 0x00, //preivew
		{
			{VC_STAGGER_NE, 0x00, 0x2b, 0x1000, 0xc00},
			{VC_PDAF_STATS, 0x01, 0x30, 0x1400, 0x300},
#if PD_PIX_2_EN
			{VC_PDAF_STATS_NE_PIX_2, 0x01, 0x2b, 0x1000, 0x300},
#endif
		},
		1
	},
	{
		0x03, 0x0a, 0x00, 0x08, 0x40, 0x00, //60fps
		{
			{VC_STAGGER_NE, 0x00, 0x2b, 0x0800, 0x0600},
			{VC_PDAF_STATS, 0x01, 0x30, 0xA00, 0x300},
#if PD_PIX_2_EN
			{VC_PDAF_STATS_NE_PIX_2, 0x01, 0x2b, 0x1000, 0x180},
#endif
		},
		1
	},
	{
		0x03, 0x0a, 0x00, 0x08, 0x40, 0x00, //custom4
		{
			{VC_STAGGER_NE, 0x00, 0x2b, 0x1000, 0x900},
			{VC_STAGGER_ME, 0x01, 0x2b, 0x1000, 0x900},
			{VC_PDAF_STATS, 0x02, 0x30, 0x1400, 0x240},
#if PD_PIX_2_EN
			{VC_PDAF_STATS_NE_PIX_2, 0x01, 0x2b, 0x1000, 0x240},
#endif
		},
		1
	},
	{
		0x03, 0x0a, 0x00, 0x08, 0x40, 0x00, //normal video
		{
			{VC_STAGGER_NE, 0x00, 0x2b, 0x1000, 0x900},
			{VC_PDAF_STATS, 0x01, 0x30, 0x1400, 0x240},
#if PD_PIX_2_EN
			{VC_PDAF_STATS_NE_PIX_2, 0x01, 0x2b, 0x1000, 0x240},
#endif
		},
		1
	},
};

static void get_vc_info_2(struct SENSOR_VC_INFO2_STRUCT *pvcinfo2, kal_uint32 scenario)
{
	switch (scenario) {

	case MSDK_SCENARIO_ID_CUSTOM2:
		memcpy((void *)pvcinfo2, (void *)&SENSOR_VC_INFO2[1],
			sizeof(struct SENSOR_VC_INFO2_STRUCT));
		break;
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		memcpy((void *)pvcinfo2, (void *)&SENSOR_VC_INFO2[0],
			sizeof(struct SENSOR_VC_INFO2_STRUCT));
		break;
	case MSDK_SCENARIO_ID_CUSTOM4:
                memcpy((void *)pvcinfo2, (void *)&SENSOR_VC_INFO2[2],
                        sizeof(struct SENSOR_VC_INFO2_STRUCT));
                break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		memcpy((void *)pvcinfo2, (void *)&SENSOR_VC_INFO2[3],
			sizeof(struct SENSOR_VC_INFO2_STRUCT));
		break;
	default:
		break;
	}
}

#if FPT_PDAF_SUPPORT
/*PD information update*/
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 0,
	.i4OffsetY = 0,
	.i4PitchX = 0,
	.i4PitchY = 0,
	.i4PairNum = 0,
	.i4SubBlkW = 0,
	.i4SubBlkH = 0,
	.i4PosL = {{0, 0} },
	.i4PosR = {{0, 0} },
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst = 0,
	.i4Crop = {
		{0, 0}, {0, 0}, {0, 384}, {0, 0}, {0, 0},
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
	},  //{0, 1632}
	.iMirrorFlip = 0,
};
#endif

static struct IMGSENSOR_I2C_CFG *get_i2c_cfg(void)
{
	return &(((struct IMGSENSOR_SENSOR_INST *)
		  (imgsensor.psensor_func->psensor_inst))->i2c_cfg);
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
	char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF),
			(char)(para & 0xFF)};

	imgsensor_i2c_write(
		get_i2c_cfg(),
		pusendcmd,
		3,
		3,
		imgsensor.i2c_write_id,
		imgsensor_info.i2c_speed);
}
/*
static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {
	(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF) };
	imgsensor_i2c_write(
		get_i2c_cfg(),
		pusendcmd,
		4,
		4,
		imgsensor.i2c_write_id,
		IMGSENSOR_I2C_SPEED);
}*/

static void write_frame_len(kal_uint32 fll)
{
	pr_debug("fll %d\n", imgsensor.frame_length);

	if (imgsensor.extend_frame_length_en == KAL_FALSE) {
		write_cmos_sensor_8(0x3840, (imgsensor.frame_length >> 16) & 0xFF);
		write_cmos_sensor_8(0x380e, (imgsensor.frame_length >> 8) & 0XFF);
		write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);
	}
}

static void set_dummy(void)
{
	pr_err("dummyline = %d, dummypixels = %d\n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor_8(0x3840, (imgsensor.frame_length >> 16) & 0xFF);
	write_cmos_sensor_8(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
	write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);

	write_cmos_sensor_8(0x380c, imgsensor.line_length >> 8);
	write_cmos_sensor_8(0x380d, imgsensor.line_length & 0xFF);
}	/*	set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor_8(0x300a) << 16) | (read_cmos_sensor_8(0x300b) << 8) | read_cmos_sensor_8(0x300c) + 1);
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	pr_err("framerate = %d, min framelength should enable %d\n", framerate, min_framelength_en);

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
			/* Extend frame length */
			write_cmos_sensor_8(0x3840, (imgsensor.frame_length >> 16) & 0xFF);
			write_cmos_sensor_8(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
			write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);
		}
	} else {
		/* Extend frame length*/
		write_cmos_sensor_8(0x3840, (imgsensor.frame_length >> 16) & 0xFF);
		write_cmos_sensor_8(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
		write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);
	}

	/* Update Shutter */
	write_cmos_sensor_8(0x3500, (shutter >> 16) & 0xFF);
	write_cmos_sensor_8(0x3501, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(0x3502, shutter & 0xFF);

	pr_err("frame_length = %d , shutter = %d\n", imgsensor.frame_length, shutter);

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
				     kal_uint16 frame_length,
				     kal_bool auto_extend_en)
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

	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;

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
			write_cmos_sensor_8(0x3840, (imgsensor.frame_length >> 16) & 0xFF);
			write_cmos_sensor_8(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
			write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor_8(0x3840, (imgsensor.frame_length >> 16) & 0xFF);
		write_cmos_sensor_8(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
		write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);
	}

	/* Update Shutter */
	write_cmos_sensor_8(0x3500, (shutter >> 16) & 0xFF);
	write_cmos_sensor_8(0x3501, (shutter >> 8) & 0xFF);
	write_cmos_sensor_8(0x3502, shutter & 0xFF);

    pr_err("frame_length = %d , shutter = %d \n", imgsensor.frame_length, shutter);

}	/* set_shutter_frame_length */

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 iReg = 0x0000;

	//remosaic maxgain = 64*BASEGAIN, other sensor maxgain=255*BASEGAIN
	iReg = gain*256/BASEGAIN;
	return iReg;		/* sensorGlobalGain */
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
	kal_uint32 max_gain = imgsensor_info.max_gain;
	if(full_remosaic_mode == 1)
	{
		max_gain = 16*BASEGAIN;
	}
	if(crop_remosaic_mode == 1)
	{
		max_gain = 64*BASEGAIN;
	}

	if (gain < imgsensor_info.min_gain || gain > max_gain) {
		pr_err("Error gain setting: gain %d, min_gain %d, max_gain %d\n", gain, imgsensor_info.min_gain, max_gain);

		if (gain <imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else if (gain > max_gain)
			gain = max_gain;
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	pr_err("gain = %d, reg_gain = 0x%x, max_gain:0x%x\n",
		gain, reg_gain, max_gain);

        write_cmos_sensor_8(0x03508, reg_gain >> 8);
        write_cmos_sensor_8(0x03509, reg_gain & 0xff);

	return gain;
} /* set_gain */

static void hdr_write_tri_shutter(kal_uint16 le, kal_uint16 me, kal_uint16 se)
{
	kal_uint16 realtime_fps = 0;

/*	kal_uint16 maxcit = imgsensor.frame_length - 46;
	kal_uint16 lastse = 0;
	kal_uint16 maxcitlong = MAXCIT -lastse;

	pr_debug("E! le:0x%x, me:0x%x, se:0x%x maxcit %d maxcitlong %d frame_length %d\n",
                le, me, se, MAXCIT, maxcitlong, imgsensor.frame_length);

	le = (kal_uint16)max(imgsensor_info.min_shutter, (kal_uint32)le);
	se = (kal_uint16)max(imgsensor_info.min_shutter, (kal_uint32)se);

	le = (kal_uint16)min((kal_uint32)MAXCIT, (kal_uint32)le);
	le = (kal_uint16)min((kal_uint32)maxcitlong, (kal_uint32)le);

	le = (kal_uint16)max((kal_uint32)le, (kal_uint32)se);
	lastse = se;
*/
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = max((kal_uint32)(le + me + se + imgsensor_info.margin),
		imgsensor.min_frame_length);
	imgsensor.frame_length = min(imgsensor.frame_length, imgsensor_info.max_frame_length);
	spin_unlock(&imgsensor_drv_lock);

	pr_debug("E! le:0x%x, me:0x%x, se:0x%x autoflicker_en %d frame_length %d\n",
		le, me, se, imgsensor.autoflicker_en, imgsensor.frame_length);

	if (imgsensor.autoflicker_en) {
		realtime_fps =
			imgsensor.pclk / imgsensor.line_length * 10 /
			imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			write_frame_len(imgsensor.frame_length);
		}
	} else {
		write_frame_len(imgsensor.frame_length);
	}

	/* Long exposure */
	write_cmos_sensor_8(0x3500, (le >> 16) & 0xFF);
	write_cmos_sensor_8(0x3501, (le >> 8) & 0xFF);
	write_cmos_sensor_8(0x3502, le & 0xFF);
	/* Muddle exposure */
	if (me != 0) {
		write_cmos_sensor_8(0x3580, (me >> 16) & 0xFF);
		/*MID_COARSE_INTEG_TIME[15:8]*/
		write_cmos_sensor_8(0x3581, (me >> 8) & 0xFF);
		/*MID_COARSE_INTEG_TIME[7:0]*/
		write_cmos_sensor_8(0x3582, me & 0xFF);
	}
	/* Short exposure */
	write_cmos_sensor_8(0x3540, (se >> 16) & 0xFF);
	write_cmos_sensor_8(0x3541, (se >> 8) & 0xFF);
	write_cmos_sensor_8(0x3542, se & 0xFF);

	pr_debug("L! le:0x%x, me:0x%x, se:0x%x\n", le, me, se);
}

static void hdr_write_tri_gain(kal_uint16 lgain, kal_uint16 mg, kal_uint16 sg)
{
	kal_uint16 reg_lg, reg_mg, reg_sg;

	reg_lg = gain2reg(lgain);
	reg_mg = gain2reg(mg);
	reg_sg = gain2reg(sg);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_lg;
	spin_unlock(&imgsensor_drv_lock);

	if (reg_lg > 0x4000) {
		reg_lg = 0x4000;
	} else {
		if (reg_lg < 0x100)// sensor 1xGain
			reg_lg = 0x100;
	}

	if (reg_sg > 0x4000) {
		reg_sg = 0x4000;
	} else {
		if (reg_sg < 0x100)// sensor 1xGain
			reg_sg = 0x100;
	}

	/* Long Gian */
	write_cmos_sensor_8(0x3508, (reg_lg>>8) & 0xFF);
	write_cmos_sensor_8(0x3509, reg_lg & 0xFF);
	/* Middle Gian */
	if (mg != 0) {
		write_cmos_sensor_8(0x3588, (reg_mg>>8) & 0xFF);
		write_cmos_sensor_8(0x3589, reg_mg & 0xFF);
	}
	/* Short Gian */
	write_cmos_sensor_8(0x3548, (reg_sg>>8) & 0xFF);
	write_cmos_sensor_8(0x3549, reg_sg & 0xFF);

	pr_debug(
		"lgain:0x%x, reg_lg:0x%x, mg:0x%x, reg_mg:0x%x, sg:0x%x, reg_sg:0x%x\n",
		lgain, reg_lg, mg, reg_mg, sg, reg_sg);
}

void extend_frame_length(kal_uint32 ns)
{
	UINT32 old_fl = imgsensor.frame_length;

	kal_uint32 per_frame_ms =
		(kal_uint32)(((unsigned long long)imgsensor.frame_length *
		(unsigned long long)imgsensor.line_length * 1000) /
		(unsigned long long)imgsensor.pclk);
	pr_debug("per_frame_ms: %d / %d = %d",
		(imgsensor.frame_length * imgsensor.line_length * 1000),
		imgsensor.pclk,
		per_frame_ms);

	imgsensor.frame_length = (per_frame_ms + (ns / 1000000)) *
		imgsensor.frame_length / per_frame_ms;

	write_frame_len(imgsensor.frame_length);

	imgsensor.extend_frame_length_en = KAL_TRUE;

	pr_debug("new frame len = %d, old frame len = %d, per_frame_ms = %d, add more %d ms",
		imgsensor.frame_length, old_fl, per_frame_ms, (ns / 1000000));
}

/*
static void set_mirror_flip(kal_uint8 image_mirror)
{
	pr_err("image_mirror = %d", image_mirror);

	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor_8(0x3820,
			((read_cmos_sensor_8(0x3820) & 0xF9) | 0x00));
		write_cmos_sensor_8(0x3821,
			((read_cmos_sensor_8(0x3821) & 0xF9) | 0x00));
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor_8(0x3820,
			((read_cmos_sensor_8(0x3820) & 0xF9) | 0x04));
		write_cmos_sensor_8(0x3821,
			((read_cmos_sensor_8(0x3821) & 0xF9) | 0x00));
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor_8(0x3820,
			((read_cmos_sensor_8(0x3820) & 0xF9) | 0x00));
		write_cmos_sensor_8(0x3821,
			((read_cmos_sensor_8(0x3821) & 0xF9) | 0x04));
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor_8(0x3820,
			((read_cmos_sensor_8(0x3820) & 0xF9) | 0x04));
		write_cmos_sensor_8(0x3821,
			((read_cmos_sensor_8(0x3821) & 0xF9) | 0x04));
		break;
	default:
		pr_err("Error image_mirror setting");
		break;
	}

}*/


static kal_uint32 streaming_control(kal_bool enable)
{
	pr_err("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable)
		write_cmos_sensor_8(0x0100, 0x01); // stream on
	else
		write_cmos_sensor_8(0x0100, 0x00); // stream off

	mdelay(10);
	return ERROR_NONE;
}

#define I2C_BUFFER_LEN 765
kal_uint16 mot_lyriq_ov50a_table_write_cmos_sensor(kal_uint16 *para, kal_uint32 len)
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
		if ((I2C_BUFFER_LEN - tosend) < 3 || IDX == len || addr != addr_last) {
			imgsensor_i2c_write(
						get_i2c_cfg(),
						puSendCmd,
						tosend,
						3,
						imgsensor.i2c_write_id,
						imgsensor_info.i2c_speed);
			tosend = 0;
		}
	}
	return 0;
}

static void sensor_init(void)
{
	pr_err("MOT LYRIQ OV50A init start\n");
	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_init_mot_lyriq_ov50a,
	    sizeof(addr_data_pair_init_mot_lyriq_ov50a)/sizeof(kal_uint16));
       pr_err("%s: Applying xtalk...", __func__);
       write_cross_talk_data();
       pr_err("MOT LYRIQ OV50A init end\n");
}	/*	  sensor_init  */

static void preview_setting(void)
{
	pr_err("MOT LYRIQ OV50A preview_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =0;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_preview_mot_lyriq_ov50a,
		sizeof(addr_data_pair_preview_mot_lyriq_ov50a)/sizeof(kal_uint16));

	pr_err("MOT LYRIQ OV50A preview_setting end\n");

} /* preview_setting */

/*full size 30fps*/
static void capture_setting(kal_uint16 currefps)
{
	pr_err("MOT LYRIQ OV50A capture_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =0;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_capture_mot_lyriq_ov50a,
		sizeof(addr_data_pair_capture_mot_lyriq_ov50a)/sizeof(kal_uint16));

	pr_err("MOT LYRIQ OV50A capture_setting end\n");
}

static void normal_video_setting(kal_uint16 currefps)
{
	pr_err("MOT LYRIQ OV50A normal_video_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =0;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_video_mot_lyriq_ov50a,
		sizeof(addr_data_pair_video_mot_lyriq_ov50a)/sizeof(kal_uint16));

	pr_err("MOT LYRIQ OV50A normal_video_setting end\n");
}

static void hs_video_setting(void)
{
	pr_err("MOT LYRIQ OV50A hs_video_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =0;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_120fpsov50a,
		sizeof(addr_data_pair_120fpsov50a)/sizeof(kal_uint16));

	pr_err("MOT LYRIQ OV50A hs_video_setting end\n");
}

static void slim_video_setting(void)
{
	pr_err("MOT LYRIQ OV50A slim_video_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =0;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_slim_video_mot_lyriq_ov50a,
		sizeof(addr_data_pair_slim_video_mot_lyriq_ov50a)/sizeof(kal_uint16));

	pr_err("MOT LYRIQ OV50A slim_video_setting end\n");
}

static void custom1_setting(void)
{
	pr_err("MOT LYRIQ OV50A custom1_setting start\n");

	full_remosaic_mode =1;
	crop_remosaic_mode =0;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_fullsize_ov50a,
		sizeof(addr_data_pair_fullsize_ov50a)/sizeof(kal_uint16));
	pr_err("OV50A40 custom1_setting end\n");
}
static void custom2_setting(void)
{
	pr_err("OV50A40 custom2_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =0;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_60fps_ov50a,
		sizeof(addr_data_pair_60fps_ov50a)/sizeof(kal_uint16));
	pr_err("OV50A40 custom2_setting end\n");
}

static void custom3_setting(void)
{
	pr_err("MOT LYRIQ OV50A custom3_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =1;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_custom3,
		sizeof(addr_data_pair_custom3)/sizeof(kal_uint16));
	pr_err("OV50A40 custom3_setting end\n");
}
static void custom4_setting(void)
{
	pr_err("OV50A40 custom4_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =1;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_custom4,
		sizeof(addr_data_pair_custom4)/sizeof(kal_uint16));

	pr_err("OV50A40 custom4_setting end\n");
}
static void custom5_setting(void)
{
	pr_err("OV50A40 custom5_setting start\n");

	full_remosaic_mode =0;
	crop_remosaic_mode =1;

	mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_custom5,
		sizeof(addr_data_pair_custom5)/sizeof(kal_uint16));

	pr_err("OV50A40 custom5_setting end\n");
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
			if (*sensor_id == imgsensor_info.sensor_id) {
				pr_err("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
				LYRIQ_OV50A_eeprom_format_calibration_data(&imgsensor);
				return ERROR_NONE;
			}

			pr_err("Read sensor id fail,read:0x%x id: 0x%x\n", return_sensor_id(), imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
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

	pr_err("%s +\n", __func__);

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				pr_err("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
				break;
			}
			pr_err("Read sensor id fail, id: 0x%x\n",
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

	sensor_init();
	//set_mirror_flip(imgsensor.mirror);

	spin_lock(&imgsensor_drv_lock);

	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	spin_unlock(&imgsensor_drv_lock);
	pr_err("%s -\n", __func__);

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
	pr_err("E\n");
	/* No Need to implement this function */
	streaming_control(KAL_FALSE);
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
	pr_err("%s E\n", __func__);

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
	pr_err("%s E\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;

	if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
		pr_err(
			"Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
			imgsensor.current_fps,
			imgsensor_info.cap.max_framerate / 10);
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;

	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);

	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("%s E\n", __func__);

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
	pr_err("%s E\n", __func__);

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

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("%s E\n", __func__);

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

	return ERROR_NONE;
}	/* slim_video */

static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("%s E\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	custom1_setting();
	return ERROR_NONE;
}
static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("%s E\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	imgsensor.pclk = imgsensor_info.custom2.pclk;
	imgsensor.line_length = imgsensor_info.custom2.linelength;
	imgsensor.frame_length = imgsensor_info.custom2.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom2_setting();
	return ERROR_NONE;
}

static kal_uint32 Custom3(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("%s E\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
	imgsensor.pclk = imgsensor_info.custom3.pclk;
	imgsensor.line_length = imgsensor_info.custom3.linelength;
	imgsensor.frame_length = imgsensor_info.custom3.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	custom3_setting();
	return ERROR_NONE;
}

static kal_uint32 Custom4(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("%s E\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM4;
	imgsensor.pclk = imgsensor_info.custom4.pclk;
	imgsensor.line_length = imgsensor_info.custom4.linelength;
	imgsensor.frame_length = imgsensor_info.custom4.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom4.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom4_setting();
	return ERROR_NONE;
}

static kal_uint32 Custom5(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("%s E\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM5;
	imgsensor.pclk = imgsensor_info.custom5.pclk;
	imgsensor.line_length = imgsensor_info.custom5.linelength;
	imgsensor.frame_length = imgsensor_info.custom5.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom5.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom5_setting();
	return ERROR_NONE;
}
static kal_uint32
get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	pr_err("E\n");
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

	sensor_resolution->SensorCustom3Width =
		imgsensor_info.custom3.grabwindow_width;
	sensor_resolution->SensorCustom3Height =
		imgsensor_info.custom3.grabwindow_height;

	sensor_resolution->SensorCustom4Width =
		imgsensor_info.custom4.grabwindow_width;
	sensor_resolution->SensorCustom4Height =
		imgsensor_info.custom4.grabwindow_height;

	sensor_resolution->SensorCustom5Width =
		imgsensor_info.custom5.grabwindow_width;
	sensor_resolution->SensorCustom5Height =
		imgsensor_info.custom5.grabwindow_height;

	return ERROR_NONE;
} /* get_resolution */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("scenario_id = %d\n", scenario_id);

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
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
	sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;
	sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame;
	sensor_info->Custom3DelayFrame = imgsensor_info.custom3_delay_frame;
	sensor_info->Custom4DelayFrame = imgsensor_info.custom4_delay_frame;
	sensor_info->Custom5DelayFrame = imgsensor_info.custom5_delay_frame;

	/*Apply calibration status and manufacture info*/
	memcpy(&sensor_info->calibration_status, LYRIQ_OV50A_eeprom_get_calibration_status(), sizeof(mot_calibration_status_t));
	memcpy(&sensor_info->mnf_calibration, LYRIQ_OV50A_eeprom_get_mnf_info(), sizeof(mot_calibration_mnf_t));

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
#if FPT_PDAF_SUPPORT
/*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode*/
	sensor_info->PDAF_Support = PDAF_SUPPORT_CAMSV_QPD;
#else
	sensor_info->PDAF_Support = 0;
#endif
	sensor_info->HDR_Support = HDR_SUPPORT_STAGGER_FDOL;
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->TEMPERATURE_SUPPORT = imgsensor_info.temperature_support;
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

	sensor_info->FrameTimeDelayFrame =
		imgsensor_info.frame_time_delay_frame;

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
	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom2.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		sensor_info->SensorGrabStartX = imgsensor_info.custom3.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom3.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom3.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CUSTOM4:
		sensor_info->SensorGrabStartX = imgsensor_info.custom4.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom4.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom4.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CUSTOM5:
		sensor_info->SensorGrabStartX = imgsensor_info.custom5.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom5.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom5.mipi_data_lp2hs_settle_dc;

		break;
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_err("%s, scenario_id = %d\n", __func__, scenario_id);
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
		Custom1(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		Custom2(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		Custom3(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM4:
		Custom4(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM5:
		Custom5(image_window, sensor_config_data);
		break;
	default:
		pr_err("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}

	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	pr_err("framerate = %d\n ", framerate);
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
	pr_err("enable = %d, framerate = %d\n",
		enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) /*enable auto flicker*/ {
		imgsensor.autoflicker_en = KAL_TRUE;
		pr_err("enable! fps = %d", framerate);
	} else {
		 /*Cancel Auto flick*/
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	pr_err("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

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
	if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
		pr_err(
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
		? (frame_length - imgsensor_info.custom1.framelength) : 0;
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
		? (frame_length - imgsensor_info.custom2.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.custom2.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		frame_length = imgsensor_info.custom3.pclk / framerate * 10
				/ imgsensor_info.custom3.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom3.framelength)
		? (frame_length - imgsensor_info.custom3.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.custom3.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM4:
		frame_length = imgsensor_info.custom4.pclk / framerate * 10
				/ imgsensor_info.custom4.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom4.framelength)
		? (frame_length - imgsensor_info.custom4.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.custom4.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM5:
		frame_length = imgsensor_info.custom5.pclk / framerate * 10
				/ imgsensor_info.custom5.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom5.framelength)
		? (frame_length - imgsensor_info.custom5.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.custom5.framelength
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
		pr_err("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	pr_err("scenario_id = %d\n", scenario_id);

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
	case MSDK_SCENARIO_ID_CUSTOM3:
		*framerate = imgsensor_info.custom3.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM4:
		*framerate = imgsensor_info.custom4.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM5:
		*framerate = imgsensor_info.custom5.max_framerate;
		break;
	default:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	pr_err("set_test_pattern_mode enable: %d", enable);

	if (enable) {
		write_cmos_sensor_8(0x3510, 0x00);
		write_cmos_sensor_8(0x4019, 0x00);
	} else {
		write_cmos_sensor_8(0x3510, 0x01);
		write_cmos_sensor_8(0x4019, 0x40);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

enum {
	SHUTTER_NE_FRM_1 = 0,
	GAIN_NE_FRM_1,
	FRAME_LEN_NE_FRM_1,
	HDR_TYPE_FRM_1,
	SHUTTER_NE_FRM_2,
	GAIN_NE_FRM_2,
	FRAME_LEN_NE_FRM_2,
	HDR_TYPE_FRM_2,
	SHUTTER_SE_FRM_1,
	GAIN_SE_FRM_1,
	SHUTTER_SE_FRM_2,
	GAIN_SE_FRM_2,
	SHUTTER_ME_FRM_1,
	GAIN_ME_FRM_1,
	SHUTTER_ME_FRM_2,
	GAIN_ME_FRM_2,
};

static kal_uint32 seamless_switch(enum MSDK_SCENARIO_ID_ENUM scenario_id, uint32_t *ae_ctrl)
{

	imgsensor.extend_frame_length_en = KAL_FALSE;

	pr_debug("%s %d\n", __func__, scenario_id);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CUSTOM4:
	{
		mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_seamless_switch_group2_start,
                        sizeof(addr_data_pair_seamless_switch_group2_start)/sizeof(kal_uint16));

		control(scenario_id, NULL, NULL);

		if (ae_ctrl) {
			pr_debug("call MSDK_SCENARIO_ID_CUSTOM4 %d %d %d %d %d %d",
				ae_ctrl[SHUTTER_NE_FRM_1], ae_ctrl[SHUTTER_ME_FRM_1],
				ae_ctrl[SHUTTER_SE_FRM_1],
				ae_ctrl[GAIN_NE_FRM_1], ae_ctrl[GAIN_ME_FRM_1],
				ae_ctrl[GAIN_SE_FRM_1]);
			hdr_write_tri_shutter(ae_ctrl[SHUTTER_NE_FRM_1],
				0,
				ae_ctrl[SHUTTER_SE_FRM_1]);
			hdr_write_tri_gain(ae_ctrl[GAIN_NE_FRM_1], 0,
				ae_ctrl[GAIN_SE_FRM_1]);
		}

		mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_seamless_switch_group2_end,
                        sizeof(addr_data_pair_seamless_switch_group2_end)/sizeof(kal_uint16));
	}
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	{
		mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_seamless_switch_group1_start,
                        sizeof(addr_data_pair_seamless_switch_group1_start)/sizeof(kal_uint16));

		control(scenario_id, NULL, NULL);

		if (ae_ctrl) {
			pr_debug("call MSDK_SCENARIO_ID_VIDEO_PREVIEW %d %d",
				ae_ctrl[SHUTTER_NE_FRM_1],
				ae_ctrl[GAIN_NE_FRM_1]);
			set_shutter(ae_ctrl[SHUTTER_NE_FRM_1]);
			set_gain(ae_ctrl[GAIN_NE_FRM_1]);
		}

		mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_seamless_switch_group1_end,
                        sizeof(addr_data_pair_seamless_switch_group1_end)/sizeof(kal_uint16));
	}
		break;
	case MSDK_SCENARIO_ID_CUSTOM5:
	{
		mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_seamless_switch_group3_start,
			sizeof(addr_data_pair_seamless_switch_group3_start)/sizeof(kal_uint16));

		control(scenario_id, NULL, NULL);

		if (ae_ctrl) {
			pr_debug("call MSDK_SCENARIO_ID_CUSTOM5 %d %d",
				ae_ctrl[SHUTTER_NE_FRM_1],
				ae_ctrl[GAIN_NE_FRM_1]);
			set_shutter(ae_ctrl[SHUTTER_NE_FRM_1]);
			set_gain(ae_ctrl[GAIN_NE_FRM_1]);
		}

		mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_seamless_switch_group3_end,
			sizeof(addr_data_pair_seamless_switch_group3_end)/sizeof(kal_uint16));
	}
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	{
		mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_seamless_switch_group1_start,
			sizeof(addr_data_pair_seamless_switch_group1_start)/sizeof(kal_uint16));

		control(scenario_id, NULL, NULL);

		if (ae_ctrl) {
			pr_debug("call MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG %d %d",
				ae_ctrl[SHUTTER_NE_FRM_1],
				ae_ctrl[GAIN_NE_FRM_1]);
			set_shutter(ae_ctrl[SHUTTER_NE_FRM_1]);
			set_gain(ae_ctrl[GAIN_NE_FRM_1]);
		}

		mot_lyriq_ov50a_table_write_cmos_sensor(addr_data_pair_seamless_switch_group1_end,
			sizeof(addr_data_pair_seamless_switch_group1_end)/sizeof(kal_uint16));
	}
		break;
	default:
	{
		pr_debug("%s error! wrong setting in set_seamless_switch = %d",
			__func__,
			scenario_id);

		return 0xff;
	}
	}

	pr_debug("%s success, scenario is switched to %d", __func__, scenario_id);
	return 0;
}


static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				 UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	uint32_t *pAeCtrls;
	uint32_t *pScenarios;
	unsigned long long *feature_data = (unsigned long long *) feature_para;
	/* unsigned long long *feature_return_para
	 *  = (unsigned long long *) feature_para;
	 */
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	struct SENSOR_VC_INFO2_STRUCT *pvcinfo2;
#if FPT_PDAF_SUPPORT
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
#endif
	/* SET_SENSOR_AWB_GAIN *pSetSensorAWB
	 *  = (SET_SENSOR_AWB_GAIN *)feature_para;
	 */
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data
		= (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/*pr_err("feature_id = %d\n", feature_id);*/
	switch (feature_id) {
	case SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS:
		pScenarios = (MUINT32 *)((uintptr_t)(*(feature_data+1)));
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*pScenarios = MSDK_SCENARIO_ID_CUSTOM4;
			break;
		case MSDK_SCENARIO_ID_CUSTOM4:
			*pScenarios = MSDK_SCENARIO_ID_VIDEO_PREVIEW;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*pScenarios = MSDK_SCENARIO_ID_CUSTOM5;
			break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			*pScenarios = MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG;
			break;
		default:
			*pScenarios = 0xff;
			break;
		}
		pr_debug("SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS %d %d\n",
			*feature_data, *pScenarios);
		break;
	case SENSOR_FEATURE_SET_SEAMLESS_EXTEND_FRAME_LENGTH:
		pr_debug("extend_frame_len %d\n", *feature_data);
		extend_frame_length((MUINT32) *feature_data);
		pr_debug("extend_frame_len done %d\n", *feature_data);
		break;
	case SENSOR_FEATURE_SEAMLESS_SWITCH:
	{
		pr_debug("SENSOR_FEATURE_SEAMLESS_SWITCH");
		if ((feature_data + 1) != NULL)
			pAeCtrls = (MUINT32 *)((uintptr_t)(*(feature_data + 1)));
		else
			pr_debug("warning! no ae_ctrl input");

		if (feature_data == NULL) {
			pr_debug("error! input scenario is null!");
			return ERROR_INVALID_SCENARIO_ID;
		}
		pr_debug("call seamless_switch");
		seamless_switch((*feature_data), pAeCtrls);
	}
		break;
	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(feature_data + 2) = 16*BASEGAIN;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
		case MSDK_SCENARIO_ID_CUSTOM4:
		case MSDK_SCENARIO_ID_CUSTOM5:
			*(feature_data + 2) = 64*BASEGAIN;
			break;
		default:
			*(feature_data + 2) = imgsensor_info.max_gain;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_ANA_GAIN_TABLE:
		if ((*(feature_data + 0)) == 0) {
			*(feature_data + 0) =
				sizeof(mot_lyriq_ov50a_ana_gain_table);
		} else {
			memcpy((void *)(uintptr_t) (*(feature_data + 1)),
			(void *)mot_lyriq_ov50a_ana_gain_table,
			sizeof(mot_lyriq_ov50a_ana_gain_table));
		}
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_shutter;
		switch (*feature_data) {
			case MSDK_SCENARIO_ID_CUSTOM1:
                        case MSDK_SCENARIO_ID_CUSTOM5:
                                *(feature_data + 2) = 4;
				break;
			default:
				*(feature_data + 2) = 2;
				break;
		}
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
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom2.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom3.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom4.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom5.pclk;
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
			= 7248000;
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
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom1.framelength << 16)
				+ imgsensor_info.custom1.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom2.framelength << 16)
				+ imgsensor_info.custom2.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom3.framelength << 16)
				+ imgsensor_info.custom3.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom4.framelength << 16)
				+ imgsensor_info.custom4.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom5.framelength << 16)
				+ imgsensor_info.custom5.linelength;
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
		 /* night_mode((BOOL) *feature_data); */
		break;
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
			read_cmos_sensor_8(sensor_reg_data->RegAddr);
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
				(enum MSDK_SCENARIO_ID_ENUM)*feature_data,
				*(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		 get_default_framerate_by_scenario(
				(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
				(MUINT32 *)(uintptr_t)(*(feature_data+1)));
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		pr_err("SENSOR_FEATURE_GET_PDAF_DATA\n");
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
		pr_err("current fps :%d\n", (UINT32)*feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		pr_err("ihdr enable :%d\n", (BOOL)*feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_mode = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
	    pr_err("GET_CROP_INFO scenarioId:%d\n",
			*feature_data_32);
		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)
			(uintptr_t)(*(feature_data+1));

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
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo,
				   (void *)&imgsensor_winsize_info[5],
				   sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy((void *)wininfo,
				   (void *)&imgsensor_winsize_info[6],
			sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			memcpy((void *)wininfo,
				   (void *)&imgsensor_winsize_info[7],
				   sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM4:
			memcpy((void *)wininfo,
				   (void *)&imgsensor_winsize_info[8],
			sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			memcpy((void *)wininfo,
				   (void *)&imgsensor_winsize_info[9],
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
#if FPT_PDAF_SUPPORT
	case SENSOR_FEATURE_GET_PDAF_INFO:
		pr_err("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n",
			(UINT16) *feature_data);
		PDAFinfo =
		  (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
		memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
			sizeof(struct SET_PD_BLOCK_INFO_T));
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		pr_err(
		"SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n",
			(UINT16) *feature_data);
		/*PDAF capacity enable or not, 2p8 only full size support PDAF*/
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CUSTOM2:
		case MSDK_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_SET_PDAF:
		pr_err("PDAF mode :%d\n", *feature_data_16);
		imgsensor.pdaf_mode = *feature_data_16;
		break;
#endif
	case SENSOR_FEATURE_SET_AWB_GAIN:
		break;
	case SENSOR_FEATURE_SET_LSC_TBL:
		break;
	case SENSOR_FEATURE_GET_AWB_REQ_BY_SCENARIO:
		break;

	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		pr_err("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data,
			(UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) (*feature_data),
					(UINT16) (*(feature_data + 1)),
					(BOOL) (*(feature_data + 2)));
		break;
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
		/*
		 * 1, if driver support new sw frame sync
		 * set_shutter_frame_length() support third para auto_extend_en
		 */
		*(feature_data + 1) = 1;
		/* margin info by scenario */
		*(feature_data + 2) = imgsensor_info.margin;
		break;
	case SENSOR_FEATURE_GET_STAGGER_TARGET_SCENARIO:
		if (*feature_data == MSDK_SCENARIO_ID_VIDEO_PREVIEW) {
			switch (*(feature_data + 1)) {
			case HDR_RAW_STAGGER_2EXP:
				*(feature_data + 2) = MSDK_SCENARIO_ID_CUSTOM4;
				break;
			default:
				break;
			}
		} else if (*feature_data == MSDK_SCENARIO_ID_CUSTOM4) {
			switch (*(feature_data + 1)) {
			case HDR_NONE:
				*(feature_data + 2) = MSDK_SCENARIO_ID_VIDEO_PREVIEW;
				break;
			default:
				break;
			}
		}
		pr_debug("SENSOR_FEATURE_GET_STAGGER_TARGET_SCENARIO %d %d %d\n",
							(UINT16) (*feature_data),
				(UINT16) *(feature_data + 1),
				(UINT16) *(feature_data + 2));
		break;
	case SENSOR_FEATURE_GET_STAGGER_MAX_EXP_TIME:
		if (*feature_data == MSDK_SCENARIO_ID_CUSTOM4) {
			// see IMX766 SRM, table 5-22 constraints of COARSE_INTEG_TIME
			switch (*(feature_data + 1)) {
			case VC_STAGGER_NE:
			case VC_STAGGER_ME:
			case VC_STAGGER_SE:
			default:
				*(feature_data + 2) = 16777182;
				break;
			}
		} else {
			*(feature_data + 2) = 0;
		}
		break;
	case SENSOR_FEATURE_SET_DUAL_GAIN://for 2EXP
		pr_debug("SENSOR_FEATURE_SET_DUAL_GAIN LG=%d, SG=%d\n",
				(UINT16) *feature_data, (UINT16) *(feature_data + 1));
		// implement write gain for NE/SE
		hdr_write_tri_gain((UINT16)*feature_data,
				   0,
				   (UINT16)*(feature_data+1));
		break;

	case SENSOR_FEATURE_SET_HDR_SHUTTER:
		pr_err("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1));
		#if 0
		ihdr_write_shutter((UINT16)*feature_data,
				   (UINT16)*(feature_data+1));
		#endif
		hdr_write_tri_shutter((UINT16)*feature_data,
					0,
					(UINT16)*(feature_data+1));
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		pr_err("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		pr_err("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
			case MSDK_SCENARIO_ID_CUSTOM1:
			case MSDK_SCENARIO_ID_CUSTOM5:
				*feature_return_para_32 = 1000;
				break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			case MSDK_SCENARIO_ID_CUSTOM3:
				*feature_return_para_32 = 1256;
				break;
			case MSDK_SCENARIO_ID_CUSTOM4:
			case MSDK_SCENARIO_ID_CUSTOM2:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				*feature_return_para_32 = 1119;
			break;
		}
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.custom1.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.custom2.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.custom3.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.custom4.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.custom5.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.mipi_pixel_rate;
			break;
		}
	}
	break;
	case SENSOR_FEATURE_GET_SENSOR_HDR_CAPACITY:
		/*
		 * HDR_NONE = 0,
		 * HDR_RAW = 1,
		 * HDR_CAMSV = 2,
		 * HDR_RAW_ZHDR = 9,
		 * HDR_MultiCAMSV = 10,
		 * HDR_RAW_STAGGER_2EXP = 0xB,
		 * HDR_RAW_STAGGER_MIN = HDR_RAW_STAGGER_2EXP,
		 * HDR_RAW_STAGGER_3EXP = 0xC,
		 * HDR_RAW_STAGGER_MAX = HDR_RAW_STAGGER_3EXP,
		 */
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = HDR_RAW_STAGGER_2EXP;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		default:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = HDR_NONE;
			break;
		}
		pr_debug(
			"SENSOR_FEATURE_GET_SENSOR_HDR_CAPACITY scenarioId:%llu, HDR:%llu\n",
			*feature_data, *(MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_GET_VC_INFO2:
		pr_info("SENSOR_FEATURE_GET_VC_INFO2 %d\n", (UINT16)*feature_data);
		pvcinfo2 = (struct SENSOR_VC_INFO2_STRUCT *)(uintptr_t)(*(feature_data+1));
		get_vc_info_2(pvcinfo2, *feature_data_32);
		break;
	default:
		break;
	}
	return ERROR_NONE;
} /* feature_control() */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 MOT_LYRIQ_OV50A_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	sensor_func.arch = IMGSENSOR_ARCH_V2;
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	if (imgsensor.psensor_func == NULL)
		imgsensor.psensor_func = &sensor_func;
	return ERROR_NONE;
} /* MOT_LYRIQ_OV50A_MIPI_RAW_SensorInit */
