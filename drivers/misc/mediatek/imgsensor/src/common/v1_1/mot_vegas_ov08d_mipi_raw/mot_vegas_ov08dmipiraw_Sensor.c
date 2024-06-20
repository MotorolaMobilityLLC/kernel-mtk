// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 *
 * Filename:
 * ---------
 *     OV08Dmipi_Sensor.c
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
 *   update full pd setting for OV08DEB_03B
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#define PFX "OV08D_camera_sensor"
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

#include "mot_vegas_ov08dmipiraw_Sensor.h"

#define _I2C_BUF_SIZE 4096
// kal_uint16 ov08d_i2c_data[_I2C_BUF_SIZE];
// unsigned int _size_to_write;

#define SEAMLESS_ 0
#define SEAMLESS_NO_USE 0

#define LOG_INF(format, args...)    \
	pr_debug(PFX "[%s] " format, __func__, ##args)

#define MULTI_WRITE 1

#define FPT_PDAF_SUPPORT 0

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
    .sensor_id = MOT_VEGAS_OV08D_SENSOR_ID,

    .checksum_value = 0x43daf615, /*checksum value for Camera Auto Test*/

    .pre = {
		.pclk = 36000000,    /*record different mode's pclk*/
		.linelength  =  460,    /*record different mode's linelength*/
		.framelength = 2608,    /*record different mode's framelength*/
		.startx = 0, /*record different mode's startx of grabwindow*/
		.starty = 0,    /*record different mode's starty of grabwindow*/
		.grabwindow_width  = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
    },
    .cap = {
		.pclk = 36000000,
		.linelength  =  460,
		.framelength = 2608,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
    },
    .normal_video = {
		.pclk = 36000000,
		.linelength  =  460,
		.framelength = 2608,
		.startx = 0,
		.starty = 306,
		.grabwindow_width  = 3264,
		.grabwindow_height = 1836,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
    },
    .hs_video = {
		.pclk = 36000000,
		.linelength  =  460,
		.framelength = 2608,
		.startx = 0,
		.starty = 306,
		.grabwindow_width  = 3264,
		.grabwindow_height = 1836,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
    },
    .slim_video = {
		.pclk = 36000000,
		.linelength  =  460,
		.framelength = 2608,
		.startx = 0,
		.starty = 306,
		.grabwindow_width  = 3264,
		.grabwindow_height = 1836,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 288000000,
    },

    .margin = 20,      /*sensor framelength & shutter margin*/
    .min_shutter = 4,  /*min shutter*/
    .min_gain =  64,   /*1x gain*/
    .max_gain = 992,   /*15.5x gain*/
    .min_gain_iso = 100,
    .gain_step = 1, /*minimum step = 4 in 1x~2x gain*/
    .gain_type = 1,    /*to be modify,no gain table for sony*/
    .max_frame_length = 0x3FFFFF,     /* max framelength by sensor register's limitation */
    .ae_shut_delay_frame = 0,
    .ae_sensor_gain_delay_frame = 0,
    .ae_ispGain_delay_frame = 2,   /*isp gain delay frame for AE cycle*/
    .frame_time_delay_frame = 2,
    .ihdr_support = 0,             /*1, support; 0,not support*/
    .ihdr_le_firstline = 0,        /*1,le first ; 0, se first*/
    .temperature_support = 0,/* 1, support; 0,not support */
    .sensor_mode_num = 5,
    .cap_delay_frame = 0,          /*enter capture delay frame num*/
    .pre_delay_frame = 0,          /*enter preview delay frame num*/
    .video_delay_frame = 0,        /*enter video delay frame num*/
    .hs_video_delay_frame = 0, /*enter high speed video  delay frame num*/
    .slim_video_delay_frame = 0,/*enter slim video delay frame num*/
    .isp_driving_current = ISP_DRIVING_8MA, /*mclk driving current*/
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
    .mipi_sensor_type = MIPI_OPHY_NCSI2,
    .mipi_settle_delay_mode = 0, //0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
    .mclk = 24,/*mclk value, suggest 24 or 26 for 24Mhz or 26Mhz*/
    .mipi_lane_num = SENSOR_MIPI_2_LANE,/*mipi lane num*/
    .i2c_addr_table = {0x20, 0xff},
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
    .vblank_convert = 2504, /* vts to vblank*/
    .current_ae_effective_frame = 2,
    .max_shutter = 1523810,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
    {3264, 2448,   0,   0, 3264, 2448, 3264, 2448, 0, 0, 3264, 2448, 0, 0, 3264, 2448}, // Preview
    {3264, 2448,   0,   0, 3264, 2448, 3264, 2448, 0, 0, 3264, 2448, 0, 0, 3264, 2448}, // capture
    {3264, 2448,   0, 306, 3264, 1836, 3264, 1836, 0, 0, 3264, 1836, 0, 0, 3264, 1836}, // video
    {3264, 2448,   0, 306, 3264, 1836, 3264, 1836, 0, 0, 3264, 1836, 0, 0, 3264, 1836}, // hs_video
    {3264, 2448,   0, 306, 3264, 1836, 3264, 1836, 0, 0, 3264, 1836, 0, 0, 3264, 1836}, // slim_video
};


#if FPT_PDAF_SUPPORT
/*PD information update*/
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	 .i4OffsetX = 16,
	 .i4OffsetY = 4,
	 .i4PitchX = 16,
	 .i4PitchY = 16,
	 .i4PairNum = 8,
	 .i4SubBlkW = 8,
	 .i4SubBlkH = 4,
	 .i4PosL = {{23, 6}, {31, 6}, {19, 10}, {27, 10},
		{23, 14}, {31, 14}, {19, 18}, {27, 18} },
	 .i4PosR = {{22, 6}, {30, 6}, {18, 10}, {26, 10},
		{22, 14}, {30, 14}, {18, 18}, {26, 18} },
	 .iMirrorFlip = 0,
	 .i4BlockNumX = 248,
	 .i4BlockNumY = 187,
	 .i4Crop = { {0, 0}, {0, 0}, {0, 372}, {0, 0}, {0, 0},
				 {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
};
#endif

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
static kal_uint16 ov08d_table_write_cmos_sensor(
					kal_uint16 *para, kal_uint32 len)
{
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	pr_debug("%s len %d\n", __func__, len);
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
#if SEAMLESS_
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
    pr_debug("imgsensor.frame_length = %d\n", imgsensor.frame_length);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0x05, (((imgsensor.frame_length - imgsensor.vblank_convert) * 2) & 0xFF00) >> 8);
    write_cmos_sensor(0x06, ((imgsensor.frame_length - imgsensor.vblank_convert )* 2) & 0xFF);
    write_cmos_sensor(0x01, 0x01);
}

