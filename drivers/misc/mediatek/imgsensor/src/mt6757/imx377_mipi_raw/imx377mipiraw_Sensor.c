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
 *	 IMX377mipi_Sensor.c
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

#include <linux/types.h>
#include <linux/io.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx377mipiraw_Sensor.h"

#define PFX "IMX377_camera_sensor"
//#define LOG_WRN(format, args...) xlog_printk(ANDROID_LOG_WARN ,PFX, "[%S] " format, __FUNCTION__, ##args)
//#defineLOG_INF(format, args...) xlog_printk(ANDROID_LOG_INFO ,PFX, "[%s] " format, __FUNCTION__, ##args)
//#define LOG_DBG(format, args...) xlog_printk(ANDROID_LOG_DEBUG ,PFX, "[%S] " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static imgsensor_info_struct imgsensor_info = {
	.sensor_id = IMX377_SENSOR_ID,

	.checksum_value =0x539e456e,

	.pre = {
        .pclk = 72000000,              //record different mode's pclk
		.linelength = 750,				//record different mode's linelength
              .framelength = 3200,         //record different mode's framelength
		.startx = 38,					//record different mode's startx of grabwindow
		.starty = 14,					//record different mode's starty of grabwindow
		.grabwindow_width = 2000,		//record different mode's width of grabwindow
		.grabwindow_height = 1500,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
	},
	.cap = {
		.pclk =72000000,
		.linelength = 750,
		.framelength = 3200,
		.startx =76,
		.starty =40,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	.cap1 = {
		.pclk =72000000,
		.linelength = 750,
		.framelength = 4000,
		.startx = 74,
		.starty = 40,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 240,
	},
        .cap2 = {
              .pclk =72000000,
              .linelength = 1280,
              .framelength = 3750,
              .startx = 74,
              .starty =40,
              .grabwindow_width = 4000,
              .grabwindow_height = 3000,
              .mipi_data_lp2hs_settle_dc = 85,
              .max_framerate = 150,
	},
	.normal_video = {
		.pclk =72000000,
		.linelength = 750,
		.framelength = 3200,
		.startx =74,
		.starty =40,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 72000000,
		.linelength = 264,
		.framelength =2275,
		.startx = 26,
		.starty = 14,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 72000000,
		.linelength = 300,
		.framelength = 1000,
		.startx = 24,
		.starty = 14,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 2400,
	},
	.margin = 0,
	.min_shutter = 1,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame =1,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_2MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = 0, //0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x34,0xff},
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x3D0,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_mode = 0, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x34,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{{ 4104  , 3062,  0,  0, 4104, 3048, 2052,  1524, 0000, 0000, 2052,  1524, 26,   12, 2000, 1500}, // Preview
{  4104  , 3062,  0,  0, 4104, 3048, 4104,  3048, 0000, 0000, 4104,  3048, 52,   24, 4000, 3000}, // capture
{  4104  , 3062,  0,  0, 4104, 3048, 4104,  3048, 0000, 0000, 4104,  3048, 52,   24, 4000, 3000}, // video
{  4104  , 3062,  0,  0, 4104, 3048, 2052,  1524, 0000, 0000, 2052,  1524, 386, 402, 1280,  720}, // hight speed video 120fps
{  4104  , 3062,  0,  0, 4104, 3048, 2052,  1524, 0000, 0000, 2052,  1524, 386, 402, 1280,  720}};// hight speed video 240fps


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = 0x%x, dummypixels = 0x%x \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	//return; //for test
       write_cmos_sensor(0x302D, 1);
	write_cmos_sensor(0x30F6,imgsensor.line_length >> 8);//750
       write_cmos_sensor(0x30F5,imgsensor.line_length & 0xFF);
       write_cmos_sensor(0x30F8,imgsensor.frame_length >> 8);//3200
       write_cmos_sensor(0x30F7,imgsensor.frame_length & 0xFF);
       write_cmos_sensor(0x302D, 0);

}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
//	kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	LOG_INF("framerate = %d, min framelength should enable = %d\n", framerate,min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	if (frame_length >= imgsensor.min_frame_length)
		imgsensor.frame_length = frame_length;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
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

	kal_uint16 realtime_fps = 0;
//	kal_uint16 SVR,SHR,shutter1;
//	kal_uint32 frame_length = 0;
		kal_uint16 SHR;
 #if 1
       //shutter=5000;//for test
       //return; //for test

	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	if (shutter < imgsensor_info.min_shutter) shutter = imgsensor_info.min_shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
		else {
		// Extend frame length
		write_cmos_sensor(0x30F8, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x30F7, imgsensor.frame_length & 0xFF);
	    }
	} else {
		// Extend frame length
		write_cmos_sensor(0x30F8, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x30F7, imgsensor.frame_length & 0xFF);
	}
      LOG_INF("shutter1=%d, framelength =%d\n", shutter,imgsensor.frame_length);

	  SHR=imgsensor.frame_length-shutter;

	  if(SHR<8)
	  	SHR=8;
	  else if(SHR>(imgsensor.frame_length-6))
	  	SHR=imgsensor.frame_length-6;

             write_cmos_sensor(0x300C, (SHR >> 8) & 0xFF);
             write_cmos_sensor(0x300B, SHR  & 0xFF);
	LOG_INF("shutter2=%d, framelength =%d\n", SHR,imgsensor.frame_length);
