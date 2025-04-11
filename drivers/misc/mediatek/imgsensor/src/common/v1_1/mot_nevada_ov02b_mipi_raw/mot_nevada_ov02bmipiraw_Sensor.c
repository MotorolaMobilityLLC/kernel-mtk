// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *     OV02Bmipi_Sensor.c
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
 *   update full pd setting for OV02BEB_03B
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include "mot_nevada_ov02bmipiraw_Sensor.h"

#define PFX "OV02B_camera_sensor"
#define LOG_INF(format, args...)    \
	pr_err(PFX "[%s] " format, __func__, ##args)
extern void ov02b_read_otp_data(struct imgsensor_struct *pImgsensor);
#define MULTI_WRITE 1
#define FPT_PDAF_SUPPORT 0
#define _I2C_BUF_SIZE 4096
// #define SUPPORT_GET_TEMPERATURE

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
    .sensor_id = MOT_NEVADA_OV02B_SENSOR_ID,

    .checksum_value = 0xb1893b4f, /*checksum value for Camera Auto Test*/

    .pre = {
        .pclk = 18000000,    /*record different mode's pclk*/
        .linelength  = 425,    /*record different mode's linelength*/
        .framelength = 1410,    /*record different mode's framelength*/
        .startx = 0, /*record different mode's startx of grabwindow*/
        .starty = 0,    /*record different mode's starty of grabwindow*/
        .grabwindow_width  = 1600,
        .grabwindow_height = 1200,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
        .mipi_pixel_rate = 72000000,
    },
    .cap = {
        .pclk = 18000000,    /*record different mode's pclk*/
        .linelength  = 425,    /*record different mode's linelength*/
        .framelength = 1410,    /*record different mode's framelength*/
        .startx = 0, /*record different mode's startx of grabwindow*/
        .starty = 0,    /*record different mode's starty of grabwindow*/
        .grabwindow_width  = 1600,
        .grabwindow_height = 1200,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
        .mipi_pixel_rate = 72000000,
    },
    .normal_video = { /* cap*/
        .pclk = 18000000,    /*record different mode's pclk*/
        .linelength  = 425,    /*record different mode's linelength*/
        .framelength = 1410,    /*record different mode's framelength*/
        .startx = 0, /*record different mode's startx of grabwindow*/
        .starty = 0,    /*record different mode's starty of grabwindow*/
        .grabwindow_width  = 1600,
        .grabwindow_height = 900,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
        .mipi_pixel_rate = 72000000,
    },
    .hs_video = {
        .pclk = 18000000,    /*record different mode's pclk*/
        .linelength  = 425,    /*record different mode's linelength*/
        .framelength = 1410,    /*record different mode's framelength*/
        .startx = 0, /*record different mode's startx of grabwindow*/
        .starty = 0,    /*record different mode's starty of grabwindow*/
        .grabwindow_width  = 1600,
        .grabwindow_height = 1200,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
        .mipi_pixel_rate = 72000000,
    },
    .slim_video = {/*pre*/
        .pclk = 18000000,    /*record different mode's pclk*/
        .linelength  = 425,    /*record different mode's linelength*/
        .framelength = 1410,    /*record different mode's framelength*/
        .startx = 0, /*record different mode's startx of grabwindow*/
        .starty = 0,    /*record different mode's starty of grabwindow*/
        .grabwindow_width  = 1600,
        .grabwindow_height = 1200,
        .mipi_data_lp2hs_settle_dc = 85,
        .max_framerate = 300,
        .mipi_pixel_rate = 72000000,
    },

    .margin = 7,      /*sensor framelength & shutter margin*/
    .min_shutter = 4,  /*min shutter*/
    .min_gain =  64,   /*1x gain*/
    .max_gain = 992,   /*15.5x gain*/
    .min_gain_iso = 100,
    .gain_step = 4,
    .gain_type = 1,    /*to be modify,no gain table for sony*/
    .max_frame_length = 0x84c3,
    .ae_shut_delay_frame = 0,
    .ae_sensor_gain_delay_frame = 0,
    .ae_ispGain_delay_frame = 2,   /*isp gain delay frame for AE cycle*/
    .frame_time_delay_frame = 2,
    .ihdr_support = 0,             /*1, support; 0,not support*/
    .ihdr_le_firstline = 0,        /*1,le first ; 0, se first*/
    .temperature_support = 0,/* 1, support; 0,not support */
    .sensor_mode_num = 5,
    .cap_delay_frame = 2,          /*enter capture delay frame num*/
    .pre_delay_frame = 2,          /*enter preview delay frame num*/
    .video_delay_frame = 2,        /*enter video delay frame num*/
    .hs_video_delay_frame = 2, /*enter high speed video  delay frame num*/
    .slim_video_delay_frame = 2,/*enter slim video delay frame num*/
    .isp_driving_current = ISP_DRIVING_8MA, /*mclk driving current*/
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
    .mipi_sensor_type = MIPI_OPHY_CSI2,
    .mipi_settle_delay_mode = 0, //0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
    .mclk = 24,/*mclk value, suggest 24 or 26 for 24Mhz or 26Mhz*/
    .mipi_lane_num = SENSOR_MIPI_1_LANE,/*mipi lane num*/
    .i2c_addr_table = {0x78, 0xff},
    .i2c_speed = 400,
};

