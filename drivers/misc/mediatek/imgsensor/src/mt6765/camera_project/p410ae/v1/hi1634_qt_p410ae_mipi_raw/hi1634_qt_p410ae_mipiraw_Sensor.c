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

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>



#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_typedef.h"
#include "hi1634_qt_p410ae_mipiraw_Sensor.h"

#ifdef CONFIG_TINNO_PRODUCT_INFO
#include "dev_info.h"
#endif

#define EEPROM_WRITE_ID   0xa2
#define PFX "hi634_qtech_p410ae_camera_sensor"
#define LOG_INF(format, args...)    \
	pr_err(PFX "[%s] " format, __func__, ##args)

//PDAF
#define ENABLE_PDAF 0
//#define e2prom 1

#define per_frame 1

// bool read_hi1634_qt_p410ae_eeprom( kal_uint16 addr, BYTE *data, kal_uint32 size); 
//extern bool read_eeprom( kal_uint16 addr, BYTE * data, kal_uint32 size);

#define MULTI_WRITE 1

static MUINT8 pOtp_data[16];
static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = { 
	.sensor_id = HI1634_QT_P410AE_SENSOR_ID,
	
	.checksum_value = 0x4f1b1d5e,       //0x6d01485c // Auto Test Mode ÃßÈÄ..
	.pre = {
		.pclk = 80000000,				//record different mode's pclk
		.linelength =  710, 			//record different mode's linelength
		.framelength = 3755, 			//record different mode's framelength
		.startx = 0,				    //record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2328, 		//record different mode's width of grabwindow
		.grabwindow_height = 1752,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
		.mipi_pixel_rate = 339200000, //(714M*4/10)
	},
	.cap = {
		.pclk = 80000000,
		.linelength = 710, 	
		.framelength = 3755, 
		.startx = 0,	
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 3504,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 678400000, //(1428M * 4 / 10 )
	},
	// need to setting
    .cap1 = {                            
		.pclk = 80000000,
		.linelength = 710, 	
		.framelength = 4694, 
		.startx = 0,	
		.starty = 0,
		.grabwindow_width = 4656,
		.grabwindow_height = 3504,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 240,
		.mipi_pixel_rate = 678400000, //(1428M * 4 / 10 )
    },
	.normal_video = {
		.pclk = 80000000,
		.linelength =  710,
		.framelength = 3755,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2328,
		.grabwindow_height = 1752,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,	
		.mipi_pixel_rate = 339200000, //(714M*4/10)
	},
	.hs_video = {
		.pclk = 80000000,
		.linelength =  710,
		.framelength = 3755,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2328,
		.grabwindow_height = 1752,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,	
		.mipi_pixel_rate = 339200000, //(714M*4/10)
	},
    .slim_video = {
		.pclk = 80000000,
		.linelength =  710,
		.framelength = 3755,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2328,
		.grabwindow_height = 1752,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,	
		.mipi_pixel_rate = 339200000, //(714M*4/10)
    },

	.margin = 7,
	.min_shutter = 7,
	.max_frame_length = 0x7FFF,
#if per_frame
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
#else
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 1,
	.ae_ispGain_delay_frame = 2,
#endif

	.ihdr_support = 0,      //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num

	.cap_delay_frame = 2, 
	.pre_delay_frame = 2, 
	.video_delay_frame = 2, 
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO, //0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x42, 0xff},
	.i2c_speed = 1000,
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x0100,
	.gain = 0xe0,
	.dummy_pixel = 0,
	.dummy_line = 0,
//full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = 0,
	.i2c_write_id = 0x42,
};

//* Sensor output window information */
static  struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] ={
	{4672, 3520, 0, 4, 4672, 3512, 2336, 1756, 4, 2, 2328, 1752, 0, 0, 2328, 1752},    /* Preview */
	{4672, 3520, 0, 6, 4672, 3508, 4672, 3508, 8, 2, 4656, 3504, 0, 0, 4656, 3504},    /* capture */
	{4672, 3520, 0, 4, 4672, 3512, 2336, 1756, 4, 2, 2328, 1752, 0, 0, 2328, 1752},    /* video */
        {4672, 3520, 0, 4, 4672, 3512, 2336, 1756, 4, 2, 2328, 1752, 0, 0, 2328, 1752}, /* hight speed video*/
        {4672, 3520, 0, 4, 4672, 3512, 2336, 1756, 4, 2, 2328, 1752, 0, 0, 2328, 1752},    /* slim video */
};// slim video

#if ENABLE_PDAF

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
{
	/* Preview mode setting */
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum	
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8	2:4x4  3:1x1 */
	  0x00, 0x2B, 0x0838, 0x0618,	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
	  0x01, 0x30, 0x0000, 0x0000,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??
	/* Capture mode setting */ 
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum	
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8	2:4x4  3:1x1 */
	  0x00, 0x2B, 0x1070, 0x0C30,	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
	  0x01, 0x30, 0x0140, 0x0300,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??
	/* Video mode setting */
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum	
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8	2:4x4  3:1x1 */
	  0x00, 0x2B, 0x1070, 0x0C30,	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
	  0x01, 0x30, 0x0140, 0x0300,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??

};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX	= 56,
	.i4OffsetY	= 24,
	.i4PitchX	= 32,
	.i4PitchY	= 32,
	.i4PairNum	= 8,
	.i4SubBlkW	= 16,
	.i4SubBlkH	= 8,
	.i4BlockNumX = 128,
	.i4BlockNumY = 96,
	.iMirrorFlip = 0,
	.i4PosR =	{
						{61,24}, {61,40}, {69,36}, {69,52},
						{77,24}, {77,40}, {85,36}, {85,52},
				},
	.i4PosL =	{
						{61,28}, {61,44}, {69,32}, {69,48}, 
						{77,28}, {77,44}, {85,32}, {85,48}, 
				}


};
#endif


#if MULTI_WRITE
#define I2C_BUFFER_LEN 1020

static kal_uint16 HI1634_QT_p410ae_table_write_cmos_sensor(
					kal_uint16 *para, kal_uint32 len)
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

		if ((I2C_BUFFER_LEN - tosend) < 4 ||
			len == IDX ||
			addr != addr_last) {
			iBurstWriteReg_multi(puSendCmd, tosend,
				imgsensor.i2c_write_id,
				4, imgsensor_info.i2c_speed);

			tosend = 0;
		}
	}
	return 0;
}
#endif

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[4] = {(char)(addr >> 8),
		(char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
}

static void write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[4] = {(char)(addr >> 8),
		(char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n",
		imgsensor.dummy_line, imgsensor.dummy_pixel);
	write_cmos_sensor(0x020e, imgsensor.frame_length & 0xFFFF); 
	write_cmos_sensor(0x0206, imgsensor.line_length & 0xFFFF);

}	/*	set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor(0x0716) << 8) | read_cmos_sensor(0x0717));

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
		imgsensor.dummy_line = imgsensor.frame_length -
			imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;

	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */

static void write_shutter(kal_uint32 shutter)
{
	kal_uint32 realtime_fps = 0;

	spin_lock(&imgsensor_drv_lock);

	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);

	LOG_INF("shutter = %d, imgsensor.frame_length = %d, imgsensor.min_frame_length = %d\n",
		shutter, imgsensor.frame_length, imgsensor.min_frame_length);


	shutter = (shutter < imgsensor_info.min_shutter) ?
		imgsensor_info.min_shutter : shutter;
	shutter = (shutter >
		(imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
		(imgsensor_info.max_frame_length - imgsensor_info.margin) :
		shutter;
	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk /
			(imgsensor.line_length * imgsensor.frame_length) * 10;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else
			write_cmos_sensor(0x020e, imgsensor.frame_length);

	} else{
			write_cmos_sensor(0x020e, imgsensor.frame_length);
	}

	write_cmos_sensor_8(0x020D, (shutter & 0xFF0000) >> 16 );
	write_cmos_sensor(0x020A, shutter);

	LOG_INF("frame_length = %d , shutter = %d \n", imgsensor.frame_length, shutter);


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

	LOG_INF("set_shutter");
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}	/*	set_shutter */


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
static kal_uint16 gain2reg(kal_uint16 gain)
{
    kal_uint16 reg_gain = 0x0000;
    reg_gain = gain / 4 - 16;

    return (kal_uint16)reg_gain;

}


static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

    /* 0x350A[0:1], 0x350B[0:7] AGC real gain */
    /* [0:3] = N meams N /16 X    */
    /* [4:9] = M meams M X         */
    /* Total gain = M + N /16 X   */

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
	
    write_cmos_sensor_8(0x0213,reg_gain);
	return gain;

}

#if 0
static void ihdr_write_shutter_gain(kal_uint16 le,
				kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n", le, se, gain);
	if (imgsensor.ihdr_en) {
		spin_lock(&imgsensor_drv_lock);
		if (le > imgsensor.min_frame_length - imgsensor_info.margin)
			imgsensor.frame_length = le + imgsensor_info.margin;
		else
			imgsensor.frame_length = imgsensor.min_frame_length;
		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
			imgsensor.frame_length =
				imgsensor_info.max_frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (le < imgsensor_info.min_shutter)
			le = imgsensor_info.min_shutter;
		if (se < imgsensor_info.min_shutter)
			se = imgsensor_info.min_shutter;
		// Extend frame length first
		write_cmos_sensor(0x0006, imgsensor.frame_length);
		write_cmos_sensor(0x3502, (le << 4) & 0xFF);
		write_cmos_sensor(0x3501, (le >> 4) & 0xFF);
		write_cmos_sensor(0x3500, (le >> 12) & 0x0F);
		write_cmos_sensor(0x3508, (se << 4) & 0xFF);
		write_cmos_sensor(0x3507, (se >> 4) & 0xFF);
		write_cmos_sensor(0x3506, (se >> 12) & 0x0F);
		set_gain(gain);
	}
}
#endif


#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d", image_mirror);

	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor(0x0000, 0x0000);
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor(0x0000, 0x0100);

		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor(0x0000, 0x0200);

		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor(0x0000, 0x0300);

		break;
	default:
		LOG_INF("Error image_mirror setting");
		break;
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