#else
	// Update Shutter
	LOG_INF("shutter1=%d\n", shutter);

	     shutter1=shutter%(imgsensor.frame_length);
	     SVR=shutter/(imgsensor.frame_length);
            SHR=imgsensor.frame_length-shutter1;

	   if(SHR<8)
	   	SHR=8;
	   else if(SHR>((SVR+1)*((imgsensor.frame_length)-6)))
	   	SHR=((SVR+1)*((imgsensor.frame_length)-6));

	      write_cmos_sensor(0x302D, 1);
             write_cmos_sensor(0x300C, (SHR >> 8) & 0xFF);
             write_cmos_sensor(0x300B, SHR  & 0xFF);
	      write_cmos_sensor(0x300E, (SVR >> 8) & 0xFF);
             write_cmos_sensor(0x300D, SVR  & 0xFF);
             //write_cmos_sensor(0x302D, 0);
	LOG_INF("SVR =%d, SHR=%d\n", SVR,SHR);

	//LOG_INF("frame_length = %d ", frame_length);
#endif
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
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}	/*	set_shutter */


#define imx377_MIPI_table_write_cmos_sensor imx377_MIPI_table_write_cmos_sensor_multi
#define I2C_BUFFER_LEN 240
 static kal_uint16 imx377_MIPI_table_write_cmos_sensor_multi(kal_uint16* para, kal_uint32 len, bool HW_trig)
 {
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	tosend = 0;
	IDX = 0;
	//LOG_INF("enter imx377_MIPI_table_write_cmos_sensor_multi len  %d\n",len);

	while(IDX < len)
	{
	   addr = para[IDX];

		{

			puSendCmd[tosend++] = (char)(addr >> 8);
			puSendCmd[tosend++] = (char)(addr & 0xFF);
			data = para[IDX+1];
			puSendCmd[tosend++] = (char)(data & 0xFF);
			//LOG_INF("addr 0x%x, data 0x%x\n",addr,data);
			//LOG_INF("puSendCmd[0] 0%x, puSendCmd[1] 0%x, puSendCmd[2] 0%x\n",puSendCmd[tosend-3],puSendCmd[tosend-2],puSendCmd[tosend-1]);

			IDX += 2;
			addr_last = addr;

		}

		if (tosend >= I2C_BUFFER_LEN || IDX == len || addr != addr_last)
		{
			//LOG_INF("IDX %d,tosend %d addr_last 0x%x,addr 0x%x\n",IDX, tosend, addr_last, addr);
			if(HW_trig)
				iBurstWriteReg_HW(puSendCmd , tosend, imgsensor.i2c_write_id, 3, 0);
			else
				iBurstWriteReg_multi(puSendCmd , tosend, imgsensor.i2c_write_id, 3, 0);
			tosend = 0;
		}
	}
	//LOG_INF("exit imx377_MIPI_table_write_cmos_sensor_multi\n");

	return 0;
 }
static void write_shutter_buf_mode(kal_uint16 shutter)
{
	kal_uint16 SHR;
	static kal_uint16 addr_data_pair_shutter[] =
	{
		0x30F8,0x00,
		0x30F7,0x00,
		0x300C,0x00,
		0x300B,0x00,
	};

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
	// Extend frame length
	addr_data_pair_shutter[1] = imgsensor.frame_length >> 8;
	addr_data_pair_shutter[3] = imgsensor.frame_length & 0xFF;

	SHR=imgsensor.frame_length-shutter;

	if(SHR<8)
		SHR=8;
	else if(SHR>(imgsensor.frame_length-6))
		SHR=imgsensor.frame_length-6;

	addr_data_pair_shutter[5] =  (SHR >> 8) & 0xFF;
	addr_data_pair_shutter[7] =  SHR  & 0xFF;
	imx377_MIPI_table_write_cmos_sensor(addr_data_pair_shutter, sizeof(addr_data_pair_shutter)/sizeof(kal_uint16),true);
#if 0
	for(i=0;i<(sizeof(addr_data_pair_shutter)/sizeof(kal_uint16));i+=2){
	//	LOG_INF("read_cmos_sensor(0x%x) 	0x%x\n",addr_data_pair_shutter[i],read_cmos_sensor(addr_data_pair_shutter[i]));
	}
#endif

	LOG_INF("exit buf mode SHR =%d,shutter =%d, framelength =%d\n", SHR, shutter, imgsensor.frame_length);

}	/*	write_shutter  */

static void set_shutter_buf_mode(kal_uint16 shutter)
{
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter_buf_mode(shutter);
}	/*	set_shutter */



