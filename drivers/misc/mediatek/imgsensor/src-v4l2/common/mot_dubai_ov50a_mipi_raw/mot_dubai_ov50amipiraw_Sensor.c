// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *     MOT_DUBAI_OV50A2Qmipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 * Setting version:
 * ------------
 *   update full pd setting for MOT_DUBAI_OV50A2QEB_03B
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
#include <linux/module.h>
#include "mot_dubai_ov50amipiraw_Sensor.h"
#include "mot_dubai_ov50a_Sensor_setting.h"
#include "mot_dubai_ov50a_ana_gain_table.h"

#define CPHY_3TRIO
#define read_cmos_sensor_8(...) subdrv_i2c_rd_u8(__VA_ARGS__)
#define read_cmos_sensor(...) subdrv_i2c_rd_u16(__VA_ARGS__)
#define write_cmos_sensor_8(...) subdrv_i2c_wr_u8(__VA_ARGS__)
#define write_cmos_sensor(...) subdrv_i2c_wr_u16(__VA_ARGS__)
#define table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u8(__VA_ARGS__)
//#define table_write_cmos_sensor(...) subdrv_i2c_wr_regs_u16(__VA_ARGS__)

#define _I2C_BUF_SIZE 4096
static kal_uint16 _i2c_data[_I2C_BUF_SIZE];
static unsigned int _size_to_write;
static bool _is_seamless = false;
#define PD_PIX_2_EN 0

#define PFX "mot_dubai_ov50a"
static int mot_ov50a_camera_debug = 0;
module_param(mot_ov50a_camera_debug,int, 0644);

static int mot_ov50a_xtalk_en = 1;
module_param(mot_ov50a_xtalk_en,int, 0644);

#define LOG_INF(format, args...)        do { if (mot_ov50a_camera_debug ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define LOG_INF_N(format, args...)     pr_debug(PFX "[%s %d] " format, __func__, __LINE__, ##args)
#define LOG_ERR(format, args...)       pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)

#define MULTI_WRITE 1
#define FPT_PDAF_SUPPORT 1
#define SEAMLESS_ 1
#define SEAMLESS_NO_USE 0
static int remosaic_mode =0;
static int settledebug = 0x10;
module_param(settledebug,int, 0644);
MODULE_PARM_DESC(settledebug, "settledebug");
static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_DUBAI_OV50A_SENSOR_ID,

	.checksum_value = 0x388c7147,//test_Pattern_mode

	.pre = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4168,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1280448000,
	},
	.cap = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4168,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1280448000,
	},
	.normal_video = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4168,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1280448000,
	},
	.hs_video = {
		.pclk = 75000000,
		.linelength = 275,
		.framelength = 2272,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
		.mipi_pixel_rate = 1280448000,
	},
	.slim_video = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4168,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1280448000,
	},
	.custom1 = {
		.pclk = 75000000,
		.linelength = 550,
		.framelength = 6816,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 8192,
		.grabwindow_height = 6144,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 200,
		.mipi_pixel_rate = 1280448000,
	},
	.custom2 = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 2084,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 600,
		.mipi_pixel_rate = 1280448000,
	},
	.custom3 = {
		.pclk = 75000000,
		.linelength = 275,
		.framelength = 1136,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 2400,
		.mipi_pixel_rate = 1280448000,
	},
	.custom4 = {
		.pclk = 75000000,
		.linelength = 600,
		.framelength = 4168,
		.startx = 2048,
		.starty = 1536,
		.grabwindow_width = 4096,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 1280448000,
	},

	.margin = 32,					/* sensor framelength & shutter margin */
	.min_shutter = 16,				/* min shutter */
	.min_gain = BASEGAIN, /*1x gain*/
	.max_gain = 255*BASEGAIN, /*255x gain*/
	.min_gain_iso = 100,
	.gain_step = 16, /*minimum step = 4 in 1x~2x gain*/
	.gain_type = 1,/*to be modify,no gain table for sony*/
	.max_frame_length = 0xffffff,     /* max framelength by sensor register's limitation */
	.ae_shut_delay_frame = 0,		//check
	.ae_sensor_gain_delay_frame = 0,//check
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 9,			//support sensor mode num

	.cap_delay_frame = 2,			//enter capture delay frame num
	.pre_delay_frame = 2,			//enter preview delay frame num
	.video_delay_frame = 2,			//enter video delay frame num
	.hs_video_delay_frame = 2,		//enter high speed video  delay frame num
	.slim_video_delay_frame = 2,	//enter slim video delay frame num
	.custom1_delay_frame = 2,		//enter custom1 delay frame num
	.custom2_delay_frame = 2,		//enter custom2 delay frame num
	.custom3_delay_frame = 2,		//enter custom3 delay frame num
	.custom4_delay_frame = 2,		//enter custom3 delay frame num
	.frame_time_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
#ifdef CPHY_3TRIO
	.mipi_sensor_type = MIPI_CPHY,
#else
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
#endif
	.mipi_settle_delay_mode = 1,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_R,
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
#ifdef CPHY_3TRIO
	.mipi_lane_num = SENSOR_MIPI_3_LANE,//mipi lane num
#else
	.mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
#endif
	.i2c_addr_table = {0x20, 0xff},
	.i2c_speed = 1000,
};


/* Sensor output window information */

