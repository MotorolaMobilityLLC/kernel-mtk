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
 *	 ov23850_ensor.c
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
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov23850_sensor.h"

#define PFX "ov23850"
#define LOG_INF(fmt, args...)   pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##args)

#define MULTI_WRITE 0
/*Enable PDAF function */
//#define ENABLE_PDAF_VC
//#define HW_DESKEW_ENABLE
static DEFINE_SPINLOCK(imgsensor_drv_lock);


static imgsensor_info_struct imgsensor_info = {
	.sensor_id = OV23850_SENSOR_ID,

	.checksum_value = 0xd6650427,

	.pre = {
		.pclk = 480000000,
		.linelength = 6240,
		.framelength = 2562,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
		.cap = {/*1.8Gbps*/
		.pclk = 720000000,
		.linelength = 6240,
		.framelength = 4806,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 5632,
		.grabwindow_height = 4224,
		.mipi_data_lp2hs_settle_dc = 14, //check
		.max_framerate = 240,

	},
	/*same as capture*/
	.cap1 = {
		.pclk = 720000000,
		.linelength = 6240,
		.framelength = 4806,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 5632,
		.grabwindow_height = 4224,
		.mipi_data_lp2hs_settle_dc = 14, //check
		.max_framerate = 240,
	},
	.normal_video = {/*1.8Gbps*/
		.pclk = 720000000,
		.linelength = 6240,
		.framelength = 3844,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 5632,
		.grabwindow_height = 3168,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 720000000,
		.linelength = 6240,
		.framelength = 960,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 360000000,
		.linelength = 6240,
		.framelength = 1920,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,/*(R380c:R380d)*/
		.grabwindow_height = 720,/*(R380e:R380f)*/
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
    },
    .custom1 = {
		.pclk = 480000000,
		.linelength = 6240,
		.framelength = 2562,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
    },
    .custom2 = {
		.pclk = 480000000,
		.linelength = 6240,
		.framelength = 2562,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
    },
    .custom3 = {
		.pclk = 480000000,
		.linelength = 6240,
		.framelength = 2562,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
    },
    .custom4 = {
		.pclk = 480000000,
		.linelength = 6240,
		.framelength = 2562,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
    },
    .custom5 = {
		.pclk = 480000000,
		.linelength = 6240,
		.framelength = 2562,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,
		.grabwindow_height = 2112,
		.mipi_data_lp2hs_settle_dc = 14,
		.max_framerate = 300,
    },
	.margin =8,
	.min_shutter = 2,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,  //check
	.ae_sensor_gain_delay_frame = 0,  //check
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 10,

	.cap_delay_frame = 3,  //check
	.pre_delay_frame = 3,  //check
	.video_delay_frame = 3,  //check
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,
    .custom1_delay_frame = 3,
    .custom2_delay_frame = 3,
    .custom3_delay_frame = 3,
    .custom4_delay_frame = 3,
    .custom5_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
    .mipi_sensor_type = MIPI_OPHY_NCSI2,
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x6c, 0xff}, //0x6c or 0x20
    .i2c_speed = 400,
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,
	.gain = 0x100,
	.dummy_pixel = 0,
	.dummy_line = 0,
    .current_fps = 30,
    .autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = 0,
	.i2c_write_id = 0x20,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] =
{{ 5664, 4240, 0,   0, 5664, 4240, 2832, 2120,  8,   4,   2816, 2112, 0,   0,  2816,  2112}, // Preview 
 { 5664, 4240, 0,   0, 5664, 4240, 5664, 4240, 00,  00,   5664, 4240,16,   8,  5632,  4224}, // capture  
 { 5664, 4240, 0,   0, 5664, 4240, 5664, 4240, 00,  00,   5664, 4240,16,   8,  5632,  3168}, // normal video 
 { 2624, 1956, 8, 246, 2608, 1460, 1280,  720, 00,  00,   1280,  720, 2,   2,  1280,   720}, //hight speed video
 { 2624, 1956, 8, 246, 2608, 1460, 1280,  720, 00,  00,   1280,  720, 2,   2,  1280,   720}, //slim video
 { 5664, 4240, 0,   0, 5664, 4240, 2832, 2120,  8,   4,   2816, 2112, 0,   0,  2816,  2112}, // Custom1 (defaultuse preview)
 { 5664, 4240, 0,   0, 5664, 4240, 2832, 2120,  8,   4,   2816, 2112, 0,   0,  2816,  2112}, // Custom2
 { 5664, 4240, 0,   0, 5664, 4240, 2832, 2120,  8,   4,   2816, 2112, 0,   0,  2816,  2112}, // Custom3
 { 5664, 4240, 0,   0, 5664, 4240, 2832, 2120,  8,   4,   2816, 2112, 0,   0,  2816,  2112}, // Custom4
 { 5664, 4240, 0,   0, 5664, 4240, 2832, 2120,  8,   4,   2816, 2112, 0,   0,  2816,  2112}, // Custom5
 };// slim video

/*VC1 for HDR(N/A) , VC2 for PDAF(VC1,DT=0X2b), unit : 10bit*/
static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
{/* Preview mode setting */
 {0x02, 0x0a,   0x00,   0x00, 0x00, 0x00,
  0x00, 0x2b, 0x1600, 0x1080, 0x00, 0x00, 0x0000, 0x0000,
  0x01, 0x2b, 0x00a8, 0x0800, 0x03, 0x00, 0x0000, 0x0000},
  /* Capture mode setting */
 {0x02, 0x0a,   0x00,   0x00, 0x00, 0x00,
  0x00, 0x2b, 0x1600, 0x1080, 0x00, 0x00, 0x0000, 0x0000,
  0x01, 0x2b, 0x00a8, 0x0800, 0x03, 0x00, 0x0000, 0x0000},
  /* Video mode setting */
 {0x02, 0x0a,   0x00,   0x00, 0x00, 0x00,
  0x00, 0x2b, 0x1600, 0x1080, 0x01, 0x2b, 0x00a8, 0x0800,
  0x02, 0x00, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000}
};

 /*PD information*/
 static SET_PD_BLOCK_INFO_T imgsensor_pd_info =
 {
	 .i4OffsetX = 128,
	 .i4OffsetY = 68,
	 .i4PitchX	= 32,
	 .i4PitchY	= 32,
	 .i4PairNum  =8,
	 .i4SubBlkW  =16,
	 .i4SubBlkH  =8,
	 .i4PosL = {{142,71},{158,71},{133,78},{149,78},{138,87},{154,87},{133,94},{149,94},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
	 .i4PosR = {{141,70},{157,70},{134,79},{150,79},{137,86},{153,86},{134,95},{150,95},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
 };

#define ov23850_MIPI_table_write_cmos_sensor ov23850_MIPI_table_write_cmos_sensor_multi
#define I2C_BUFFER_LEN 240
 static kal_uint16 ov23850_MIPI_table_write_cmos_sensor_multi(kal_uint16* para, kal_uint32 len)
 {

	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;
    int ret =0;
	
	tosend = 0;
	IDX = 0;
	//LOG_INF("enter ov23850_MIPI_table_write_cmos_sensor_multi len  %d\n",len);
#if MULTI_WRITE
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
			iBurstWriteReg_multi(puSendCmd , tosend, imgsensor.i2c_write_id, 3, imgsensor_info.i2c_speed);
			tosend = 0;
		}
	}
#else

	while(IDX < len)
	{
	    addr = para[IDX];

		puSendCmd[0] = (char)(addr >> 8);
		puSendCmd[1] = (char)(addr & 0xFF);
		data = para[IDX+1];
		puSendCmd[2] = (char)(data & 0xFF);
		IDX += 2;
		addr_last = addr;
		
		ret = iWriteRegI2CTiming(puSendCmd , 3, imgsensor.i2c_write_id, imgsensor_info.i2c_speed);
		
		if(ret != 0)
		{
			LOG_INF("Error : write i2c fail addr(0x%x), data(0x%x)\n",para[IDX], puSendCmd[2]);
		}
	}
	
#endif
	//LOG_INF("exit ov23850_MIPI_table_write_cmos_sensor_multi\n");

	return 0;
}

//#define USE_OV23850_EEPROM
#ifdef USE_OV23850_EEPROM
#define EEPROM_READ_ID  0xA1
#define EEPROM_WRITE_ID  0xA0
//int ov23850_module_id = 0;
char primax_ov23850_eeprom[8192];

static kal_uint16 eeprom_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(pusendcmd , 1, (u8*)&get_byte,1,EEPROM_WRITE_ID);
    return get_byte;
}
static void eeprom_write_cmos_sensor(char addr, char para)
{
    char pusendcmd[2] = {addr ,para};
    iWriteRegI2C(pusendcmd , 2, EEPROM_WRITE_ID);
}
static void eeprom_read_data(void)
{
	int i;
	for(i=0;i<0x2000;i++){
		primax_ov23850_eeprom[i] = eeprom_read_cmos_sensor(i);
		LOG_INF("eeprom[0x%x]= 0x%x\n",i,primax_ov23850_eeprom[i]);
	}
}

#endif

//#define USE_OIS
#ifdef USE_OIS
#define OIS_I2C_WRITE_ID 0x1c
#define OIS_I2C_READ_ID 0x1d

#define OP_CODE       0X84
#define OIS_CTRL_ADDR 0X7F
#define OIS_ON        0x0C0C
#define OIS_OFF       0x0000

static void OIS_write_cmos_sensor(kal_uint32 addr, kal_uint32 op_code, kal_uint32 para)
{
    char pusendcmd[4] = {(char)(op_code & 0xFF) ,(char)(addr & 0xFF) ,(char)(para >> 8) , (char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 4, OIS_I2C_WRITE_ID);
	//_WriteReg(pusendcmd , 4, OIS_I2C_WRITE_ID);
}
#if 1
static kal_uint16 OIS_read_cmos_sensor(kal_uint32 addr,u8 op_code)
{
    kal_uint16 get_word[2];
	kal_uint16 data;
    char pusendcmd[3] = {op_code,(char)(addr & 0xFF) };
    iReadRegI2C(pusendcmd , 2, get_word,2,OIS_I2C_WRITE_ID);
	//_ReadReg(pusendcmd , 3, get_word,2,OIS_I2C_WRITE_ID);
	data = get_word[1]<<8 | get_word[0];
    return data;
}
#else
static kal_uint16 OIS_read_cmos_sensor(kal_uint32 addr,u8 op_code)
{
    kal_uint16 get_byte = 0;

    char pu_send_cmd[2] = {op_code,(char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 2, OIS_I2C_WRITE_ID);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

#endif
static int BU64164_OIS_on(void)
{
	//mt_set_gpio_mode(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_CAMERA_OIS_LDO_EN_PIN_M_GPIO);
	//mt_set_gpio_dir(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_DIR_OUT);
	//mt_set_gpio_out(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_OUT_ONE);

	OIS_write_cmos_sensor(OIS_CTRL_ADDR,OP_CODE,OIS_ON);
}

static int BU64164_OIS_off(void)
{

	//mt_set_gpio_mode(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_CAMERA_OIS_LDO_EN_PIN_M_GPIO);
	//mt_set_gpio_dir(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_DIR_OUT);
	//mt_set_gpio_out(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_OUT_ZERO);

	OIS_write_cmos_sensor(OIS_CTRL_ADDR,OP_CODE,OIS_OFF);
}
static int BU64164_read_id(void)
{
	int BU64164_id=0;
	//mt_set_gpio_mode(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_CAMERA_OIS_LDO_EN_PIN_M_GPIO);
	//mt_set_gpio_dir(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_DIR_OUT);
	//mt_set_gpio_out(GPIO_CAMERA_OIS_LDO_EN_PIN,GPIO_OUT_ONE);
	BU64164_id=OIS_read_cmos_sensor(0x00,0x82);
	LOG_INF("%s %d BU64164_id=0x%x\n",__func__,__LINE__,BU64164_id);

}
#endif
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    //iReadRegI2C(pusendcmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
	iReadRegI2CTiming(pusendcmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
    return get_byte;
}


static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 3, imgsensor.i2c_write_id);
	iWriteRegI2CTiming(pusendcmd , 3, imgsensor.i2c_write_id, imgsensor_info.i2c_speed);
}


static void set_dummy(void)
{
    //check
    //LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);

    write_cmos_sensor(0x380c, imgsensor.line_length >> 8);
    write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);
    write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
    write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
}


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
    //kal_int16 dummy_line;
    kal_uint32 frame_length = imgsensor.frame_length;

    //LOG_INF("framerate = %d, min_framelength_en=%d\n", framerate,min_framelength_en);
    frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
    //LOG_INF("frame_length =%d\n", frame_length);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
    imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
    {
        imgsensor.frame_length = imgsensor_info.max_frame_length;
        imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
    }
    if (min_framelength_en)
        imgsensor.min_frame_length = imgsensor.frame_length;
    spin_unlock(&imgsensor_drv_lock);
    //LOG_INF("framerate = %d, min_framelength_en=%d\n", framerate,min_framelength_en);
    set_dummy();
}


