 /*
 *
 * Filename:
 * ---------
 *     gc02m1bmipi_Sensor.c
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
 *-----------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
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
#include <linux/module.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define_v4l2.h"
#include "kd_imgsensor_errcode.h"

#include "mot_dubai_gc02m1bmipiraw_Sensor.h"

#include "adaptor-subdrv.h"
#include "adaptor-i2c.h"


#define read_cmos_sensor(...) subdrv_i2c_rd_u8_u8(__VA_ARGS__)
#define write_cmos_sensor(...) subdrv_i2c_wr_u8_u8(__VA_ARGS__)
#define read_cmos_sensor_8(...) subdrv_i2c_rd_u8(__VA_ARGS__)
#define write_cmos_sensor_8(...) subdrv_i2c_wr_u8(__VA_ARGS__)
#define read_cmos_sensor_16(...) subdrv_i2c_rd_u16(__VA_ARGS__)
#define write_cmos_sensor_16(...) subdrv_i2c_wr_u16(__VA_ARGS__)
#define gc02m1b_table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u8_u8(__VA_ARGS__)
//#define gc02m1b_table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u8(__VA_ARGS__)


/************************** Modify Following Strings for Debug **************************/
#define PFX "mot_gc02m1b"
static int mot_gc02m1b_camera_debug = 0;
module_param(mot_gc02m1b_camera_debug,int, 0644);
#define LOG_INF(format, args...)        do { if (mot_gc02m1b_camera_debug ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define LOG_INF_N(format, args...)     pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)
#define LOG_ERR(format, args...)       pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)
/****************************   Modify end    *******************************************/

#define MULTI_WRITE    1
#define EEPROM_READY 0

#define _I2C_BUF_SIZE 256
//static kal_uint16 _i2c_data[_I2C_BUF_SIZE];
//static unsigned int _size_to_write;

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_DUBAI_GC02M1B_SENSOR_ID,
	.checksum_value = 0x5622f670,
	.pre = {
		.pclk = 84000000,
		.linelength = 2192,
		.framelength = 1276,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 67200000,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 84000000,
		.linelength = 2192,
		.framelength = 1276,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 67200000,
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 84000000,
		.linelength = 2192,
		.framelength = 1276,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 67200000,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 84000000,
		.linelength = 2192,
		.framelength = 1276,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 67200000,
		.max_framerate = 300,
	},
	.slim_video = {
		.pclk = 84000000,
		.linelength = 2192,
		.framelength = 1276,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 67200000,
		.max_framerate = 300,
	},

	.min_gain = GC02M1B_SENSOR_GAIN_BASE,
	.max_gain = GC02M1B_SENSOR_GAIN_MAX,
	.min_gain_iso = 100,
	.gain_step = 1,
	.gain_type = 4,
	.margin = 16,
	.min_shutter = 4,
	.max_frame_length = 0x3fff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 5,

	.cap_delay_frame = 2,
	.pre_delay_frame = 2,
	.video_delay_frame = 2,
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,
	.frame_time_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_MONO,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_1_LANE,
	.i2c_addr_table = {0x6e, 0xff},
	.i2c_speed = 400,
};


/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}, /* Preview */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}, /* capture */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}, /* video */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200}, /* HS video */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000, 1600, 1200, 0, 0, 1600, 1200},  /* slim video */
};

static void set_dummy(struct subdrv_ctx *ctx)
{
	write_cmos_sensor(ctx, 0xfe, 0x00);
	write_cmos_sensor(ctx, 0x41, (ctx->frame_length >> 8) & 0x3f);
	write_cmos_sensor(ctx, 0x42, ctx->frame_length & 0xff);
}

static void set_max_framerate(struct subdrv_ctx *ctx,
		UINT16 framerate, kal_bool min_framelength_en)
{
	/*kal_int16 dummy_line;*/
	kal_uint32 frame_length = ctx->frame_length;

	LOG_INF(
		"framerate = %d, min framelength should enable %d\n", framerate,
		min_framelength_en);

	frame_length = ctx->pclk / framerate * 10 / ctx->line_length;
	if (frame_length >= ctx->min_frame_length)
		ctx->frame_length = frame_length;
	else
		ctx->frame_length = ctx->min_frame_length;

	ctx->dummy_line =
			ctx->frame_length - ctx->min_frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length) {
		ctx->frame_length = imgsensor_info.max_frame_length;
		ctx->dummy_line =
			ctx->frame_length - ctx->min_frame_length;
	}
	if (min_framelength_en)
		ctx->min_frame_length = ctx->frame_length;
	set_dummy(ctx);
}	/*	set_max_framerate  */