static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[9] = {
	/* Preview*/
	{8192, 6144,    0,    0, 8192, 6144, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
	/* capture */
	{8192, 6144,    0,    0, 8192, 6144, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
	/* video */
	{8192, 6144,    0,    0, 8192, 6144, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
	/* hs_video */
	{8192, 6144,    256, 912, 7680, 4320, 1920, 1080,  0,   0, 1920, 1080, 0, 0, 1920, 1080},
	/* slim_video */
	{8192, 6144,    0,    0, 8192, 6144, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
	/* custom1 */
	{8192, 6144,    0,    0, 8192, 6144, 8192, 6144,  0,   0, 8192, 6144, 0, 0, 8192, 6144},
	/* custom2 */
	{8192, 6144,    256, 912, 7680, 4320, 1920, 1080,  0,   0, 1920, 1080, 0, 0, 1920, 1080},
	/* custom3 */
	{8192, 6144,    1536, 1632, 5120, 2880, 1280, 720,  0,   0, 1280, 720, 0, 0, 1280, 720},
	/* custom4 */
	{8192, 6144,    2048,    1536, 4096, 3072, 4096, 3072,  0,   0, 4096, 3072, 0, 0, 4096, 3072},
};


//the index order of VC_STAGGER_NE/ME/SE in array identify the order of readout in MIPI transfer
static struct SENSOR_VC_INFO2_STRUCT SENSOR_VC_INFO2[2] = {
	{
		0x03, 0x0a, 0x00, 0x08, 0x40, 0x00, //preivew
		{
			{VC_STAGGER_NE, 0x00, 0x2b, 0x1000, 0xc00},
			{VC_PDAF_STATS, 0x01, 0x2b, 0x800, 0x300},
#if PD_PIX_2_EN
			{VC_PDAF_STATS_NE_PIX_2, 0x01, 0x2b, 0x800, 0x300},
#endif
		},
		1
	},
	{
		0x03, 0x0a, 0x00, 0x08, 0x40, 0x00, //1080p 60fps
		{
			{VC_STAGGER_NE, 0x00, 0x2b, 0x0780, 0x0438},
			{VC_PDAF_STATS, 0x01, 0x2b, 0x0780, 0x21c},
#if PD_PIX_2_EN
			{VC_PDAF_STATS_NE_PIX_2, 0x01, 0x2b, 0x0780, 0x21c},
#endif
		},
		1
	},
};

#define OV50A_EEPROM_IIC_ADDR 0xA0
#define OV50A_SENSOR_IIC_ADDR 0x20
#define OV50A_XTLK_EEPROM_OFFSET 0x150F
#define OV50A_XTLK_REG_OFFSET 0x71F0
#define OV50A_XTLK_BYTES 3568
static u8 ov50a_xtlk_buf[OV50A_XTLK_BYTES];
static u8 xtalk_ready = 0;

static void get_vc_info_2(struct SENSOR_VC_INFO2_STRUCT *pvcinfo2, kal_uint32 scenario)
{
	switch (scenario) {

	case SENSOR_SCENARIO_ID_CUSTOM2:
		memcpy((void *)pvcinfo2, (void *)&SENSOR_VC_INFO2[1],
			sizeof(struct SENSOR_VC_INFO2_STRUCT));
		break;
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
	case SENSOR_SCENARIO_ID_CUSTOM4:
	default:
		memcpy((void *)pvcinfo2, (void *)&SENSOR_VC_INFO2[0],
			sizeof(struct SENSOR_VC_INFO2_STRUCT));
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
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
		{0, 0}, {64, 228}, {0, 0}, {2048, 1536}
	},  //{0, 1632}
	.iMirrorFlip = 0,
};
#endif

#if MULTI_WRITE
#define I2C_BUFFER_LEN 765	/*trans# max is 255, each 3 bytes*/
#else
#define I2C_BUFFER_LEN 3
#endif

static void set_dummy(struct subdrv_ctx *ctx)
{

       if (!_is_seamless) {
               memset(_i2c_data, 0x0, sizeof(_i2c_data));
               _size_to_write = 0;
               _i2c_data[_size_to_write++] = 0x380e;
               _i2c_data[_size_to_write++] = ctx->frame_length >> 8;
               _i2c_data[_size_to_write++] = 0x380f;
               _i2c_data[_size_to_write++] = ctx->frame_length & 0xFF;
               table_write_cmos_sensor(ctx, _i2c_data,
               _size_to_write);
       } else {
               _i2c_data[_size_to_write++] = 0x3840;
               _i2c_data[_size_to_write++] = ctx->frame_length >> 16;
               _i2c_data[_size_to_write++] = 0x380e;
               _i2c_data[_size_to_write++] = ctx->frame_length >> 8;
               _i2c_data[_size_to_write++] = 0x380f;
               _i2c_data[_size_to_write++] = ctx->frame_length & 0xFF;
       }
}

static void set_max_framerate(struct subdrv_ctx *ctx, UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = ctx->frame_length;

	frame_length = ctx->pclk / framerate * 10 / ctx->line_length;

	ctx->frame_length = (frame_length > ctx->min_frame_length) ?
			frame_length : ctx->min_frame_length;
	ctx->dummy_line = ctx->frame_length -
		ctx->min_frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length) {
		ctx->frame_length = imgsensor_info.max_frame_length;
		ctx->dummy_line = ctx->frame_length - ctx->min_frame_length;
	}
	if (min_framelength_en)
		ctx->min_frame_length = ctx->frame_length;

}

static void set_max_framerate_video(struct subdrv_ctx *ctx, UINT16 framerate,
					kal_bool min_framelength_en)
{
	set_max_framerate(ctx, framerate, min_framelength_en);
	set_dummy(ctx);
}

static kal_uint32 streaming_control(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF_N("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
		write_cmos_sensor_8(ctx, 0x0100, 0X01);
		ctx->is_streaming = true;
	} else {
		write_cmos_sensor_8(ctx, 0x0100, 0x00);
		ctx->is_streaming = false;
	}
	mdelay(3);
	return ERROR_NONE;
}
static void write_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
#if SEAMLESS_
	kal_uint16 realtime_fps = 0;

	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
	if (shutter > ctx->min_frame_length - imgsensor_info.margin)
		ctx->frame_length = shutter + imgsensor_info.margin;
	else
		ctx->frame_length = ctx->min_frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;

	shutter = (shutter < imgsensor_info.min_shutter) ?
				imgsensor_info.min_shutter : shutter;
	shutter = (shutter >
				(imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
				(imgsensor_info.max_frame_length - imgsensor_info.margin) :
				shutter;



	if (ctx->autoflicker_en == KAL_TRUE) {
		realtime_fps = ctx->pclk / ctx->line_length * 10 /
			ctx->frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			realtime_fps = 296;
			set_max_framerate(ctx, realtime_fps, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			realtime_fps = 146;
			set_max_framerate(ctx, realtime_fps, 0);
		} else {
#if SEAMLESS_NO_USE
			if (!_is_seamless)
			{
                               _i2c_data[_size_to_write++] = 0x380e;
                               _i2c_data[_size_to_write++] = ctx->frame_length >> 8;
                               _i2c_data[_size_to_write++] = 0x380f;
                               _i2c_data[_size_to_write++] = ctx->frame_length & 0xFF;

			}
			else
			{
                               //_i2c_data[_size_to_write++] = 0x3840;
                               //_i2c_data[_size_to_write++] = ctx->frame_length >> 16;
                               _i2c_data[_size_to_write++] = 0x380e;
                               _i2c_data[_size_to_write++] = ctx->frame_length >> 8;
                               _i2c_data[_size_to_write++] = 0x380f;
                               _i2c_data[_size_to_write++] = ctx->frame_length & 0xFF;

			}
#endif
		}
	}
#if SEAMLESS_NO_USE
	else {
		if (!_is_seamless) {
			write_cmos_sensor_8(ctx, 0x3840, ctx->frame_length >> 16);
			write_cmos_sensor_8(ctx, 0x380e, ctx->frame_length >> 8);
			write_cmos_sensor_8(ctx, 0x380f, ctx->frame_length & 0xFF);
		}
#if SEAMLESS_NO_USE
		else {
                       _i2c_data[_size_to_write++] = 0x380e;
                       _i2c_data[_size_to_write++] = ctx->frame_length >> 8;
                       _i2c_data[_size_to_write++] = 0x380f;
                       _i2c_data[_size_to_write++] = ctx->frame_length & 0xFF;
		}
#endif
	}
#endif
	/*Warning : shutter must be even. Odd might happen Unexpected Results */
	if (!_is_seamless) {
               memset(_i2c_data, 0x0, sizeof(_i2c_data));
               _size_to_write = 0;
               _i2c_data[_size_to_write++] = 0x3840;
               _i2c_data[_size_to_write++] = ctx->frame_length >> 16;
               _i2c_data[_size_to_write++] = 0x380e;
               _i2c_data[_size_to_write++] = ctx->frame_length >> 8;
               _i2c_data[_size_to_write++] = 0x380f;
               _i2c_data[_size_to_write++] = ctx->frame_length & 0xFF;
               _i2c_data[_size_to_write++] = 0x3500;
               _i2c_data[_size_to_write++] = (shutter >> 16) & 0xFF;
               _i2c_data[_size_to_write++] = 0x3501;
               _i2c_data[_size_to_write++] = (shutter >> 8) & 0xFF;
               _i2c_data[_size_to_write++] = 0x3502;
               _i2c_data[_size_to_write++] = (shutter)  & 0xFF;
               table_write_cmos_sensor(ctx, _i2c_data,
               _size_to_write);

	} else {
               _i2c_data[_size_to_write++] = 0x3840;
	       _i2c_data[_size_to_write++] = ctx->frame_length >> 16;
               _i2c_data[_size_to_write++] = 0x380e;
               _i2c_data[_size_to_write++] = ctx->frame_length >> 8;
               _i2c_data[_size_to_write++] = 0x380f;
               _i2c_data[_size_to_write++] = ctx->frame_length & 0xFF;
               _i2c_data[_size_to_write++] = 0x3500;
               _i2c_data[_size_to_write++] = (shutter >> 16) & 0xFF;
               _i2c_data[_size_to_write++] = 0x3501;
               _i2c_data[_size_to_write++] = (shutter >> 8) & 0xFF;
               _i2c_data[_size_to_write++] = 0x3502;
               _i2c_data[_size_to_write++] = (shutter)  & 0xFF;

	}
	LOG_INF("shutter =%d, framelength =%d, realtime_fps =%d _is_seamless %d\n",
		shutter, ctx->frame_length, realtime_fps, _is_seamless);
#endif
}
//should not be kal_uint16 -- can't reach long exp
static void set_shutter(struct subdrv_ctx *ctx, kal_uint32 shutter)
{
	ctx->shutter = shutter;
	write_shutter(ctx, shutter);
}

static kal_uint16 gain2reg(struct subdrv_ctx *ctx, const kal_uint32 gain)
{
	kal_uint16 iReg = 0x0000;

	//remosaic maxgain = 64*BASEGAIN, other sensor maxgain=255*BASEGAIN
	iReg = gain*256/BASEGAIN;
	return iReg;		/* sensorGlobalGain */
}

static kal_uint32 set_gain(struct subdrv_ctx *ctx, kal_uint32 gain)
{
	kal_uint16 reg_gain;
	kal_uint32 max_gain = imgsensor_info.max_gain;
       if(remosaic_mode == 1)
       {
		max_gain = 63.75*BASEGAIN;
	}
	if (gain < imgsensor_info.min_gain || gain > max_gain) {
		LOG_INF_N("Error gain setting\n");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else if (gain > max_gain)
			gain = max_gain;
	}
	reg_gain = gain2reg(ctx, gain);
	ctx->gain = reg_gain;

#if SEAMLESS_
	if (!_is_seamless) {
               memset(_i2c_data, 0x0, sizeof(_i2c_data));
               _size_to_write = 0;
               _i2c_data[_size_to_write++] = 0x03508;
               _i2c_data[_size_to_write++] =  reg_gain >> 8;
               _i2c_data[_size_to_write++] = 0x03509;
               _i2c_data[_size_to_write++] =  reg_gain & 0xff;
               table_write_cmos_sensor(ctx, _i2c_data,
               _size_to_write);

	} else {
               _i2c_data[_size_to_write++] = 0x03508;
               _i2c_data[_size_to_write++] =  reg_gain >> 8;
               _i2c_data[_size_to_write++] = 0x03509;
               _i2c_data[_size_to_write++] =  reg_gain & 0xff;
	}
#endif
	LOG_INF("gain = %d , reg_gain = 0x%x\n", gain, reg_gain);

	return gain;
}

static void set_frame_length(struct subdrv_ctx *ctx, kal_uint32 frame_length)
{
	if (frame_length > 1)
		ctx->frame_length = frame_length;

	if (ctx->frame_length > imgsensor_info.max_frame_length)
		ctx->frame_length = imgsensor_info.max_frame_length;
	if (ctx->min_frame_length > ctx->frame_length)
		ctx->frame_length = ctx->min_frame_length;

       /* Extend frame length */
       if (!_is_seamless) {
               memset(_i2c_data, 0x0, sizeof(_i2c_data));
               _size_to_write = 0;
       }

       _i2c_data[_size_to_write++] = 0x3840;
       _i2c_data[_size_to_write++] = ctx->frame_length >> 16;
       _i2c_data[_size_to_write++] = 0x380e;
       _i2c_data[_size_to_write++] = ctx->frame_length >> 8;
       _i2c_data[_size_to_write++] = 0x380f;
       _i2c_data[_size_to_write++] = ctx->frame_length & 0xFF;

       if (!_is_seamless) {
               table_write_cmos_sensor(ctx, _i2c_data,
                                       _size_to_write);
       }
	LOG_INF("Framelength: set=%d/input=%d/min=%d\n",
		ctx->frame_length, frame_length, ctx->min_frame_length);
}

/* ITD: Modify Dualcam By Jesse 190924 Start */
static void set_shutter_frame_length(struct subdrv_ctx *ctx, kal_uint32 shutter,
					kal_uint32 target_frame_length)
{

	if (target_frame_length > 1)
		ctx->dummy_line = target_frame_length - ctx->frame_length;
	ctx->frame_length = ctx->frame_length + ctx->dummy_line;
	ctx->min_frame_length = ctx->frame_length;
	set_shutter(ctx, shutter);
}
/* ITD: Modify Dualcam By Jesse 190924 End */

static void ihdr_write_shutter_gain(struct subdrv_ctx *ctx, kal_uint16 le,
				kal_uint16 se, kal_uint16 gain)
{
}

static void night_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
}

static void ov50a_qpd_xtalk_read(struct subdrv_ctx *ctx)
{
	int ret = 0;

	memset(ov50a_xtlk_buf, 0x0, sizeof(ov50a_xtlk_buf));
	LOG_INF("%s: xtalk read start",__func__);
	ret = adaptor_i2c_rd_p8(ctx->i2c_client, OV50A_EEPROM_IIC_ADDR >> 1, OV50A_XTLK_EEPROM_OFFSET, ov50a_xtlk_buf, sizeof(ov50a_xtlk_buf));
	LOG_INF("%s: xtalk read end",__func__);
	if (ret < 0) {
		pr_err("%s: sequential read xtalk failed, ret: %d\n", __func__, ret);
	} else {
		xtalk_ready = 1;
		LOG_INF("%s: sequential read xtalk success\n", __func__);
	}
}

static void ov50a_qpd_calibration_apply(struct subdrv_ctx *ctx)
{
	/*Registers need write:
	    0x71f0 ~ 0x720f      0x20 bytes
	    0x7290 ~ 0x805f      0xDD0 bytes
	*/
	#define XTALK_FIRST_SECTION_BYTES 0x20
	#define XTALK_SECOND_SECTION_START (0x7290)
	#define XTALK_SECOND_SECTION_BYTES (OV50A_XTLK_BYTES-XTALK_FIRST_SECTION_BYTES)
	int ret = 0;

	if (!xtalk_ready) {//Try again if power on reading failed
		LOG_INF("%s: xtalk read retry.", __func__);
		ov50a_qpd_xtalk_read(ctx);
	}

	if (xtalk_ready) {
		ret = adaptor_i2c_wr_seq_p8(ctx->i2c_client, OV50A_SENSOR_IIC_ADDR >> 1, OV50A_XTLK_REG_OFFSET, ov50a_xtlk_buf, XTALK_FIRST_SECTION_BYTES);
		ret |= adaptor_i2c_wr_seq_p8(ctx->i2c_client, OV50A_SENSOR_IIC_ADDR >> 1, XTALK_SECOND_SECTION_START,
		                            &ov50a_xtlk_buf[XTALK_FIRST_SECTION_BYTES], XTALK_SECOND_SECTION_BYTES);
		if (ret < 0) {
			pr_err("%s: Sequential Apply xtalk failed, ret: %d\n", __func__, ret);
		} else {
			write_cmos_sensor_8(ctx, 0x5002, 0x04);
			LOG_INF("%s: Sequential Apply xtalk success\n", __func__);
		}
	} else {
		pr_err("%s: read xtalk failed, ret: %d\n", __func__, ret);
	}

	return;
}

static void sensor_init(struct subdrv_ctx *ctx)
{
	write_cmos_sensor_8(ctx, 0x0103, 0x01);//SW Reset, need delay
	mdelay(1);
	LOG_INF("%s start\n", __func__);

	if (mot_ov50a_xtalk_en) {//Settings that removed xtalk default data.
		table_write_cmos_sensor(ctx,
		    addr_data_pair_init_mot_dubai_ov50a2q,
		    sizeof(addr_data_pair_init_mot_dubai_ov50a2q) / sizeof(kal_uint16));

		LOG_INF("%s: Applying xtalk...", __func__);
		ov50a_qpd_calibration_apply(ctx);
	} else {
		LOG_INF("%s: skip EEPROM xtalk, use default one...", __func__);
		table_write_cmos_sensor(ctx,
		    addr_data_pair_init_mot_dubai_ov50a_20220312,
		    sizeof(addr_data_pair_init_mot_dubai_ov50a_20220312) / sizeof(kal_uint16));
	}
	LOG_INF("%s end\n", __func__);
}

static void preview_setting(struct subdrv_ctx *ctx)
{
	int _length = 0;
	remosaic_mode =0;
	LOG_INF("%s start\n", __func__);
	_length = sizeof(addr_data_pair_preview_mot_dubai_ov50a2q_seamless_switch) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
			addr_data_pair_preview_mot_dubai_ov50a2q,
			_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_preview_mot_dubai_ov50a2q_seamless_switch,
			sizeof(addr_data_pair_preview_mot_dubai_ov50a2q_seamless_switch));
		_size_to_write += _length;
	}
	LOG_INF("%s end\n", __func__);
}

static void capture_setting(struct subdrv_ctx *ctx, kal_uint16 currefps)
{
	int _length = 0;

	LOG_INF("%s start\n", __func__);
	remosaic_mode =0;
	_length = sizeof(addr_data_pair_capture_mot_dubai_ov50a2q) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
			addr_data_pair_capture_mot_dubai_ov50a2q,
			_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);
		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_capture_mot_dubai_ov50a2q,
			sizeof(addr_data_pair_capture_mot_dubai_ov50a2q));
		_size_to_write += _length;
	}
	LOG_INF("%s end\n", __func__);
}