static void write_shutter(kal_uint16 shutter)
    {
    //check
        kal_uint16 realtime_fps = 0;
        //kal_uint32 frame_length = 0;
#if 0
        if(imgsensor.sensor_mode == IMGSENSOR_MODE_HIGH_SPEED_VIDEO)
        {
            if(shutter > imgsensor.min_frame_length - imgsensor_info.margin)
                shutter = imgsensor.min_frame_length - imgsensor_info.margin;
            write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
            write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
            write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
            write_cmos_sensor(0x3502, shutter  & 0xFF);
			LOG_INF("shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
            return;
        }
#endif
		/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
		/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */
		// OV Recommend Solution
		// if shutter bigger than frame_length, should extend frame length first

        //LOG_INF("shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
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

		if (imgsensor.autoflicker_en == KAL_TRUE)
		{
            realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
            if(realtime_fps >= 297 && realtime_fps <= 305){
				realtime_fps = 296;
                set_max_framerate(realtime_fps,0);
			}
            else if(realtime_fps >= 147 && realtime_fps <= 150){
				realtime_fps = 146;
                set_max_framerate(realtime_fps ,0);
			}
            else
            {
            	imgsensor.frame_length = (imgsensor.frame_length  >> 1) << 1;
                write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
                write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
            }
        }
        else
        {
        	imgsensor.frame_length = (imgsensor.frame_length  >> 1) << 1;
            write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
            write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
        }
		/*Warning : shutter must be even. Odd might happen Unexpected Results */
		shutter = (shutter >> 1) << 1;
        write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
        write_cmos_sensor(0x3502, shutter  & 0xFF);
        //LOG_INF("realtime_fps =%d\n", realtime_fps);
        LOG_INF("shutter =%d, framelength =%d, realtime_fps =%d\n", shutter,imgsensor.frame_length, realtime_fps);
    }



static void set_shutter(kal_uint16 shutter)
{
    unsigned long flags;
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
    write_shutter(shutter);
}


#if 0
static kal_uint16 gain2reg(const kal_uint16 gain)
{       
	return 0;
}
#endif


static kal_uint16 set_gain(kal_uint16 gain)
{
    //check
    gain = gain / 4;//platform 1xgain = 64, sensor driver 1*gain = 16. div =4
    write_cmos_sensor(0x0350b,(gain&0xff)); 
    return 0;
}

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
    LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);

}


#if 0
static void set_mirror_flip(kal_uint8 image_mirror)
{
    //check
    kal_int16 mirror=0,flip=0;
    mirror= read_cmos_sensor(0x3820);
    flip  = read_cmos_sensor(0x3821);

    LOG_INF("image_mirror = %d\n", image_mirror);

    switch (image_mirror)
    {
        case IMAGE_NORMAL:
            write_cmos_sensor(0x3820,((mirror & 0xF9) | 0x00));
            write_cmos_sensor(0x3821,((flip & 0xF9) | 0x06));
            break;
        case IMAGE_H_MIRROR:
            write_cmos_sensor(0x3820,((mirror & 0xF9) | 0x00));
            write_cmos_sensor(0x3821,((flip & 0xF9) | 0x00));
            break;
        case IMAGE_V_MIRROR:
            write_cmos_sensor(0x3820,((mirror & 0xF9) | 0x06));
            write_cmos_sensor(0x3821,((flip & 0xF9) | 0x06));
            break;
        case IMAGE_HV_MIRROR:
            write_cmos_sensor(0x3820,((mirror & 0xF9) | 0x06));
            write_cmos_sensor(0x3821,((flip & 0xF9) | 0x00));
            break;
    }
}
#endif


static void night_mode(kal_bool enable)
{

}

