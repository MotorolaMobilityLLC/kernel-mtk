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
 *   S5K3H7mipi_Sensor.c
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

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <asm/system.h>
//#include <linux/xlog.h>
#include "kd_camera_typedef.h"
//#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "s5k3h7ymipiraw_Sensor.h"


#define PFX "s5k3h7yx_camera_sensor"
#define LOG_1 LOG_INF("s5k3h7yx,MIPI 4LANE\n")
#define LOG_2 LOG_INF("preview 2664*1500@30fps,888Mbps/lane; video 5328*3000@30fps,1390Mbps/lane; capture 16M@30fps,1390Mbps/lane\n")
//#define LOG_DBG(format, args...) xlog_printk(ANDROID_LOG_DEBUG ,PFX, "[%S] " format, __FUNCTION__, ##args)
//#define LOG_INF(format, args...)	xlog_printk(ANDROID_LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)

#ifndef CONFIG_MTK_FPGA
#define LOGE(fmt, args...)   pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##args)
#define LOG_INF(format, args...)	pr_debug(format, ##args)
#else
#define LOGE(fmt, args...)   printk(PFX "[%s] " fmt, __FUNCTION__, ##args)
#define LOG_INF(fmt, args...)    printk(PFX "[%s] " fmt, __FUNCTION__, ##args)
#endif

static DEFINE_SPINLOCK(imgsensor_drv_lock);

#define SLOW_MOTION_120FPS

static imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K3H7Y_SENSOR_ID,
	.checksum_value = 0x9c198b8c,
	.pre = {
		.pclk = 280000000,				//record different mode's pclk
		.linelength = 3688,				//record different mode's linelength
		.framelength =2530, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 1600,		//record different mode's width of grabwindow
		.grabwindow_height = 650,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 280000000,
		.linelength =3688,
		.framelength = 2530,
		.startx = 0,
		.starty = 0,
		.grabwindow_width =3200,//5334,
		.grabwindow_height = 2400,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
		},
	.cap1 = {
		.pclk = 280000000,
		.linelength =3688,
		.framelength = 2530,
		.startx = 0,
		.starty = 0,
		.grabwindow_width =3200,//5334,
		.grabwindow_height = 2400,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 280000000,
		.linelength = 3688,
		.framelength = 2530,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3200,//5334,
		.grabwindow_height = 2400,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
	},

	.hs_video = {
		.pclk = 280000000,
		.linelength = 3560,
		.framelength = 654,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 800,		//record different mode's width of grabwindow
		.grabwindow_height = 600,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 1200,
	},

	.slim_video = {
		.pclk = 280000000,
		.linelength = 3088,
		.framelength = 766,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1200,//1280,
		.grabwindow_height =700,// 720,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
	},
	.margin = 16,
	.min_shutter = 3,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 2,
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,

	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,
	.mclk = 12,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x20, 0x5A, 0xff},
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x3D0,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 30,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x5A,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{{ 3280, 2464,	  0,   0, 3280, 2464, 1634,  1224, 0000, 0000, 1634, 1224,0,   0, 1600,  1200}, // Preview
 { 3280, 2464,	  0,   0, 3280, 2464, 3264, 2448, 0000, 0000, 3264, 2448,	 0,   0, 3200, 2400}, // capture
 { 3280, 2464,	  0,   0, 3280, 2464, 3264, 2448, 0000, 0000, 3264, 2448,	 0,   0,3200,2400}, // video
 { 3280, 2464,	  0,   0, 3280, 2464, 816, 612, 0000, 0000, 816, 612,	  0,   0, 800, 600}, //hight speed video
 { 3280, 2464,	  0,   0, 3280, 2464, 1280,  720, 0000, 0000, 1280,  720,	 0,   0, 1200,  700}};// slim video

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(pusendcmd , 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}


static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
}
#if 0
static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(pusendcmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
    return get_byte;
}
#endif
static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 3, imgsensor.i2c_write_id);
}

static void S5K3H7Y_wordwrite_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
}
static void S5K3H7Y_bytewrite_cmos_sensor(kal_uint16 addr, kal_uint8 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{

//	 LOG_INF("currently the mode is %d,dummyline = %d, dummypixels = %d ", imgsensor.dummy_line, imgsensor.dummy_pixel);
    write_cmos_sensor_8(0x0104, 0x01);
    write_cmos_sensor(0x0340, imgsensor.frame_length);
    write_cmos_sensor(0x0342, imgsensor.line_length);
    write_cmos_sensor_8(0x0104, 0x00);


}	/*	set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	return (read_cmos_sensor(0x0000));
}
static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
		kal_uint32 frame_length = imgsensor.frame_length;
		//unsigned long flags;

		LOG_INF("framerate = %d, min framelength should enable? \n", framerate);

		frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
		//dummy_line = frame_length - imgsensor.min_frame_length;
		//if (dummy_line < 0)
			//imgsensor.dummy_line = 0;
		//else
			//imgsensor.dummy_line = dummy_line;
		//imgsensor.frame_length = frame_length + imgsensor.dummy_line;
		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		{
			imgsensor.frame_length = imgsensor_info.max_frame_length;
			imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
		}
		if (min_framelength_en)
			imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();


}	/*	set_max_framerate  */


static void write_shutter(kal_uint16 shutter)
{

	//kal_uint16 realtime_fps = 0;
		//kal_uint32 frame_length = 0;
		unsigned long flags;
		spin_lock_irqsave(&imgsensor_drv_lock, flags);
		imgsensor.shutter = shutter;
		spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

		spin_lock_irqsave(&imgsensor_drv_lock, flags);
		if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
			imgsensor.frame_length = shutter + imgsensor_info.margin;
		else
			imgsensor.frame_length = imgsensor.min_frame_length;
		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
			imgsensor.frame_length = imgsensor_info.max_frame_length;
		spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
		shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
		shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;



 #if 0
		if (imgsensor.autoflicker_en) {
			realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
			if(realtime_fps >= 297 && realtime_fps <= 305)
				set_max_framerate(296,0);
			else if(realtime_fps >= 147 && realtime_fps <= 150)
				set_max_framerate(146,0);
		} else {
			// Extend frame length
			write_cmos_sensor_8(0x0104,0x01);
			write_cmos_sensor(0x0340, imgsensor.frame_length);
			write_cmos_sensor_8(0x0104,0x00);
		}
#endif
		// Update Shutter
		//write_cmos_sensor_8(0x0104,0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length);
		write_cmos_sensor(0x0202, shutter);
		//write_cmos_sensor_8(0x0104,0x00);
        LOG_INF("Currently camera mode is %d,shutter is %d, framelength=%d,linelength=%d\n",imgsensor.sensor_mode,imgsensor.shutter,imgsensor.frame_length,imgsensor.line_length);

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
#if 0
static void set_shutter(kal_uint16 shutter)
{

	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
	LOG_INF("Currently camera mode is %d,framerate is %d , framelength=%d,linelength=%d\n",imgsensor.sensor_mode,imgsensor.current_fps,imgsensor.frame_length,imgsensor.line_length);

}
#endif

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	 kal_uint16 reg_gain = 0x0;

    reg_gain = gain/2;
    return (kal_uint16)reg_gain;
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
	 /* [0:3] = N meams N /16 X  */
	 /* [4:9] = M meams M X 	  */
	 /* Total gain = M + N /16 X   */

	 //
	 if (gain < BASEGAIN || gain > 32 * BASEGAIN) {
		 LOG_INF("Error gain setting");

		 if (gain < BASEGAIN)
			 gain = BASEGAIN;
		 else if (gain > 32 * BASEGAIN)
			 gain = 32 * BASEGAIN;
	 }

	 reg_gain = gain2reg(gain);
	 spin_lock(&imgsensor_drv_lock);
	 imgsensor.gain = reg_gain;
	 spin_unlock(&imgsensor_drv_lock);
	 LOG_INF("gain = %d , reg_gain = 0x%x,shutter=%d,the result of gain*shutter is %d ", gain, reg_gain,imgsensor.shutter,gain*(imgsensor.shutter));

	 //write_cmos_sensor_8(0x0104, 0x01);
	 write_cmos_sensor_8(0x0204,(reg_gain>>8));
	 write_cmos_sensor_8(0x0205,(reg_gain&0xff));
	 //write_cmos_sensor_8(0x0104, 0x00);


	 return gain;

}

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
#if 1
		LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
		if (imgsensor.ihdr_en) {

			spin_lock(&imgsensor_drv_lock);
				if (le > imgsensor.min_frame_length - imgsensor_info.margin)
					imgsensor.frame_length = le + imgsensor_info.margin;
				else
					imgsensor.frame_length = imgsensor.min_frame_length;
				if (imgsensor.frame_length > imgsensor_info.max_frame_length)
					imgsensor.frame_length = imgsensor_info.max_frame_length;
				spin_unlock(&imgsensor_drv_lock);
				if (le < imgsensor_info.min_shutter) le = imgsensor_info.min_shutter;
				if (se < imgsensor_info.min_shutter) se = imgsensor_info.min_shutter;


					// Extend frame length first
			write_cmos_sensor_8(0x0104,0x01);
			write_cmos_sensor(0x0340, imgsensor.frame_length);

			//write_cmos_sensor(0x0202, se);
			//write_cmos_sensor(0x021e,le);
			write_cmos_sensor(0x602A,0x021e);
			write_cmos_sensor(0x6f12,le);
			write_cmos_sensor(0x602A,0x0202);
			write_cmos_sensor(0x6f12,se);
			 write_cmos_sensor_8(0x0104,0x00);
		LOG_INF("iHDR:imgsensor.frame_length=%d\n",imgsensor.frame_length);
			set_gain(gain);
		}

#endif





}