static void write_shutter(struct subdrv_ctx *ctx, kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;

	if (shutter > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;
	if (shutter < imgsensor_info.min_shutter)
		shutter = imgsensor_info.min_shutter;

	if (ctx->autoflicker_en) {
		realtime_fps =
			ctx->pclk / ctx->line_length
			* 10 / ctx->frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
		else
			set_max_framerate(ctx, realtime_fps, 0);
	} else
		set_max_framerate(ctx, realtime_fps, 0);


	/* Update Shutter*/
	write_cmos_sensor(ctx, 0xfe, 0x00);
	write_cmos_sensor(ctx, 0x03, (shutter >> 8) & 0x3f);
	write_cmos_sensor(ctx, 0x04, shutter  & 0xff);

	LOG_INF("gc02m1b shutter = %d, framelength = %d\n",
		shutter, ctx->frame_length);
}	/*	write_shutter  */


static void set_shutter(struct subdrv_ctx *ctx, kal_uint16 shutter)
{
	ctx->shutter = shutter;

	write_shutter(ctx, shutter);
}	/*	set_shutter */

static void set_shutter_frame_length(struct subdrv_ctx *ctx,
	kal_uint16 shutter, kal_uint16 frame_length)
{	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	ctx->shutter = shutter;

	/* Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - ctx->frame_length;

	ctx->frame_length =
		ctx->frame_length + dummy_line;


	if (shutter > ctx->frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;


	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;


	shutter =
		(shutter < imgsensor_info.min_shutter)
		? imgsensor_info.min_shutter
		: shutter;
	shutter =
		(shutter >
		(imgsensor_info.max_frame_length - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin)
		: shutter;

	if (ctx->autoflicker_en) {
		realtime_fps =
			ctx->pclk / ctx->line_length *
			10 / ctx->frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(ctx, 296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(ctx, 146, 0);
	}

	set_dummy(ctx);

	/* Update Shutter */
	write_cmos_sensor(ctx, 0xfe, 0x00);
	write_cmos_sensor(ctx, 0x03, (shutter >> 8) & 0x3f);
	write_cmos_sensor(ctx, 0x04, shutter  & 0xff);


	LOG_INF("shutter = %d, framelength = %d/%d, dummy_line= %d\n",
		shutter, ctx->frame_length,
		frame_length, dummy_line);

}	/* set_shutter_frame_length */

static kal_uint32 set_gain(struct subdrv_ctx *ctx, kal_uint32 gain)
{
	kal_uint32 temp_gain;
	kal_int16 gain_index;
	kal_uint16 GC02M1B_AGC_Param[GC02M1B_SENSOR_GAIN_MAX_VALID_INDEX][2] = {
		{  1024,  0 },
		{  1536,  1 },
		{  2035,  2 },
		{  2519,  3 },
		{  3165,  4 },
		{  3626,  5 },
		{  4147,  6 },
		{  4593,  7 },
		{  5095,  8 },
		{  5697,  9 },
		{  6270, 10 },
		{  6714, 11 },
		{  7210, 12 },
		{  7686, 13 },
		{  8214, 14 },
		{ 10337, 15 },
	};

	if (gain < imgsensor_info.min_gain || gain > imgsensor_info.max_gain) {
		LOG_INF("Error gain setting");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else if (gain > imgsensor_info.max_gain)
			gain = imgsensor_info.max_gain;
	}

	for (gain_index = GC02M1B_SENSOR_GAIN_MAX_VALID_INDEX - 1; gain_index >= 0; gain_index--)
		if (gain >= GC02M1B_AGC_Param[gain_index][0])
			break;

	write_cmos_sensor(ctx, 0xfe, 0x00);
	write_cmos_sensor(ctx, 0xb6, GC02M1B_AGC_Param[gain_index][1]);
	temp_gain = gain * GC02M1B_SENSOR_DGAIN_BASE / GC02M1B_AGC_Param[gain_index][0];
	write_cmos_sensor(ctx, 0xb1, (temp_gain >> 8) & 0x1f);
	write_cmos_sensor(ctx, 0xb2, temp_gain & 0xff);
	LOG_INF("GC02M1B_AGC_Param[gain_index][1] = 0x%x, temp_gain = 0x%x, gain = %d\n",
		GC02M1B_AGC_Param[gain_index][1], temp_gain, gain);

	return gain;
}
/*
static void ihdr_write_shutter_gain(struct subdrv_ctx *ctx, kal_uint16 le,
		kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le: 0x%x, se: 0x%x, gain: 0x%x\n", le, se, gain);
}
*/
/*static void set_mirror_flip(kal_uint8 image_mirror)
*{
*	LOG_INF("image_mirror = %d\n", image_mirror);
*}
*/

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
 *    None *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void night_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
/*No Need to implement this function*/
}    /*    night_mode    */