static void set_mirror_flip(kal_uint8 image_mirror)
{
    pr_debug("image_mirror = %d\n", image_mirror);
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
    pr_debug("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
    if (enable){
        write_cmos_sensor(0xfd, 0x00);
        write_cmos_sensor(0xa0, 0x01);
    }
    else{
        write_cmos_sensor(0xfd, 0x00);
        write_cmos_sensor(0xa0, 0x00);
        mdelay(10);
    }

    return ERROR_NONE;
}

static int long_exposure_status = 0;

static void write_shutter(kal_uint32 shutter)
{
    kal_uint16 realtime_fps = 0;
	pr_debug("shutter1_from_external = %d, frame_length = %d\n", shutter, imgsensor.frame_length);
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
    if (shutter < imgsensor_info.min_shutter) {
        shutter = imgsensor_info.min_shutter;
    }
    if (imgsensor.autoflicker_en) {
        realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
        if(realtime_fps >= 297 && realtime_fps <= 305){
            set_max_framerate(296, 0);
        } else if(realtime_fps >= 147 && realtime_fps <= 150){
            set_max_framerate(146, 0);
        } else {
            set_max_framerate(realtime_fps, 0);
		}
    }

    if (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) {
        if (shutter > imgsensor.max_shutter) {
            shutter = imgsensor.max_shutter;
        }
        //Frame exposure mode customization for LE
        imgsensor.ae_frm_mode.frame_mode_1 = IMGSENSOR_AE_MODE_SE;
        imgsensor.ae_frm_mode.frame_mode_2 = IMGSENSOR_AE_MODE_SE;
        write_cmos_sensor(0xfd, 0x01);
        write_cmos_sensor(0x24, 0x10);
        write_cmos_sensor(0x02, 0x02);
        write_cmos_sensor(0x03, 0x63);
        write_cmos_sensor(0x04, 0x69);
        write_cmos_sensor(0x01, 0x01);
        long_exposure_status = 1;
    } else if(long_exposure_status == 1){
        pr_debug("le shutter is %d exit\n",shutter);
        write_cmos_sensor(0xfd, 0x00);
        write_cmos_sensor(0x24, 0x10);
        write_cmos_sensor(0x02, 0x00);
        write_cmos_sensor(0x03, 0x06);
        write_cmos_sensor(0x04, 0x1D);
        write_cmos_sensor(0x01, 0x01);
        long_exposure_status = 0;
    }

    imgsensor.current_ae_effective_frame = 2;

    if(long_exposure_status == 0){
        imgsensor.frame_length = (imgsensor.frame_length  >> 2) << 2;
	    write_cmos_sensor(0xfd, 0x01);
	    write_cmos_sensor(0x05, (((imgsensor.frame_length - imgsensor.vblank_convert) * 2) & 0xFF00) >> 8);
	    write_cmos_sensor(0x06, (((imgsensor.frame_length - imgsensor.vblank_convert) * 2)) & 0xFF);
	    write_cmos_sensor(0x01, 0x01);
    }
	pr_debug("shutter2_write_register = %d\n", shutter);
	write_cmos_sensor(0xfd, 0x01);
	write_cmos_sensor(0x02, (shutter*2 >> 16) & 0xFF);
	write_cmos_sensor(0x03, (shutter*2 >> 8) & 0xFF);
	write_cmos_sensor(0x04,  shutter*2  & 0xFF);
	write_cmos_sensor(0x01, 0x01);

	LOG_INF("0x05 = 0x%x, 0x06 = 0x%x\n", read_cmos_sensor(0x05), read_cmos_sensor(0x06));
}

static void set_shutter(kal_uint32 shutter)  //should not be kal_uint16 -- can't reach long exp
{
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	write_shutter(shutter);
}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 iReg = 0x0000;

	//platform 1xgain = 64, sensor driver 1*gain = 0x100
	iReg = gain*16/BASEGAIN;
	return iReg;		/* sensorGlobalGain */
}

static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain, max_gain = imgsensor_info.max_gain;
	unsigned long flags;

	if (gain < imgsensor_info.min_gain || gain > max_gain) {
		pr_debug("Error gain setting");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else if (gain > max_gain)
			gain = max_gain;
	}

	reg_gain = gain2reg(gain);
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.gain = reg_gain;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	pr_debug("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0xfd, 0x01);
	write_cmos_sensor(0x24, (reg_gain & 0xFF));
	write_cmos_sensor(0x01, 0x01);
	return gain;
}

/* ITD: Modify Dualcam By Jesse 190924 Start */
static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 target_frame_length)
{

	spin_lock(&imgsensor_drv_lock);
	if (target_frame_length > 1)
		imgsensor.dummy_line = target_frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_shutter(shutter);
}
/* ITD: Modify Dualcam By Jesse 190924 End */

static void ihdr_write_shutter_gain(kal_uint16 le,
				kal_uint16 se, kal_uint16 gain)
{
}

static void night_mode(kal_bool enable)
{
}

static void init_setting(void)
{
    pr_debug("%s start\n", __func__);
    write_cmos_sensor(0xfd, 0x01);
    pr_debug("%s end\n", __func__);
}