static void normal_video_setting(struct subdrv_ctx *ctx, kal_uint16 currefps)
{
	int _length = 0;

	LOG_INF("%s start\n", __func__);
	remosaic_mode =0;
	_length = sizeof(addr_data_pair_video_mot_dubai_ov50a2q) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
			addr_data_pair_video_mot_dubai_ov50a2q,
			_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_video_mot_dubai_ov50a2q,
			sizeof(addr_data_pair_video_mot_dubai_ov50a2q));
		_size_to_write += _length;
	}
	LOG_INF("%s end\n", __func__);
}

static void hs_video_setting(struct subdrv_ctx *ctx)
{
	int _length = 0;

	LOG_INF("%s start\n", __func__);
	remosaic_mode =0;
	_length = sizeof(addr_data_pair_hs_video_mot_dubai_ov50a2q) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
		addr_data_pair_hs_video_mot_dubai_ov50a2q,
		_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_hs_video_mot_dubai_ov50a2q,
			sizeof(addr_data_pair_hs_video_mot_dubai_ov50a2q));
		_size_to_write += _length;
	}
	LOG_INF("%s end\n", __func__);
}

static void slim_video_setting(struct subdrv_ctx *ctx)
{
	int _length = 0;
	LOG_INF("%s start\n", __func__);
	remosaic_mode =0;
	_length = sizeof(addr_data_pair_slim_video_mot_dubai_ov50a2q) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
		addr_data_pair_slim_video_mot_dubai_ov50a2q,
		_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_slim_video_mot_dubai_ov50a2q,
			sizeof(addr_data_pair_slim_video_mot_dubai_ov50a2q));
		_size_to_write += _length;
	}
	LOG_INF("%s end\n", __func__);
}