static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d", image_mirror);

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
	spin_lock(&imgsensor_drv_lock);
    imgsensor.mirror= image_mirror;
    spin_unlock(&imgsensor_drv_lock);
    switch (image_mirror) {

        case IMAGE_NORMAL:
            write_cmos_sensor_8(0x0101,0x00);   // Gr
            break;
        case IMAGE_H_MIRROR:
            write_cmos_sensor_8(0x0101,0x01);
            break;
        case IMAGE_V_MIRROR:
            write_cmos_sensor_8(0x0101,0x02);
            break;
        case IMAGE_HV_MIRROR:
            write_cmos_sensor_8(0x0101,0x03);//Gb
            break;
        default:
			LOG_INF("Error image_mirror setting\n");
    }

}

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
#if 0
static void S5K3H7YInitSetting_FPGA(void)
{
	LOG_INF("S5K3H7YInitSetting_FPGA  enter\n");
	 // SENSORDB("enter\n");
	  //#define MCLK10MHZ
	//1600x1200
	  S5K3H7Y_wordwrite_cmos_sensor(0x6010,0x0001);   // Reset
	  mdelay(10);//; delay(10ms)
	  S5K3H7Y_wordwrite_cmos_sensor(0x6218,0xF1D0);   // open all clocks
	  S5K3H7Y_wordwrite_cmos_sensor(0x6214,0xF9F0);   // open all clocks
	  S5K3H7Y_wordwrite_cmos_sensor(0xF400,0x0BBC); // workaround for the SW standby current
	  S5K3H7Y_wordwrite_cmos_sensor(0x6226,0x0001);   // open APB clock for I2C transaction
	  S5K3H7Y_wordwrite_cmos_sensor(0xB0C0,0x000C);
	  S5K3H7Y_wordwrite_cmos_sensor(0x6226,0x0000);   // close APB clock for I2C transaction
	  S5K3H7Y_wordwrite_cmos_sensor(0x6218,0xF9F0);   // close all clocks
	  S5K3H7Y_wordwrite_cmos_sensor(0x38FA,0x0030);  // gisp_offs_gains_bls_offs_0_
	  S5K3H7Y_wordwrite_cmos_sensor(0x38FC,0x0030);  // gisp_offs_gains_bls_offs_1_
	  S5K3H7Y_wordwrite_cmos_sensor(0x32CE,0x0060);   // senHal_usWidthStOfsInit
	  S5K3H7Y_wordwrite_cmos_sensor(0x32D0,0x0024);   // senHal_usHeightStOfsInit
  #ifdef USE_MIPI_2_LANES
	  S5K3H7Y_wordwrite_cmos_sensor(0x0114,0x01); // #smiaRegs_rw_output_lane_mode
  #endif
	  S5K3H7Y_wordwrite_cmos_sensor(0x0086,0x01FF);   // analogue_gain_code_max
  #ifdef MCLK10MHZ
	  S5K3H7Y_wordwrite_cmos_sensor(0x0136,0x0a28);   // #smiaRegs_rw_op_cond_extclk_frequency_mhz
  #else /*12Mhz*/
	  S5K3H7Y_wordwrite_cmos_sensor(0x0136,0x0FA0);   // #smiaRegs_rw_op_cond_extclk_frequency_mhz
  #endif
	  S5K3H7Y_wordwrite_cmos_sensor(0x0300,0x0002);   // smiaRegs_rw_clocks_vt_pix_clk_div
	  S5K3H7Y_wordwrite_cmos_sensor(0x0302,0x0001);   // smiaRegs_rw_clocks_vt_sys_clk_div
	  S5K3H7Y_wordwrite_cmos_sensor(0x0304,0x0003);   // smiaRegs_rw_clocks_pre_pll_clk_div
	  S5K3H7Y_wordwrite_cmos_sensor(0x0306,0x0094);    // smiaRegs_rw_clocks_pll_multiplier
	  S5K3H7Y_wordwrite_cmos_sensor(0x0308,0x0008);   // smiaRegs_rw_clocks_op_pix_clk_div
	  S5K3H7Y_wordwrite_cmos_sensor(0x030A,0x0006);   // smiaRegs_rw_clocks_op_sys_clk_div
	  S5K3H7Y_wordwrite_cmos_sensor(0x030C,0x0003);   // smiaRegs_rw_clocks_secnd_pre_pll_clk_div
  #ifdef MCLK10MHZ
	  S5K3H7Y_wordwrite_cmos_sensor(0x030E,0x0094);    // smiaRegs_rw_clocks_secnd_pll_multiplier
  #else
	  S5K3H7Y_wordwrite_cmos_sensor(0x030E,0x0080);    // smiaRegs_rw_clocks_secnd_pll_multiplier
  #endif
	  S5K3H7Y_wordwrite_cmos_sensor(0x311C,0x0BB8);   //Increase Blank time on account of process time
	  S5K3H7Y_wordwrite_cmos_sensor(0x311E,0x0BB8);   //Increase Blank time on account of process time
	  S5K3H7Y_wordwrite_cmos_sensor(0x034C,0x0cc0);   // smiaRegs_rw_frame_timing_x_output_size
	  S5K3H7Y_wordwrite_cmos_sensor(0x034E,0x0990);   // smiaRegs_rw_frame_timing_y_output_size
	  S5K3H7Y_wordwrite_cmos_sensor(0x0380,0x0001);   // #smiaRegs_rw_sub_sample_x_even_inc
	  S5K3H7Y_wordwrite_cmos_sensor(0x0382,0x0001);   // #smiaRegs_rw_sub_sample_x_odd_inc
	  S5K3H7Y_wordwrite_cmos_sensor(0x0384,0x0001);   // #smiaRegs_rw_sub_sample_y_even_inc
	  S5K3H7Y_wordwrite_cmos_sensor(0x0386,0x0001);   // #smiaRegs_rw_sub_sample_y_odd_inc

	  S5K3H7Y_wordwrite_cmos_sensor(0x0112,0x0A0A); 	//raw 10 foramt //DPCM
	  //S5K3H7Y_bytewrite_cmos_sensor(0x3053,0x01); 			//line start/end short packet
	  S5K3H7Y_wordwrite_cmos_sensor(0x300C,0x102);			//pixel order B Gb Gr R

	  S5K3H7Y_wordwrite_cmos_sensor(0x0900,0x0111);   // #smiaRegs_rw_binning_mode
	  //S5K3H7Y_bytewrite_cmos_sensor(0x0901,0x0011); // #smiaRegs_rw_binning_type
	  S5K3H7Y_wordwrite_cmos_sensor(0x0902,0x0100);   // #smiaRegs_rw_binning_weighting
	  S5K3H7Y_wordwrite_cmos_sensor(0x0342,0xC86A);   // smiaRegs_rw_frame_timing_line_length_pck
	  S5K3H7Y_wordwrite_cmos_sensor(0x0340,0x09c4);   // smiaRegs_rw_frame_timing_frame_length_lines
	  S5K3H7Y_wordwrite_cmos_sensor(0x0200,0x0BEF); 	// smiaRegs_rw_integration_time_fine_integration_time
	  S5K3H7Y_wordwrite_cmos_sensor(0x0202,0x02d9); 	// smiaRegs_rw_integration_time_coarse_integration_time

	  S5K3H7Y_wordwrite_cmos_sensor(0x0344,0x0004);   // #smiaRegs_rw_binning_weighting
	  S5K3H7Y_wordwrite_cmos_sensor(0x0346,0x0004);   // smiaRegs_rw_frame_timing_line_length_pck
	  S5K3H7Y_wordwrite_cmos_sensor(0x0348,0x0cc3);   // smiaRegs_rw_frame_timing_frame_length_lines
	  S5K3H7Y_wordwrite_cmos_sensor(0x034a,0x0993); 	// smiaRegs_rw_integration_time_fine_integration_time


	  S5K3H7Y_wordwrite_cmos_sensor(0x37F8,0x0001); 	// Analog Gain Precision, 0/1/2/3 = 32/64/128/256 base 1X, set 1=> 64 =1X


	  S5K3H7Y_wordwrite_cmos_sensor(0x0204,0x0020);   // X1
	  S5K3H7Y_wordwrite_cmos_sensor(0x0B05,0x0001); 	// #smiaRegs_rw_isp_mapped_couplet_correct_enable
	  S5K3H7Y_wordwrite_cmos_sensor(0x0B00,0x0080); 	// #smiaRegs_rw_isp_shading_correction_enable
	  S5K3H7Y_bytewrite_cmos_sensor(0x0100,0x00);	// smiaRegs_rw_general_setup_mode_select
  #if 0  // test pattern = Color Checker
	  S5K3H7Y_wordwrite_cmos_sensor(0x0600,0x0100);
  #endif
	  S5K3H7Y_bytewrite_cmos_sensor(0x0100,0x01);   // smiaRegs_rw_general_setup_mode_select

write_cmos_sensor(0x0100,	0x0100);


}
#endif