#if MULTI_WRITE
kal_uint16 addr_data_pair_init_HI1634_QT_p410ae[] = {
0x0790, 0x0100,
0x2000, 0x1001,
0x2002, 0x0000,
0x2006, 0x40B2,
0x2008, 0xB038,
0x200A, 0x8430,
0x200C, 0x40B2,
0x200E, 0xB082,
0x2010, 0x8476,
0x2012, 0x40B2,
0x2014, 0xB0F4,
0x2016, 0x847A,
0x2018, 0x40B2,
0x201A, 0xB128,
0x201C, 0x8434,
0x201E, 0x40B2,
0x2020, 0xB19C,
0x2022, 0x8488,
0x2024, 0x40B2,
0x2026, 0xB236,
0x2028, 0x8486,
0x202A, 0x40B2,
0x202C, 0xB43E,
0x202E, 0x871E,
0x2030, 0x40B2,
0x2032, 0xB3B8,
0x2034, 0x86C8,
0x2036, 0x4130,
0x2038, 0x120B,
0x203A, 0x120A,
0x203C, 0x403B,
0x203E, 0x0261,
0x2040, 0x4B6A,
0x2042, 0xC3EB,
0x2044, 0x0000,
0x2046, 0x1292,
0x2048, 0xD000,
0x204A, 0x4ACB,
0x204C, 0x0000,
0x204E, 0xB3EB,
0x2050, 0x0000,
0x2052, 0x2411,
0x2054, 0x421F,
0x2056, 0x85DA,
0x2058, 0xD21F,
0x205A, 0x85D8,
0x205C, 0x930F,
0x205E, 0x2404,
0x2060, 0x40F2,
0x2062, 0xFF80,
0x2064, 0x0619,
0x2066, 0x3C07,
0x2068, 0x90F2,
0x206A, 0x0011,
0x206C, 0x0619,
0x206E, 0x2803,
0x2070, 0x50F2,
0x2072, 0xFFF0,
0x2074, 0x0619,
0x2076, 0x40B2,
0x2078, 0xB3BE,
0x207A, 0x86D0,
0x207C, 0x413A,
0x207E, 0x413B,
0x2080, 0x4130,
0x2082, 0x120B,
0x2084, 0x120A,
0x2086, 0x8231,
0x2088, 0x430A,
0x208A, 0x93C2,
0x208C, 0x0C0A,
0x208E, 0x2404,
0x2090, 0xB3D2,
0x2092, 0x0B05,
0x2094, 0x2401,
0x2096, 0x431A,
0x2098, 0x403B,
0x209A, 0x8438,
0x209C, 0x422D,
0x209E, 0x403E,
0x20A0, 0x192A,
0x20A2, 0x403F,
0x20A4, 0x86EC,
0x20A6, 0x12AB,
0x20A8, 0x422D,
0x20AA, 0x403E,
0x20AC, 0x86EC,
0x20AE, 0x410F,
0x20B0, 0x12AB,
0x20B2, 0x930A,
0x20B4, 0x2003,
0x20B6, 0xD3D2,
0x20B8, 0x1921,
0x20BA, 0x3C09,
0x20BC, 0x403D,
0x20BE, 0x0200,
0x20C0, 0x422E,
0x20C2, 0x403F,
0x20C4, 0x86EC,
0x20C6, 0x1292,
0x20C8, 0x8448,
0x20CA, 0xC3D2,
0x20CC, 0x1921,
0x20CE, 0x1292,
0x20D0, 0xD046,
0x20D2, 0x403B,
0x20D4, 0x8438,
0x20D6, 0x422D,
0x20D8, 0x410E,
0x20DA, 0x403F,
0x20DC, 0x86EC,
0x20DE, 0x12AB,
0x20E0, 0x422D,
0x20E2, 0x403E,
0x20E4, 0x86EC,
0x20E6, 0x403F,
0x20E8, 0x192A,
0x20EA, 0x12AB,
0x20EC, 0x5231,
0x20EE, 0x413A,
0x20F0, 0x413B,
0x20F2, 0x4130,
0x20F4, 0x4382,
0x20F6, 0x052C,
0x20F8, 0x4F0D,
0x20FA, 0x930D,
0x20FC, 0x3402,
0x20FE, 0xE33D,
0x2100, 0x531D,
0x2102, 0xF03D,
0x2104, 0x07F0,
0x2106, 0x4D0E,
0x2108, 0xC312,
0x210A, 0x100E,
0x210C, 0x110E,
0x210E, 0x110E,
0x2110, 0x110E,
0x2112, 0x930F,
0x2114, 0x3803,
0x2116, 0x4EC2,
0x2118, 0x052C,
0x211A, 0x3C04,
0x211C, 0x4EC2,
0x211E, 0x052D,
0x2120, 0xE33D,
0x2122, 0x531D,
0x2124, 0x4D0F,
0x2126, 0x4130,
0x2128, 0x120B,
0x212A, 0x425F,
0x212C, 0x0205,
0x212E, 0xC312,
0x2130, 0x104F,
0x2132, 0x114F,
0x2134, 0x114F,
0x2136, 0x114F,
0x2138, 0x114F,
0x213A, 0x114F,
0x213C, 0x4F0B,
0x213E, 0xF31B,
0x2140, 0x5B0B,
0x2142, 0x5B0B,
0x2144, 0x5B0B,
0x2146, 0x503B,
0x2148, 0xD1CC,
0x214A, 0x1292,
0x214C, 0xD004,
0x214E, 0x93C2,
0x2150, 0x86BF,
0x2152, 0x240B,
0x2154, 0xB2E2,
0x2156, 0x0400,
0x2158, 0x2008,
0x215A, 0x425F,
0x215C, 0x86BB,
0x215E, 0xD36F,
0x2160, 0xF37F,
0x2162, 0x5F0F,
0x2164, 0x5F0B,
0x2166, 0x4BA2,
0x2168, 0x0402,
0x216A, 0x93C2,
0x216C, 0x86C1,
0x216E, 0x2414,
0x2170, 0x421F,
0x2172, 0x86C6,
0x2174, 0x4FA2,
0x2176, 0x8606,
0x2178, 0x425F,
0x217A, 0x86BD,
0x217C, 0x425E,
0x217E, 0x86BA,
0x2180, 0x5F0F,
0x2182, 0x5E0F,
0x2184, 0x5F0F,
0x2186, 0x4F0E,
0x2188, 0x521E,
0x218A, 0x86CA,
0x218C, 0x4EA2,
0x218E, 0x8600,
0x2190, 0x521F,
0x2192, 0x86CC,
0x2194, 0x4FA2,
0x2196, 0x8602,
0x2198, 0x413B,
0x219A, 0x4130,
0x219C, 0x8231,
0x219E, 0xD3D2,
0x21A0, 0x7A12,
0x21A2, 0xC3D2,
0x21A4, 0x0F00,
0x21A6, 0x422D,
0x21A8, 0x403E,
0x21AA, 0x06D6,
0x21AC, 0x410F,
0x21AE, 0x1292,
0x21B0, 0x8438,
0x21B2, 0x93C2,
0x21B4, 0x86C1,
0x21B6, 0x243B,
0x21B8, 0x421F,
0x21BA, 0x0402,
0x21BC, 0x0B00,
0x21BE, 0x7304,
0x21C0, 0x0000,
0x21C2, 0x4F82,
0x21C4, 0x0402,
0x21C6, 0x421F,
0x21C8, 0x7100,
0x21CA, 0xF03F,
0x21CC, 0x0003,
0x21CE, 0x0800,
0x21D0, 0x7A10,
0x21D2, 0x931F,
0x21D4, 0x2425,
0x21D6, 0x931F,
0x21D8, 0x281C,
0x21DA, 0x932F,
0x21DC, 0x2414,
0x21DE, 0x903F,
0x21E0, 0x0003,
0x21E2, 0x240B,
0x21E4, 0x425E,
0x21E6, 0x86BB,
0x21E8, 0xEE0F,
0x21EA, 0xF31F,
0x21EC, 0x5F0F,
0x21EE, 0x4F1F,
0x21F0, 0xB466,
0x21F2, 0x9382,
0x21F4, 0x7112,
0x21F6, 0x27E2,
0x21F8, 0x3C1C,
0x21FA, 0x41A2,
0x21FC, 0x06D6,
0x21FE, 0x4192,
0x2200, 0x0002,
0x2202, 0x06D8,
0x2204, 0x3FEF,
0x2206, 0x4192,
0x2208, 0x0002,
0x220A, 0x06DA,
0x220C, 0x41A2,
0x220E, 0x06DC,
0x2210, 0x3FE9,
0x2212, 0x4192,
0x2214, 0x0004,
0x2216, 0x06DA,
0x2218, 0x4192,
0x221A, 0x0006,
0x221C, 0x06DC,
0x221E, 0x3FE2,
0x2220, 0x4192,
0x2222, 0x0006,
0x2224, 0x06D6,
0x2226, 0x4192,
0x2228, 0x0004,
0x222A, 0x06D8,
0x222C, 0x3FDB,
0x222E, 0x1292,
0x2230, 0xD058,
0x2232, 0x5231,
0x2234, 0x4130,
0x2236, 0x93C2,
0x2238, 0x86C1,
0x223A, 0x2427,
0x223C, 0x430C,
0x223E, 0x4C0F,
0x2240, 0x5F0F,
0x2242, 0x5F0F,
0x2244, 0x5F0F,
0x2246, 0x5F0F,
0x2248, 0x5F0F,
0x224A, 0x4F1D,
0x224C, 0x84C4,
0x224E, 0x4F1E,
0x2250, 0x84C6,
0x2252, 0x4F9F,
0x2254, 0x84C0,
0x2256, 0x84C4,
0x2258, 0x4F9F,
0x225A, 0x84C2,
0x225C, 0x84C6,
0x225E, 0x4D8F,
0x2260, 0x84C0,
0x2262, 0x4E8F,
0x2264, 0x84C2,
0x2266, 0x4F1D,
0x2268, 0x84CC,
0x226A, 0x4F1E,
0x226C, 0x84CE,
0x226E, 0x4F9F,
0x2270, 0x84C8,
0x2272, 0x84CC,
0x2274, 0x4F9F,
0x2276, 0x84CA,
0x2278, 0x84CE,
0x227A, 0x4D8F,
0x227C, 0x84C8,
0x227E, 0x4E8F,
0x2280, 0x84CA,
0x2282, 0x531C,
0x2284, 0x903C,
0x2286, 0x0005,
0x2288, 0x3BDA,
0x228A, 0x1292,
0x228C, 0xD056,
0x228E, 0x4130,
0x2290, 0x7400,
0x2292, 0x8058,
0x2294, 0x1807,
0x2296, 0x00E0,
0x2298, 0x7002,
0x229A, 0x17C7,
0x229C, 0x7000,
0x229E, 0x1305,
0x22A0, 0x0006,
0x22A2, 0x001F,
0x22A4, 0x0055,
0x22A6, 0x00DB,
0x22A8, 0x0012,
0x22AA, 0x1754,
0x22AC, 0x206F,
0x22AE, 0x009E,
0x22B0, 0x00DD,
0x22B2, 0x5023,
0x22B4, 0x00DE,
0x22B6, 0x005B,
0x22B8, 0x0119,
0x22BA, 0x0390,
0x22BC, 0x00D1,
0x22BE, 0x0055,
0x22C0, 0x0040,
0x22C2, 0x0553,
0x22C4, 0x0456,
0x22C6, 0x5041,
0x22C8, 0x700D,
0x22CA, 0x2F99,
0x22CC, 0x2318,
0x22CE, 0x005C,
0x22D0, 0x7000,
0x22D2, 0x1586,
0x22D4, 0x0001,
0x22D6, 0x2032,
0x22D8, 0x0012,
0x22DA, 0x0008,
0x22DC, 0x0343,
0x22DE, 0x0148,
0x22E0, 0x2123,
0x22E2, 0x0046,
0x22E4, 0x05DD,
0x22E6, 0x00DE,
0x22E8, 0x00DD,
0x22EA, 0x00DC,
0x22EC, 0x00DE,
0x22EE, 0x07D6,
0x22F0, 0x5061,
0x22F2, 0x704F,
0x22F4, 0x2F99,
0x22F6, 0x005C,
0x22F8, 0x5080,
0x22FA, 0x4D90,
0x22FC, 0x50A1,
0x22FE, 0x2122,
0x2300, 0x7800,
0x2302, 0xC08C,
0x2304, 0x0001,
0x2306, 0x9038,
0x2308, 0x59F7,
0x230A, 0x903B,
0x230C, 0x121C,
0x230E, 0x9034,
0x2310, 0x1218,
0x2312, 0x8C34,
0x2314, 0x0180,
0x2316, 0x8DC0,
0x2318, 0x01C0,
0x231A, 0x7400,
0x231C, 0x8058,
0x231E, 0x1807,
0x2320, 0x00E0,
0x2322, 0x00DF,
0x2324, 0x0047,
0x2326, 0x7000,
0x2328, 0x17C5,
0x232A, 0x0046,
0x232C, 0x0095,
0x232E, 0x7000,
0x2330, 0x148C,
0x2332, 0x005B,
0x2334, 0x0014,
0x2336, 0x001D,
0x2338, 0x216F,
0x233A, 0x005E,
0x233C, 0x00DD,
0x233E, 0x2244,
0x2340, 0x001C,
0x2342, 0x00DE,
0x2344, 0x005B,
0x2346, 0x0519,
0x2348, 0x0150,
0x234A, 0x0091,
0x234C, 0x00D5,
0x234E, 0x0040,
0x2350, 0x0393,
0x2352, 0x0356,
0x2354, 0x5021,
0x2356, 0x700D,
0x2358, 0x2F99,
0x235A, 0x2318,
0x235C, 0x005C,
0x235E, 0x0006,
0x2360, 0x0016,
0x2362, 0x425A,
0x2364, 0x0012,
0x2366, 0x0008,
0x2368, 0x0403,
0x236A, 0x01C8,
0x236C, 0x2123,
0x236E, 0x0046,
0x2370, 0x095D,
0x2372, 0x00DE,
0x2374, 0x00DD,
0x2376, 0x00DC,
0x2378, 0x00DE,
0x237A, 0x04D6,
0x237C, 0x5041,
0x237E, 0x704F,
0x2380, 0x2F99,
0x2382, 0x7000,
0x2384, 0x1702,
0x2386, 0x202C,
0x2388, 0x0016,
0x238A, 0x5060,
0x238C, 0x2122,
0x238E, 0x7800,
0x2390, 0xC08C,
0x2392, 0x0001,
0x2394, 0x903B,
0x2396, 0x121C,
0x2398, 0x9034,
0x239A, 0x1218,
0x239C, 0x8DC0,
0x239E, 0x01C0,
0x23A0, 0x0000,
0x23A2, 0xB290,
0x23A4, 0x0000,
0x23A6, 0xB290,
0x23A8, 0xB302,
0x23AA, 0x0002,
0x23AC, 0x0000,
0x23AE, 0xB31A,
0x23B0, 0x0000,
0x23B2, 0xB31A,
0x23B4, 0xB390,
0x23B6, 0x0002,
0x23B8, 0xB3A0,
0x23BA, 0xB3AC,
0x23BC, 0xFCE0,
0x23BE, 0x0040,
0x23C0, 0x0040,
0x23C2, 0x0040,
0x23C4, 0x0045,
0x23C6, 0x004C,
0x23C8, 0x0050,
0x23CA, 0x005A,
0x23CC, 0x005D,
0x23CE, 0x0064,
0x23D0, 0x0066,
0x23D2, 0x0068,
0x23D4, 0x0071,
0x23D6, 0x0078,
0x23D8, 0x007D,
0x23DA, 0x0087,
0x23DC, 0x008C,
0x23DE, 0x0094,
0x23E0, 0x0098,
0x23E2, 0x00AD,
0x23E4, 0x00B0,
0x23E6, 0x00C3,
0x23E8, 0x00C4,
0x23EA, 0x00D9,
0x23EC, 0x00DE,
0x23EE, 0x00F0,
0x23F0, 0x00FF,
0x23F2, 0x0106,
0x23F4, 0x011A,
0x23F6, 0x0117,
0x23F8, 0x0133,
0x23FA, 0x0126,
0x23FC, 0x0126,
0x23FE, 0x0040,
0x2400, 0x0040,
0x2402, 0x0040,
0x2404, 0x0045,
0x2406, 0x004C,
0x2408, 0x0050,
0x240A, 0x005A,
0x240C, 0x005D,
0x240E, 0x0064,
0x2410, 0x0066,
0x2412, 0x006B,
0x2414, 0x0071,
0x2416, 0x0078,
0x2418, 0x007D,
0x241A, 0x0087,
0x241C, 0x008C,
0x241E, 0x0094,
0x2420, 0x0098,
0x2422, 0x00AD,
0x2424, 0x00B0,
0x2426, 0x00C3,
0x2428, 0x00C4,
0x242A, 0x00D9,
0x242C, 0x00DE,
0x242E, 0x00F0,
0x2430, 0x00FF,
0x2432, 0x0106,
0x2434, 0x011A,
0x2436, 0x0117,
0x2438, 0x0133,
0x243A, 0x0126,
0x243C, 0x0126,
0x243E, 0x0041,
0x2440, 0x0060,
0x2442, 0x0124,
0x2444, 0x023B,
0x2446, 0x05FC,
0x2448, 0x0041,
0x244A, 0x0060,
0x244C, 0x0124,
0x244E, 0x023B,
0x2450, 0x05FC,
0x2452, 0x0041,
0x2454, 0x004E,
0x2456, 0x007D,
0x2458, 0x01B8,
0x245A, 0x05FC,
0x245C, 0x0041,
0x245E, 0x004B,
0x2460, 0x0078,
0x2462, 0x019D,
0x2464, 0x05FC,
0x2466, 0x72D8,
0x2468, 0x278D,

0x0262, 0x0600,
0x026A, 0xFFFF,
0x026C, 0x00FF,
0x026E, 0x0000,
0x0360, 0x0E8E,
0x040C, 0x01EB,
0x0600, 0x1132,
0x0604, 0x8008,
0x0644, 0x07FE,
0x0676, 0x07FF,
0x0678, 0x0002,
0x06A8, 0x0350,
0x06AA, 0x0160,
0x06AC, 0x0041,
0x06AE, 0x03FC,
0x06B4, 0x3FFF,
0x06CC, 0x00FF,
0x06E2, 0xFF00,
0x052A, 0x0000,
0x052C, 0x0000,
0x0F00, 0x0000,
0x0B20, 0x0100,
0x1102, 0x0008,
0x1106, 0x0124,
0x11C2, 0x0400,
0x0902, 0x0003,
0x0904, 0x0003,
0x0912, 0x0303,
0x0914, 0x0300,
0x0A04, 0xB4C5,
0x0A06, 0xC400,
0x0A08, 0xA881,
0x0A0E, 0xFEC0,
0x0A12, 0x0000,
0x0A18, 0x0010,
0x0A20, 0x0015,
0x070C, 0x0000,
0x0780, 0x010E,
0x1060, 0x353D,
0x1062, 0x3000,
0x1202, 0x1E00,
0x1204, 0xD700,
0x1210, 0x8028,
0x1216, 0xA0A0,
0x1218, 0x00A0,
0x121A, 0x0000,
0x121C, 0x4128,
0x121E, 0x0000,
0x1220, 0x0000,
0x1222, 0x28FA,
0x105C, 0x0F0B,
0x1958, 0x0041,
0x195A, 0x0060,
0x195C, 0x0124,
0x195E, 0x023B,
0x1960, 0x05FC,
0x1962, 0x0041,
0x1964, 0x0060,
0x1966, 0x0124,
0x1968, 0x023B,
0x196A, 0x05FC,
0x196C, 0x0041,
0x196E, 0x004E,
0x1970, 0x007D,
0x1972, 0x01B8,
0x1974, 0x05FC,
0x1976, 0x0041,
0x1978, 0x004B,
0x197A, 0x0078,
0x197C, 0x019D,
0x197E, 0x05FC,
0x1980, 0x0082,
0x1982, 0x001F,
0x1984, 0x2006,
0x1986, 0x0031,
0x1988, 0x0308,
0x198A, 0x0000,
0x198C, 0x0F86,
0x198E, 0x0000,
0x1990, 0x310D,
0x1992, 0x0000,
0x1994, 0x3E88,
0x1996, 0x0002,
0x19C0, 0x0082,
0x19C2, 0x001F,
0x19C4, 0x2006,
0x19C6, 0x0031,
0x19C8, 0x0308,
0x19CA, 0x0000,
0x19CC, 0x0F86,
0x19CE, 0x0000,
0x19D0, 0x310D,
0x19D2, 0x0000,
0x19D4, 0x3E88,
0x19D6, 0x0002,
0x1A00, 0x0071,
0x1A02, 0x0053,
0x1A04, 0x0000,
0x1A06, 0x0016,
0x1A08, 0x01B7,
0x1A0A, 0x0000,
0x1A0C, 0x034B,
0x1A0E, 0x0000,
0x1A10, 0x171E,
0x1A12, 0x0000,
0x1A14, 0x0999,
0x1A16, 0x0002,
0x1A40, 0x008A,
0x1A42, 0x005E,
0x1A44, 0x0000,
0x1A46, 0x0012,
0x1A48, 0x01E5,
0x1A4A, 0x0000,
0x1A4C, 0x03CE,
0x1A4E, 0x0000,
0x1A50, 0x1853,
0x1A52, 0x0000,
0x1A54, 0x0030,
0x1A56, 0x0000,
0x19BC, 0x2000,
0x19FC, 0x2000,
0x1A3C, 0x2000,
0x1A7C, 0x2000,
0x361C, 0x0000,
0x027E, 0x0100,
};
#endif