/* ITD: Modify Dualcam By Jesse 190924 Start */
static void custom1_setting(struct subdrv_ctx *ctx)
{
	int _length = 0;
	LOG_INF("%s start\n", __func__);
	remosaic_mode =1;
	_length = sizeof(addr_data_pair_custom1) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
		addr_data_pair_custom1,
		_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_custom1,
			sizeof(addr_data_pair_custom1));
		_size_to_write += _length;
	}

	LOG_INF("%s end\n", __func__);
}	/*	custom1_setting  */

static void custom2_setting(struct subdrv_ctx *ctx)
{
	int _length = 0;
	LOG_INF("%s start\n", __func__);
	remosaic_mode =0;
	_length = sizeof(addr_data_pair_custom2) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
		addr_data_pair_custom2,
		_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_custom2,
			sizeof(addr_data_pair_custom2));
		_size_to_write += _length;
	}
	LOG_INF("%s end\n", __func__);
}	/*	custom2_setting  */

static void custom3_setting(struct subdrv_ctx *ctx)
{
	int _length = 0;
	LOG_INF("%s start\n", __func__);
	remosaic_mode =1;
	_length = sizeof(addr_data_pair_custom3) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
		addr_data_pair_custom3,
		_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_custom3,
			sizeof(addr_data_pair_custom3));
		_size_to_write += _length;
	}
	LOG_INF("%s end\n", __func__);
}	/*	custom2_setting  */

static void custom4_setting(struct subdrv_ctx *ctx)
{
	int _length = 0;
	LOG_INF("%s start\n", __func__);
	remosaic_mode =1;
	_length = sizeof(addr_data_pair_custom4) / sizeof(kal_uint16);
	if (!_is_seamless) {
		table_write_cmos_sensor(ctx,
		addr_data_pair_custom4,
		_length);
	} else {
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
			return;
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_custom4,
			sizeof(addr_data_pair_custom4));
		_size_to_write += _length;
	}
	LOG_INF("%s end\n", __func__);
}	/*	custom4_setting  */


/* ITD: Modify Dualcam By Jesse 190924 End */
static kal_uint16 read_cmos_eeprom_8(struct subdrv_ctx *ctx, kal_uint16 addr)
{

	u8 data;

	adaptor_i2c_rd_u8(ctx->i2c_client, 0xA0 >> 1, addr, &data);

	return (u16)data;
}

static kal_uint16 mot_dubai_ov50a_PDC_setting[8*2];
static kal_uint16 mot_dubai_ov50a_PDC_setting_burst[720*2];

static void read_sensor_Cali(struct subdrv_ctx *ctx)
{
	kal_uint16 idx = 0, eeprom_PDC_addr = 0x1638;
	kal_uint16 sensor_PDC_addr1 = 0x5C0E, sensor_PDC_addr2 = 0x5900;

	for (idx = 0; idx < 8; idx++) {
		eeprom_PDC_addr = 0x1638 + idx;
		sensor_PDC_addr1 = 0x5C0E + idx;
		mot_dubai_ov50a_PDC_setting[2 * idx] = sensor_PDC_addr1;
		mot_dubai_ov50a_PDC_setting[2 * idx + 1] =
			read_cmos_eeprom_8(ctx, eeprom_PDC_addr);
	}

	for (idx = 8; idx < 728; idx++) {
		eeprom_PDC_addr = 0x1638 + idx;
		sensor_PDC_addr2 = 0x5900 + idx - 8;
		mot_dubai_ov50a_PDC_setting_burst[2 * (idx-8)] = sensor_PDC_addr2;
		mot_dubai_ov50a_PDC_setting_burst[2 * (idx-8) + 1] =
			read_cmos_eeprom_8(ctx, eeprom_PDC_addr);
	}

	ctx->is_read_preload_eeprom = 1;
}
#if 0
static void write_sensor_PDC(struct subdrv_ctx *ctx)
{
	table_write_cmos_sensor(ctx, mot_dubai_ov50a_PDC_setting,
		sizeof(mot_dubai_ov50a_PDC_setting)/sizeof(kal_uint16));
	table_write_cmos_sensor(ctx, mot_dubai_ov50a_PDC_setting_burst,
		sizeof(mot_dubai_ov50a_PDC_setting_burst)/sizeof(kal_uint16));
}
#endif
static kal_uint32 return_sensor_id(struct subdrv_ctx *ctx)
{
	return ((read_cmos_sensor_8(ctx, 0x300a) << 16) |
		(read_cmos_sensor_8(ctx, 0x300b) << 8) | read_cmos_sensor_8(ctx, 0x300c));
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
		LOG_INF_N("i2c write id: 0x%x, sensor id: 0x%x\n",
			ctx->i2c_write_id, *sensor_id);
		ov50a_qpd_xtalk_read(ctx);
		return ERROR_NONE;
	}
		retry--;
	} while (retry > 0);
	i++;
	retry = 1;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		LOG_INF_N("%s: 0x%x fail\n", __func__, *sensor_id);
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	return ERROR_NONE;
}

static int open(struct subdrv_ctx *ctx)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 1;
	kal_uint32 sensor_id = 0;

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		ctx->i2c_write_id = imgsensor_info.i2c_addr_table[i];
	do {
		sensor_id = return_sensor_id(ctx);
	if (sensor_id == imgsensor_info.sensor_id) {
		LOG_INF_N("2c write id: 0x%x, sensor id: 0x%x\n",
			ctx->i2c_write_id, sensor_id);
		break;
	}
		retry--;
	} while (retry > 0);
	i++;
	if (sensor_id == imgsensor_info.sensor_id)
		break;
	retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id) {
		LOG_INF_N("Open sensor id: 0x%x fail\n", sensor_id);
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	sensor_init(ctx);

	//write_sensor_PDC(ctx);
	ctx->autoflicker_en = KAL_FALSE;
	ctx->sensor_mode = IMGSENSOR_MODE_INIT;
	ctx->shutter = 0x3D0;
	ctx->gain = 0x100;
	ctx->pclk = imgsensor_info.pre.pclk;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	ctx->dummy_pixel = 0;
	ctx->dummy_line = 0;
	//ctx->ihdr_en = 0;
	ctx->test_pattern = KAL_FALSE;
	ctx->current_fps = imgsensor_info.pre.max_framerate;

	return ERROR_NONE;
}