kal_uint16 addr_data_pair_slim_video[] = {
	0x0100,0x00,
	0x0300,0x00,
	0x0301,0x64,
	0x0302,0x10,
	0x0303,0x10,
	0x0304,0x47,
	0x030d,0x2d,
	0x030e,0x00,
	0x030f,0x05,
	0x0312,0x02,
	0x0313,0x11,
	0x0316,0x1e,
	0x0317,0x02,
	0x0318,0x03,
	0x031c,0x01,
	0x031d,0x02,
	0x031e,0x09,
	0x2b05,0x02,
	0x2b06,0x87,
	0x2b07,0x01,
	0x2b08,0xa8,
	0x0320,0x02,
	0x3002,0x00,
	0x300f,0x11,
	0x3010,0x01,
	0x3012,0x41,
	0x3016,0xd2,
	0x3018,0x70,
	0x3019,0xc3,
	0x301b,0x96,
	0x3022,0x0f,
	0x3023,0xb4,
	0x3031,0x91,
	0x3034,0x41,
	0x340c,0xff,
	0x3501,0x03,
	0x3502,0xa0,
	0x3503,0x00,
	0x3507,0x00,
	0x3508,0x00,
	0x3509,0x12,
	0x350a,0x00,
	0x350b,0x80,
	0x350f,0x10,
	0x3541,0x02,
	0x3542,0x00,
	0x3543,0x00,
	0x3547,0x00,
	0x3548,0x00,
	0x3549,0x12,
	0x354b,0x10,
	0x354f,0x10,
	0x3601,0xa2,
	0x3603,0x97,
	0x3604,0x02,
	0x3605,0xf2,
	0x3606,0x88,
	0x3607,0x11,
	0x360a,0x34,
	0x360c,0x13,
	0x3618,0xcc,
	0x3620,0x50,
	0x3621,0x99,
	0x3622,0x7d,
	0x3624,0x05,
	0x362a,0x25,
	0x3650,0x04,
	0x3660,0xc0,
	0x3661,0x00,
	0x3662,0x00,
	0x3664,0x88,
	0x3667,0x00,
	0x366a,0x5c,
	0x366c,0x80,
	0x3700,0x62,
	0x3701,0x08,
	0x3702,0x10,
	0x3703,0x3e,
	0x3704,0x26,
	0x3705,0x01,
	0x3706,0x3a,
	0x3707,0xc4,
	0x3708,0x3c,
	0x3709,0x1c,
	0x370a,0xa3,
	0x370b,0x2c,
	0x370c,0x42,
	0x370d,0xa4,
	0x370e,0x14,
	0x370f,0x0a,
	0x3710,0x15,
	0x3711,0x0a,
	0x3712,0xa2,
	0x3713,0x00,
	0x371e,0x2a,
	0x371f,0x13,
	0x3714,0x00,
	0x3717,0x00,
	0x3719,0x00,
	0x371c,0x04,
	0x3720,0xaa,
	0x3721,0x10,
	0x3722,0x50,
	0x3725,0xf0,
	0x3726,0x22,
	0x3727,0x44,
	0x3728,0x40,
	0x3729,0x00,
	0x372b,0x00,
	0x372c,0x92,
	0x372d,0x0c,
	0x372e,0x22,
	0x372f,0x88,
	0x3732,0x01,
	0x3733,0xd0,
	0x3730,0x01,
	0x3731,0xc8,
	0x3744,0x01,
	0x3745,0x24,
	0x3746,0x00,
	0x3747,0xd0,
	0x3748,0x27,
	0x374a,0x4b,
	0x374b,0x44,
	0x3760,0xd1,
	0x3761,0x52,
	0x3762,0xa4,
	0x3763,0x14,
	0x3766,0x0c,
	0x3767,0x25,
	0x3768,0x0c,
	0x3769,0x24,
	0x376a,0x09,
	0x376b,0x02,
	0x376d,0x01,
	0x376e,0x53,
	0x376f,0x01,
	0x378c,0x08,
	0x378d,0x46,
	0x378e,0x14,
	0x378f,0x02,
	0x3790,0xc4,
	0x3792,0x64,
	0x3793,0x5d,
	0x3794,0x29,
	0x3795,0x4f,
	0x3796,0x43,
	0x3797,0x09,
	0x3798,0x02,
	0x3799,0x33,
	0x379a,0x09,
	0x379b,0x1e,
	0x379f,0x3e,
	0x37a0,0x44,
	0x37a1,0x00,
	0x37a2,0x44,
	0x37a3,0x41,
	0x37a4,0x88,
	0x37a5,0x69,
	0x37b0,0x48,
	0x37b1,0x20,
	0x37b2,0x03,
	0x37b3,0x48,
	0x37b4,0x02,
	0x37b5,0x33,
	0x37b6,0x22,
	0x37b8,0x02,
	0x37bc,0x02,
	0x37c0,0x3b,
	0x37c1,0xc2,
	0x37c2,0x06,
	0x37c3,0x06,
	0x37c5,0x33,
	0x37c6,0x35,
	0x37c7,0x00,
	0x3800,0x00,
	0x3801,0x14,
	0x3802,0x02,
	0x3803,0x94,
	0x3804,0x10,
	0x3805,0x8b,
	0x3806,0x0e,
	0x3807,0x0b,
	0x3808,0x05,
	0x3809,0x00,
	0x380a,0x02,
	0x380b,0xd0,
	0x380c,0x30,
	0x380d,0xc0,
	0x380e,0x03,
	0x380f,0xc0,
	0x3810,0x00,
	0x3811,0x03,
	0x3813,0x08,
	0x3814,0x11,
	0x3815,0x53,
	0x3820,0x01,
	0x3821,0x04,
	0x3834,0x00,
	0x3835,0x04,
	0x3836,0x18,
	0x3837,0x02,
	0x382f,0x84,
	0x383c,0xc8,
	0x383d,0xe3,
	0x3842,0x20,
	0x384b,0x00,
	0x3d85,0x16,
	0x3d8c,0x77,
	0x3d8d,0x10,
	0x3f00,0x52,
	0x4000,0x17,
	0x4001,0x60,
	0x4001,0x60,
	0x4008,0x00,
	0x4009,0x03,
	0x400f,0x00,
	0x4011,0xfb,
	0x4017,0x08,
	0x4018,0x00,
	0x401a,0xce,
	0x4019,0x06,
	0x4020,0x08,
	0x4022,0x08,
	0x4024,0x08,
	0x4026,0x08,
	0x4028,0x08,
	0x402a,0x08,
	0x402c,0x08,
	0x402e,0x08,
	0x4030,0x08,
	0x4032,0x08,
	0x4034,0x08,
	0x4036,0x08,
	0x4038,0x08,
	0x403a,0x08,
	0x403c,0x08,
	0x403e,0x08,
	0x405c,0x3f,
	0x4066,0x02,
	0x4051,0x01,
	0x4052,0x00,
	0x4053,0x80,
	0x4054,0x00,
	0x4055,0x80,
	0x4056,0x00,
	0x4057,0x80,
	0x4058,0x00,
	0x4059,0x80,
	0x4202,0x00,
	0x4203,0x01,
	0x430b,0xff,
	0x430d,0x00,
	0x4500,0x72,
	0x4605,0x00,
	0x4640,0x01,
	0x4641,0x04,
	0x4645,0x00,
	0x4800,0x04,
	0x4809,0x2b,
	0x4813,0x90,
	0x4817,0x04,
	0x4833,0x18,
	0x4837,0x11,
	0x484b,0x01,
	0x4850,0x5c,
	0x4852,0x27,
	0x4856,0x5c,
	0x4857,0x55,
	0x486a,0xaa,
	0x486e,0x03,
	0x486f,0x55,
	0x4875,0xf0,
	0x4b04,0x80,
	0x4b05,0xb3,
	0x4b06,0x00,
	0x4c01,0xdf,
	0x4d00,0x04,
	0x4d01,0xf0,
	0x4d02,0xb8,
	0x4d03,0xf2,
	0x4d04,0x88,
	0x4d05,0x9d,
	0x4e00,0x04,
	0x4e17,0x04,
	0x4e33,0x18,
	0x4e37,0x11,
	0x4e4b,0x01,
	0x4e50,0x5c,
	0x4e52,0x27,
	0x4e56,0x5c,
	0x4e57,0x55,
	0x4e6a,0xaa,
	0x4e6e,0x03,
	0x4e6f,0x55,
	0x4e75,0xf0,
	0x5000,0xdb,
	0x5001,0x42,
	0x5002,0x10,
	0x5003,0x01,
	0x5004,0x00,
	0x5005,0x00,
	0x501d,0x00,
	0x501f,0x06,
	0x5020,0x03,
	0x5021,0x00,
	0x5022,0x13,
	0x5061,0xff,
	0x5062,0xff,
	0x5063,0xff,
	0x5064,0xff,
	0x506f,0x00,
	0x5280,0x00,
	0x5282,0x00,
	0x5283,0x01,
	0x5200,0x00,
	0x5201,0x62,
	0x5203,0x00,
	0x5204,0x00,
	0x5205,0x40,
	0x5209,0x00,
	0x520a,0x80,
	0x520b,0x04,
	0x520c,0x01,
	0x5210,0x10,
	0x5211,0xa0,
	0x5292,0x04,
	0x5500,0x00,
	0x5501,0x00,
	0x5502,0x00,
	0x5503,0x00,
	0x5504,0x00,
	0x5505,0x00,
	0x5506,0x00,
	0x5507,0x00,
	0x5508,0x00,
	0x5509,0x00,
	0x550a,0x00,
	0x550b,0x00,
	0x550c,0x00,
	0x550d,0x00,
	0x550e,0x00,
	0x550f,0x00,
	0x5510,0x00,
	0x5511,0x00,
	0x5512,0x00,
	0x5513,0x00,
	0x5514,0x00,
	0x5515,0x00,
	0x5516,0x00,
	0x5517,0x00,
	0x5518,0x00,
	0x5519,0x00,
	0x551a,0x00,
	0x551b,0x00,
	0x551c,0x00,
	0x551d,0x00,
	0x551e,0x00,
	0x551f,0x00,
	0x5520,0x00,
	0x5521,0x00,
	0x5522,0x00,
	0x5523,0x00,
	0x5524,0x00,
	0x5525,0x00,
	0x5526,0x00,
	0x5527,0x00,
	0x5528,0x00,
	0x5529,0x00,
	0x552a,0x00,
	0x552b,0x00,
	0x552c,0x00,
	0x552d,0x00,
	0x552e,0x00,
	0x552f,0x00,
	0x5530,0x00,
	0x5531,0x00,
	0x5532,0x00,
	0x5533,0x00,
	0x5534,0x00,
	0x5535,0x00,
	0x5536,0x00,
	0x5537,0x00,
	0x5538,0x00,
	0x5539,0x00,
	0x553a,0x00,
	0x553b,0x00,
	0x553c,0x00,
	0x553d,0x00,
	0x553e,0x00,
	0x553f,0x00,
	0x5540,0x00,
	0x5541,0x00,
	0x5542,0x00,
	0x5543,0x00,
	0x5544,0x00,
	0x5545,0x00,
	0x5546,0x00,
	0x5547,0x00,
	0x5548,0x00,
	0x5549,0x00,
	0x554a,0x00,
	0x554b,0x00,
	0x554c,0x00,
	0x554d,0x00,
	0x554e,0x00,
	0x554f,0x00,
	0x5550,0x00,
	0x5551,0x00,
	0x5552,0x00,
	0x5553,0x00,
	0x5554,0x00,
	0x5555,0x00,
	0x5556,0x00,
	0x5557,0x00,
	0x5558,0x00,
	0x5559,0x00,
	0x555a,0x00,
	0x555b,0x00,
	0x555c,0x00,
	0x555d,0x00,
	0x555e,0x00,
	0x555f,0x00,
	0x5560,0x00,
	0x5561,0x00,
	0x5562,0x00,
	0x5563,0x00,
	0x5564,0x00,
	0x5565,0x00,
	0x5566,0x00,
	0x5567,0x00,
	0x5568,0x00,
	0x5569,0x00,
	0x556a,0x00,
	0x556b,0x00,
	0x556c,0x00,
	0x556d,0x00,
	0x556e,0x00,
	0x556f,0x00,
	0x5570,0x00,
	0x5571,0x00,
	0x5572,0x00,
	0x5573,0x00,
	0x5574,0x00,
	0x5575,0x00,
	0x5576,0x00,
	0x5577,0x00,
	0x5578,0x00,
	0x5579,0x00,
	0x557a,0x00,
	0x557b,0x00,
	0x557c,0x00,
	0x557d,0x00,
	0x557e,0x00,
	0x557f,0x00,
	0x5580,0x00,
	0x5581,0x00,
	0x5582,0x00,
	0x5583,0x00,
	0x5584,0x00,
	0x5585,0x00,
	0x5586,0x00,
	0x5587,0x00,
	0x5588,0x00,
	0x5589,0x00,
	0x558a,0x00,
	0x558b,0x00,
	0x558c,0x00,
	0x558d,0x00,
	0x558e,0x00,
	0x558f,0x00,
	0x5590,0x00,
	0x5591,0x00,
	0x5592,0x00,
	0x5593,0x00,
	0x5594,0x00,
	0x5595,0x00,
	0x5596,0x00,
	0x5597,0x00,
	0x5598,0x00,
	0x5599,0x00,
	0x559a,0x00,
	0x559b,0x00,
	0x559c,0x00,
	0x559d,0x00,
	0x559e,0x00,
	0x559f,0x00,
	0x55a0,0x00,
	0x55a1,0x00,
	0x55a2,0x00,
	0x55a3,0x00,
	0x55a4,0x00,
	0x55a5,0x00,
	0x55a6,0x00,
	0x55a7,0x00,
	0x55a8,0x00,
	0x55a9,0x00,
	0x55aa,0x00,
	0x55ab,0x00,
	0x55ac,0x00,
	0x55ad,0x00,
	0x55ae,0x00,
	0x55af,0x00,
	0x55b0,0x00,
	0x55b1,0x00,
	0x55b2,0x00,
	0x55b3,0x00,
	0x55b4,0x00,
	0x55b5,0x00,
	0x55b6,0x00,
	0x55b7,0x00,
	0x55b8,0x00,
	0x55b9,0x00,
	0x55ba,0x00,
	0x55bb,0x00,
	0x55bc,0x00,
	0x55bd,0x00,
	0x55be,0x00,
	0x55bf,0x00,
	0x55c0,0x00,
	0x55c1,0x00,
	0x55c2,0x00,
	0x55c3,0x00,
	0x55c4,0x00,
	0x55c5,0x00,
	0x55c6,0x00,
	0x55c7,0x00,
	0x55c8,0x00,
	0x55c9,0x00,
	0x55ca,0x00,
	0x55cb,0x00,
	0x55cc,0x00,
	0x55cd,0x00,
	0x55ce,0x00,
	0x55cf,0x00,
	0x55d0,0x00,
	0x55d1,0x00,
	0x55d2,0x00,
	0x55d3,0x00,
	0x55d4,0x00,
	0x55d5,0x00,
	0x55d6,0x00,
	0x55d7,0x00,
	0x55d8,0x00,
	0x55d9,0x00,
	0x55da,0x00,
	0x55db,0x00,
	0x55dc,0x00,
	0x55dd,0x00,
	0x55de,0x00,
	0x55df,0x00,
	0x55e0,0x00,
	0x55e1,0x00,
	0x55e2,0x00,
	0x55e3,0x00,
	0x55e4,0x00,
	0x55e5,0x00,
	0x55e6,0x00,
	0x55e7,0x00,
	0x55e8,0x00,
	0x55e9,0x00,
	0x55ea,0x00,
	0x55eb,0x00,
	0x55ec,0x00,
	0x55ed,0x00,
	0x55ee,0x00,
	0x55ef,0x00,
	0x55f0,0x00,
	0x55f1,0x00,
	0x55f2,0x00,
	0x55f3,0x00,
	0x55f4,0x00,
	0x55f5,0x00,
	0x55f6,0x00,
	0x55f7,0x00,
	0x55f8,0x00,
	0x55f9,0x00,
	0x55fa,0x00,
	0x55fb,0x00,
	0x55fc,0x00,
	0x55fd,0x00,
	0x55fe,0x00,
	0x55ff,0x00,
	0x5600,0x30,
	0x5601,0x00,
	0x5602,0x00,
	0x5603,0x00,
	0x5604,0x00,
	0x5605,0x00,
	0x5606,0x00,
	0x5607,0x01,
	0x5608,0x01,
	0x5609,0x01,
	0x560f,0xfc,
	0x5610,0xf0,
	0x5611,0x10,
	0x562f,0xfc,
	0x5630,0xf0,
	0x5631,0x10,
	0x564f,0xfc,
	0x5650,0xf0,
	0x5651,0x10,
	0x566f,0xfc,
	0x5670,0xf0,
	0x5671,0x10,
	0x567b,0x40,
	0x5690,0x00,
	0x5691,0x00,
	0x5692,0x00,
	0x5693,0x00,
	0x5694,0x80,
	0x5696,0x06,
	0x5697,0x0a,
	0x5698,0x00,
	0x5699,0x90,
	0x569a,0x15,
	0x569b,0x90,
	0x569c,0x00,
	0x569d,0x50,
	0x569e,0x10,
	0x569f,0x50,
	0x56a0,0x36,
	0x56a1,0x50,
	0x56a2,0x16,
	0x56a3,0x20,
	0x56a4,0x10,
	0x56a5,0xa0,
	0x5c80,0x06,
	0x5c81,0x80,
	0x5c82,0x09,
	0x5c83,0x5f,
	0x5d04,0x00,
	0x5d05,0x00,
	0x5d06,0x00,
	0x5d07,0x00,
	0x5d12,0x00,
	0x5d13,0x00,
	0x5d14,0x00,
	0x5d15,0x00,
	0x5d16,0x00,
	0x5d17,0x00,
	0x5d18,0x00,
	0x5d19,0x00,
	0x5d1a,0x00,
	0x5d1b,0x00,
	0x5d24,0x00,
	0x5d25,0x00,
	0x5d26,0x00,
	0x5d27,0x00,
	0x5d28,0x80,
	0x5d2a,0x00,
	0x5d2b,0x90,
	0x5d2c,0x15,
	0x5d2d,0x90,
	0x5d2e,0x00,
	0x5d2f,0x50,
	0x5d30,0x10,
	0x5d31,0x50,
	0x5d32,0x10,
	0x5d34,0x36,
	0x5d35,0x20,
	0x5d36,0x90,
	0x5d37,0xa0,
	0x5d38,0x5c,
	0x5d39,0x7b,
	0x5d1c,0x00,
	0x5d1d,0x00,
	0x5d1e,0x00,
	0x5d1f,0x00,
	0x5d20,0x16,
	0x5d21,0x20,
	0x5d22,0x10,
	0x5d23,0xa0,
	0x5d29,0x80,
	0x3008,0x01,
	0x3663,0x60,
	0x3002,0x01,
	0x3c00,0x3c,
	0x3025,0x03,
	0x3668,0xf0,
	0x3400,0x04,
	0x0100,0x01,
};
kal_uint16 addr_data_pair_hs_video[] = {
	0x0303, 0x10,
	0x0317, 0x00,
	0x3501, 0x3 ,
	0x3502, 0xa0,
	0x3662, 0x0 ,
	0x370a, 0xa3,
	0x3808, 0x5 ,
	0x380a, 0x2 ,
	0x380b, 0xd0,
	0x380e, 0x3 ,
	0x380f, 0xc0,
	0x3811, 0x3 ,
	0x3813, 0x8 ,
	0x3815, 0x53,
	0x3820, 0x1 ,
	0x3834, 0x0 ,
	0x383d, 0xe3,
	0x3842, 0x20,
	0x4008, 0x0 ,
	0x4009, 0x3 ,
	0x4019, 0x6 ,
	0x4066, 0x2 ,
	0x4051, 0x1 ,
	0x4837, 0x11,
	0x4e37, 0x11,
	0x5500, 0x0 ,
	0x5502, 0x0 ,
	0x5504, 0x0 ,
	0x5506, 0x0 ,
	0x5508, 0x0 ,
	0x550a, 0x0 ,
	0x550c, 0x0 ,
	0x550e, 0x0 ,
	0x5511, 0x0 ,
	0x5513, 0x0 ,
	0x5515, 0x0 ,
	0x5517, 0x0 ,
	0x5520, 0x0 ,
	0x5522, 0x0 ,
	0x5524, 0x0 ,
	0x5526, 0x0 ,
	0x5529, 0x0 ,
	0x552b, 0x0 ,
	0x552d, 0x0 ,
	0x552f, 0x0 ,
	0x5531, 0x0 ,
	0x5533, 0x0 ,
	0x5535, 0x0 ,
	0x5537, 0x0 ,
	0x5540, 0x0 ,
	0x5542, 0x0 ,
	0x5544, 0x0 ,
	0x5546, 0x0 ,
	0x5548, 0x0 ,
	0x554a, 0x0 ,
	0x554c, 0x0 ,
	0x554e, 0x0 ,
	0x5551, 0x0 ,
	0x5553, 0x0 ,
	0x5555, 0x0 ,
	0x5557, 0x0 ,
	0x5560, 0x0 ,
	0x5562, 0x0 ,
	0x5564, 0x0 ,
	0x5566, 0x0 ,
	0x5569, 0x0 ,
	0x556b, 0x0 ,
	0x556d, 0x0 ,
	0x556f, 0x0 ,
	0x5571, 0x0 ,
	0x5573, 0x0 ,
	0x5575, 0x0 ,
	0x5577, 0x0 ,
	0x5580, 0x0 ,
	0x5582, 0x0 ,
	0x5584, 0x0 ,
	0x5586, 0x0 ,
	0x5588, 0x0 ,
	0x558a, 0x0 ,
	0x558c, 0x0 ,
	0x558e, 0x0 ,
	0x5590, 0x0 ,
	0x5591, 0x0 ,
	0x5592, 0x0 ,
	0x5593, 0x0 ,
	0x5595, 0x0 ,
	0x5597, 0x0 ,
	0x5599, 0x0 ,
	0x559b, 0x0 ,
	0x55a0, 0x0 ,
	0x55a1, 0x0 ,
	0x55a2, 0x0 ,
	0x55a3, 0x0 ,
	0x55a4, 0x0 ,
	0x55a5, 0x0 ,
	0x55a6, 0x0 ,
	0x55a7, 0x0 ,
	0x55a8, 0x0 ,
	0x55a9, 0x0 ,
	0x55aa, 0x0 ,
	0x55ab, 0x0 ,
	0x55ad, 0x0 ,
	0x55af, 0x0 ,
	0x55b1, 0x0 ,
	0x55b3, 0x0 ,
	0x55b5, 0x0 ,
	0x55b7, 0x0 ,
	0x55b9, 0x0 ,
	0x55bb, 0x0 ,
	0x55c0, 0x0 ,
	0x55c2, 0x0 ,
	0x55c4, 0x0 ,
	0x55c6, 0x0 ,
	0x55c8, 0x0 ,
	0x55c9, 0x0 ,
	0x55ca, 0x0 ,
	0x55cb, 0x0 ,
	0x55cc, 0x0 ,
	0x55ce, 0x0 ,
	0x55d0, 0x0 ,
	0x55d1, 0x0 ,
	0x55d2, 0x0 ,
	0x55d3, 0x0 ,
	0x55d5, 0x0 ,
	0x55d7, 0x0 ,
	0x55d9, 0x0 ,
	0x55db, 0x0 ,
	0x55e0, 0x0 ,
	0x55e1, 0x0 ,
	0x55e2, 0x0 ,
	0x55e3, 0x0 ,
	0x55e4, 0x0 ,
	0x55e5, 0x0 ,
	0x55e6, 0x0 ,
	0x55e7, 0x0 ,
	0x55e8, 0x0 ,
	0x55e9, 0x0 ,
	0x55ea, 0x0 ,
	0x55eb, 0x0 ,
	0x55ed, 0x0 ,
	0x55ef, 0x0 ,
	0x55f1, 0x0 ,
	0x55f3, 0x0 ,
	0x55f5, 0x0 ,
	0x55f7, 0x0 ,
	0x55f9, 0x0 ,
	0x55fb, 0x0 ,
	0x5693, 0x0 ,
	0x5694, 0x80,
	0x5d12, 0x0 ,
	0x5d13, 0x0 ,
	0x5d14, 0x0 ,
	0x5d15, 0x0 ,
	0x5d16, 0x0 ,
	0x5d17, 0x0 ,
	0x5d18, 0x0 ,
	0x5d19, 0x0 ,
	0x5d1a, 0x0 ,
	0x5d1b, 0x0 ,
	0x5d27, 0x0 ,
	0x5d28, 0x80,
	0x5d36, 0x90,
	0x5d1d, 0x0 ,
	0x5d1f, 0x0 ,
	0x5d20, 0x16,
	0x5d21, 0x20,
	0x5d22, 0x10,
	0x5d23, 0xa0,

};
kal_uint16 addr_data_pair_normal_video[] = 
{
	0x0303, 0x00,
	0x0317, 0x00,
	0x3501, 0xe ,
	0x3502, 0xe3,
	0x3662, 0x0 ,
	0x370a, 0x23,
	0x3808, 0x16,
	0x380a, 0xc ,
	0x380b, 0x60,
	0x380e, 0xf ,
	0x380f, 0x4 ,
	0x3811, 0x3 ,
	0x3813, 0x6 ,
	0x3815, 0x11,
	0x3820, 0x0 ,
	0x3834, 0x0 ,
	0x383d, 0xff,
	0x3842, 0x0 ,
	0x4008, 0x0 ,
	0x4009, 0x13,
	0x4019, 0x18,
	0x4066, 0x4 ,
	0x4051, 0x3 ,
	0x4837, 0x08,
	0x4e37, 0x11,
	0x5500, 0x0 ,
	0x5502, 0x0 ,
	0x5504, 0x0 ,
	0x5506, 0x0 ,
	0x5508, 0x10,
	0x550a, 0x10,
	0x550c, 0x20,
	0x550e, 0x20,
	0x5511, 0x0 ,
	0x5513, 0x0 ,
	0x5515, 0x0 ,
	0x5517, 0x0 ,
	0x5520, 0x0 ,
	0x5522, 0x0 ,
	0x5524, 0x0 ,
	0x5526, 0x0 ,
	0x5529, 0x10,
	0x552b, 0x10,
	0x552d, 0x20,
	0x552f, 0x20,
	0x5531, 0x0 ,
	0x5533, 0x0 ,
	0x5535, 0x0 ,
	0x5537, 0x0 ,
	0x5540, 0x0 ,
	0x5542, 0x0 ,
	0x5544, 0x0 ,
	0x5546, 0x0 ,
	0x5548, 0x1 ,
	0x554a, 0x1 ,
	0x554c, 0x2 ,
	0x554e, 0x2 ,
	0x5551, 0x0 ,
	0x5553, 0x0 ,
	0x5555, 0x0 ,
	0x5557, 0x0 ,
	0x5560, 0x0 ,
	0x5562, 0x0 ,
	0x5564, 0x0 ,
	0x5566, 0x0 ,
	0x5569, 0x10,
	0x556b, 0x10,
	0x556d, 0x20,
	0x556f, 0x20,
	0x5571, 0x0 ,
	0x5573, 0x0 ,
	0x5575, 0x0 ,
	0x5577, 0x0 ,
	0x5580, 0x0 ,
	0x5582, 0x0 ,
	0x5584, 0x10,
	0x5586, 0x10,
	0x5588, 0x38,
	0x558a, 0x38,
	0x558c, 0x70,
	0x558e, 0x70,
	0x5590, 0x20,
	0x5591, 0x0 ,
	0x5592, 0x20,
	0x5593, 0x0 ,
	0x5595, 0x0 ,
	0x5597, 0x0 ,
	0x5599, 0x0 ,
	0x559b, 0x0 ,
	0x55a0, 0x0 ,
	0x55a1, 0x0 ,
	0x55a2, 0x0 ,
	0x55a3, 0x0 ,
	0x55a4, 0x0 ,
	0x55a5, 0x10,
	0x55a6, 0x0 ,
	0x55a7, 0x10,
	0x55a8, 0x0 ,
	0x55a9, 0x38,
	0x55aa, 0x0 ,
	0x55ab, 0x38,
	0x55ad, 0x70,
	0x55af, 0x70,
	0x55b1, 0x20,
	0x55b3, 0x20,
	0x55b5, 0x0 ,
	0x55b7, 0x0 ,
	0x55b9, 0x0 ,
	0x55bb, 0x0 ,
	0x55c0, 0x0 ,
	0x55c2, 0x0 ,
	0x55c4, 0x1 ,
	0x55c6, 0x1 ,
	0x55c8, 0x3 ,
	0x55c9, 0x80,
	0x55ca, 0x3 ,
	0x55cb, 0x80,
	0x55cc, 0x7 ,
	0x55ce, 0x7 ,
	0x55d0, 0x2 ,
	0x55d1, 0x0 ,
	0x55d2, 0x2 ,
	0x55d3, 0x0 ,
	0x55d5, 0x0 ,
	0x55d7, 0x0 ,
	0x55d9, 0x0 ,
	0x55db, 0x0 ,
	0x55e0, 0x0 ,
	0x55e1, 0x0 ,
	0x55e2, 0x0 ,
	0x55e3, 0x0 ,
	0x55e4, 0x0 ,
	0x55e5, 0x10,
	0x55e6, 0x0 ,
	0x55e7, 0x10,
	0x55e8, 0x0 ,
	0x55e9, 0x38,
	0x55ea, 0x0 ,
	0x55eb, 0x38,
	0x55ed, 0x70,
	0x55ef, 0x70,
	0x55f1, 0x20,
	0x55f3, 0x20,
	0x55f5, 0x0 ,
	0x55f7, 0x0 ,
	0x55f9, 0x0 ,
	0x55fb, 0x0 ,
	0x5693, 0x1a,
	0x5694, 0x80,
	0x5d12, 0xf7,
	0x5d13, 0xdd,
	0x5d14, 0x97,
	0x5d15, 0x69,
	0x5d16, 0x61,
	0x5d17, 0x5b,
	0x5d18, 0x62,
	0x5d19, 0x8d,
	0x5d1a, 0xdd,
	0x5d1b, 0xff,
	0x5d27, 0x1a,
	0x5d28, 0x80,
	0x5d36, 0x90,
	0x5d1d, 0x0 ,
	0x5d1f, 0x0 ,
	0x5d20, 0x16,
	0x5d21, 0x20,
	0x5d22, 0x10,
	0x5d23, 0xa0,
};
kal_uint16 addr_data_pair_pdaf_on[] = {
	0x3016, 0xf0,
	0x3018, 0xf0,
	0x3661, 0x03,
	0x366c, 0x90,
	0x4605, 0x03,
	0x4640, 0x00,
	0x4645, 0x04,
	0x4809, 0x2b,/*Type data = raw10*/
	0x4813, 0x98,
	0x486e, 0x07,
	0x5021, 0x30,
	0x4641, 0x15,/*package size = 168 pixels*/
	0x4643, 0x08,/*Fix H-size*/
	0x5d1c,0x00,
	0x5d1d,0x90,
	0x5d1e,0x00,
	0x5d1f,0x50,
	0x5d20,0x15,
	0x5d21,0x00,
	0x5d22,0x10,
	0x5d23,0x00,
};