kal_uint16 addr_data_pair_init_gc02m1b[] = {
	/*system*/
	0xfc, 0x01,
	0xf4, 0x41,
	0xf5, 0xc0,
	0xf6, 0x44,
	0xf8, 0x38,
	0xf9, 0x82,
	0xfa, 0x00,
	0xfd, 0x80,
	0xfc, 0x81,
	0xfe, 0x03,
	0x01, 0x0b,
	0xf7, 0x01,
	0xfc, 0x80,
	0xfc, 0x80,
	0xfc, 0x80,
	0xfc, 0x8e,

	/*CISCTL*/
	0xfe, 0x00,
	0x87, 0x09,
	0xee, 0x72,
	0xfe, 0x01,
	0x8c, 0x90,
	0xfe, 0x00,
	0x90, 0x00,
	0x03, 0x04,
	0x04, 0x7d,
	0x41, 0x04,
	0x42, 0xf4,
	0x05, 0x04,
	0x06, 0x48,
	0x07, 0x00,
	0x08, 0x18,
	0x9d, 0x18,
	0x09, 0x00,
	0x0a, 0x02,
	0x0d, 0x04,
	0x0e, 0xbc,
	0x17, GC02M1B_MIRROR,
	0x19, 0x04,
	0x24, 0x00,
	0x56, 0x20,
	0x5b, 0x00,
	0x5e, 0x01,

	/*analog Register width*/
	0x21, 0x3c,
	0x44, 0x20,
	0xcc, 0x01,

	/*analog mode*/
	0x1a, 0x04,
	0x1f, 0x11,
	0x27, 0x30,
	0x2b, 0x00,
	0x33, 0x00,
	0x53, 0x90,
	0xe6, 0x50,

	/*analog voltage*/
	0x39, 0x07,
	0x43, 0x04,
	0x46, 0x2a,
	0x7c, 0xa0,
	0xd0, 0xbe,
	0xd1, 0x60,
	0xd2, 0x40,
	0xd3, 0xf3,
	0xde, 0x1d,

	/*analog current*/
	0xcd, 0x05,
	0xce, 0x6f,

	/*CISCTL RESET*/
	0xfc, 0x88,
	0xfe, 0x10,
	0xfe, 0x00,
	0xfc, 0x8e,
	0xfe, 0x00,
	0xfe, 0x00,
	0xfe, 0x00,
	0xfe, 0x00,
	0xfc, 0x88,
	0xfe, 0x10,
	0xfe, 0x00,
	0xfc, 0x8e,
	0xfe, 0x04,
	0xe0, 0x01,
	0xfe, 0x00,

	/*ISP*/
	0xfe, 0x01,
	0x53, 0x44,
	0x87, 0x53,
	0x89, 0x03,

	/*Gain*/
	0xfe, 0x00,
	0xb0, 0x74,
	0xb1, 0x04,
	0xb2, 0x00,
	0xb6, 0x00,
	0xfe, 0x04,
	0xd8, 0x00,
	0xc0, 0x40,
	0xc0, 0x00,
	0xc0, 0x00,
	0xc0, 0x00,
	0xc0, 0x60,
	0xc0, 0x00,
	0xc0, 0xc0,
	0xc0, 0x2a,
	0xc0, 0x80,
	0xc0, 0x00,
	0xc0, 0x00,
	0xc0, 0x40,
	0xc0, 0xa0,
	0xc0, 0x00,
	0xc0, 0x90,
	0xc0, 0x19,
	0xc0, 0xc0,
	0xc0, 0x00,
	0xc0, 0xD0,
	0xc0, 0x2F,
	0xc0, 0xe0,
	0xc0, 0x00,
	0xc0, 0x90,
	0xc0, 0x39,
	0xc0, 0x00,
	0xc0, 0x01,
	0xc0, 0x20,
	0xc0, 0x04,
	0xc0, 0x20,
	0xc0, 0x01,
	0xc0, 0xe0,
	0xc0, 0x0f,
	0xc0, 0x40,
	0xc0, 0x01,
	0xc0, 0xe0,
	0xc0, 0x1a,
	0xc0, 0x60,
	0xc0, 0x01,
	0xc0, 0x20,
	0xc0, 0x25,
	0xc0, 0x80,
	0xc0, 0x01,
	0xc0, 0xa0,
	0xc0, 0x2c,
	0xc0, 0xa0,
	0xc0, 0x01,
	0xc0, 0xe0,
	0xc0, 0x32,
	0xc0, 0xc0,
	0xc0, 0x01,
	0xc0, 0x20,
	0xc0, 0x38,
	0xc0, 0xe0,
	0xc0, 0x01,
	0xc0, 0x60,
	0xc0, 0x3c,
	0xc0, 0x00,
	0xc0, 0x02,
	0xc0, 0xa0,
	0xc0, 0x40,
	0xc0, 0x80,
	0xc0, 0x02,
	0xc0, 0x18,
	0xc0, 0x5c,
	0xfe, 0x00,
	0x9f, 0x10,

	/*BLK*/
	0xfe, 0x00,
	0x26, 0x20,
	0xfe, 0x01,
	0x40, 0x22,
	0x46, 0x7f,
	0x49, 0x0f,
	0x4a, 0xf0,
	0xfe, 0x04,
	0x14, 0x80,
	0x15, 0x80,
	0x16, 0x80,
	0x17, 0x80,

	/*ant _blooming*/
	0xfe, 0x01,
	0x41, 0x20,
	0x4c, 0x00,
	0x4d, 0x0c,
	0x44, 0x08,
	0x48, 0x03,

	/*Window 1600X1200*/
	0xfe, 0x01,
	0x90, 0x01,
	0x91, 0x00,
	0x92, 0x06,
	0x93, 0x00,
	0x94, 0x06,
	0x95, 0x04,
	0x96, 0xb0,
	0x97, 0x06,
	0x98, 0x40,

	/*mipi*/
	0xfe, 0x03,
	0x01, 0x23,
	0x03, 0xce,
	0x04, 0x48,
	0x15, 0x00,
	0x21, 0x10,
	0x22, 0x05,
	0x23, 0x20,
	0x25, 0x20,
	0x26, 0x08,
	0x29, 0x06,
	0x2a, 0x0a,
	0x2b, 0x08,

	/*out*/
	0xfe, 0x01,
	0x8c, 0x10,
	0xfe, 0x00,
	0x3e, 0x00,
};