static kal_uint16 gain2reg(const kal_uint16 gain)
{
     return (2048-(2048*64/gain));
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

    //gain=1024;//for test
    //return; //for test

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
    LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

    write_cmos_sensor(0x300A, (reg_gain>>8)& 0xFF);
    write_cmos_sensor(0x3009, reg_gain & 0xFF);

    return gain;
}	/*	set_gain  */
static kal_uint16 set_gain_buf_mode(kal_uint16 gain)
{
    kal_uint16 reg_gain;
	static kal_uint16 addr_data_pair_gain[] =
	{
		0x300A,0x00,
		0x3009,0x00,
	};

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
    LOG_INF("buf mode gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

    addr_data_pair_gain[1]= (reg_gain>>8)& 0xFF;
    addr_data_pair_gain[3]= reg_gain & 0xFF;
	imx377_MIPI_table_write_cmos_sensor(addr_data_pair_gain, sizeof(addr_data_pair_gain)/sizeof(kal_uint16),true);
#if 0
	for(i=0;i<(sizeof(addr_data_pair_gain)/sizeof(kal_uint16));i+=2){
		LOG_INF("read_cmos_sensor(0x%x) 	0x%x\n",addr_data_pair_gain[i],read_cmos_sensor(addr_data_pair_gain[i]));
	}
#endif

    return gain;
}	/*	set_gain  */
#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

//	kal_uint8 itemp;

	itemp=read_cmos_sensor(0x301A);
	itemp &= ~0x01;

	switch(image_mirror)
		{

		   case IMAGE_NORMAL:
		   	     //write_cmos_sensor(0x0101, itemp);
			      break;

		   case IMAGE_V_MIRROR:
			     write_cmos_sensor(0x301A, itemp | 0x01);
			     break;

		   case IMAGE_H_MIRROR:
			     //write_cmos_sensor(0x0101, itemp | 0x01);
			     break;

		   case IMAGE_HV_MIRROR:
			    // write_cmos_sensor(0x0101, itemp | 0x03);
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

static void sensor_init(void)
{
	LOG_INF("E\n");

   //Setting 0
    write_cmos_sensor(0x3120,0xC0);
    write_cmos_sensor(0x3121,0x00);
    write_cmos_sensor(0x3122,0x02);
    write_cmos_sensor(0x3123,0x00);
    write_cmos_sensor(0x3124,0x00);
    write_cmos_sensor(0x3125,0x01);
    write_cmos_sensor(0x3127,0x02);
    write_cmos_sensor(0x3129,0x90);
    write_cmos_sensor(0x312A,0x02);
    write_cmos_sensor(0x312D,0x02);
    //Setting 1
    write_cmos_sensor(0x3000,0x16);
    write_cmos_sensor(0x3018,0xA2);
    write_cmos_sensor(0x3399,0x01);
    write_cmos_sensor(0x310B,0x11);
    write_cmos_sensor(0x3125,0x01);
    write_cmos_sensor(0x3A56,0x00);
    write_cmos_sensor(0x3045,0x32);
    write_cmos_sensor(0x3003,0x20);
    write_cmos_sensor(0x304E,0x02);
    write_cmos_sensor(0x3057,0x4A);
    write_cmos_sensor(0x3058,0xF6);
    write_cmos_sensor(0x3059,0x00);
    write_cmos_sensor(0x306B,0x04);
    write_cmos_sensor(0x3145,0x00);
    write_cmos_sensor(0x3202,0x63);
    write_cmos_sensor(0x3203,0x00);
    write_cmos_sensor(0x3236,0x64);
    write_cmos_sensor(0x3237,0x00);
    write_cmos_sensor(0x3304,0x0B);
    write_cmos_sensor(0x3305,0x00);
    write_cmos_sensor(0x3306,0x0B);
    write_cmos_sensor(0x3307,0x00);
    write_cmos_sensor(0x337F,0x64);
    write_cmos_sensor(0x3380,0x00);
    write_cmos_sensor(0x338D,0x64);
    write_cmos_sensor(0x338E,0x00);
    write_cmos_sensor(0x3510,0x72);
    write_cmos_sensor(0x3511,0x00);
    write_cmos_sensor(0x3528,0x0F);
	write_cmos_sensor(0x3529,0x0F);
	write_cmos_sensor(0x352A,0x0F);
	write_cmos_sensor(0x352B,0x0F);
	write_cmos_sensor(0x3538,0x0F);
    write_cmos_sensor(0x3539,0x13);
    write_cmos_sensor(0x353C,0x01);
    write_cmos_sensor(0x3553,0x00);
	write_cmos_sensor(0x3554,0x00);
	write_cmos_sensor(0x3555,0x00);
	write_cmos_sensor(0x3556,0x00);
	write_cmos_sensor(0x3557,0x00);
	write_cmos_sensor(0x3558,0x00);
	write_cmos_sensor(0x3559,0x00);
	write_cmos_sensor(0x355A,0x00);
    write_cmos_sensor(0x357D,0x07);
    write_cmos_sensor(0x357F,0x07);
    write_cmos_sensor(0x3580,0x04);
    write_cmos_sensor(0x3583,0x60);
    write_cmos_sensor(0x3587,0x01);
    write_cmos_sensor(0x3590,0x0B);
    write_cmos_sensor(0x3591,0x00);
    write_cmos_sensor(0x35BA,0x0F);
    write_cmos_sensor(0x366A,0x0C);
    write_cmos_sensor(0x366B,0x0B);
    write_cmos_sensor(0x366C,0x07);
    write_cmos_sensor(0x366D,0x00);
    write_cmos_sensor(0x366E,0x00);
    write_cmos_sensor(0x366F,0x00);
    write_cmos_sensor(0x3670,0x00);
    write_cmos_sensor(0x3671,0x00);
    write_cmos_sensor(0x3672,0x00);
    write_cmos_sensor(0x3673,0x00);
    write_cmos_sensor(0x3674,0xDF);
    write_cmos_sensor(0x3675,0x00);
    write_cmos_sensor(0x3676,0xA7);
    write_cmos_sensor(0x3677,0x01);
    write_cmos_sensor(0x3687,0x00);
    write_cmos_sensor(0x375C,0x02);
    write_cmos_sensor(0x380A,0x0A);
    write_cmos_sensor(0x382B,0x16);

	//Setting 2
    write_cmos_sensor(0x310B,0x00);
	//Setting 3
    write_cmos_sensor(0x3000,0x04);
    write_cmos_sensor(0x3001,0x10);
    write_cmos_sensor(0x30F4,0x00);

}	/*	sensor_init  */


static void preview_setting(void)
{
	//Preview 2104*1560 30fps 24M MCLK 4lane 608Mbps/lane
	// preview 30.01fps
write_cmos_sensor(0x3004,0xA2);
write_cmos_sensor(0x3005,0x21);
write_cmos_sensor(0x3006,0x00);
write_cmos_sensor(0x3007,0xA0);
write_cmos_sensor(0x300D,0x00);
write_cmos_sensor(0x300E,0x00);
write_cmos_sensor(0x301A,0x00);
write_cmos_sensor(0x3039,0x00);
write_cmos_sensor(0x303A,0x00);
write_cmos_sensor(0x303E,0x00);
write_cmos_sensor(0x303F,0x00);
write_cmos_sensor(0x3040,0x00);
write_cmos_sensor(0x3068,0x00);
write_cmos_sensor(0x307E,0x00);
write_cmos_sensor(0x307F,0x00);
write_cmos_sensor(0x3080,0x00);
write_cmos_sensor(0x3081,0x00);
write_cmos_sensor(0x3082,0x00);
write_cmos_sensor(0x3083,0x00);
write_cmos_sensor(0x3084,0x00);
write_cmos_sensor(0x3085,0x00);
write_cmos_sensor(0x3086,0x00);
write_cmos_sensor(0x3087,0x00);
write_cmos_sensor(0x3095,0x00);
write_cmos_sensor(0x3096,0x00);
write_cmos_sensor(0x3097,0x00);
write_cmos_sensor(0x3098,0x00);
write_cmos_sensor(0x3099,0x00);
write_cmos_sensor(0x309A,0x00);
write_cmos_sensor(0x309B,0x00);
write_cmos_sensor(0x309C,0x00);
write_cmos_sensor(0x30BC,0x00);
write_cmos_sensor(0x30BD,0x00);
write_cmos_sensor(0x30BE,0x00);
write_cmos_sensor(0x30BF,0x00);
write_cmos_sensor(0x30C0,0x00);
write_cmos_sensor(0x30C1,0x00);
write_cmos_sensor(0x30C2,0x00);
write_cmos_sensor(0x30C3,0x00);
write_cmos_sensor(0x30C4,0x00);
write_cmos_sensor(0x30C5,0x00);
write_cmos_sensor(0x30C6,0x00);
write_cmos_sensor(0x30C7,0x00);
write_cmos_sensor(0x30C8,0x00);
write_cmos_sensor(0x30C9,0x00);
write_cmos_sensor(0x30CA,0x00);
write_cmos_sensor(0x30CB,0x00);
write_cmos_sensor(0x30CC,0x00);
write_cmos_sensor(0x30D0,0x08);
write_cmos_sensor(0x30D1,0x10);
write_cmos_sensor(0x30D5,0x00);
write_cmos_sensor(0x30D6,0x00);
write_cmos_sensor(0x30D7,0x00);
write_cmos_sensor(0x30D8,0x00);
write_cmos_sensor(0x30D9,0x00);
write_cmos_sensor(0x30DA,0x02);
write_cmos_sensor(0x30F5,0xEE);
write_cmos_sensor(0x30F6,0x02);//750
write_cmos_sensor(0x30F7,0x80);
write_cmos_sensor(0x30F8,0x0C);//3200
write_cmos_sensor(0x30F9,0x00);
write_cmos_sensor(0x312F,0xF4);
write_cmos_sensor(0x3130,0x05);
write_cmos_sensor(0x3131,0xF0);
write_cmos_sensor(0x3132,0x05);
write_cmos_sensor(0x3A41,0x04);
write_cmos_sensor(0x312E,0x03);
write_cmos_sensor(0x303D,0x03);
write_cmos_sensor(0x3AC4,0x00);
write_cmos_sensor(0x3123,0x00);
write_cmos_sensor(0x3000,0x04);

}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
    // full size 29.76fps
    // capture setting 4208*3120  480MCLK 1.2Gp/lane
if(currefps==300){

write_cmos_sensor(0x3004,0x00);
write_cmos_sensor(0x3005,0x01);
write_cmos_sensor(0x3006,0x00);
write_cmos_sensor(0x3007,0xA0);
write_cmos_sensor(0x300D,0x00);
write_cmos_sensor(0x300E,0x00);
write_cmos_sensor(0x301A,0x00);
write_cmos_sensor(0x3039,0x00);
write_cmos_sensor(0x303A,0x00);
write_cmos_sensor(0x303E,0x00);
write_cmos_sensor(0x303F,0x00);
write_cmos_sensor(0x3040,0x00);
write_cmos_sensor(0x3068,0x00);
write_cmos_sensor(0x307E,0x00);
write_cmos_sensor(0x307F,0x00);
write_cmos_sensor(0x3080,0x00);
write_cmos_sensor(0x3081,0x00);
write_cmos_sensor(0x3082,0x00);
write_cmos_sensor(0x3083,0x00);
write_cmos_sensor(0x3084,0x00);
write_cmos_sensor(0x3085,0x00);
write_cmos_sensor(0x3086,0x00);
write_cmos_sensor(0x3087,0x00);
write_cmos_sensor(0x3095,0x00);
write_cmos_sensor(0x3096,0x00);
write_cmos_sensor(0x3097,0x00);
write_cmos_sensor(0x3098,0x00);
write_cmos_sensor(0x3099,0x00);
write_cmos_sensor(0x309A,0x00);
write_cmos_sensor(0x309B,0x00);
write_cmos_sensor(0x309C,0x00);
write_cmos_sensor(0x30BC,0x00);
write_cmos_sensor(0x30BD,0x00);
write_cmos_sensor(0x30BE,0x00);
write_cmos_sensor(0x30BF,0x00);
write_cmos_sensor(0x30C0,0x00);
write_cmos_sensor(0x30C1,0x00);
write_cmos_sensor(0x30C2,0x00);
write_cmos_sensor(0x30C3,0x00);
write_cmos_sensor(0x30C4,0x00);
write_cmos_sensor(0x30C5,0x00);
write_cmos_sensor(0x30C6,0x00);
write_cmos_sensor(0x30C7,0x00);
write_cmos_sensor(0x30C8,0x00);
write_cmos_sensor(0x30C9,0x00);
write_cmos_sensor(0x30CA,0x00);
write_cmos_sensor(0x30CB,0x00);
write_cmos_sensor(0x30CC,0x00);
write_cmos_sensor(0x30D0,0x08);
write_cmos_sensor(0x30D1,0x10);
write_cmos_sensor(0x30D5,0x00);
write_cmos_sensor(0x30D6,0x00);
write_cmos_sensor(0x30D7,0x00);
write_cmos_sensor(0x30D8,0x00);
write_cmos_sensor(0x30D9,0x00);
write_cmos_sensor(0x30DA,0x00);
write_cmos_sensor(0x30F5,0xEE);
write_cmos_sensor(0x30F6,0x02);//750
write_cmos_sensor(0x30F7,0x80);
write_cmos_sensor(0x30F8,0x0C);//3200
write_cmos_sensor(0x30F9,0x00);
write_cmos_sensor(0x312F,0xF6);
write_cmos_sensor(0x3130,0x0B);
write_cmos_sensor(0x3131,0xE6);
write_cmos_sensor(0x3132,0x0B);
write_cmos_sensor(0x3A41,0x10);
write_cmos_sensor(0x312E,0x03);
write_cmos_sensor(0x303D,0x03);
write_cmos_sensor(0x3AC4,0x00);
write_cmos_sensor(0x3123,0x00);
write_cmos_sensor(0x3000,0x04);
mdelay(10);

}
else if(currefps==240){
write_cmos_sensor(0x3004,0x00);
write_cmos_sensor(0x3005,0x01);
write_cmos_sensor(0x3006,0x00);
write_cmos_sensor(0x3007,0xA0);
write_cmos_sensor(0x300D,0x00);
write_cmos_sensor(0x300E,0x00);
write_cmos_sensor(0x301A,0x00);
write_cmos_sensor(0x3039,0x00);
write_cmos_sensor(0x303A,0x00);
write_cmos_sensor(0x303E,0x00);
write_cmos_sensor(0x303F,0x00);
write_cmos_sensor(0x3040,0x00);
write_cmos_sensor(0x3068,0x00);
write_cmos_sensor(0x307E,0x00);
write_cmos_sensor(0x307F,0x00);
write_cmos_sensor(0x3080,0x00);
write_cmos_sensor(0x3081,0x00);
write_cmos_sensor(0x3082,0x00);
write_cmos_sensor(0x3083,0x00);
write_cmos_sensor(0x3084,0x00);
write_cmos_sensor(0x3085,0x00);
write_cmos_sensor(0x3086,0x00);
write_cmos_sensor(0x3087,0x00);
write_cmos_sensor(0x3095,0x00);
write_cmos_sensor(0x3096,0x00);
write_cmos_sensor(0x3097,0x00);
write_cmos_sensor(0x3098,0x00);
write_cmos_sensor(0x3099,0x00);
write_cmos_sensor(0x309A,0x00);
write_cmos_sensor(0x309B,0x00);
write_cmos_sensor(0x309C,0x00);
write_cmos_sensor(0x30BC,0x00);
write_cmos_sensor(0x30BD,0x00);
write_cmos_sensor(0x30BE,0x00);
write_cmos_sensor(0x30BF,0x00);
write_cmos_sensor(0x30C0,0x00);
write_cmos_sensor(0x30C1,0x00);
write_cmos_sensor(0x30C2,0x00);
write_cmos_sensor(0x30C3,0x00);
write_cmos_sensor(0x30C4,0x00);
write_cmos_sensor(0x30C5,0x00);
write_cmos_sensor(0x30C6,0x00);
write_cmos_sensor(0x30C7,0x00);
write_cmos_sensor(0x30C8,0x00);
write_cmos_sensor(0x30C9,0x00);
write_cmos_sensor(0x30CA,0x00);
write_cmos_sensor(0x30CB,0x00);
write_cmos_sensor(0x30CC,0x00);
write_cmos_sensor(0x30D0,0x08);
write_cmos_sensor(0x30D1,0x10);
write_cmos_sensor(0x30D5,0x00);
write_cmos_sensor(0x30D6,0x00);
write_cmos_sensor(0x30D7,0x00);
write_cmos_sensor(0x30D8,0x00);
write_cmos_sensor(0x30D9,0x00);
write_cmos_sensor(0x30DA,0x00);
write_cmos_sensor(0x30F5,0xEE);
write_cmos_sensor(0x30F6,0x02);//750
write_cmos_sensor(0x30F7,0xA0);
write_cmos_sensor(0x30F8,0x0F);//4000
write_cmos_sensor(0x30F9,0x00);
write_cmos_sensor(0x312F,0xF6);
write_cmos_sensor(0x3130,0x0B);
write_cmos_sensor(0x3131,0xE6);
write_cmos_sensor(0x3132,0x0B);
write_cmos_sensor(0x3A41,0x10);
write_cmos_sensor(0x312E,0x03);
write_cmos_sensor(0x303D,0x03);
write_cmos_sensor(0x3AC4,0x00);
write_cmos_sensor(0x3123,0x00);
write_cmos_sensor(0x3000,0x04);}
else{ //15fps

write_cmos_sensor(0x3004,0x00);
write_cmos_sensor(0x3005,0x01);
write_cmos_sensor(0x3006,0x00);
write_cmos_sensor(0x3007,0xA0);
write_cmos_sensor(0x300D,0x00);
write_cmos_sensor(0x300E,0x00);
write_cmos_sensor(0x301A,0x00);
write_cmos_sensor(0x3039,0x00);
write_cmos_sensor(0x303A,0x00);
write_cmos_sensor(0x303E,0x00);
write_cmos_sensor(0x303F,0x00);
write_cmos_sensor(0x3040,0x00);
write_cmos_sensor(0x3068,0x00);
write_cmos_sensor(0x307E,0x00);
write_cmos_sensor(0x307F,0x00);
write_cmos_sensor(0x3080,0x00);
write_cmos_sensor(0x3081,0x00);
write_cmos_sensor(0x3082,0x00);
write_cmos_sensor(0x3083,0x00);
write_cmos_sensor(0x3084,0x00);
write_cmos_sensor(0x3085,0x00);
write_cmos_sensor(0x3086,0x00);
write_cmos_sensor(0x3087,0x00);
write_cmos_sensor(0x3095,0x00);
write_cmos_sensor(0x3096,0x00);
write_cmos_sensor(0x3097,0x00);
write_cmos_sensor(0x3098,0x00);
write_cmos_sensor(0x3099,0x00);
write_cmos_sensor(0x309A,0x00);
write_cmos_sensor(0x309B,0x00);
write_cmos_sensor(0x309C,0x00);
write_cmos_sensor(0x30BC,0x00);
write_cmos_sensor(0x30BD,0x00);
write_cmos_sensor(0x30BE,0x00);
write_cmos_sensor(0x30BF,0x00);
write_cmos_sensor(0x30C0,0x00);
write_cmos_sensor(0x30C1,0x00);
write_cmos_sensor(0x30C2,0x00);
write_cmos_sensor(0x30C3,0x00);
write_cmos_sensor(0x30C4,0x00);
write_cmos_sensor(0x30C5,0x00);
write_cmos_sensor(0x30C6,0x00);
write_cmos_sensor(0x30C7,0x00);
write_cmos_sensor(0x30C8,0x00);
write_cmos_sensor(0x30C9,0x00);
write_cmos_sensor(0x30CA,0x00);
write_cmos_sensor(0x30CB,0x00);
write_cmos_sensor(0x30CC,0x00);
write_cmos_sensor(0x30D0,0x08);
write_cmos_sensor(0x30D1,0x10);
write_cmos_sensor(0x30D5,0x00);
write_cmos_sensor(0x30D6,0x00);
write_cmos_sensor(0x30D7,0x00);
write_cmos_sensor(0x30D8,0x00);
write_cmos_sensor(0x30D9,0x00);
write_cmos_sensor(0x30DA,0x00);
write_cmos_sensor(0x30F5,0x00);
write_cmos_sensor(0x30F6,0x05);//1280
write_cmos_sensor(0x30F7,0xA6);
write_cmos_sensor(0x30F8,0x0E);//3750
write_cmos_sensor(0x30F9,0x00);
write_cmos_sensor(0x312F,0xF6);
write_cmos_sensor(0x3130,0x0B);
write_cmos_sensor(0x3131,0xE6);
write_cmos_sensor(0x3132,0x0B);
write_cmos_sensor(0x3A41,0x10);
write_cmos_sensor(0x312E,0x03);
write_cmos_sensor(0x303D,0x03);
write_cmos_sensor(0x3AC4,0x00);
write_cmos_sensor(0x3123,0x00);
write_cmos_sensor(0x3000,0x04);

}

}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
    // full size 30fps
write_cmos_sensor(0x3004,0x00);
write_cmos_sensor(0x3005,0x01);
write_cmos_sensor(0x3006,0x00);
write_cmos_sensor(0x3007,0xA0);
write_cmos_sensor(0x300D,0x00);
write_cmos_sensor(0x300E,0x00);
write_cmos_sensor(0x301A,0x00);
write_cmos_sensor(0x3039,0x00);
write_cmos_sensor(0x303A,0x00);
write_cmos_sensor(0x303E,0x00);
write_cmos_sensor(0x303F,0x00);
write_cmos_sensor(0x3040,0x00);
write_cmos_sensor(0x3068,0x00);
write_cmos_sensor(0x307E,0x00);
write_cmos_sensor(0x307F,0x00);
write_cmos_sensor(0x3080,0x00);
write_cmos_sensor(0x3081,0x00);
write_cmos_sensor(0x3082,0x00);
write_cmos_sensor(0x3083,0x00);
write_cmos_sensor(0x3084,0x00);
write_cmos_sensor(0x3085,0x00);
write_cmos_sensor(0x3086,0x00);
write_cmos_sensor(0x3087,0x00);
write_cmos_sensor(0x3095,0x00);
write_cmos_sensor(0x3096,0x00);
write_cmos_sensor(0x3097,0x00);
write_cmos_sensor(0x3098,0x00);
write_cmos_sensor(0x3099,0x00);
write_cmos_sensor(0x309A,0x00);
write_cmos_sensor(0x309B,0x00);
write_cmos_sensor(0x309C,0x00);
write_cmos_sensor(0x30BC,0x00);
write_cmos_sensor(0x30BD,0x00);
write_cmos_sensor(0x30BE,0x00);
write_cmos_sensor(0x30BF,0x00);
write_cmos_sensor(0x30C0,0x00);
write_cmos_sensor(0x30C1,0x00);
write_cmos_sensor(0x30C2,0x00);
write_cmos_sensor(0x30C3,0x00);
write_cmos_sensor(0x30C4,0x00);
write_cmos_sensor(0x30C5,0x00);
write_cmos_sensor(0x30C6,0x00);
write_cmos_sensor(0x30C7,0x00);
write_cmos_sensor(0x30C8,0x00);
write_cmos_sensor(0x30C9,0x00);
write_cmos_sensor(0x30CA,0x00);
write_cmos_sensor(0x30CB,0x00);
write_cmos_sensor(0x30CC,0x00);
write_cmos_sensor(0x30D0,0x08);
write_cmos_sensor(0x30D1,0x10);
write_cmos_sensor(0x30D5,0x00);
write_cmos_sensor(0x30D6,0x00);
write_cmos_sensor(0x30D7,0x00);
write_cmos_sensor(0x30D8,0x00);
write_cmos_sensor(0x30D9,0x00);
write_cmos_sensor(0x30DA,0x00);
write_cmos_sensor(0x30F5,0xEE);
write_cmos_sensor(0x30F6,0x02);
write_cmos_sensor(0x30F7,0x80);
write_cmos_sensor(0x30F8,0x0C);
write_cmos_sensor(0x30F9,0x00);
write_cmos_sensor(0x312F,0xF6);
write_cmos_sensor(0x3130,0x0B);
write_cmos_sensor(0x3131,0xE6);
write_cmos_sensor(0x3132,0x0B);
write_cmos_sensor(0x3A41,0x10);
write_cmos_sensor(0x312E,0x03);
write_cmos_sensor(0x303D,0x03);
write_cmos_sensor(0x3AC4,0x00);
write_cmos_sensor(0x3123,0x00);

}