kal_uint16 addr_data_pair_pdaf_off[] = {
	0x3016, 0xd2,
	0x3018, 0x70,
	0x3661, 0x00,
	0x366c, 0x80,
	0x4605, 0x00,
	0x4640, 0x01,
	0x4641, 0x04,
	0x4643, 0x00,
	0x4645, 0x00,
	0x4809, 0x2b,
	0x4813, 0x90,
	0x486e, 0x03,
	0x5021, 0x00,
};

kal_uint16 addr_data_pair_cap[] =
{
0x0303,0x0 ,
0x0317,0x0 ,
0x3501,0x12,
0x3502,0xa6,
0x3662,0x0 ,
0x370a,0x23,
0x3808,0x16,
0x380a,0x10,
0x380b,0x80,
0x380e,0x12,
0x380f,0xc6,
0x3811,0x3 ,
0x3813,0x6 ,
0x3815,0x11,
0x3820,0x0 ,
0x3834,0x0 ,
0x383d,0xff,
0x3842,0x0 ,
0x4008,0x0 ,
0x4009,0x13,
0x4019,0x18,
0x4066,0x4 ,
0x4051,0x3 ,
0x4837,0x8 ,
0x4e37,0x8 ,
0x5500,0x0 ,
0x5502,0x0 ,
0x5504,0x0 ,
0x5506,0x0 ,
0x5508,0x10,
0x550a,0x10,
0x550c,0x20,
0x550e,0x20,
0x5511,0x0 ,
0x5513,0x0 ,
0x5515,0x0 ,
0x5517,0x0 ,
0x5520,0x0 ,
0x5522,0x0 ,
0x5524,0x0 ,
0x5526,0x0 ,
0x5529,0x10,
0x552b,0x10,
0x552d,0x20,
0x552f,0x20,
0x5531,0x0 ,
0x5533,0x0 ,
0x5535,0x0 ,
0x5537,0x0 ,
0x5540,0x0 ,
0x5542,0x0 ,
0x5544,0x0 ,
0x5546,0x0 ,
0x5548,0x1 ,
0x554a,0x1 ,
0x554c,0x2 ,
0x554e,0x2 ,
0x5551,0x0 ,
0x5553,0x0 ,
0x5555,0x0 ,
0x5557,0x0 ,
0x5560,0x0 ,
0x5562,0x0 ,
0x5564,0x0 ,
0x5566,0x0 ,
0x5569,0x10,
0x556b,0x10,
0x556d,0x20,
0x556f,0x20,
0x5571,0x0 ,
0x5573,0x0 ,
0x5575,0x0 ,
0x5577,0x0 ,
0x5580,0x0 ,
0x5582,0x0 ,
0x5584,0x10,
0x5586,0x10,
0x5588,0x38,
0x558a,0x38,
0x558c,0x70,
0x558e,0x70,
0x5590,0x20,
0x5591,0x0 ,
0x5592,0x20,
0x5593,0x0 ,
0x5595,0x0 ,
0x5597,0x0 ,
0x5599,0x0 ,
0x559b,0x0 ,
0x55a0,0x0 ,
0x55a1,0x0 ,
0x55a2,0x0 ,
0x55a3,0x0 ,
0x55a4,0x0 ,
0x55a5,0x10,
0x55a6,0x0 ,
0x55a7,0x10,
0x55a8,0x0 ,
0x55a9,0x38,
0x55aa,0x0 ,
0x55ab,0x38,
0x55ad,0x70,
0x55af,0x70,
0x55b1,0x20,
0x55b3,0x20,
0x55b5,0x0 ,
0x55b7,0x0 ,
0x55b9,0x0 ,
0x55bb,0x0 ,
0x55c0,0x0 ,
0x55c2,0x0 ,
0x55c4,0x1 ,
0x55c6,0x1 ,
0x55c8,0x3 ,
0x55c9,0x80,
0x55ca,0x3 ,
0x55cb,0x80,
0x55cc,0x7 ,
0x55ce,0x7 ,
0x55d0,0x2 ,
0x55d1,0x0 ,
0x55d2,0x2 ,
0x55d3,0x0 ,
0x55d5,0x0 ,
0x55d7,0x0 ,
0x55d9,0x0 ,
0x55db,0x0 ,
0x55e0,0x0 ,
0x55e1,0x0 ,
0x55e2,0x0 ,
0x55e3,0x0 ,
0x55e4,0x0 ,
0x55e5,0x10,
0x55e6,0x0 ,
0x55e7,0x10,
0x55e8,0x0 ,
0x55e9,0x38,
0x55ea,0x0 ,
0x55eb,0x38,
0x55ed,0x70,
0x55ef,0x70,
0x55f1,0x20,
0x55f3,0x20,
0x55f5,0x0 ,
0x55f7,0x0 ,
0x55f9,0x0 ,
0x55fb,0x0 ,
0x5693,0xa ,
0x5694,0x80,
0x5d12,0xf7,
0x5d13,0xdd,
0x5d14,0x97,
0x5d15,0x69,
0x5d16,0x61,
0x5d17,0x5b,
0x5d18,0x62,
0x5d19,0x8d,
0x5d1a,0xdd,
0x5d1b,0xff,
0x5d27,0xa ,
0x5d28,0x80,
0x5d36,0x90,
0x5d1d,0x90,
0x5d1f,0x50,
0x5d20,0x15,
0x5d21,0x0 ,
0x5d22,0x10,
0x5d23,0x0 ,
};
kal_uint16 addr_data_pair_prv[] = {
	0x0303,0x10,
	0x0317,0x1 ,
	0x3501,0x9 ,
	0x3502,0xe1, 
	0x3662,0x10,
	0x370a,0x63, 
	0x3808,0xb ,
	0x380a,0x8 ,
	0x380b,0x40, 
	0x380e,0xa ,
	0x380f,0x2 ,
	0x3811,0x7 ,
	0x3813,0x8 ,
	0x3815,0x31, 
	0x3820,0x1 ,
	0x3834,0x1 ,
	0x383d,0xef, 
	0x3842,0x40,
	0x4008,0x2 ,
	0x4009,0x9 ,
	0x4019,0xc ,
	0x4066,0x2 ,
	0x4051,0x1 ,
	0x4837,0x11,
	0x4e37,0x11,
	0x5500,0x10,
	0x5502,0x10,
	0x5504,0x20,
	0x5506,0x20,
	0x5508,0x0 ,
	0x550a,0x0 ,
	0x550c,0x0 ,
	0x550e,0x0 ,
	0x5511,0x10,
	0x5513,0x10,
	0x5515,0x20,
	0x5517,0x20,
	0x5520,0x1 ,
	0x5522,0x1 ,
	0x5524,0x2 ,
	0x5526,0x2 ,
	0x5529,0x0 ,
	0x552b,0x0 ,
	0x552d,0x0 ,
	0x552f,0x0 ,
	0x5531,0x10,
	0x5533,0x10,
	0x5535,0x20,
	0x5537,0x20,
	0x5540,0x10,
	0x5542,0x10,
	0x5544,0x20,
	0x5546,0x20,
	0x5548,0x0 ,
	0x554a,0x0 ,
	0x554c,0x0 ,
	0x554e,0x0 ,
	0x5551,0x10,
	0x5553,0x10,
	0x5555,0x20,
	0x5557,0x20,
	0x5560,0x1 ,
	0x5562,0x1 ,
	0x5564,0x2 ,
	0x5566,0x2 ,
	0x5569,0x0 ,
	0x556b,0x0 ,
	0x556d,0x0 ,
	0x556f,0x0 ,
	0x5571,0x10,
	0x5573,0x10,
	0x5575,0x20,
	0x5577,0x20,
	0x5580,0x38,
	0x5582,0x38,
	0x5584,0x70, 
	0x5586,0x70, 
	0x5588,0x20, 
	0x558a,0x20, 
	0x558c,0x0 ,
	0x558e,0x0 ,
	0x5590,0x0 ,
	0x5591,0x38,
	0x5592,0x0 ,
	0x5593,0x38,
	0x5595,0x70,
	0x5597,0x70,
	0x5599,0x20,
	0x559b,0x20,
	0x55a0,0x3 ,
	0x55a1,0x80,
	0x55a2,0x3 ,
	0x55a3,0x80,
	0x55a4,0x7 ,
	0x55a5,0x0 ,
	0x55a6,0x7 ,
	0x55a7,0x0 ,
	0x55a8,0x2 ,
	0x55a9,0x0 ,
	0x55aa,0x2 ,
	0x55ab,0x0 ,
	0x55ad,0x0 ,
	0x55af,0x0 ,
	0x55b1,0x38, 
	0x55b3,0x38, 
	0x55b5,0x70,
	0x55b7,0x70,
	0x55b9,0x20,
	0x55bb,0x20,
	0x55c0,0x38,
	0x55c2,0x38,
	0x55c4,0x70,
	0x55c6,0x70,
	0x55c8,0x20,
	0x55c9,0x0 ,
	0x55ca,0x20,
	0x55cb,0x0 ,
	0x55cc,0x0 ,
	0x55ce,0x0 ,
	0x55d0,0x0 ,
	0x55d1,0x38,
	0x55d2,0x0 ,
	0x55d3,0x38,
	0x55d5,0x70,
	0x55d7,0x70,
	0x55d9,0x20,
	0x55db,0x20,
	0x55e0,0x3 ,
	0x55e1,0x80,
	0x55e2,0x3 ,
	0x55e3,0x80,
	0x55e4,0x7 ,
	0x55e5,0x0 ,
	0x55e6,0x7 ,
	0x55e7,0x0 ,
	0x55e8,0x2 ,
	0x55e9,0x0 ,
	0x55ea,0x2 ,
	0x55eb,0x0 ,
	0x55ed,0x0 ,
	0x55ef,0x0 ,
	0x55f1,0x38, 
	0x55f3,0x38, 
	0x55f5,0x70,
	0x55f7,0x70,
	0x55f9,0x20,
	0x55fb,0x20,
	0x5693,0x0 ,
	0x5694,0xb0, 
	0x5d12,0x5e, 
	0x5d13,0x5c, 
	0x5d14,0x53, 
	0x5d15,0x4a, 
	0x5d16,0x48, 
	0x5d17,0x46, 
	0x5d18,0x48, 
	0x5d19,0x52, 
	0x5d1a,0x5c, 
	0x5d1b,0x5e, 
	0x5d27,0x0 ,
	0x5d28,0xb0, 
	0x5d36,0x50, 
	0x5d1d,0x0 ,
	0x5d1f,0x0 ,
	0x5d20,0xa ,
	0x5d21,0xf0,
	0x5d22,0x8 ,
	0x5d23,0x20,
};
kal_uint16 addr_data_pair_init[] =
{             
	0x0300,0x00,
	0x0301,0x64,
	0x0302,0x10,
	0x0303,0x00,
	0x0304,0x47,
	0x030d,0x2d,
	0x030e,0x00,
	0x030f,0x05,
	0x0312,0x02,
	0x0313,0x11,
	0x0316,0x1e,
	0x0317,0x00,
	0x0318,0x03,
	0x031c,0x01,
	0x031d,0x02,
	0x031e,0x09,
	0x2b05,0x02,
	0x2b06,0x87,
	0x2b07,0x01,
	0x2b08,0xa8,
	0x0320,0x02,
	0x3002,0x00,
	0x300f,0x11,
	0x3010,0x01,
	0x3012,0x41,
	0x3016,0xd2,
	0x3018,0x70,
	0x3019,0xc3,
	0x301b,0x96,
	0x3022,0x0f,
	0x3023,0xb4,
	0x3031,0x91,
	0x3034,0x41,
	0x340c,0xff,
	0x3501,0x12,
	0x3502,0xa6,
	0x3503,0x00,
	0x3507,0x00,
	0x3508,0x00,
	0x3509,0x12,
	0x350a,0x00,
	0x350b,0x80,
	0x350f,0x10,
	0x3541,0x02,
	0x3542,0x00,
	0x3543,0x00,
	0x3547,0x00,
	0x3548,0x00,
	0x3549,0x12,
	0x354b,0x10,
	0x354f,0x10,
	0x3601,0xa2,
	0x3603,0x97,
	0x3604,0x02,
	0x3605,0xf2,
	0x3606,0x88,
	0x3607,0x11,
	0x360a,0x34,
	0x360c,0x13,
	0x3618,0xcc,
	0x3620,0x50,
	0x3621,0x99,
	0x3622,0x7d,
	0x3624,0x05,
	0x362a,0x25,
	0x3650,0x04,
	0x3660,0xc0,
	0x3661,0x00,
	0x3662,0x00,
	0x3664,0x88,
	0x3667,0x00,
	0x366a,0x5c,
	0x366c,0x80,
	0x3700,0x62,
	0x3701,0x08,
	0x3702,0x10,
	0x3703,0x3e,
	0x3704,0x26,
	0x3705,0x01,
	0x3706,0x3a,
	0x3707,0xc4,
	0x3708,0x3c,
	0x3709,0x1c,
	0x370a,0x23,
	0x370b,0x2c,
	0x370c,0x42,
	0x370d,0xa4,
	0x370e,0x14,
	0x370f,0x0a,
	0x3710,0x15,
	0x3711,0x0a,
	0x3712,0xa2,
	0x3713,0x00,
	0x371e,0x2a,
	0x371f,0x13,
	0x3714,0x00,
	0x3717,0x00,
	0x3719,0x00,
	0x371c,0x04,
	0x3720,0xaa,
	0x3721,0x10,
	0x3722,0x50,
	0x3725,0xf0,
	0x3726,0x22,
	0x3727,0x44,
	0x3728,0x40,
	0x3729,0x00,
	0x372b,0x00,
	0x372c,0x92,
	0x372d,0x0c,
	0x372e,0x22,
	0x372f,0x91,
	0x3732,0x01,
	0x3733,0xd0,
	0x3730,0x01,
	0x3731,0xc8,
	0x3744,0x01,
	0x3745,0x24,
	0x3746,0x00,
	0x3747,0xd0,
	0x3748,0x27,
	0x374a,0x4b,
	0x374b,0x44,
	0x3760,0xd1,
	0x3761,0x52,
	0x3762,0xa4,
	0x3763,0x14,
	0x3766,0x0c,
	0x3767,0x25,
	0x3768,0x0c,
	0x3769,0x24,
	0x376a,0x09,
	0x376b,0x02,
	0x376d,0x01,
	0x376e,0x53,
	0x376f,0x01,
	0x378c,0x08,
	0x378d,0x46,
	0x378e,0x14,
	0x378f,0x02,
	0x3790,0xc4,
	0x3792,0x64,
	0x3793,0x5d,
	0x3794,0x29,
	0x3795,0x4f,
	0x3796,0x43,
	0x3797,0x09,
	0x3798,0x02,
	0x3799,0x33,
	0x379a,0x09,
	0x379b,0x1e,
	0x379f,0x3e,
	0x37a0,0x44,
	0x37a1,0x00,
	0x37a2,0x44,
	0x37a3,0x41,
	0x37a4,0x88,
	0x37a5,0x69,
	0x37b0,0x48,
	0x37b1,0x20,
	0x37b2,0x03,
	0x37b3,0x48,
	0x37b4,0x02,
	0x37b5,0x33,
	0x37b6,0x22,
	0x37b8,0x02,
	0x37bc,0x02,
	0x37c0,0x3b,
	0x37c1,0xc2,
	0x37c2,0x06,
	0x37c3,0x06,
	0x37c5,0x33,
	0x37c6,0x35,
	0x37c7,0x00,
	0x3800,0x00,
	0x3801,0x14,
	0x3802,0x00,
	0x3803,0x0c,
	0x3804,0x10,
	0x3805,0x8b,
	0x3806,0x0c,
	0x3807,0x43,
	0x3808,0x16,
	0x3809,0x00,
	0x380a,0x10,
	0x380b,0x80,
	0x380c,0x18,
	0x380d,0x60,
	0x380e,0x12,
	0x380f,0xc6,
	0x3810,0x00,
	0x3811,0x03,
	0x3813,0x06,
	0x3814,0x11,
	0x3815,0x11,
	0x3820,0x00,
	0x3821,0x04,
	0x3834,0x00,
	0x3835,0x04,
	0x3836,0x18,
	0x3837,0x02,
	0x382f,0x84,
	0x383c,0xc8,
	0x383d,0xff,
	0x3842,0x00,
	0x384b,0x00,
	0x3d85,0x16,
	0x3d8c,0x77,
	0x3d8d,0x10,
	0x3f00,0x52,
	0x4000,0x17,
	0x4001,0x60,
	0x4001,0x60,
	0x4008,0x00,
	0x4009,0x13,
	0x400f,0x00,
	0x4011,0xfb,
	0x4017,0x08,
	0x4018,0x00,
	0x401a,0xce,
	0x4019,0x18,
	0x4020,0x08,
	0x4022,0x08,
	0x4024,0x08,
	0x4026,0x08,
	0x4028,0x08,
	0x402a,0x08,
	0x402c,0x08,
	0x402e,0x08,
	0x4030,0x08,
	0x4032,0x08,
	0x4034,0x08,
	0x4036,0x08,
	0x4038,0x08,
	0x403a,0x08,
	0x403c,0x08,
	0x403e,0x08,
	0x405c,0x3f,
	0x4066,0x04,
	0x4051,0x03,
	0x4052,0x00,
	0x4053,0x80,
	0x4054,0x00,
	0x4055,0x80,
	0x4056,0x00,
	0x4057,0x80,
	0x4058,0x00,
	0x4059,0x80,
	0x4202,0x00,
	0x4203,0x01,
	0x430b,0xff,
	0x430d,0x00,
	0x4500,0x72,
	0x4605,0x00,
	0x4640,0x01,
	0x4641,0x04,
	0x4645,0x00,
	0x4800,0x04,
	0x4809,0x2b,
	0x4813,0x90,
	0x4817,0x04,
	0x4833,0x18,
	0x4837,0x08,
	0x484b,0x01,
	0x4850,0x5c,
	0x4852,0x27,
	0x4856,0x5c,
	0x4857,0x55,
	0x486a,0xaa,
	0x486e,0x03,
	0x486f,0x55,
	0x4875,0xf0,
	0x4b04,0x80,
	0x4b05,0xb3,
	0x4b06,0x00,
	0x4c01,0xdf,
	0x4d00,0x04,
	0x4d01,0xf0,
	0x4d02,0xb8,
	0x4d03,0xf2,
	0x4d04,0x88,
	0x4d05,0x9d,
	0x4e00,0x04,
	0x4e17,0x04,
	0x4e33,0x18,
	0x4e37,0x08,
	0x4e4b,0x01,
	0x4e50,0x5c,
	0x4e52,0x27,
	0x4e56,0x5c,
	0x4e57,0x55,
	0x4e6a,0xaa,
	0x4e6e,0x03,
	0x4e6f,0x55,
	0x4e75,0xf0,
	0x5000,0x9b,
	0x5001,0x42,
	0x5002,0x10,
	0x5003,0x01,
	0x5004,0x00,
	0x5005,0x00,
	0x501d,0x00,
	0x501f,0x06,
	0x5020,0x03,
	0x5021,0x00,
	0x5022,0x13,
	0x5061,0xff,
	0x5062,0xff,
	0x5063,0xff,
	0x5064,0xff,
	0x506f,0x00,
	0x5280,0x00,
	0x5282,0x00,
	0x5283,0x01,
	0x5200,0x00,
	0x5201,0x71,
	0x5203,0x04,
	0x5204,0x00,
	0x5205,0x88,
	0x5209,0x00,
	0x520a,0x80,
	0x520b,0x04,
	0x520c,0x01,
	0x5210,0x10,
	0x5211,0xa0,
	0x5292,0x04,
	0x5500,0x00,
	0x5501,0x00,
	0x5502,0x00,
	0x5503,0x00,
	0x5504,0x00,
	0x5505,0x00,
	0x5506,0x00,
	0x5507,0x00,
	0x5508,0x10,
	0x5509,0x00,
	0x550a,0x10,
	0x550b,0x00,
	0x550c,0x20,
	0x550d,0x00,
	0x550e,0x20,
	0x550f,0x00,
	0x5510,0x00,
	0x5511,0x00,
	0x5512,0x00,
	0x5513,0x00,
	0x5514,0x00,
	0x5515,0x00,
	0x5516,0x00,
	0x5517,0x00,
	0x5518,0x00,
	0x5519,0x00,
	0x551a,0x00,
	0x551b,0x00,
	0x551c,0x00,
	0x551d,0x00,
	0x551e,0x00,
	0x551f,0x00,
	0x5520,0x00,
	0x5521,0x00,
	0x5522,0x00,
	0x5523,0x00,
	0x5524,0x00,
	0x5525,0x00,
	0x5526,0x00,
	0x5527,0x00,
	0x5528,0x00,
	0x5529,0x10,
	0x552a,0x00,
	0x552b,0x10,
	0x552c,0x00,
	0x552d,0x20,
	0x552e,0x00,
	0x552f,0x20,
	0x5530,0x00,
	0x5531,0x00,
	0x5532,0x00,
	0x5533,0x00,
	0x5534,0x00,
	0x5535,0x00,
	0x5536,0x00,
	0x5537,0x00,
	0x5538,0x00,
	0x5539,0x00,
	0x553a,0x00,
	0x553b,0x00,
	0x553c,0x00,
	0x553d,0x00,
	0x553e,0x00,
	0x553f,0x00,
	0x5540,0x00,
	0x5541,0x00,
	0x5542,0x00,
	0x5543,0x00,
	0x5544,0x00,
	0x5545,0x00,
	0x5546,0x00,
	0x5547,0x00,
	0x5548,0x01,
	0x5549,0x00,
	0x554a,0x01,
	0x554b,0x00,
	0x554c,0x02,
	0x554d,0x00,
	0x554e,0x02,
	0x554f,0x00,
	0x5550,0x00,
	0x5551,0x00,
	0x5552,0x00,
	0x5553,0x00,
	0x5554,0x00,
	0x5555,0x00,
	0x5556,0x00,
	0x5557,0x00,
	0x5558,0x00,
	0x5559,0x00,
	0x555a,0x00,
	0x555b,0x00,
	0x555c,0x00,
	0x555d,0x00,
	0x555e,0x00,
	0x555f,0x00,
	0x5560,0x00,
	0x5561,0x00,
	0x5562,0x00,
	0x5563,0x00,
	0x5564,0x00,
	0x5565,0x00,
	0x5566,0x00,
	0x5567,0x00,
	0x5568,0x00,
	0x5569,0x10,
	0x556a,0x00,
	0x556b,0x10,
	0x556c,0x00,
	0x556d,0x20,
	0x556e,0x00,
	0x556f,0x20,
	0x5570,0x00,
	0x5571,0x00,
	0x5572,0x00,
	0x5573,0x00,
	0x5574,0x00,
	0x5575,0x00,
	0x5576,0x00,
	0x5577,0x00,
	0x5578,0x00,
	0x5579,0x00,
	0x557a,0x00,
	0x557b,0x00,
	0x557c,0x00,
	0x557d,0x00,
	0x557e,0x00,
	0x557f,0x00,
	0x5580,0x00,
	0x5581,0x00,
	0x5582,0x00,
	0x5583,0x00,
	0x5584,0x10,
	0x5585,0x00,
	0x5586,0x10,
	0x5587,0x00,
	0x5588,0x38,
	0x5589,0x00,
	0x558a,0x38,
	0x558b,0x00,
	0x558c,0x70,
	0x558d,0x00,
	0x558e,0x70,
	0x558f,0x00,
	0x5590,0x20,
	0x5591,0x00,
	0x5592,0x20,
	0x5593,0x00,
	0x5594,0x00,
	0x5595,0x00,
	0x5596,0x00,
	0x5597,0x00,
	0x5598,0x00,
	0x5599,0x00,
	0x559a,0x00,
	0x559b,0x00,
	0x559c,0x00,
	0x559d,0x00,
	0x559e,0x00,
	0x559f,0x00,
	0x55a0,0x00,
	0x55a1,0x00,
	0x55a2,0x00,
	0x55a3,0x00,
	0x55a4,0x00,
	0x55a5,0x10,
	0x55a6,0x00,
	0x55a7,0x10,
	0x55a8,0x00,
	0x55a9,0x38,
	0x55aa,0x00,
	0x55ab,0x38,
	0x55ac,0x00,
	0x55ad,0x70,
	0x55ae,0x00,
	0x55af,0x70,
	0x55b0,0x00,
	0x55b1,0x20,
	0x55b2,0x00,
	0x55b3,0x20,
	0x55b4,0x00,
	0x55b5,0x00,
	0x55b6,0x00,
	0x55b7,0x00,
	0x55b8,0x00,
	0x55b9,0x00,
	0x55ba,0x00,
	0x55bb,0x00,
	0x55bc,0x00,
	0x55bd,0x00,
	0x55be,0x00,
	0x55bf,0x00,
	0x55c0,0x00,
	0x55c1,0x00,
	0x55c2,0x00,
	0x55c3,0x00,
	0x55c4,0x01,
	0x55c5,0x00,
	0x55c6,0x01,
	0x55c7,0x00,
	0x55c8,0x03,
	0x55c9,0x80,
	0x55ca,0x03,
	0x55cb,0x80,
	0x55cc,0x07,
	0x55cd,0x00,
	0x55ce,0x07,
	0x55cf,0x00,
	0x55d0,0x02,
	0x55d1,0x00,
	0x55d2,0x02,
	0x55d3,0x00,
	0x55d4,0x00,
	0x55d5,0x00,
	0x55d6,0x00,
	0x55d7,0x00,
	0x55d8,0x00,
	0x55d9,0x00,
	0x55da,0x00,
	0x55db,0x00,
	0x55dc,0x00,
	0x55dd,0x00,
	0x55de,0x00,
	0x55df,0x00,
	0x55e0,0x00,
	0x55e1,0x00,
	0x55e2,0x00,
	0x55e3,0x00,
	0x55e4,0x00,
	0x55e5,0x10,
	0x55e6,0x00,
	0x55e7,0x10,
	0x55e8,0x00,
	0x55e9,0x38,
	0x55ea,0x00,
	0x55eb,0x38,
	0x55ec,0x00,
	0x55ed,0x70,
	0x55ee,0x00,
	0x55ef,0x70,
	0x55f0,0x00,
	0x55f1,0x20,
	0x55f2,0x00,
	0x55f3,0x20,
	0x55f4,0x00,
	0x55f5,0x00,
	0x55f6,0x00,
	0x55f7,0x00,
	0x55f8,0x00,
	0x55f9,0x00,
	0x55fa,0x00,
	0x55fb,0x00,
	0x55fc,0x00,
	0x55fd,0x00,
	0x55fe,0x00,
	0x55ff,0x00,
	0x5600,0x30,
	0x5601,0x00,
	0x5602,0x00,
	0x5603,0x00,
	0x5604,0x00,
	0x5605,0x00,
	0x5606,0x00,
	0x5607,0x01,
	0x5608,0x01,
	0x5609,0x01,
	0x560f,0xfc,
	0x5610,0xf0,
	0x5611,0x10,
	0x562f,0xfc,
	0x5630,0xf0,
	0x5631,0x10,
	0x564f,0xfc,
	0x5650,0xf0,
	0x5651,0x10,
	0x566f,0xfc,
	0x5670,0xf0,
	0x5671,0x10,
	0x567b,0x40,
	0x5690,0x00,
	0x5691,0x00,
	0x5692,0x00,
	0x5693,0x0a,
	0x5694,0x80,
	0x5696,0x06,
	0x5697,0x0a,
	0x5698,0x00,
	0x5699,0x90,
	0x569a,0x15,
	0x569b,0x90,
	0x569c,0x00,
	0x569d,0x50,
	0x569e,0x10,
	0x569f,0x50,
	0x56a0,0x36,
	0x56a1,0x50,
	0x56a2,0x16,
	0x56a3,0x20,
	0x56a4,0x10,
	0x56a5,0xa0,
	0x5c80,0x06,
	0x5c81,0x80,
	0x5c82,0x09,
	0x5c83,0x5f,
	0x5d04,0x01,
	0x5d05,0x1a,
	0x5d06,0x01,
	0x5d07,0x1a,
	0x5d12,0xf7,
	0x5d13,0xdd,
	0x5d14,0x97,
	0x5d15,0x69,
	0x5d16,0x61,
	0x5d17,0x5b,
	0x5d18,0x62,
	0x5d19,0x8d,
	0x5d1a,0xdd,
	0x5d1b,0xff,
	0x5d24,0x00,
	0x5d25,0x00,
	0x5d26,0x00,
	0x5d27,0x0a,
	0x5d28,0x80,
	0x5d2a,0x00,
	0x5d2b,0x90,
	0x5d2c,0x15,
	0x5d2d,0x90,
	0x5d2e,0x00,
	0x5d2f,0x50,
	0x5d30,0x10,
	0x5d31,0x50,
	0x5d32,0x10,
	0x5d34,0x36,
	0x5d35,0x20,
	0x5d36,0x90,
	0x5d37,0xa0,
	0x5d38,0x5c,
	0x5d39,0x7b,
	0x5d1c,0x00,
	0x5d1d,0x90,
	0x5d1e,0x00,
	0x5d1f,0x50,
	0x5d20,0x15,
	0x5d21,0x00,
	0x5d22,0x10,
	0x5d23,0x00,
	0x5d29,0xc0,
	0x3008,0x01,
	0x3663,0x60,
	0x3002,0x01,
	0x3c00,0x3c,
	0x3025,0x03,
	0x3668,0xf0,
	0x3400,0x04,
};
static void sensor_init(void)
{
//int i = 0 ;
LOG_INF("sensor_init MULTI_WRITE");
write_cmos_sensor(0x0103, 0x01);//SW Reset  , need delay 
mdelay(10);
ov23850_MIPI_table_write_cmos_sensor(addr_data_pair_init, sizeof(addr_data_pair_init)/sizeof(kal_uint16));
#if 0 //for debug
for(i=0;i<(sizeof(addr_data_pair_init)/sizeof(kal_uint16));i+=2){
	LOG_INF("read_cmos_sensor(0x%x)     0x%x\n",addr_data_pair_init[i],read_cmos_sensor(addr_data_pair_init[i]));
}
#endif

}