static void preview_setting(void)
{
    pr_debug("%s start\n", __func__);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0e);
    msleep(3);
    write_cmos_sensor(0x20, 0x0b);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x11, 0x2a);
    write_cmos_sensor(0x14, 0x43);
    write_cmos_sensor(0x1e, 0x23);
    write_cmos_sensor(0x16, 0x82);
    write_cmos_sensor(0x21, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x02, 0x00);
    write_cmos_sensor(0x03, 0x12);
    write_cmos_sensor(0x04, 0x50);
    write_cmos_sensor(0x05, 0x00);
    write_cmos_sensor(0x06, 0xd0);
    write_cmos_sensor(0x07, 0x05);
    write_cmos_sensor(0x21, 0x02);
    write_cmos_sensor(0x24, 0x30);
    write_cmos_sensor(0x32, 0x03);
    write_cmos_sensor(0x01, 0x01);
    write_cmos_sensor(0x33, 0x03);
    write_cmos_sensor(0x01, 0x03);
    write_cmos_sensor(0x19, 0x10);
    write_cmos_sensor(0x42, 0x55);
    write_cmos_sensor(0x43, 0x00);
    write_cmos_sensor(0x47, 0x07);
    write_cmos_sensor(0x48, 0x08);
    write_cmos_sensor(0x4c, 0x38);
    write_cmos_sensor(0xb2, 0x7e);
    write_cmos_sensor(0xb3, 0x7b);
    write_cmos_sensor(0xbd, 0x08);
    write_cmos_sensor(0xd2, 0x47);
    write_cmos_sensor(0xd3, 0x10);
    write_cmos_sensor(0xd4, 0x0d);
    write_cmos_sensor(0xd5, 0x08);
    write_cmos_sensor(0xd6, 0x07);
    write_cmos_sensor(0xb1, 0x00);
    write_cmos_sensor(0xb4, 0x00);
    write_cmos_sensor(0xb7, 0x0a);
    write_cmos_sensor(0xbc, 0x44);
    write_cmos_sensor(0xbf, 0x42);
    write_cmos_sensor(0xc1, 0x10);
    write_cmos_sensor(0xc3, 0x24);
    write_cmos_sensor(0xc8, 0x03);
    write_cmos_sensor(0xc9, 0xf8);
    write_cmos_sensor(0xe1, 0x33);
    write_cmos_sensor(0xe2, 0xbb);
    write_cmos_sensor(0x51, 0x0c);
    write_cmos_sensor(0x52, 0x0a);
    write_cmos_sensor(0x57, 0x8c);
    write_cmos_sensor(0x59, 0x09);
    write_cmos_sensor(0x5a, 0x08);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x60, 0x02);
    write_cmos_sensor(0x6d, 0x5c);
    write_cmos_sensor(0x76, 0x16);
    write_cmos_sensor(0x7c, 0x11);
    write_cmos_sensor(0x90, 0x28);
    write_cmos_sensor(0x91, 0x16);
    write_cmos_sensor(0x92, 0x1c);
    write_cmos_sensor(0x93, 0x24);
    write_cmos_sensor(0x95, 0x48);
    write_cmos_sensor(0x9c, 0x06);
    write_cmos_sensor(0xca, 0x0c);
    write_cmos_sensor(0xce, 0x0d);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc0, 0x00);
    write_cmos_sensor(0xdd, 0x18);
    write_cmos_sensor(0xde, 0x19);
    write_cmos_sensor(0xdf, 0x32);
    write_cmos_sensor(0xe0, 0x70);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc2, 0x05);
    write_cmos_sensor(0xd7, 0x88);
    write_cmos_sensor(0xd8, 0x77);
    write_cmos_sensor(0xd9, 0x66);
    write_cmos_sensor(0xfd, 0x07);
    write_cmos_sensor(0x00, 0xf8);
    write_cmos_sensor(0x01, 0x2b);
    write_cmos_sensor(0x05, 0x40);
    write_cmos_sensor(0x08, 0x06);
    write_cmos_sensor(0x09, 0x11);
    write_cmos_sensor(0x28, 0x6f);
    write_cmos_sensor(0x2a, 0x20);
    write_cmos_sensor(0x2b, 0x05);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x52, 0x00);
    write_cmos_sensor(0x53, 0x80);
    write_cmos_sensor(0x54, 0x00);
    write_cmos_sensor(0x55, 0x80);
    write_cmos_sensor(0x56, 0x00);
    write_cmos_sensor(0x57, 0x80);
    write_cmos_sensor(0x58, 0x00);
    write_cmos_sensor(0x59, 0x80);
    write_cmos_sensor(0x5c, 0x3f);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0x9a, 0x30);
    write_cmos_sensor(0xa8, 0x02);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0xa0, 0x00);
    write_cmos_sensor(0xa1, 0x08);
    write_cmos_sensor(0xa2, 0x09);
    write_cmos_sensor(0xa3, 0x90);
    write_cmos_sensor(0xa4, 0x00);
    write_cmos_sensor(0xa5, 0x08);
    write_cmos_sensor(0xa6, 0x0c);
    write_cmos_sensor(0xa7, 0xc0);
    write_cmos_sensor(0xfd, 0x05);
    write_cmos_sensor(0x04, 0x40);
    write_cmos_sensor(0x07, 0x00);
    write_cmos_sensor(0x0D, 0x01);
    write_cmos_sensor(0x0F, 0x01);
    write_cmos_sensor(0x10, 0x0c);
    write_cmos_sensor(0x11, 0xcf);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x13, 0x00);
    write_cmos_sensor(0x14, 0x09);
    write_cmos_sensor(0x15, 0x9f);
    write_cmos_sensor(0x18, 0x00);
    write_cmos_sensor(0x19, 0x00);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x24, 0x01);
    write_cmos_sensor(0xc0, 0x16);
    write_cmos_sensor(0xc1, 0x08);
    write_cmos_sensor(0xc2, 0x30);
    write_cmos_sensor(0x8e, 0x0c);
    write_cmos_sensor(0x8f, 0xc0);
    write_cmos_sensor(0x90, 0x09);
    write_cmos_sensor(0x91, 0x90);
    write_cmos_sensor(0xb7, 0x02);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0f);
    write_cmos_sensor(0xe7, 0x03);
    write_cmos_sensor(0xe7, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    pr_debug("%s end\n", __func__);
}