static struct imgsensor_struct imgsensor = {
    .mirror = IMAGE_HV_MIRROR,
    .sensor_mode = IMGSENSOR_MODE_INIT,
    .shutter = 0x0410,            /*current shutter*/
    .gain = 0x40,                 /*current gain*/
    .dummy_pixel = 0,
    .dummy_line = 0,
    .current_fps = 300,
    .autoflicker_en = KAL_FALSE,
    .test_pattern = KAL_FALSE,
    .current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
    .ihdr_en = 0,
    .i2c_write_id = 0x20,
    .vblank_convert = 1221, /* vts to vblank*/
    .current_ae_effective_frame = 2,
    .max_shutter = 1523810,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
    {1600, 1200, 0,   0, 1600, 1200, 1600, 1200, 0, 0, 1600, 1200, 0, 0, 1600, 1200}, // Preview
    {1600, 1200, 0,   0, 1600, 1200, 1600, 1200, 0, 0, 1600, 1200, 0, 0, 1600, 1200}, // capture
    {1600, 1200, 0, 150, 1600,  900, 1600,  900, 0, 0, 1600,  900, 0, 0, 1600,  900}, // video
    {1600, 1200, 0,   0, 1600, 1200, 1600, 1200, 0, 0, 1600, 1200, 0, 0, 1600, 1200}, // hight video 120
    {1600, 1200, 0,   0, 1600, 1200, 1600, 1200, 0, 0, 1600, 1200, 0, 0, 1600, 1200}, // slim video
};

#if MULTI_WRITE
#define I2C_BUFFER_LEN 765	/*trans# max is 255, each 3 bytes*/
#else
#define I2C_BUFFER_LEN 3
#endif

static struct IMGSENSOR_I2C_CFG *get_i2c_cfg(void)
{
	return &(((struct IMGSENSOR_SENSOR_INST *)
		  (imgsensor.psensor_func->psensor_inst))->i2c_cfg);
}

#ifdef Table_write
static kal_uint16 OV02B_table_write_cmos_sensor(
					kal_uint16 *para, kal_uint32 len)
{
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	LOG_INF("%s len %d\n", __func__, len);
	tosend = 0;
	IDX = 0;
	while (len > IDX) {
		addr = para[IDX];
		{
			puSendCmd[tosend++] = (char)(addr & 0xFF);
			data = para[IDX + 1];
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;
		}
#if Table_write
		if ((I2C_BUFFER_LEN - tosend) < 2 ||
			len == IDX ||
			addr != addr_last) {
			imgsensor_i2c_write(
				get_i2c_cfg(),
				puSendCmd,
				tosend,
				2,
				imgsensor.i2c_write_id,
				imgsensor_info.i2c_speed);
			tosend = 0;
		}
#else
		imgsensor_i2c_write(
			get_i2c_cfg(),
			puSendCmd,
			3,
			3,
			imgsensor.i2c_write_id,
			IMGSENSOR_I2C_SPEED);
		tosend = 0;

#endif
	}
	return 0;
}
#endif

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[1] = {(char)(addr & 0xFF) };

	imgsensor_i2c_read(
		get_i2c_cfg(),
		pusendcmd,
		1,
		(u8 *)&get_byte,
		1,
		imgsensor.i2c_write_id,
		IMGSENSOR_I2C_SPEED);
	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pusendcmd[2] = {(char)(addr & 0xFF), (char)(para & 0xFF)};
	imgsensor_i2c_write(
		get_i2c_cfg(),
		pusendcmd,
		2,
		2,
		imgsensor.i2c_write_id,
		IMGSENSOR_I2C_SPEED);
}

static void set_dummy(void)
{
    if (imgsensor.frame_length%2 != 0) {
        imgsensor.frame_length = imgsensor.frame_length - imgsensor.frame_length % 2;
    }
    LOG_INF("imgsensor.frame_length = %d\n", imgsensor.frame_length);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0x14, (((imgsensor.frame_length - imgsensor.vblank_convert)) & 0x7F00) >> 8);
    write_cmos_sensor(0x15, ((imgsensor.frame_length - imgsensor.vblank_convert )) & 0xFF);
    write_cmos_sensor(0xfe, 0x02);
}

static void set_mirror_flip(kal_uint8 image_mirror)
{
    LOG_INF("image_mirror = %d\n", image_mirror);
    switch (image_mirror) {
    case IMAGE_NORMAL:
        write_cmos_sensor(0xfd, 0x01);
        write_cmos_sensor(0x12, 0x00);   /* Gr*/
        break;

    case IMAGE_H_MIRROR:
        write_cmos_sensor(0xfd, 0x01);
        write_cmos_sensor(0x12, 0x01);
        break;

    case IMAGE_V_MIRROR:
        write_cmos_sensor(0xfd, 0x01);
        write_cmos_sensor(0x12, 0x02);
        break;

    case IMAGE_HV_MIRROR:
        write_cmos_sensor(0xfd, 0x01);
        write_cmos_sensor(0x12, 0x03);  /*Gb*/
        break;
    default:
        LOG_INF("Error image_mirror setting\n");
    }
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
    kal_uint32 frame_length = imgsensor.frame_length;

    frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;

    spin_lock(&imgsensor_drv_lock);
    imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ?
        frame_length : imgsensor.min_frame_length;
    imgsensor.dummy_line = imgsensor.frame_length -
        imgsensor.min_frame_length;

    if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
        imgsensor.frame_length = imgsensor_info.max_frame_length;
        imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
    }
    if (min_framelength_en)
        imgsensor.min_frame_length = imgsensor.frame_length;
    spin_unlock(&imgsensor_drv_lock);

}