#define PDAF_ON  1
#define PDAF_OFF 0
static void ov23850_setting_PDAF(int enable)
{
#if defined(ENABLE_PDAF_VC)
	LOG_INF("PDAF is %d\n",enable);
	if(enable)
		ov23850_MIPI_table_write_cmos_sensor(addr_data_pair_pdaf_on, sizeof(addr_data_pair_pdaf_on)/sizeof(kal_uint16));
	else
		ov23850_MIPI_table_write_cmos_sensor(addr_data_pair_pdaf_off, sizeof(addr_data_pair_pdaf_off)/sizeof(kal_uint16));
#else
	LOG_INF("PDAF is %d\n",enable);
#endif
}

static void ov23850_setting_Deskew(int enable)
{
#if defined(HW_DESKEW_ENABLE)
	if(enable){
		/*Deskew funciton*/
		write_cmos_sensor(0x4800, 0x64);//r4800 = r4800 | 60, clk gate en
		write_cmos_sensor(0x484b, 0x03);//r484b = r484b|02 ; [1] clk start after mipi rst
		write_cmos_sensor(0x4850, 0x5c);//[6] eof_busy_en
										//[5] one_time_one_lane
										//[4] r_wait_pa_cal
										//[3] r_deskew_auto_en
										//[2] r_frame_skew_en
										//[1] r_deskew_manual1
										//[0] r_deskew_manual0
		
		write_cmos_sensor(0x4852, 0x27);//4852 06 ; 27 ; ;deskew length for periodic
		write_cmos_sensor(0x4856, 0x5e);//r4856 = r4856 & fc | 02;[1:0] 01: DPHY plus, 10: dphy12 ,11: cphy 
		write_cmos_sensor(0x4857, 0xaa);//4857 aa ; 55 ;r_mode12_hsdat
		write_cmos_sensor(0x486f, 0x55);//486f 55 ; ;r_mode12_clk_data

	}
	else{
		/*Deskew funciton*/
		write_cmos_sensor(0x4800, 0x04);//r4800 = r4800 | 60, clk gate en
		write_cmos_sensor(0x484b, 0x01);//r484b = r484b|02 ; [1] clk start after mipi rst
	}
#endif
}