static void capture_setting(kal_uint16 currefps)
{
    pr_debug("%s start currefps = %d\n", __func__);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0e);
    msleep(3);
    write_cmos_sensor(0x20, 0x0b);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x11, 0x2a);
    write_cmos_sensor(0x14, 0x43);
    write_cmos_sensor(0x1e, 0x23);
    write_cmos_sensor(0x16, 0x82);
    write_cmos_sensor(0x21, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x02, 0x00);
    write_cmos_sensor(0x03, 0x12);
    write_cmos_sensor(0x04, 0x50);
    write_cmos_sensor(0x05, 0x00);
    write_cmos_sensor(0x06, 0xd0);
    write_cmos_sensor(0x07, 0x05);
    write_cmos_sensor(0x21, 0x02);
    write_cmos_sensor(0x24, 0x30);
    write_cmos_sensor(0x32, 0x03);
    write_cmos_sensor(0x01, 0x01);
    write_cmos_sensor(0x33, 0x03);
    write_cmos_sensor(0x01, 0x03);
    write_cmos_sensor(0x19, 0x10);
    write_cmos_sensor(0x42, 0x55);
    write_cmos_sensor(0x43, 0x00);
    write_cmos_sensor(0x47, 0x07);
    write_cmos_sensor(0x48, 0x08);
    write_cmos_sensor(0x4c, 0x38);
    write_cmos_sensor(0xb2, 0x7e);
    write_cmos_sensor(0xb3, 0x7b);
    write_cmos_sensor(0xbd, 0x08);
    write_cmos_sensor(0xd2, 0x47);
    write_cmos_sensor(0xd3, 0x10);
    write_cmos_sensor(0xd4, 0x0d);
    write_cmos_sensor(0xd5, 0x08);
    write_cmos_sensor(0xd6, 0x07);
    write_cmos_sensor(0xb1, 0x00);
    write_cmos_sensor(0xb4, 0x00);
    write_cmos_sensor(0xb7, 0x0a);
    write_cmos_sensor(0xbc, 0x44);
    write_cmos_sensor(0xbf, 0x42);
    write_cmos_sensor(0xc1, 0x10);
    write_cmos_sensor(0xc3, 0x24);
    write_cmos_sensor(0xc8, 0x03);
    write_cmos_sensor(0xc9, 0xf8);
    write_cmos_sensor(0xe1, 0x33);
    write_cmos_sensor(0xe2, 0xbb);
    write_cmos_sensor(0x51, 0x0c);
    write_cmos_sensor(0x52, 0x0a);
    write_cmos_sensor(0x57, 0x8c);
    write_cmos_sensor(0x59, 0x09);
    write_cmos_sensor(0x5a, 0x08);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x60, 0x02);
    write_cmos_sensor(0x6d, 0x5c);
    write_cmos_sensor(0x76, 0x16);
    write_cmos_sensor(0x7c, 0x11);
    write_cmos_sensor(0x90, 0x28);
    write_cmos_sensor(0x91, 0x16);
    write_cmos_sensor(0x92, 0x1c);
    write_cmos_sensor(0x93, 0x24);
    write_cmos_sensor(0x95, 0x48);
    write_cmos_sensor(0x9c, 0x06);
    write_cmos_sensor(0xca, 0x0c);
    write_cmos_sensor(0xce, 0x0d);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc0, 0x00);
    write_cmos_sensor(0xdd, 0x18);
    write_cmos_sensor(0xde, 0x19);
    write_cmos_sensor(0xdf, 0x32);
    write_cmos_sensor(0xe0, 0x70);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc2, 0x05);
    write_cmos_sensor(0xd7, 0x88);
    write_cmos_sensor(0xd8, 0x77);
    write_cmos_sensor(0xd9, 0x66);
    write_cmos_sensor(0xfd, 0x07);
    write_cmos_sensor(0x00, 0xf8);
    write_cmos_sensor(0x01, 0x2b);
    write_cmos_sensor(0x05, 0x40);
    write_cmos_sensor(0x08, 0x06);
    write_cmos_sensor(0x09, 0x11);
    write_cmos_sensor(0x28, 0x6f);
    write_cmos_sensor(0x2a, 0x20);
    write_cmos_sensor(0x2b, 0x05);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x52, 0x00);
    write_cmos_sensor(0x53, 0x80);
    write_cmos_sensor(0x54, 0x00);
    write_cmos_sensor(0x55, 0x80);
    write_cmos_sensor(0x56, 0x00);
    write_cmos_sensor(0x57, 0x80);
    write_cmos_sensor(0x58, 0x00);
    write_cmos_sensor(0x59, 0x80);
    write_cmos_sensor(0x5c, 0x3f);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0x9a, 0x30);
    write_cmos_sensor(0xa8, 0x02);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0xa0, 0x00);
    write_cmos_sensor(0xa1, 0x08);
    write_cmos_sensor(0xa2, 0x09);
    write_cmos_sensor(0xa3, 0x90);
    write_cmos_sensor(0xa4, 0x00);
    write_cmos_sensor(0xa5, 0x08);
    write_cmos_sensor(0xa6, 0x0c);
    write_cmos_sensor(0xa7, 0xc0);
    write_cmos_sensor(0xfd, 0x05);
    write_cmos_sensor(0x04, 0x40);
    write_cmos_sensor(0x07, 0x00);
    write_cmos_sensor(0x0D, 0x01);
    write_cmos_sensor(0x0F, 0x01);
    write_cmos_sensor(0x10, 0x0c);
    write_cmos_sensor(0x11, 0xcf);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x13, 0x00);
    write_cmos_sensor(0x14, 0x09);
    write_cmos_sensor(0x15, 0x9f);
    write_cmos_sensor(0x18, 0x00);
    write_cmos_sensor(0x19, 0x00);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x24, 0x01);
    write_cmos_sensor(0xc0, 0x16);
    write_cmos_sensor(0xc1, 0x08);
    write_cmos_sensor(0xc2, 0x30);
    write_cmos_sensor(0x8e, 0x0c);
    write_cmos_sensor(0x8f, 0xc0);
    write_cmos_sensor(0x90, 0x09);
    write_cmos_sensor(0x91, 0x90);
    write_cmos_sensor(0xb7, 0x02);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0f);
    write_cmos_sensor(0xe7, 0x03);
    write_cmos_sensor(0xe7, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    pr_debug("%s end\n", __func__);
}