static void set_max_framerate_video(UINT16 framerate, kal_bool min_framelength_en)
{
	set_max_framerate(framerate, min_framelength_en);
	set_dummy();
}




static kal_uint32 streaming_control(kal_bool enable)
{
    LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
               write_cmos_sensor(0xfd, 0x03);
               write_cmos_sensor(0xc2, 0x01);
	} else {
               write_cmos_sensor(0xfd, 0x03);
               write_cmos_sensor(0xc2, 0x00);
	}
	return ERROR_NONE;
}


static void write_shutter(kal_uint32 shutter)
{
	kal_uint32 realtime_fps = 0;
	kal_uint32 v_blank = 0;

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
		(shutter > (imgsensor_info.max_frame_length -
		imgsensor_info.margin)) ? (imgsensor_info.max_frame_length -
		imgsensor_info.margin) : shutter;

	//frame_length and shutter should be an even number.
	shutter = (shutter >> 1) << 1;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;
//auroflicker:need to avoid 15fps and 30 fps
	if (imgsensor.autoflicker_en == KAL_TRUE) {
		realtime_fps = imgsensor.pclk /
			imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			realtime_fps = 296;
	                set_max_framerate(realtime_fps, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			realtime_fps = 146;
	                set_max_framerate(realtime_fps, 0);
		} else {
			imgsensor.frame_length = (imgsensor.frame_length  >> 1) << 1;
			v_blank = ((imgsensor.frame_length- imgsensor.vblank_convert) < imgsensor_info.margin) ? imgsensor_info.margin : (imgsensor.frame_length- imgsensor.vblank_convert);
            write_cmos_sensor(0xfd, 0x01);
			write_cmos_sensor(0x14, (v_blank >> 8) & 0x7F);
			write_cmos_sensor(0x15, v_blank & 0xFF);
			write_cmos_sensor(0xfe, 0x02);	//fresh
		}
	} else {
		imgsensor.frame_length = (imgsensor.frame_length  >> 1) << 1;
		v_blank = ((imgsensor.frame_length- imgsensor.vblank_convert) < imgsensor_info.margin) ? imgsensor_info.margin : (imgsensor.frame_length- imgsensor.vblank_convert);
        write_cmos_sensor(0xfd, 0x01);
		write_cmos_sensor(0x14, (v_blank >> 8) & 0x7F);
		write_cmos_sensor(0x15, v_blank & 0xFF);
		write_cmos_sensor(0xfe, 0x02);	//fresh
	}

		write_cmos_sensor(0xfd, 0x01);
		write_cmos_sensor(0x0e, (shutter >> 8) & 0xFF);
		write_cmos_sensor(0x0f, shutter  & 0xFF);
		write_cmos_sensor(0xfe, 0x02);	//fresh

        LOG_INF("shutter =%d, framelength =%d, v_blank = %d\n", shutter, imgsensor.frame_length, v_blank);
}

static void set_shutter(kal_uint32 shutter)  //should not be kal_uint16 -- can't reach long exp
{
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	write_shutter(shutter);
}


static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint8  iReg;

	if((gain >= 0x40) && (gain <= (15*0x40))) //base gain = 0x40
	{
		iReg = 0x10 * gain/BASEGAIN;        //change mtk gain base to aptina gain base

		if(iReg<=0x10)
		{
			write_cmos_sensor(0xfd, 0x01);
			write_cmos_sensor(0x22, 0x10);//0x23
			write_cmos_sensor(0xfe, 0x02);	//fresh
			LOG_INF("OV02BMIPI_SetGain = 16");
		}
		else if(iReg>= 0xf8)//gpw
		{
			write_cmos_sensor(0xfd, 0x01);
			write_cmos_sensor(0x22,0xf8);
			write_cmos_sensor(0xfe, 0x02);	//fresh
			LOG_INF("OV02BMIPI_SetGain = 160");
		}
		else
		{
			write_cmos_sensor(0xfd, 0x01);
			write_cmos_sensor(0x22, (kal_uint8)iReg);
			write_cmos_sensor(0xfe, 0x02);	//fresh
			LOG_INF("OV02BMIPI_SetGain = %d",iReg);
		}
	}
	else
		LOG_INF("error gain setting");

	return gain;
}