static void preview_setting(void)
{
	int retry=0;
	int frame_cnt = 0;
	LOG_INF("preview_setting\n");
	write_cmos_sensor(0x0100,0x00);
	mdelay(1);
	//Deskew
	ov23850_setting_Deskew(0);
	
	ov23850_MIPI_table_write_cmos_sensor(addr_data_pair_prv, sizeof(addr_data_pair_prv)/sizeof(kal_uint16));
	
	/*None PDAF focus windows range*/
	ov23850_setting_PDAF(PDAF_OFF);

	write_cmos_sensor(0x0100,0x01);
	frame_cnt = read_cmos_sensor(0x3863);
	while(retry<10)
	{
		if(frame_cnt == read_cmos_sensor(0x3863))
		{
			msleep(5);
			retry++;
			//LOG_INF("Sensor has not output stream %x\n",read_cmos_sensor(0x3863));
		}
		else
		{
			LOG_INF("Sensor has output(%x), retry(%x)\n", read_cmos_sensor(0x3863), retry);
			retry=0;
			break;
		}
	}
}

static void capture_setting(kal_uint16 currefps)
{
	int retry=0;
	int frame_cnt = 0;
	LOG_INF("capture_setting\n");
	write_cmos_sensor(0x0100,0x00);
	mdelay(1);
	//Deskew
	ov23850_setting_Deskew(1);
	
	ov23850_MIPI_table_write_cmos_sensor(addr_data_pair_cap, sizeof(addr_data_pair_cap)/sizeof(kal_uint16));

	/*None PDAF focus windows range*/
    if(imgsensor.pdaf_mode == 1)
		ov23850_setting_PDAF(PDAF_ON);
    else
		ov23850_setting_PDAF(PDAF_OFF);

	write_cmos_sensor(0x0100, 0x01);

	while(retry<10)
	{
		frame_cnt = read_cmos_sensor(0x3863);
		if(frame_cnt==0x00)
		{
			msleep(5);
			retry++;
			//LOG_INF("Sensor has not output stream %x\n",read_cmos_sensor(0x3863));
		}
		else
		{
			LOG_INF("Sensor has output(%x), retry(%x)\n", read_cmos_sensor(0x3863), retry);
			retry=0;
			break;
		}
	}
}