static void normal_video_setting(kal_uint16 currefps)
{
    pr_debug("%s start currefps = %d\n", __func__);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0e);
    msleep(3);
    write_cmos_sensor(0x20, 0x0b);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x11, 0x2a);
    write_cmos_sensor(0x14, 0x43);
    write_cmos_sensor(0x1e, 0x23);
    write_cmos_sensor(0x16, 0x82);
    write_cmos_sensor(0x21, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x02, 0x00);
    write_cmos_sensor(0x03, 0x12);
    write_cmos_sensor(0x04, 0x50);
    write_cmos_sensor(0x05, 0x00);
    write_cmos_sensor(0x06, 0xd0);
    write_cmos_sensor(0x07, 0x05);
    write_cmos_sensor(0x21, 0x02);
    write_cmos_sensor(0x24, 0x30);
    write_cmos_sensor(0x32, 0x03);
    write_cmos_sensor(0x01, 0x01);
    write_cmos_sensor(0x33, 0x03);
    write_cmos_sensor(0x01, 0x03);
    write_cmos_sensor(0x19, 0x10);
    write_cmos_sensor(0x42, 0x55);
    write_cmos_sensor(0x43, 0x00);
    write_cmos_sensor(0x47, 0x07);
    write_cmos_sensor(0x48, 0x08);
    write_cmos_sensor(0x4c, 0x38);
    write_cmos_sensor(0xb2, 0x7e);
    write_cmos_sensor(0xb3, 0x7b);
    write_cmos_sensor(0xbd, 0x08);
    write_cmos_sensor(0xd2, 0x47);
    write_cmos_sensor(0xd3, 0x10);
    write_cmos_sensor(0xd4, 0x0d);
    write_cmos_sensor(0xd5, 0x08);
    write_cmos_sensor(0xd6, 0x07);
    write_cmos_sensor(0xb1, 0x00);
    write_cmos_sensor(0xb4, 0x00);
    write_cmos_sensor(0xb7, 0x0a);
    write_cmos_sensor(0xbc, 0x44);
    write_cmos_sensor(0xbf, 0x42);
    write_cmos_sensor(0xc1, 0x10);
    write_cmos_sensor(0xc3, 0x24);
    write_cmos_sensor(0xc8, 0x03);
    write_cmos_sensor(0xc9, 0xf8);
    write_cmos_sensor(0xe1, 0x33);
    write_cmos_sensor(0xe2, 0xbb);
    write_cmos_sensor(0x51, 0x0c);
    write_cmos_sensor(0x52, 0x0a);
    write_cmos_sensor(0x57, 0x8c);
    write_cmos_sensor(0x59, 0x09);
    write_cmos_sensor(0x5a, 0x08);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x60, 0x02);
    write_cmos_sensor(0x6d, 0x5c);
    write_cmos_sensor(0x76, 0x16);
    write_cmos_sensor(0x7c, 0x11);
    write_cmos_sensor(0x90, 0x28);
    write_cmos_sensor(0x91, 0x16);
    write_cmos_sensor(0x92, 0x1c);
    write_cmos_sensor(0x93, 0x24);
    write_cmos_sensor(0x95, 0x48);
    write_cmos_sensor(0x9c, 0x06);
    write_cmos_sensor(0xca, 0x0c);
    write_cmos_sensor(0xce, 0x0d);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc0, 0x00);
    write_cmos_sensor(0xdd, 0x18);
    write_cmos_sensor(0xde, 0x19);
    write_cmos_sensor(0xdf, 0x32);
    write_cmos_sensor(0xe0, 0x70);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc2, 0x05);
    write_cmos_sensor(0xd7, 0x88);
    write_cmos_sensor(0xd8, 0x77);
    write_cmos_sensor(0xd9, 0x66);
    write_cmos_sensor(0xfd, 0x07);
    write_cmos_sensor(0x00, 0xf8);
    write_cmos_sensor(0x01, 0x2b);
    write_cmos_sensor(0x05, 0x40);
    write_cmos_sensor(0x08, 0x06);
    write_cmos_sensor(0x09, 0x11);
    write_cmos_sensor(0x28, 0x6f);
    write_cmos_sensor(0x2a, 0x20);
    write_cmos_sensor(0x2b, 0x05);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x52, 0x00);
    write_cmos_sensor(0x53, 0x80);
    write_cmos_sensor(0x54, 0x00);
    write_cmos_sensor(0x55, 0x80);
    write_cmos_sensor(0x56, 0x00);
    write_cmos_sensor(0x57, 0x80);
    write_cmos_sensor(0x58, 0x00);
    write_cmos_sensor(0x59, 0x80);
    write_cmos_sensor(0x5c, 0x3f);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0x9a, 0x30);
    write_cmos_sensor(0xa8, 0x02);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0xa0, 0x01);
    write_cmos_sensor(0xa1, 0x3a);
    write_cmos_sensor(0xa2, 0x07);
    write_cmos_sensor(0xa3, 0x2c);
    write_cmos_sensor(0xa4, 0x00);
    write_cmos_sensor(0xa5, 0x08);
    write_cmos_sensor(0xa6, 0x0c);
    write_cmos_sensor(0xa7, 0xc0);
    write_cmos_sensor(0xfd, 0x05);
    write_cmos_sensor(0x04, 0x40);
    write_cmos_sensor(0x07, 0x00);
    write_cmos_sensor(0x0D, 0x01);
    write_cmos_sensor(0x0F, 0x01);
    write_cmos_sensor(0x10, 0x0c);
    write_cmos_sensor(0x11, 0xcf);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x13, 0x00);
    write_cmos_sensor(0x14, 0x09);
    write_cmos_sensor(0x15, 0x9f);
    write_cmos_sensor(0x18, 0x00);
    write_cmos_sensor(0x19, 0x00);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x24, 0x01);
    write_cmos_sensor(0xc0, 0x16);
    write_cmos_sensor(0xc1, 0x08);
    write_cmos_sensor(0xc2, 0x30);
    write_cmos_sensor(0x8e, 0x0c);
    write_cmos_sensor(0x8f, 0xc0);
    write_cmos_sensor(0x90, 0x07);
    write_cmos_sensor(0x91, 0x2c);
    write_cmos_sensor(0xb7, 0x02);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0f);
    write_cmos_sensor(0xe7, 0x03);
    write_cmos_sensor(0xe7, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    pr_debug("%s end\n", __func__);
}