kal_uint16 addr_data_pair_preview_gc02m1b[] = {
	0xfe, 0x00,
	0x3e, 0x90,
};

kal_uint16 addr_data_pair_capture_gc02m1b[] = {
	0xfe, 0x00,
	0x3e, 0x90,
};

kal_uint16 addr_data_pair_normal_video_gc02m1b[] = {
	0xfe, 0x00,
	0x3e, 0x90,
};

kal_uint16 addr_data_pair_hs_video_gc02m1b[] = {
	0xfe, 0x00,
	0x3e, 0x90,
};

kal_uint16 addr_data_pair_slim_video_gc02m1b[] = {
	0xfe, 0x00,
	0x3e, 0x90,
};

static void sensor_init(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");

	gc02m1b_table_write_cmos_sensor(ctx,
	    addr_data_pair_init_gc02m1b,
		sizeof(addr_data_pair_init_gc02m1b) /
		sizeof(kal_uint16));
	LOG_INF("sensor_init write end\n");
}

static void preview_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	gc02m1b_table_write_cmos_sensor(ctx,
		addr_data_pair_preview_gc02m1b,
		sizeof(addr_data_pair_preview_gc02m1b) /
		sizeof(kal_uint16));
	LOG_INF("X\n");
}

static void capture_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	gc02m1b_table_write_cmos_sensor(ctx,
		addr_data_pair_capture_gc02m1b,
		sizeof(addr_data_pair_capture_gc02m1b) /
		sizeof(kal_uint16));
	LOG_INF("X\n");
}

static void normal_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	gc02m1b_table_write_cmos_sensor(ctx,
	    addr_data_pair_normal_video_gc02m1b,
		sizeof(addr_data_pair_normal_video_gc02m1b) /
		sizeof(kal_uint16));
	LOG_INF("X\n");
}

static void hs_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	gc02m1b_table_write_cmos_sensor(ctx,
		addr_data_pair_hs_video_gc02m1b,
		sizeof(addr_data_pair_hs_video_gc02m1b) /
		sizeof(kal_uint16));
	LOG_INF("X\n");
}

static void slim_video_setting(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
	gc02m1b_table_write_cmos_sensor(ctx,
		addr_data_pair_slim_video_gc02m1b,
		sizeof(addr_data_pair_slim_video_gc02m1b) /
		sizeof(kal_uint16));
	LOG_INF("X\n");
}

static kal_uint32 set_test_pattern_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		write_cmos_sensor(ctx, 0xfe, 0x01);
		write_cmos_sensor(ctx, 0x8c, 0x11);
		write_cmos_sensor(ctx, 0xfe, 0x00);
	} else {
		write_cmos_sensor(ctx, 0xfe, 0x01);
		write_cmos_sensor(ctx, 0x8c, 0x10);
		write_cmos_sensor(ctx, 0xfe, 0x00);
	}
	ctx->test_pattern = enable;
	return ERROR_NONE;
}


#ifdef EEPROM_READY
#define GC02M1B_VAILD_DATA_SIZE 8
#define GRP_FLAG_IN_ARR 0

#define MODULE_GROUP_FLAG_ADDR 0x78
#define MODULE_GROUP1_START_ADDR 0x80
#define MODULE_GROUP2_START_ADDR 0xC0
#define MODULE_DATA_SIZE 16
unsigned char gc02m1b_valid_data[GC02M1B_VAILD_DATA_SIZE] = {0}; //add flag and data
unsigned char gc02m1b_data_eeprom[MODULE_DATA_SIZE + 1] = {0}; //add flag and data


EXPORT_SYMBOL_GPL(gc02m1b_valid_data);
EXPORT_SYMBOL_GPL(gc02m1b_data_eeprom);