static void set_shutter_frame_length(kal_uint32 shutter,
				     kal_uint32 frame_length,
				     kal_bool auto_extend_en)
{
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;
	unsigned long flags;
	kal_uint32 v_blank = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	if (frame_length > 1)
	    dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;

	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;

	spin_unlock(&imgsensor_drv_lock);

	shutter = (shutter < imgsensor_info.min_shutter) ?
		imgsensor_info.min_shutter : shutter;
	shutter =
		(shutter > (imgsensor_info.max_frame_length -
		imgsensor_info.margin)) ? (imgsensor_info.max_frame_length -
		imgsensor_info.margin) : shutter;

	//frame_length and shutter should be an even number.
	shutter = (shutter >> 1) << 1;
	imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;
//auroflicker:need to avoid 15fps and 30 fps
	if (imgsensor.autoflicker_en == KAL_TRUE) {
		realtime_fps = imgsensor.pclk /
			imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			realtime_fps = 296;
			set_max_framerate(realtime_fps, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			realtime_fps = 146;
			set_max_framerate(realtime_fps, 0);
		} else {
			imgsensor.frame_length = (imgsensor.frame_length  >> 1) << 1;
			v_blank = ((imgsensor.frame_length- imgsensor.vblank_convert) < imgsensor_info.margin) ? imgsensor_info.margin : (imgsensor.frame_length- imgsensor.vblank_convert);
			write_cmos_sensor(0xfd, 0x01);
			write_cmos_sensor(0x14, (v_blank >> 8) & 0x7F);
			write_cmos_sensor(0x15, v_blank & 0xFF);
			write_cmos_sensor(0xfe, 0x02);
		}
	} else {
		imgsensor.frame_length = (imgsensor.frame_length  >> 1) << 1;
		v_blank = ((imgsensor.frame_length- imgsensor.vblank_convert) < imgsensor_info.margin) ? imgsensor_info.margin : (imgsensor.frame_length- imgsensor.vblank_convert);
		write_cmos_sensor(0xfd, 0x01);
		write_cmos_sensor(0x14, (v_blank >> 8) & 0x7F);
		write_cmos_sensor(0x15, v_blank & 0xFF);
		write_cmos_sensor(0xfe, 0x02);
	}

	 write_cmos_sensor(0xfd, 0x01);
	 write_cmos_sensor(0x0e, (shutter >> 8) & 0xFF);
	 write_cmos_sensor(0x0f, shutter  & 0xFF);
	 write_cmos_sensor(0xfe, 0x02);

	LOG_INF("shutter =%d, framelength =%d, realtime_fps =%d, v_blank = %d\n",
		shutter, imgsensor.frame_length, realtime_fps, v_blank);
}				/* set_shutter_frame_length */
static void ihdr_write_shutter_gain(kal_uint16 le,
				kal_uint16 se, kal_uint16 gain)
{
}

static void night_mode(kal_bool enable)
{
}

static void sensor_init(void)
{
	LOG_INF("%s E\n", __func__);
	write_cmos_sensor(0xfc,0x01);
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x24,0x01);
	write_cmos_sensor(0x25,0x04);
	write_cmos_sensor(0x29,0x01);
	write_cmos_sensor(0x2a,0x1b);
	write_cmos_sensor(0x1e,0x17);
	write_cmos_sensor(0x33,0x07);
	write_cmos_sensor(0x35,0x07);
	write_cmos_sensor(0x4a,0x0c);
	write_cmos_sensor(0x3a,0x05);
	write_cmos_sensor(0x3b,0x02);
	write_cmos_sensor(0x3e,0x00);
	write_cmos_sensor(0x46,0x01);
	write_cmos_sensor(0x6d,0x03);
	write_cmos_sensor(0xfd,0x01);
	write_cmos_sensor(0x0e,0x02);
	write_cmos_sensor(0x0f,0x1a);
	write_cmos_sensor(0x12,0x03);
	write_cmos_sensor(0x18,0x00);
	write_cmos_sensor(0x22,0xff);
	write_cmos_sensor(0x23,0x02);
	write_cmos_sensor(0x17,0x2d);
	write_cmos_sensor(0x19,0x20);
	write_cmos_sensor(0x1b,0x07);
	write_cmos_sensor(0x1c,0x04);
	write_cmos_sensor(0x20,0x03);
	write_cmos_sensor(0x30,0x01);
	write_cmos_sensor(0x33,0x01);
	write_cmos_sensor(0x31,0x0a);
	write_cmos_sensor(0x32,0x09);
	write_cmos_sensor(0x38,0x01);
	write_cmos_sensor(0x39,0x01);
	write_cmos_sensor(0x3a,0x01);
	write_cmos_sensor(0x3b,0x01);
	write_cmos_sensor(0x4f,0x04);
	write_cmos_sensor(0x4e,0x05);
	write_cmos_sensor(0x50,0x01);
	write_cmos_sensor(0x35,0x0c);
	write_cmos_sensor(0x45,0x2a);
	write_cmos_sensor(0x46,0x2a);
	write_cmos_sensor(0x47,0x2a);
	write_cmos_sensor(0x48,0x2a);
	write_cmos_sensor(0x4a,0x2c);
	write_cmos_sensor(0x4b,0x2c);
	write_cmos_sensor(0x4c,0x2c);
	write_cmos_sensor(0x4d,0x2c);
	write_cmos_sensor(0x56,0x30);
	write_cmos_sensor(0x57,0x07);
	write_cmos_sensor(0x58,0x27);
	write_cmos_sensor(0x59,0x20);
	write_cmos_sensor(0x5a,0x07);
	write_cmos_sensor(0x5b,0xf4);
	write_cmos_sensor(0x37,0x0a);
	write_cmos_sensor(0x42,0x0e);
	write_cmos_sensor(0x68,0x90);
	write_cmos_sensor(0x69,0xcd);
	write_cmos_sensor(0x6a,0x8f);
	write_cmos_sensor(0x7c,0x0a);
	write_cmos_sensor(0x7d,0x09);
	write_cmos_sensor(0x7e,0x09);
	write_cmos_sensor(0x7f,0x08);
	write_cmos_sensor(0x83,0x14);
	write_cmos_sensor(0x84,0x14);
	write_cmos_sensor(0x86,0x14);
	write_cmos_sensor(0x87,0x07);
	write_cmos_sensor(0x88,0x0f);
	write_cmos_sensor(0x94,0x02);
	write_cmos_sensor(0x98,0xd1);
	write_cmos_sensor(0xfe,0x02);
	write_cmos_sensor(0xfd,0x03);
	write_cmos_sensor(0x97,0x6c);
	write_cmos_sensor(0x98,0x60);
	write_cmos_sensor(0x99,0x60);
	write_cmos_sensor(0x9a,0x6c);
	write_cmos_sensor(0xa1,0x40);
	write_cmos_sensor(0xaf,0x04);
	write_cmos_sensor(0xb1,0x40);
	write_cmos_sensor(0xae,0x0d);
	write_cmos_sensor(0x88,0x5b);
	write_cmos_sensor(0x89,0x7c);
	write_cmos_sensor(0xb4,0x05);
	write_cmos_sensor(0x8c,0x40);
	write_cmos_sensor(0x8e,0x40);
	write_cmos_sensor(0x90,0x40);
	write_cmos_sensor(0x92,0x40);
	write_cmos_sensor(0x9b,0x46);
	write_cmos_sensor(0xac,0x40);
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x5a,0x15);
	write_cmos_sensor(0x74,0x01);

	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x50,0x40);
	write_cmos_sensor(0x52,0xb0);
	write_cmos_sensor(0xfd,0x01);
	write_cmos_sensor(0x03,0x70);
	write_cmos_sensor(0x05,0x10);
	write_cmos_sensor(0x07,0x20);
	write_cmos_sensor(0x09,0xb0);

	write_cmos_sensor(0xfd,0x03);
	write_cmos_sensor(0xc2,0x01);
	write_cmos_sensor(0xfb,0x01);
	write_cmos_sensor(0xfd,0x01);
	write_cmos_sensor(0x14,0x00);
	write_cmos_sensor(0x15,0xbe);
	write_cmos_sensor(0xfe,0x02);
	set_mirror_flip(imgsensor.mirror);
	LOG_INF("%s X\n", __func__);
}