static void hs_video_setting(void)
{
    pr_debug("%s start\n", __func__);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0e);
    msleep(3);
    write_cmos_sensor(0x20, 0x0b);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x11, 0x2a);
    write_cmos_sensor(0x14, 0x43);
    write_cmos_sensor(0x1e, 0x23);
    write_cmos_sensor(0x16, 0x82);
    write_cmos_sensor(0x21, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x02, 0x00);
    write_cmos_sensor(0x03, 0x12);
    write_cmos_sensor(0x04, 0x50);
    write_cmos_sensor(0x05, 0x00);
    write_cmos_sensor(0x06, 0xd0);
    write_cmos_sensor(0x07, 0x05);
    write_cmos_sensor(0x21, 0x02);
    write_cmos_sensor(0x24, 0x30);
    write_cmos_sensor(0x32, 0x03);
    write_cmos_sensor(0x01, 0x01);
    write_cmos_sensor(0x33, 0x03);
    write_cmos_sensor(0x01, 0x03);
    write_cmos_sensor(0x19, 0x10);
    write_cmos_sensor(0x42, 0x55);
    write_cmos_sensor(0x43, 0x00);
    write_cmos_sensor(0x47, 0x07);
    write_cmos_sensor(0x48, 0x08);
    write_cmos_sensor(0x4c, 0x38);
    write_cmos_sensor(0xb2, 0x7e);
    write_cmos_sensor(0xb3, 0x7b);
    write_cmos_sensor(0xbd, 0x08);
    write_cmos_sensor(0xd2, 0x47);
    write_cmos_sensor(0xd3, 0x10);
    write_cmos_sensor(0xd4, 0x0d);
    write_cmos_sensor(0xd5, 0x08);
    write_cmos_sensor(0xd6, 0x07);
    write_cmos_sensor(0xb1, 0x00);
    write_cmos_sensor(0xb4, 0x00);
    write_cmos_sensor(0xb7, 0x0a);
    write_cmos_sensor(0xbc, 0x44);
    write_cmos_sensor(0xbf, 0x42);
    write_cmos_sensor(0xc1, 0x10);
    write_cmos_sensor(0xc3, 0x24);
    write_cmos_sensor(0xc8, 0x03);
    write_cmos_sensor(0xc9, 0xf8);
    write_cmos_sensor(0xe1, 0x33);
    write_cmos_sensor(0xe2, 0xbb);
    write_cmos_sensor(0x51, 0x0c);
    write_cmos_sensor(0x52, 0x0a);
    write_cmos_sensor(0x57, 0x8c);
    write_cmos_sensor(0x59, 0x09);
    write_cmos_sensor(0x5a, 0x08);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x60, 0x02);
    write_cmos_sensor(0x6d, 0x5c);
    write_cmos_sensor(0x76, 0x16);
    write_cmos_sensor(0x7c, 0x11);
    write_cmos_sensor(0x90, 0x28);
    write_cmos_sensor(0x91, 0x16);
    write_cmos_sensor(0x92, 0x1c);
    write_cmos_sensor(0x93, 0x24);
    write_cmos_sensor(0x95, 0x48);
    write_cmos_sensor(0x9c, 0x06);
    write_cmos_sensor(0xca, 0x0c);
    write_cmos_sensor(0xce, 0x0d);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc0, 0x00);
    write_cmos_sensor(0xdd, 0x18);
    write_cmos_sensor(0xde, 0x19);
    write_cmos_sensor(0xdf, 0x32);
    write_cmos_sensor(0xe0, 0x70);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc2, 0x05);
    write_cmos_sensor(0xd7, 0x88);
    write_cmos_sensor(0xd8, 0x77);
    write_cmos_sensor(0xd9, 0x66);
    write_cmos_sensor(0xfd, 0x07);
    write_cmos_sensor(0x00, 0xf8);
    write_cmos_sensor(0x01, 0x2b);
    write_cmos_sensor(0x05, 0x40);
    write_cmos_sensor(0x08, 0x06);
    write_cmos_sensor(0x09, 0x11);
    write_cmos_sensor(0x28, 0x6f);
    write_cmos_sensor(0x2a, 0x20);
    write_cmos_sensor(0x2b, 0x05);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x52, 0x00);
    write_cmos_sensor(0x53, 0x80);
    write_cmos_sensor(0x54, 0x00);
    write_cmos_sensor(0x55, 0x80);
    write_cmos_sensor(0x56, 0x00);
    write_cmos_sensor(0x57, 0x80);
    write_cmos_sensor(0x58, 0x00);
    write_cmos_sensor(0x59, 0x80);
    write_cmos_sensor(0x5c, 0x3f);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0x9a, 0x30);
    write_cmos_sensor(0xa8, 0x02);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0xa0, 0x01);
    write_cmos_sensor(0xa1, 0x3a);
    write_cmos_sensor(0xa2, 0x07);
    write_cmos_sensor(0xa3, 0x2c);
    write_cmos_sensor(0xa4, 0x00);
    write_cmos_sensor(0xa5, 0x08);
    write_cmos_sensor(0xa6, 0x0c);
    write_cmos_sensor(0xa7, 0xc0);
    write_cmos_sensor(0xfd, 0x05);
    write_cmos_sensor(0x04, 0x40);
    write_cmos_sensor(0x07, 0x00);
    write_cmos_sensor(0x0D, 0x01);
    write_cmos_sensor(0x0F, 0x01);
    write_cmos_sensor(0x10, 0x0c);
    write_cmos_sensor(0x11, 0xcf);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x13, 0x00);
    write_cmos_sensor(0x14, 0x09);
    write_cmos_sensor(0x15, 0x9f);
    write_cmos_sensor(0x18, 0x00);
    write_cmos_sensor(0x19, 0x00);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x24, 0x01);
    write_cmos_sensor(0xc0, 0x16);
    write_cmos_sensor(0xc1, 0x08);
    write_cmos_sensor(0xc2, 0x30);
    write_cmos_sensor(0x8e, 0x0c);
    write_cmos_sensor(0x8f, 0xc0);
    write_cmos_sensor(0x90, 0x07);
    write_cmos_sensor(0x91, 0x2c);
    write_cmos_sensor(0xb7, 0x02);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0f);
    write_cmos_sensor(0xe7, 0x03);
    write_cmos_sensor(0xe7, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    pr_debug("%s end\n", __func__);
}