static void hs_video_setting(void)
{
	LOG_INF("E\n");
//720p 120fps
write_cmos_sensor(0x3004,0x53);
write_cmos_sensor(0x3005,0x35);
write_cmos_sensor(0x3006,0x01);
write_cmos_sensor(0x3007,0xA0);
write_cmos_sensor(0x300D,0x00);
write_cmos_sensor(0x300E,0x00);
write_cmos_sensor(0x301A,0x00);
write_cmos_sensor(0x3039,0x00);
write_cmos_sensor(0x303A,0x00);
write_cmos_sensor(0x303E,0x00);
write_cmos_sensor(0x303F,0x00);
write_cmos_sensor(0x3040,0x00);
write_cmos_sensor(0x3068,0x00);
write_cmos_sensor(0x307E,0x00);
write_cmos_sensor(0x307F,0x00);
write_cmos_sensor(0x3080,0x00);
write_cmos_sensor(0x3081,0x00);
write_cmos_sensor(0x3082,0x00);
write_cmos_sensor(0x3083,0x00);
write_cmos_sensor(0x3084,0x00);
write_cmos_sensor(0x3085,0x00);
write_cmos_sensor(0x3086,0x00);
write_cmos_sensor(0x3087,0x00);
write_cmos_sensor(0x3095,0x00);
write_cmos_sensor(0x3096,0x00);
write_cmos_sensor(0x3097,0x00);
write_cmos_sensor(0x3098,0x00);
write_cmos_sensor(0x3099,0x00);
write_cmos_sensor(0x309A,0x00);
write_cmos_sensor(0x309B,0x00);
write_cmos_sensor(0x309C,0x00);
write_cmos_sensor(0x30BC,0x00);
write_cmos_sensor(0x30BD,0x00);
write_cmos_sensor(0x30BE,0x00);
write_cmos_sensor(0x30BF,0x00);
write_cmos_sensor(0x30C0,0x00);
write_cmos_sensor(0x30C1,0x00);
write_cmos_sensor(0x30C2,0x00);
write_cmos_sensor(0x30C3,0x00);
write_cmos_sensor(0x30C4,0x00);
write_cmos_sensor(0x30C5,0x00);
write_cmos_sensor(0x30C6,0x00);
write_cmos_sensor(0x30C7,0x00);
write_cmos_sensor(0x30C8,0x00);
write_cmos_sensor(0x30C9,0x00);
write_cmos_sensor(0x30CA,0x00);
write_cmos_sensor(0x30CB,0x00);
write_cmos_sensor(0x30CC,0x00);
write_cmos_sensor(0x30D0,0x68);
write_cmos_sensor(0x30D1,0x10);
write_cmos_sensor(0x30D5,0x00);
write_cmos_sensor(0x30D6,0x00);
write_cmos_sensor(0x30D7,0x00);
write_cmos_sensor(0x30D8,0x00);
write_cmos_sensor(0x30D9,0x00);
write_cmos_sensor(0x30DA,0x04);
write_cmos_sensor(0x30F5,0x08);
write_cmos_sensor(0x30F6,0x01);//264
write_cmos_sensor(0x30F7,0xE3);
write_cmos_sensor(0x30F8,0x08);//2275
write_cmos_sensor(0x30F9,0x00);
write_cmos_sensor(0x312F,0xE2);
write_cmos_sensor(0x3130,0x02);
write_cmos_sensor(0x3131,0xDE);
write_cmos_sensor(0x3132,0x02);
write_cmos_sensor(0x3A41,0x04);
write_cmos_sensor(0x312E,0x03);
write_cmos_sensor(0x303D,0x03);
write_cmos_sensor(0x3AC4,0x00);
write_cmos_sensor(0x3123,0x00);
write_cmos_sensor(0x3000,0x04);

}