static int close(struct subdrv_ctx *ctx)
{
	_is_seamless = false;
	_size_to_write = 0;
	return ERROR_NONE;
}   /*  close  */

static kal_uint32 preview(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_PREVIEW;
	ctx->pclk = imgsensor_info.pre.pclk;
	//ctx->video_mode = KAL_FALSE;
	ctx->line_length = imgsensor_info.pre.linelength;
	ctx->frame_length = imgsensor_info.pre.framelength;
	ctx->min_frame_length = imgsensor_info.pre.framelength;
	//ctx->autoflicker_en = KAL_FALSE;
	preview_setting(ctx);
	return ERROR_NONE;
}

static kal_uint32 capture(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CAPTURE;
	ctx->pclk = imgsensor_info.cap.pclk;
	//ctx->video_mode = KAL_FALSE;
	ctx->line_length = imgsensor_info.cap.linelength;
	ctx->frame_length = imgsensor_info.cap.framelength;
	ctx->min_frame_length = imgsensor_info.cap.framelength;
	//ctx->autoflicker_en = KAL_FALSE;
	capture_setting(ctx, ctx->current_fps);
	return ERROR_NONE;
} /* capture(ctx) */

static kal_uint32 normal_video(struct subdrv_ctx *ctx,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_VIDEO;
	ctx->pclk = imgsensor_info.normal_video.pclk;
	ctx->line_length = imgsensor_info.normal_video.linelength;
	ctx->frame_length = imgsensor_info.normal_video.framelength;
	ctx->min_frame_length = imgsensor_info.normal_video.framelength;
	//ctx->current_fps = 300;
	//ctx->autoflicker_en = KAL_FALSE;
	normal_video_setting(ctx, ctx->current_fps);
	return ERROR_NONE;
}

static kal_uint32 hs_video(struct subdrv_ctx *ctx,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	ctx->pclk = imgsensor_info.hs_video.pclk;
	//ctx->video_mode = KAL_TRUE;
	ctx->line_length = imgsensor_info.hs_video.linelength;
	ctx->frame_length = imgsensor_info.hs_video.framelength;
	ctx->min_frame_length = imgsensor_info.hs_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	//ctx->current_fps = 300;
	ctx->autoflicker_en = KAL_FALSE;
	hs_video_setting(ctx);
	return ERROR_NONE;
}

static kal_uint32 slim_video(struct subdrv_ctx *ctx,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	ctx->pclk = imgsensor_info.slim_video.pclk;
	//ctx->video_mode = KAL_TRUE;
	ctx->line_length = imgsensor_info.slim_video.linelength;
	ctx->frame_length = imgsensor_info.slim_video.framelength;
	ctx->min_frame_length = imgsensor_info.slim_video.framelength;
	ctx->dummy_line = 0;
	ctx->dummy_pixel = 0;
	//ctx->current_fps = 300;
	ctx->autoflicker_en = KAL_FALSE;
	slim_video_setting(ctx);
	return ERROR_NONE;
}

/* ITD: Modify Dualcam By Jesse 190924 Start */
static kal_uint32 Custom1(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	ctx->pclk = imgsensor_info.custom1.pclk;
	//ctx->video_mode = KAL_FALSE;
	ctx->line_length = imgsensor_info.custom1.linelength;
	ctx->frame_length = imgsensor_info.custom1.framelength;
	ctx->min_frame_length = imgsensor_info.custom1.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	custom1_setting(ctx);
	return ERROR_NONE;
}   /*  Custom1   */

static kal_uint32 Custom2(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	ctx->pclk = imgsensor_info.custom2.pclk;
	//ctx->video_mode = KAL_FALSE;
	ctx->line_length = imgsensor_info.custom2.linelength;
	ctx->frame_length = imgsensor_info.custom2.framelength;
	ctx->min_frame_length = imgsensor_info.custom2.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	custom2_setting(ctx);
	return ERROR_NONE;
}   /*  Custom2   */

static kal_uint32 Custom3(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM3;
	ctx->pclk = imgsensor_info.custom3.pclk;
	//ctx->video_mode = KAL_FALSE;
	ctx->line_length = imgsensor_info.custom3.linelength;
	ctx->frame_length = imgsensor_info.custom3.framelength;
	ctx->min_frame_length = imgsensor_info.custom3.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	custom3_setting(ctx);
	return ERROR_NONE;
}   /*  Custom3   */

static kal_uint32 Custom4(struct subdrv_ctx *ctx, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");
	ctx->sensor_mode = IMGSENSOR_MODE_CUSTOM4;
	ctx->pclk = imgsensor_info.custom4.pclk;
	//ctx->video_mode = KAL_FALSE;
	ctx->line_length = imgsensor_info.custom4.linelength;
	ctx->frame_length = imgsensor_info.custom4.framelength;
	ctx->min_frame_length = imgsensor_info.custom4.framelength;
	ctx->autoflicker_en = KAL_FALSE;
	custom4_setting(ctx);
	return ERROR_NONE;
}   /*  Custom4   */


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
}   /*  get_resolution  */

static int get_info(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
		      MSDK_SENSOR_INFO_STRUCT *sensor_info,
		      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	//sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
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
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM1] =
		imgsensor_info.custom1_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM2] =
		imgsensor_info.custom2_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM3] =
		imgsensor_info.custom3_delay_frame;
	sensor_info->DelayFrame[SENSOR_SCENARIO_ID_CUSTOM4] =
		imgsensor_info.custom4_delay_frame;
	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
/* The frame of setting shutter default 0 for TG int */
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	/* The frame of setting sensor gain */
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

	//sensor_info->HDR_Support = 0; /*0: NO HDR, 1: iHDR, 2:mvHDR, 3:zHDR*/
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;   // 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

	return ERROR_NONE;
}   /*  get_info  */


static int control(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
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
	case SENSOR_SCENARIO_ID_CUSTOM1:
		Custom1(ctx, image_window, sensor_config_data);
	break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		Custom2(ctx, image_window, sensor_config_data);
	break;
	case SENSOR_SCENARIO_ID_CUSTOM3:
		Custom3(ctx, image_window, sensor_config_data);
	break;
	case SENSOR_SCENARIO_ID_CUSTOM4:
		Custom4(ctx, image_window, sensor_config_data);
	break;
	default:
		LOG_ERR("Error ScenarioId setting");
		preview(ctx, image_window, sensor_config_data);
	return ERROR_INVALID_SCENARIO_ID;
	}

	return ERROR_NONE;
}   /* control(ctx) */