static void S5K3H7YInitSetting_2Lane84Mhz(void)
{
	LOG_INF("S5K3H7YInitSetting  enter\n");
  //1600x1200
	S5K3H7Y_wordwrite_cmos_sensor(0x6010,0x0001);	// Reset
	mdelay(10);//; delay(10ms)
	// Base setfile : Rev - 3126
	// Date: 2012-01-05 15:10:35 +0900 (THU, 05 JAN 2012)
	// 3H7 Analog set file BQ Mode
	//=====================================================================================
	S5K3H7Y_wordwrite_cmos_sensor(0x6028,0xD000); // set page D000
	S5K3H7Y_wordwrite_cmos_sensor(0x38FA,0x0030);	// gisp_offs_gains_bls_offs_0_
	S5K3H7Y_wordwrite_cmos_sensor(0x38FC,0x0030);	// gisp_offs_gains_bls_offs_1_
	S5K3H7Y_wordwrite_cmos_sensor(0x0086,0x01FF);	//#smiaRegs_rd_analog_gain_analogue_gain_code_max
	S5K3H7Y_wordwrite_cmos_sensor(0x012A,0x0060); //0040	//#smiaRegs_rw_analog_gain_mode_AG_th
	S5K3H7Y_wordwrite_cmos_sensor(0x012C,0x7077);	//#smiaRegs_rw_analog_gain_mode_F430_val
	S5K3H7Y_wordwrite_cmos_sensor(0x012E,0x7777);	//#smiaRegs_rw_analog_gain_mode_F430_default_val
	//=====================================================================================
	// Sensor XY cordination
	//=====================================================================================
	S5K3H7Y_wordwrite_cmos_sensor(0x32CE,0x0060);	// senHal_usWidthStOfsInit	 add
	S5K3H7Y_wordwrite_cmos_sensor(0x32D0,0x0024);	// senHal_usHeightStOfsInit  add
	//========================================================
	// Setting for MIPI CLK (Don't change)
	S5K3H7Y_wordwrite_cmos_sensor(0x6218,0xF1D0);	// open all clocks
	S5K3H7Y_wordwrite_cmos_sensor(0x6214,0xF9F0);	// open all clocks
	S5K3H7Y_wordwrite_cmos_sensor(0x6226,0x0001);	// open APB clock for I2C transaction
	//=====================================================================================
	// START OF HW REGISTERS APS/Analog UPDATING
	//=====================================================================================
	S5K3H7Y_wordwrite_cmos_sensor(0xB0C0 ,0x000C);
	S5K3H7Y_wordwrite_cmos_sensor(0xF400 ,0x0BBC);
	S5K3H7Y_wordwrite_cmos_sensor(0xF616 ,0x0004); //aig_tmc_gain
	//=====================================================================================
	// END OF HW REGISTERS APS/Analog UPDATING
	//=====================================================================================
	S5K3H7Y_wordwrite_cmos_sensor(0x6226 ,0x0000); //close APB clock for I2C transaction
	S5K3H7Y_wordwrite_cmos_sensor(0x6218 ,0xF9F0); //close all clocks
	S5K3H7Y_wordwrite_cmos_sensor(0x3338 ,0x0264); //senHal_MaxCdsTime
	////////////////////////////////////////////////
	//											  //
	//	   PLUSARGS for configuration	//
	//											  //
	////////////////////////////////////////////////
	S5K3H7Y_wordwrite_cmos_sensor(0x6218,0xF1D0); //open all clocks
	S5K3H7Y_wordwrite_cmos_sensor(0x6214,0xF9F0); //open all clocks
	S5K3H7Y_wordwrite_cmos_sensor(0x6226,0x0001); //open APB clock for I2C transaction
	S5K3H7Y_wordwrite_cmos_sensor(0xB0C0,0x000C); //open APB clock for I2C transaction
	S5K3H7Y_wordwrite_cmos_sensor(0x6226,0x0000); //close APB clock for I2C transaction
	S5K3H7Y_wordwrite_cmos_sensor(0x6218,0xF9F0); //close all clocks
	S5K3H7Y_bytewrite_cmos_sensor(0x0114,0x01);	// 1 :2Lane, 3: 4 Lane

	// set PLL
	S5K3H7Y_wordwrite_cmos_sensor(0x0136, 0x0600); //smiaRegs_rw_op_cond_extclk_frequency_mhz, Mclk 6Mhz
	S5K3H7Y_wordwrite_cmos_sensor(0x0304, 0x0002); //smiaRegs_rw_clocks_pre_pll_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x0306, 0x00A8); //smiaRegs_rw_clocks_pll_multiplier
	S5K3H7Y_wordwrite_cmos_sensor(0x0302, 0x0001); //smiaRegs_rw_clocks_vt_sys_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x0300, 0x0002); //smiaRegs_rw_clocks_vt_pix_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x030C ,0x0002); // smiaRegs_rw_clocks_secnd_pre_pll_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x030E ,0x00A8); // smiaRegs_rw_clocks_secnd_pll_multiplier
	S5K3H7Y_wordwrite_cmos_sensor(0x030A ,0x0006); //smiaRegs_rw_clocks_op_sys_clk_div


	// Set FPS
	S5K3H7Y_wordwrite_cmos_sensor(0x0342 ,0xFFFF); //F068 //1C68	// smiaRegs_rw_frame_timing_line_length_pck //For MIPI CLK 648Mhz 30.03fps (Default 30fps MIPI_clk 664Mhz 0E50)
	S5K3H7Y_wordwrite_cmos_sensor(0x0340 ,0x09b5);	// smiaRegs_rw_frame_timing_frame_length_lines //For MIPI CLK 648MHz 30.03fps (Default 30fps MIPI_clk 664Mhz 09F2)
	// Set int.time
	S5K3H7Y_wordwrite_cmos_sensor(0x0200 ,0x0618);	// smiaRegs_rw_integration_time_fine_integration_time
	S5K3H7Y_wordwrite_cmos_sensor(0x0202 ,0x0150);	// smiaRegs_rw_integration_time_coarse_integration_time

	// Set gain
	S5K3H7Y_wordwrite_cmos_sensor(0x0204 ,0x0020);	// X1
	//CONFIGURATION REGISTERS

	//M2M
	S5K3H7Y_wordwrite_cmos_sensor(0x31FE ,0xC004); // ash_uDecompressXgrid[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x3200 ,0xC4F0); // ash_uDecompressXgrid[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x3202 ,0xCEC8); // ash_uDecompressXgrid[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x3204 ,0xD8A0); // ash_uDecompressXgrid[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x3206 ,0xE278); // ash_uDecompressXgrid[4]
	S5K3H7Y_wordwrite_cmos_sensor(0x3208 ,0xEC50); // ash_uDecompressXgrid[5]
	S5K3H7Y_wordwrite_cmos_sensor(0x320A ,0xF628); // ash_uDecompressXgrid[6]
	S5K3H7Y_wordwrite_cmos_sensor(0x320C ,0x0000); // ash_uDecompressXgrid[7]
	S5K3H7Y_wordwrite_cmos_sensor(0x320E ,0x09D8); // ash_uDecompressXgrid[8]
	S5K3H7Y_wordwrite_cmos_sensor(0x3210 ,0x13B0); // ash_uDecompressXgrid[9]
	S5K3H7Y_wordwrite_cmos_sensor(0x3212 ,0x1D88); // ash_uDecompressXgrid[10]
	S5K3H7Y_wordwrite_cmos_sensor(0x3214 ,0x2760); // ash_uDecompressXgrid[11]
	S5K3H7Y_wordwrite_cmos_sensor(0x3216 ,0x3138); // ash_uDecompressXgrid[12]
	S5K3H7Y_wordwrite_cmos_sensor(0x3218 ,0x3B10); // ash_uDecompressXgrid[13]
	S5K3H7Y_wordwrite_cmos_sensor(0x321A ,0x3FFC); // ash_uDecompressXgrid[14]

	S5K3H7Y_wordwrite_cmos_sensor(0x0086,0x01FF);	// analogue_gain_code_max
	S5K3H7Y_wordwrite_cmos_sensor(0x0136,0x0FA0);	// #smiaRegs_rw_op_cond_extclk_frequency_mhz
	S5K3H7Y_wordwrite_cmos_sensor(0x0300,0x0002);	// smiaRegs_rw_clocks_vt_pix_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x0302,0x0001);	// smiaRegs_rw_clocks_vt_sys_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x0304,0x0003);	// smiaRegs_rw_clocks_pre_pll_clk_div
    S5K3H7Y_wordwrite_cmos_sensor(0x0306,0x0094);    // smiaRegs_rw_clocks_pll_multiplier
	S5K3H7Y_wordwrite_cmos_sensor(0x0308,0x0008);	// smiaRegs_rw_clocks_op_pix_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x030A,0x0004);	// smiaRegs_rw_clocks_op_sys_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x030C,0x0003);	// smiaRegs_rw_clocks_secnd_pre_pll_clk_div
   	S5K3H7Y_wordwrite_cmos_sensor(0x030E,0x0080);    // smiaRegs_rw_clocks_secnd_pll_multiplier
    S5K3H7Y_wordwrite_cmos_sensor(0x311C,0x0BB8);   //Increase Blank time on account of process time
    S5K3H7Y_wordwrite_cmos_sensor(0x311E,0x0BB8);   //Increase Blank time on account of process time
	S5K3H7Y_wordwrite_cmos_sensor(0x034C,0x0cc0);	// smiaRegs_rw_frame_timing_x_output_size
	S5K3H7Y_wordwrite_cmos_sensor(0x034E,0x0990);	// smiaRegs_rw_frame_timing_y_output_size
	S5K3H7Y_wordwrite_cmos_sensor(0x0380,0x0001);	// #smiaRegs_rw_sub_sample_x_even_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0382,0x0001);	// #smiaRegs_rw_sub_sample_x_odd_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0384,0x0001);	// #smiaRegs_rw_sub_sample_y_even_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0386,0x0001);	// #smiaRegs_rw_sub_sample_y_odd_inc

	S5K3H7Y_wordwrite_cmos_sensor(0x321C ,0xC004); // ash_uDecompressYgrid[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x321E ,0xCCD0); // ash_uDecompressYgrid[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x3220 ,0xD99C); // ash_uDecompressYgrid[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x3222 ,0xE668); // ash_uDecompressYgrid[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x3224 ,0xF334); // ash_uDecompressYgrid[4]
	S5K3H7Y_wordwrite_cmos_sensor(0x3226 ,0x0000); // ash_uDecompressYgrid[5]
	S5K3H7Y_wordwrite_cmos_sensor(0x3228 ,0x0CCC); // ash_uDecompressYgrid[6]
	S5K3H7Y_wordwrite_cmos_sensor(0x322A ,0x1998); // ash_uDecompressYgrid[7]
	S5K3H7Y_wordwrite_cmos_sensor(0x322C ,0x2664); // ash_uDecompressYgrid[8]
	S5K3H7Y_wordwrite_cmos_sensor(0x322E ,0x3330); // ash_uDecompressYgrid[9]
	S5K3H7Y_wordwrite_cmos_sensor(0x3230 ,0x3FFC); // ash_uDecompressYgrid[10]

	S5K3H7Y_wordwrite_cmos_sensor(0x3232 ,0x0100); // ash_uDecompressWidth
	S5K3H7Y_wordwrite_cmos_sensor(0x3234 ,0x0100); // ash_uDecompressHeight
	S5K3H7Y_bytewrite_cmos_sensor(0x3237 ,0x00); // ash_uDecompressScale          // 00 - the value for this register is read from NVM page #0 byte #47 bits [3]-[7] i.e. 5 MSB bits  // other value - e.g. 0E, will be read from this register settings in the set file and ignore the value set in NVM as described above
	S5K3H7Y_bytewrite_cmos_sensor(0x3238 ,0x09); // ash_uDecompressRadiusShifter
	S5K3H7Y_bytewrite_cmos_sensor(0x3239 ,0x09); // ash_uDecompressParabolaScale
	S5K3H7Y_bytewrite_cmos_sensor(0x323A ,0x0B); // ash_uDecompressFinalScale
	S5K3H7Y_bytewrite_cmos_sensor(0x3160 ,0x06);	// ash_GrasCfg  06  // 36  // [5:5]	fegras_gain_clamp   0 _ clamp gain to 0..1023 // _V_// 1 _ clamp_gain to 256..1023// [4:4]	fegras_plus_zero   Adjust final gain by the one or the zero // 0 _ [Output = Input x Gain x Alfa]  // _V_// 1 _ [Output = Input x (1 + Gain x Alfa)]
	                                                //BASE Profile parabola start
	S5K3H7Y_bytewrite_cmos_sensor(0x3161 ,0x00); // ash_GrasShifter 00
	S5K3H7Y_wordwrite_cmos_sensor(0x3164 ,0x09C4); // ash_luma_params[0]_tmpr
	S5K3H7Y_wordwrite_cmos_sensor(0x3166 ,0x0100); // ash_luma_params[0]_alpha[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x3168 ,0x0100); // ash_luma_params[0]_alpha[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x316A ,0x0100); // ash_luma_params[0]_alpha[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x316C ,0x0100); // ash_luma_params[0]_alpha[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x316E ,0x001A); // ash_luma_params[0]_beta[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x3170 ,0x002F); // ash_luma_params[0]_beta[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x3172 ,0x0000); // ash_luma_params[0]_beta[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x3174 ,0x001A); // ash_luma_params[0]_beta[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x3176 ,0x0A8C); // ash_luma_params[1]_tmpr
	S5K3H7Y_wordwrite_cmos_sensor(0x3178 ,0x0100); // ash_luma_params[1]_alpha[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x317A ,0x0100); // ash_luma_params[1]_alpha[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x317C ,0x0100); // ash_luma_params[1]_alpha[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x317E ,0x0100); // ash_luma_params[1]_alpha[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x3180 ,0x001A); // ash_luma_params[1]_beta[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x3182 ,0x002F); // ash_luma_params[1]_beta[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x3184 ,0x0000); // ash_luma_params[1]_beta[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x3186 ,0x001A); // ash_luma_params[1]_beta[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x3188 ,0x0CE4); // ash_luma_params[2]_tmpr
	S5K3H7Y_wordwrite_cmos_sensor(0x318A ,0x0100); // ash_luma_params[2]_alpha[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x318C ,0x0100); // ash_luma_params[2]_alpha[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x318E ,0x0100); // ash_luma_params[2]_alpha[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x3190 ,0x0100); // ash_luma_params[2]_alpha[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x3192 ,0x001A); // ash_luma_params[2]_beta[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x3194 ,0x002F); // ash_luma_params[2]_beta[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x3196 ,0x0000); // ash_luma_params[2]_beta[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x3198 ,0x001A); // ash_luma_params[2]_beta[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x319A ,0x1004); // ash_luma_params[3]_tmpr
	S5K3H7Y_wordwrite_cmos_sensor(0x319C ,0x0100); // ash_luma_params[3]_alpha[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x319E ,0x0100); // ash_luma_params[3]_alpha[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x31A0 ,0x0100); // ash_luma_params[3]_alpha[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x31A2 ,0x0100); // ash_luma_params[3]_alpha[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x31A4 ,0x001A); // ash_luma_params[3]_beta[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x31A6 ,0x002F); // ash_luma_params[3]_beta[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x31A8 ,0x0000); // ash_luma_params[3]_beta[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x31AA ,0x001A); // ash_luma_params[3]_beta[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x31AC ,0x1388); // ash_luma_params[4]_tmpr
	S5K3H7Y_wordwrite_cmos_sensor(0x31AE ,0x0100); // ash_luma_params[4]_alpha[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x31B0 ,0x0100); // ash_luma_params[4]_alpha[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x31B2 ,0x0100); // ash_luma_params[4]_alpha[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x31B4 ,0x0100); // ash_luma_params[4]_alpha[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x31B6 ,0x001A); // ash_luma_params[4]_beta[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x31B8 ,0x002F); // ash_luma_params[4]_beta[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x31BA ,0x0000); // ash_luma_params[4]_beta[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x31BC ,0x001A); // ash_luma_params[4]_beta[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x31BE ,0x1964); // ash_luma_params[5]_tmpr
	S5K3H7Y_wordwrite_cmos_sensor(0x31C0 ,0x0100); // ash_luma_params[5]_alpha[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x31C2 ,0x0100); // ash_luma_params[5]_alpha[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x31C4 ,0x0100); // ash_luma_params[5]_alpha[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x31C6 ,0x0100); // ash_luma_params[5]_alpha[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x31C8 ,0x001A); // ash_luma_params[5]_beta[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x31CA ,0x002F); // ash_luma_params[5]_beta[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x31CC ,0x0000); // ash_luma_params[5]_beta[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x31CE ,0x001A); // ash_luma_params[5]_beta[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x31D0 ,0x1D4C); // ash_luma_params[6]_tmpr
	S5K3H7Y_wordwrite_cmos_sensor(0x31D2 ,0x0100); // ash_luma_params[6]_alpha[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x31D4 ,0x0100); // ash_luma_params[6]_alpha[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x31D6 ,0x0100); // ash_luma_params[6]_alpha[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x31D8 ,0x0100); // ash_luma_params[6]_alpha[3]
	S5K3H7Y_wordwrite_cmos_sensor(0x31DA ,0x001A); // ash_luma_params[6]_beta[0]
	S5K3H7Y_wordwrite_cmos_sensor(0x31DC ,0x002F); // ash_luma_params[6]_beta[1]
	S5K3H7Y_wordwrite_cmos_sensor(0x31DE ,0x0000); // ash_luma_params[6]_beta[2]
	S5K3H7Y_wordwrite_cmos_sensor(0x31E0 ,0x001A); // ash_luma_params[6]_beta[3]


	S5K3H7Y_bytewrite_cmos_sensor(0x3162 ,0x01	);	// ash_bLumaMode 01

	S5K3H7Y_wordwrite_cmos_sensor(0x301C ,0x0100);	// smiaRegs_vendor_gras_nvm_address
	S5K3H7Y_bytewrite_cmos_sensor(0x301E ,0x03	); 	//5 for SRAM test // smiaRegs_vendor_gras_load_from 03
	S5K3H7Y_bytewrite_cmos_sensor(0x323C ,0x00  );	// ash_bSkipNvmGrasOfs 01 // skipping the value set in nvm page 0 address 47
	S5K3H7Y_bytewrite_cmos_sensor(0x323D ,0x01	); 	// ash_uNvmGrasTblOfs 01 // load shading table 1 from nvm
	S5K3H7Y_bytewrite_cmos_sensor(0x1989 ,0x04  );	//smiaRegs_ro_edof_cap_uAlphaTempInd 04

	S5K3H7Y_bytewrite_cmos_sensor(0x0B01 ,0x00	);	// smiaRegs_rw_isp_luminance_correction_level
	                                					// Set shading power value, according to the shading power set at CCKIT and M2M tool when building the base
	                                					// LSC profile, where 0x80(128dec) stands for 1 (100%), 0x4F stands for 0.62(62%) etc.
	                                					//	When this register is set to 00 in the set file- the value for this register is read from NVM page #0 byte #55
	                                					// When this register is set to a value other than 00- e.g. 80, the value will be read from this register settings
	                                					// in the set file and ignore the value set in NVM as described above

	S5K3H7Y_bytewrite_cmos_sensor(0x0B00, 0x01);// smiaRegs_rw_isp_shading_correction_enable 01

	                                         // Streaming on
	S5K3H7Y_bytewrite_cmos_sensor(0x0100, 0x01);// smiaRegs_rw_general_setup_mode_select


}

