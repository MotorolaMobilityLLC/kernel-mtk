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

#include "hi1333mipiraw_Sensor.h"

#define PFX "hi1333_camera_sensor"
#define LOG_INF(format, args...)    \
	pr_err(PFX "[%s] " format, __func__, ##args)


#define SET_STREAMING_TEST 1
//PDAF
#define ENABLE_PDAF 1
#define e2prom 1

#define per_frame 1

extern bool read_hi1333_eeprom( kal_uint16 addr, BYTE *data, kal_uint32 size); 
extern bool read_eeprom( kal_uint16 addr, BYTE * data, kal_uint32 size);


#define MULTI_WRITE 0
static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = { 
	.sensor_id = HI1333_SENSOR_ID,
	
	.checksum_value = 0xf908de1f,       //0x6d01485c // Auto Test Mode ÃßÈÄ..
	.pre = {
		.pclk = 600000000,				//record different mode's pclk
		.linelength =  5952, 			//record different mode's linelength
		.framelength = 3360,//3300, 			//record different mode's framelength
		.startx = 0,				    //record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2104, //2104,		//record different mode's width of grabwindow
		.grabwindow_height = 1560,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.mipi_pixel_rate = 218000000,
		.max_framerate = 300,	
	},
	.cap = {
		.pclk = 600000000,
		.linelength = 5952, 	
		.framelength = 3360, 
		.startx = 0,	
		.starty = 0,
		.grabwindow_width = 4208,
		.grabwindow_height = 3120,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 435000000,
		.max_framerate = 300,

//rate = (600000000/(5952 - 80))  *4208=430000000
	},
	// need to setting
    .cap1 = {                            
		.pclk = 300000000,
		.linelength = 5952,	 
		.framelength = 3360,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4208,
		.grabwindow_height = 3120,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 435000000,
		.max_framerate = 150,
    },
	.normal_video = {
		.pclk = 600000000,
		.linelength = 5952, 	
		.framelength = 3360,
		.startx = 0,	
		.starty = 0,
		.grabwindow_width = 4208,
		.grabwindow_height = 3120,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 435000000,
		.max_framerate = 300,
	},
	.hs_video = {
        .pclk = 600000000,
        .linelength = 5952,				
		.framelength = 840, //832,			
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 640 ,		
		.grabwindow_height = 480 ,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 1200,
	},
    .slim_video = {
		.pclk = 600000000,
		.linelength = 5952,
		.framelength = 840,//832, 
		.startx = 0,
		.starty = 0,
    	.grabwindow_width = 1280,
    	.grabwindow_height = 720,
    	.mipi_data_lp2hs_settle_dc = 85,//unit , ns
    	.max_framerate = 1200, //1200,
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
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x42, 0xff},
	.i2c_speed = 400,
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

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
	{ 4240, 3124,   0,   0, 4240, 3124,	 2120, 1562,  16, 2, 2104, 1560, 0, 0, 2104, 1560},		// preview (2104 x 1560)
	{ 4240, 3124,   0,   0, 4240, 3124,	 4240, 3124,  16, 2, 4208, 3120, 0, 0, 4208, 3120},		// capture (4208 x 3120)
	{ 4240, 3124,   0,   0, 4240, 3124,	 4240, 3124,  16, 2, 4208, 3120, 0, 0, 4208, 3120},		// VIDEO (4208 x 3120)
	{ 4240, 3124,   0, 110, 4240, 2904,	  704,  484,  34, 2,  640,  480, 0, 0,  640,  480},		// hight speed video (640 x 480)
 	{ 4240, 3124,   0, 476, 4240, 2172,      1408,  724,  66, 2, 1280,  720, 0, 0, 1280,  720},       // slim video (1280 x 720)
};


#if ENABLE_PDAF

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
{
	/* Preview mode setting */
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum	
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8  2:4x4  3:1x1 */
	  0x00, 0x2b, 0x0838, 0x0618, 	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
	  0x00, 0x00, 0x0000, 0x0000, 	// VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??
	/* Capture mode setting */ 
	 {0x02, //VC_Num
	  0x0A, //VC_PixelNum	
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8  2:4x4  3:1x1 */
	  0x00, 0x2b, 0x1070, 0x0C30, 	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
      //0x01, 0x2b, 0x0180, 0x0100,   // VC2 PDAF
      0x01, 0x30, 0x0140, 0x0180,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??
	/* Video mode setting */
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum	
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8  2:4x4  3:1x1 */
	  0x00, 0x2b, 0x1070, 0x0C30, 	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
      0x01, 0x30, 0x0140, 0x0180,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??
};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX	= 56,
	.i4OffsetY	= 24,
	.i4PitchX	= 64,
	.i4PitchY	= 64,
	.i4PairNum	= 16,
	.i4SubBlkW	= 16,
	.i4SubBlkH	= 16,
	.i4BlockNumX = 64,
	.i4BlockNumY = 48,
	.iMirrorFlip = 0,
	.i4PosL = 	{
		         {67,36}, {79,36}, {99,36}, {111,36}, 
				 {63,48}, {83,48}, {95,48}, {115,48}, 
				 {67,68}, {79,68}, {99,68}, {111,68}, 
				 {63,80}, {83,80}, {95,80}, {115,80}
				},
	.i4PosR = 	{
			     {67,32}, {79,32}, {99,32}, {111,32}, 
				 {63,52}, {83,52}, {95,52}, {115,52}, 
				 {67,64}, {79,64}, {99,64}, {111,64}, 
				 {63,84}, {83,84}, {95,84}, {115,84}
				}
};
#endif


#if MULTI_WRITE
#define I2C_BUFFER_LEN 512 //1020

static kal_uint16 hi1333_table_write_cmos_sensor(
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
    write_cmos_sensor(0x0006, imgsensor.frame_length & 0xFFFF);
		write_cmos_sensor(0x0008, imgsensor.line_length / 8);

}	/*	set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor(0x0F17) << 8) | read_cmos_sensor(0x0F16));
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
	kal_uint16 realtime_fps = 0;

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
	shutter = (shutter >
		(imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
		(imgsensor_info.max_frame_length - imgsensor_info.margin) :
		shutter;
	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk * 10 /
			(imgsensor.line_length * imgsensor.frame_length);
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else
			write_cmos_sensor(0x0006, imgsensor.frame_length);

	} else{
		// Extend frame length
			write_cmos_sensor(0x0006, imgsensor.frame_length);
	}

	// Update Shutter
	write_cmos_sensor_8(0x0073, ((shutter & 0xFF0000) >> 16));
	write_cmos_sensor(0x0074, shutter);// & 0x00FFFF);//DIFF
	LOG_INF("shutter =%d, framelength =%d",
		shutter, imgsensor.frame_length);
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
	
	reg_gain = reg_gain & 0x00FF;
	write_cmos_sensor_8(0x0077, reg_gain);


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
kal_uint16 addr_data_pair_init_hi1333[] = {

0x2ffe, 0xd800,
0x0e04, 0x0013,
0x3048, 0x5020,
0x0f30, 0x001f,
0x0f36, 0x001f,
0x0c00, 0x11d8,
0x0c02, 0x0011,
0x0c04, 0x5000,
0x0c06, 0x01eb,
0x0c10, 0x0040,
0x0c12, 0x0040,
0x0c14, 0x0040,
0x0c16, 0x0040,
0x0c18, 0x8000,
0x0c62, 0x0194,
0x0c64, 0x0286,
0x0c66, 0x0294,
0x0c68, 0x0100,
0x0cb2, 0x0200,
0x0714, 0xe8e8,
0x0716, 0xede8,
0x000e, 0x0100,
0x0a10, 0x400c,
0x003e, 0x0000,
0x0074, 0x0d18,
0x0a04, 0x03ea,
0x0076, 0x0000,
0x0724, 0x0f1f,
0x0068, 0x0703,
0x0060, 0x0008,
0x0062, 0x0200,
0x075e, 0x0535,
0x0012, 0x0fcd,
0x0806, 0x0002,
0x0900, 0x0300,
0x0902, 0xc319,
0x095a, 0x0099,
0x095c, 0x1111,
0x095e, 0xbac0,
0x0960, 0x5dae,
0x0a38, 0x080c,
0x0a3a, 0x140c,
0x0a3c, 0x280c,
0x0a3e, 0x340c,
0x0a40, 0x0418,
0x0a42, 0x1818,
0x0a44, 0x2418,
0x0a46, 0x3818,
0x0a48, 0x082c,
0x0a4a, 0x142c,
0x0a4c, 0x282c,
0x0a4e, 0x342c,
0x0a50, 0x0438,
0x0a52, 0x1838,
0x0a54, 0x2438,
0x0a56, 0x3838,
0x0a58, 0x0808,
0x0a5a, 0x1408,
0x0a5c, 0x2808,
0x0a5e, 0x3408,
0x0a60, 0x041c,
0x0a62, 0x181c,
0x0a64, 0x241c,
0x0a66, 0x381c,
0x0a68, 0x0828,
0x0a6a, 0x1428,
0x0a6c, 0x2828,
0x0a6e, 0x3428,
0x0a70, 0x043c,
0x0a72, 0x183c,
0x0a74, 0x243c,
0x0a76, 0x383c,
0x0404, 0x015c,
0x0406, 0x0138,
0x040a, 0x0115,
0x040c, 0x022a,
0x040e, 0xbc52,
0x0410, 0x0056,
0x0412, 0x00ac,
0x0414, 0x6907,
0x0422, 0x0011,
0x0424, 0x0023,
0x0426, 0x2f15,
0x0428, 0x0016,
0x042a, 0x002b,
0x042c, 0x3483,
0x0324, 0x0100,
0x0600, 0x003e,
0x0a78, 0x0400,


};
#endif

static void sensor_init(void)
{
LOG_INF(" sensor init E\n");

#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0000); // stream off
mdelay(10);
#endif

#if MULTI_WRITE
LOG_INF(" sensor init MULTI_WRITE E\n");
	hi1333_table_write_cmos_sensor(
		addr_data_pair_init_hi1333,
		sizeof(addr_data_pair_init_hi1333) /
		sizeof(kal_uint16));
#else
LOG_INF(" sensor init \n");
write_cmos_sensor(0x2ffe, 0xd800);
write_cmos_sensor(0x0e04, 0x0013);
write_cmos_sensor(0x3048, 0x5020);
write_cmos_sensor(0x0f30, 0x001f);
write_cmos_sensor(0x0f36, 0x001f);
write_cmos_sensor(0x0c00, 0x11d8);
write_cmos_sensor(0x0c02, 0x0011);
write_cmos_sensor(0x0c04, 0x5000);
write_cmos_sensor(0x0c06, 0x01eb);
write_cmos_sensor(0x0c10, 0x0040);
write_cmos_sensor(0x0c12, 0x0040);
write_cmos_sensor(0x0c14, 0x0040);
write_cmos_sensor(0x0c16, 0x0040);
write_cmos_sensor(0x0c18, 0x8000);
write_cmos_sensor(0x0c62, 0x0194);
write_cmos_sensor(0x0c64, 0x0286);
write_cmos_sensor(0x0c66, 0x0294);
write_cmos_sensor(0x0c68, 0x0100);
write_cmos_sensor(0x0cb2, 0x0200);
write_cmos_sensor(0x0714, 0xe8e8);
write_cmos_sensor(0x0716, 0xede8);
write_cmos_sensor(0x000e, 0x0100);
write_cmos_sensor(0x0a10, 0x400c);
write_cmos_sensor(0x003e, 0x0000);
write_cmos_sensor(0x0074, 0x0d18);
write_cmos_sensor(0x0a04, 0x03ea);
write_cmos_sensor(0x0076, 0x0000);
write_cmos_sensor(0x0724, 0x0f1f);
write_cmos_sensor(0x0068, 0x0703);
write_cmos_sensor(0x0060, 0x0008);
write_cmos_sensor(0x0062, 0x0200);
write_cmos_sensor(0x075e, 0x0535);
write_cmos_sensor(0x0012, 0x0fcd);
write_cmos_sensor(0x0806, 0x0002);
write_cmos_sensor(0x0900, 0x0300);
write_cmos_sensor(0x0902, 0xc319);
write_cmos_sensor(0x095a, 0x0099);
write_cmos_sensor(0x095c, 0x1111);
write_cmos_sensor(0x095e, 0xbac0);
write_cmos_sensor(0x0960, 0x5dae);
write_cmos_sensor(0x0a38, 0x080c);
write_cmos_sensor(0x0a3a, 0x140c);
write_cmos_sensor(0x0a3c, 0x280c);
write_cmos_sensor(0x0a3e, 0x340c);
write_cmos_sensor(0x0a40, 0x0418);
write_cmos_sensor(0x0a42, 0x1818);
write_cmos_sensor(0x0a44, 0x2418);
write_cmos_sensor(0x0a46, 0x3818);
write_cmos_sensor(0x0a48, 0x082c);
write_cmos_sensor(0x0a4a, 0x142c);
write_cmos_sensor(0x0a4c, 0x282c);
write_cmos_sensor(0x0a4e, 0x342c);
write_cmos_sensor(0x0a50, 0x0438);
write_cmos_sensor(0x0a52, 0x1838);
write_cmos_sensor(0x0a54, 0x2438);
write_cmos_sensor(0x0a56, 0x3838);
write_cmos_sensor(0x0a58, 0x0808);
write_cmos_sensor(0x0a5a, 0x1408);
write_cmos_sensor(0x0a5c, 0x2808);
write_cmos_sensor(0x0a5e, 0x3408);
write_cmos_sensor(0x0a60, 0x041c);
write_cmos_sensor(0x0a62, 0x181c);
write_cmos_sensor(0x0a64, 0x241c);
write_cmos_sensor(0x0a66, 0x381c);
write_cmos_sensor(0x0a68, 0x0828);
write_cmos_sensor(0x0a6a, 0x1428);
write_cmos_sensor(0x0a6c, 0x2828);
write_cmos_sensor(0x0a6e, 0x3428);
write_cmos_sensor(0x0a70, 0x043c);
write_cmos_sensor(0x0a72, 0x183c);
write_cmos_sensor(0x0a74, 0x243c);
write_cmos_sensor(0x0a76, 0x383c);
write_cmos_sensor(0x0404, 0x015c);
write_cmos_sensor(0x0406, 0x0138);
write_cmos_sensor(0x040a, 0x0115);
write_cmos_sensor(0x040c, 0x022a);
write_cmos_sensor(0x040e, 0xbc52);
write_cmos_sensor(0x0410, 0x0056);
write_cmos_sensor(0x0412, 0x00ac);
write_cmos_sensor(0x0414, 0x6907);
write_cmos_sensor(0x0422, 0x0011);
write_cmos_sensor(0x0424, 0x0023);
write_cmos_sensor(0x0426, 0x2f15);
write_cmos_sensor(0x0428, 0x0016);
write_cmos_sensor(0x042a, 0x002b);
write_cmos_sensor(0x042c, 0x3483);
write_cmos_sensor(0x0324, 0x0100);
write_cmos_sensor(0x0600, 0x003e);
write_cmos_sensor(0x0a78, 0x0400);
#endif
/*
#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0100); // stream on
mdelay(10);
#endif
*/
LOG_INF(" sensor init X \n");
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_preview_hi1333[] = {
//Sensor Information////////////////////////////
//Sensor	  : SL-1333
//Date		  : 2018-02-22
//Customer        : LGE
//Image size	  : 2104x1560
//MCLK		  : 24MHz
//MIPI speed(Mbps): 726Mbps x 4Lane | Continuous mode
//Frame Length	  : 3359
//Line Length 	  : 5952
//Max Fps 	  : 30.0fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offse	  : 64code
////////////////////////////////////////////////
0x0f38, 0x0379,
0x0f3a, 0x4107,
0x093e, 0x0100,
0x0920, 0xc103,
0x0922, 0x030d,
0x0924, 0x0203,
0x0926, 0x0606,
0x0928, 0x0704,
0x092a, 0x0505,
0x092c, 0x0a00,
0x0910, 0x033e,
0x0912, 0x0054,
0x0914, 0x0002,
0x0916, 0x0002,
0x091a, 0x0001,
0x0938, 0x0000,
0x0904, 0x2bb0,
0x0942, 0x0008,

//0x0944, 0x0100,
0x0944, 0x0180,
0x0946, 0x0004,

0x0520, 0x0100,
0x0532, 0x0100,
0x0a2a, 0x8060,
0x0a2c, 0x2020,
0x0a32, 0x0301,
0x0a26, 0x0048,
0x0a28, 0x001c,
0x0a36, 0x0000,
0x0408, 0x0000,
0x0418, 0x0000,
0x0800, 0x0400,
0x0008, 0x02e8,
0x000c, 0x000c,
0x0804, 0x0008,
0x0026, 0x003c,
0x002c, 0x0c71,
0x005c, 0x0204,
0x002e, 0x1111,
0x0032, 0x3311,
0x0006, 0x0d1f,
0x0074, 0x0d18,
0x0a0e, 0x0002,
0x0a12, 0x0838,
0x0a14, 0x0618,
0x075c, 0x0100,
0x0050, 0x4300,
0x0722, 0x0700,
0x004c, 0x0100,
};
#endif

static void preview_setting(void)
{
LOG_INF(" sensor preview_setting \n");
#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0000); // stream off
mdelay(10);
#endif
#if MULTI_WRITE
	hi1333_table_write_cmos_sensor(
		addr_data_pair_preview_hi1333,
		sizeof(addr_data_pair_preview_hi1333) /
		sizeof(kal_uint16));
#else
//Sensor Information////////////////////////////
//Sensor	  : SL-1333
//Date		  : 2018-02-22
//Customer        : LGE
//Image size	  : 2104x1560
//MCLK		  : 24MHz
//MIPI speed(Mbps): 726Mbps x 4Lane | Continuous mode
//Frame Length	  : 3359
//Line Length 	  : 5952
//Max Fps 	  : 30.0fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offse	  : 64code
////////////////////////////////////////////////
write_cmos_sensor(0x0f38, 0x0379);
write_cmos_sensor(0x0f3a, 0x4107);
write_cmos_sensor(0x093e, 0x0100);
write_cmos_sensor(0x0920, 0xc103);
write_cmos_sensor(0x0922, 0x030d);
write_cmos_sensor(0x0924, 0x0203);
write_cmos_sensor(0x0926, 0x0606);
write_cmos_sensor(0x0928, 0x0704);
write_cmos_sensor(0x092a, 0x0505);
write_cmos_sensor(0x092c, 0x0a00);
write_cmos_sensor(0x0910, 0x033e);
write_cmos_sensor(0x0912, 0x0054);
write_cmos_sensor(0x0914, 0x0002);
write_cmos_sensor(0x0916, 0x0002);
write_cmos_sensor(0x091a, 0x0001);
write_cmos_sensor(0x0938, 0x0000);
write_cmos_sensor(0x0904, 0x2bab);
write_cmos_sensor(0x0942, 0x0008);
write_cmos_sensor(0x0944, 0x0100);
write_cmos_sensor(0x0946, 0x0004);
write_cmos_sensor(0x0520, 0x0100);
write_cmos_sensor(0x0532, 0x0100);
write_cmos_sensor(0x0a2a, 0x8060);
write_cmos_sensor(0x0a2c, 0x2020);
write_cmos_sensor(0x0a32, 0x0301);
write_cmos_sensor(0x0a26, 0x0048);
write_cmos_sensor(0x0a28, 0x001c);
write_cmos_sensor(0x0a36, 0x0000);
write_cmos_sensor(0x0408, 0x0000);
write_cmos_sensor(0x0418, 0x0000);
write_cmos_sensor(0x0800, 0x0400);
write_cmos_sensor(0x0008, 0x02e8);
write_cmos_sensor(0x000c, 0x000c);
write_cmos_sensor(0x0804, 0x0008);
write_cmos_sensor(0x0026, 0x003c);
write_cmos_sensor(0x002c, 0x0c71);
write_cmos_sensor(0x005c, 0x0204);
write_cmos_sensor(0x002e, 0x1111);
write_cmos_sensor(0x0032, 0x3311);
write_cmos_sensor(0x0006, 0x0d1f);
write_cmos_sensor(0x0074, 0x0d18);
write_cmos_sensor(0x0a0e, 0x0002);
write_cmos_sensor(0x0a12, 0x0838);
write_cmos_sensor(0x0a14, 0x0618);
write_cmos_sensor(0x075c, 0x0100);
write_cmos_sensor(0x0050, 0x4300);
write_cmos_sensor(0x0722, 0x0700);
write_cmos_sensor(0x004c, 0x0100);
#endif

#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0100); // stream on
mdelay(10);
#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_capture_30fps_hi1333[] = {
//Sensor Information////////////////////////////
//Sensor	  : SL-1333
//Date		  : 2018-02-22
//Customer        : LGE
//Image size	  : 4208x3120
//MCLK		  : 24MHz
//MIPI speed(Mbps): 1.452Gbps x 4Lane | Continuous mode
//Frame Length	  : 3359
//Line Length 	  : 5952
//Max Fps 	  : 30.0fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offse	  : 64code
////////////////////////////////////////////////
0x0f38, 0x0379,
0x0f3a, 0x4007,
0x093e, 0x0000,
0x0920, 0xc106,
0x0922, 0x061a,
0x0924, 0x0207,
0x0926, 0x0b09,
0x0928, 0x0c08,
0x092a, 0x0a06,
0x092c, 0x1600,
0x0910, 0x0696,
0x0912, 0x00bb,
0x0914, 0x002d,
0x0916, 0x002d,
0x091a, 0x002a,
0x0938, 0x4100,

0x0904, 0x2bb0,
0x0942, 0x0108,		// 10bit
0x0944, 0x0100,
0x0946, 0x0004,
0x0520, 0x0100,
0x0532, 0x0100,
0x0a2a, 0x8060,
0x0a2c, 0x2020,
0x0a32, 0x0301,
0x0a26, 0x0048,
0x0a28, 0x001a,
0x0a36, 0x0000,
0x0408, 0x0202,
0x0418, 0x0000,
0x0800, 0x0000,
0x0008, 0x02e8,
0x000c, 0x000c,
0x0804, 0x0010,
0x0026, 0x003e,
0x002c, 0x0c71,
0x005c, 0x0202,
0x002e, 0x1111,
0x0032, 0x1111,
0x0006, 0x0d1f,
0x0074, 0x0d18,
0x0a0e, 0x0001,
0x0a12, 0x1070,
0x0a14, 0x0c30,
0x075c, 0x0100,
0x0050, 0x4300,
0x0722, 0x0700,
0x004c, 0x0100,
};

kal_uint16 addr_data_pair_capture_15fps_hi1333[] = {
0x0f38, 0x077d,  //pll_cfg_mipi1 b7-0: 4C mipi_mdiv
0x0f3a, 0x4007,  //pll_cfg_mipi2
0x093e, 0x0000,  //mipi_tx_col_read_ctrl
0x0920, 0xc103,  //mipi_exit_seq, tlpx
0x0922, 0x030d,  //tclk_prepare, tclk_zero
0x0924, 0x0204,  //tclk_pre, ths_prepare
0x0926, 0x0606,  //ths_zero, ths_trail
0x0928, 0x0704,  //tclk_post, tclk_trail
0x092a, 0x0505,  //texit, tsync
0x092c, 0x0a00,  //tpd_sync
0x0910, 0x036a,  //mipi_vblank_delay
0x0918, 0x0365,  //mipi_pd_vblank_delay
0x0912, 0x00eb,  //mipi_hblank_delay
0x0914, 0x004e,  //mipi_hblank_short_delay1
0x0916, 0x004e,  //mipi_hblank_short_delay2
0x091a, 0x004a,  //mipi_pd_hblank_delay
0x0938, 0x0000,  //0x4100);  //mipi_virtual_channel_ctrl
0x0904, 0x2bab,  //mipi_data_id_ctrl, mipi_pd_data_id_ctrl
0x0942, 0x0008,  //0x0108);  //mipi_pd_sep_ctrl1, mipi_pd_sep_ctrl2
0x0944, 0x0100,  //mipi_pd_col_size
0x0946, 0x0004,  //mipi_pd_row_size
0x0a2a, 0x8060,  //PDAF patch x/y num normal
0x0a2c, 0x2020,  //PDAF patch x/y size normal
0x0a32, 0x0301,  //PDAF cnt012 normal
0x0a26, 0x0048,  //PDAF x patch offset normal
0x0a28, 0x001a,  //PDAF y patch offset normal
0x0a36, 0x0000,  //PDAF win loc sx/sy normal
0x0408, 0x0202,  //lsc spare
0x0418, 0x0000,  //lsc win_h
0x0c74, 0x00ed,  //act_win_ystart
0x0c78, 0x0adb,  //act_win_yend
0x0a7e, 0x0005,  //lsc shift
0x0800, 0x0000,  //fmt ctrl
0x0008, 0x05d0,  //line length pck
0x000c, 0x000c,  //colgen start
0x0804, 0x0010,  //fmt x cropping
0x0026, 0x003e,  //y addr start active
0x002c, 0x0c71,  //y addr end active
0x005c, 0x0202,  //y dummy size
0x002e, 0x1111,  //y even/odd inc tobp
0x0032, 0x1111,  //y even/odd inc active
0x0006, 0x0d20,  //frame length lines
0x0074, 0x0d19,  //coarse integ time
0x0a0e, 0x0001,  //image mode/digial binning mode
0x0a12, 0x1070,  //x output size
0x0a14, 0x0c30,  //y output size
0x075c, 0x0100,  //OTP ctrl b5:lsc_flag b4:lsc_checksum b3:pdlsc_en b2:dga_en b1:lsc_en b0:adpc_en 
0x0050, 0x4300,  //analog control b7:vblank_analog_off b6:ag_cal b5:pd_rst2 b4:rdo_set_off b3:pat_addr_en b2:sub3 b1:sreg re-load b0:sreg write 
0x0722, 0x0700,  //d2a_pxl_drv_pwr/d2a_row_binning_en
0x004c, 0x0100,  //tg enable,hdr off
};
#endif


static void capture_setting(kal_uint16 currefps)
{
#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0000); // stream off
mdelay(10);
#endif

#if MULTI_WRITE
	if (currefps == 300) {
	hi1333_table_write_cmos_sensor(
		addr_data_pair_capture_30fps_hi1333,
		sizeof(addr_data_pair_capture_30fps_hi1333) /
		sizeof(kal_uint16));
	} else {
	hi1333_table_write_cmos_sensor(
		addr_data_pair_capture_15fps_hi1333,
		sizeof(addr_data_pair_capture_15fps_hi1333) /
		sizeof(kal_uint16));
	}
#else
  if( currefps == 300) {
//Sensor Information////////////////////////////
//Sensor	  : SL-1333
//Date		  : 2018-02-22
//Customer        : LGE
//Image size	  : 4208x3120
//MCLK		  : 24MHz
//MIPI speed(Mbps): 1.452Gbps x 4Lane | Continuous mode
//Frame Length	  : 3359
//Line Length 	  : 5952
//Max Fps 	  : 30.0fps
//Pixel order 	  : Green 1st (=GB)
//X/Y-flip	  : X-flip
//BLC offse	  : 64code
////////////////////////////////////////////////
write_cmos_sensor(0x0f38, 0x0379);
write_cmos_sensor(0x0f3a, 0x4007);
write_cmos_sensor(0x093e, 0x0000);
write_cmos_sensor(0x0920, 0xc106);
write_cmos_sensor(0x0922, 0x061a);
write_cmos_sensor(0x0924, 0x0207);
write_cmos_sensor(0x0926, 0x0b09);
write_cmos_sensor(0x0928, 0x0c08);
write_cmos_sensor(0x092a, 0x0a06);
write_cmos_sensor(0x092c, 0x1600);
write_cmos_sensor(0x0910, 0x0696);
write_cmos_sensor(0x0912, 0x00bb);
write_cmos_sensor(0x0914, 0x002d);
write_cmos_sensor(0x0916, 0x002d);
write_cmos_sensor(0x091a, 0x002a);
write_cmos_sensor(0x0938, 0x4100);
write_cmos_sensor(0x0904, 0x2bb0);
write_cmos_sensor(0x0942, 0x0108);
write_cmos_sensor(0x0944, 0x0100);
write_cmos_sensor(0x0946, 0x0004);
write_cmos_sensor(0x0520, 0x0100);
write_cmos_sensor(0x0532, 0x0100);
write_cmos_sensor(0x0a2a, 0x8060);
write_cmos_sensor(0x0a2c, 0x2020);
write_cmos_sensor(0x0a32, 0x0301);
write_cmos_sensor(0x0a26, 0x0048);
write_cmos_sensor(0x0a28, 0x001a);
write_cmos_sensor(0x0a36, 0x0000);
write_cmos_sensor(0x0408, 0x0202);
write_cmos_sensor(0x0418, 0x0000);
write_cmos_sensor(0x0800, 0x0000);
write_cmos_sensor(0x0008, 0x02e8);
write_cmos_sensor(0x000c, 0x000c);
write_cmos_sensor(0x0804, 0x0010);
write_cmos_sensor(0x0026, 0x003e);
write_cmos_sensor(0x002c, 0x0c71);
write_cmos_sensor(0x005c, 0x0202);
write_cmos_sensor(0x002e, 0x1111);
write_cmos_sensor(0x0032, 0x1111);
write_cmos_sensor(0x0006, 0x0d1f);
write_cmos_sensor(0x0074, 0x0d18);
write_cmos_sensor(0x0a0e, 0x0001);
write_cmos_sensor(0x0a12, 0x1070);
write_cmos_sensor(0x0a14, 0x0c30);
write_cmos_sensor(0x075c, 0x0100);
write_cmos_sensor(0x0050, 0x4300);
write_cmos_sensor(0x0722, 0x0700);
write_cmos_sensor(0x004c, 0x0100);
// PIP
  } else	{
write_cmos_sensor(0x0f38, 0x077d);  //pll_cfg_mipi1 b7-0: 4C mipi_mdiv
write_cmos_sensor(0x0f3a, 0x4007);  //pll_cfg_mipi2
write_cmos_sensor(0x093e, 0x0000);  //mipi_tx_col_read_ctrl
write_cmos_sensor(0x0920, 0xc103);  //mipi_exit_seq, tlpx
write_cmos_sensor(0x0922, 0x030d);  //tclk_prepare, tclk_zero
write_cmos_sensor(0x0924, 0x0204);  //tclk_pre, ths_prepare
write_cmos_sensor(0x0926, 0x0606);  //ths_zero, ths_trail
write_cmos_sensor(0x0928, 0x0704);  //tclk_post, tclk_trail
write_cmos_sensor(0x092a, 0x0505);  //texit, tsync
write_cmos_sensor(0x092c, 0x0a00);  //tpd_sync
write_cmos_sensor(0x0910, 0x036a);  //mipi_vblank_delay
write_cmos_sensor(0x0918, 0x0365);  //mipi_pd_vblank_delay
write_cmos_sensor(0x0912, 0x00eb);  //mipi_hblank_delay
write_cmos_sensor(0x0914, 0x004e);  //mipi_hblank_short_delay1
write_cmos_sensor(0x0916, 0x004e);  //mipi_hblank_short_delay2
write_cmos_sensor(0x091a, 0x004a);  //mipi_pd_hblank_delay
write_cmos_sensor(0x0938, 0x0000);  //0x4100);  //mipi_virtual_channel_ctrl
write_cmos_sensor(0x0904, 0x2bab);  //mipi_data_id_ctrl, mipi_pd_data_id_ctrl
write_cmos_sensor(0x0942, 0x0008);  //0x0108);  //mipi_pd_sep_ctrl1, mipi_pd_sep_ctrl2
write_cmos_sensor(0x0944, 0x0100);  //mipi_pd_col_size
write_cmos_sensor(0x0946, 0x0004);  //mipi_pd_row_size
write_cmos_sensor(0x0a2a, 0x8060);  //PDAF patch x/y num normal
write_cmos_sensor(0x0a2c, 0x2020);  //PDAF patch x/y size normal
write_cmos_sensor(0x0a32, 0x0301);  //PDAF cnt012 normal
write_cmos_sensor(0x0a26, 0x0048);  //PDAF x patch offset normal
write_cmos_sensor(0x0a28, 0x001a);  //PDAF y patch offset normal
write_cmos_sensor(0x0a36, 0x0000);  //PDAF win loc sx/sy normal
write_cmos_sensor(0x0408, 0x0202);  //lsc spare
write_cmos_sensor(0x0418, 0x0000);  //lsc win_h
write_cmos_sensor(0x0c74, 0x00ed);  //act_win_ystart
write_cmos_sensor(0x0c78, 0x0adb);  //act_win_yend
write_cmos_sensor(0x0a7e, 0x0005);  //lsc shift
write_cmos_sensor(0x0800, 0x0000);  //fmt ctrl
write_cmos_sensor(0x0008, 0x05d0);  //line length pck
write_cmos_sensor(0x000c, 0x000c);  //colgen start
write_cmos_sensor(0x0804, 0x0010);  //fmt x cropping
write_cmos_sensor(0x0026, 0x003e);  //y addr start active
write_cmos_sensor(0x002c, 0x0c71);  //y addr end active
write_cmos_sensor(0x005c, 0x0202);  //y dummy size
write_cmos_sensor(0x002e, 0x1111);  //y even/odd inc tobp
write_cmos_sensor(0x0032, 0x1111);  //y even/odd inc active
write_cmos_sensor(0x0006, 0x0d20);  //frame length lines
write_cmos_sensor(0x0074, 0x0d19);  //coarse integ time
write_cmos_sensor(0x0a0e, 0x0001);  //image mode/digial binning mode
write_cmos_sensor(0x0a12, 0x1070);  //x output size
write_cmos_sensor(0x0a14, 0x0c30);  //y output size
write_cmos_sensor(0x075c, 0x0100);  //OTP ctrl b5:lsc_flag b4:lsc_checksum b3:pdlsc_en b2:dga_en b1:lsc_en b0:adpc_en 
write_cmos_sensor(0x0050, 0x4300);  //analog control b7:vblank_analog_off b6:ag_cal b5:pd_rst2 b4:rdo_set_off b3:pat_addr_en b2:sub3 b1:sreg re-load b0:sreg write 
write_cmos_sensor(0x0722, 0x0700);  //d2a_pxl_drv_pwr/d2a_row_binning_en
write_cmos_sensor(0x004c, 0x0100);  //tg enable,hdr off
	}
#endif


#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0100); // stream on
mdelay(10);
#endif
}



#if MULTI_WRITE
kal_uint16 addr_data_pair_hs_video_hi1333[] = {
0x0f38, 0x037d, //pll_cfg_mipi1
0x0f3a, 0x4407, //pll_cfg_mipi2 b10-8:100 mipi 1/6 
0x093e, 0x0500, //mipi_tx_col_read_ctrl
0x0920, 0xc101, //mipi_exit_seq, tlpx
0x0922, 0x0105, //tclk_prepare, tclk_zero
0x0924, 0x0201, //tclk_pre, ths_prepare
0x0926, 0x0103, //ths_zero, ths_trail
0x0928, 0x0602, //tclk_post, tclk_trail
0x092a, 0x0203, //texit, tsync
0x092c, 0x0100, //tpd_sync
0x0910, 0x007b, //mipi_vblank_delay
0x0918, 0x0077, //mipi_pd_vblank_delay
0x0912, 0x001f, //mipi_hblank_delay
0x0938, 0x0000, //0x4100); //mipi_virtual_channel_ctrl  // ADD ODIN
0x0942, 0x0008, //mipi_pd_sep_ctrl1, mipi_pd_sep_ctrl2 0x0108 -> PD / 0x0008 -> nonPD
0x0a2a, 0x4030, //PDAF patch x/y num vga
0x0a2c, 0x4040, //PDAF patch x/y size vga
0x0a32, 0x030c, //PDAF cnt012 vga
0x0a26, 0x0048, //PDAF x patch offset vga
0x0a28, 0x0000, //PDAF y patch offset vga
0x0a36, 0x0002, //PDAF win loc sx/sy vga
0x0408, 0x0200, //lsc spare
0x0418, 0x006c, //lsc win_h
0x0c74, 0x0052, //act_win_ystart
0x0c78, 0x018f, //act_win_yend
0x0a7e, 0x0002, //lsc shift
0x0800, 0x1400, //fmt ctrl
0x0008, 0x02e8, //line length pck
0x000c, 0x000c, //colgen start
0x0804, 0x0022, //fmt x cropping
0x0026, 0x00ac, //y addr start active
0x002c, 0x0bff, //y addr end active
0x005c, 0x040c, //y dummy size
0x002e, 0x3311, //y even/odd inc tobp
0x0032, 0x5577, //y even/odd inc active
0x0006, 0x0348, //frame length lines
0x0074, 0x0341, //coarse integ time
0x0a0e, 0x0006, //image mode/digial binning mode
0x0a12, 0x0280, //x output size
0x0a14, 0x01e0, //y output size
0x075c, 0x0100, //OTP ctrl b5:lsc_flag b4:lsc_checksum b3:pdlsc_en b2:dga_en b1:lsc_en b0:adpc_en 
0x0050, 0x4720, //analog control b7:vblank_analog_off b6:ag_cal b5:pd_rst2 b4:rdo_set_off b3:pat_addr_en b2:sub3 b1:sreg re-load b0:sreg write 
0x0722, 0x0702, //d2a_pxl_drv_pwr/d2a_row_binning_en - on
0x004c, 0x0100, //tg enable,hdr off


};
#endif

static void hs_video_setting(void)
{
#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0000); // stream off
mdelay(10);
#endif
#if MULTI_WRITE
	hi1333_table_write_cmos_sensor(
		addr_data_pair_hs_video_hi1333,
		sizeof(addr_data_pair_hs_video_hi1333) /
		sizeof(kal_uint16));
#else
// setting ver5.0 None PD 640x480 120fps
write_cmos_sensor(0x0f38, 0x037d); //pll_cfg_mipi1
write_cmos_sensor(0x0f3a, 0x4407); //pll_cfg_mipi2 b10-8:100 mipi 1/6 
write_cmos_sensor(0x093e, 0x0500); //mipi_tx_col_read_ctrl
write_cmos_sensor(0x0920, 0xc101); //mipi_exit_seq, tlpx
write_cmos_sensor(0x0922, 0x0105); //tclk_prepare, tclk_zero
write_cmos_sensor(0x0924, 0x0201); //tclk_pre, ths_prepare
write_cmos_sensor(0x0926, 0x0103); //ths_zero, ths_trail
write_cmos_sensor(0x0928, 0x0602); //tclk_post, tclk_trail
write_cmos_sensor(0x092a, 0x0203); //texit, tsync
write_cmos_sensor(0x092c, 0x0100); //tpd_sync
write_cmos_sensor(0x0910, 0x007b); //mipi_vblank_delay
write_cmos_sensor(0x0918, 0x0077); //mipi_pd_vblank_delay
write_cmos_sensor(0x0912, 0x001f); //mipi_hblank_delay
write_cmos_sensor(0x0938, 0x0000); //0x4100); //mipi_virtual_channel_ctrl  // ADD ODIN
write_cmos_sensor(0x0942, 0x0008); //mipi_pd_sep_ctrl1, mipi_pd_sep_ctrl2 0x0108 -> PD / 0x0008 -> nonPD
write_cmos_sensor(0x0a2a, 0x4030); //PDAF patch x/y num vga
write_cmos_sensor(0x0a2c, 0x4040); //PDAF patch x/y size vga
write_cmos_sensor(0x0a32, 0x030c); //PDAF cnt012 vga
write_cmos_sensor(0x0a26, 0x0048); //PDAF x patch offset vga
write_cmos_sensor(0x0a28, 0x0000); //PDAF y patch offset vga
write_cmos_sensor(0x0a36, 0x0002); //PDAF win loc sx/sy vga
write_cmos_sensor(0x0408, 0x0200); //lsc spare
write_cmos_sensor(0x0418, 0x006c); //lsc win_h
write_cmos_sensor(0x0c74, 0x0052); //act_win_ystart
write_cmos_sensor(0x0c78, 0x018f); //act_win_yend
write_cmos_sensor(0x0a7e, 0x0002); //lsc shift
write_cmos_sensor(0x0800, 0x1400); //fmt ctrl
write_cmos_sensor(0x0008, 0x02e8); //line length pck
write_cmos_sensor(0x000c, 0x000c); //colgen start
write_cmos_sensor(0x0804, 0x0022); //fmt x cropping
write_cmos_sensor(0x0026, 0x00ac); //y addr start active
write_cmos_sensor(0x002c, 0x0bff); //y addr end active
write_cmos_sensor(0x005c, 0x040c); //y dummy size
write_cmos_sensor(0x002e, 0x3311); //y even/odd inc tobp
write_cmos_sensor(0x0032, 0x5577); //y even/odd inc active
write_cmos_sensor(0x0006, 0x0348); //frame length lines
write_cmos_sensor(0x0074, 0x0341); //coarse integ time
write_cmos_sensor(0x0a0e, 0x0006); //image mode/digial binning mode
write_cmos_sensor(0x0a12, 0x0280); //x output size
write_cmos_sensor(0x0a14, 0x01e0); //y output size
write_cmos_sensor(0x075c, 0x0100); //OTP ctrl b5:lsc_flag b4:lsc_checksum b3:pdlsc_en b2:dga_en b1:lsc_en b0:adpc_en 
write_cmos_sensor(0x0050, 0x4720); //analog control b7:vblank_analog_off b6:ag_cal b5:pd_rst2 b4:rdo_set_off b3:pat_addr_en b2:sub3 b1:sreg re-load b0:sreg write 
write_cmos_sensor(0x0722, 0x0702); //d2a_pxl_drv_pwr/d2a_row_binning_en - on
write_cmos_sensor(0x004c, 0x0100); //tg enable,hdr off


#endif

#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0100); // stream on
mdelay(10);
#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_slim_video_hi1333[] = {
0x0f38, 0x037d,  //pll_cfg_mipi1
0x0f3a, 0x4207,  //pll_cfg_mipi2 b10-8:010 mipi 1/3
0x093e, 0x0200,  //mipi_tx_col_read_ctrl
0x0920, 0xc102,  //mipi_exit_seq, tlpx
0x0922, 0x0209,  //tclk_prepare, tclk_zero
0x0924, 0x0203,  //tclk_pre, ths_prepare
0x0926, 0x0304,  //ths_zero, ths_trail
0x0928, 0x0703,  //tclk_post, tclk_trail
0x092a, 0x0304,  //texit, tsync
0x092c, 0x0400,  //tpd_sync
0x0910, 0x0112,  //mipi_vblank_delay
0x0918, 0x010d,  //mipi_pd_vblank_delay
0x0912, 0x0057,  //mipi_hblank_delay
0x0938, 0x0000,  //0x4100);  //mipi_virtual_channel_ctrl
0x0942, 0x0008,  //0x0108);  //mipi_pd_sep_ctrl1, mipi_pd_sep_ctrl2
0x0a2a, 0x4030,  //PDAF patch x/y num hd
0x0a2c, 0x4040,  //PDAF patch x/y size hd
0x0a32, 0x1300,  //PDAF cnt012 hd
0x0a26, 0x0048,  //PDAF x patch offset hd
0x0a28, 0x0000,  //PDAF y patch offset hd
0x0a36, 0x0000,  //PDAF win loc sx/sy hd
0x0408, 0x0200,  //lsc spare
0x0418, 0x01a2,  //lsc win_h
0x0c74, 0x002b,  //act_win_ystart
0x0c78, 0x017a,  //act_win_yend
0x0a7e, 0x0003,  //lsc shift
0x0800, 0x0800,  //fmt ctrl
0x0008, 0x02e8,  //line length pck
0x000c, 0x000c,  //colgen start
0x0804, 0x0042,  //fmt x cropping
0x0026, 0x021a,  //y addr start active
0x002c, 0x0a93,  //y addr end active
0x005c, 0x0206,  //y dummy size
0x002e, 0x1111,  //y even/odd inc tobp
0x0032, 0x3333,  //y even/odd inc active
0x0006, 0x0348,  //frame length lines
0x0074, 0x0341,  //coarse integ time
0x0a0e, 0x0003,  //image mode/digial binning mode
0x0a12, 0x0500,  //x output size
0x0a14, 0x02d0,  //y output size
0x075c, 0x0100,  //OTP ctrl b5:lsc_flag b4:lsc_checksum b3:pdlsc_en b2:dga_en b1:lsc_en b0:adpc_en 
0x0050, 0x4720,  //analog control b7:vblank_analog_off b6:ag_cal b5:pd_rst2 b4:rdo_set_off b3:pat_addr_en b2:sub3 b1:sreg re-load b0:sreg write 
0x0722, 0x0700,  //d2a_pxl_drv_pwr/d2a_row_binning_en
0x004c, 0x0100,  //tg enable,hdr off
};
#endif


static void slim_video_setting(void)
{

#if MULTI_WRITE
	hi1333_table_write_cmos_sensor(
		addr_data_pair_slim_video_hi1333,
		sizeof(addr_data_pair_slim_video_hi1333) /
		sizeof(kal_uint16));
#else
//////////////////////////////////
//  HD 1280x720 120fps
//
//////////////////////////////////
write_cmos_sensor(0x0f38, 0x037d);  //pll_cfg_mipi1
write_cmos_sensor(0x0f3a, 0x4207);  //pll_cfg_mipi2 b10-8:010 mipi 1/3
write_cmos_sensor(0x093e, 0x0200);  //mipi_tx_col_read_ctrl
write_cmos_sensor(0x0920, 0xc102);  //mipi_exit_seq, tlpx
write_cmos_sensor(0x0922, 0x0209);  //tclk_prepare, tclk_zero
write_cmos_sensor(0x0924, 0x0203);  //tclk_pre, ths_prepare
write_cmos_sensor(0x0926, 0x0304);  //ths_zero, ths_trail
write_cmos_sensor(0x0928, 0x0703);  //tclk_post, tclk_trail
write_cmos_sensor(0x092a, 0x0304);  //texit, tsync
write_cmos_sensor(0x092c, 0x0400);  //tpd_sync
write_cmos_sensor(0x0910, 0x0112);  //mipi_vblank_delay
write_cmos_sensor(0x0918, 0x010d);  //mipi_pd_vblank_delay
write_cmos_sensor(0x0912, 0x0057);  //mipi_hblank_delay
write_cmos_sensor(0x0938, 0x0000);  //0x4100);  //mipi_virtual_channel_ctrl

write_cmos_sensor(0x0942, 0x0008);  //0x0108);  //mipi_pd_sep_ctrl1, mipi_pd_sep_ctrl2
write_cmos_sensor(0x0a2a, 0x4030);  //PDAF patch x/y num hd
write_cmos_sensor(0x0a2c, 0x4040);  //PDAF patch x/y size hd
write_cmos_sensor(0x0a32, 0x1300);  //PDAF cnt012 hd
write_cmos_sensor(0x0a26, 0x0048);  //PDAF x patch offset hd
write_cmos_sensor(0x0a28, 0x0000);  //PDAF y patch offset hd
write_cmos_sensor(0x0a36, 0x0000);  //PDAF win loc sx/sy hd
write_cmos_sensor(0x0408, 0x0200);  //lsc spare
write_cmos_sensor(0x0418, 0x01a2);  //lsc win_h
write_cmos_sensor(0x0c74, 0x002b);  //act_win_ystart
write_cmos_sensor(0x0c78, 0x017a);  //act_win_yend
write_cmos_sensor(0x0a7e, 0x0003);  //lsc shift
write_cmos_sensor(0x0800, 0x0800);  //fmt ctrl
write_cmos_sensor(0x0008, 0x02e8);  //line length pck
write_cmos_sensor(0x000c, 0x000c);  //colgen start
write_cmos_sensor(0x0804, 0x0042);  //fmt x cropping
write_cmos_sensor(0x0026, 0x021a);  //y addr start active
write_cmos_sensor(0x002c, 0x0a93);  //y addr end active
write_cmos_sensor(0x005c, 0x0206);  //y dummy size
write_cmos_sensor(0x002e, 0x1111);  //y even/odd inc tobp
write_cmos_sensor(0x0032, 0x3333);  //y even/odd inc active
write_cmos_sensor(0x0006, 0x0348);  //frame length lines
write_cmos_sensor(0x0074, 0x0341);  //coarse integ time
write_cmos_sensor(0x0a0e, 0x0003);  //image mode/digial binning mode
write_cmos_sensor(0x0a12, 0x0500);  //x output size
write_cmos_sensor(0x0a14, 0x02d0);  //y output size
write_cmos_sensor(0x075c, 0x0100);  //OTP ctrl b5:lsc_flag b4:lsc_checksum b3:pdlsc_en b2:dga_en b1:lsc_en b0:adpc_en 
write_cmos_sensor(0x0050, 0x4720);  //analog control b7:vblank_analog_off b6:ag_cal b5:pd_rst2 b4:rdo_set_off b3:pat_addr_en b2:sub3 b1:sreg re-load b0:sreg write 
write_cmos_sensor(0x0722, 0x0700);  //d2a_pxl_drv_pwr/d2a_row_binning_en
write_cmos_sensor(0x004c, 0x0100);  //tg enable,hdr off
#endif

#if SET_STREAMING_TEST
		write_cmos_sensor(0x0a00, 0x0100); // stream on
#endif
}

//extern int back_camera_find_success;
//extern bool camera_back_probe_ok;//bit2
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
			//back_camera_find_success=3;
			//camera_back_probe_ok=1;
			LOG_INF("i2c write id : 0x%x, sensor id: 0x%x\n",
			imgsensor.i2c_write_id, *sensor_id);

			return ERROR_NONE;
			}

			retry--;
		} while (retry > 0);
		i++;
	retry = 1;
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
	kal_uint32 sensor_id = 0;

	LOG_INF("[open]: PLATFORM:MT6737,MIPI 24LANE\n");
	LOG_INF("preview 1296*972@30fps,360Mbps/lane;"
		"capture 2592*1944@30fps,880Mbps/lane\n");
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
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
	LOG_INF("E");
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
	capture_setting(imgsensor.current_fps);
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
	sensor_info->PDAF_Support = 2;
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
		LOG_INF("[odin]preview\n");
		preview(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		LOG_INF("[odin]capture\n");
	case MSDK_SCENARIO_ID_CAMERA_ZSD:
		capture(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		LOG_INF("[odin]video preview\n");
		normal_video(image_window, sensor_config_data);
		//capture(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		hs_video(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
	    slim_video(image_window, sensor_config_data);
		break;
	default:
		LOG_INF("[odin]default mode\n");
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
	UINT16 enable_TP = 0; 
	enable_TP = ((read_cmos_sensor(0x0A04) << 8) | read_cmos_sensor(0x0A05));

	LOG_INF("enable: %d", enable);
    
	if (enable) {
        write_cmos_sensor(0x0938,0x0000);
        write_cmos_sensor(0x0942,0x0008);
        write_cmos_sensor(0x0A04,0x0141);
		write_cmos_sensor(0x020A,0x0200);

	} else {
        write_cmos_sensor(0x0938,0x4100);
        write_cmos_sensor(0x0942,0x0108);
        write_cmos_sensor(0x0A04,0x03C1);
		write_cmos_sensor(0x020A,0x0000);
	}	 
	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	pr_err("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable)
		write_cmos_sensor(0x0a00, 0x0100); // stream on
	else
		write_cmos_sensor(0x0a00, 0x0000); // stream off

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
	case SENSOR_FEATURE_GET_PIXEL_RATE:
		switch (*feature_data) 
		{
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
			break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.hs_video.pclk /
			(imgsensor_info.hs_video.linelength - 80))*
			imgsensor_info.hs_video.grabwindow_width;
			break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.slim_video.pclk /
			(imgsensor_info.slim_video.linelength - 80))*
			imgsensor_info.slim_video.grabwindow_width;
			break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.pre.pclk /
			(imgsensor_info.pre.linelength - 80))*
			imgsensor_info.pre.grabwindow_width;
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
	    write_cmos_sensor(sensor_reg_data->RegAddr,						sensor_reg_data->RegData);
	break;
	case SENSOR_FEATURE_GET_REGISTER:
	    sensor_reg_data->RegData =				read_cmos_sensor(sensor_reg_data->RegAddr);
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
	    set_auto_flicker_mode((BOOL)*feature_data_16,			*(feature_data_16+1));
	break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
	    set_max_framerate_by_scenario(			(enum MSDK_SCENARIO_ID_ENUM)*feature_data,			*(feature_data+1));
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
	    LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",				(UINT32)*feature_data);

	    wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)			(uintptr_t)(*(feature_data+1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo,				(void *)&imgsensor_winsize_info[1],				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo,				(void *)&imgsensor_winsize_info[2],				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo,				(void *)&imgsensor_winsize_info[3],				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo,				(void *)&imgsensor_winsize_info[4],				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,				(void *)&imgsensor_winsize_info[0],				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
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
		LOG_INF("odin GET_PDAF_DATA EEPROM\n");
		// read from e2prom
#ifdef e2prom
		read_eeprom((kal_uint16)(*feature_data), 
				(char *)(uintptr_t)(*(feature_data+1)), 
				(kal_uint32)(*(feature_data+2)) );
#else
		// read from file

	        LOG_INF("READ PDCAL DATA\n");
		read_hi1333_eeprom((kal_uint16)(*feature_data), 
				(char *)(uintptr_t)(*(feature_data+1)), 
				(kal_uint32)(*(feature_data+2)) );

#endif
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
           	switch (*feature_data) 
	   	 	 {
           	     case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
           	     case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
           	          *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
	   	         break;
           	     case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
           	     //case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
           	     case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
           	     case MSDK_SCENARIO_ID_SLIM_VIDEO:
           	          *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
	   	          break;
           	     default:
           	          *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
           	         break;
           	 }
	   	 break;
/*
	case SENSOR_FEATURE_SET_PDAF:
			 	imgsensor.pdaf_mode = *feature_data_16;
	        	LOG_INF("[odin] pdaf mode : %d \n", imgsensor.pdaf_mode);
				break;
*/	
#endif
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

UINT32 HI1333_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc =  &sensor_func;
	return ERROR_NONE;
}	/*	HI1333_MIPI_RAW_SensorInit	*/
