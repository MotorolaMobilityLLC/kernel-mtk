/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "blackjack_sea_mt9d015mipiraw_Sensor.h"

/**************** Modify Following Strings for Debug ******************/
#define PFX "blackjack_sea_mt9d015_camera_sensor"
#define LOG_1 cam_pr_debug("blackjack_sea_mt9d015, MIPI 1LANE\n")
/********************   Modify end    *********************************/

#define LOG_INF(format, args...) \
	pr_debug(PFX "[%s] " format, __func__, ##args)
#define cam_pr_debug(format, args...) \
	pr_debug(PFX "[%s] " format, __func__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = BLACKJACK_SEA_MT9D015_SENSOR_ID,
	.checksum_value = 0xa3658b61,
	.pre = {
		.pclk = 95000000,
		.linelength = 2352,
		.framelength = 1347,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 62400000,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 95000000,
		.linelength = 2352,
		.framelength = 1347,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 62400000,
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 95000000,
		.linelength = 2352,
		.framelength = 1347,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 62400000,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 95000000,
		.linelength = 2352,
		.framelength = 1347,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 62400000,
		.max_framerate = 300,
	},
	.slim_video = {
		.pclk = 95000000,
		.linelength = 2352,
		.framelength = 1347,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1600,
		.grabwindow_height = 1200,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 62400000,
		.max_framerate = 300,
	},
	.margin = 8,
	.min_shutter = 5,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 5,

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_1_LANE,
	.i2c_addr_table = {0x20, 0x6c, 0xff},
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,
	.gain = 0x100,
	.dummy_pixel = 0,
	.dummy_line = 0,
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = 0,
	.i2c_write_id = 0x2e,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000,
		1600, 1200, 0, 0, 1600, 1200}, /* Preview */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000,
		1600, 1200, 0, 0, 1600, 1200}, /* capture */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000,
		1600, 1200, 0, 0, 1600, 1200}, /* video */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000,
		1600, 1200, 0, 0, 1600, 1200}, /* HS video */
	{1600, 1200, 0, 0, 1600, 1200, 1600, 1200, 0000, 0000,
		1600, 1200, 0, 0, 1600, 1200}  /* slim video */
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *) &get_byte, 2,
		imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[4] ={(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
}

static kal_uint16 read_cmos_sensor_8(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
    return get_byte;
}

static void write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para)
{
    char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
    iWriteRegI2C(puSendCmd , 3, imgsensor.i2c_write_id);
}

#if 0
static void set_dummy()
{
    kal_uint16 Line_length, Frame_length;

    Line_length   = 2352 + imgsensor.dummy_pixel;
    Frame_length = 1347  + imgsensor.dummy_line;
    
    cam_pr_debug("[A2030MIPI_SetDummy]Frame_length = %d, Line_length = %d", Frame_length, Line_length);

    write_cmos_sensor_8(0x0104, 0x01); // GROUPED_PARAMETER_HOLD
    write_cmos_sensor(0x0340, Frame_length);
    write_cmos_sensor(0x0342, Line_length);
    write_cmos_sensor_8(0x0104, 0x00); //GROUPED_PARAMETER_HOLD
  
}				/*    set_dummy  */
#else
static void set_dummy()
{

	 cam_pr_debug("[A2030MIPI_SetDummy]Frame_length = %d, Line_length = %d", imgsensor.frame_length, imgsensor.line_length);
#if 1
    write_cmos_sensor_8(0x0104, 0x01); // GROUPED_PARAMETER_HOLD
    write_cmos_sensor(0x0340, imgsensor.frame_length);
    write_cmos_sensor(0x0342, imgsensor.line_length);
    write_cmos_sensor_8(0x0104, 0x00); //GROUPED_PARAMETER_HOLD
#endif
}    
#endif


static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x00001));
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	cam_pr_debug("framerate = %d, min framelength should enable = %d\n",
		framerate,
		min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length =
		(frame_length > imgsensor.min_frame_length)
		? frame_length
		: imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length -
		imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length -
			imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	imgsensor.dummy_pixel = 0;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}				/*    set_max_framerate  */

/*************************************************************************
 * FUNCTION
 *    set_shutter
 *
 * DESCRIPTION
 *    This function set e-shutter of sensor to change exposure time.
 *
 * PARAMETERS
 *    iShutter : exposured lines
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	
	cam_pr_debug("enter! shutter =%d, framelength =%d\n",
            shutter, imgsensor.frame_length);

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter)
		? imgsensor_info.min_shutter
		: shutter;
	shutter =
		(shutter >
		(imgsensor_info.max_frame_length -
		imgsensor_info.margin))
		? (imgsensor_info.max_frame_length -
		imgsensor_info.margin)
		: shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 /
			imgsensor.frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else
			set_dummy();
	} else {
		set_dummy();
	}
	//shutter = 1347;
	/* Update Shutter */
    write_cmos_sensor_8(0x0104, 0x01);    // GROUPED_PARAMETER_HOLD
    write_cmos_sensor(0x0202, shutter);  /* course_integration_time */
    write_cmos_sensor_8(0x0104, 0x00);    // GROUPED_PARAMETER_HOLD
	cam_pr_debug("Exit! shutter =%d, framelength =%d\n",
		shutter, imgsensor.frame_length);

}	
			/*    set_shutter */