static void sensor_init(void)
{
#if MULTI_WRITE
LOG_INF("LZH sensor_init");
	HI1634_QT_p410ae_table_write_cmos_sensor(
		addr_data_pair_init_HI1634_QT_p410ae,
		sizeof(addr_data_pair_init_HI1634_QT_p410ae) /
		sizeof(kal_uint16));
#else
	write_cmos_sensor(0x2000, 0x0021);
	write_cmos_sensor(0x2002, 0x04a5);
	write_cmos_sensor(0x2004, 0xb124);
	write_cmos_sensor(0x2006, 0xc09c);
	write_cmos_sensor(0x2008, 0x0064);
	write_cmos_sensor(0x200a, 0x088e);
	write_cmos_sensor(0x200c, 0x01c2);
	write_cmos_sensor(0x200e, 0x00b4);
	write_cmos_sensor(0x2010, 0x4020);
	write_cmos_sensor(0x2012, 0x4292);
	write_cmos_sensor(0x2014, 0xf00a);
	write_cmos_sensor(0x2016, 0x0310);
	write_cmos_sensor(0x2018, 0x12b0);
	write_cmos_sensor(0x201a, 0xc3f2);
	write_cmos_sensor(0x201c, 0x425f);
	write_cmos_sensor(0x201e, 0x0282);
	write_cmos_sensor(0x2020, 0xf35f);
	write_cmos_sensor(0x2022, 0xf37f);
	write_cmos_sensor(0x2024, 0x5f0f);
	write_cmos_sensor(0x2026, 0x4f92);
	write_cmos_sensor(0x2028, 0xf692);
	write_cmos_sensor(0x202a, 0x0402);
	write_cmos_sensor(0x202c, 0x93c2);
	write_cmos_sensor(0x202e, 0x82cc);
	write_cmos_sensor(0x2030, 0x2403);
	write_cmos_sensor(0x2032, 0xf0f2);
	write_cmos_sensor(0x2034, 0xffe7);
	write_cmos_sensor(0x2036, 0x0254);
	write_cmos_sensor(0x2038, 0x4130);
	write_cmos_sensor(0x203a, 0x120b);
	write_cmos_sensor(0x203c, 0x120a);
	write_cmos_sensor(0x203e, 0x1209);
	write_cmos_sensor(0x2040, 0x425f);
	write_cmos_sensor(0x2042, 0x0600);
	write_cmos_sensor(0x2044, 0xf35f);
	write_cmos_sensor(0x2046, 0x4f4b);
	write_cmos_sensor(0x2048, 0x12b0);
	write_cmos_sensor(0x204a, 0xc6ae);
	write_cmos_sensor(0x204c, 0x403d);
	write_cmos_sensor(0x204e, 0x0100);
	write_cmos_sensor(0x2050, 0x403e);
	write_cmos_sensor(0x2052, 0x2bfc);
	write_cmos_sensor(0x2054, 0x403f);
	write_cmos_sensor(0x2056, 0x8020);
	write_cmos_sensor(0x2058, 0x12b0);
	write_cmos_sensor(0x205a, 0xc476);
	write_cmos_sensor(0x205c, 0x930b);
	write_cmos_sensor(0x205e, 0x2009);
	write_cmos_sensor(0x2060, 0x93c2);
	write_cmos_sensor(0x2062, 0x0c0a);
	write_cmos_sensor(0x2064, 0x2403);
	write_cmos_sensor(0x2066, 0x43d2);
	write_cmos_sensor(0x2068, 0x0e1f);
	write_cmos_sensor(0x206a, 0x3c13);
	write_cmos_sensor(0x206c, 0x43c2);
	write_cmos_sensor(0x206e, 0x0e1f);
	write_cmos_sensor(0x2070, 0x3c10);
	write_cmos_sensor(0x2072, 0x4039);
	write_cmos_sensor(0x2074, 0x0e08);
	write_cmos_sensor(0x2076, 0x492a);
	write_cmos_sensor(0x2078, 0x421c);
	write_cmos_sensor(0x207a, 0xf010);
	write_cmos_sensor(0x207c, 0x430b);
	write_cmos_sensor(0x207e, 0x430d);
	write_cmos_sensor(0x2080, 0x12b0);
	write_cmos_sensor(0x2082, 0xdfb6);
	write_cmos_sensor(0x2084, 0x403d);
	write_cmos_sensor(0x2086, 0x000e);
	write_cmos_sensor(0x2088, 0x12b0);
	write_cmos_sensor(0x208a, 0xc62c);
	write_cmos_sensor(0x208c, 0x4e89);
	write_cmos_sensor(0x208e, 0x0000);
	write_cmos_sensor(0x2090, 0x3fe7);
	write_cmos_sensor(0x2092, 0x4139);
	write_cmos_sensor(0x2094, 0x413a);
	write_cmos_sensor(0x2096, 0x413b);
	write_cmos_sensor(0x2098, 0x4130);
	write_cmos_sensor(0x209a, 0xb0b2);
	write_cmos_sensor(0x209c, 0x0020);
	write_cmos_sensor(0x209e, 0xf002);
	write_cmos_sensor(0x20a0, 0x2429);
	write_cmos_sensor(0x20a2, 0x421e);
	write_cmos_sensor(0x20a4, 0x0256);
	write_cmos_sensor(0x20a6, 0x532e);
	write_cmos_sensor(0x20a8, 0x421f);
	write_cmos_sensor(0x20aa, 0xf008);
	write_cmos_sensor(0x20ac, 0x9e0f);
	write_cmos_sensor(0x20ae, 0x2c01);
	write_cmos_sensor(0x20b0, 0x4e0f);
	write_cmos_sensor(0x20b2, 0x4f0c);
	write_cmos_sensor(0x20b4, 0x430d);
	write_cmos_sensor(0x20b6, 0x421e);
	write_cmos_sensor(0x20b8, 0x7300);
	write_cmos_sensor(0x20ba, 0x421f);
	write_cmos_sensor(0x20bc, 0x7302);
	write_cmos_sensor(0x20be, 0x5c0e);
	write_cmos_sensor(0x20c0, 0x6d0f);
	write_cmos_sensor(0x20c2, 0x821e);
	write_cmos_sensor(0x20c4, 0x830c);
	write_cmos_sensor(0x20c6, 0x721f);
	write_cmos_sensor(0x20c8, 0x830e);
	write_cmos_sensor(0x20ca, 0x2c0d);
	write_cmos_sensor(0x20cc, 0x0900);
	write_cmos_sensor(0x20ce, 0x7312);
	write_cmos_sensor(0x20d0, 0x421e);
	write_cmos_sensor(0x20d2, 0x7300);
	write_cmos_sensor(0x20d4, 0x421f);
	write_cmos_sensor(0x20d6, 0x7302);
	write_cmos_sensor(0x20d8, 0x5c0e);
	write_cmos_sensor(0x20da, 0x6d0f);
	write_cmos_sensor(0x20dc, 0x821e);
	write_cmos_sensor(0x20de, 0x830c);
	write_cmos_sensor(0x20e0, 0x721f);
	write_cmos_sensor(0x20e2, 0x830e);
	write_cmos_sensor(0x20e4, 0x2bf3);
	write_cmos_sensor(0x20e6, 0x4292);
	write_cmos_sensor(0x20e8, 0x8248);
	write_cmos_sensor(0x20ea, 0x0a08);
	write_cmos_sensor(0x20ec, 0x0c10);
	write_cmos_sensor(0x20ee, 0x4292);
	write_cmos_sensor(0x20f0, 0x8252);
	write_cmos_sensor(0x20f2, 0x0a12);
	write_cmos_sensor(0x20f4, 0x12b0);
	write_cmos_sensor(0x20f6, 0xdc9c);
	write_cmos_sensor(0x20f8, 0xd0f2);
	write_cmos_sensor(0x20fa, 0x0018);
	write_cmos_sensor(0x20fc, 0x0254);
	write_cmos_sensor(0x20fe, 0x4130);
	write_cmos_sensor(0x2100, 0x120b);
	write_cmos_sensor(0x2102, 0x12b0);
	write_cmos_sensor(0x2104, 0xcfc8);
	write_cmos_sensor(0x2106, 0x4f4b);
	write_cmos_sensor(0x2108, 0x12b0);
	write_cmos_sensor(0x210a, 0xcfc8);
	write_cmos_sensor(0x210c, 0xf37f);
	write_cmos_sensor(0x210e, 0x108f);
	write_cmos_sensor(0x2110, 0xdb0f);
	write_cmos_sensor(0x2112, 0x413b);
	write_cmos_sensor(0x2114, 0x4130);
	write_cmos_sensor(0x2116, 0x120b);
	write_cmos_sensor(0x2118, 0x12b0);
	write_cmos_sensor(0x211a, 0xcfc8);
	write_cmos_sensor(0x211c, 0x4f4b);
	write_cmos_sensor(0x211e, 0x108b);
	write_cmos_sensor(0x2120, 0x12b0);
	write_cmos_sensor(0x2122, 0xcfc8);
	write_cmos_sensor(0x2124, 0xf37f);
	write_cmos_sensor(0x2126, 0xdb0f);
	write_cmos_sensor(0x2128, 0x413b);
	write_cmos_sensor(0x212a, 0x4130);
	write_cmos_sensor(0x212c, 0x120b);
	write_cmos_sensor(0x212e, 0x120a);
	write_cmos_sensor(0x2130, 0x1209);
	write_cmos_sensor(0x2132, 0x1208);
	write_cmos_sensor(0x2134, 0x4338);
	write_cmos_sensor(0x2136, 0x40b2);
	write_cmos_sensor(0x2138, 0x17fb);
	write_cmos_sensor(0x213a, 0x83be);
	write_cmos_sensor(0x213c, 0x12b0);
	write_cmos_sensor(0x213e, 0xcfc8);
	write_cmos_sensor(0x2140, 0xf37f);
	write_cmos_sensor(0x2142, 0x903f);
	write_cmos_sensor(0x2144, 0x0013);
	write_cmos_sensor(0x2146, 0x244c);
	write_cmos_sensor(0x2148, 0x12b0);
	write_cmos_sensor(0x214a, 0xf100);
	write_cmos_sensor(0x214c, 0x4f82);
	write_cmos_sensor(0x214e, 0x82a4);
	write_cmos_sensor(0x2150, 0xb3e2);
	write_cmos_sensor(0x2152, 0x0282);
	write_cmos_sensor(0x2154, 0x240a);
	write_cmos_sensor(0x2156, 0x5f0f);
	write_cmos_sensor(0x2158, 0x5f0f);
	write_cmos_sensor(0x215a, 0x521f);
	write_cmos_sensor(0x215c, 0x83be);
	write_cmos_sensor(0x215e, 0x533f);
	write_cmos_sensor(0x2160, 0x4f82);
	write_cmos_sensor(0x2162, 0x83be);
	write_cmos_sensor(0x2164, 0x43f2);
	write_cmos_sensor(0x2166, 0x83c0);
	write_cmos_sensor(0x2168, 0x4308);
	write_cmos_sensor(0x216a, 0x4309);
	write_cmos_sensor(0x216c, 0x9219);
	write_cmos_sensor(0x216e, 0x82a4);
	write_cmos_sensor(0x2170, 0x2c34);
	write_cmos_sensor(0x2172, 0xb3e2);
	write_cmos_sensor(0x2174, 0x0282);
	write_cmos_sensor(0x2176, 0x242a);
	write_cmos_sensor(0x2178, 0x12b0);
	write_cmos_sensor(0x217a, 0xf116);
	write_cmos_sensor(0x217c, 0x4f0b);
	write_cmos_sensor(0x217e, 0x12b0);
	write_cmos_sensor(0x2180, 0xf116);
	write_cmos_sensor(0x2182, 0x4f0a);
	write_cmos_sensor(0x2184, 0x490f);
	write_cmos_sensor(0x2186, 0x5f0f);
	write_cmos_sensor(0x2188, 0x5f0f);
	write_cmos_sensor(0x218a, 0x4b8f);
	write_cmos_sensor(0x218c, 0x2bfc);
	write_cmos_sensor(0x218e, 0x4a8f);
	write_cmos_sensor(0x2190, 0x2bfe);
	write_cmos_sensor(0x2192, 0x5319);
	write_cmos_sensor(0x2194, 0x9039);
	write_cmos_sensor(0x2196, 0x0100);
	write_cmos_sensor(0x2198, 0x2be9);
	write_cmos_sensor(0x219a, 0x43d2);
	write_cmos_sensor(0x219c, 0x83c0);
	write_cmos_sensor(0x219e, 0x421e);
	write_cmos_sensor(0x21a0, 0x82a4);
	write_cmos_sensor(0x21a2, 0x903e);
	write_cmos_sensor(0x21a4, 0x0080);
	write_cmos_sensor(0x21a6, 0x2810);
	write_cmos_sensor(0x21a8, 0x421f);
	write_cmos_sensor(0x21aa, 0x2d28);
	write_cmos_sensor(0x21ac, 0x503f);
	write_cmos_sensor(0x21ae, 0x0014);
	write_cmos_sensor(0x21b0, 0x4f82);
	write_cmos_sensor(0x21b2, 0x82a0);
	write_cmos_sensor(0x21b4, 0x903e);
	write_cmos_sensor(0x21b6, 0x00c0);
	write_cmos_sensor(0x21b8, 0x2805);
	write_cmos_sensor(0x21ba, 0x421f);
	write_cmos_sensor(0x21bc, 0x2e28);
	write_cmos_sensor(0x21be, 0x503f);
	write_cmos_sensor(0x21c0, 0x0014);
	write_cmos_sensor(0x21c2, 0x3c12);
	write_cmos_sensor(0x21c4, 0x480f);
	write_cmos_sensor(0x21c6, 0x3c10);
	write_cmos_sensor(0x21c8, 0x480f);
	write_cmos_sensor(0x21ca, 0x3ff2);
	write_cmos_sensor(0x21cc, 0x12b0);
	write_cmos_sensor(0x21ce, 0xf100);
	write_cmos_sensor(0x21d0, 0x4f0a);
	write_cmos_sensor(0x21d2, 0x12b0);
	write_cmos_sensor(0x21d4, 0xf100);
	write_cmos_sensor(0x21d6, 0x4f0b);
	write_cmos_sensor(0x21d8, 0x3fd5);
	write_cmos_sensor(0x21da, 0x430a);
	write_cmos_sensor(0x21dc, 0x430b);
	write_cmos_sensor(0x21de, 0x3fd2);
	write_cmos_sensor(0x21e0, 0x40b2);
	write_cmos_sensor(0x21e2, 0x1bfe);
	write_cmos_sensor(0x21e4, 0x83be);
	write_cmos_sensor(0x21e6, 0x3fb0);
	write_cmos_sensor(0x21e8, 0x4f82);
	write_cmos_sensor(0x21ea, 0x82a2);
	write_cmos_sensor(0x21ec, 0x4138);
	write_cmos_sensor(0x21ee, 0x4139);
	write_cmos_sensor(0x21f0, 0x413a);
	write_cmos_sensor(0x21f2, 0x413b);
	write_cmos_sensor(0x21f4, 0x4130);
	write_cmos_sensor(0x21f6, 0x43d2);
	write_cmos_sensor(0x21f8, 0x0300);
	write_cmos_sensor(0x21fa, 0x12b0);
	write_cmos_sensor(0x21fc, 0xcf6a);
	write_cmos_sensor(0x21fe, 0x12b0);
	write_cmos_sensor(0x2200, 0xcf0a);
	write_cmos_sensor(0x2202, 0xb3d2);
	write_cmos_sensor(0x2204, 0x0267);
	write_cmos_sensor(0x2206, 0x2404);
	write_cmos_sensor(0x2208, 0x12b0);
	write_cmos_sensor(0x220a, 0xf12c);
	write_cmos_sensor(0x220c, 0xc3d2);
	write_cmos_sensor(0x220e, 0x0267);
	write_cmos_sensor(0x2210, 0x12b0);
	write_cmos_sensor(0x2212, 0xd0d4);
	write_cmos_sensor(0x2214, 0x0261);
	write_cmos_sensor(0x2216, 0x0000);
	write_cmos_sensor(0x2218, 0x43c2);
	write_cmos_sensor(0x221a, 0x0300);
	write_cmos_sensor(0x221c, 0x4392);
	write_cmos_sensor(0x221e, 0x732a);
	write_cmos_sensor(0x2220, 0x4130);
	write_cmos_sensor(0x2222, 0x90f2);
	write_cmos_sensor(0x2224, 0x0010);
	write_cmos_sensor(0x2226, 0x0260);
	write_cmos_sensor(0x2228, 0x2002);
	write_cmos_sensor(0x222a, 0x12b0);
	write_cmos_sensor(0x222c, 0xd4aa);
	write_cmos_sensor(0x222e, 0x12b0);
	write_cmos_sensor(0x2230, 0xd5fa);
	write_cmos_sensor(0x2232, 0x4392);
	write_cmos_sensor(0x2234, 0x732a);
	write_cmos_sensor(0x2236, 0x12b0);
	write_cmos_sensor(0x2238, 0xf1f6);
	write_cmos_sensor(0x223a, 0x4130);
	write_cmos_sensor(0x223c, 0x120b);
	write_cmos_sensor(0x223e, 0x120a);
	write_cmos_sensor(0x2240, 0x1209);
	write_cmos_sensor(0x2242, 0x1208);
	write_cmos_sensor(0x2244, 0x1207);
	write_cmos_sensor(0x2246, 0x1206);
	write_cmos_sensor(0x2248, 0x1205);
	write_cmos_sensor(0x224a, 0x1204);
	write_cmos_sensor(0x224c, 0x8031);
	write_cmos_sensor(0x224e, 0x000a);
	write_cmos_sensor(0x2250, 0x4291);
	write_cmos_sensor(0x2252, 0x82d8);
	write_cmos_sensor(0x2254, 0x0004);
	write_cmos_sensor(0x2256, 0x411f);
	write_cmos_sensor(0x2258, 0x0004);
	write_cmos_sensor(0x225a, 0x4fa1);
	write_cmos_sensor(0x225c, 0x0006);
	write_cmos_sensor(0x225e, 0x4257);
	write_cmos_sensor(0x2260, 0x82e5);
	write_cmos_sensor(0x2262, 0x4708);
	write_cmos_sensor(0x2264, 0xd038);
	write_cmos_sensor(0x2266, 0xff00);
	write_cmos_sensor(0x2268, 0x4349);
	write_cmos_sensor(0x226a, 0x4346);
	write_cmos_sensor(0x226c, 0x90b2);
	write_cmos_sensor(0x226e, 0x07d1);
	write_cmos_sensor(0x2270, 0x0b94);
	write_cmos_sensor(0x2272, 0x2806);
	write_cmos_sensor(0x2274, 0x40b2);
	write_cmos_sensor(0x2276, 0x0246);
	write_cmos_sensor(0x2278, 0x0228);
	write_cmos_sensor(0x227a, 0x40b2);
	write_cmos_sensor(0x227c, 0x09fb);
	write_cmos_sensor(0x227e, 0x0232);
	write_cmos_sensor(0x2280, 0x4291);
	write_cmos_sensor(0x2282, 0x0422);
	write_cmos_sensor(0x2284, 0x0000);
	write_cmos_sensor(0x2286, 0x421f);
	write_cmos_sensor(0x2288, 0x0424);
	write_cmos_sensor(0x228a, 0x812f);
	write_cmos_sensor(0x228c, 0x4f81);
	write_cmos_sensor(0x228e, 0x0002);
	write_cmos_sensor(0x2290, 0x4291);
	write_cmos_sensor(0x2292, 0x8248);
	write_cmos_sensor(0x2294, 0x0008);
	write_cmos_sensor(0x2296, 0x4214);
	write_cmos_sensor(0x2298, 0x0310);
	write_cmos_sensor(0x229a, 0x421a);
	write_cmos_sensor(0x229c, 0x82a0);
	write_cmos_sensor(0x229e, 0xf80a);
	write_cmos_sensor(0x22a0, 0x421b);
	write_cmos_sensor(0x22a2, 0x82a2);
	write_cmos_sensor(0x22a4, 0xf80b);
	write_cmos_sensor(0x22a6, 0x4382);
	write_cmos_sensor(0x22a8, 0x7334);
	write_cmos_sensor(0x22aa, 0x0f00);
	write_cmos_sensor(0x22ac, 0x7304);
	write_cmos_sensor(0x22ae, 0x4192);
	write_cmos_sensor(0x22b0, 0x0008);
	write_cmos_sensor(0x22b2, 0x0a08);
	write_cmos_sensor(0x22b4, 0x4382);
	write_cmos_sensor(0x22b6, 0x040c);
	write_cmos_sensor(0x22b8, 0x4305);
	write_cmos_sensor(0x22ba, 0x9382);
	write_cmos_sensor(0x22bc, 0x7112);
	write_cmos_sensor(0x22be, 0x2001);
	write_cmos_sensor(0x22c0, 0x4315);
	write_cmos_sensor(0x22c2, 0x421e);
	write_cmos_sensor(0x22c4, 0x7100);
	write_cmos_sensor(0x22c6, 0xb2f2);
	write_cmos_sensor(0x22c8, 0x0261);
	write_cmos_sensor(0x22ca, 0x2406);
	write_cmos_sensor(0x22cc, 0xb3d2);
	write_cmos_sensor(0x22ce, 0x0b02);
	write_cmos_sensor(0x22d0, 0x2403);
	write_cmos_sensor(0x22d2, 0x42d2);
	write_cmos_sensor(0x22d4, 0x0809);
	write_cmos_sensor(0x22d6, 0x0b00);
	write_cmos_sensor(0x22d8, 0x40b2);
	write_cmos_sensor(0x22da, 0x00b6);
	write_cmos_sensor(0x22dc, 0x7334);
	write_cmos_sensor(0x22de, 0x0f00);
	write_cmos_sensor(0x22e0, 0x7304);
	write_cmos_sensor(0x22e2, 0x4482);
	write_cmos_sensor(0x22e4, 0x0a08);
	write_cmos_sensor(0x22e6, 0xb2e2);
	write_cmos_sensor(0x22e8, 0x0b05);
	write_cmos_sensor(0x22ea, 0x2404);
	write_cmos_sensor(0x22ec, 0x4392);
	write_cmos_sensor(0x22ee, 0x7a0e);
	write_cmos_sensor(0x22f0, 0x0800);
	write_cmos_sensor(0x22f2, 0x7a10);
	write_cmos_sensor(0x22f4, 0xf80e);
	write_cmos_sensor(0x22f6, 0x93c2);
	write_cmos_sensor(0x22f8, 0x82de);
	write_cmos_sensor(0x22fa, 0x2468);
	write_cmos_sensor(0x22fc, 0x9e0a);
	write_cmos_sensor(0x22fe, 0x2803);
	write_cmos_sensor(0x2300, 0x9349);
	write_cmos_sensor(0x2302, 0x2001);
	write_cmos_sensor(0x2304, 0x4359);
	write_cmos_sensor(0x2306, 0x9e0b);
	write_cmos_sensor(0x2308, 0x2802);
	write_cmos_sensor(0x230a, 0x9369);
	write_cmos_sensor(0x230c, 0x245c);
	write_cmos_sensor(0x230e, 0x421f);
	write_cmos_sensor(0x2310, 0x731a);
	write_cmos_sensor(0x2312, 0xc312);
	write_cmos_sensor(0x2314, 0x100f);
	write_cmos_sensor(0x2316, 0x4f82);
	write_cmos_sensor(0x2318, 0x7334);
	write_cmos_sensor(0x231a, 0x0f00);
	write_cmos_sensor(0x231c, 0x7304);
	write_cmos_sensor(0x231e, 0x4192);
	write_cmos_sensor(0x2320, 0x0008);
	write_cmos_sensor(0x2322, 0x0a08);
	write_cmos_sensor(0x2324, 0x421e);
	write_cmos_sensor(0x2326, 0x7100);
	write_cmos_sensor(0x2328, 0x812e);
	write_cmos_sensor(0x232a, 0x425c);
	write_cmos_sensor(0x232c, 0x0419);
	write_cmos_sensor(0x232e, 0x537c);
	write_cmos_sensor(0x2330, 0xfe4c);
	write_cmos_sensor(0x2332, 0x9305);
	write_cmos_sensor(0x2334, 0x2003);
	write_cmos_sensor(0x2336, 0x40b2);
	write_cmos_sensor(0x2338, 0x0c78);
	write_cmos_sensor(0x233a, 0x7100);
	write_cmos_sensor(0x233c, 0x421f);
	write_cmos_sensor(0x233e, 0x731a);
	write_cmos_sensor(0x2340, 0xc312);
	write_cmos_sensor(0x2342, 0x100f);
	write_cmos_sensor(0x2344, 0x503f);
	write_cmos_sensor(0x2346, 0x00b6);
	write_cmos_sensor(0x2348, 0x4f82);
	write_cmos_sensor(0x234a, 0x7334);
	write_cmos_sensor(0x234c, 0x0f00);
	write_cmos_sensor(0x234e, 0x7304);
	write_cmos_sensor(0x2350, 0x4482);
	write_cmos_sensor(0x2352, 0x0a08);
	write_cmos_sensor(0x2354, 0x9e81);
	write_cmos_sensor(0x2356, 0x0002);
	write_cmos_sensor(0x2358, 0x2814);
	write_cmos_sensor(0x235a, 0xf74c);
	write_cmos_sensor(0x235c, 0x434d);
	write_cmos_sensor(0x235e, 0x411f);
	write_cmos_sensor(0x2360, 0x0004);
	write_cmos_sensor(0x2362, 0x4f1e);
	write_cmos_sensor(0x2364, 0x0002);
	write_cmos_sensor(0x2366, 0x9381);
	write_cmos_sensor(0x2368, 0x0006);
	write_cmos_sensor(0x236a, 0x240b);
	write_cmos_sensor(0x236c, 0x4e6f);
	write_cmos_sensor(0x236e, 0xf74f);
	write_cmos_sensor(0x2370, 0x9c4f);
	write_cmos_sensor(0x2372, 0x2423);
	write_cmos_sensor(0x2374, 0x535d);
	write_cmos_sensor(0x2376, 0x503e);
	write_cmos_sensor(0x2378, 0x0006);
	write_cmos_sensor(0x237a, 0x4d4f);
	write_cmos_sensor(0x237c, 0x911f);
	write_cmos_sensor(0x237e, 0x0006);
	write_cmos_sensor(0x2380, 0x2bf5);
	write_cmos_sensor(0x2382, 0x9359);
	write_cmos_sensor(0x2384, 0x2403);
	write_cmos_sensor(0x2386, 0x9079);
	write_cmos_sensor(0x2388, 0x0003);
	write_cmos_sensor(0x238a, 0x2028);
	write_cmos_sensor(0x238c, 0x434d);
	write_cmos_sensor(0x238e, 0x464f);
	write_cmos_sensor(0x2390, 0x5f0f);
	write_cmos_sensor(0x2392, 0x5f0f);
	write_cmos_sensor(0x2394, 0x4f9f);
	write_cmos_sensor(0x2396, 0x2dfc);
	write_cmos_sensor(0x2398, 0x8020);
	write_cmos_sensor(0x239a, 0x4f9f);
	write_cmos_sensor(0x239c, 0x2dfe);
	write_cmos_sensor(0x239e, 0x8022);
	write_cmos_sensor(0x23a0, 0x5356);
	write_cmos_sensor(0x23a2, 0x9076);
	write_cmos_sensor(0x23a4, 0x0040);
	write_cmos_sensor(0x23a6, 0x2407);
	write_cmos_sensor(0x23a8, 0x9076);
	write_cmos_sensor(0x23aa, 0xff80);
	write_cmos_sensor(0x23ac, 0x2404);
	write_cmos_sensor(0x23ae, 0x535d);
	write_cmos_sensor(0x23b0, 0x926d);
	write_cmos_sensor(0x23b2, 0x2bed);
	write_cmos_sensor(0x23b4, 0x3c13);
	write_cmos_sensor(0x23b6, 0x5359);
	write_cmos_sensor(0x23b8, 0x3c11);
	write_cmos_sensor(0x23ba, 0x4ea2);
	write_cmos_sensor(0x23bc, 0x040c);
	write_cmos_sensor(0x23be, 0x4e92);
	write_cmos_sensor(0x23c0, 0x0002);
	write_cmos_sensor(0x23c2, 0x040e);
	write_cmos_sensor(0x23c4, 0x3fde);
	write_cmos_sensor(0x23c6, 0x4079);
	write_cmos_sensor(0x23c8, 0x0003);
	write_cmos_sensor(0x23ca, 0x3fa1);
	write_cmos_sensor(0x23cc, 0x9a0e);
	write_cmos_sensor(0x23ce, 0x2803);
	write_cmos_sensor(0x23d0, 0x9349);
	write_cmos_sensor(0x23d2, 0x2001);
	write_cmos_sensor(0x23d4, 0x4359);
	write_cmos_sensor(0x23d6, 0x9b0e);
	write_cmos_sensor(0x23d8, 0x2b9a);
	write_cmos_sensor(0x23da, 0x3f97);
	write_cmos_sensor(0x23dc, 0x9305);
	write_cmos_sensor(0x23de, 0x2363);
	write_cmos_sensor(0x23e0, 0x5031);
	write_cmos_sensor(0x23e2, 0x000a);
	write_cmos_sensor(0x23e4, 0x4134);
	write_cmos_sensor(0x23e6, 0x4135);
	write_cmos_sensor(0x23e8, 0x4136);
	write_cmos_sensor(0x23ea, 0x4137);
	write_cmos_sensor(0x23ec, 0x4138);
	write_cmos_sensor(0x23ee, 0x4139);
	write_cmos_sensor(0x23f0, 0x413a);
	write_cmos_sensor(0x23f2, 0x413b);
	write_cmos_sensor(0x23f4, 0x4130);
	write_cmos_sensor(0x23f6, 0x120b);
	write_cmos_sensor(0x23f8, 0x120a);
	write_cmos_sensor(0x23fa, 0x1209);
	write_cmos_sensor(0x23fc, 0x1208);
	write_cmos_sensor(0x23fe, 0x1207);
	write_cmos_sensor(0x2400, 0x1206);
	write_cmos_sensor(0x2402, 0x1205);
	write_cmos_sensor(0x2404, 0x1204);
	write_cmos_sensor(0x2406, 0x8221);
	write_cmos_sensor(0x2408, 0x425f);
	write_cmos_sensor(0x240a, 0x0600);
	write_cmos_sensor(0x240c, 0xf35f);
	write_cmos_sensor(0x240e, 0x4fc1);
	write_cmos_sensor(0x2410, 0x0002);
	write_cmos_sensor(0x2412, 0x43c1);
	write_cmos_sensor(0x2414, 0x0003);
	write_cmos_sensor(0x2416, 0x403f);
	write_cmos_sensor(0x2418, 0x0603);
	write_cmos_sensor(0x241a, 0x4fe1);
	write_cmos_sensor(0x241c, 0x0000);
	write_cmos_sensor(0x241e, 0xb3ef);
	write_cmos_sensor(0x2420, 0x0000);
	write_cmos_sensor(0x2422, 0x2431);
	write_cmos_sensor(0x2424, 0x4344);
	write_cmos_sensor(0x2426, 0x4445);
	write_cmos_sensor(0x2428, 0x450f);
	write_cmos_sensor(0x242a, 0x5f0f);
	write_cmos_sensor(0x242c, 0x5f0f);
	write_cmos_sensor(0x242e, 0x403d);
	write_cmos_sensor(0x2430, 0x000e);
	write_cmos_sensor(0x2432, 0x4f1e);
	write_cmos_sensor(0x2434, 0x0632);
	write_cmos_sensor(0x2436, 0x4f1f);
	write_cmos_sensor(0x2438, 0x0634);
	write_cmos_sensor(0x243a, 0x12b0);
	write_cmos_sensor(0x243c, 0xc62c);
	write_cmos_sensor(0x243e, 0x4e08);
	write_cmos_sensor(0x2440, 0x4f09);
	write_cmos_sensor(0x2442, 0x421e);
	write_cmos_sensor(0x2444, 0xf00c);
	write_cmos_sensor(0x2446, 0x430f);
	write_cmos_sensor(0x2448, 0x480a);
	write_cmos_sensor(0x244a, 0x490b);
	write_cmos_sensor(0x244c, 0x4e0c);
	write_cmos_sensor(0x244e, 0x4f0d);
	write_cmos_sensor(0x2450, 0x12b0);
	write_cmos_sensor(0x2452, 0xdf96);
	write_cmos_sensor(0x2454, 0x421a);
	write_cmos_sensor(0x2456, 0xf00e);
	write_cmos_sensor(0x2458, 0x430b);
	write_cmos_sensor(0x245a, 0x403d);
	write_cmos_sensor(0x245c, 0x0009);
	write_cmos_sensor(0x245e, 0x12b0);
	write_cmos_sensor(0x2460, 0xc62c);
	write_cmos_sensor(0x2462, 0x4e06);
	write_cmos_sensor(0x2464, 0x4f07);
	write_cmos_sensor(0x2466, 0x5a06);
	write_cmos_sensor(0x2468, 0x6b07);
	write_cmos_sensor(0x246a, 0x425f);
	write_cmos_sensor(0x246c, 0x0668);
	write_cmos_sensor(0x246e, 0xf37f);
	write_cmos_sensor(0x2470, 0x9f08);
	write_cmos_sensor(0x2472, 0x2c6b);
	write_cmos_sensor(0x2474, 0x4216);
	write_cmos_sensor(0x2476, 0x06ca);
	write_cmos_sensor(0x2478, 0x4307);
	write_cmos_sensor(0x247a, 0x5505);
	write_cmos_sensor(0x247c, 0x4685);
	write_cmos_sensor(0x247e, 0x065e);
	write_cmos_sensor(0x2480, 0x5354);
	write_cmos_sensor(0x2482, 0x9264);
	write_cmos_sensor(0x2484, 0x2bd0);
	write_cmos_sensor(0x2486, 0x403b);
	write_cmos_sensor(0x2488, 0x0603);
	write_cmos_sensor(0x248a, 0x416f);
	write_cmos_sensor(0x248c, 0xc36f);
	write_cmos_sensor(0x248e, 0x4fcb);
	write_cmos_sensor(0x2490, 0x0000);
	write_cmos_sensor(0x2492, 0x12b0);
	write_cmos_sensor(0x2494, 0xcd42);
	write_cmos_sensor(0x2496, 0x41eb);
	write_cmos_sensor(0x2498, 0x0000);
	write_cmos_sensor(0x249a, 0x421f);
	write_cmos_sensor(0x249c, 0x0256);
	write_cmos_sensor(0x249e, 0x522f);
	write_cmos_sensor(0x24a0, 0x421b);
	write_cmos_sensor(0x24a2, 0xf008);
	write_cmos_sensor(0x24a4, 0x532b);
	write_cmos_sensor(0x24a6, 0x9f0b);
	write_cmos_sensor(0x24a8, 0x2c01);
	write_cmos_sensor(0x24aa, 0x4f0b);
	write_cmos_sensor(0x24ac, 0x9381);
	write_cmos_sensor(0x24ae, 0x0002);
	write_cmos_sensor(0x24b0, 0x2409);
	write_cmos_sensor(0x24b2, 0x430a);
	write_cmos_sensor(0x24b4, 0x421e);
	write_cmos_sensor(0x24b6, 0x0614);
	write_cmos_sensor(0x24b8, 0x503e);
	write_cmos_sensor(0x24ba, 0x000a);
	write_cmos_sensor(0x24bc, 0x421f);
	write_cmos_sensor(0x24be, 0x0680);
	write_cmos_sensor(0x24c0, 0x9f0e);
	write_cmos_sensor(0x24c2, 0x2801);
	write_cmos_sensor(0x24c4, 0x431a);
	write_cmos_sensor(0x24c6, 0xb0b2);
	write_cmos_sensor(0x24c8, 0x0020);
	write_cmos_sensor(0x24ca, 0xf002);
	write_cmos_sensor(0x24cc, 0x241f);
	write_cmos_sensor(0x24ce, 0x93c2);
	write_cmos_sensor(0x24d0, 0x82cc);
	write_cmos_sensor(0x24d2, 0x201c);
	write_cmos_sensor(0x24d4, 0x4b0e);
	write_cmos_sensor(0x24d6, 0x430f);
	write_cmos_sensor(0x24d8, 0x521e);
	write_cmos_sensor(0x24da, 0x7300);
	write_cmos_sensor(0x24dc, 0x621f);
	write_cmos_sensor(0x24de, 0x7302);
	write_cmos_sensor(0x24e0, 0x421c);
	write_cmos_sensor(0x24e2, 0x7316);
	write_cmos_sensor(0x24e4, 0x421d);
	write_cmos_sensor(0x24e6, 0x7318);
	write_cmos_sensor(0x24e8, 0x8c0e);
	write_cmos_sensor(0x24ea, 0x7d0f);
	write_cmos_sensor(0x24ec, 0x2c0f);
	write_cmos_sensor(0x24ee, 0x930a);
	write_cmos_sensor(0x24f0, 0x240d);
	write_cmos_sensor(0x24f2, 0x421f);
	write_cmos_sensor(0x24f4, 0x8248);
	write_cmos_sensor(0x24f6, 0xf03f);
	write_cmos_sensor(0x24f8, 0xf7ff);
	write_cmos_sensor(0x24fa, 0x4f82);
	write_cmos_sensor(0x24fc, 0x0a08);
	write_cmos_sensor(0x24fe, 0x0c10);
	write_cmos_sensor(0x2500, 0x421f);
	write_cmos_sensor(0x2502, 0x8252);
	write_cmos_sensor(0x2504, 0xd03f);
	write_cmos_sensor(0x2506, 0x00c0);
	write_cmos_sensor(0x2508, 0x4f82);
	write_cmos_sensor(0x250a, 0x0a12);
	write_cmos_sensor(0x250c, 0x4b0a);
	write_cmos_sensor(0x250e, 0x430b);
	write_cmos_sensor(0x2510, 0x421e);
	write_cmos_sensor(0x2512, 0x7300);
	write_cmos_sensor(0x2514, 0x421f);
	write_cmos_sensor(0x2516, 0x7302);
	write_cmos_sensor(0x2518, 0x5a0e);
	write_cmos_sensor(0x251a, 0x6b0f);
	write_cmos_sensor(0x251c, 0x421c);
	write_cmos_sensor(0x251e, 0x7316);
	write_cmos_sensor(0x2520, 0x421d);
	write_cmos_sensor(0x2522, 0x7318);
	write_cmos_sensor(0x2524, 0x8c0e);
	write_cmos_sensor(0x2526, 0x7d0f);
	write_cmos_sensor(0x2528, 0x2c1a);
	write_cmos_sensor(0x252a, 0x0900);
	write_cmos_sensor(0x252c, 0x7312);
	write_cmos_sensor(0x252e, 0x421e);
	write_cmos_sensor(0x2530, 0x7300);
	write_cmos_sensor(0x2532, 0x421f);
	write_cmos_sensor(0x2534, 0x7302);
	write_cmos_sensor(0x2536, 0x5a0e);
	write_cmos_sensor(0x2538, 0x6b0f);
	write_cmos_sensor(0x253a, 0x421c);
	write_cmos_sensor(0x253c, 0x7316);
	write_cmos_sensor(0x253e, 0x421d);
	write_cmos_sensor(0x2540, 0x7318);
	write_cmos_sensor(0x2542, 0x8c0e);
	write_cmos_sensor(0x2544, 0x7d0f);
	write_cmos_sensor(0x2546, 0x2bf1);
	write_cmos_sensor(0x2548, 0x3c0a);
	write_cmos_sensor(0x254a, 0x460e);
	write_cmos_sensor(0x254c, 0x470f);
	write_cmos_sensor(0x254e, 0x803e);
	write_cmos_sensor(0x2550, 0x0800);
	write_cmos_sensor(0x2552, 0x730f);
	write_cmos_sensor(0x2554, 0x2b92);
	write_cmos_sensor(0x2556, 0x4036);
	write_cmos_sensor(0x2558, 0x07ff);
	write_cmos_sensor(0x255a, 0x4307);
	write_cmos_sensor(0x255c, 0x3f8e);
	write_cmos_sensor(0x255e, 0x5221);
	write_cmos_sensor(0x2560, 0x4134);
	write_cmos_sensor(0x2562, 0x4135);
	write_cmos_sensor(0x2564, 0x4136);
	write_cmos_sensor(0x2566, 0x4137);
	write_cmos_sensor(0x2568, 0x4138);
	write_cmos_sensor(0x256a, 0x4139);
	write_cmos_sensor(0x256c, 0x413a);
	write_cmos_sensor(0x256e, 0x413b);
	write_cmos_sensor(0x2570, 0x4130);
	write_cmos_sensor(0x2572, 0x7400);
	write_cmos_sensor(0x2574, 0x2003);
	write_cmos_sensor(0x2576, 0x72a1);
	write_cmos_sensor(0x2578, 0x2f00);
	write_cmos_sensor(0x257a, 0x7020);
	write_cmos_sensor(0x257c, 0x2f21);
	write_cmos_sensor(0x257e, 0x7800);
	write_cmos_sensor(0x2580, 0x0040);
	write_cmos_sensor(0x2582, 0x7400);
	write_cmos_sensor(0x2584, 0x2005);
	write_cmos_sensor(0x2586, 0x72a1);
	write_cmos_sensor(0x2588, 0x2f00);
	write_cmos_sensor(0x258a, 0x7020);
	write_cmos_sensor(0x258c, 0x2f22);
	write_cmos_sensor(0x258e, 0x7800);
	write_cmos_sensor(0x2590, 0x7400);
	write_cmos_sensor(0x2592, 0x2011);
	write_cmos_sensor(0x2594, 0x72a1);
	write_cmos_sensor(0x2596, 0x2f00);
	write_cmos_sensor(0x2598, 0x7020);
	write_cmos_sensor(0x259a, 0x2f21);
	write_cmos_sensor(0x259c, 0x7800);
	write_cmos_sensor(0x259e, 0x7400);
	write_cmos_sensor(0x25a0, 0x2009);
	write_cmos_sensor(0x25a2, 0x72a1);
	write_cmos_sensor(0x25a4, 0x2f1f);
	write_cmos_sensor(0x25a6, 0x7021);
	write_cmos_sensor(0x25a8, 0x3f40);
	write_cmos_sensor(0x25aa, 0x7800);
	write_cmos_sensor(0x25ac, 0x7400);
	write_cmos_sensor(0x25ae, 0x2005);
	write_cmos_sensor(0x25b0, 0x72a1);
	write_cmos_sensor(0x25b2, 0x2f1f);
	write_cmos_sensor(0x25b4, 0x7021);
	write_cmos_sensor(0x25b6, 0x3f40);
	write_cmos_sensor(0x25b8, 0x7800);
	write_cmos_sensor(0x25ba, 0x7400);
	write_cmos_sensor(0x25bc, 0x2009);
	write_cmos_sensor(0x25be, 0x72a1);
	write_cmos_sensor(0x25c0, 0x2f00);
	write_cmos_sensor(0x25c2, 0x7020);
	write_cmos_sensor(0x25c4, 0x2f22);
	write_cmos_sensor(0x25c6, 0x7800);
	write_cmos_sensor(0x25c8, 0x0009);
	write_cmos_sensor(0x25ca, 0xf572);
	write_cmos_sensor(0x25cc, 0x0009);
	write_cmos_sensor(0x25ce, 0xf582);
	write_cmos_sensor(0x25d0, 0x0009);
	write_cmos_sensor(0x25d2, 0xf590);
	write_cmos_sensor(0x25d4, 0x0009);
	write_cmos_sensor(0x25d6, 0xf59e);
	write_cmos_sensor(0x25d8, 0xf580);
	write_cmos_sensor(0x25da, 0x0004);
	write_cmos_sensor(0x25dc, 0x0009);
	write_cmos_sensor(0x25de, 0xf590);
	write_cmos_sensor(0x25e0, 0x0009);
	write_cmos_sensor(0x25e2, 0xf5ba);
	write_cmos_sensor(0x25e4, 0x0009);
	write_cmos_sensor(0x25e6, 0xf572);
	write_cmos_sensor(0x25e8, 0x0009);
	write_cmos_sensor(0x25ea, 0xf5ac);
	write_cmos_sensor(0x25ec, 0xf580);
	write_cmos_sensor(0x25ee, 0x0004);
	write_cmos_sensor(0x25f0, 0x0009);
	write_cmos_sensor(0x25f2, 0xf572);
	write_cmos_sensor(0x25f4, 0x0009);
	write_cmos_sensor(0x25f6, 0xf5ac);
	write_cmos_sensor(0x25f8, 0x0009);
	write_cmos_sensor(0x25fa, 0xf590);
	write_cmos_sensor(0x25fc, 0x0009);
	write_cmos_sensor(0x25fe, 0xf59e);
	write_cmos_sensor(0x2600, 0xf580);
	write_cmos_sensor(0x2602, 0x0004);
	write_cmos_sensor(0x2604, 0x0009);
	write_cmos_sensor(0x2606, 0xf590);
	write_cmos_sensor(0x2608, 0x0009);
	write_cmos_sensor(0x260a, 0xf59e);
	write_cmos_sensor(0x260c, 0x0009);
	write_cmos_sensor(0x260e, 0xf572);
	write_cmos_sensor(0x2610, 0x0009);
	write_cmos_sensor(0x2612, 0xf5ac);
	write_cmos_sensor(0x2614, 0xf580);
	write_cmos_sensor(0x2616, 0x0004);
	write_cmos_sensor(0x2618, 0x0212);
	write_cmos_sensor(0x261a, 0x0217);
	write_cmos_sensor(0x261c, 0x041f);
	write_cmos_sensor(0x261e, 0x1017);
	write_cmos_sensor(0x2620, 0x0413);
	write_cmos_sensor(0x2622, 0x0103);
	write_cmos_sensor(0x2624, 0x010b);
	write_cmos_sensor(0x2626, 0x1c0a);
	write_cmos_sensor(0x2628, 0x0202);
	write_cmos_sensor(0x262a, 0x0407);
	write_cmos_sensor(0x262c, 0x0205);
	write_cmos_sensor(0x262e, 0x0204);
	write_cmos_sensor(0x2630, 0x0114);
	write_cmos_sensor(0x2632, 0x0110);
	write_cmos_sensor(0x2634, 0xffff);
	write_cmos_sensor(0x2636, 0x0048);
	write_cmos_sensor(0x2638, 0x0090);
	write_cmos_sensor(0x263a, 0x0000);
	write_cmos_sensor(0x263c, 0x0000);
	write_cmos_sensor(0x263e, 0xf618);
	write_cmos_sensor(0x2640, 0x0000);
	write_cmos_sensor(0x2642, 0x0000);
	write_cmos_sensor(0x2644, 0x0060);
	write_cmos_sensor(0x2646, 0x0078);
	write_cmos_sensor(0x2648, 0x0060);
	write_cmos_sensor(0x264a, 0x0078);
	write_cmos_sensor(0x264c, 0x004f);
	write_cmos_sensor(0x264e, 0x0037);
	write_cmos_sensor(0x2650, 0x0048);
	write_cmos_sensor(0x2652, 0x0090);
	write_cmos_sensor(0x2654, 0x0000);
	write_cmos_sensor(0x2656, 0x0000);
	write_cmos_sensor(0x2658, 0xf618);
	write_cmos_sensor(0x265a, 0x0000);
	write_cmos_sensor(0x265c, 0x0000);
	write_cmos_sensor(0x265e, 0x0180);
	write_cmos_sensor(0x2660, 0x0780);
	write_cmos_sensor(0x2662, 0x0180);
	write_cmos_sensor(0x2664, 0x0780);
	write_cmos_sensor(0x2666, 0x04cf);
	write_cmos_sensor(0x2668, 0x0337);
	write_cmos_sensor(0x266a, 0xf636);
	write_cmos_sensor(0x266c, 0xf650);
	write_cmos_sensor(0x266e, 0xf5c8);
	write_cmos_sensor(0x2670, 0xf5dc);
	write_cmos_sensor(0x2672, 0xf5f0);
	write_cmos_sensor(0x2674, 0xf604);
	write_cmos_sensor(0x2676, 0x0100);
	write_cmos_sensor(0x2678, 0xff8a);
	write_cmos_sensor(0x267a, 0xffff);
	write_cmos_sensor(0x267c, 0x0104);
	write_cmos_sensor(0x267e, 0xff0a);
	write_cmos_sensor(0x2680, 0xffff);
	write_cmos_sensor(0x2682, 0x0108);
	write_cmos_sensor(0x2684, 0xff02);
	write_cmos_sensor(0x2686, 0xffff);
	write_cmos_sensor(0x2688, 0x010c);
	write_cmos_sensor(0x268a, 0xff82);
	write_cmos_sensor(0x268c, 0xffff);
	write_cmos_sensor(0x268e, 0x0004);
	write_cmos_sensor(0x2690, 0xf676);
	write_cmos_sensor(0x2692, 0xe4e4);
	write_cmos_sensor(0x2694, 0x4e4e);
	write_cmos_sensor(0x2ffe, 0xc114);
	write_cmos_sensor(0x3224, 0xf222);
	write_cmos_sensor(0x322a, 0xf23c);
	write_cmos_sensor(0x3230, 0xf03a);
	write_cmos_sensor(0x3238, 0xf09a);
	write_cmos_sensor(0x323a, 0xf012);
	write_cmos_sensor(0x323e, 0xf3f6);
	write_cmos_sensor(0x32a0, 0x0000);
	write_cmos_sensor(0x32a2, 0x0000);
	write_cmos_sensor(0x32a4, 0x0000);
	write_cmos_sensor(0x32b0, 0x0000);
	write_cmos_sensor(0x32c0, 0xf66a);
	write_cmos_sensor(0x32c2, 0xf66e);
	write_cmos_sensor(0x32c4, 0x0000);
	write_cmos_sensor(0x32c6, 0xf66e);
	write_cmos_sensor(0x32c8, 0x0000);
	write_cmos_sensor(0x32ca, 0xf68e);
	//----- Initial -----//
	write_cmos_sensor(0x0a7e, 0x219c); //sreg write flag 
	write_cmos_sensor(0x3244, 0x8400); //sreg2 - PCP ON
	write_cmos_sensor(0x3246, 0xe400); //sreg3 - NCP ON
	write_cmos_sensor(0x3248, 0xc88e); //sreg4 - CDS
	write_cmos_sensor(0x324e, 0xfcd8); //sreg7 - RAMP
	write_cmos_sensor(0x3250, 0xa060); //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	write_cmos_sensor(0x325a, 0x7a37); //sreg13- BGR
	write_cmos_sensor(0x0734, 0x4b0b); //pll_cfg_ramp_a PLL_CLK=600mhz b7-6:00_tg_div2(1/1) 
	write_cmos_sensor(0x0736, 0xd8b0); //pll_cfg_ramp_b b13-11:011_tg_div1(1/4) b1-0:00_ramp_div(1/1) 
	write_cmos_sensor(0x0600, 0x1190); //blc00 - b4:dpc_en b0:blc_en, b7:bwi_en, b4:act_ofs b0:tobp_ofs
	write_cmos_sensor(0x0602, 0x0052); //blc01 - b8:hdr_en, b4:dither_en, b1:dither_nr_en
	write_cmos_sensor(0x0604, 0x1008); //blc02 - b15:obp bypass, b8: dsc_en, b3:g_sum_en
	write_cmos_sensor(0x0606, 0x0200); //blc_dig_offset
	write_cmos_sensor(0x0616, 0x0040); //act_rr_offset
	write_cmos_sensor(0x0614, 0x0040); //act_gr_offset
	write_cmos_sensor(0x0612, 0x0040); //act_gb_offset
	write_cmos_sensor(0x0610, 0x0040); //act_bb_offset
	write_cmos_sensor(0x06b2, 0x0500); //obp multi-window number
	write_cmos_sensor(0x06b4, 0x3ff0); //obp multi-window threshold(10.4f)
	write_cmos_sensor(0x0618, 0x0a80); //blc_pdaf_ctrl b12:PD active offset off b10:PD OBP average off, b8:PD digital offset off 
	write_cmos_sensor(0x0668, 0x4302); //blc_spare b15:8:blc nr weight threshold, b2:obp_range_scaler, b1:0:blc_dpc_mode 
	write_cmos_sensor(0x06ca, 0x02cc); //blc_nr_weight b10:0 nr_weight = 50% 
	write_cmos_sensor(0x066e, 0x0050); //blc_dpc_th_high_level b10:0 obp_dpc_threshold high = +80 
	write_cmos_sensor(0x0670, 0x0050); //blc_dpc_th_low_level b10:0 obp_dpc_threshold low = -80 
	write_cmos_sensor(0x113c, 0x0001); //LSC en_fall_off B[15:8]en_fall_off, B[7:0]loc_add: add location for lsc block distance 
	write_cmos_sensor(0x11c4, 0x1080); //LSC image width
	write_cmos_sensor(0x11c6, 0x0c34); //LSC image height
	write_cmos_sensor(0x1104, 0x0160); //LSC block width x/y
	write_cmos_sensor(0x1106, 0x0138); //LSC block height x/y
	write_cmos_sensor(0x110a, 0x010e); //LSC inv_col02 h/l 
	write_cmos_sensor(0x110c, 0x021d); //LSC inv_col1 h/l 
	write_cmos_sensor(0x110e, 0xba2e); //LSC inv_col35 h/l 
	write_cmos_sensor(0x1110, 0x0056); //LSC inv_row02 h/l 
	write_cmos_sensor(0x1112, 0x00ac); //LSC inv_row1 h/l 
	write_cmos_sensor(0x1114, 0x6907); //LSC inv_row35 h/l 
	write_cmos_sensor(0x1122, 0x0011); //LSC inv_pd_col02 h/l 
	write_cmos_sensor(0x1124, 0x0022); //LSC inv_pd_col1 h/l 
	write_cmos_sensor(0x1126, 0x2e8c); //LSC inv_pd_col35 h/l 
	write_cmos_sensor(0x1128, 0x0016); //LSC inv_pd_row02 h/l 
	write_cmos_sensor(0x112a, 0x002b); //LSC inv_pd_row1 h/l 
	write_cmos_sensor(0x112c, 0x3483); //LSC inv_pd_row35 h/l 
	write_cmos_sensor(0x1130, 0x0200); //LSC lpd gain 1x
	write_cmos_sensor(0x1132, 0x0200); //LSC rpd gain 1x
	write_cmos_sensor(0x1102, 0x0028); //LSC lsc_ctl3 B[5]: div 512 for PD, B[3]: pd pedestal enable
	write_cmos_sensor(0x113e, 0x0200); //LSC pd_usr_gain B[11:0]: pd user gain 1x
	write_cmos_sensor(0x0d00, 0x4000); //ADPC ctl
	write_cmos_sensor(0x0d02, 0x8004); //ADPC col offset
	write_cmos_sensor(0x120a, 0x0a00); //PDPC boundary_trim : OBP_ON, Vcrop ON 
	write_cmos_sensor(0x0214, 0x0200); //Digital Gain GR - x1
	write_cmos_sensor(0x0216, 0x0200); //Digital Gain GB - x1
	write_cmos_sensor(0x0218, 0x0200); //Digital Gain R  - x1
	write_cmos_sensor(0x021a, 0x0200); //Digital Gain B  - x1
	write_cmos_sensor(0x1000, 0x0300); //mipi_tx_ctrl_h, mipi_tx_test_mode
	write_cmos_sensor(0x1002, 0xc319); //mipi_tx_op_mode_h, mipi_tx_op_mode_l
	//write_cmos_sensor(0x1004, 0x2bab); //mipi_data_id_ctrl, mipi_pd_data_id_ctrl
	write_cmos_sensor(0x1004, 0x2bb0); //mipi_data_id_ctrl, mipi_pd_data_id_ctrl
	write_cmos_sensor(0x105a, 0x0091); //mipi_static0
	write_cmos_sensor(0x105c, 0x0f08); //mipi_static1
	write_cmos_sensor(0x105e, 0x0000); //mipi_static2
	write_cmos_sensor(0x1060, 0x3008); //mipi_static3
	write_cmos_sensor(0x1062, 0x0000); //mipi_static4
	write_cmos_sensor(0x0202, 0x0100); //image orient
	write_cmos_sensor(0x0b10, 0x400c); //data pedestal - on
	write_cmos_sensor(0x0212, 0x0000); //analog gain
	write_cmos_sensor(0x035e, 0x0701); //d2a_pxl_drv_pwr_hv, d2a_bgr_en
	write_cmos_sensor(0x040a, 0x0000); //pdaf_control
	write_cmos_sensor(0x0420, 0x0003); //pdaf start pixel count offset
	write_cmos_sensor(0x0424, 0x0c47); //pdaf_y_end
	write_cmos_sensor(0x0418, 0x1010); //pdaf_block_size
	write_cmos_sensor(0x0740, 0x004f); //temp sensor enable
	write_cmos_sensor(0x0354, 0x1000); //ramp dither enable
	write_cmos_sensor(0x035c, 0x0303); //d2a_row_tx_drv_option_en, d2a_row_rx_drv_option_en
	write_cmos_sensor(0x050e, 0x0000); //readout_ramp_pofs_0
	write_cmos_sensor(0x0510, 0x0058); //readout_ramp_pofs_1
	write_cmos_sensor(0x0512, 0x0058); //readout_ramp_pofs_2
	write_cmos_sensor(0x0514, 0x0050); //readout_ramp_pofs_3
	write_cmos_sensor(0x0516, 0x0050); //readout_ramp_pofs_4
	write_cmos_sensor(0x0260, 0x0003); //TG ctrl b15:8:otp mode, b3:mode_sel remap, b1:ADCO start pcnt set, b0:SREG reload 
	write_cmos_sensor(0x0262, 0x0700); //BLC gain and int. time changed frame ctrl
	write_cmos_sensor(0x0266, 0x0007); //OTP ctrl b7:pd_lsc_flag_err, b6:pd_lsc_chksum_err, b5:lsc_flag_err b4:lsc_checksum_err b2:pdlsc_en b1:lsc_en b0:adpc_en 
	write_cmos_sensor(0x0250, 0x0000); //Fsync Enable b9:fsync_slave_mode, b8:fsync_enable 
	write_cmos_sensor(0x0258, 0x0002); //FSYNC vs adj. mode rising line count 
	write_cmos_sensor(0x025c, 0x0002); //FSYNC vs adj. mode falling line count 
	write_cmos_sensor(0x025a, 0x03e8); //FSYNC slave mode int. time threshold 
	write_cmos_sensor(0x0256, 0x0100); //FSYNC slave mode detection margin 
	write_cmos_sensor(0x0254, 0x0001); //Fsync ctrl b12:11:fsync_vs_adj_en, b10:9:fsync_out_sel, b8:fsync_inv_sel, b4:fsync_in_vs_mode, b0:fsync_gpio_mode 
	write_cmos_sensor(0x0440, 0x000c); //adco_pattern_start_pcnt 
	write_cmos_sensor(0x0908, 0x0003); //DPC Memory sckpw=0, sdly=3 
	write_cmos_sensor(0x0708, 0x2f00); //vblank isp clock gating on 
#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_preview_HI1634_QT_p410ae[] = {
0x0B00, 0x0000,
0x0204, 0x0408,
0x0206, 0x02C6,
0x020A, 0x0EA7,
0x020E, 0x0EAB,
0x0224, 0x002C,
0x022A, 0x0016,
0x022C, 0x0E2A,
0x022E, 0x0DE2,
0x0234, 0x2222,
0x0236, 0x2222,
0x0238, 0x2222,
0x023A, 0x2222,
0x0268, 0x0108,
0x0400, 0x0E10,
0x0404, 0x0008,
0x0406, 0x1244,
0x0408, 0x0001,
0x040E, 0x0200,
0x0440, 0x011D,
0x0D00, 0x4000,
0x0D28, 0x0004,
0x0D2A, 0x0923,
0x0602, 0x3112,
0x0608, 0x0248,
0x067A, 0x0303,
0x067C, 0x0303,
0x06DE, 0x0303,
0x06E0, 0x0303,
0x06E4, 0x8A00,
0x06E6, 0x8A00,
0x06E8, 0x8A00,
0x06EA, 0x8A00,
0x0524, 0x5858,
0x0526, 0x5858,
0x0F04, 0x0004,
0x0F06, 0x0002,
0x0F08, 0x0011,
0x0F0A, 0x2233,
0x0B04, 0x009D,
0x0B12, 0x0918,
0x0B14, 0x06D8,
0x0B30, 0x0000,
0x1100, 0x1100,
0x1108, 0x0402,
0x1116, 0x0000,
0x1118, 0x0000,
0x0A0A, 0x8388,
0x0A10, 0xB040,
0x0A1E, 0x0013,
0x0C00, 0x0021,
0x0C14, 0x0004,
0x0C16, 0x0002,
0x0C18, 0x0920,
0x0C1A, 0x0700,
0x0708, 0x6F81,
0x0736, 0x0050,
0x0738, 0x0002,
0x073C, 0x0700,
0x0746, 0x00D4,
0x0748, 0x0002,
0x074A, 0x0900,
0x074C, 0x0100,
0x074E, 0x0100,
0x1200, 0x4946,
0x1206, 0x1800,
0x122E, 0x0490,
0x1230, 0x0248,
0x1000, 0x0300,
0x1002, 0xC311,
0x1004, 0x2BB0,
0x1010, 0x06D1,
0x1012, 0x0092,
0x1020, 0xC107,
0x1022, 0x081F,
0x1024, 0x0509,
0x1026, 0x0B0A,
0x1028, 0x1409,
0x102A, 0x0C0A,
0x102C, 0x1300,
0x1066, 0x06F4,
0x1600, 0x0400,
};
#endif