static void read_gc02m1b_module_info(struct subdrv_ctx *ctx)
{
	kal_uint16 i, start_addr = 0, otp_grp_flag = 0, otp_count = 0;

	//init setting
	write_cmos_sensor(ctx, 0xfc, 0x01);
	write_cmos_sensor(ctx, 0xf4, 0x41);
	write_cmos_sensor(ctx, 0xf5, 0xc0);
	write_cmos_sensor(ctx, 0xf6, 0x44);
	write_cmos_sensor(ctx, 0xf8, 0x38);
	write_cmos_sensor(ctx, 0xf9, 0x82);
	write_cmos_sensor(ctx, 0xfa, 0x00);
	write_cmos_sensor(ctx, 0xfd, 0x80);
	write_cmos_sensor(ctx, 0xfc, 0x81);
	write_cmos_sensor(ctx, 0xfe, 0x03);
	write_cmos_sensor(ctx, 0x01, 0x0b);
	write_cmos_sensor(ctx, 0xf7, 0x01);
	write_cmos_sensor(ctx, 0xfc, 0x80);
	write_cmos_sensor(ctx, 0xfc, 0x80);
	write_cmos_sensor(ctx, 0xfc, 0x80);
	write_cmos_sensor(ctx, 0xfc, 0x8e);

	write_cmos_sensor(ctx, 0xf3, 0x30); //OTP read init set
	write_cmos_sensor(ctx, 0xfe, 0x02); //page select
	write_cmos_sensor(ctx, 0x17, MODULE_GROUP_FLAG_ADDR); //set addr
	write_cmos_sensor(ctx, 0xf3, 0x34); //otp read pulse
	otp_grp_flag = read_cmos_sensor(ctx, 0x19); //read value
	LOG_INF_N("otp_grp_flag = 0x%x\n", otp_grp_flag);
	gc02m1b_data_eeprom[GRP_FLAG_IN_ARR] = otp_grp_flag;

	if(((otp_grp_flag&0xC0)>>6) == 0x01){ //Bit[7:6] 01:Valid 11:Invalid
		start_addr = MODULE_GROUP1_START_ADDR;
		otp_count = 1;
		LOG_INF_N("otp data is group1\n");
	} else if(((otp_grp_flag&0x30)>>4) == 0x01){ //Bit[5:4] 01:Valid 11:Invalid
		start_addr = MODULE_GROUP2_START_ADDR;
		otp_count = 9;
		LOG_INF_N("otp data is group2\n");
	} else {
		LOG_INF_N("gc02m1 OTP has no otp data\n");
	}

	for(i = 0; i < MODULE_DATA_SIZE; i++){
		write_cmos_sensor(ctx, 0x17, (start_addr+i*0x08));
		write_cmos_sensor(ctx, 0xf3, 0x34);
		gc02m1b_data_eeprom[i+1] = read_cmos_sensor(ctx, 0x19);
		LOG_INF_N("value = 0x%x\n",gc02m1b_data_eeprom[i+1]);
	}
	for(i = 0; i < GC02M1B_VAILD_DATA_SIZE; i++){
		gc02m1b_valid_data[i] = gc02m1b_data_eeprom[i+otp_count];
		LOG_INF_N("addr = 0x%x, value = 0x%x\n", (start_addr+i*0x08), gc02m1b_valid_data[i]);
	}
}
#endif

static kal_uint32 return_sensor_id(struct subdrv_ctx *ctx)
{
              kal_uint32 sensor_id = 0;

              sensor_id = ((read_cmos_sensor(ctx, 0xf0) << 8) | read_cmos_sensor(ctx, 0xf1));

              LOG_INF_N("[%s] sensor_id: 0x%x", __func__, sensor_id);

              return sensor_id;
}