#if 1
#if 1
#define BLACKJACK_SEA_MT9D015MIPI_MaxGainIndex (117)
kal_uint16 BLACKJACK_SEA_MT9D015MIPI_sensorGainMapping[BLACKJACK_SEA_MT9D015MIPI_MaxGainIndex][2] ={
#if 0
//{72,  0x1024},
//{80,  0x1028},
  {88,  0x102C},
  {96,  0x1030},
  {104,	0x1034},
  {112,	0x1038},
  {120,	0x103C},
  {128,	0x10A0},
  {136,	0x10A2},
  {144,	0x10A4},
  {152,	0x10A6},
  {160,	0x10A8},
  {168,	0x10AA},
  {176,	0x10AC},
  {184,	0x10AE},
  {192,	0x10B0},
  {200,	0x10B2},
  {208,	0x10B4},
  {216,	0x10B6},
  {224,	0x10B8},
  {232,	0x10BA},
  {240,	0x10BC},
  {248,	0x10BE},
  {256,	0x10C0},
  {264,	0x10C2},
  {272,	0x10C4},
  {280,	0x10C6},
  {288,	0x10C8},
  {296,	0x10CA},
  {304,	0x10CC},
  {312,	0x10CE},
  {320,	0x10D0},
  {328,	0x10D2},
  {336,	0x10D4},
  {344,	0x10D6},
  {352,	0x10D8},
  {360,	0x10DA},
  {368,	0x10DC},
  {376,	0x10DE},
  {384,	0x10E0},
  {392,	0x10E2},
  {400,	0x10E4},
  {408,	0x10E6},
  {416,	0x10E8},
  {424,	0x10EA},
  {432,	0x10EC},
  {440,	0x10EE},
  {448,	0x10F0},
  {456,	0x10F2},
  {464,	0x10F4},
  {472,	0x10F6},
  {480,	0x10F8},
  {488,	0x10FA},
  {496,	0x10FC},
  {504,	0x10FE},
  {512,	0x11C0},
  {520,	0x11C1},
  {528,	0x11C2},
  {536,	0x11C3},
  {544,	0x11C4},
  {552,	0x11C5},
  {560,	0x11C6},
  {568,	0x11C7},
  {576,	0x11C8},
  {584,	0x11C9},
  {592,	0x11CA},
  {600,	0x11CB},
  {608,	0x11CC},
  {616,	0x11CD},
  {624,	0x11CE},
  {632,	0x11CF},
  {640,	0x11D0},
  {648,	0x11D1},
  {656,	0x11D2},
  {664,	0x11D3},
  {672,	0x11D4},
  {680,	0x11D5},
  {688,	0x11D6},
  {696,	0x11D7},
  {704,	0x11D8},
  {712,	0x11D9},
  {720,	0x11DA},
  {728,	0x11DB},
  {736,	0x11DC},
  {744,	0x11DD},
  {752,	0x11DE},
  {760,	0x11DF},
  {768,	0x11E0},
  {776,	0x11E1},
  {784,	0x11E2},
  {792,	0x11E3},
  {800,	0x11E4},
  {808,	0x11E5},
  {816,	0x11E6},
  {824,	0x11E7},
  {832,	0x11E8},
  {840,	0x11E9},
  {848,	0x11EA},
  {856,	0x11EB},
  {864,	0x11EC},
  {872,	0x11ED},
  {880,	0x11EE},
  {888,	0x11EF},
  {896,	0x11F0},
  {904,	0x11F1},
  {912,	0x11F2},
  {920,	0x11F3},
  {928,	0x11F4},
  {936,	0x11F5},
  {944,	0x11F6},
  {952,	0x11F7},
  {960,	0x11F8},
  {968,	0x11F9},
  {976,	0x11FA},
  {984,	0x11FB},
  {992,	0x11FC},
  {1000,	0x11FD},
  {1008,	0x11FE},
  {1016,	0x11FF},

#else
//{72,  0x0009},
//{80,  0x000A},
  {88,  0x000B},
  {96,  0x000C},
  {104,	0x000D},
  {112,	0x000E},
  {120,	0x000F},
  {128,	0x0010},
  {136,	0x0011},
  {144,	0x0012},
  {152,	0x0013},
  {160,	0x0014},
  {168,	0x0015},
  {176,	0x0016},
  {184,	0x0017},
  {192,	0x0018},
  {200,	0x0019},
  {208,	0x001A},
  {216,	0x001B},
  {224,	0x001C},
  {232,	0x001D},
  {240,	0x001E},
  {248,	0x001F},
  {256,	0x0020},
  {264,	0x0021},
  {272,	0x0022},
  {280,	0x0023},
  {288,	0x0024},
  {296,	0x0025},
  {304,	0x0026},
  {312,	0x0027},
  {320,	0x0028},
  {328,	0x0029},
  {336,	0x002A},
  {344,	0x002B},
  {352,	0x002C},
  {360,	0x002D},
  {368,	0x002E},
  {376,	0x002F},
  {384,	0x0030},
  {392,	0x0031},
  {400,	0x0032},
  {408,	0x0033},
  {416,	0x0034},
  {424,	0x0035},
  {432,	0x0036},
  {440,	0x0037},
  {448,	0x0038},
  {456,	0x0039},
  {464,	0x003A},
  {472,	0x003B},
  {480,	0x003C},
  {488,	0x003D},
  {496,	0x003E},
  {504,	0x003F},
  {512,	0x0040},
  {520,	0x0041},
  {528,	0x0042},
  {536,	0x0043},
  {544,	0x0044},
  {552,	0x0045},
  {560,	0x0046},
  {568,	0x0047},
  {576,	0x0048},
  {584,	0x0049},
  {592,	0x004A},
  {600,	0x004B},
  {608,	0x004C},
  {616,	0x004D},
  {624,	0x004E},
  {632,	0x004F},
  {640,	0x0050},
  {648,	0x0051},
  {656,	0x0052},
  {664,	0x0053},
  {672,	0x0054},
  {680,	0x0055},
  {688,	0x0056},
  {696,	0x0057},
  {704,	0x0058},
  {712,	0x0059},
  {720,	0x005A},
  {728,	0x005B},
  {736,	0x005C},
  {744,	0x005D},
  {752,	0x005E},
  {760,	0x005F},
  {768,	0x0060},
  {776,	0x0061},
  {784,	0x0062},
  {792,	0x0063},
  {800,	0x0064},
  {808,	0x0065},
  {816,	0x0066},
  {824,	0x0067},
  {832,	0x0068},
  {840,	0x0069},
  {848,	0x006A},
  {856,	0x006B},
  {864,	0x006C},
  {872,	0x006D},
  {880,	0x006E},
  {888,	0x006F},
  {896,	0x0070},
  {904,	0x0071},
  {912,	0x0072},
  {920,	0x0073},
  {928,	0x0074},
  {936,	0x0075},
  {944,	0x0076},
  {952,	0x0077},
  {960,	0x0078},
  {968,	0x0079},
  {976,	0x007A},
  {984,	0x007B},
  {992,	0x007C},
  {1000,0x007D},
  {1008,0x007E},
  {1016,0x007F},
#endif
};