static void preview_setting(void)
{
#if MULTI_WRITE
LOG_INF("LZH %s",__func__);
	HI1634_QT_p410ae_table_write_cmos_sensor(
		addr_data_pair_preview_HI1634_QT_p410ae,
		sizeof(addr_data_pair_preview_HI1634_QT_p410ae) /
		sizeof(kal_uint16));
#else
	write_cmos_sensor(0x3250, 0xa470); //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	write_cmos_sensor(0x0730, 0x770f); //pll_cfg_mipi_a PLL_CLK=750mhz b7-6:00_isp_div2(1/1) 
	write_cmos_sensor(0x0732, 0xe1b0); //pll_cfg_mipi_b b13-11:100_isp_div1(1/5) b10-8:001_mipi_div1(1/2) b1-0:00_mipi_div2(1/1) 
	write_cmos_sensor(0x1118, 0x0004); //LSC r_win_y B[11]: Bit8 of y offset in start block when cropping. B[10:8] y start index of block when cropping. B[7:0] y offset in start block when cropping.
	write_cmos_sensor(0x1200, 0x011f); //PDPC BYPASS : Dyna-DPC ON, PDEN flag OFF, PD-DPC ON
	write_cmos_sensor(0x1204, 0x1c01); //PDPC DC Counting OFF, PD Around BYPASS OFF
	write_cmos_sensor(0x1240, 0x0100); //pdpc_pd_cnt_max_value
	write_cmos_sensor(0x0b20, 0x8200); //HBIN mode
	write_cmos_sensor(0x0f00, 0x0400); //fmt ctrl
	write_cmos_sensor(0x103e, 0x0100); //mipi_tx_col_read_ctrl
	write_cmos_sensor(0x1020, 0xc106); //mipi_exit_seq, tlpx
	write_cmos_sensor(0x1022, 0x0617); //mipi_tclk_prepare, tclk_zero
	write_cmos_sensor(0x1024, 0x0306); //mipi_tclk_pre, ths_prepare
	write_cmos_sensor(0x1026, 0x0609); //mipi_ths_zero, ths_trail
	write_cmos_sensor(0x1028, 0x1207); //mipi_tclk_post, tclk_trail
	write_cmos_sensor(0x102a, 0x090a); //mipi_texit, tsync
	write_cmos_sensor(0x102c, 0x1400); //mipi_tpd_sync
	write_cmos_sensor(0x1010, 0x07d0); //mipi_vblank_delay
	write_cmos_sensor(0x1012, 0x00ba); //mipi_ch0_hblank_delay
	write_cmos_sensor(0x1014, 0x001b); //mipi_hblank_short_delay1
	write_cmos_sensor(0x1016, 0x001b); //mipi_hblank_short_delay2
	write_cmos_sensor(0x101a, 0x001b); //mipi_pd_hblank_delay
	write_cmos_sensor(0x1038, 0x0000); //mipi_virtual_channel_ctrl
	write_cmos_sensor(0x1042, 0x0008); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
	write_cmos_sensor(0x1048, 0x0080); //mipi_pd_max_col_size
	write_cmos_sensor(0x1044, 0x0100); //mipi_pd_col_size
	write_cmos_sensor(0x1046, 0x0004); //mipi_pd_row_size
	write_cmos_sensor(0x0404, 0x0008); //x addr start active
	write_cmos_sensor(0x0406, 0x1087); //x addr end active
	write_cmos_sensor(0x0220, 0x0008); //y addr start fobp
	write_cmos_sensor(0x022a, 0x0015); //y addr end fobp
	write_cmos_sensor(0x0222, 0x0c80); //y addr start dummy
	write_cmos_sensor(0x022c, 0x0c89); //y addr end dummy
	write_cmos_sensor(0x0224, 0x002c); //y addr start active
	write_cmos_sensor(0x022e, 0x0c61); //y addr end active
	write_cmos_sensor(0x0f04, 0x0004); //fmt x cropping
	write_cmos_sensor(0x0f06, 0x0000); //fmt y cropping
	write_cmos_sensor(0x023a, 0x2222); //y dummy size
	write_cmos_sensor(0x0234, 0x3311); //y even/odd inc tobp
	write_cmos_sensor(0x0238, 0x3311); //y even/odd inc active
	write_cmos_sensor(0x0246, 0x0020); //y read dummy address
	write_cmos_sensor(0x020a, 0x0cfb); //coarse integ time
	write_cmos_sensor(0x021c, 0x0008); //coarse integ time short for iHDR
	write_cmos_sensor(0x0206, 0x05dd); //line length pck
	write_cmos_sensor(0x020e, 0x0d00); //frame length lines
	write_cmos_sensor(0x0b12, 0x0838); //x output size
	write_cmos_sensor(0x0b14, 0x0618); //y output size
	write_cmos_sensor(0x0204, 0x0200); //d2a_row_binning_en
	write_cmos_sensor(0x041c, 0x0048); //pdaf patch start x-address 
	write_cmos_sensor(0x041e, 0x1047); //pdaf patch end x-address 
	write_cmos_sensor(0x0b04, 0x037e); //isp enable
	write_cmos_sensor(0x027e, 0x0100); //tg enable
#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_capture_30fps_HI1634_QT_p410ae[] = {
0x0B00, 0x0000,
0x0204, 0x0008,
0x0206, 0x02C6,
0x020A, 0x0EA7,
0x020E, 0x0EAB,
0x0224, 0x0030,
0x022A, 0x0017,
0x022C, 0x0E1F,
0x022E, 0x0DDF,
0x0234, 0x1111,
0x0236, 0x1111,
0x0238, 0x1111,
0x023A, 0x1111,
0x0268, 0x0108,
0x0400, 0x0A10,
0x0404, 0x0008,
0x0406, 0x1244,
0x0408, 0x0001,
0x040E, 0x0200,
0x0440, 0x011D,
0x0D00, 0x400B,
0x0D28, 0x0008,
0x0D2A, 0x1247,
0x0602, 0x3712,
0x0608, 0x0490,
0x067A, 0x0900,
0x067C, 0x0009,
0x06DE, 0x0900,
0x06E0, 0x0009,
0x06E4, 0x8300,
0x06E6, 0x0100,
0x06E8, 0x0100,
0x06EA, 0x8300,
0x0524, 0x5858,
0x0526, 0x5858,
0x0F04, 0x0008,
0x0F06, 0x0000,
0x0F08, 0x0022,
0x0F0A, 0x1133,
0x0B04, 0x0095,
0x0B12, 0x1230,
0x0B14, 0x0DB0,
0x0B30, 0x0001,
0x1100, 0x1100,
0x1108, 0x0002,
0x1116, 0x0000,
0x1118, 0x0000,
0x0A0A, 0x8388,
0x0A10, 0xB040,
0x0A1E, 0x0013,
0x0C00, 0x0023,
0x0C14, 0x0008,
0x0C16, 0x0000,
0x0C18, 0x1240,
0x0C1A, 0x0E00,
0x0708, 0x6F80,
0x0736, 0x0050,
0x0738, 0x0002,
0x073C, 0x0700,
0x0746, 0x00D4,
0x0748, 0x0002,
0x074A, 0x0900,
0x074C, 0x0000,
0x074E, 0x0100,
0x1200, 0x0946,
0x1206, 0x0800,
0x122E, 0x0490,
0x1230, 0x0248,
0x1000, 0x0300,
0x1002, 0xC311,
0x1004, 0x2BB0,
0x1010, 0x0DC7,
0x1012, 0x013E,
0x1020, 0xC10E,
0x1022, 0x103F,
0x1024, 0x0511,
0x1026, 0x1712,
0x1028, 0x1F10,
0x102A, 0x170A,
0x102C, 0x2500,
0x1066, 0x0420,
0x1600, 0x0000,
};