static int get_imgsensor_id(struct subdrv_ctx *ctx, UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {

		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];

		do {
			*sensor_id = return_sensor_id(ctx);
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF_N("i2c write id: 0x%x, sensor id: 0x%x\n", ctx->i2c_write_id, *sensor_id);
#ifdef EEPROM_READY
				read_gc02m1b_module_info(ctx);
#endif
				return ERROR_NONE;
			}
			LOG_INF_N("Read sensor id fail, write id: 0x%x, id: 0x%x\n", ctx->i2c_write_id, *sensor_id);
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
static int open(struct subdrv_ctx *ctx)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
		do {
	    sensor_id = return_sensor_id(ctx);
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF_N("i2c write id: 0x%x, sensor id: 0x%x\n",
					ctx->i2c_write_id, sensor_id);
				break;
			}
			LOG_INF_N("Read sensor id fail, id: 0x%x\n",
				ctx->i2c_write_id);
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
	sensor_init(ctx);

	ctx->autoflicker_en = KAL_FALSE;
	ctx->sensor_mode = IMGSENSOR_MODE_INIT;
	ctx->shutter = 0x3D0;
	ctx->gain = BASEGAIN * 4;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->dummy_pixel = 0;
	ctx->dummy_line = 0;
	ctx->ihdr_mode = 0;
	ctx->test_pattern = KAL_FALSE;
	ctx->current_fps = imgsensor_info.pre.max_framerate;

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
static int close(struct subdrv_ctx *ctx)
{
	LOG_INF("E\n");
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
static kal_uint32 preview(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_PREVIEW;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
    ctx->current_fps = imgsensor_info.pre.max_framerate;
	ctx->autoflicker_en = KAL_FALSE;

	preview_setting(ctx);

	LOG_INF("X\n");
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
static kal_uint32 capture(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CAPTURE;
	ctx->pclk = imgsensor_info.cap.pclk;
	ctx->line_length = imgsensor_info.cap.linelength;
	ctx->frame_length = imgsensor_info.cap.framelength;
	ctx->min_frame_length = imgsensor_info.cap.framelength;
	ctx->current_fps = imgsensor_info.cap.max_framerate;
	ctx->autoflicker_en = KAL_FALSE;
   	LOG_INF("Caputre fps:%d\n", ctx->current_fps);
	capture_setting(ctx);
	LOG_INF("X\n");
	return ERROR_NONE;
} /* capture(ctx) */

static kal_uint32 normal_video(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_VIDEO;
	ctx->pclk = imgsensor_info.normal_video.pclk;
	ctx->line_length = imgsensor_info.normal_video.linelength;
	ctx->frame_length = imgsensor_info.normal_video.framelength;
	ctx->min_frame_length = imgsensor_info.normal_video.framelength;
	ctx->current_fps = 300;
	ctx->autoflicker_en = KAL_FALSE;
	normal_video_setting(ctx);
	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");

	ctx->sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	ctx->pclk = imgsensor_info.hs_video.pclk;
	ctx->line_length = imgsensor_info.hs_video.linelength;
	ctx->frame_length = imgsensor_info.hs_video.framelength;
	ctx->min_frame_length = imgsensor_info.hs_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	ctx->autoflicker_en = KAL_FALSE;
	hs_video_setting(ctx);
	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(struct subdrv_ctx *ctx,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	ctx->pclk = imgsensor_info.slim_video.pclk;
	ctx->line_length = imgsensor_info.slim_video.linelength;
	ctx->frame_length = imgsensor_info.slim_video.framelength;
	ctx->min_frame_length = imgsensor_info.slim_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	ctx->autoflicker_en = KAL_FALSE;
	slim_video_setting(ctx);
	return ERROR_NONE;
}	/*	slim_video	 */


static int get_resolution(struct subdrv_ctx *ctx,
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	int i = 0;

	for (i = SENSOR_SCENARIO_ID_MIN; i < SENSOR_SCENARIO_ID_MAX; i++) {
		if (i < imgsensor_info.sensor_mode_num) {
			sensor_resolution->SensorWidth[i] = imgsensor_winsize_info[i].w2_tg_size;
			sensor_resolution->SensorHeight[i] = imgsensor_winsize_info[i].h2_tg_size;
		} else {
			sensor_resolution->SensorWidth[i] = 0;
			sensor_resolution->SensorHeight[i] = 0;
		}
	}

	return ERROR_NONE;
} /* get_resolution */


static int get_info(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_INFO_STRUCT *sensor_info,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	/* inverse with datasheet*/
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType =
		imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType =
		imgsensor_info.mipi_sensor_type;
	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_PREVIEW] =
		imgsensor_info.pre_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_CAPTURE] =
		imgsensor_info.cap_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_NORMAL_VIDEO] =
		imgsensor_info.video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO] =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_SLIM_VIDEO] =
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

	sensor_info->SensorWidthSampling = 0;	/* 0 is default 1x*/
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x*/
	sensor_info->SensorPacketECCOrder = 1;

	return ERROR_NONE;
}	/*	get_info  */