static void preview_setting(void)
{
    LOG_INF("%s start\n", __func__);
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x4f,0x06);
	write_cmos_sensor(0x50,0x40);
	write_cmos_sensor(0x51,0x04);
	write_cmos_sensor(0x52,0xb0);
	write_cmos_sensor(0xfd,0x01);
	write_cmos_sensor(0x02,0x00);
	write_cmos_sensor(0x03,0x70);
	write_cmos_sensor(0x04,0x00);
	write_cmos_sensor(0x05,0x10);
	write_cmos_sensor(0x06,0x03);
	write_cmos_sensor(0x07,0x20);
	write_cmos_sensor(0x08,0x04);
	write_cmos_sensor(0x09,0xb0);
	write_cmos_sensor(0x14,0x00);
	write_cmos_sensor(0x15,0xbe);
	write_cmos_sensor(0xfe,0x02);

    LOG_INF("%s end\n", __func__);
}

static void capture_setting(kal_uint16 currefps)
{
    LOG_INF("%s start currefps = %d\n", __func__, currefps);
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x4f,0x06);
	write_cmos_sensor(0x50,0x40);
	write_cmos_sensor(0x51,0x04);
	write_cmos_sensor(0x52,0xb0);
	write_cmos_sensor(0xfd,0x01);
	write_cmos_sensor(0x02,0x00);
	write_cmos_sensor(0x03,0x70);
	write_cmos_sensor(0x04,0x00);
	write_cmos_sensor(0x05,0x10);
	write_cmos_sensor(0x06,0x03);
	write_cmos_sensor(0x07,0x20);
	write_cmos_sensor(0x08,0x04);
	write_cmos_sensor(0x09,0xb0);
	write_cmos_sensor(0x14,0x00);
	write_cmos_sensor(0x15,0xbe);
	write_cmos_sensor(0xfe,0x02);

    LOG_INF("%s end\n", __func__);
}

static void normal_video_setting(void)
{
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x4f,0x06);
	write_cmos_sensor(0x50,0x40);
	write_cmos_sensor(0x51,0x03);
	write_cmos_sensor(0x52,0x84);
	write_cmos_sensor(0xfd,0x01);
	write_cmos_sensor(0x02,0x00);
	write_cmos_sensor(0x03,0x70);
	write_cmos_sensor(0x04,0x00);
	write_cmos_sensor(0x05,0xA6);
	write_cmos_sensor(0x06,0x03);
	write_cmos_sensor(0x07,0x20);
	write_cmos_sensor(0x08,0x03);
	write_cmos_sensor(0x09,0x84);
	write_cmos_sensor(0x14,0x01);
	write_cmos_sensor(0x15,0xea);
	write_cmos_sensor(0xfe,0x02);
    LOG_INF("%s end\n", __func__);
}