static void slim_video_setting(void)
{
    pr_debug("%s start\n", __func__);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0e);
    msleep(3);
    write_cmos_sensor(0x20, 0x0b);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x11, 0x2a);
    write_cmos_sensor(0x14, 0x43);
    write_cmos_sensor(0x1e, 0x23);
    write_cmos_sensor(0x16, 0x82);
    write_cmos_sensor(0x21, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x02, 0x00);
    write_cmos_sensor(0x03, 0x12);
    write_cmos_sensor(0x04, 0x50);
    write_cmos_sensor(0x05, 0x00);
    write_cmos_sensor(0x06, 0xd0);
    write_cmos_sensor(0x07, 0x05);
    write_cmos_sensor(0x21, 0x02);
    write_cmos_sensor(0x24, 0x30);
    write_cmos_sensor(0x32, 0x03);
    write_cmos_sensor(0x01, 0x01);
    write_cmos_sensor(0x33, 0x03);
    write_cmos_sensor(0x01, 0x03);
    write_cmos_sensor(0x19, 0x10);
    write_cmos_sensor(0x42, 0x55);
    write_cmos_sensor(0x43, 0x00);
    write_cmos_sensor(0x47, 0x07);
    write_cmos_sensor(0x48, 0x08);
    write_cmos_sensor(0x4c, 0x38);
    write_cmos_sensor(0xb2, 0x7e);
    write_cmos_sensor(0xb3, 0x7b);
    write_cmos_sensor(0xbd, 0x08);
    write_cmos_sensor(0xd2, 0x47);
    write_cmos_sensor(0xd3, 0x10);
    write_cmos_sensor(0xd4, 0x0d);
    write_cmos_sensor(0xd5, 0x08);
    write_cmos_sensor(0xd6, 0x07);
    write_cmos_sensor(0xb1, 0x00);
    write_cmos_sensor(0xb4, 0x00);
    write_cmos_sensor(0xb7, 0x0a);
    write_cmos_sensor(0xbc, 0x44);
    write_cmos_sensor(0xbf, 0x42);
    write_cmos_sensor(0xc1, 0x10);
    write_cmos_sensor(0xc3, 0x24);
    write_cmos_sensor(0xc8, 0x03);
    write_cmos_sensor(0xc9, 0xf8);
    write_cmos_sensor(0xe1, 0x33);
    write_cmos_sensor(0xe2, 0xbb);
    write_cmos_sensor(0x51, 0x0c);
    write_cmos_sensor(0x52, 0x0a);
    write_cmos_sensor(0x57, 0x8c);
    write_cmos_sensor(0x59, 0x09);
    write_cmos_sensor(0x5a, 0x08);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x60, 0x02);
    write_cmos_sensor(0x6d, 0x5c);
    write_cmos_sensor(0x76, 0x16);
    write_cmos_sensor(0x7c, 0x11);
    write_cmos_sensor(0x90, 0x28);
    write_cmos_sensor(0x91, 0x16);
    write_cmos_sensor(0x92, 0x1c);
    write_cmos_sensor(0x93, 0x24);
    write_cmos_sensor(0x95, 0x48);
    write_cmos_sensor(0x9c, 0x06);
    write_cmos_sensor(0xca, 0x0c);
    write_cmos_sensor(0xce, 0x0d);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc0, 0x00);
    write_cmos_sensor(0xdd, 0x18);
    write_cmos_sensor(0xde, 0x19);
    write_cmos_sensor(0xdf, 0x32);
    write_cmos_sensor(0xe0, 0x70);
    write_cmos_sensor(0xfd, 0x01);
    write_cmos_sensor(0xc2, 0x05);
    write_cmos_sensor(0xd7, 0x88);
    write_cmos_sensor(0xd8, 0x77);
    write_cmos_sensor(0xd9, 0x66);
    write_cmos_sensor(0xfd, 0x07);
    write_cmos_sensor(0x00, 0xf8);
    write_cmos_sensor(0x01, 0x2b);
    write_cmos_sensor(0x05, 0x40);
    write_cmos_sensor(0x08, 0x06);
    write_cmos_sensor(0x09, 0x11);
    write_cmos_sensor(0x28, 0x6f);
    write_cmos_sensor(0x2a, 0x20);
    write_cmos_sensor(0x2b, 0x05);
    write_cmos_sensor(0x5e, 0x10);
    write_cmos_sensor(0x52, 0x00);
    write_cmos_sensor(0x53, 0x80);
    write_cmos_sensor(0x54, 0x00);
    write_cmos_sensor(0x55, 0x80);
    write_cmos_sensor(0x56, 0x00);
    write_cmos_sensor(0x57, 0x80);
    write_cmos_sensor(0x58, 0x00);
    write_cmos_sensor(0x59, 0x80);
    write_cmos_sensor(0x5c, 0x3f);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0x9a, 0x30);
    write_cmos_sensor(0xa8, 0x02);
    write_cmos_sensor(0xfd, 0x02);
    write_cmos_sensor(0xa0, 0x01);
    write_cmos_sensor(0xa1, 0x3a);
    write_cmos_sensor(0xa2, 0x07);
    write_cmos_sensor(0xa3, 0x2c);
    write_cmos_sensor(0xa4, 0x00);
    write_cmos_sensor(0xa5, 0x08);
    write_cmos_sensor(0xa6, 0x0c);
    write_cmos_sensor(0xa7, 0xc0);
    write_cmos_sensor(0xfd, 0x05);
    write_cmos_sensor(0x04, 0x40);
    write_cmos_sensor(0x07, 0x00);
    write_cmos_sensor(0x0D, 0x01);
    write_cmos_sensor(0x0F, 0x01);
    write_cmos_sensor(0x10, 0x0c);
    write_cmos_sensor(0x11, 0xcf);
    write_cmos_sensor(0x12, 0x00);
    write_cmos_sensor(0x13, 0x00);
    write_cmos_sensor(0x14, 0x09);
    write_cmos_sensor(0x15, 0x9f);
    write_cmos_sensor(0x18, 0x00);
    write_cmos_sensor(0x19, 0x00);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x24, 0x01);
    write_cmos_sensor(0xc0, 0x16);
    write_cmos_sensor(0xc1, 0x08);
    write_cmos_sensor(0xc2, 0x30);
    write_cmos_sensor(0x8e, 0x0c);
    write_cmos_sensor(0x8f, 0xc0);
    write_cmos_sensor(0x90, 0x07);
    write_cmos_sensor(0x91, 0x2c);
    write_cmos_sensor(0xb7, 0x02);
    write_cmos_sensor(0xfd, 0x00);
    write_cmos_sensor(0x20, 0x0f);
    write_cmos_sensor(0xe7, 0x03);
    write_cmos_sensor(0xe7, 0x00);
    write_cmos_sensor(0xfd, 0x01);
    pr_debug("%s end\n", __func__);
}