static void sensor_init(void)
{
      LOG_INF("3H7 sensor_init enter\n");
//S5K3H7YInitSetting();

//S5K3H7YInitSetting_FPGA();
S5K3H7YInitSetting_2Lane84Mhz();


LOG_INF("RRR2 enter\n");
#if 0
	 // SENSORDB("enter\n");
	//1600x1200
	  write_cmos_sensor(0x6010,0x0001);   // Reset
	  mdelay(50);//; delay(10ms)
	  write_cmos_sensor(0x38FA,   0x0030);
	  write_cmos_sensor(0x38FC,   0x0030);
	  write_cmos_sensor(0x0086,   0x01FF);
	  write_cmos_sensor(0x6218,   0xF1D0);
	  write_cmos_sensor(0x6214,   0xF9F0);
	  write_cmos_sensor(0x6226,   0x0001);
	  write_cmos_sensor(0xB0C0,   0x000C);
	  write_cmos_sensor(0xF400,   0x0BBC);
	  write_cmos_sensor(0xF616,   0x0004);
	  write_cmos_sensor(0x6226,   0x0000);
	  write_cmos_sensor(0x6218,   0xF9F0);
	  write_cmos_sensor(0x6218,   0xF1D0);
	  write_cmos_sensor(0x6214,   0xF9F0);
	  write_cmos_sensor(0x6226,   0x0001);
	  write_cmos_sensor(0xB0C0,   0x000C);
	  write_cmos_sensor(0x6226,   0x0000);
	  write_cmos_sensor(0x6218,   0xF9F0);
	  write_cmos_sensor(0x38FA,   0x0030);
	  write_cmos_sensor(0x38FC,   0x0030);
	  write_cmos_sensor(0x32CE,   0x0060);
	  write_cmos_sensor(0x32D0,   0x0024);
	  write_cmos_sensor_8(0x0114,   0x03	  );
	  write_cmos_sensor(0x030E,   0x00A5);
	  write_cmos_sensor(0x0342,   0x0E68);
	  write_cmos_sensor(0x0340,   0x09E2);
	  write_cmos_sensor(0x0200,   0x0618);
	  write_cmos_sensor(0x0202,   0x09C2);
	  write_cmos_sensor(0x0204,   0x0020);
	  write_cmos_sensor_8(0x3011,   0x02	  );
	  write_cmos_sensor_8(0x0900,   0x01	);
	  write_cmos_sensor_8(0x0901,   0x12	  );
	  write_cmos_sensor(0x034C,   0x0662);
	  write_cmos_sensor(0x034E,   0x04C8);
	  write_cmos_sensor(0x6028,   0xD000);
	  write_cmos_sensor(0x602A,   0x012A);
	  write_cmos_sensor(0x6F12,   0x0040);
	  write_cmos_sensor(0x6F12,   0x7077);
	  write_cmos_sensor(0x6F12,   0x7777);

   #endif

	  mdelay(2);
	  LOG_INF("exit init\n");

}	/*	sensor_init  */