kal_uint16 addr_data_pair_capture_15fps_HI1634_QT_p410ae[] = {
0x0B00, 0x0000,
0x0204, 0x0008,
0x0206, 0x02C6,
0x020A, 0x0EA7,
0x020E, 0x0EAB,
0x0224, 0x0030,
0x022A, 0x0017,
0x022C, 0x0E1F,
0x022E, 0x0DDF,
0x0234, 0x1111,
0x0236, 0x1111,
0x0238, 0x1111,
0x023A, 0x1111,
0x0268, 0x0108,
0x0400, 0x0A10,
0x0404, 0x0008,
0x0406, 0x1244,
0x0408, 0x0001,
0x040E, 0x0200,
0x0440, 0x011D,
0x0D00, 0x400B,
0x0D28, 0x0008,
0x0D2A, 0x1247,
0x0602, 0x3712,
0x0608, 0x0490,
0x067A, 0x0900,
0x067C, 0x0009,
0x06DE, 0x0900,
0x06E0, 0x0009,
0x06E4, 0x8300,
0x06E6, 0x0100,
0x06E8, 0x0100,
0x06EA, 0x8300,
0x0524, 0x5858,
0x0526, 0x5858,
0x0F04, 0x0008,
0x0F06, 0x0000,
0x0F08, 0x0022,
0x0F0A, 0x1133,
0x0B04, 0x0095,
0x0B12, 0x1230,
0x0B14, 0x0DB0,
0x0B30, 0x0001,
0x1100, 0x1100,
0x1108, 0x0002,
0x1116, 0x0000,
0x1118, 0x0000,
0x0A0A, 0x8388,
0x0A10, 0xB040,
0x0A1E, 0x0013,
0x0C00, 0x0023,
0x0C14, 0x0008,
0x0C16, 0x0000,
0x0C18, 0x1240,
0x0C1A, 0x0E00,
0x0708, 0x6F80,
0x0736, 0x0050,
0x0738, 0x0002,
0x073C, 0x0700,
0x0746, 0x00D4,
0x0748, 0x0002,
0x074A, 0x0900,
0x074C, 0x0000,
0x074E, 0x0100,
0x1200, 0x0946,
0x1206, 0x0800,
0x122E, 0x0490,
0x1230, 0x0248,
0x1000, 0x0300,
0x1002, 0xC311,
0x1004, 0x2BB0,
0x1010, 0x0DC7,
0x1012, 0x013E,
0x1020, 0xC10E,
0x1022, 0x103F,
0x1024, 0x0511,
0x1026, 0x1712,
0x1028, 0x1F10,
0x102A, 0x170A,
0x102C, 0x2500,
0x1066, 0x0420,
0x1600, 0x0000,


	};