static void normal_video_setting(kal_uint16 currefps)//1080p
{
	write_cmos_sensor(0x0100,0x00);
	mdelay(1);
	//Deskew
	ov23850_setting_Deskew(0);
	ov23850_MIPI_table_write_cmos_sensor(addr_data_pair_normal_video, sizeof(addr_data_pair_normal_video)/sizeof(kal_uint16));

	if(imgsensor.pdaf_mode == 1)
		ov23850_setting_PDAF(PDAF_ON);
	else
		ov23850_setting_PDAF(PDAF_OFF);

	write_cmos_sensor(0x0100,0x01);
}

static void hs_video_setting(void)
{
	write_cmos_sensor(0x0100,0x00);
	mdelay(1);
	//Deskew
	ov23850_setting_Deskew(0);
	ov23850_MIPI_table_write_cmos_sensor(addr_data_pair_hs_video, sizeof(addr_data_pair_hs_video)/sizeof(kal_uint16));
	ov23850_setting_PDAF(PDAF_OFF);
	write_cmos_sensor(0x0100,0x01);
}

static void slim_video_setting(void)//1280x720
{
	write_cmos_sensor(0x0100,0x00);
	mdelay(1);
	//Deskew
	ov23850_setting_Deskew(0);
	ov23850_MIPI_table_write_cmos_sensor(addr_data_pair_slim_video, sizeof(addr_data_pair_slim_video)/sizeof(kal_uint16));
	ov23850_setting_PDAF(PDAF_OFF);
	write_cmos_sensor(0x0100,0x01);

}