static void preview_setting(void)
{
	//1600x1200
	LOG_INF("preview seting enter,I'm the new one rrr1 \n");

			LOG_INF("preview seting enter,I'm the new one rrrr2\n");
		//	return 0;
	write_cmos_sensor(0x0100,   0x0000);
	mdelay(10);

	write_cmos_sensor_8(0x0105,	0x04 		); // mask interrupt
	write_cmos_sensor_8(0x0104,	0x01    );
	write_cmos_sensor_8(0x0114,	0x03	  ); //3
	write_cmos_sensor(0x030E,	0x00A5	);
	write_cmos_sensor(0x0342,	0x0E68	);
	write_cmos_sensor(0x0340,	0x09E2	);
	write_cmos_sensor(0x0200,	0x0618	);
	write_cmos_sensor(0x0202,	0x09C2	);
	write_cmos_sensor(0x0204,	0x0020	);
	write_cmos_sensor_8(0x3011,	0x02	  );
	write_cmos_sensor_8(0x0900,	0x01    );
	write_cmos_sensor_8(0x0901,	0x12    );
	write_cmos_sensor(0x0346,	0x0004	);
	write_cmos_sensor(0x034A,	0x0993	);
	write_cmos_sensor(0x034C,	0x0662	);
	write_cmos_sensor(0x034E,	0x04C8	);
	write_cmos_sensor(0x6004,	0x0000  );
	write_cmos_sensor(0x6028,	0xD000  );
	write_cmos_sensor(0x602A,	0x012A  );
	write_cmos_sensor(0x6F12,	0x0040	);
	write_cmos_sensor(0x6F12,	0x7077	);
	write_cmos_sensor(0x6F12,	0x7777	);
	write_cmos_sensor_8(0x0104,	0x00    );
	mdelay(10);
	write_cmos_sensor(0x0100,   0x0100);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.line_length = imgsensor_info.pre.linelength;
	spin_unlock(&imgsensor_drv_lock);
        mdelay(50);

}	/*	preview_setting  */