static void hs_video_setting(void)
{
    LOG_INF("%s start\n", __func__);
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x4f,0x06);
	write_cmos_sensor(0x50,0x40);
	write_cmos_sensor(0x51,0x04);
	write_cmos_sensor(0x52,0xb0);
	write_cmos_sensor(0xfd,0x01);
	write_cmos_sensor(0x02,0x00);
	write_cmos_sensor(0x03,0x70);
	write_cmos_sensor(0x04,0x00);
	write_cmos_sensor(0x05,0x10);
	write_cmos_sensor(0x06,0x03);
	write_cmos_sensor(0x07,0x20);
	write_cmos_sensor(0x08,0x04);
	write_cmos_sensor(0x09,0xb0);
	write_cmos_sensor(0x14,0x00);
	write_cmos_sensor(0x15,0xbe);
	write_cmos_sensor(0xfe,0x02);
    LOG_INF("%s end\n", __func__);
}

static void slim_video_setting(void)
{
    LOG_INF("%s start\n", __func__);
	write_cmos_sensor(0xfd,0x00);
	write_cmos_sensor(0x4f,0x06);
	write_cmos_sensor(0x50,0x40);
	write_cmos_sensor(0x51,0x04);
	write_cmos_sensor(0x52,0xb0);
	write_cmos_sensor(0xfd,0x01);
	write_cmos_sensor(0x02,0x00);
	write_cmos_sensor(0x03,0x70);
	write_cmos_sensor(0x04,0x00);
	write_cmos_sensor(0x05,0x10);
	write_cmos_sensor(0x06,0x03);
	write_cmos_sensor(0x07,0x20);
	write_cmos_sensor(0x08,0x04);
	write_cmos_sensor(0x09,0xb0);
	write_cmos_sensor(0x14,0x00);
	write_cmos_sensor(0x15,0xbe);
	write_cmos_sensor(0xfe,0x02);
    LOG_INF("%s end\n", __func__);
}



static kal_uint32 return_sensor_id(void)
{
    write_cmos_sensor(0xfd, 0x00);
	return ((read_cmos_sensor(0x02) << 8) | read_cmos_sensor(0x03));
}

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
				LOG_INF("[%s] i2c write id_v1-1: 0x%x, sensor id: 0x%x\n",
					__func__, imgsensor.i2c_write_id, *sensor_id);
				ov02b_read_otp_data(&imgsensor);
				return ERROR_NONE;
			}
			retry--;
		} while (retry > 0);
		i++;
		retry = 1;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		LOG_INF("%s: 0x%x fail\n", __func__, *sensor_id);
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	return ERROR_NONE;
}

static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;
	LOG_INF("%s +\n", __func__);

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id_v1-1: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
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
		LOG_INF("Open sensor id: 0x%x fail\n", sensor_id);
		return ERROR_SENSOR_CONNECT_FAIL;
	}

	sensor_init();

	// write_sensor_PDC();
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
	imgsensor.ihdr_en = 0;
	imgsensor.test_pattern = 0;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static kal_uint32 close(void)
{
	return ERROR_NONE;
}   /*  close  */

static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s E\n", __func__);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
        imgsensor.vblank_convert = 1220; //for 1632x1224 30fps
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	//imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
	LOG_INF("%s X\n", __func__);
	return ERROR_NONE;
}

static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s E\n", __func__);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	imgsensor.vblank_convert = 1220; //for 3264x2448
	imgsensor.pclk = imgsensor_info.cap.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	//imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);
	LOG_INF("%s X\n", __func__);
	return ERROR_NONE;
} /* capture() */

static kal_uint32 normal_video(
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s E\n", __func__);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.vblank_convert = 1220; //for 3264x2448
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	//imgsensor.current_fps = 300;
	//imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting();
	LOG_INF("%s X\n", __func__);
	return ERROR_NONE;
}

static kal_uint32 hs_video(
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s E\n", __func__);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.vblank_convert = 1220;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	LOG_INF("%s X\n", __func__);
	return ERROR_NONE;
}

static kal_uint32 slim_video(
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("%s E\n", __func__);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.vblank_convert = 1220;//776;//1280x720_90fps
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	LOG_INF("%s X\n", __func__);
	return ERROR_NONE;
}

/* ITD: Modify Dualcam By Jesse 190924 End */

static kal_uint32 get_resolution(
		MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
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


/* ITD: Modify Dualcam By Jesse 190924 End */

	return ERROR_NONE;
}   /*  get_resolution  */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
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
	sensor_info->PDAF_Support = 2;
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

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;   // 0 is default 1x
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


/* ITD: Modify Dualcam By Jesse 190924 End */
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
	break;
	}

	return ERROR_NONE;
}   /*  get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
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

/* ITD: Modify Dualcam By Jesse 190924 End */
	default:
		LOG_INF("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
	return ERROR_INVALID_SCENARIO_ID;
	}

	return ERROR_NONE;
}   /* control() */