static kal_uint32 return_sensor_id(void)
{
    return ( (read_cmos_sensor(0x300a) << 16) | (read_cmos_sensor(0x300b) << 8) | read_cmos_sensor(0x300c));
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
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
                return ERROR_NONE;
            }
            LOG_INF("Read sensor id fail:0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        retry = 1;
    }
    if (*sensor_id != imgsensor_info.sensor_id) {
        *sensor_id = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
#if 0
	eeprom_read_data();

	//module_id = eeprom_read_cmos_sensor(0x00);
	module_id = primax_ov23850_eeprom[0];
	LOG_INF("pangfei primax module_id =0x%x %s %d\n",module_id,__func__,__LINE__);
	if(module_id==0x47)	{
		LOG_INF("pangfei primax module_id read ok\n");
		return ERROR_NONE;
	}
	else
	{
		*sensor_id = 0xFFFFFFFF;
		LOG_INF("pangfei primax module_id %s read fail\n",__func__);
		return ERROR_SENSOR_CONNECT_FAIL;
	}
#endif

    return ERROR_NONE;
}



static kal_uint32 open(void)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 1;
    kal_uint32 sensor_id = 0;
    LOG_INF("PLATFORM:MT6595,MIPI 2LANE\n");
    LOG_INF("preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n");

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
            LOG_INF("Read sensor id fail: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        if (sensor_id == imgsensor_info.sensor_id)
            break;
        retry = 2;
    }
    if (imgsensor_info.sensor_id != sensor_id)
        return ERROR_SENSOR_CONNECT_FAIL;


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
    imgsensor.ihdr_en = 0;
    imgsensor.test_pattern = KAL_FALSE;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	imgsensor.pdaf_mode= 0;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}



static kal_uint32 close(void)
{
    LOG_INF("E\n");


    return ERROR_NONE;
}   /*  close  */



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
    imgsensor.current_fps = imgsensor.current_fps;
    //imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}


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
        //imgsensor.autoflicker_en = KAL_FALSE;
    } else {
        if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
            LOG_INF("Warning: current_fps %d fps is not support, so use cap1's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap1.max_framerate/10);
        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
        //imgsensor.autoflicker_en = KAL_FALSE;
    }

    spin_unlock(&imgsensor_drv_lock);

    capture_setting(imgsensor.current_fps);

    return ERROR_NONE;
}   /* capture() */

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
    //imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    normal_video_setting(imgsensor.current_fps);
    return ERROR_NONE;
}

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
}

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
}


/*************************************************************************
* FUNCTION
* Custom1
*
* DESCRIPTION
*   This function start the sensor Custom1.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
    imgsensor.pclk = imgsensor_info.custom1.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom1.linelength;
    imgsensor.frame_length = imgsensor_info.custom1.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom1   */

static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
    imgsensor.pclk = imgsensor_info.custom2.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom2.linelength;
    imgsensor.frame_length = imgsensor_info.custom2.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom2   */

static kal_uint32 Custom3(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
    imgsensor.pclk = imgsensor_info.custom3.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom3.linelength;
    imgsensor.frame_length = imgsensor_info.custom3.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom3   */

static kal_uint32 Custom4(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM4;
    imgsensor.pclk = imgsensor_info.custom4.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom4.linelength;
    imgsensor.frame_length = imgsensor_info.custom4.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom4.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom4   */


static kal_uint32 Custom5(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM5;
    imgsensor.pclk = imgsensor_info.custom5.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom5.linelength;
    imgsensor.frame_length = imgsensor_info.custom5.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom5.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    preview_setting();
    return ERROR_NONE;
}   /*  Custom5   */

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
    LOG_INF("E\n");
    sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
    sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

    sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
    sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

    sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
    sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


    sensor_resolution->SensorHighSpeedVideoWidth     = imgsensor_info.hs_video.grabwindow_width;
    sensor_resolution->SensorHighSpeedVideoHeight    = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth    = imgsensor_info.slim_video.grabwindow_width;
    sensor_resolution->SensorSlimVideoHeight     = imgsensor_info.slim_video.grabwindow_height;
	
	sensor_resolution->SensorCustom1Width  = imgsensor_info.custom1.grabwindow_width;
    sensor_resolution->SensorCustom1Height     = imgsensor_info.custom1.grabwindow_height;

    sensor_resolution->SensorCustom2Width  = imgsensor_info.custom2.grabwindow_width;
    sensor_resolution->SensorCustom2Height     = imgsensor_info.custom2.grabwindow_height;

    sensor_resolution->SensorCustom3Width  = imgsensor_info.custom3.grabwindow_width;
    sensor_resolution->SensorCustom3Height     = imgsensor_info.custom3.grabwindow_height;

    sensor_resolution->SensorCustom4Width  = imgsensor_info.custom4.grabwindow_width;
    sensor_resolution->SensorCustom4Height     = imgsensor_info.custom4.grabwindow_height;

    sensor_resolution->SensorCustom5Width  = imgsensor_info.custom5.grabwindow_width;
    sensor_resolution->SensorCustom5Height     = imgsensor_info.custom5.grabwindow_height;
    return ERROR_NONE;
}   /*  get_resolution  */

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
                      MSDK_SENSOR_INFO_STRUCT *sensor_info,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	if(scenario_id == 0)
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
    sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
    sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
    sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

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

    sensor_info->SensorMasterClockSwitch = 0; /* not use */
    sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

    sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;          /* The frame of setting shutter default 0 for TG int */
    sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;    /* The frame of setting sensor gain */
    sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
    sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
    sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
    sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
#if defined(ENABLE_PDAF_VC)
	sensor_info->PDAF_Support = 2; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode*/
#else
	sensor_info->PDAF_Support = 0; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode*/
#endif
	sensor_info->HDR_Support = 0; /*0: NO HDR, 1: iHDR, 2:mvHDR, 3:zHDR*/
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
#if defined(HW_DESKEW_ENABLE)
	/*Dphy 1.2  Deskew support. 0: not support, 1: DPHY1.2 Deskew */	
    sensor_info->SensorMIPIDeskew = 1;
#endif
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
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            sensor_info->SensorGrabStartX = imgsensor_info.custom3.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.custom3.starty;
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            sensor_info->SensorGrabStartX = imgsensor_info.custom4.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.custom4.starty;
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            sensor_info->SensorGrabStartX = imgsensor_info.custom5.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.custom5.starty;
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
            break;
        default:
            sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
            sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
            break;
    }

    return ERROR_NONE;
}   /*  get_info  */


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
		case MSDK_SCENARIO_ID_CUSTOM1:
            Custom1(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            Custom2(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            Custom3(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            Custom4(image_window, sensor_config_data); // Custom1
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            Custom5(image_window, sensor_config_data); // Custom1
            break;
        default:
            LOG_INF("Error ScenarioId setting");
            preview(image_window, sensor_config_data);
            return ERROR_INVALID_SCENARIO_ID;
    }
    return ERROR_NONE;
}   /* control() */



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
    //kal_int16 dummyLine;
    kal_uint32 frameHeight;

    LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

    //yanglin add this workaround
    if(framerate == 0)
        return ERROR_NONE;

    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            frameHeight = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
                LOG_INF("frameHeight = %d\n", frameHeight);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = frameHeight - imgsensor_info.pre.framelength;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length =imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            if(framerate == 0)
                return ERROR_NONE;
            frameHeight = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = frameHeight - imgsensor_info.normal_video.framelength;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);

            set_dummy();
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            frameHeight = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = frameHeight - imgsensor_info.cap.framelength;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length =imgsensor_info.cap.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            frameHeight = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = frameHeight - imgsensor_info.hs_video.framelength;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
			imgsensor.frame_length =imgsensor_info.cap.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            frameHeight = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = frameHeight - imgsensor_info.slim_video.framelength;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length =imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
		case MSDK_SCENARIO_ID_CUSTOM1:
            frameHeight = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frameHeight > imgsensor_info.custom1.framelength) ? (frameHeight - imgsensor_info.custom1.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            frameHeight = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frameHeight > imgsensor_info.custom2.framelength) ? (frameHeight - imgsensor_info.custom2.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_CUSTOM3:
            frameHeight = imgsensor_info.custom3.pclk / framerate * 10 / imgsensor_info.custom3.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frameHeight > imgsensor_info.custom3.framelength) ? (frameHeight - imgsensor_info.custom3.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom3.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_CUSTOM4:
            frameHeight = imgsensor_info.custom4.pclk / framerate * 10 / imgsensor_info.custom4.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frameHeight > imgsensor_info.custom4.framelength) ? (frameHeight - imgsensor_info.custom4.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom4.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            break;
        case MSDK_SCENARIO_ID_CUSTOM5:
            frameHeight = imgsensor_info.custom5.pclk / framerate * 10 / imgsensor_info.custom5.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frameHeight > imgsensor_info.custom5.framelength) ? (frameHeight - imgsensor_info.custom5.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            break;
        default:  //coding with  preview scenario by default
            frameHeight = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = frameHeight - imgsensor_info.pre.framelength;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length =imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            set_dummy();
            LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
            break;
    }
    return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	if(scenario_id == 0)
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
            break;
    }

    return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    //check
		kal_int16 color_bar=0;
		LOG_INF("enable: %d\n", enable);

    color_bar= read_cmos_sensor(0x5280);

    if(enable)
        write_cmos_sensor(0x5280, ((color_bar & 0x73) | 0x84));
    else
        write_cmos_sensor(0x5280, (color_bar & 0x7f));

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
    SENSOR_VC_INFO_STRUCT *pvcinfo;
	SET_PD_BLOCK_INFO_T *PDAFinfo;
    
	if(!((feature_id == 3040) || (feature_id == 3058)))
		LOG_INF("feature_id = %d\n", feature_id);
	
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
            set_shutter(*feature_data);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            night_mode((BOOL) *feature_data);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            set_gain((UINT16) *feature_data);
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
            //get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data_32, (MUINT32 *)(*(feature_data_32+1)));
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
        case SENSOR_FEATURE_GET_CROP_INFO:
            //LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", *feature_data_32);
            //wininfo = (SENSOR_WINSIZE_INFO_STRUCT *)(*(feature_data_32+1));
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

		case SENSOR_FEATURE_GET_VC_INFO:
				LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
				pvcinfo = (SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
				switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[1],sizeof(SENSOR_VC_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[2],sizeof(SENSOR_VC_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[0],sizeof(SENSOR_VC_INFO_STRUCT));
					break;
				}
				break;
		case SENSOR_FEATURE_GET_PDAF_INFO:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld\n", *feature_data);
			PDAFinfo= (SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(SET_PD_BLOCK_INFO_T));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
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
#if defined(ENABLE_PDAF_VC)
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
#else
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
#endif
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
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
		case SENSOR_FEATURE_SET_PDAF:
#if defined(ENABLE_PDAF_VC)
			imgsensor.pdaf_mode= *feature_data_16;
#endif
			LOG_INF("PDAF mode :%d\n", imgsensor.pdaf_mode);
			break;
        default:
            break;
    }

    return ERROR_NONE;
}   /*  feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
    open,
    get_info,
    get_resolution,
    feature_control,
    control,
    close
};

UINT32 OV23850_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}