static void normal_capture_setting(void)
{
	LOG_INF("capture setting enter\n");
//return 0;
	write_cmos_sensor(0x0100,   0x0000);
	mdelay(10);
	write_cmos_sensor_8(0x0105,	0x04   );
	write_cmos_sensor_8(0x0104,	0x01   );
	write_cmos_sensor_8(0x0114,	0x03	 );
	write_cmos_sensor(0x030E,	0x00A5 );
	write_cmos_sensor(0x0342,	0x0E68 );
	write_cmos_sensor(0x0340,	0x09E2 );
	write_cmos_sensor(0x0200,	0x0618 );
	write_cmos_sensor(0x0202,	0x09C2 );
	write_cmos_sensor(0x0204,	0x0020 );
	write_cmos_sensor(0x0346,	0x0004 );
	write_cmos_sensor(0x034A,	0x0993 );
	write_cmos_sensor(0x034C,	0x0CC0 );
	write_cmos_sensor(0x034E,	0x0990 );
	write_cmos_sensor(0x0900,	0x0011 );
	write_cmos_sensor(0x0901,	0x0011 );
	write_cmos_sensor(0x3011,	0x0001 );
	write_cmos_sensor(0x6004,	0x0000 );
	write_cmos_sensor(0x6028,	0xD000 );
	write_cmos_sensor(0x602A,	0x012A );
	write_cmos_sensor(0x6F12,	0x0040 );
	write_cmos_sensor(0x6F12,	0x7077 );
	write_cmos_sensor(0x6F12,	0x7777 );
	write_cmos_sensor_8(0x0104,	0x00    );
	mdelay(10);
	write_cmos_sensor(0x0100,   0x0100);
	//write_cmos_sensor_8(0x0104, 0x00   );
	spin_lock(&imgsensor_drv_lock);
	imgsensor.line_length = imgsensor_info.cap.linelength;
	spin_unlock(&imgsensor_drv_lock);
		mdelay(2);





}
#if 0
static void pip_capture_setting(void)
{
	normal_capture_setting();
}
#endif                         