static kal_uint32 set_video_mode(UINT16 framerate)
{
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;

	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);

	set_max_framerate_video(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable,
			UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n",
		enable, framerate);

	spin_lock(&imgsensor_drv_lock);
	if (enable) //enable auto flicker
		imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frameHeight;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	if (framerate == 0)
		return ERROR_NONE;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	    frameHeight = imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frameHeight > imgsensor_info.pre.framelength) ?
			(frameHeight - imgsensor_info.pre.framelength):0;
	    imgsensor.frame_length = imgsensor_info.pre.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	    frameHeight = imgsensor_info.normal_video.pclk / framerate * 10 /
				imgsensor_info.normal_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.normal_video.framelength) ?
		(frameHeight - imgsensor_info.normal_video.framelength):0;
	    imgsensor.frame_length = imgsensor_info.normal_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	    frameHeight = imgsensor_info.cap.pclk / framerate * 10 /
			imgsensor_info.cap.linelength;
	    spin_lock(&imgsensor_drv_lock);

		imgsensor.dummy_line =
			(frameHeight > imgsensor_info.cap.framelength) ?
			(frameHeight - imgsensor_info.cap.framelength):0;
	    imgsensor.frame_length = imgsensor_info.cap.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
	    frameHeight = imgsensor_info.hs_video.pclk / framerate * 10 /
			imgsensor_info.hs_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frameHeight > imgsensor_info.hs_video.framelength) ?
			(frameHeight - imgsensor_info.hs_video.framelength):0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
	    frameHeight = imgsensor_info.slim_video.pclk / framerate * 10 /
			imgsensor_info.slim_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.slim_video.framelength) ?
			(frameHeight - imgsensor_info.slim_video.framelength):0;
	    imgsensor.frame_length = imgsensor_info.slim_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;


/* ITD: Modify Dualcam By Jesse 190924 End */
	default:  //coding with  preview scenario by default
	    frameHeight = imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.pre.framelength) ?
			(frameHeight - imgsensor_info.pre.framelength):0;
	    imgsensor.frame_length = imgsensor_info.pre.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MUINT32 *framerate)
{
	LOG_INF("[3058]scenario_id = %d\n", scenario_id);

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


/* ITD: Modify Dualcam By Jesse 190924 End */
	default:
	break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_uint32 modes)
{
	LOG_INF("Test_Pattern modes: %d\n", modes);
	if (modes == 2) {//colorbar
		write_cmos_sensor(0xfd, 0x00);
		write_cmos_sensor(0xb6, 0x21);
	} else if (modes == 5) {
		//sensor enter black view
		write_cmos_sensor(0xfd, 0x01);
		write_cmos_sensor(0x21, 0x00);
		write_cmos_sensor(0x22, 0x00);
		write_cmos_sensor(0x01, 0x01);
		write_cmos_sensor(0xfd, 0x07);
		write_cmos_sensor(0x04, 0x00);
		write_cmos_sensor(0x05, 0x00);
	}

	if ((modes != 2) && (imgsensor.test_pattern == 2)) {
		write_cmos_sensor(0xfd, 0x00);
		write_cmos_sensor(0xb6, 0x20);
	} else if (modes != 5 && (imgsensor.test_pattern == 5)) {
		//sensor out black view
		write_cmos_sensor(0xfd, 0x01);
		write_cmos_sensor(0x21, 0x02);
		write_cmos_sensor(0x22, 0x00);
		write_cmos_sensor(0x01, 0x01);
		write_cmos_sensor(0xfd, 0x07);
		write_cmos_sensor(0x04, 0x00);
		write_cmos_sensor(0x05, 0x40);
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = modes;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

#ifdef SUPPORT_GET_TEMPERATURE
static kal_uint32 get_sensor_temperature(void)
{
	UINT32 temperature = 0;
	INT32 temperature_convert = 0;

	/*TEMP_SEN_CTL */
	write_cmos_sensor(0x4d12, 0x01);
	temperature = (read_cmos_sensor(0x4d13) << 8) |
		read_cmos_sensor(0x4d13);
	if (temperature < 0xc000)
		temperature_convert = temperature / 256;
	else
		temperature_convert = 192 - temperature / 256;

	return temperature_convert;
}
#endif

static kal_uint32 OV02B_ana_gain_table_16x[] = {
     1024,  1088,  1152,  1216,
     1280,  1344,  1408,  1472,
     1536,  1600,  1664,  1728,
     1792,  1856,  1920,  1984,
     2048,  2176,  2304,  2432,
     2560,  2688,  2816,  2944,
     3072,  3200,  3328,  3456,
     3584,  3712,  3840,  3968,
     4096,  4352,  4608,  4864,
     5120,  5376,  5632,  5888,
     6144,  6400,  6656,  6912,
     7168,  7424,  7680,  7936,
     8192,  8704,  9216,  9728,
    10240, 10752, 11264, 11776,
    12288, 12800, 13312, 13824,
    14336, 14848, 15360, 15872
};

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
			UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
#ifdef SUPPORT_GET_TEMPERATURE
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
#endif
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	// UINT32 *pAeCtrls = NULL;
	UINT32 *pScenarios = NULL;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

#if FPT_PDAF_SUPPORT
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
#endif
	//LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_ANA_GAIN_TABLE:
		LOG_INF("use_my_gain_table,feature_id = %d\n", feature_id);
		if ((void *)(uintptr_t) (*(feature_data + 1)) == NULL) {
			*(feature_data + 0) =
				sizeof(OV02B_ana_gain_table_16x);
		} else {
			memcpy((void *)(uintptr_t) (*(feature_data + 1)),
			(void *)OV02B_ana_gain_table_16x,
			sizeof(OV02B_ana_gain_table_16x));
		}
		break;
    case SENSOR_FEATURE_GET_AWB_REQ_BY_SCENARIO:
        *(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
        break;
    case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
        *(feature_data + 1) = imgsensor_info.min_gain;
        *(feature_data + 2) = imgsensor_info.max_gain;
        break;
    case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
        *(feature_data + 1) = 1;
        *(feature_data + 2) = imgsensor_info.margin;
        break;
	case SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS:
		pScenarios = (MUINT32 *)((uintptr_t)(*(feature_data+1)));
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:

			/*
			 **pScenarios = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
			 *break;
			 */
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_CUSTOM2:
		case MSDK_SCENARIO_ID_CUSTOM3:
		default:
			*pScenarios = 0xff;
			break;
		}
		LOG_INF("SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS %llu %d\n",
			*feature_data, *pScenarios);
		break;

	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
	*(feature_data + 1) = imgsensor_info.min_shutter;
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(feature_data + 2) = 2;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
		default:
			*(feature_data + 2) = 1;
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

		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
			break;
		}
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
		// if (sensor_reg_data->RegAddr == 0xff)
		// 	// seamless_switch(sensor_reg_data->RegData, 1920, 369, 960, 369);
		// else
			write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
	break;
	case SENSOR_FEATURE_GET_REGISTER:
	    if (sensor_reg_data->RegAddr == 0x0000) {
	        write_cmos_sensor(0xfd, 0x00);
		}
	    sensor_reg_data->RegData =
			read_cmos_sensor(sensor_reg_data->RegAddr);
	break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
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
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((UINT32)*feature_data);
	break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
	    *feature_return_para_32 = imgsensor_info.checksum_value;
	    *feature_para_len = 4;
	break;
	case SENSOR_FEATURE_SET_FRAMERATE:
	    spin_lock(&imgsensor_drv_lock);
	    imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		LOG_INF("current fps :%d\n", imgsensor.current_fps);
	break;
	case SENSOR_FEATURE_GET_CROP_INFO:
	    //LOG_INF("GET_CROP_INFO scenarioId:%d\n",
		//	*feature_data_32);

	    wininfo = (struct  SENSOR_WINSIZE_INFO_STRUCT *)
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

/* ITD: Modify Dualcam By Jesse 190924 End */
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		}
	break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
	    LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
	    ihdr_write_shutter_gain((UINT16)*feature_data,
			(UINT16)*(feature_data+1),
				(UINT16)*(feature_data+2));
	break;
    case SENSOR_FEATURE_GET_AE_EFFECTIVE_FRAME_FOR_LE:
        *feature_return_para_32 = imgsensor.current_ae_effective_frame;
        break;
    case SENSOR_FEATURE_GET_AE_FRAME_MODE_FOR_LE:
        memcpy(feature_return_para_32, &imgsensor.ae_frm_mode,
            sizeof(struct IMGSENSOR_AE_FRM_MODE));
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