static kal_uint32 set_video_mode(struct subdrv_ctx *ctx, UINT16 framerate)
{
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;

	if ((framerate == 300) && (ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 296;
	else if ((framerate == 150) && (ctx->autoflicker_en == KAL_TRUE))
		ctx->current_fps = 146;
	else
		ctx->current_fps = framerate;

	set_max_framerate_video(ctx, ctx->current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(struct subdrv_ctx *ctx, kal_bool enable,
			UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n",
		enable, framerate);

	if (enable) //enable auto flicker
		ctx->autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		ctx->autoflicker_en = KAL_FALSE;

	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(struct subdrv_ctx *ctx,
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frameHeight;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	if (framerate == 0)
		return ERROR_NONE;

	switch (scenario_id) {
	case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
	    frameHeight = imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
		ctx->dummy_line =
			(frameHeight > imgsensor_info.pre.framelength) ?
			(frameHeight - imgsensor_info.pre.framelength):0;
	    ctx->frame_length = imgsensor_info.pre.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
	    frameHeight = imgsensor_info.normal_video.pclk / framerate * 10 /
				imgsensor_info.normal_video.linelength;
		ctx->dummy_line = (frameHeight >
			imgsensor_info.normal_video.framelength) ?
		(frameHeight - imgsensor_info.normal_video.framelength):0;
	    ctx->frame_length = imgsensor_info.normal_video.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
	    frameHeight = imgsensor_info.cap.pclk / framerate * 10 /
			imgsensor_info.cap.linelength;

		ctx->dummy_line =
			(frameHeight > imgsensor_info.cap.framelength) ?
			(frameHeight - imgsensor_info.cap.framelength):0;
	    ctx->frame_length = imgsensor_info.cap.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
	    frameHeight = imgsensor_info.hs_video.pclk / framerate * 10 /
			imgsensor_info.hs_video.linelength;
		ctx->dummy_line =
			(frameHeight > imgsensor_info.hs_video.framelength) ?
			(frameHeight - imgsensor_info.hs_video.framelength):0;
		ctx->frame_length = imgsensor_info.hs_video.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
	    frameHeight = imgsensor_info.slim_video.pclk / framerate * 10 /
			imgsensor_info.slim_video.linelength;
		ctx->dummy_line = (frameHeight >
			imgsensor_info.slim_video.framelength) ?
			(frameHeight - imgsensor_info.slim_video.framelength):0;
	    ctx->frame_length = imgsensor_info.slim_video.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
	    frameHeight = imgsensor_info.custom1.pclk / framerate * 10 /
			imgsensor_info.custom1.linelength;
		ctx->dummy_line = (frameHeight >
			imgsensor_info.custom1.framelength) ?
			(frameHeight - imgsensor_info.custom1.framelength):0;
	    ctx->frame_length = imgsensor_info.custom1.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
	    frameHeight = imgsensor_info.custom2.pclk / framerate * 10 /
			imgsensor_info.custom2.linelength;
		ctx->dummy_line = (frameHeight >
			imgsensor_info.custom2.framelength) ?
			(frameHeight - imgsensor_info.custom2.framelength):0;
	    ctx->frame_length = imgsensor_info.custom2.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;

	case SENSOR_SCENARIO_ID_CUSTOM3:
	    frameHeight = imgsensor_info.custom3.pclk / framerate * 10 /
			imgsensor_info.custom3.linelength;
		ctx->dummy_line = (frameHeight >
			imgsensor_info.custom2.framelength) ?
			(frameHeight - imgsensor_info.custom3.framelength):0;
	    ctx->frame_length = imgsensor_info.custom3.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	case SENSOR_SCENARIO_ID_CUSTOM4:
	    frameHeight = imgsensor_info.custom4.pclk / framerate * 10 /
			imgsensor_info.custom4.linelength;
		ctx->dummy_line = (frameHeight >
			imgsensor_info.custom4.framelength) ?
			(frameHeight - imgsensor_info.custom4.framelength):0;
	    ctx->frame_length = imgsensor_info.custom4.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	default:
	    frameHeight = imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
		ctx->dummy_line = (frameHeight >
			imgsensor_info.pre.framelength) ?
			(frameHeight - imgsensor_info.pre.framelength):0;
	    ctx->frame_length = imgsensor_info.pre.framelength +
			ctx->dummy_line;
	    ctx->min_frame_length = ctx->frame_length;
		if (ctx->frame_length > ctx->shutter)
			set_dummy(ctx);
	break;
	}
	LOG_INF("scenario_id = %d, framerate = %d done\n", scenario_id, framerate);
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(struct subdrv_ctx *ctx,
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MUINT32 *framerate)
{
	LOG_INF_N("scenario_id = %d\n", scenario_id);

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
/* ITD: Modify Dualcam By Jesse 190924 Start */
	case SENSOR_SCENARIO_ID_CUSTOM1:
	    *framerate = imgsensor_info.custom1.max_framerate;
	break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
	    *framerate = imgsensor_info.custom2.max_framerate;
	break;
	case SENSOR_SCENARIO_ID_CUSTOM3:
	    *framerate = imgsensor_info.custom3.max_framerate;
	break;
	case SENSOR_SCENARIO_ID_CUSTOM4:
	    *framerate = imgsensor_info.custom4.max_framerate;
	break;
/* ITD: Modify Dualcam By Jesse 190924 End */
	default:
	break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(struct subdrv_ctx *ctx, kal_bool enable)
{
	LOG_INF("Test_Pattern enable: %d\n", enable);
	if (enable) {
		write_cmos_sensor(ctx,0x3510,0x0000);
		write_cmos_sensor(ctx,0x4019,0x0000);
	} else {
		write_cmos_sensor(ctx,0x3510,0x0100);
		write_cmos_sensor(ctx,0x4019,0x0040);
	}

	ctx->test_pattern = enable;
	return ERROR_NONE;
}

static kal_uint32 get_sensor_temperature(struct subdrv_ctx *ctx)
{
	UINT32 temperature = 0;
	INT32 temperature_convert = 0;
	INT32 read_sensor_temperature = 0;
	/*TEMP_SEN_CTL */
	write_cmos_sensor_8(ctx, 0x4d12, 0x01);
	read_sensor_temperature = read_cmos_sensor_8(ctx, 0x4d13);

	temperature = (read_sensor_temperature << 8) |
		read_sensor_temperature;
	if (temperature < 0xc000)
		temperature_convert = temperature / 256;
	else
		temperature_convert = 192 - temperature / 256;

	return temperature_convert;
}

static kal_uint32 seamless_switch(struct subdrv_ctx *ctx, enum MSDK_SCENARIO_ID_ENUM scenario_id,
	kal_uint32 shutter, kal_uint32 gain,
	kal_uint32 shutter_2ndframe, kal_uint32 gain_2ndframe)
{
#if SEAMLESS_NO_USE
	int k = 0;
#endif
        int _length;
	_is_seamless = true;
	memset(_i2c_data, 0x0, sizeof(_i2c_data));
	_size_to_write = 0;
	if(scenario_id ==SENSOR_SCENARIO_ID_NORMAL_PREVIEW)
	{
	    _length = sizeof(addr_data_pair_seamless_switch_group1_start) / sizeof(kal_uint16);
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_seamless_switch_group1_start,
			sizeof(addr_data_pair_seamless_switch_group1_start));
		_size_to_write += _length;
	}

	if(scenario_id ==SENSOR_SCENARIO_ID_CUSTOM4)
	{
		  _length = sizeof(addr_data_pair_seamless_switch_group2_start) / sizeof(kal_uint16);
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_seamless_switch_group2_start,
			sizeof(addr_data_pair_seamless_switch_group2_start));
		_size_to_write += _length;

	}
	LOG_INF("%s %d, %d, %d, %d, %d sizeof(_i2c_data) %d\n", __func__,
		scenario_id, shutter, gain, shutter_2ndframe, gain_2ndframe, sizeof(_i2c_data));
	control(ctx, scenario_id, NULL, NULL);
	if (shutter != 0)
		set_shutter(ctx, shutter);
	if (gain != 0)
		set_gain(ctx, gain);

	if (shutter_2ndframe != 0)
		set_shutter(ctx, shutter_2ndframe);
	if (gain_2ndframe != 0)
		set_gain(ctx, gain_2ndframe);

	if(scenario_id ==SENSOR_SCENARIO_ID_NORMAL_PREVIEW)
	{
	    _length = sizeof(addr_data_pair_seamless_switch_group1_end) / sizeof(kal_uint16);
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_seamless_switch_group1_end,
			sizeof(addr_data_pair_seamless_switch_group1_end));
		_size_to_write += _length;
	}

	if(scenario_id ==SENSOR_SCENARIO_ID_CUSTOM4)
	{
		  _length = sizeof(addr_data_pair_seamless_switch_group2_end) / sizeof(kal_uint16);
		LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);

		if (_size_to_write + _length > _I2C_BUF_SIZE) {
			LOG_INF("_too much i2c data for fast siwtch %d\n",
				_size_to_write + _length);
		}
		memcpy((void *) (_i2c_data + _size_to_write),
			addr_data_pair_seamless_switch_group2_end,
			sizeof(addr_data_pair_seamless_switch_group2_end));
		_size_to_write += _length;

	}
	LOG_INF("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);
#if SEAMLESS_NO_USE
	for (k = 0; k < _size_to_write; k += 2)
		pr_debug("k = %d, 0x%04x , 0x%02x\n", k,  _i2c_data[k], _i2c_data[k+1]);


#endif
	table_write_cmos_sensor(ctx,
		_i2c_data,
		_size_to_write);

#if SEAMLESS_NO_USE
	LOG_INF("===========================================\n");

	for (k = 0; k < _size_to_write; k += 2)
		LOG_INF("k = %d, 0x%04x , 0x%02x\n",
			k,  _i2c_data[k], read_cmos_sensor_8(ctx, _i2c_data[k]));
#endif

	_is_seamless = false;
	LOG_INF_N("exit\n");
	return ERROR_NONE;
}


static void set_multi_shutter_frame_length(struct subdrv_ctx *ctx,
				kal_uint32 *shutters, kal_uint32 shutter_cnt,
				kal_uint32 frame_length)
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
		write_cmos_sensor_8(ctx, 0x3840, ctx->frame_length >> 16);
		write_cmos_sensor_8(ctx, 0x380e, ctx->frame_length >> 8);
		write_cmos_sensor_8(ctx, 0x380f, ctx->frame_length & 0xFF);
		write_cmos_sensor_8(ctx, 0x3500, (shutters[0] >> 16) & 0xFF);
		write_cmos_sensor_8(ctx, 0x3501, (shutters[0] >> 8) & 0xFF);
		write_cmos_sensor_8(ctx, 0x3502, shutters[0] & 0xFF);
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
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	UINT32 *pAeCtrls = NULL;
	UINT32 *pScenarios = NULL;
	struct SENSOR_VC_INFO2_STRUCT *pvcinfo2;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

#if FPT_PDAF_SUPPORT
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
#endif
	//pr_debug("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_OUTPUT_FORMAT_BY_SCENARIO:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_CUSTOM1:
		case SENSOR_SCENARIO_ID_CUSTOM2:
		case SENSOR_SCENARIO_ID_CUSTOM3:
		case SENSOR_SCENARIO_ID_CUSTOM4:
			*(feature_data + 1)
			= (enum ACDK_SENSOR_OUTPUT_DATA_FORMAT_ENUM)
				imgsensor_info.sensor_output_dataformat;
			break;
		}
	break;
	case SENSOR_FEATURE_GET_ANA_GAIN_TABLE:
	if ((void *)(uintptr_t) (*(feature_data + 1)) == NULL) {
		*(feature_data + 0) =
			sizeof(mot_dubai_ov50a_ana_gain_table);
	} else {
		memcpy((void *)(uintptr_t) (*(feature_data + 1)),
		(void *)mot_dubai_ov50a_ana_gain_table,
		sizeof(mot_dubai_ov50a_ana_gain_table));
	}
		break;
	case SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS:
		pScenarios = (MUINT32 *)((uintptr_t)(*(feature_data+1)));
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
			*pScenarios = SENSOR_SCENARIO_ID_CUSTOM4;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM4:
			*pScenarios = SENSOR_SCENARIO_ID_NORMAL_PREVIEW;
			break;
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_CUSTOM1:
		case SENSOR_SCENARIO_ID_CUSTOM2:
		case SENSOR_SCENARIO_ID_CUSTOM3:
		default:
			*pScenarios = 0xff;
			break;
		}
		LOG_INF_N("SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS %d %d\n",
			*feature_data, *pScenarios);
		break;
	case SENSOR_FEATURE_SEAMLESS_SWITCH:
		pAeCtrls = (MUINT32 *)((uintptr_t)(*(feature_data+1)));
		if (pAeCtrls)
			seamless_switch(ctx, (*feature_data), *pAeCtrls,
				*(pAeCtrls+5), *(pAeCtrls+10), *(pAeCtrls+15));
		else
			seamless_switch(ctx, (*feature_data), 0, 0, 0, 0);
		break;
	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
        	switch (*feature_data) {
        		case SENSOR_SCENARIO_ID_CUSTOM1:
        		case SENSOR_SCENARIO_ID_CUSTOM3:
        		case SENSOR_SCENARIO_ID_CUSTOM4:
				*(feature_data + 1) = imgsensor_info.min_gain;
				*(feature_data + 2) = 63.75 * BASEGAIN;
				break;
			default:
            			*(feature_data + 1) = imgsensor_info.min_gain;
            			*(feature_data + 2) = imgsensor_info.max_gain;
				break;
		}
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_shutter;
		switch (*feature_data) {
			case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
			case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
			case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
			case SENSOR_SCENARIO_ID_SLIM_VIDEO:
			case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
			case SENSOR_SCENARIO_ID_CUSTOM2:
			case SENSOR_SCENARIO_ID_CUSTOM3:
				*(feature_data + 2) = 2;
			case SENSOR_SCENARIO_ID_CUSTOM1:
			case SENSOR_SCENARIO_ID_CUSTOM4:
				*(feature_data + 2) = 4;
			break;
		default:
				*(feature_data + 2) = 2;
			break;
		}
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
		case SENSOR_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.pclk;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom2.pclk;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom3.pclk;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom4.pclk;
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
			= 7248000;
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
		case SENSOR_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom1.framelength << 16)
				+ imgsensor_info.custom1.linelength;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom2.framelength << 16)
				+ imgsensor_info.custom2.linelength;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom3.framelength << 16)
				+ imgsensor_info.custom3.linelength;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom4.framelength << 16)
				+ imgsensor_info.custom4.linelength;
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
	    set_gain(ctx, (UINT32) * feature_data);
	break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
	break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
	break;
	case SENSOR_FEATURE_SET_REGISTER:
		if (sensor_reg_data->RegAddr == 0xff)
			seamless_switch(ctx, sensor_reg_data->RegData, 1920, 369, 960, 369);
		else
			write_cmos_sensor_8(ctx, sensor_reg_data->RegAddr,
							sensor_reg_data->RegData);
	break;
	case SENSOR_FEATURE_GET_REGISTER:
	    sensor_reg_data->RegData =
			read_cmos_sensor_8(ctx, sensor_reg_data->RegAddr);
	break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
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
			*(feature_data+1));
	break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
	    get_default_framerate_by_scenario(ctx,
			(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data+1)));
	break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode(ctx, (BOOL)*feature_data);
	break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
	    *feature_return_para_32 = imgsensor_info.checksum_value;
	    *feature_para_len = 4;
	break;
	case SENSOR_FEATURE_SET_FRAMERATE:
	    ctx->current_fps = *feature_data_32;
		LOG_INF_N("current fps :%d\n", ctx->current_fps);
	break;
	case SENSOR_FEATURE_GET_CROP_INFO:
	    //pr_debug("GET_CROP_INFO scenarioId:%d\n",
		//	*feature_data_32);

	    wininfo = (struct  SENSOR_WINSIZE_INFO_STRUCT *)
			(uintptr_t)(*(feature_data+1));
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
/* ITD: Modify Dualcam By Jesse 190924 Start */
		case SENSOR_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[5],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case SENSOR_SCENARIO_ID_CUSTOM2:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[6],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case SENSOR_SCENARIO_ID_CUSTOM3:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[7],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case SENSOR_SCENARIO_ID_CUSTOM4:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[8],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