static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint8 iI;
    cam_pr_debug("[BLACKJACK_SEA_MT9D015MIPI]enter BLACKJACK_SEA_MT9D015MIPIGain2Reg function\n");
    for (iI = 0; iI < BLACKJACK_SEA_MT9D015MIPI_MaxGainIndex; iI++)
	{
		if(gain <= BLACKJACK_SEA_MT9D015MIPI_sensorGainMapping[iI][0])
		{
			return BLACKJACK_SEA_MT9D015MIPI_sensorGainMapping[iI][1];
		}


    }
	if(iI != BLACKJACK_SEA_MT9D015MIPI_MaxGainIndex)
	{
    	if(gain != BLACKJACK_SEA_MT9D015MIPI_sensorGainMapping[iI][0])
    	{
        	 cam_pr_debug("Gain mapping don't correctly:%d %d \n", gain, BLACKJACK_SEA_MT9D015MIPI_sensorGainMapping[iI][0]);
    	}
    }
	cam_pr_debug("exit BLACKJACK_SEA_MT9D015MIPIGain2Reg function\n");
    return BLACKJACK_SEA_MT9D015MIPI_sensorGainMapping[iI-1][1];
}
static kal_uint16 set_gain(kal_uint16 gain)
{
    kal_uint16 reg_gain;
	if (gain < BASEGAIN || gain > 8 * BASEGAIN) {
		pr_debug("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 8 * BASEGAIN)
			gain = 8 * BASEGAIN;
	}

	//reg_gain = gain2reg(gain);
	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
    cam_pr_debug("reg_gain =%d, gain = %d", reg_gain, gain);
	
    write_cmos_sensor_8(0x0104, 0x01);        //parameter_hold
    //write_cmos_sensor(0x305E,reg_gain);  
   	//write_cmos_sensor(0x0204,reg_gain);  
    write_cmos_sensor_8(0x0205,reg_gain);  
    write_cmos_sensor_8(0x0104, 0x00);        //parameter_hold
 
	return gain;
}
#else
static kal_uint8 gain2reg(const kal_uint16 iGain)
{
	//kal_uint8 BASEGAIN = 64;
    kal_uint16 ireg = 0;
	
	if(iGain >= BASEGAIN && iGain <= 32*BASEGAIN)
		ireg = 8 * iGain/BASEGAIN;

	return ireg;
}
static kal_uint16 set_gain(kal_uint16 iGain)
{
	kal_uint16 reg_gain;
  reg_gain = gain2reg(iGain);
	LOG_INF("iGain = %d\n",reg_gain);
	write_cmos_sensor_8(0x0104, 0x01);		//parameter_hold

	reg_gain = 8 * iGain/BASEGAIN;        //change mtk gain base to aptina gain base
	if(reg_gain < 0x0B)
		reg_gain = 0x0B;
	if(reg_gain < 0x7F)
		write_cmos_sensor_8(0x0205,reg_gain);
	else
	    LOG_INF("error gain setting");
	    //write_cmos_sensor(0x0204, reg_gain);

	write_cmos_sensor_8(0x0104, 0x00);		//parameter_hold

	return iGain;
}
#endif
#else
static kal_uint16 gain2reg(const kal_uint16 gain)
{
    kal_uint16 reg_gain;
    kal_uint16 collumn_gain = 0, asc1_gain = 0, initial_gain = 0;
    kal_uint16 collumn_gain_shift = 10, asc1_gain_shift = 8;

    if (gain < (4 * BASEGAIN) / 3) {
        collumn_gain = (0x00&0x03) << collumn_gain_shift;
        asc1_gain = (0x00&0x03) << asc1_gain_shift;
        initial_gain = (32 * gain / BASEGAIN) & 0x7F;
    } else if (gain < 2 * BASEGAIN) {
        collumn_gain = (0x00&0x03) << collumn_gain_shift;
        asc1_gain = (0x01&0x03) << asc1_gain_shift;
        initial_gain = (32 * gain * 3 / (BASEGAIN * 4)) & 0x7F;
    } else if (gain < (8 * BASEGAIN)/3 + 1) {
        collumn_gain = (0x02&0x03) << collumn_gain_shift;
        asc1_gain = (0x00&0x03) << asc1_gain_shift;
        initial_gain = (32 * gain / (BASEGAIN * 2)) & 0x7F;
    } else if (gain < 3 * BASEGAIN) {
        collumn_gain = (0x02&0x03) << collumn_gain_shift;
        asc1_gain = (0x01&0x03) << asc1_gain_shift;
        initial_gain = (32 * gain * 3 / (BASEGAIN * 2 * 4)) & 0x7F;
    } else if (gain < 4 * BASEGAIN) {
        collumn_gain = (0x01&0x03) << collumn_gain_shift;
        asc1_gain = (0x00&0x03) << asc1_gain_shift;
        initial_gain = (32 * gain / (BASEGAIN * 3)) & 0x7F;
    } else if (gain < (16 * BASEGAIN) / 3 + 1) {
        collumn_gain = (0x03&0x03) << collumn_gain_shift;
        asc1_gain = (0x00&0x03) << asc1_gain_shift;
        initial_gain = (32 * gain / (BASEGAIN * 4)) & 0x7F;
    } else if (gain < 8 * BASEGAIN) {
        collumn_gain = (0x03&0x03) << collumn_gain_shift;
        asc1_gain = (0x01&0x03) << asc1_gain_shift;
        initial_gain = (32 * gain * 3 / (BASEGAIN * 4 * 4)) & 0x7F;
    } else if (gain < 32 * BASEGAIN) {
        collumn_gain = (0x03&0x03) << collumn_gain_shift;
        asc1_gain = (0x02&0x03) << asc1_gain_shift;
        initial_gain = (32 * gain / (BASEGAIN * 4 * 2)) & 0x7F;
    } else {
        // not exist
        cam_pr_debug("error gain setting");
    }

    reg_gain = collumn_gain | asc1_gain | initial_gain;

    return reg_gain;
}

static kal_uint16 set_gain(kal_uint16 gain)
{
    kal_uint16 reg_gain;
	if (gain < BASEGAIN || gain > 15 * BASEGAIN) {
		pr_debug("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 15 * BASEGAIN)
			gain = 15 * BASEGAIN;
	}

	//reg_gain = gain2reg(gain);
	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
    cam_pr_debug("reg_gain =%d, gain = %d", reg_gain, gain);
	
    write_cmos_sensor_8(0x0104, 0x01);        //parameter_hold
    write_cmos_sensor(0x0204,reg_gain);  
    write_cmos_sensor_8(0x0104, 0x00);        //parameter_hold
 
	return gain;
}

#endif
/*************************************************************************
 * FUNCTION
 *    set_gain
 *
 * DESCRIPTION
 *    This function is to set global gain to sensor.
 *
 * PARAMETERS
 *    iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *    the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/

static void ihdr_write_shutter_gain(
	kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	cam_pr_debug("le:0x%x, se:0x%x, gain:0x%x\n", le, se, gain);
}


#if 0				/* not used function */
static void set_mirror_flip(kal_uint8 image_mirror)
{
	cam_pr_debug("image_mirror = %d\n", image_mirror);

    switch (image_mirror)
    {
        case IMAGE_NORMAL:
            write_cmos_sensor_8(0x0101,0x00);
        break;
        case IMAGE_H_MIRROR:
            write_cmos_sensor_8(0x0101,0x01);
        break;
        case IMAGE_V_MIRROR:
            write_cmos_sensor_8(0x0101,0x02);
        break;
        case IMAGE_HV_MIRROR:
            write_cmos_sensor_8(0x0101,0x03);
        break;
    }
#endif

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
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void night_mode(kal_bool enable)
{
	/*No Need to implement this function */
}			/*    night_mode    */

static void sensor_init(void)
{
	cam_pr_debug("E");

	write_cmos_sensor_8(0x0103, 0x01);    //SOFTWARE_RESET (clears itself)
    mdelay(50);      //Initialization Time
    
    //stop_streaming
	write_cmos_sensor_8(0x0100,0x00  ); // MODE_SELECT				 
	write_cmos_sensor(0x301A,0x0110); // RESET_REGISTER			 
	write_cmos_sensor(0x3064,0x0805); // SMIA_TEST				 
	write_cmos_sensor(0x31BE,0xC007); // MIPI_CONFIG_STATUS		 
	write_cmos_sensor(0x3E00,0x0430); // DYNAMIC_SEQRAM_00		 
	write_cmos_sensor(0x3E02,0x3FFF); // DYNAMIC_SEQRAM_02		 
	write_cmos_sensor(0x3E1E,0x67CA); // DYNAMIC_SEQRAM_1E		 
	write_cmos_sensor(0x3E2A,0xCA67); // DYNAMIC_SEQRAM_2A		 
	write_cmos_sensor(0x3E2E,0x8054); // DYNAMIC_SEQRAM_2E		 
	write_cmos_sensor(0x3E30,0x8255); // DYNAMIC_SEQRAM_30		 
	write_cmos_sensor(0x3E32,0x8410); // DYNAMIC_SEQRAM_32		 
	write_cmos_sensor(0x3E36,0x5FB0); // DYNAMIC_SEQRAM_36		 
	write_cmos_sensor(0x3E38,0x4C82); // DYNAMIC_SEQRAM_38		 
	write_cmos_sensor(0x3E3A,0x4DB0); // DYNAMIC_SEQRAM_3A		 
	write_cmos_sensor(0x3E3C,0x5F82); // DYNAMIC_SEQRAM_3C		 
	write_cmos_sensor(0x3E3E,0x1170); // DYNAMIC_SEQRAM_3E		 
	write_cmos_sensor(0x3E40,0x8055); // DYNAMIC_SEQRAM_40		 
	write_cmos_sensor(0x3E42,0x8061); // DYNAMIC_SEQRAM_42		 
	write_cmos_sensor(0x3E44,0x68D8); // DYNAMIC_SEQRAM_44		 
	write_cmos_sensor(0x3E46,0x6882); // DYNAMIC_SEQRAM_46		 
	write_cmos_sensor(0x3E48,0x6182); // DYNAMIC_SEQRAM_48		 
	write_cmos_sensor(0x3E4A,0x4D82); // DYNAMIC_SEQRAM_4A		 
	write_cmos_sensor(0x3E4C,0x4C82); // DYNAMIC_SEQRAM_4C		 
	write_cmos_sensor(0x3E4E,0x6368); // DYNAMIC_SEQRAM_4E		 
	write_cmos_sensor(0x3E50,0xD868); // DYNAMIC_SEQRAM_50		 
	write_cmos_sensor(0x3E52,0x8263); // DYNAMIC_SEQRAM_52		 
	write_cmos_sensor(0x3E54,0x824D); // DYNAMIC_SEQRAM_54		 
	write_cmos_sensor(0x3E56,0x8203); // DYNAMIC_SEQRAM_56		 
	write_cmos_sensor(0x3E58,0x9D66); // DYNAMIC_SEQRAM_58		 
	write_cmos_sensor(0x3E5A,0x8045); // DYNAMIC_SEQRAM_5A		 
	write_cmos_sensor(0x3E5C,0x4E7C); // DYNAMIC_SEQRAM_5C		 
	write_cmos_sensor(0x3E5E,0x0970); // DYNAMIC_SEQRAM_5E		 
	write_cmos_sensor(0x3E60,0x8072); // DYNAMIC_SEQRAM_60		 
	write_cmos_sensor(0x3E62,0x5484); // DYNAMIC_SEQRAM_62		 
	write_cmos_sensor(0x3E64,0x2037); // DYNAMIC_SEQRAM_64		 
	write_cmos_sensor(0x3E66,0x8216); // DYNAMIC_SEQRAM_66		 
	write_cmos_sensor(0x3E68,0x0486); // DYNAMIC_SEQRAM_68		 
	write_cmos_sensor(0x3E6A,0x1070); // DYNAMIC_SEQRAM_6A		 
	write_cmos_sensor(0x3E6C,0x825E); // DYNAMIC_SEQRAM_6C		 
	write_cmos_sensor(0x3E6E,0xEE54); // DYNAMIC_SEQRAM_6E		 
	write_cmos_sensor(0x3E70,0x825E); // DYNAMIC_SEQRAM_70		 
	write_cmos_sensor(0x3E72,0x8212); // DYNAMIC_SEQRAM_72		 
	write_cmos_sensor(0x3E74,0x7086); // DYNAMIC_SEQRAM_74		 
	write_cmos_sensor(0x3E76,0x1404); // DYNAMIC_SEQRAM_76		 
	write_cmos_sensor(0x3E78,0x8220); // DYNAMIC_SEQRAM_78		 
	write_cmos_sensor(0x3E7A,0x377C); // DYNAMIC_SEQRAM_7A		 
	write_cmos_sensor(0x3E7C,0x6170); // DYNAMIC_SEQRAM_7C		 
	write_cmos_sensor(0x3E7E,0x8082); // DYNAMIC_SEQRAM_7E		 
	write_cmos_sensor(0x3E80,0x4F82); // DYNAMIC_SEQRAM_80		 
	write_cmos_sensor(0x3E82,0x4E82); // DYNAMIC_SEQRAM_82		 
	write_cmos_sensor(0x3E84,0x5FCA); // DYNAMIC_SEQRAM_84		 
	write_cmos_sensor(0x3E86,0x5F82); // DYNAMIC_SEQRAM_86		 
	write_cmos_sensor(0x3E88,0x4E82); // DYNAMIC_SEQRAM_88		 
	write_cmos_sensor(0x3E8A,0x4F81); // DYNAMIC_SEQRAM_8A		 
	write_cmos_sensor(0x3E8C,0x7C7F); // DYNAMIC_SEQRAM_8C		 
	write_cmos_sensor(0x3E8E,0x7000); // DYNAMIC_SEQRAM_8E		 
	write_cmos_sensor(0x30D4,0xE200); // COLUMN_CORRECTION		 
	write_cmos_sensor(0x3174,0x8000); // ANALOG_CONTROL3			 
	write_cmos_sensor(0x3EE0,0x0020); // DAC_LD_20_21 			 
	write_cmos_sensor(0x3EE2,0x0016); // DAC_LD_22_23 			 
	write_cmos_sensor(0x3F00,0x0002); // BM_T0					 
	write_cmos_sensor(0x3F02,0x0028); // BM_T1					 
	write_cmos_sensor(0x3F0A,0x0300); // NOISE_FLOOR10			 
	write_cmos_sensor(0x3F0C,0x1008); // NOISE_FLOOR32			 
	write_cmos_sensor(0x3F10,0x0405); // SINGLE_K_FACTOR0 		 
	write_cmos_sensor(0x3F12,0x0101); // SINGLE_K_FACTOR1 		 
	write_cmos_sensor(0x3F14,0x0000); // SINGLE_K_FACTOR2 		 
	write_cmos_sensor(0x1140,0x0093); // MIN_FRAME_LENGTH_LINES	 
	write_cmos_sensor(0x114A,0x0093); // MIN_FRAME_BLANKING_LINES  
	write_cmos_sensor(0x3600,0x0130); // P_GR_P0Q0				 
	write_cmos_sensor(0x3602,0x618B); // P_GR_P0Q1				 
	write_cmos_sensor(0x3604,0x12B2); // P_GR_P0Q2				 
	write_cmos_sensor(0x3606,0x810F); // P_GR_P0Q3				 
	write_cmos_sensor(0x3608,0xC152); // P_GR_P0Q4				 
	write_cmos_sensor(0x360A,0x00D0); // P_RD_P0Q0				 
	write_cmos_sensor(0x360C,0x1D6D); // P_RD_P0Q1				 
	write_cmos_sensor(0x360E,0x5FB2); // P_RD_P0Q2				 
	write_cmos_sensor(0x3610,0xEBD0); // P_RD_P0Q3				 
	write_cmos_sensor(0x3612,0xB3B3); // P_RD_P0Q4				 
	write_cmos_sensor(0x3614,0x0190); // P_BL_P0Q0				 
	write_cmos_sensor(0x3616,0xD98C); // P_BL_P0Q1				 
	write_cmos_sensor(0x3618,0x0891); // P_BL_P0Q2				 
	write_cmos_sensor(0x361A,0xBC8C); // P_BL_P0Q3				 
	write_cmos_sensor(0x361C,0x9151); // P_BL_P0Q4				 
	write_cmos_sensor(0x361E,0x02D0); // P_GB_P0Q0				 
	write_cmos_sensor(0x3620,0x30CC); // P_GB_P0Q1				 
	write_cmos_sensor(0x3622,0x3B72); // P_GB_P0Q2				 
	write_cmos_sensor(0x3624,0xDBF0); // P_GB_P0Q3				 
	write_cmos_sensor(0x3626,0xA7D3); // P_GB_P0Q4				 
	write_cmos_sensor(0x3640,0xF048); // P_GR_P1Q0				 
	write_cmos_sensor(0x3642,0x024D); // P_GR_P1Q1				 
	write_cmos_sensor(0x3644,0xCCCE); // P_GR_P1Q2				 
	write_cmos_sensor(0x3646,0x908A); // P_GR_P1Q3				 
	write_cmos_sensor(0x3648,0x7A10); // P_GR_P1Q4				 
	write_cmos_sensor(0x364A,0x444A); // P_RD_P1Q0				 
	write_cmos_sensor(0x364C,0x0FCE); // P_RD_P1Q1				 
	write_cmos_sensor(0x364E,0x0B49); // P_RD_P1Q2				 
	write_cmos_sensor(0x3650,0x2D4F); // P_RD_P1Q3				 
	write_cmos_sensor(0x3652,0x220F); // P_RD_P1Q4				 
	write_cmos_sensor(0x3654,0x224C); // P_BL_P1Q0				 
	write_cmos_sensor(0x3656,0x4A4D); // P_BL_P1Q1				 
	write_cmos_sensor(0x3658,0x92EE); // P_BL_P1Q2				 
	write_cmos_sensor(0x365A,0x20AF); // P_BL_P1Q3				 
	write_cmos_sensor(0x365C,0x3F10); // P_BL_P1Q4				 
	write_cmos_sensor(0x365E,0x41AC); // P_GB_P1Q0				 
	write_cmos_sensor(0x3660,0x30AD); // P_GB_P1Q1				 
	write_cmos_sensor(0x3662,0x54AE); // P_GB_P1Q2				 
	write_cmos_sensor(0x3664,0xE0AD); // P_GB_P1Q3				 
	write_cmos_sensor(0x3666,0xE030); // P_GB_P1Q4				 
	write_cmos_sensor(0x3680,0x20B2); // P_GR_P2Q0				 
	write_cmos_sensor(0x3682,0xB6AF); // P_GR_P2Q1				 
	write_cmos_sensor(0x3684,0x6EF2); // P_GR_P2Q2				 
	write_cmos_sensor(0x3686,0xFAF3); // P_GR_P2Q3				 
	write_cmos_sensor(0x3688,0x9AF7); // P_GR_P2Q4				 
	write_cmos_sensor(0x368A,0x5632); // P_RD_P2Q0				 
	write_cmos_sensor(0x368C,0xF50F); // P_RD_P2Q1				 
	write_cmos_sensor(0x368E,0x51D3); // P_RD_P2Q2				 
	write_cmos_sensor(0x3690,0x8874); // P_RD_P2Q3				 
	write_cmos_sensor(0x3692,0xDF97); // P_RD_P2Q4				 
	write_cmos_sensor(0x3694,0x3931); // P_BL_P2Q0				 
	write_cmos_sensor(0x3696,0x83CF); // P_BL_P2Q1				 
	write_cmos_sensor(0x3698,0x3292); // P_BL_P2Q2				 
	write_cmos_sensor(0x369A,0xC452); // P_BL_P2Q3				 
	write_cmos_sensor(0x369C,0xB4D6); // P_BL_P2Q4				 
	write_cmos_sensor(0x369E,0x1F92); // P_GB_P2Q0				 
	write_cmos_sensor(0x36A0,0xA350); // P_GB_P2Q1				 
	write_cmos_sensor(0x36A2,0x3FF1); // P_GB_P2Q2				 
	write_cmos_sensor(0x36A4,0xD472); // P_GB_P2Q3				 
	write_cmos_sensor(0x36A6,0x8B17); // P_GB_P2Q4				 
	write_cmos_sensor(0x36C0,0x11CF); // P_GR_P3Q0				 
	write_cmos_sensor(0x36C2,0x1090); // P_GR_P3Q1				 
	write_cmos_sensor(0x36C4,0x0CF2); // P_GR_P3Q2				 
	write_cmos_sensor(0x36C6,0x8E51); // P_GR_P3Q3				 
	write_cmos_sensor(0x36C8,0x9873); // P_GR_P3Q4				 
	write_cmos_sensor(0x36CA,0x4050); // P_RD_P3Q0				 
	write_cmos_sensor(0x36CC,0x566F); // P_RD_P3Q1				 
	write_cmos_sensor(0x36CE,0x8752); // P_RD_P3Q2				 
	write_cmos_sensor(0x36D0,0xCC13); // P_RD_P3Q3				 
	write_cmos_sensor(0x36D2,0xAC4D); // P_RD_P3Q4				 
	write_cmos_sensor(0x36D4,0x9E0E); // P_BL_P3Q0				 
	write_cmos_sensor(0x36D6,0x1010); // P_BL_P3Q1				 
	write_cmos_sensor(0x36D8,0x5F12); // P_BL_P3Q2				 
	write_cmos_sensor(0x36DA,0xF673); // P_BL_P3Q3				 
	write_cmos_sensor(0x36DC,0xCA15); // P_BL_P3Q4				 
	write_cmos_sensor(0x36DE,0xF64C); // P_GB_P3Q0				 
	write_cmos_sensor(0x36E0,0x2110); // P_GB_P3Q1				 
	write_cmos_sensor(0x36E2,0x2231); // P_GB_P3Q2				 
	write_cmos_sensor(0x36E4,0xBD72); // P_GB_P3Q3				 
	write_cmos_sensor(0x36E6,0x8F35); // P_GB_P3Q4				 
	write_cmos_sensor(0x3700,0xC452); // P_GR_P4Q0				 
	write_cmos_sensor(0x3702,0xA913); // P_GR_P4Q1				 
	write_cmos_sensor(0x3704,0xE677); // P_GR_P4Q2				 
	write_cmos_sensor(0x3706,0x1A16); // P_GR_P4Q3				 
	write_cmos_sensor(0x3708,0x17B9); // P_GR_P4Q4				 
	write_cmos_sensor(0x370A,0xCA71); // P_RD_P4Q0				 
	write_cmos_sensor(0x370C,0xC093); // P_RD_P4Q1				 
	write_cmos_sensor(0x370E,0xA9B8); // P_RD_P4Q2				 
	write_cmos_sensor(0x3710,0x1AD6); // P_RD_P4Q3				 
	write_cmos_sensor(0x3712,0x3B39); // P_RD_P4Q4				 
	write_cmos_sensor(0x3714,0x8B72); // P_BL_P4Q0				 
	write_cmos_sensor(0x3716,0xF5F1); // P_BL_P4Q1				 
	write_cmos_sensor(0x3718,0x99D7); // P_BL_P4Q2				 
	write_cmos_sensor(0x371A,0x1B35); // P_BL_P4Q3				 
	write_cmos_sensor(0x371C,0x0F79); // P_BL_P4Q4				 
	write_cmos_sensor(0x371E,0xDD92); // P_GB_P4Q0				 
	write_cmos_sensor(0x3720,0xA952); // P_GB_P4Q1				 
	write_cmos_sensor(0x3722,0xCA17); // P_GB_P4Q2				 
	write_cmos_sensor(0x3724,0x2775); // P_GB_P4Q3				 
	write_cmos_sensor(0x3726,0x3DF8); // P_GB_P4Q4				 
	write_cmos_sensor(0x3782,0x0360); // POLY_ORIGIN_C			 
	write_cmos_sensor(0x3784,0x0258); // POLY_ORIGIN_R			 
	write_cmos_sensor(0x3780,0x0000); // POLY_SC_ENABLE			 
	write_cmos_sensor(0x0112,0x0A0A); // CCP_DATA_FORMAT			 
	write_cmos_sensor(0x0300,0x0008); // VT_PIX_CLK_DIV			 
	write_cmos_sensor(0x0302,0x0001); // VT_SYS_CLK_DIV			 
	write_cmos_sensor(0x0304,0x0003); // PRE_PLL_CLK_DIV			 
	write_cmos_sensor(0x0306,0x005F); // PLL_MULTIPLIER			 
	write_cmos_sensor(0x0308,0x000A); // OP_PIX_CLK_DIV			 
	write_cmos_sensor(0x030A,0x0001); // OP_SYS_CLK_DIV	
	write_cmos_sensor(0x31B4,0x0D66); // MIPI_TIMING_0			 
	write_cmos_sensor(0x31B6,0x0918); // MIPI_TIMING_1			 
	write_cmos_sensor(0x31B8,0x010C); // MIPI_TIMING_2			 
	write_cmos_sensor(0x31BA,0x050A); // MIPI_TIMING_3			 
	write_cmos_sensor(0x31BC,0x0A08); // MIPI_TIMING_4			 
	write_cmos_sensor_8(0x0104,0x01  );// GROUPED_PARAMETER_HOLD    
	write_cmos_sensor(0x0200,0x01E5); // FINE_INTEGRATION_TIME	 
	write_cmos_sensor(0x0202,0x0543); // COARSE_INTEGRATION_TIME	 
	write_cmos_sensor(0x3010,0x0094); // FINE_CORRECTION			 
	write_cmos_sensor(0x3040,0x0041); // READ_MODE				 
	write_cmos_sensor(0x0340,0x0543); // FRAME_LENGTH_LINES		 
	write_cmos_sensor(0x0342,0x0930); // LINE_LENGTH_PCK			 
	write_cmos_sensor(0x0344,0x0004); // X_ADDR_START 			 
	write_cmos_sensor(0x0346,0x0004); // Y_ADDR_START 			 
	write_cmos_sensor(0x0348,0x0643); // X_ADDR_END				 
	write_cmos_sensor(0x034A,0x04B3); // Y_ADDR_END				 
	write_cmos_sensor(0x034C,0x0640); // X_OUTPUT_SIZE			 
	write_cmos_sensor(0x034E,0x04B0); // Y_OUTPUT_SIZE			 
	write_cmos_sensor(0x0400,0x0000); // SCALING_MODE 			 
	write_cmos_sensor(0x0404,0x0010); // SCALE_M					 
	write_cmos_sensor(0x0204,0x000b); // GLOBAL_GAIN				 
	write_cmos_sensor(0x301A,0x0118); // RESET_REGISTER			 
	write_cmos_sensor_8(0x0104,0x00  ); // GROUPED_PARAMETER_HOLD	 
	write_cmos_sensor_8(0x0100,0x01  ); // MODE_SELECT
	write_cmos_sensor_8(0x0101,0x03  ); // MODE_SELECT	
	write_cmos_sensor_8(0x0105,0x01  ); // MODE_SELECT	
	mdelay(50);
}

static void preview_setting(void)
{
	cam_pr_debug("E\n");
	//set_dummy();
}

static void capture_setting(kal_uint16 currefps)
{
	cam_pr_debug("E\n");
	//set_dummy();
}

static void normal_video_setting(kal_uint16 currefps)
{
	cam_pr_debug("E\n");
	//set_dummy();
}

static void hs_video_setting(void)
{
	cam_pr_debug("E\n");
	//set_dummy();
}

static void slim_video_setting(void)
{
	cam_pr_debug("E\n");
	//set_dummy();
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	cam_pr_debug("enable: %d\n", enable);

	if (enable) {
		write_cmos_sensor(0x0600,0x02);
	} else {
		write_cmos_sensor(0x0600,0x00);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	//mdelay(50);
	return ERROR_NONE;
}

/*************************************************************************
 * FUNCTION
 *    get_imgsensor_id
 *
 * DESCRIPTION
 *    This function get the sensor ID
 *
 * PARAMETERS
 *    *sensorID : return the sensor ID
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
extern char backaux2_cam_name[64];
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
				memset(backaux2_cam_name, 0x00, sizeof(backaux2_cam_name));
				memcpy(backaux2_cam_name, "3_blackjack_sea_mt9d015", 64);
				ontim_get_otp_data(*sensor_id, NULL, 0);
				cam_pr_debug("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}
			cam_pr_debug("Read sensor id fail, write id: 0x%x, id: 0x%x\n",
				imgsensor.i2c_write_id, *sensor_id);
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
 *    open
 *
 * DESCRIPTION
 *    This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *    None
 *
 * RETURNS
 *    None
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
				cam_pr_debug("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
				break;
			}
			cam_pr_debug("Read sensor id fail, write id: 0x%x, id: 0x%x\n",
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
}

/*************************************************************************
 * FUNCTION
 *    close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *    None
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 close(void)
{
	cam_pr_debug("E\n");
	/*No Need to implement this function */
	return ERROR_NONE;
}			/*    close  */

/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *    This function start the sensor preview.
 *
 * PARAMETERS
 *    *image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 preview(
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT * image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	cam_pr_debug("E\n");

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
}			/*    preview   */

/*************************************************************************
 * FUNCTION
 *    capture
 *
 * DESCRIPTION
 *    This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *    None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT * image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	cam_pr_debug("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);
	return ERROR_NONE;
}			/* capture() */

static kal_uint32 normal_video(
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT * image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	cam_pr_debug("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	/* imgsensor.current_fps = 300; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	return ERROR_NONE;
}			/*    normal_video   */

static kal_uint32 hs_video(
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT * image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	cam_pr_debug("E\n");

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
}			/*    hs_video   */

static kal_uint32 slim_video(
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT * image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	cam_pr_debug("E\n");

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
}			/*    slim_video     */

static kal_uint32 get_resolution(
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT * sensor_resolution)
{
	cam_pr_debug("E\n");

	spin_lock(&imgsensor_drv_lock);
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
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}			/*    get_resolution    */

static kal_uint32 get_info(
	enum MSDK_SCENARIO_ID_ENUM scenario_id,
	MSDK_SENSOR_INFO_STRUCT *sensor_info,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	cam_pr_debug("scenario_id = %d\n", scenario_id);

	spin_lock(&imgsensor_drv_lock);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 1;	/* not use */
	sensor_info->SensorResetActiveHigh = FALSE;	/* not use */
	sensor_info->SensorResetDelayCount = 5;	/* not use */

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

	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;

	/* The frame of setting sensor gain */
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
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
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static kal_uint32 control(
	enum MSDK_SCENARIO_ID_ENUM scenario_id,
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	cam_pr_debug("scenario_id = %d\n", scenario_id);
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
		cam_pr_debug("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}			/* control() */

static kal_uint32 set_video_mode(UINT16 framerate)
{
	cam_pr_debug("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)
		/* Dynamic frame rate */
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 303;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 148;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	cam_pr_debug("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)	/* enable auto flicker */
		imgsensor.autoflicker_en = KAL_TRUE;
	else		/* Cancel Auto flick */
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id,
	MUINT32 framerate)
{
	kal_uint32 frame_length;

	cam_pr_debug("scenario_id = %d, framerate = %d\n",
		scenario_id,
		framerate);

	if (framerate == 0)
		return ERROR_NONE;
	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length =
			imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			imgsensor_info.pre.framelength)
			? (frame_length -
			imgsensor_info.pre.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength +
			imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		imgsensor.dummy_pixel = 1855;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		frame_length =
			imgsensor_info.normal_video.pclk / framerate * 10 /
			imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			imgsensor_info.normal_video.framelength)
			? (frame_length -
			imgsensor_info.normal_video.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.normal_video.framelength +
			imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		imgsensor.dummy_pixel = 1855;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		frame_length =
			imgsensor_info.cap1.pclk / framerate * 10 /
			imgsensor_info.cap1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			imgsensor_info.cap1.framelength)
			? (frame_length -
			imgsensor_info.cap1.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.cap1.framelength +
			imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		imgsensor.dummy_pixel = 1102;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length =
			imgsensor_info.hs_video.pclk / framerate * 10 /
			imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			imgsensor_info.hs_video.framelength)
			? (frame_length -
			imgsensor_info.hs_video.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.hs_video.framelength +
			imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length =
			imgsensor_info.slim_video.pclk / framerate * 10 /
			imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			imgsensor_info.slim_video.framelength)
			? (frame_length -
			imgsensor_info.slim_video.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.slim_video.framelength +
			imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	default:	/* coding with  preview scenario by default */
		frame_length =
			imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			imgsensor_info.pre.framelength)
			? (frame_length -
			imgsensor_info.pre.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		cam_pr_debug("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id,
	MUINT32 *framerate)
{
	cam_pr_debug("scenario_id = %d\n", scenario_id);
	if (framerate == NULL) {
		cam_pr_debug("---- framerate is NULL ---- check here\n");
		return ERROR_NONE;
	}
	spin_lock(&imgsensor_drv_lock);
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
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}


static kal_uint32 streaming_control(kal_bool enable)
{
	cam_pr_debug("streaming_enable(0=Sw Standby,1=streaming): %d\n",
		enable);
	if (enable) {
		write_cmos_sensor_8(0x0100, 0x01);
	} else {
		write_cmos_sensor_8(0x0100, 0x00);
	}

	return ERROR_NONE;
}

static kal_uint32 feature_control(
	MSDK_SENSOR_FEATURE_ENUM feature_id,
	UINT8 *feature_para,
	UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
	    (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	if (feature_para == NULL) {
		cam_pr_debug(" ---- %s ---- error params1\n", __func__);
		return ERROR_NONE;
	}

	cam_pr_debug("feature_id = %d\n", feature_id);

	switch (feature_id) {
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = imgsensor.line_length;
		*feature_return_para_16 = imgsensor.frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		cam_pr_debug("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n",
			imgsensor.pclk, imgsensor.current_fps);
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
		set_auto_flicker_mode((BOOL) * feature_data_16,
			*(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM) *feature_data,
			*(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM) *(feature_data),
			(MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL) * feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		cam_pr_debug("current fps :%d\n", (UINT32) *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		cam_pr_debug("ihdr enable :%d\n", (BOOL) * feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_en = (BOOL) * feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		cam_pr_debug("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			(UINT32) *feature_data);

		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)
			(uintptr_t) (*(feature_data + 1));

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
		cam_pr_debug("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16) *feature_data, (UINT16) *(feature_data + 1),
			(UINT16) *(feature_data + 2));
		ihdr_write_shutter_gain((UINT16) *feature_data,
			(UINT16) *(feature_data + 1),
			(UINT16) *(feature_data + 2));
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
		kal_uint32 rate;

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			rate = imgsensor_info.cap.mipi_pixel_rate;
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
		*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = rate;
	}
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		cam_pr_debug("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		cam_pr_debug("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_PIXEL_RATE:
	{
		kal_uint32 rate;

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			rate = (imgsensor_info.cap.pclk /
			       (imgsensor_info.cap.linelength - 80))*
			       imgsensor_info.cap.grabwindow_width;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			rate = (imgsensor_info.normal_video.pclk /
			       (imgsensor_info.normal_video.linelength - 80))*
			       imgsensor_info.normal_video.grabwindow_width;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			rate = (imgsensor_info.hs_video.pclk /
			       (imgsensor_info.hs_video.linelength - 80))*
			       imgsensor_info.hs_video.grabwindow_width;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			rate = (imgsensor_info.slim_video.pclk /
			       (imgsensor_info.slim_video.linelength - 80))*
			       imgsensor_info.slim_video.grabwindow_width;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			rate = (imgsensor_info.pre.pclk /
			       (imgsensor_info.pre.linelength - 80))*
			       imgsensor_info.pre.grabwindow_width;
			break;
		}
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
	}
		break;
	default:
		break;
	}

	return ERROR_NONE;
}

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 BLACKJACK_SEA_MT9D015_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}