static void capture_setting(kal_uint16 currefps)
{
	//capture_20fps();

		normal_capture_setting();

}

static void normal_video_setting(kal_uint16 currefps)
{
	   LOG_INF(" normal video setting,use capture setting,now grab window is %d * %d",imgsensor_info.normal_video.grabwindow_width,imgsensor_info.normal_video.grabwindow_height);
	#if 0

	    write_cmos_sensor(0x0100,   0x0000);
	    mdelay(10);
		write_cmos_sensor_8(0x0105,	0x04);
		write_cmos_sensor_8(0x0104,	0x01);
		write_cmos_sensor_8(0x0114,	  0x03);
		write_cmos_sensor(0x030E,	0x00A5);
		write_cmos_sensor(0x0342,	0x0E68);
		write_cmos_sensor(0x0340,	0x09E2);
		write_cmos_sensor(0x0200,	0x0618);
		write_cmos_sensor(0x0202,	0x09C2);
		write_cmos_sensor(0x0204,	0x0020);
		write_cmos_sensor(0x0346,	0x0136);
		write_cmos_sensor(0x034A,	0x0861);
		write_cmos_sensor(0x034C,	0x0CC0);
		write_cmos_sensor(0x034E,	0x072C);
		write_cmos_sensor(0x0900,	0x0011);
		write_cmos_sensor(0x0901,	0x0011);
		write_cmos_sensor(0x3011,	0x0001);
		write_cmos_sensor(0x6004,	0x0000);
		write_cmos_sensor(0x6028,	0xD000);
		write_cmos_sensor(0x602A,	0x012A);
		write_cmos_sensor(0x6F12,	0x0040);
		write_cmos_sensor(0x6F12,	0x7077);
		write_cmos_sensor(0x6F12,	0x7777);
		write_cmos_sensor_8(0x0104, 0x00	);
		write_cmos_sensor(0x0100,   0x0100);
	#endif
   // use capture setting
normal_capture_setting();




}
static void hs_video_setting(void)
{
    LOG_INF("enter hs_video setting\n");
	write_cmos_sensor(0x0100, 0x00);
	mdelay(10);
	write_cmos_sensor(0x6218, 0xF1D0);
	write_cmos_sensor(0x6214, 0xF9F0);
	write_cmos_sensor(0x6226, 0x0001);
	write_cmos_sensor(0xB0C0, 0x000C);
	write_cmos_sensor(0x6226, 0x0000);
	write_cmos_sensor(0x6218, 0xF9F0);
	write_cmos_sensor_8(0x0114,	0x03);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0002);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x008C);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A,0x0001);
	write_cmos_sensor(0x030C, 0x0006);
	write_cmos_sensor(0x030E, 0x00AF);
	write_cmos_sensor(0x0342, 0x0DE8);
	write_cmos_sensor(0x0340, 0x028E);
	write_cmos_sensor(0x0200, 0x0618);
	write_cmos_sensor(0x0202, 0x0002);
	write_cmos_sensor(0x0204, 0x0020);
	write_cmos_sensor_8(0x3011,	0x04);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0386, 0x0001);
	write_cmos_sensor_8(0x0900,	0x01);
	write_cmos_sensor_8(0x0901,	0x14);
	write_cmos_sensor(0x0344, 0x0000);
	write_cmos_sensor(0x0348, 0x0CC7);
	write_cmos_sensor(0x0346, 0x0004);
	write_cmos_sensor(0x034A, 0x0993);
	write_cmos_sensor(0x034C, 0x0330);
	write_cmos_sensor(0x034E, 0x0264);
	write_cmos_sensor(0x0400, 0x0002);
	write_cmos_sensor(0x0404, 0x0010);
	mdelay(10);
	write_cmos_sensor_8(0x0100,	0x01);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
    spin_unlock(&imgsensor_drv_lock);
    mdelay(2);






}