#endif


static void capture_setting(kal_uint16 currefps)
{
#if MULTI_WRITE
	if (currefps == 300) {
	HI1634_QT_p410ae_table_write_cmos_sensor(
		addr_data_pair_capture_30fps_HI1634_QT_p410ae,
		sizeof(addr_data_pair_capture_30fps_HI1634_QT_p410ae) /
		sizeof(kal_uint16));

	} else {
	HI1634_QT_p410ae_table_write_cmos_sensor(
		addr_data_pair_capture_15fps_HI1634_QT_p410ae,
		sizeof(addr_data_pair_capture_15fps_HI1634_QT_p410ae) /
		sizeof(kal_uint16));
	}
#else
  if( currefps == 300) {
	  write_cmos_sensor(0x3250, 0xa060); //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	  write_cmos_sensor(0x0730, 0x770f); //pll_cfg_mipi_a PLL_CLK=750mhz b7-6:00_isp_div2(1/1) 
	  write_cmos_sensor(0x0732, 0xe0b0); //pll_cfg_mipi_b b13-11:100_isp_div1(1/5) b10-8:000_mipi_div1(1/1) b1-0:00_mipi_div2(1/1) 
	  write_cmos_sensor(0x1118, 0x0006); //LSC r_win_y B[11]: Bit8 of y offset in start block when cropping. B[10:8] y start index of block when cropping. B[7:0] y offset in start block when cropping.
	  write_cmos_sensor(0x1200, 0x011f); //PDPC BYPASS : Dyna-DPC ON, PDEN flag OFF, PD-DPC ON
	  write_cmos_sensor(0x1204, 0x1c01); //PDPC DC Counting OFF, PD Around BYPASS OFF
	  write_cmos_sensor(0x1240, 0x0100); //pdpc_pd_cnt_max_value
	  write_cmos_sensor(0x0b20, 0x8100); //HBIN mode
	  write_cmos_sensor(0x0f00, 0x0000); //fmt ctrl
	  write_cmos_sensor(0x103e, 0x0000); //mipi_tx_col_read_ctrl
	  write_cmos_sensor(0x1020, 0xc10b); //mipi_exit_seq, tlpx
	  write_cmos_sensor(0x1022, 0x0a31); //mipi_tclk_prepare, tclk_zero
	  write_cmos_sensor(0x1024, 0x030b); //mipi_tclk_pre, ths_prepare
	  write_cmos_sensor(0x1026, 0x0d0f); //mipi_ths_zero, ths_trail
	  write_cmos_sensor(0x1028, 0x1a0e); //mipi_tclk_post, tclk_trail
	  write_cmos_sensor(0x102a, 0x1311); //mipi_texit, tsync
	  write_cmos_sensor(0x102c, 0x2400); //mipi_tpd_sync
	  write_cmos_sensor(0x1010, 0x07d0); //mipi_vblank_delay
	  write_cmos_sensor(0x1012, 0x017d); //mipi_ch0_hblank_delay
	  write_cmos_sensor(0x1014, 0x006a); //mipi_hblank_short_delay1
	  write_cmos_sensor(0x1016, 0x006a); //mipi_hblank_short_delay2
	  write_cmos_sensor(0x101a, 0x006a); //mipi_pd_hblank_delay
	  write_cmos_sensor(0x1038, 0x4100); //mipi_virtual_channel_ctrl
	  write_cmos_sensor(0x1042, 0x0108); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
	  write_cmos_sensor(0x1048, 0x0080); //mipi_pd_max_col_size
	  write_cmos_sensor(0x1044, 0x0100); //mipi_pd_col_size
	  write_cmos_sensor(0x1046, 0x0004); //mipi_pd_row_size
	  write_cmos_sensor(0x0404, 0x0008); //x addr start active
	  write_cmos_sensor(0x0406, 0x1087); //x addr end active
	  write_cmos_sensor(0x0220, 0x0008); //y addr start fobp
	  write_cmos_sensor(0x022a, 0x0017); //y addr end fobp
	  write_cmos_sensor(0x0222, 0x0c80); //y addr start dummy
	  write_cmos_sensor(0x022c, 0x0c89); //y addr end dummy
	  write_cmos_sensor(0x0224, 0x002e); //y addr start active
	  write_cmos_sensor(0x022e, 0x0c61); //y addr end active
	  write_cmos_sensor(0x0f04, 0x0008); //fmt x cropping
	  write_cmos_sensor(0x0f06, 0x0000); //fmt y cropping
	  write_cmos_sensor(0x023a, 0x1111); //y dummy size
	  write_cmos_sensor(0x0234, 0x1111); //y even/odd inc tobp
	  write_cmos_sensor(0x0238, 0x1111); //y even/odd inc active
	  write_cmos_sensor(0x0246, 0x0020); //y read dummy address
	  write_cmos_sensor(0x020a, 0x0cfb); //coarse integ time
	  write_cmos_sensor(0x021c, 0x0008); //coarse integ time short for iHDR
	  write_cmos_sensor(0x0206, 0x05dd); //line length pck
	  write_cmos_sensor(0x020e, 0x0d00); //frame length lines
	  write_cmos_sensor(0x0b12, 0x1070); //x output size
	  write_cmos_sensor(0x0b14, 0x0c30); //y output size
	  write_cmos_sensor(0x0204, 0x0000); //d2a_row_binning_en
	  write_cmos_sensor(0x041c, 0x0048); //pdaf patch start x-address 
	  write_cmos_sensor(0x041e, 0x1047); //pdaf patch end x-address 
	  write_cmos_sensor(0x0b04, 0x037e); //isp enable
	  write_cmos_sensor(0x027e, 0x0100); //tg enable

// PIP
  } else	{
	  write_cmos_sensor(0x3250, 0xa060); //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	  write_cmos_sensor(0x0730, 0x7d4f); //pll_cfg_mipi_a PLL_CLK=750mhz b7-6:01_isp_div2(1/2) 
	  write_cmos_sensor(0x0732, 0xe0b1); //pll_cfg_mipi_b b13-11:100_isp_div1(1/5) b10-8:000_mipi_div1(1/1) b1-0:01_mipi_div2(1/2) 
	  write_cmos_sensor(0x1118, 0x0006); //LSC r_win_y B[11]: Bit8 of y offset in start block when cropping. B[10:8] y start index of block when cropping. B[7:0] y offset in start block when cropping.
	  write_cmos_sensor(0x1200, 0x011f); //PDPC BYPASS : Dyna-DPC ON, PDEN flag OFF, PD-DPC ON
	  write_cmos_sensor(0x1204, 0x1c01); //PDPC DC Counting OFF, PD Around BYPASS OFF
	  write_cmos_sensor(0x1240, 0x0100); //pdpc_pd_cnt_max_value
	  write_cmos_sensor(0x0b20, 0x8100); //HBIN mode
	  write_cmos_sensor(0x0f00, 0x0000); //fmt ctrl
	  write_cmos_sensor(0x103e, 0x0000); //mipi_tx_col_read_ctrl
	  write_cmos_sensor(0x1020, 0xc106); //mipi_exit_seq, tlpx
	  write_cmos_sensor(0x1022, 0x0617); //mipi_tclk_prepare, tclk_zero
	  write_cmos_sensor(0x1024, 0x0306); //mipi_tclk_pre, ths_prepare
	  write_cmos_sensor(0x1026, 0x0609); //mipi_ths_zero, ths_trail
	  write_cmos_sensor(0x1028, 0x1207); //mipi_tclk_post, tclk_trail
	  write_cmos_sensor(0x102a, 0x0a0a); //mipi_texit, tsync
	  write_cmos_sensor(0x102c, 0x1600); //mipi_tpd_sync
	  write_cmos_sensor(0x1010, 0x07d0); //mipi_vblank_delay
	  write_cmos_sensor(0x1012, 0x0201); //mipi_ch0_hblank_delay
	  write_cmos_sensor(0x1014, 0x00b9); //mipi_hblank_short_delay1
	  write_cmos_sensor(0x1016, 0x00b9); //mipi_hblank_short_delay2
	  write_cmos_sensor(0x101a, 0x00b9); //mipi_pd_hblank_delay
	  write_cmos_sensor(0x1038, 0x4100); //mipi_virtual_channel_ctrl
	  write_cmos_sensor(0x1042, 0x0108); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
	  write_cmos_sensor(0x1048, 0x0080); //mipi_pd_max_col_size
	  write_cmos_sensor(0x1044, 0x0100); //mipi_pd_col_size
	  write_cmos_sensor(0x1046, 0x0004); //mipi_pd_row_size
	  write_cmos_sensor(0x0404, 0x0008); //x addr start active
	  write_cmos_sensor(0x0406, 0x1087); //x addr end active
	  write_cmos_sensor(0x0220, 0x0008); //y addr start fobp
	  write_cmos_sensor(0x022a, 0x0017); //y addr end fobp
	  write_cmos_sensor(0x0222, 0x0c80); //y addr start dummy
	  write_cmos_sensor(0x022c, 0x0c89); //y addr end dummy
	  write_cmos_sensor(0x0224, 0x002e); //y addr start active
	  write_cmos_sensor(0x022e, 0x0c61); //y addr end active
	  write_cmos_sensor(0x0f04, 0x0008); //fmt x cropping
	  write_cmos_sensor(0x0f06, 0x0000); //fmt y cropping
	  write_cmos_sensor(0x023a, 0x1111); //y dummy size
	  write_cmos_sensor(0x0234, 0x1111); //y even/odd inc tobp
	  write_cmos_sensor(0x0238, 0x1111); //y even/odd inc active
	  write_cmos_sensor(0x0246, 0x0020); //y read dummy address
	  write_cmos_sensor(0x020a, 0x05dc); //coarse integ time
	  write_cmos_sensor(0x021c, 0x0008); //coarse integ time short for iHDR
	  write_cmos_sensor(0x0206, 0x0bb8); //line length pck
	  write_cmos_sensor(0x020e, 0x0d00); //frame length lines
	  write_cmos_sensor(0x0b12, 0x1070); //x output size
	  write_cmos_sensor(0x0b14, 0x0c30); //y output size
	  write_cmos_sensor(0x0204, 0x0000); //d2a_row_binning_en
	  write_cmos_sensor(0x041c, 0x0048); //pdaf patch start x-address 
	  write_cmos_sensor(0x041e, 0x1047); //pdaf patch end x-address 
	  write_cmos_sensor(0x0b04, 0x037e); //isp enable
	  write_cmos_sensor(0x027e, 0x0100); //tg enable
	}
#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_video_HI1634_QT_p410ae[] = {
0x0B00, 0x0000,
0x0204, 0x0408,
0x0206, 0x02C6,
0x020A, 0x0EA7,
0x020E, 0x0EAB,
0x0224, 0x002C,
0x022A, 0x0016,
0x022C, 0x0E2A,
0x022E, 0x0DE2,
0x0234, 0x2222,
0x0236, 0x2222,
0x0238, 0x2222,
0x023A, 0x2222,
0x0268, 0x0108,
0x0400, 0x0E10,
0x0404, 0x0008,
0x0406, 0x1244,
0x0408, 0x0001,
0x040E, 0x0200,
0x0440, 0x011D,
0x0D00, 0x4000,
0x0D28, 0x0004,
0x0D2A, 0x0923,
0x0602, 0x3112,
0x0608, 0x0248,
0x067A, 0x0303,
0x067C, 0x0303,
0x06DE, 0x0303,
0x06E0, 0x0303,
0x06E4, 0x8A00,
0x06E6, 0x8A00,
0x06E8, 0x8A00,
0x06EA, 0x8A00,
0x0524, 0x5858,
0x0526, 0x5858,
0x0F04, 0x0004,
0x0F06, 0x0002,
0x0F08, 0x0011,
0x0F0A, 0x2233,
0x0B04, 0x009D,
0x0B12, 0x0918,
0x0B14, 0x06D8,
0x0B30, 0x0000,
0x1100, 0x1100,
0x1108, 0x0402,
0x1116, 0x0000,
0x1118, 0x0000,
0x0A0A, 0x8388,
0x0A10, 0xB040,
0x0A1E, 0x0013,
0x0C00, 0x0021,
0x0C14, 0x0004,
0x0C16, 0x0002,
0x0C18, 0x0920,
0x0C1A, 0x0700,
0x0708, 0x6F81,
0x0736, 0x0050,
0x0738, 0x0002,
0x073C, 0x0700,
0x0746, 0x00D4,
0x0748, 0x0002,
0x074A, 0x0900,
0x074C, 0x0100,
0x074E, 0x0100,
0x1200, 0x4946,
0x1206, 0x1800,
0x122E, 0x0490,
0x1230, 0x0248,
0x1000, 0x0300,
0x1002, 0xC311,
0x1004, 0x2BB0,
0x1010, 0x06D1,
0x1012, 0x0092,
0x1020, 0xC107,
0x1022, 0x081F,
0x1024, 0x0509,
0x1026, 0x0B0A,
0x1028, 0x1409,
0x102A, 0x0C0A,
0x102C, 0x1300,
0x1066, 0x06F4,
0x1600, 0x0400,
};
#endif