static void hs_video_setting_240fps(void)
{
	LOG_INF("E\n");
//720p 240fps
write_cmos_sensor(0x3004,0x14);
write_cmos_sensor(0x3005,0x31);
write_cmos_sensor(0x3006,0x01);
write_cmos_sensor(0x3007,0xA0);
write_cmos_sensor(0x300D,0x00);
write_cmos_sensor(0x300E,0x00);
write_cmos_sensor(0x301A,0x00);
write_cmos_sensor(0x3039,0xD8);
write_cmos_sensor(0x303A,0x0F);
write_cmos_sensor(0x303E,0xC0);
write_cmos_sensor(0x303F,0x00);
write_cmos_sensor(0x3040,0x01);
write_cmos_sensor(0x3068,0x00);
write_cmos_sensor(0x307E,0x00);
write_cmos_sensor(0x307F,0x08);
write_cmos_sensor(0x3080,0x00);
write_cmos_sensor(0x3081,0x00);
write_cmos_sensor(0x3082,0x00);
write_cmos_sensor(0x3083,0x00);
write_cmos_sensor(0x3084,0x00);
write_cmos_sensor(0x3085,0x00);
write_cmos_sensor(0x3086,0x00);
write_cmos_sensor(0x3087,0x00);
write_cmos_sensor(0x3095,0x00);
write_cmos_sensor(0x3096,0x00);
write_cmos_sensor(0x3097,0x00);
write_cmos_sensor(0x3098,0x00);
write_cmos_sensor(0x3099,0x00);
write_cmos_sensor(0x309A,0x00);
write_cmos_sensor(0x309B,0x00);
write_cmos_sensor(0x309C,0x00);
write_cmos_sensor(0x30BC,0x00);
write_cmos_sensor(0x30BD,0x00);
write_cmos_sensor(0x30BE,0x00);
write_cmos_sensor(0x30BF,0x00);
write_cmos_sensor(0x30C0,0x00);
write_cmos_sensor(0x30C1,0x00);
write_cmos_sensor(0x30C2,0x00);
write_cmos_sensor(0x30C3,0x00);
write_cmos_sensor(0x30C4,0x00);
write_cmos_sensor(0x30C5,0x00);
write_cmos_sensor(0x30C6,0x00);
write_cmos_sensor(0x30C7,0x00);
write_cmos_sensor(0x30C8,0x00);
write_cmos_sensor(0x30C9,0x00);
write_cmos_sensor(0x30CA,0x00);
write_cmos_sensor(0x30CB,0x00);
write_cmos_sensor(0x30CC,0x00);
write_cmos_sensor(0x30D0,0x68);
write_cmos_sensor(0x30D1,0x10);
write_cmos_sensor(0x30D5,0x00);
write_cmos_sensor(0x30D6,0x00);
write_cmos_sensor(0x30D7,0x00);
write_cmos_sensor(0x30D8,0x00);
write_cmos_sensor(0x30D9,0x00);
write_cmos_sensor(0x30DA,0x05);
write_cmos_sensor(0x30F5,0x2C);
write_cmos_sensor(0x30F6,0x01);
write_cmos_sensor(0x30F7,0xE8);
write_cmos_sensor(0x30F8,0x03);
write_cmos_sensor(0x30F9,0x00);
write_cmos_sensor(0x312F,0xE2);
write_cmos_sensor(0x3130,0x02);
write_cmos_sensor(0x3131,0xDE);
write_cmos_sensor(0x3132,0x02);
write_cmos_sensor(0x3A41,0x04);
write_cmos_sensor(0x312E,0x03);
write_cmos_sensor(0x303D,0x03);
write_cmos_sensor(0x3AC4,0x00);
write_cmos_sensor(0x3123,0x00);
write_cmos_sensor(0x3000,0x04);

}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
	//preview_setting();
	hs_video_setting_240fps();
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
			//*sensor_id = ((read_cmos_sensor(0x3913) << 8) | read_cmos_sensor(0x3912));
		      *sensor_id=read_cmos_sensor(0x30F5);
			LOG_INF("read_0x30F5=0x%x\n",read_cmos_sensor(0x30F5));
			if (*sensor_id ==0x72) {
				*sensor_id=IMX377_SENSOR_ID;
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
				//iReadData(0x00,452,OTPData);
				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail,write_id(0x%x), id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
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
	LOG_INF("PLATFORM:MT6595,MIPI 2LANE\n");
	LOG_INF("preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n");

	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			//sensor_id = ((read_cmos_sensor(0x3913) << 8) | read_cmos_sensor(0x3912));
			sensor_id=read_cmos_sensor(0x30F5);
			if (sensor_id == 0x72) {
				sensor_id = IMX377_SENSOR_ID;
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail,write_id(0x%x), id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_mode = 0;
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

	preview_setting();
	//hs_video_setting();

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
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}  else if(imgsensor.current_fps == imgsensor_info.cap2.max_framerate){
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",imgsensor.current_fps, imgsensor_info.cap1.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",imgsensor.current_fps, imgsensor_info.cap1.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	 capture_setting(imgsensor.current_fps);

	return ERROR_NONE;
}	/* capture() */
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
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

        normal_video_setting(imgsensor.current_fps);

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
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

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
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

	return ERROR_NONE;
}	/*	slim_video	 */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
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
	LOG_INF("scenario_id = %d\n", scenario_id);


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
	//sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	//sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
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
	LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) //enable auto flicker
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
        	  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
                frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
                frame_length = imgsensor_info.cap2.pclk / framerate * 10 / imgsensor_info.cap2.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap2.framelength) ? (frame_length - imgsensor_info.cap2.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap2.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else {
        		    if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
                frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
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

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {

		write_cmos_sensor(0x303B,0x11);
		write_cmos_sensor(0x303C,0x0B);
	} else {
              write_cmos_sensor(0x303B,0x01);
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
  //  unsigned long long *feature_return_para=(unsigned long long *) feature_para;

	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
//    SET_SENSOR_AWB_GAIN *pSetSensorAWB=(SET_SENSOR_AWB_GAIN *)feature_para;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk,imgsensor.current_fps);
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
                     set_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
                     night_mode((BOOL) *feature_data);
			break;
		case SENSOR_FEATURE_SET_GAIN:
                     set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_SHUTTER_BUF_MODE:
			set_shutter_buf_mode(*feature_data);
			break;
		case SENSOR_FEATURE_SET_GAIN_BUF_MODE:
			set_gain_buf_mode((UINT16) *feature_data);
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
                     get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
                    set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;
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
			imgsensor.ihdr_mode = *feature_data;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:
            LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data);
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
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
            LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            //ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
			break;
        case SENSOR_FEATURE_SET_AWB_GAIN:
            break;
        case SENSOR_FEATURE_SET_HDR_SHUTTER:
            LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1));
            //ihdr_write_shutter((UINT16)*feature_data,(UINT16)*(feature_data+1));
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

//kin0603
UINT32 IMX377_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
//UINT32 IMX214_MIPI_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	OV5693_MIPI_RAW_SensorInit	*/