/* ITD: Modify Dualcam By Jesse 190924 End */
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		default:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		}
	break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
	    LOG_INF_N("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
	    ihdr_write_shutter_gain(ctx, (UINT16)*feature_data,
			(UINT16)*(feature_data+1),
				(UINT16)*(feature_data+2));
	break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
			switch (*feature_data) {
			case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.cap.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.normal_video.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.hs_video.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_SLIM_VIDEO:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.slim_video.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_CUSTOM1:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom1.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_CUSTOM2:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom2.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_CUSTOM3:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom3.mipi_pixel_rate;
				break;
			case SENSOR_SCENARIO_ID_CUSTOM4:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom4.mipi_pixel_rate;
				break;

			case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.pre.mipi_pixel_rate;
				break;
			}
	break;

	case SENSOR_FEATURE_GET_VC_INFO2:
		LOG_INF_N("SENSOR_FEATURE_GET_VC_INFO2 %d\n",
							(UINT16) (*feature_data));
		pvcinfo2 = (struct SENSOR_VC_INFO2_STRUCT *) (uintptr_t) (*(feature_data + 1));
		get_vc_info_2(pvcinfo2, *feature_data_32);
		break;
#if FPT_PDAF_SUPPORT
/******************** PDAF START ********************/
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		switch (*feature_data) {
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_CUSTOM2:
		case SENSOR_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF_N("SENSOR_FEATURE_GET_PDAF_INFO, scenarioId:%d\n",
			(UINT16) *feature_data);
		PDAFinfo =
		  (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
		memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
			sizeof(struct SET_PD_BLOCK_INFO_T));
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		break;
	case SENSOR_FEATURE_SET_PDAF:
			ctx->pdaf_mode = *feature_data_16;
		break;
/******************** PDAF END ********************/
#endif
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = get_sensor_temperature(ctx);
		*feature_para_len = 4;
	break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		LOG_INF_N("SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME\n");
		set_shutter_frame_length(ctx, (UINT32)*feature_data, (UINT32)*(feature_data+1));
		break;
/* ITD: Modify Dualcam By Jesse 190924 End */
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		streaming_control(ctx, KAL_FALSE);
		break;

	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		if (*feature_data != 0)
			set_shutter(ctx, *feature_data);
		streaming_control(ctx, KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case SENSOR_SCENARIO_ID_CUSTOM1:
		case SENSOR_SCENARIO_ID_CUSTOM4:
			*feature_return_para_32 = 1000;
			break;
		case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		case SENSOR_SCENARIO_ID_CUSTOM3:
			*feature_return_para_32 = 1256;
			break;
		case SENSOR_SCENARIO_ID_CUSTOM2:
		case SENSOR_SCENARIO_ID_NORMAL_PREVIEW:
		case SENSOR_SCENARIO_ID_NORMAL_CAPTURE:
		case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		case SENSOR_SCENARIO_ID_SLIM_VIDEO:

		default:
			*feature_return_para_32 = 1119; /*BINNING_AVERAGED*/
			break;
		}
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_PRELOAD_EEPROM_DATA:
		/*get eeprom preloader data*/
		*feature_return_para_32 = ctx->is_read_preload_eeprom;
		*feature_para_len = 4;
		if (ctx->is_read_preload_eeprom != 1)
			read_sensor_Cali(ctx);
		break;
	case SENSOR_FEATURE_SET_FRAMELENGTH:
		set_frame_length(ctx, (UINT32) (*feature_data));
		break;
	case SENSOR_FEATURE_SET_MULTI_SHUTTER_FRAME_TIME:
		set_multi_shutter_frame_length(ctx, (UINT32 *)(*feature_data),
					(UINT32) (*(feature_data + 1)),
					(UINT32) (*(feature_data + 2)));
	case SENSOR_FEATURE_GET_MAX_EXP_LINE:
		*(feature_data + 2) =
			imgsensor_info.max_frame_length - imgsensor_info.margin;
		break;
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
		/*
		 * 1, if driver support new sw frame sync
		 * set_shutter_frame_length(ctx) support third para auto_extend_en
		 */
		*(feature_data + 1) = 1;
		/* margin info by scenario */
		*(feature_data + 2) = imgsensor_info.margin;
		break;
	default:
	break;
	}

	return ERROR_NONE;
}   /*  feature_control(ctx)  */