/* ITD: Modify Dualcam By Jesse 190924 End */
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.pre.mipi_pixel_rate;
				break;
			}
	break;


#if FPT_PDAF_SUPPORT
/******************** PDAF START ********************/
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		PDAFinfo = (struct SET_PD_BLOCK_INFO_T *)
			(uintptr_t)(*(feature_data+1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			imgsensor_pd_info.i4BlockNumX = 248;
			imgsensor_pd_info.i4BlockNumY = 187;
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			imgsensor_pd_info.i4BlockNumX = 248;
			imgsensor_pd_info.i4BlockNumY = 141;
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:

		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		break;
	case SENSOR_FEATURE_SET_PDAF:
			imgsensor.pdaf_mode = *feature_data_16;
		break;
/******************** PDAF END ********************/
#endif
	case SENSOR_FEATURE_SET_AWB_GAIN:
        break;
    case SENSOR_FEATURE_SET_LSC_TBL:
        break;
#ifdef SUPPORT_GET_TEMPERATURE
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = get_sensor_temperature();
		*feature_para_len = 4;
	break;
#endif
/* ITD: Modify Dualcam By Jesse 190924 Start */
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		LOG_INF("SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME\n");
		set_shutter_frame_length((UINT16) (*feature_data),
					(UINT16) (*(feature_data + 1)),
					(BOOL) (*(feature_data + 2)));
		break;
/* ITD: Modify Dualcam By Jesse 190924 End */
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		streaming_control(KAL_FALSE);
		break;

	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*feature_return_para_32 = 1; /*BINNING_SUMMED*/
			break;
		default:
			*feature_return_para_32 = 1; /*BINNING_AVERAGED*/
			break;
		}
		LOG_INF("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
		*feature_para_len = 4;

		break;
	default:
	break;
	}

	return ERROR_NONE;
}   /*  feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 MOT_NEVADA_OV02B_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	sensor_func.arch = IMGSENSOR_ARCH_V2;
	if (pfFunc != NULL)
		*pfFunc =  &sensor_func;
	if (imgsensor.psensor_func == NULL)
		imgsensor.psensor_func = &sensor_func;
	return ERROR_NONE;
}