static void normal_video_setting(void)
{
#if MULTI_WRITE
	HI1634_QT_p410ae_table_write_cmos_sensor(
		addr_data_pair_video_HI1634_QT_p410ae,
		sizeof(addr_data_pair_video_HI1634_QT_p410ae) /
		sizeof(kal_uint16));
#else
	  write_cmos_sensor(0x3250, 0xa060); //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	  write_cmos_sensor(0x0730, 0x770f); //pll_cfg_mipi_a PLL_CLK=750mhz b7-6:00_isp_div2(1/1) 
	  write_cmos_sensor(0x0732, 0xe0b0); //pll_cfg_mipi_b b13-11:100_isp_div1(1/5) b10-8:000_mipi_div1(1/1) b1-0:00_mipi_div2(1/1) 
	  write_cmos_sensor(0x1118, 0x0006); //LSC r_win_y B[11]: Bit8 of y offset in start block when cropping. B[10:8] y start index of block when cropping. B[7:0] y offset in start block when cropping.
	  write_cmos_sensor(0x1200, 0x011f); //PDPC BYPASS : Dyna-DPC ON, PDEN flag OFF, PD-DPC ON
	  write_cmos_sensor(0x1204, 0x1c01); //PDPC DC Counting OFF, PD Around BYPASS OFF
	  write_cmos_sensor(0x1240, 0x0100); //pdpc_pd_cnt_max_value
	  write_cmos_sensor(0x0b20, 0x8100); //HBIN mode
	  write_cmos_sensor(0x0f00, 0x0000); //fmt ctrl
	  write_cmos_sensor(0x103e, 0x0000); //mipi_tx_col_read_ctrl
	  write_cmos_sensor(0x1020, 0xc10b); //mipi_exit_seq, tlpx
	  write_cmos_sensor(0x1022, 0x0a31); //mipi_tclk_prepare, tclk_zero
	  write_cmos_sensor(0x1024, 0x030b); //mipi_tclk_pre, ths_prepare
	  write_cmos_sensor(0x1026, 0x0d0f); //mipi_ths_zero, ths_trail
	  write_cmos_sensor(0x1028, 0x1a0e); //mipi_tclk_post, tclk_trail
	  write_cmos_sensor(0x102a, 0x1311); //mipi_texit, tsync
	  write_cmos_sensor(0x102c, 0x2400); //mipi_tpd_sync
	  write_cmos_sensor(0x1010, 0x07d0); //mipi_vblank_delay
	  write_cmos_sensor(0x1012, 0x017d); //mipi_ch0_hblank_delay
	  write_cmos_sensor(0x1014, 0x006a); //mipi_hblank_short_delay1
	  write_cmos_sensor(0x1016, 0x006a); //mipi_hblank_short_delay2
	  write_cmos_sensor(0x101a, 0x006a); //mipi_pd_hblank_delay
	  write_cmos_sensor(0x1038, 0x4100); //mipi_virtual_channel_ctrl
	  write_cmos_sensor(0x1042, 0x0108); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
	  write_cmos_sensor(0x1048, 0x0080); //mipi_pd_max_col_size
	  write_cmos_sensor(0x1044, 0x0100); //mipi_pd_col_size
	  write_cmos_sensor(0x1046, 0x0004); //mipi_pd_row_size
	  write_cmos_sensor(0x0404, 0x0008); //x addr start active
	  write_cmos_sensor(0x0406, 0x1087); //x addr end active
	  write_cmos_sensor(0x0220, 0x0008); //y addr start fobp
	  write_cmos_sensor(0x022a, 0x0017); //y addr end fobp
	  write_cmos_sensor(0x0222, 0x0c80); //y addr start dummy
	  write_cmos_sensor(0x022c, 0x0c89); //y addr end dummy
	  write_cmos_sensor(0x0224, 0x002e); //y addr start active
	  write_cmos_sensor(0x022e, 0x0c61); //y addr end active
	  write_cmos_sensor(0x0f04, 0x0008); //fmt x cropping
	  write_cmos_sensor(0x0f06, 0x0000); //fmt y cropping
	  write_cmos_sensor(0x023a, 0x1111); //y dummy size
	  write_cmos_sensor(0x0234, 0x1111); //y even/odd inc tobp
	  write_cmos_sensor(0x0238, 0x1111); //y even/odd inc active
	  write_cmos_sensor(0x0246, 0x0020); //y read dummy address
	  write_cmos_sensor(0x020a, 0x0cfb); //coarse integ time
	  write_cmos_sensor(0x021c, 0x0008); //coarse integ time short for iHDR
	  write_cmos_sensor(0x0206, 0x05dd); //line length pck
	  write_cmos_sensor(0x020e, 0x0d00); //frame length lines
	  write_cmos_sensor(0x0b12, 0x1070); //x output size
	  write_cmos_sensor(0x0b14, 0x0c30); //y output size
	  write_cmos_sensor(0x0204, 0x0000); //d2a_row_binning_en
	  write_cmos_sensor(0x041c, 0x0048); //pdaf patch start x-address 
	  write_cmos_sensor(0x041e, 0x1047); //pdaf patch end x-address 
	  write_cmos_sensor(0x0b04, 0x037e); //isp enable
	  write_cmos_sensor(0x027e, 0x0100); //tg enable
#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_hs_video_HI1634_QT_p410ae[] = {
	0x3250, 0xa470, //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	0x0730, 0x770f, //pll_cfg_mipi_a PLL_CLK=750mhz b7-6:00_isp_div2(1/1) 
	0x0732, 0xe4b0, //pll_cfg_mipi_b b13-11:100_isp_div1(1/5) b10-8:100_mipi_div1(1/6) b1-0:00_mipi_div2(1/1) 
	0x1118, 0x0072, //LSC r_win_y B[11]: Bit8 of y offset in start block when cropping. B[10:8] y start index of block when cropping. B[7:0] y offset in start block when cropping.
	0x1200, 0x011f, //PDPC BYPASS : Dyna-DPC ON, PDEN flag OFF, PD-DPC ON
	0x1204, 0x1c01, //PDPC DC Counting OFF, PD Around BYPASS OFF
	0x1240, 0x0100, //pdpc_pd_cnt_max_value
	0x0b20, 0x8600, //HBIN mode
	0x0f00, 0x1400, //fmt ctrl
	0x103e, 0x0500, //mipi_tx_col_read_ctrl
	0x1020, 0xc102, //mipi_exit_seq, tlpx
	0x1022, 0x0209, //mipi_tclk_prepare, tclk_zero
	0x1024, 0x0303, //mipi_tclk_pre, ths_prepare
	0x1026, 0x0305, //mipi_ths_zero, ths_trail
	0x1028, 0x0903, //mipi_tclk_post, tclk_trail
	0x102a, 0x0404, //mipi_texit, tsync
	0x102c, 0x0400, //mipi_tpd_sync
	0x1010, 0x07d0, //mipi_vblank_delay
	0x1012, 0x0040, //mipi_ch0_hblank_delay
	0x1014, 0xffea, //mipi_hblank_short_delay1
	0x1016, 0xffea, //mipi_hblank_short_delay2
	0x101a, 0xffea, //mipi_pd_hblank_delay
	0x1038, 0x0000, //mipi_virtual_channel_ctrl
	0x1042, 0x0008, //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
	0x1048, 0x0080, //mipi_pd_max_col_size
	0x1044, 0x0100, //mipi_pd_col_size
	0x1046, 0x0004, //mipi_pd_row_size
	0x0404, 0x0008, //x addr start active
	0x0406, 0x1087, //x addr end active
	0x0220, 0x000c, //y addr start fobp
	0x022a, 0x0013, //y addr end fobp
	0x0222, 0x0c80, //y addr start dummy
	0x022c, 0x0c89, //y addr end dummy
	0x0224, 0x009c, //y addr start active
	0x022e, 0x0bed, //y addr end active
	0x0f04, 0x0020, //fmt x cropping
	0x0f06, 0x0000, //fmt y cropping
	0x023a, 0x6666, //y dummy size
	0x0234, 0x7755, //y even/odd inc tobp
	0x0238, 0x7755, //y even/odd inc active
	0x0246, 0x0020, //y read dummy address
	0x020a, 0x0339, //coarse integ time
	0x021c, 0x0008, //coarse integ time short for iHDR
	0x0206, 0x05dd, //line length pck
	0x020e, 0x0340, //frame length lines
	0x0b12, 0x0280, //x output size
	0x0b14, 0x01e0, //y output size
	0x0204, 0x0200, //d2a_row_binning_en
	0x041c, 0x0048, //pdaf patch start x-address 
	0x041e, 0x1047, //pdaf patch end x-address 
	0x0b04, 0x037e, //isp enable
	0x027e, 0x0100, //tg enable
};
#endif

static void hs_video_setting(void)
{

#if MULTI_WRITE
	HI1634_QT_p410ae_table_write_cmos_sensor(
		addr_data_pair_hs_video_HI1634_QT_p410ae,
		sizeof(addr_data_pair_hs_video_HI1634_QT_p410ae) /
		sizeof(kal_uint16));
#else
// setting ver5.0 None PD 640x480 120fps
	write_cmos_sensor(0x3250, 0xa470); //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	write_cmos_sensor(0x0730, 0x770f); //pll_cfg_mipi_a PLL_CLK=750mhz b7-6:00_isp_div2(1/1) 
	write_cmos_sensor(0x0732, 0xe4b0); //pll_cfg_mipi_b b13-11:100_isp_div1(1/5) b10-8:100_mipi_div1(1/6) b1-0:00_mipi_div2(1/1) 
	write_cmos_sensor(0x1118, 0x0072); //LSC r_win_y B[11]: Bit8 of y offset in start block when cropping. B[10:8] y start index of block when cropping. B[7:0] y offset in start block when cropping.
	write_cmos_sensor(0x1200, 0x011f); //PDPC BYPASS : Dyna-DPC ON, PDEN flag OFF, PD-DPC ON
	write_cmos_sensor(0x1204, 0x1c01); //PDPC DC Counting OFF, PD Around BYPASS OFF
	write_cmos_sensor(0x1240, 0x0100); //pdpc_pd_cnt_max_value
	write_cmos_sensor(0x0b20, 0x8600); //HBIN mode
	write_cmos_sensor(0x0f00, 0x1400); //fmt ctrl
	write_cmos_sensor(0x103e, 0x0500); //mipi_tx_col_read_ctrl
	write_cmos_sensor(0x1020, 0xc102); //mipi_exit_seq, tlpx
	write_cmos_sensor(0x1022, 0x0209); //mipi_tclk_prepare, tclk_zero
	write_cmos_sensor(0x1024, 0x0303); //mipi_tclk_pre, ths_prepare
	write_cmos_sensor(0x1026, 0x0305); //mipi_ths_zero, ths_trail
	write_cmos_sensor(0x1028, 0x0903); //mipi_tclk_post, tclk_trail
	write_cmos_sensor(0x102a, 0x0404); //mipi_texit, tsync
	write_cmos_sensor(0x102c, 0x0400); //mipi_tpd_sync
	write_cmos_sensor(0x1010, 0x07d0); //mipi_vblank_delay
	write_cmos_sensor(0x1012, 0x0040); //mipi_ch0_hblank_delay
	write_cmos_sensor(0x1014, 0xffea); //mipi_hblank_short_delay1
	write_cmos_sensor(0x1016, 0xffea); //mipi_hblank_short_delay2
	write_cmos_sensor(0x101a, 0xffea); //mipi_pd_hblank_delay
	write_cmos_sensor(0x1038, 0x0000); //mipi_virtual_channel_ctrl
	write_cmos_sensor(0x1042, 0x0008); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
	write_cmos_sensor(0x1048, 0x0080); //mipi_pd_max_col_size
	write_cmos_sensor(0x1044, 0x0100); //mipi_pd_col_size
	write_cmos_sensor(0x1046, 0x0004); //mipi_pd_row_size
	write_cmos_sensor(0x0404, 0x0008); //x addr start active
	write_cmos_sensor(0x0406, 0x1087); //x addr end active
	write_cmos_sensor(0x0220, 0x000c); //y addr start fobp
	write_cmos_sensor(0x022a, 0x0013); //y addr end fobp
	write_cmos_sensor(0x0222, 0x0c80); //y addr start dummy
	write_cmos_sensor(0x022c, 0x0c89); //y addr end dummy
	write_cmos_sensor(0x0224, 0x009c); //y addr start active
	write_cmos_sensor(0x022e, 0x0bed); //y addr end active
	write_cmos_sensor(0x0f04, 0x0020); //fmt x cropping
	write_cmos_sensor(0x0f06, 0x0000); //fmt y cropping
	write_cmos_sensor(0x023a, 0x6666); //y dummy size
	write_cmos_sensor(0x0234, 0x7755); //y even/odd inc tobp
	write_cmos_sensor(0x0238, 0x7755); //y even/odd inc active
	write_cmos_sensor(0x0246, 0x0020); //y read dummy address
	write_cmos_sensor(0x020a, 0x0339); //coarse integ time
	write_cmos_sensor(0x021c, 0x0008); //coarse integ time short for iHDR
	write_cmos_sensor(0x0206, 0x05dd); //line length pck
	write_cmos_sensor(0x020e, 0x0340); //frame length lines
	write_cmos_sensor(0x0b12, 0x0280); //x output size
	write_cmos_sensor(0x0b14, 0x01e0); //y output size
	write_cmos_sensor(0x0204, 0x0200); //d2a_row_binning_en
	write_cmos_sensor(0x041c, 0x0048); //pdaf patch start x-address 
	write_cmos_sensor(0x041e, 0x1047); //pdaf patch end x-address 
	write_cmos_sensor(0x0b04, 0x037e); //isp enable
	write_cmos_sensor(0x027e, 0x0100); //tg enable
#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_slim_video_HI1634_QT_p410ae[] = {
	0x3250, 0xa060, //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	0x0730, 0x770f, //pll_cfg_mipi_a PLL_CLK=750mhz b7-6:00_isp_div2(1/1) 
	0x0732, 0xe2b0, //pll_cfg_mipi_b b13-11:100_isp_div1(1/5) b10-8:010_mipi_div1(1/3) b1-0:00_mipi_div2(1/1) 
	0x1118, 0x01a8, //LSC r_win_y B[11]: Bit8 of y offset in start block when cropping. B[10:8] y start index of block when cropping. B[7:0] y offset in start block when cropping.
	0x1200, 0x011f, //PDPC BYPASS : Dyna-DPC ON, PDEN flag OFF, PD-DPC ON
	0x1204, 0x1c01, //PDPC DC Counting OFF, PD Around BYPASS OFF
	0x1240, 0x0100, //pdpc_pd_cnt_max_value
	0x0b20, 0x8300, //HBIN mode
	0x0f00, 0x0800, //fmt ctrl
	0x103e, 0x0200, //mipi_tx_col_read_ctrl
	0x1020, 0xc104, //mipi_exit_seq, tlpx
	0x1022, 0x0410, //mipi_tclk_prepare, tclk_zero
	0x1024, 0x0304, //mipi_tclk_pre, ths_prepare
	0x1026, 0x0507, //mipi_ths_zero, ths_trail
	0x1028, 0x0d05, //mipi_tclk_post, tclk_trail
	0x102a, 0x0704, //mipi_texit, tsync
	0x102c, 0x1400, //mipi_tpd_sync
	0x1010, 0x07d0, //mipi_vblank_delay
	0x1012, 0x009c, //mipi_ch0_hblank_delay
	0x1014, 0x0013, //mipi_hblank_short_delay1
	0x1016, 0x0013, //mipi_hblank_short_delay2
	0x101a, 0x0013, //mipi_pd_hblank_delay
	0x1038, 0x0000, //mipi_virtual_channel_ctrl
	0x1042, 0x0008, //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
	0x1048, 0x0080, //mipi_pd_max_col_size
	0x1044, 0x0100, //mipi_pd_col_size
	0x1046, 0x0004, //mipi_pd_row_size
	0x0404, 0x0008, //x addr start active
	0x0406, 0x1087, //x addr end active
	0x0220, 0x0008, //y addr start fobp
	0x022a, 0x0017, //y addr end fobp
	0x0222, 0x0c80, //y addr start dummy
	0x022c, 0x0c89, //y addr end dummy
	0x0224, 0x020a, //y addr start active
	0x022e, 0x0a83, //y addr end active
	0x0f04, 0x0040, //fmt x cropping
	0x0f06, 0x0000, //fmt y cropping
	0x023a, 0x3333, //y dummy size
	0x0234, 0x3333, //y even/odd inc tobp
	0x0238, 0x3333, //y even/odd inc active
	0x0246, 0x0020, //y read dummy address
	0x020a, 0x0339, //coarse integ time
	0x021c, 0x0008, //coarse integ time short for iHDR
	0x0206, 0x05dd, //line length pck
	0x020e, 0x0340, //frame length lines
	0x0b12, 0x0500, //x output size
	0x0b14, 0x02d0, //y output size
	0x0204, 0x0000, //d2a_row_binning_en
	0x041c, 0x0048, //pdaf patch start x-address 
	0x041e, 0x1047, //pdaf patch end x-address 
	0x0b04, 0x037e, //isp enable
	0x027e, 0x0100, //tg enable

};
#endif