static int control(struct subdrv_ctx *ctx,
		enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	ctx->current_scenario_id = scenario_id;
	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		preview(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		capture(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		normal_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		hs_video(ctx, image_window, sensor_config_data);
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		slim_video(ctx, image_window, sensor_config_data);
		break;
	default:
		LOG_INF("Error ScenarioId setting, use default mode");
		preview(ctx, image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control(ctx) */

static kal_uint32 set_video_mode(struct subdrv_ctx *ctx, UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	if (framerate == 0) {
		/* Dynamic frame rate*/
		return ERROR_NONE;
	}
	if ((framerate == 300) &&
			(ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 296;
	else if ((framerate == 150) &&
			(ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 146;
	else
		ctx->current_fps = framerate;
	set_max_framerate(ctx, ctx->current_fps, 1);
	set_dummy(ctx);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(struct subdrv_ctx *ctx, kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	if (enable) {/*enable auto flicker*/
		ctx->autoflicker_en = KAL_TRUE;
	} else {/*Cancel Auto flick*/
		ctx->autoflicker_en = KAL_FALSE;
	}
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(struct subdrv_ctx *ctx,
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		frame_length = imgsensor_info.pre.pclk /
			framerate * 10 /
			imgsensor_info.pre.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
		ctx->frame_length =
			imgsensor_info.pre.framelength
				+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk /
			framerate * 10 /
			imgsensor_info.normal_video.linelength;
		ctx->dummy_line =
			(frame_length >
				imgsensor_info.normal_video.framelength) ?
			(frame_length -
				imgsensor_info.normal_video.framelength)
			: 0;
		ctx->frame_length =
			imgsensor_info.normal_video.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		if (ctx->current_fps != imgsensor_info.cap.max_framerate) {
			LOG_INF("Warning: current_fps %d fps is not support",
				framerate);
			LOG_INF("so use cap's setting: %d fps!\n",
				imgsensor_info.cap.max_framerate / 10);
		}
		frame_length = imgsensor_info.cap.pclk /
			framerate * 10 /
			imgsensor_info.cap.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.cap.framelength) ?
			(frame_length - imgsensor_info.cap.framelength) : 0;
		ctx->frame_length =
			imgsensor_info.cap.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk /
			framerate * 10 /
			imgsensor_info.hs_video.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.hs_video.framelength) ?
			(frame_length - imgsensor_info.hs_video.framelength) :
			0;
		ctx->frame_length =
			imgsensor_info.hs_video.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk /
			framerate * 10 /
			imgsensor_info.slim_video.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.slim_video.framelength) ?
			(frame_length - imgsensor_info.slim_video.framelength) :
			0;
		ctx->frame_length =
			imgsensor_info.slim_video.framelength
			+ ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		break;
	default:/*coding with  preview scenario by default*/
		frame_length = imgsensor_info.pre.pclk /
			framerate * 10 /
			imgsensor_info.pre.linelength;
		ctx->dummy_line =
			(frame_length > imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
		ctx->frame_length =
			imgsensor_info.pre.framelength + ctx->dummy_line;
		ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
		LOG_INF("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(struct subdrv_ctx *ctx,
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		*framerate = imgsensor_info.normal_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		*framerate = imgsensor_info.cap.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
		break;
	default:
		break;
	}
	return ERROR_NONE;
}


static void set_multi_shutter_frame_length(struct subdrv_ctx *ctx,
				kal_uint32 *shutters, kal_uint16 shutter_cnt,
				kal_uint16 frame_length)
{
	if (shutter_cnt == 1) {
		ctx->shutter = shutters[0];

		/* if shutter bigger than frame_length, extend frame length first */
		if (shutters[0] > ctx->min_frame_length - imgsensor_info.margin)
			ctx->frame_length = shutters[0] + imgsensor_info.margin;
		else
			ctx->frame_length = ctx->min_frame_length;

		if (frame_length > ctx->frame_length)
			ctx->frame_length = frame_length;
		if (ctx->frame_length > imgsensor_info.max_frame_length)
			ctx->frame_length = imgsensor_info.max_frame_length;


		shutters[0] = (shutters[0] < imgsensor_info.min_shutter)
			? imgsensor_info.min_shutter
			: shutters[0];

		shutters[0] = (shutters[0] > (imgsensor_info.max_frame_length
				      - imgsensor_info.margin))
			? (imgsensor_info.max_frame_length - imgsensor_info.margin)
			: shutters[0];
		/* Update Shutter */
		write_cmos_sensor(ctx, 0xfe, 0x00);
		write_cmos_sensor(ctx, 0x41, (ctx->frame_length >> 8)& 0x3f);
		write_cmos_sensor(ctx, 0x42, ctx->frame_length & 0xFF);;
		write_cmos_sensor(ctx, 0x03, (shutters[0] >> 8) & 0x3f);
		write_cmos_sensor(ctx, 0x04, shutters[0]  & 0xff);
		LOG_INF_N("Exit! shutters =%d, framelength =%d \n",shutters[0],ctx->frame_length);
	}
}

static int feature_control(struct subdrv_ctx *ctx, MSDK_SENSOR_FEATURE_ENUM feature_id,
	UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
//    INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;
//	char *data = (char *)(uintptr_t)(*(feature_data + 1));
//	UINT16 type = (UINT16)(*feature_data);

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

// LOG_INF("feature_id = %d\n", feature_id);

	switch (feature_id) {
	case SENSOR_FEATURE_GET_OUTPUT_FORMAT_BY_SCENARIO:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
			*(feature_data + 1)
			= (enum ACDK_SENSOR_OUTPUT_DATA_FORMAT_ENUM)
				imgsensor_info.sensor_output_dataformat;
			break;
		}
		break;
/*	case SENSOR_FEATURE_GET_ANA_GAIN_TABLE:
		if ((void *)(uintptr_t) (*(feature_data + 1)) == NULL) {
			*(feature_data + 0) =
				sizeof(hi1336_ana_gain_table);
		} else {
			memcpy((void *)(uintptr_t) (*(feature_data + 1)),
			(void *)hi1336_ana_gain_table,
			sizeof(hi1336_ana_gain_table));
		}
		break;
		*/
	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		*(feature_data + 2) = imgsensor_info.max_gain;
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MAX_EXP_LINE:
		*(feature_data + 2) =
			imgsensor_info.max_frame_length - imgsensor_info.margin;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_shutter;
		*(feature_data + 2) = 1;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
					= imgsensor_info.cap.pclk;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.pclk;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
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
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.cap.framelength << 16)
				+ imgsensor_info.cap.linelength;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.normal_video.framelength << 16)
				+ imgsensor_info.normal_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.hs_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.slim_video.framelength << 16)
				+ imgsensor_info.slim_video.linelength;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			 *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = ctx->line_length;
		*feature_return_para_16 = ctx->frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_return_para_32 = ctx->pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode(ctx, (BOOL) * feature_data);
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain(ctx, (UINT32) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor(ctx, sensor_reg_data->RegAddr,
			sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(ctx, sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM */
		/* or just return LENS_DRIVER_ID_DO_NOT_CARE */
		/* if EEPROM does not exist in camera module.*/
		*feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(ctx, *feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(ctx, feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode(ctx, (BOOL)*feature_data_16,
			*(feature_data_16+1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(ctx,
			(enum MSDK_SCENARIO_ID_ENUM)*feature_data,
			*(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(ctx,
			(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode(ctx, (BOOL)*feature_data);
		break;
	/*for factory mode auto testing*/
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		ctx->current_fps = *feature_data_16;
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO:");
		LOG_INF("scenarioId:%d\n", *feature_data_32);
		wininfo =
			(struct SENSOR_WINSIZE_INFO_STRUCT *)
			(uintptr_t)(*(feature_data + 1));
		switch (*feature_data_32) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[3],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[4],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length(ctx, (UINT16) *feature_data,
			(UINT16) *(feature_data + 1));
		break;
/*	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO");
		LOG_INF("scenarioId:%lld\n", *feature_data);
		PDAFinfo =
			(struct SET_PD_BLOCK_INFO_T *)
			(uintptr_t)(*(feature_data + 1));

		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			break;
		}
		break;
*/
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		default:
			*feature_return_para_32 = 1000;
			break;
		}
		LOG_INF("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
		*(feature_data + 1) = 0;
		*(feature_data + 2) = imgsensor_info.margin;
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
		{
			kal_uint32 rate;

			switch (*feature_data) {
			case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
				rate =
				imgsensor_info.cap.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
				rate =
				imgsensor_info.normal_video.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
				rate =
				imgsensor_info.hs_video.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_SLIM_VIDEO:
				rate =
				imgsensor_info.slim_video.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
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
	case SENSOR_FEATURE_PRELOAD_EEPROM_DATA:
		break;
#if 0
	case SENSOR_FEATURE_PRELOAD_EEPROM_DATA:
		/*get eeprom preloader data*/
		*feature_return_para_32 = ctx->is_read_four_cell;
		*feature_para_len = 4;
		if (ctx->is_read_four_cell != 1)
			read_four_cell_from_eeprom(ctx, NULL);
		break;
#endif
	case SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME:
		set_multi_shutter_frame_length(ctx, (UINT32 *)(*feature_data),
					(UINT16) (*(feature_data + 1)),
					(UINT16) (*(feature_data + 2)));
		break;
	default:
		break;
	}
	return ERROR_NONE;
}	/*	feature_control(ctx)  */
#ifdef IMGSENSOR_VC_ROUTING
static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0640,
			.vsize = 0x04b0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0640,
			.vsize = 0x04b0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0640,
			.vsize = 0x04b0,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0640,
			.vsize = 0x04b0,
		},
	},
};
#endif

static int get_frame_desc(struct subdrv_ctx *ctx,
		int scenario_id, struct mtk_mbus_frame_desc *fd)
{
	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_prev);
		memcpy(fd->entry, frame_desc_prev, sizeof(frame_desc_prev));
		break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cap);
		memcpy(fd->entry, frame_desc_cap, sizeof(frame_desc_cap));
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_vid);
		memcpy(fd->entry, frame_desc_vid, sizeof(frame_desc_vid));
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_slim_vid);
		memcpy(fd->entry, frame_desc_slim_vid, sizeof(frame_desc_slim_vid));
		break;
	default:
		return -1;
	}
	return 0;
}



static const struct subdrv_ctx defctx = {

	.ana_gain_def = BASEGAIN * 4,
	.ana_gain_max = BASEGAIN * 16,
	.ana_gain_min = BASEGAIN,
	.ana_gain_step = 32,
	.exposure_def = 0x3D0,
	.exposure_max = 0xfffc - 3,
	.exposure_min = 3,
	.exposure_step = 1,
	.margin = 3,
	.max_frame_length = 0xfffc,

	.mirror = IMAGE_NORMAL, //mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3ED,	/*current shutter*/
	.gain = 0x40,			/*current gain*/
	.dummy_pixel = 0,		/*current dummypixel*/
	.dummy_line = 0,		/*current dummyline*/
	.current_fps = 300,
	.frame_time_delay_frame = 2,
	/*full size current fps : 24fps for PIP, 30fps for Normal or ZSD*/
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,

	.current_scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW,
	.ihdr_mode = 0, /*sensor need support LE, SE with HDR feature*/
	.i2c_write_id = 0x6e,
};

static int init_ctx(struct subdrv_ctx *ctx,
		struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(ctx, &defctx, sizeof(*ctx));
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static struct subdrv_ops ops = {
	.get_id = get_imgsensor_id,
	.init_ctx = init_ctx,
	.open = open,
	.get_info = get_info,
	.get_resolution = get_resolution,
	.control = control,
	.feature_control = feature_control,
	.close = close,
#ifdef IMGSENSOR_VC_ROUTING
	.get_frame_desc = get_frame_desc,
#endif
};

static struct subdrv_pw_seq_entry pw_seq[] = {
	{HW_ID_MCLK, 24, 2},
	{HW_ID_RST, 0, 2},
	{HW_ID_MCLK_DRIVING_CURRENT, 4, 1},
	{HW_ID_DOVDD, 1800000, 1},
	{HW_ID_AVDD, 2800000, 1},
	{HW_ID_RST, 1, 2}
};

const struct subdrv_entry mot_dubai_gc02m1b_mipi_raw_entry = {
	.name = "mot_dubai_gc02m1b_mipi_raw",
	.id = MOT_DUBAI_GC02M1B_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};