#ifdef IMGSENSOR_VC_ROUTING
static struct mtk_mbus_frame_desc_entry frame_desc_prev[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
      {
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x800,
			.vsize = 0x300,
			.user_data_desc = VC_PDAF_STATS,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cap[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
      {
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x800,
			.vsize = 0x300,
			.user_data_desc = VC_PDAF_STATS,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
      {
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x800,
			.vsize = 0x300,
			.user_data_desc = VC_PDAF_STATS,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_hs_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_slim_vid[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
      {
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x800,
			.vsize = 0x300,
			.user_data_desc = VC_PDAF_STATS,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus1[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x2000,
			.vsize = 0x1800,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};
static struct mtk_mbus_frame_desc_entry frame_desc_cus2[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0780,
			.vsize = 0x0438,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
      {
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x780,
			.vsize = 0x21c,
			.user_data_desc = VC_PDAF_STATS,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus3[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x0500,
			.vsize = 0x02D0,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
};

static struct mtk_mbus_frame_desc_entry frame_desc_cus4[] = {
	{
		.bus.csi2 = {
			.channel = 0,
			.data_type = 0x2b,
			.hsize = 0x1000,
			.vsize = 0x0c00,
			.user_data_desc = VC_STAGGER_NE,
		},
	},
      {
		.bus.csi2 = {
			.channel = 1,
			.data_type = 0x2b,
			.hsize = 0x800,
			.vsize = 0x300,
			.user_data_desc = VC_PDAF_STATS,
		},
	},
};

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
	case SENSOR_SCENARIO_ID_NORMAL_VIDEO:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_vid);
		memcpy(fd->entry, frame_desc_vid, sizeof(frame_desc_vid));
		break;
	case SENSOR_SCENARIO_ID_HIGHSPEED_VIDEO:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_hs_vid);
		memcpy(fd->entry, frame_desc_hs_vid, sizeof(frame_desc_hs_vid));
		break;
	case SENSOR_SCENARIO_ID_SLIM_VIDEO:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_slim_vid);
		memcpy(fd->entry, frame_desc_slim_vid, sizeof(frame_desc_slim_vid));
		break;
	case SENSOR_SCENARIO_ID_CUSTOM1:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cus1);
		memcpy(fd->entry, frame_desc_cus1, sizeof(frame_desc_cus1));
		break;
	case SENSOR_SCENARIO_ID_CUSTOM2:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cus2);
		memcpy(fd->entry, frame_desc_cus2, sizeof(frame_desc_cus2));
		break;
	case SENSOR_SCENARIO_ID_CUSTOM3:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cus3);
		memcpy(fd->entry, frame_desc_cus3, sizeof(frame_desc_cus3));
		break;
	case SENSOR_SCENARIO_ID_CUSTOM4:
		fd->type = MTK_MBUS_FRAME_DESC_TYPE_CSI2;
		fd->num_entries = ARRAY_SIZE(frame_desc_cus4);
		memcpy(fd->entry, frame_desc_cus4, sizeof(frame_desc_cus4));
		break;
	default:
		return -1;
	}

	return 0;
}
#endif

static const struct subdrv_ctx defctx = {

	.ana_gain_def = 4 * BASEGAIN,
	.ana_gain_max = 32768, /*32 * 1024  gain*/
	.ana_gain_min = BASEGAIN,
	.ana_gain_step = 4,
	.exposure_def = 0x3D0,
	.exposure_max = 0xffffe9 - 31,
	.exposure_min = 4,
	.exposure_step = 1,
	.frame_time_delay_frame = 2,
	.margin = 31,
	.max_frame_length = 0xffffe9,
	.is_streaming = KAL_FALSE,

	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,
	.gain = 4 * BASEGAIN,
	.dummy_pixel = 0,
	.dummy_line = 0,
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = SENSOR_SCENARIO_ID_NORMAL_PREVIEW,
	//.ihdr_en = 0,
	.i2c_write_id = 0x20,
};

static int init_ctx(struct subdrv_ctx *ctx,
		struct i2c_client *i2c_client, u8 i2c_write_id)
{
	memcpy(ctx, &defctx, sizeof(*ctx));
	ctx->i2c_client = i2c_client;
	ctx->i2c_write_id = i2c_write_id;
	return 0;
}

static int get_temp(struct subdrv_ctx *ctx, int *temp)
{
	*temp = get_sensor_temperature(ctx) * 1000;
	return 0;
}
static int get_csi_param(struct subdrv_ctx *ctx,
	enum SENSOR_SCENARIO_ID_ENUM scenario_id,
	struct mtk_csi_param *csi_param)
{
	csi_param->cphy_settle = settledebug;
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
	.get_temp = get_temp,
	.get_csi_param = get_csi_param,
};

static struct subdrv_pw_seq_entry pw_seq[] = {
			{HW_ID_MCLK, 24, 0},
			{HW_ID_RST, 0, 1},
			{HW_ID_MCLK_DRIVING_CURRENT, 8, 0},
			{HW_ID_DOVDD, 1800000, 0},
			{HW_ID_AVDD, 2800000, 0},
			{HW_ID_DVDD, 1100000, 1},
			{HW_ID_RST, 1, 5},
};

const struct subdrv_entry mot_dubai_ov50a_mipi_raw_entry = {
	.name = "mot_dubai_ov50a_mipi_raw",
	.id = MOT_DUBAI_OV50A_SENSOR_ID,
	.pw_seq = pw_seq,
	.pw_seq_cnt = ARRAY_SIZE(pw_seq),
	.ops = &ops,
};