static void slim_video_setting(void)
{

#if MULTI_WRITE
	HI1634_QT_p410ae_table_write_cmos_sensor(
		addr_data_pair_slim_video_HI1634_QT_p410ae,
		sizeof(addr_data_pair_slim_video_HI1634_QT_p410ae) /
		sizeof(kal_uint16));
#else
	write_cmos_sensor(0x3250, 0xa060); //sreg8 - RAMP,B[15:10]:d2a_ramp_rng_ctrl
	write_cmos_sensor(0x0730, 0x770f); //pll_cfg_mipi_a PLL_CLK=750mhz b7-6:00_isp_div2(1/1) 
	write_cmos_sensor(0x0732, 0xe2b0); //pll_cfg_mipi_b b13-11:100_isp_div1(1/5) b10-8:010_mipi_div1(1/3) b1-0:00_mipi_div2(1/1) 
	write_cmos_sensor(0x1118, 0x01a8); //LSC r_win_y B[11]: Bit8 of y offset in start block when cropping. B[10:8] y start index of block when cropping. B[7:0] y offset in start block when cropping.
	write_cmos_sensor(0x1200, 0x011f); //PDPC BYPASS : Dyna-DPC ON, PDEN flag OFF, PD-DPC ON
	write_cmos_sensor(0x1204, 0x1c01); //PDPC DC Counting OFF, PD Around BYPASS OFF
	write_cmos_sensor(0x1240, 0x0100); //pdpc_pd_cnt_max_value
	write_cmos_sensor(0x0b20, 0x8300); //HBIN mode
	write_cmos_sensor(0x0f00, 0x0800); //fmt ctrl
	write_cmos_sensor(0x103e, 0x0200); //mipi_tx_col_read_ctrl
	write_cmos_sensor(0x1020, 0xc104); //mipi_exit_seq, tlpx
	write_cmos_sensor(0x1022, 0x0410); //mipi_tclk_prepare, tclk_zero
	write_cmos_sensor(0x1024, 0x0304); //mipi_tclk_pre, ths_prepare
	write_cmos_sensor(0x1026, 0x0507); //mipi_ths_zero, ths_trail
	write_cmos_sensor(0x1028, 0x0d05); //mipi_tclk_post, tclk_trail
	write_cmos_sensor(0x102a, 0x0704); //mipi_texit, tsync
	write_cmos_sensor(0x102c, 0x1400); //mipi_tpd_sync
	write_cmos_sensor(0x1010, 0x07d0); //mipi_vblank_delay
	write_cmos_sensor(0x1012, 0x009c); //mipi_ch0_hblank_delay
	write_cmos_sensor(0x1014, 0x0013); //mipi_hblank_short_delay1
	write_cmos_sensor(0x1016, 0x0013); //mipi_hblank_short_delay2
	write_cmos_sensor(0x101a, 0x0013); //mipi_pd_hblank_delay
	write_cmos_sensor(0x1038, 0x0000); //mipi_virtual_channel_ctrl
	write_cmos_sensor(0x1042, 0x0008); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
	write_cmos_sensor(0x1048, 0x0080); //mipi_pd_max_col_size
	write_cmos_sensor(0x1044, 0x0100); //mipi_pd_col_size
	write_cmos_sensor(0x1046, 0x0004); //mipi_pd_row_size
	write_cmos_sensor(0x0404, 0x0008); //x addr start active
	write_cmos_sensor(0x0406, 0x1087); //x addr end active
	write_cmos_sensor(0x0220, 0x0008); //y addr start fobp
	write_cmos_sensor(0x022a, 0x0017); //y addr end fobp
	write_cmos_sensor(0x0222, 0x0c80); //y addr start dummy
	write_cmos_sensor(0x022c, 0x0c89); //y addr end dummy
	write_cmos_sensor(0x0224, 0x020a); //y addr start active
	write_cmos_sensor(0x022e, 0x0a83); //y addr end active
	write_cmos_sensor(0x0f04, 0x0040); //fmt x cropping
	write_cmos_sensor(0x0f06, 0x0000); //fmt y cropping
	write_cmos_sensor(0x023a, 0x3333); //y dummy size
	write_cmos_sensor(0x0234, 0x3333); //y even/odd inc tobp
	write_cmos_sensor(0x0238, 0x3333); //y even/odd inc active
	write_cmos_sensor(0x0246, 0x0020); //y read dummy address
	write_cmos_sensor(0x020a, 0x0339); //coarse integ time
	write_cmos_sensor(0x021c, 0x0008); //coarse integ time short for iHDR
	write_cmos_sensor(0x0206, 0x05dd); //line length pck
	write_cmos_sensor(0x020e, 0x0340); //frame length lines
	write_cmos_sensor(0x0b12, 0x0500); //x output size
	write_cmos_sensor(0x0b14, 0x02d0); //y output size
	write_cmos_sensor(0x0204, 0x0000); //d2a_row_binning_en
	write_cmos_sensor(0x041c, 0x0048); //pdaf patch start x-address 
	write_cmos_sensor(0x041e, 0x1047); //pdaf patch end x-address 
	write_cmos_sensor(0x0b04, 0x037e); //isp enable
	write_cmos_sensor(0x027e, 0x0100); //tg enable


#endif
}

static kal_uint16 read_cmos_sensor_eeprom(kal_uint16 addr)
{
    kal_uint16 get_byte = 0;
    char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

    iReadRegI2C(pu_send_cmd, 2, (u8 *) &get_byte, 1, EEPROM_WRITE_ID);
	return get_byte;
}


static int get_eeprom_data(MUINT8 *data)
{
    MUINT8 i =0x0;
    u8 *otp_data = (u8 *)data;

    for (i = 0x0015;i <= 0x0024; i++, otp_data++){
        *otp_data = read_cmos_sensor_eeprom(i);
        pr_err("wrt pOtp_data s5kjn1 otpdata[0x%x]=0x%x \n", i, *otp_data);
    }
    return 0;
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
			get_eeprom_data(pOtp_data);
			LOG_INF("i2c write id : 0x%x, sensor id: 0x%x\n",
			imgsensor.i2c_write_id, *sensor_id);
#ifdef CONFIG_TINNO_PRODUCT_INFO
                       if('O' ==read_cmos_sensor_eeprom(0x000D) && 'F' == read_cmos_sensor_eeprom(0x000E))
			FULL_PRODUCT_DEVICE_INFO_CAMERA(HI1634_QT_P410AE_SENSOR_ID,1,"hi1634_of_p410ae_mipi_raw",imgsensor_info.cap.grabwindow_width, imgsensor_info.cap.grabwindow_height);
                         else if ('T' ==read_cmos_sensor_eeprom(0x000D) && 'S' == read_cmos_sensor_eeprom(0x000E))
			FULL_PRODUCT_DEVICE_INFO_CAMERA(HI1634_QT_P410AE_SENSOR_ID,1,"hi1634_tsp_p410ae_mipi_raw",imgsensor_info.cap.grabwindow_width, imgsensor_info.cap.grabwindow_height);
#endif
			return ERROR_NONE;
			}

			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		LOG_INF("Read id fail,sensor id: 0x%x\n", *sensor_id);
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
	kal_uint16 sensor_id = 0;

	LOG_INF("lzh [open]: HI1634_QT_p410ae@MT6765,MIPI 4LANE\n");

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("lzh i2c write id: 0x%x, sensor id: 0x%x\n",
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
		LOG_INF("open sensor id fail: 0x%x\n", sensor_id);
		return ERROR_SENSOR_CONNECT_FAIL;
	}
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
	imgsensor.ihdr_en = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	//imgsensor.pdaf_mode = 1;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}	/*	open  */
static kal_uint32 close(void)
{
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
	LOG_INF("lzh E");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
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
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;

	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate)	{
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
	 //PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}

	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("Caputre fps:%d\n", imgsensor.current_fps);
	capture_setting(imgsensor.current_fps);

	return ERROR_NONE;

}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting();
	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	return ERROR_NONE;
}    /*    hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
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
}    /*    slim_video     */

static kal_uint32 get_resolution(
		MSDK_SENSOR_RESOLUTION_INFO_STRUCT * sensor_resolution)
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
	return ERROR_NONE;
}    /*    get_resolution    */


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

	sensor_info->SensroInterfaceType =
	imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame =
		imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame =
		imgsensor_info.slim_video_delay_frame;
	memcpy((void *)sensor_info->TinnoModuleSn,(void *)pOtp_data,sizeof(pOtp_data));

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent =
		imgsensor_info.isp_driving_current;
/* The frame of setting shutter default 0 for TG int */
	sensor_info->AEShutDelayFrame =
		imgsensor_info.ae_shut_delay_frame;
/* The frame of setting sensor gain */
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine =
		imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum =
		imgsensor_info.sensor_mode_num;

	sensor_info->SensorMIPILaneNumber =
		imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;    // 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

#if ENABLE_PDAF
	sensor_info->PDAF_Support = PDAF_SUPPORT_CAMSV;
#endif

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
	default:
	    sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
	    sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

	    sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
	break;
	}

	return ERROR_NONE;
}    /*    get_info  */


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
		LOG_INF("preview\n");
		preview(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		LOG_INF("capture\n");
	//case MSDK_SCENARIO_ID_CAMERA_ZSD:
		capture(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		LOG_INF("video preview\n");
		normal_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		hs_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
	    slim_video(image_window, sensor_config_data);
		break;
	default:
		LOG_INF("default mode\n");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d ", framerate);
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
		// Dynamic frame rate
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);

	if ((framerate == 30) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 15) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = 10 * framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);
	set_dummy();
	return ERROR_NONE;
}


static kal_uint32 set_auto_flicker_mode(kal_bool enable,
			UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d ", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)
		imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n",
				scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	    frame_length = imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
	    spin_lock(&imgsensor_drv_lock);
	    imgsensor.dummy_line = (frame_length >
			imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
	    imgsensor.frame_length = imgsensor_info.pre.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
	    frame_length = imgsensor_info.normal_video.pclk /
			framerate * 10 / imgsensor_info.normal_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
	    imgsensor.dummy_line = (frame_length >
			imgsensor_info.normal_video.framelength) ?
		(frame_length - imgsensor_info.normal_video.framelength) : 0;
	    imgsensor.frame_length = imgsensor_info.normal_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps ==
				imgsensor_info.cap1.max_framerate) {
		frame_length = imgsensor_info.cap1.pclk / framerate * 10 /
				imgsensor_info.cap1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length >
			imgsensor_info.cap1.framelength) ?
			(frame_length - imgsensor_info.cap1.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.cap1.framelength +
				imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		} else {
			if (imgsensor.current_fps !=
				imgsensor_info.cap.max_framerate)
			LOG_INF("fps %d fps not support,use cap: %d fps!\n",
			framerate, imgsensor_info.cap.max_framerate/10);
			frame_length = imgsensor_info.cap.pclk /
				framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length >
				imgsensor_info.cap.framelength) ?
			(frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length =
				imgsensor_info.cap.framelength +
				imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
		}
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
	    frame_length = imgsensor_info.hs_video.pclk /
			framerate * 10 / imgsensor_info.hs_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
	    imgsensor.dummy_line = (frame_length >
			imgsensor_info.hs_video.framelength) ? (frame_length -
			imgsensor_info.hs_video.framelength) : 0;
	    imgsensor.frame_length = imgsensor_info.hs_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
	    frame_length = imgsensor_info.slim_video.pclk /
			framerate * 10 / imgsensor_info.slim_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
	    imgsensor.dummy_line = (frame_length >
			imgsensor_info.slim_video.framelength) ? (frame_length -
			imgsensor_info.slim_video.framelength) : 0;
	    imgsensor.frame_length =
			imgsensor_info.slim_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	default:  //coding with  preview scenario by default
	    frame_length = imgsensor_info.pre.pclk / framerate * 10 /
						imgsensor_info.pre.linelength;
	    spin_lock(&imgsensor_drv_lock);
	    imgsensor.dummy_line = (frame_length >
			imgsensor_info.pre.framelength) ?
			(frame_length - imgsensor_info.pre.framelength) : 0;
	    imgsensor.frame_length = imgsensor_info.pre.framelength +
				imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	    LOG_INF("error scenario_id = %d, we use preview scenario\n",
				scenario_id);
	break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(
				enum MSDK_SCENARIO_ID_ENUM scenario_id,
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
	default:
	break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("set_test_pattern_mode enable: %d", enable);
	if (enable) {
		write_cmos_sensor(0x0b04, 0x0095);
		write_cmos_sensor(0x0C0A, 0x0100);

	} else {
		write_cmos_sensor(0x0b04, 0x0094);
		write_cmos_sensor(0x0C0A, 0x0000);

	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("LZH streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable)
		write_cmos_sensor(0x0b00, 0x0100); // stream on
	else
		write_cmos_sensor(0x0b00, 0x0000); // stream off

	mdelay(10);
	return ERROR_NONE;
}

static kal_uint32 feature_control(
			MSDK_SENSOR_FEATURE_ENUM feature_id,
			UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;

#if ENABLE_PDAF
    struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
#endif

	unsigned long long *feature_data =
		(unsigned long long *) feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

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
	    write_cmos_sensor(sensor_reg_data->RegAddr,
						sensor_reg_data->RegData);
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
	    set_test_pattern_mode((BOOL)*feature_data);
	break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
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
	    LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
				(UINT32)*feature_data);

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
	#if 0
	    ihdr_write_shutter_gain((UINT16)*feature_data,
			(UINT16)*(feature_data+1), (UINT16)*(feature_data+2));
	#endif
	break;
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = 0;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
#if ENABLE_PDAF

		case SENSOR_FEATURE_GET_VC_INFO:
				LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
				pvcinfo = (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
				switch (*feature_data_32) 
				{
					case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
						LOG_INF("SENSOR_FEATURE_GET_VC_INFO CAPTURE_JPEG\n");
						memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[1],sizeof(struct SENSOR_VC_INFO_STRUCT));
						break;
					case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
						LOG_INF("SENSOR_FEATURE_GET_VC_INFO VIDEO PREVIEW\n");
						memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[2],sizeof(struct SENSOR_VC_INFO_STRUCT));
						break;
					case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					default:
						LOG_INF("SENSOR_FEATURE_GET_VC_INFO DEFAULT_PREVIEW\n");
						memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[0],sizeof(struct SENSOR_VC_INFO_STRUCT));
						break;
				}
				break;

		case SENSOR_FEATURE_GET_PDAF_DATA:
		LOG_INF(" GET_PDAF_DATA EEPROM\n");
		// read from e2prom
/*#if e2prom
		read_eeprom((kal_uint16)(*feature_data), 
				(char *)(uintptr_t)(*(feature_data+1)), 
				(kal_uint32)(*(feature_data+2)) );
#else
		// read from file

	        LOG_INF("READ PDCAL DATA\n");
		read_HI1634_QT_p410ae_eeprom((kal_uint16)(*feature_data), 
				(char *)(uintptr_t)(*(feature_data+1)), 
				(kal_uint32)(*(feature_data+2)) );

#endif*/
		break;
		case SENSOR_FEATURE_GET_PDAF_INFO:
			PDAFinfo= (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));  
			switch( *feature_data) 
			{
		 		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		 		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info, sizeof(struct SET_PD_BLOCK_INFO_T));
					break;
		 		//case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		 		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		 		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		 		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	 	 		default:
					break;
			}
		break;

	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n", *feature_data);
		//PDAF capacity enable or not, 2p8 only full size support PDAF
		switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1; // type2 - VC enable
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
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n", *feature_data);
		break;

	case SENSOR_FEATURE_SET_PDAF:
			 	imgsensor.pdaf_mode = *feature_data_16;
	        	LOG_INF(" pdaf mode : %d \n", imgsensor.pdaf_mode);
				break;
	
#endif

	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
		kal_uint32 rate;
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
			default:
				rate = imgsensor_info.pre.mipi_pixel_rate;
				break;
			}
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;	
	}
	break;



	default:
	break;
	}

	return ERROR_NONE;
}    /*    feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 HI1634_QT_P410AE_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc =  &sensor_func;
	return ERROR_NONE;
}	/*	HI1634_QT_P410AE_MIPI_RAW_SensorInit	*/