static void slim_video_setting(void)
{
    LOG_INF("enter slim_video setting\n");
	write_cmos_sensor(0x0100, 0x00);
	mdelay(10);
	write_cmos_sensor(0x6218, 0xF1D0);
	write_cmos_sensor(0x6214, 	0xF9F0);
	write_cmos_sensor(0x6226, 	0x0001);
	write_cmos_sensor(0xB0C0, 	0x000C);
	write_cmos_sensor(0x6226, 	0x0000);
	write_cmos_sensor(0x6218, 	0xF9F0);
	write_cmos_sensor_8(0x0114,		0x03);
	write_cmos_sensor(0x0136, 	0x1800);
	write_cmos_sensor(0x0300	,	0x0002);
	write_cmos_sensor(0x0302, 	0x0001);
	write_cmos_sensor(0x0304, 	0x0006);
	write_cmos_sensor(0x0306, 	0x008C);
	write_cmos_sensor(0x0308, 	0x0008);
	write_cmos_sensor(0x030A, 	0x0001);
	write_cmos_sensor(0x030C, 	0x0006);
	write_cmos_sensor(0x030E, 	0x00A5);
	write_cmos_sensor(0x0342, 	0x0C10);
	write_cmos_sensor(0x0340, 	0x02FE);
	write_cmos_sensor(0x0200, 	0x0618);
	write_cmos_sensor(0x0202, 	0x0002);
	write_cmos_sensor(0x0204, 	0x0020);
	write_cmos_sensor_8(0x3011,		0x01);
	write_cmos_sensor(0x0382, 	0x0003);
	write_cmos_sensor(0x0386, 	0x0003);
	write_cmos_sensor_8(0x0900,		0x01);
	write_cmos_sensor_8(0x0901,		0x22);
	write_cmos_sensor(0x0344, 	0x0164);
	write_cmos_sensor(0x0348, 	0x0B63);
	write_cmos_sensor(0x0346, 	0x01FC);
	write_cmos_sensor(0x034A, 	0x079B);
	write_cmos_sensor(0x034C, 	0x0500);
	write_cmos_sensor(0x034E, 	0x02D0);
	write_cmos_sensor(0x0400, 	0x0002);
	write_cmos_sensor(0x0404, 	0x0010);
	mdelay(10);
	write_cmos_sensor_8(0x0100,		0x01);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
  spin_unlock(&imgsensor_drv_lock);
  mdelay(2);


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
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail write id %x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		// if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF
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
	//const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;
	LOG_1;
	LOG_2;
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail wirte id %x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
			retry--;
		} while(retry > 0);
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

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = KAL_FALSE;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

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
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/

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
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
	preview_setting();
	set_mirror_flip(IMAGE_HV_MIRROR);
	LOG_INF("Currently camera mode is %d, framelength=%d,linelength=%d\n",imgsensor.sensor_mode,imgsensor.frame_length,imgsensor.line_length);
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
	LOG_INF("E");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;

    if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) // 30fps
    {
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else //PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
    {
		//if (imgsensor.current_fps != imgsensor_info.cap1.max_framerate)
		//	LOG_INF("Warning: current_fps %x fps is not support, so use cap1's setting: %x fps!\n",imgsensor_info.cap1.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}

	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("Caputre fps:%d\n",imgsensor.current_fps);
	set_dummy();
	capture_setting(imgsensor.current_fps);
    set_mirror_flip(IMAGE_HV_MIRROR);
	LOG_INF("Currently camera mode is %d, framelength=%d,linelength=%d\n",imgsensor.sensor_mode,imgsensor.frame_length,imgsensor.line_length);
	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
	normal_video_setting(imgsensor.current_fps);
	set_mirror_flip(IMAGE_HV_MIRROR);
	LOG_INF("Currently camera mode is %d, framelength=%d,linelength=%d\n",imgsensor.sensor_mode,imgsensor.frame_length,imgsensor.line_length);
	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("hs_video enter i ");

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
	set_dummy();
	hs_video_setting();
	set_mirror_flip(IMAGE_HV_MIRROR);
	LOG_INF("Currently camera mode is %d,framerate is %d , framelength=%d,linelength=%d\n",imgsensor.sensor_mode,imgsensor.current_fps,imgsensor.frame_length,imgsensor.line_length);
	return ERROR_NONE;
}	/*	hs_video   */


static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("slim video enter in");

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
	set_dummy();
	slim_video_setting();
	set_mirror_flip(IMAGE_HV_MIRROR);
	LOG_INF("Currently camera mode is %d,framerate is %d , framelength=%d,linelength=%d\n",imgsensor.sensor_mode,imgsensor.current_fps,imgsensor.frame_length,imgsensor.line_length);
	return ERROR_NONE;
}	/*	slim_video	 */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF(" get resolution, now the mode is%d",imgsensor.sensor_mode);
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;

	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d", scenario_id);


	//sensor_info->SensorVideoFrameRate = imgsensor_info.normal_video.max_framerate/10; /* not use */
	//sensor_info->SensorStillCaptureFrameRate= imgsensor_info.cap.max_framerate/10; /* not use */
	//imgsensor_info->SensorWebCamCaptureFrameRate= imgsensor_info.v.max_framerate; /* not use */

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

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
	sensor_info->SensorHightSampling = 0;	// 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

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
		default:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d", scenario_id);
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
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{

	LOG_INF("framerate = %d\n ", framerate);
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
	set_max_framerate(imgsensor.current_fps,1);

	return ERROR_NONE;


}


static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
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


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			if(framerate==300)
			{
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
			else
			{
			frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			LOG_INF("scenario hs_video:Currently camera mode is %d,framerate is %d , framelength=%d,linelength=%d\n",imgsensor.sensor_mode,imgsensor.current_fps,imgsensor.frame_length,imgsensor.line_length);
			//set_dummy();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			break;
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			//set_dummy();
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			break;
	}
	return ERROR_NONE;

}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
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

static kal_uint32 set_test_pattern_mode(kal_bool enable){
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
        write_cmos_sensor(0x0600, 0x0002);
	} else {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
        write_cmos_sensor(0x0600, 0x0000);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;

}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
							 UINT8 *feature_para,UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16=(UINT16 *) feature_para;
	UINT16 *feature_data_16=(UINT16 *) feature_para;
	UINT32 *feature_return_para_32=(UINT32 *) feature_para;
	UINT32 *feature_data_32=(UINT32 *) feature_para;

	 unsigned long long *feature_data=(unsigned long long *) feature_para;
     //unsigned long long *feature_return_para=(unsigned long long *) feature_para;

	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("feature_id = %d", feature_id);
	switch (feature_id) {
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			write_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
		     //night_mode((BOOL) *feature_data);
			break;
		case SENSOR_FEATURE_SET_GAIN:
			set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			if((sensor_reg_data->RegData>>8)>0)
			   write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			else
				write_cmos_sensor_8(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*feature_return_para_32=LENS_DRIVER_ID_DO_NOT_CARE;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
			set_video_mode(*feature_data);
			break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			get_imgsensor_id(feature_return_para_32);
			break;
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			set_max_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_FRAMERATE:
			spin_lock(&imgsensor_drv_lock);
			imgsensor.current_fps = *feature_data_32;
			spin_unlock(&imgsensor_drv_lock);
			LOG_INF("current fps :%d\n", imgsensor.current_fps);
			break;
		case SENSOR_FEATURE_SET_HDR:
			//LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
			LOG_INF("Warning! Not Support IHDR Feature");
			spin_lock(&imgsensor_drv_lock);
			//imgsensor.ihdr_en = (BOOL)*feature_data;
            imgsensor.ihdr_en = KAL_FALSE;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:
			LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data_32);
			wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(SENSOR_WINSIZE_INFO_STRUCT));
					break;
			}
			break;
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
			     LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));

			            ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));

			break;
		default:
			break;
	}

	return ERROR_NONE;
}	/*	feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K3H7Y_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
printk("Jeff,enter S5K3H7YX_MIPI_RAW_SensorInit\n");
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	s5k2p8_MIPI_RAW_SensorInit	*/