static void sensor_init(void)
{
	LOG_INF("%s E\n", __func__);
	init_setting();
	set_mirror_flip(imgsensor.mirror);
	LOG_INF("%s X\n", __func__);
}

static kal_uint32 return_sensor_id(void)
{
	write_cmos_sensor(0xfd, 0x00);
	return ((read_cmos_sensor(0x01) << 8) | read_cmos_sensor(0x02));
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
				pr_info("[%s] i2c write id_v1-1: 0x%x, sensor id: 0x%x\n",
					__func__, imgsensor.i2c_write_id, *sensor_id);

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
	pr_debug("%s +\n", __func__);

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				pr_debug("i2c write id_v1-1: 0x%x, sensor id: 0x%x\n",
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
		pr_debug("Open sensor id: 0x%x fail\n", sensor_id);
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
	imgsensor.vblank_convert = 2504; //for 1632x1224 30fps
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
	imgsensor.vblank_convert = 2504; //for 3264x2448
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
	imgsensor.vblank_convert = 2504; //for 3264x2448
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	//imgsensor.current_fps = 300;
	//imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
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
	imgsensor.vblank_convert = 2504;
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
	imgsensor.vblank_convert = 2504;//776;//1280x720_90fps
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
	pr_debug("scenario_id = %d\n", scenario_id);
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
		pr_debug("Error ScenarioId setting");
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
	pr_debug("enable = %d, framerate = %d\n",
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

	pr_debug("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

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
	pr_debug("[3058]scenario_id = %d\n", scenario_id);

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
	pr_debug("Test_Pattern modes: %d\n", modes);
	if (modes == 2) {//colorbar
		write_cmos_sensor(0xfd, 0x00);
		write_cmos_sensor(0xb6, 0x21);
	} else if (modes == 5) {//black

	}

	if ((modes != 2) && (imgsensor.test_pattern == 2)) {
		write_cmos_sensor(0xfd, 0x00);
		write_cmos_sensor(0xb6, 0x20);
	} else if (modes != 5 && (imgsensor.test_pattern == 5)) {

	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = modes;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 get_sensor_temperature(void)
{
#if 0
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
#endif
	return ERROR_NONE;
}

static kal_uint32 ov08d_ana_gain_table_16x[] = {
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
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	// UINT32 *pAeCtrls = NULL;
	UINT32 *pScenarios = NULL;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

#if FPT_PDAF_SUPPORT
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
#endif
	//pr_debug("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_ANA_GAIN_TABLE:
		pr_debug("use_my_gain_table,feature_id = %d\n", feature_id);
		if ((void *)(uintptr_t) (*(feature_data + 1)) == NULL) {
			*(feature_data + 0) =
				sizeof(ov08d_ana_gain_table_16x);
		} else {
			memcpy((void *)(uintptr_t) (*(feature_data + 1)),
			(void *)ov08d_ana_gain_table_16x,
			sizeof(ov08d_ana_gain_table_16x));
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
		pr_debug("SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS %d %d\n",
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
		pr_debug("current fps :%d\n", imgsensor.current_fps);
	break;
	case SENSOR_FEATURE_GET_CROP_INFO:
	    //pr_debug("GET_CROP_INFO scenarioId:%d\n",
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
	    pr_debug("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
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
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = get_sensor_temperature();
		*feature_para_len = 4;
	break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		pr_debug("SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME\n");
		set_shutter_frame_length((UINT16)*feature_data, (UINT16)*(feature_data+1));
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
			*feature_return_para_32 = 2; /*BINNING_SUMMED*/
			break;
		default:
			*feature_return_para_32 = 1; /*BINNING_AVERAGED*/
			break;
		}
		pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
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

UINT32 MOT_VEGAS_OV08D_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	sensor_func.arch = IMGSENSOR_ARCH_V2;
	if (pfFunc != NULL)
		*pfFunc =  &sensor_func;
	if (imgsensor.psensor_func == NULL)
		imgsensor.psensor_func = &sensor_func;
	return ERROR_NONE;
}
