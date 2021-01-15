/* * Copyright (C) 2018 MediaTek Inc.
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

#include "hi4821q_mipiraw_Sensor.h"

#define PFX "hi4821q_camera_sensor"
#define LOG_INF(format, args...)    \
	pr_info(PFX "[%s] " format, __func__, ##args)

#define HI4821Q_OTP_ENABLE 0

#if HI4821Q_OTP_ENABLE
//+hi4821q otp porting
#define EEPROM_BL24SA64D_ID 0xA0

static kal_uint16 hi4821q_read_eeprom(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, EEPROM_BL24SA64D_ID);

	return get_byte;
}
//-hi4821q otp porting
#endif

//PDAF
#define ENABLE_PDAF 1

#define per_frame 1

#define MULTI_WRITE 1

#define HI4821Q_XGC_QGC_PGC_CALIB 0

#define HI4821Q_OTP_DUMP 0

#if HI4821Q_XGC_QGC_PGC_CALIB
//  OTP information setting
#define XGC_BLOCK_X  9
#define XGC_BLOCK_Y  7
#define QGC_BLOCK_X  9
#define QGC_BLOCK_Y  7
#define PGC_BLOCK_X  33
#define PGC_BLOCK_Y  25

// SRAM Information
#define SRAM_XGC_START_ADDR_48M     0x43F0
#define SRAM_QGC_START_ADDR_48M     0x4000
#define SRAM_PGC_START_ADDR_48M     0x4000  //0x8980 (2020.9.14)

u8* pgc_data_buffer = NULL;
u8* qgc_data_buffer = NULL;
u8* xgc_data_buffer = NULL;

#define PGC_DATA_SIZE 1650
#define QGC_DATA_SIZE 1008
#define XGC_DATA_SIZE 630

#if HI4821Q_OTP_DUMP
extern void dumpEEPROMData(int u4Length,u8* pu1Params);
#endif

#endif

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = HI4821Q_SENSOR_ID,

	.checksum_value = 0x4f1b1d5e,       //0x6d01485c // Auto Test Mode
	.pre = {
		.pclk = 108000000,				//record different mode's pclk
		.linelength =  902, 			//record different mode's linelength
		.framelength = 3991, 			//record different mode's framelength
		.startx = 0,				    //record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 4000, 		//record different mode's width of grabwindow
		.grabwindow_height = 3000,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 560000000,
	},
	.cap = {
		.pclk = 108000000,
		.linelength = 902,
		.framelength = 3991,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 560000000,
	},
	// need to setting
	.cap1 = {
		.pclk = 108000000,
		.linelength = 902,
		.framelength = 3991,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 560000000,
	},
	.normal_video = {
		.pclk = 108000000,
		.linelength = 1104,
		.framelength = 3260,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 560000000,
	},
	.hs_video = {
		.pclk = 108000000,
		.linelength = 745,
		.framelength = 1208,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
		.mipi_pixel_rate = 660480000,
	},
	.slim_video = {
		.pclk = 108000000,
		.linelength = 745,
		.framelength = 604,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 2400,
		.mipi_pixel_rate = 913920000,
	},
	.custom1 = {
		.pclk = 108000000,
		.linelength = 1002,
		.framelength = 3872,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 270, //27.84fps
		.mipi_pixel_rate = 456960000,
	},
	.custom2 = {
		.pclk = 108000000,
		.linelength = 1118,
		.framelength = 6440,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 150,
		.mipi_pixel_rate = 660480000,
	},
	.custom3 = {
		.pclk = 108000000,
		.linelength = 1118,
		.framelength = 6440,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 8000,
		.grabwindow_height = 6000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 150,
		.mipi_pixel_rate = 937600000,
	},

	.margin = 8,
	.min_shutter = 8,

	//+for factory mode of photo black screen
	.min_gain = 73,
	.max_gain = 4096,
	.min_gain_iso = 100,
	.exp_step = 2,
	.gain_step = 1,
	.gain_type = 1,
	//-for factory mode of photo black screen
	.max_frame_length = 0xFFFF,
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
	.sensor_mode_num = 8,	  //support sensor mode num

	.cap_delay_frame = 2,
	.pre_delay_frame = 2,
	.video_delay_frame = 2,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,
	.custom1_delay_frame = 2,
	.custom2_delay_frame = 2,
	.custom3_delay_frame = 2,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO, //0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x40, 0x42,0x44,0x46,0xff},
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
	.i2c_write_id = 0x40,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[8] = {
	{8032, 6032, 0,   12, 8032, 6008, 4016, 3004, 8, 2, 4000, 3000, 0,  0, 4000, 3000},  //preview(4000 x 3000)
	{8032, 6032, 0,   12, 8032, 6008, 4016, 3004, 8, 2, 4000, 3000, 0,  0, 4000, 3000},  //capture(4000 x 3000)
	{8032, 6032, 0,   12, 8032, 6008, 4016, 3004, 8, 2, 4000, 3000, 0,  0, 4000, 3000},  // VIDEO (4000 x 3000)
	{8032, 6032, 0,   848, 8032, 4336, 2008, 1084, 44, 2, 1920, 1080, 0,  0, 1920, 1080},    // hight speed video (1920 x 1080)
	{8032, 6032, 0,   1568, 8032, 2896, 2008, 724, 364, 2, 1280, 720, 0,  0, 1280, 720},     // slim video (1280 x 720)
	{8032, 6032, 0,   12, 8032, 6008, 4016, 3004, 8, 2, 4000, 3000, 0,  0, 4000, 3000},     // custom1 (4000 x 3000)
	{8032, 6032, 0,   12, 8032, 6008, 4016, 3004, 8, 2, 4000, 3000, 0,  0, 4000, 3000},     // custom2 (4000 x 3000)
	{8032, 6032, 0,   14, 8032, 6004, 8032, 6004, 16, 2, 8000, 6000, 0,  0, 8000, 6000},     // custom3 (8000x6000)
};


#if ENABLE_PDAF

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[4]=
{
	/* Preview mode setting */
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8	2:4x4  3:1x1 */
	  0x00, 0x2B, 0x1070, 0x0C30,	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
	  0x01, 0x30, 0x026C, 0x05D0,   // VC2 PDAF
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
	  0x01, 0x30, 0x026C, 0x05D0,   // VC2 PDAF
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
	  0x01, 0x30, 0x026C, 0x05D0,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??
	  /* Custom1 mode setting */
	 {0x02, //VC_Num
	  0x0a, //VC_PixelNum
	  0x00, //ModeSelect	/* 0:auto 1:direct */
	  0x00, //EXPO_Ratio	/* 1/1, 1/2, 1/4, 1/8 */
	  0x00, //0DValue		/* 0D Value */
	  0x00, //RG_STATSMODE	/* STATS divistion mode 0:16x16  1:8x8	2:4x4  3:1x1 */
	  0x00, 0x2B, 0x1070, 0x0C30,	// VC0 Maybe image data?
	  0x00, 0x00, 0x0000, 0x0000,	// VC1 MVHDR
	  0x01, 0x30, 0x026C, 0x05D0,   // VC2 PDAF
	  0x00, 0x00, 0x0000, 0x0000},	// VC3 ??

};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX	= 16,
	.i4OffsetY	= 12,
	.i4PitchX	= 16,
	.i4PitchY	= 16,
	.i4PairNum	= 8,
	.i4SubBlkW	= 8,
	.i4SubBlkH	= 4,
	.i4BlockNumX = 248,
	.i4BlockNumY = 186,
	.iMirrorFlip = 0,
	.i4PosR =	{
						{16,13}, {24,13}, {20,17}, {28,17},
						{16,21}, {24,21}, {20,25}, {28,25},
				},
	.i4PosL =	{
						{17,13}, {25,13}, {21,17}, {29,17},
						{17,21}, {25,21}, {21,25}, {29,25},
				}


};
#endif


#if MULTI_WRITE
#define I2C_BUFFER_LEN 1020

static kal_uint16 hi4821q_table_write_cmos_sensor(
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

#if HI4821Q_XGC_QGC_PGC_CALIB
//XGC,QGC,PGC Calibration data are applied here
static void apply_sensor_Cali(void)
{
	kal_uint16 idx = 0;
	kal_uint16 sensor_qgc_addr;
	kal_uint16 sensor_pgc_addr;
	kal_uint16 sensor_xgc_addr;

	kal_uint16 hi4821_xgc_data;
	kal_uint16 hi4821_qgc_data;
	kal_uint16 hi4821_pgc_data;

	int i;
	int isp_reg_en;
	sensor_xgc_addr = SRAM_XGC_START_ADDR_48M + 2;

	write_cmos_sensor(0x0b00,0x0000);
	isp_reg_en = read_cmos_sensor(0x0b04);
	write_cmos_sensor(0x0b04,isp_reg_en|0x000E); //XGC, QGC, PGC enable
	LOG_INF("[Start]:apply_sensor_Cali finish\n");
	//XGC data apply
	{
		write_cmos_sensor(0x301c,0x0002);
		for(i = 0; i < XGC_BLOCK_X*XGC_BLOCK_Y*10; i += 2) //9(BLOCK_X)* 7 (BLCOK_Y)*10(channel)
		{
			hi4821_xgc_data = ((((xgc_data_buffer[i+1]) & (0x00ff)) << 8) + ((xgc_data_buffer[i]) & (0x00ff)));

			if(idx == XGC_BLOCK_X*XGC_BLOCK_Y){
				sensor_xgc_addr = SRAM_XGC_START_ADDR_48M;
			}

			else if(idx == XGC_BLOCK_X*XGC_BLOCK_Y*2){
				sensor_xgc_addr += 2;
			}

			else if(idx == XGC_BLOCK_X*XGC_BLOCK_Y*3){
				sensor_xgc_addr = SRAM_XGC_START_ADDR_48M + XGC_BLOCK_X * XGC_BLOCK_Y * 4;
			}

			else if(idx == XGC_BLOCK_X*XGC_BLOCK_Y*4){
				sensor_xgc_addr += 2;
			}

			else{
				LOG_INF("sensor_xgc_addr:0x%x,[ERROR]:no XGC data need apply\n",sensor_xgc_addr);
			}
			idx++;
			write_cmos_sensor(sensor_xgc_addr,hi4821_xgc_data);
			#if HI4821Q_OTP_DUMP
				pr_info("sensor_xgc_addr:0x%x,xgc_data_buffer[%d]:0x%x,xgc_data_buffer[%d]:0x%x,hi4821_xgc_data:0x%x\n",
					sensor_xgc_addr,i,xgc_data_buffer[i],i+1,xgc_data_buffer[i+1],hi4821_xgc_data);
			#endif

			sensor_xgc_addr += 4;
		}
	}

	//QGC data apply
	{
		idx = 0;
		write_cmos_sensor(0x301c,0x0002);
		sensor_qgc_addr = SRAM_QGC_START_ADDR_48M;
		for(i = 0; i < QGC_BLOCK_X*QGC_BLOCK_Y*16;i += 2) //9(BLOCK_X)* 7 (BLCOK_Y)*16(channel)
		{
			hi4821_qgc_data = ((((qgc_data_buffer[i+1]) & (0x00ff)) << 8) + ((qgc_data_buffer[i]) & (0x00ff)));
			write_cmos_sensor(sensor_qgc_addr,hi4821_qgc_data);

			#if HI4821Q_OTP_DUMP
				pr_info("sensor_qgc_addr:0x%x,qgc_data_buffer[%d]:0x%x,qgc_data_buffer[%d]:0x%x,hi4821_qgc_data:0x%x\n",
					sensor_qgc_addr,i,qgc_data_buffer[i],i+1,qgc_data_buffer[i+1],hi4821_qgc_data);
			#endif
			sensor_qgc_addr += 2;
			idx++;
		}
	}

	//PGC data apply
	{
		idx = 0;
		write_cmos_sensor(0x301c,0x0002);
		sensor_pgc_addr = SRAM_PGC_START_ADDR_48M;
		for(i = 0; i < PGC_BLOCK_X*PGC_BLOCK_Y*2;i += 2) //33(BLOCK_X)* 25(BLCOK_Y)*1(channel)*2bytes
		{
			hi4821_pgc_data = ((((pgc_data_buffer[i+1]) & (0x00ff)) << 8) + ((pgc_data_buffer[i]) & (0x00ff)));
			write_cmos_sensor(sensor_pgc_addr,hi4821_pgc_data);

			#if HI4821Q_OTP_DUMP
				pr_info("sensor_pgc_addr:0x%x,pgc_data_buffer[%d]:0x%x,pgc_data_buffer[%d]:0x%x,hi4821_pgc_data:0x%x\n",
					sensor_pgc_addr,i,pgc_data_buffer[i],i+1,pgc_data_buffer[i+1],hi4821_pgc_data);
			#endif
			sensor_pgc_addr += 2;
			idx++;
		}
	}

	write_cmos_sensor(0x0b00,0x0100);
	mdelay(10);
	write_cmos_sensor(0x0b00,0x0100);
	LOG_INF("[End]:apply_sensor_Cali finish\n");
}
#endif

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n",
		imgsensor.dummy_line, imgsensor.dummy_pixel);
	write_cmos_sensor(0x0210, imgsensor.frame_length & 0xFFFF);
	write_cmos_sensor(0x0206,imgsensor .line_length);

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
	kal_uint16 realtime_fps = 0;

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
		realtime_fps = imgsensor.pclk * 10 /
			(imgsensor.line_length * imgsensor.frame_length);
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else
			write_cmos_sensor(0x0210, imgsensor.frame_length);

	} else{
			write_cmos_sensor(0x0210, imgsensor.frame_length);
	}


	write_cmos_sensor(0x020C, shutter);

	LOG_INF("frame_length = %d , shutter = %d \n", imgsensor.frame_length, shutter);


}	/*	write_shutter  */


static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_int32 dummy_line = 0;
	kal_uint16 realtime_fps = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	spin_lock(&imgsensor_drv_lock);
	/*Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;

	/*  */
	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);

	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	set_dummy();

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
	} else {
		// Extend frame length
		write_cmos_sensor(0x0210, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0x020C, shutter);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
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

	write_cmos_sensor(0x020A,reg_gain);
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

#if 1
static void set_mirror_flip(kal_uint8 image_mirror)
{
	int mirrorflip;
	mirrorflip = read_cmos_sensor(0x0202);
	LOG_INF("image_mirror = %d", image_mirror);
	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor(0x0202, mirrorflip|0x0000);
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor(0x0202, mirrorflip|0x0040);
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor(0x0202, mirrorflip|0x0080);
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor(0x0202, mirrorflip|0x00c0);
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
kal_uint16 addr_data_pair_init_hi4821q[] = {
	0x0790, 0x0100,
	0x9000, 0x706F,
	0x9002, 0x0000,
	0x9004, 0x67B1,
	0x9006, 0xA823,
	0x9008, 0x5AA7,
	0x900A, 0x97B7,
	0x900C, 0x0000,
	0x900E, 0x8793,
	0x9010, 0x0427,
	0x9012, 0x2623,
	0x9014, 0x12F5,
	0x9016, 0x97B7,
	0x9018, 0x0000,
	0x901A, 0x8793,
	0x901C, 0x0B47,
	0x901E, 0xDD7C,
	0x9020, 0x67A5,
	0x9022, 0x8793,
	0x9024, 0x2847,
	0x9026, 0x2A23,
	0x9028, 0x18F5,
	0x902A, 0x2783,
	0x902C, 0x22C5,
	0x902E, 0x4711,
	0x9030, 0x9323,
	0x9032, 0x00E7,
	0x9034, 0x97B7,
	0x9036, 0x0000,
	0x9038, 0x8793,
	0x903A, 0x0D07,
	0x903C, 0x2823,
	0x903E, 0x0AF5,
	0x9040, 0x8082,
	0x9042, 0x1141,
	0x9044, 0xC606,
	0x9046, 0xC422,
	0x9048, 0x67B1,
	0x904A, 0x8793,
	0x904C, 0x0007,
	0x904E, 0xA703,
	0x9050, 0x22C7,
	0x9052, 0x6689,
	0x9054, 0xD683,
	0x9056, 0x8006,
	0x9058, 0x1023,
	0x905A, 0x00D7,
	0x905C, 0xA703,
	0x905E, 0x22C7,
	0x9060, 0x6685,
	0x9062, 0xD683,
	0x9064, 0x3006,
	0x9066, 0x1123,
	0x9068, 0x00D7,
	0x906A, 0xA783,
	0x906C, 0x22C7,
	0x906E, 0x5703,
	0x9070, 0x6000,
	0x9072, 0x9223,
	0x9074, 0x00E7,
	0x9076, 0x6431,
	0x9078, 0x2783,
	0x907A, 0x5B04,
	0x907C, 0xA783,
	0x907E, 0x1807,
	0x9080, 0xA783,
	0x9082, 0x1547,
	0x9084, 0x9782,
	0x9086, 0x2783,
	0x9088, 0x5B04,
	0x908A, 0xD683,
	0x908C, 0x1D07,
	0x908E, 0x0713,
	0x9090, 0x3FD0,
	0x9092, 0x7D63,
	0x9094, 0x00D7,
	0x9096, 0xA783,
	0x9098, 0x22C7,
	0x909A, 0xD783,
	0x909C, 0x0027,
	0x909E, 0xF793,
	0x90A0, 0x0F77,
	0x90A2, 0xE793,
	0x90A4, 0x1007,
	0x90A6, 0x6705,
	0x90A8, 0x1023,
	0x90AA, 0x30F7,
	0x90AC, 0x40B2,
	0x90AE, 0x4422,
	0x90B0, 0x0141,
	0x90B2, 0x8082,
	0x90B4, 0x6789,
	0x90B6, 0xD703,
	0x90B8, 0x2307,
	0x90BA, 0x6713,
	0x90BC, 0x0017,
	0x90BE, 0x9823,
	0x90C0, 0x22E7,
	0x90C2, 0xD703,
	0x90C4, 0x2247,
	0x90C6, 0x6713,
	0x90C8, 0x0027,
	0x90CA, 0x9223,
	0x90CC, 0x22E7,
	0x90CE, 0x8082,
	0x90D0, 0x1141,
	0x90D2, 0xC606,
	0x90D4, 0x67BD,
	0x90D6, 0x4779,
	0x90D8, 0xCF98,
	0x90DA, 0x87BE,
	0x90DC, 0x67B1,
	0x90DE, 0xA783,
	0x90E0, 0x5B07,
	0x90E2, 0xA783,
	0x90E4, 0x1807,
	0x90E6, 0xA783,
	0x90E8, 0x0D87,
	0x90EA, 0x9782,
	0x90EC, 0x40B2,
	0x90EE, 0x0141,
	0x90F0, 0x8082,
	0x90F2, 0x1141,
	0x90F4, 0x0793,
	0x90F6, 0x2000,
	0x90F8, 0x53D8,
	0x90FA, 0xC23A,
	0x90FC, 0x5798,
	0x90FE, 0xC43A,
	0x9100, 0x57D8,
	0x9102, 0xC63A,
	0x9104, 0x8DA3,
	0x9106, 0x0207,
	0x9108, 0x6741,
	0x910A, 0x0693,
	0x910C, 0xFFF7,
	0x910E, 0x2223,
	0x9110, 0x22D0,
	0x9112, 0x2423,
	0x9114, 0x22D0,
	0x9116, 0x2623,
	0x9118, 0x22D0,
	0x911A, 0x56FD,
	0x911C, 0x2A23,
	0x911E, 0xA2D7,
	0x9120, 0x2E23,
	0x9122, 0xA2D7,
	0x9124, 0x673D,
	0x9126, 0x2603,
	0x9128, 0x6747,
	0x912A, 0x0613,
	0x912C, 0x0C86,
	0x912E, 0x2A23,
	0x9130, 0x60C7,
	0x9132, 0x4605,
	0x9134, 0x1423,
	0x9136, 0x66C7,
	0x9138, 0x4685,
	0x913A, 0xD754,
	0x913C, 0x2023,
	0x913E, 0x0207,
	0x9140, 0x87BE,
	0x9142, 0x2023,
	0x9144, 0x0207,
	0x9146, 0x87BE,
	0x9148, 0x4712,
	0x914A, 0xD3D8,
	0x914C, 0x0713,
	0x914E, 0x2040,
	0x9150, 0x46A2,
	0x9152, 0xD354,
	0x9154, 0x46B2,
	0x9156, 0xD714,
	0x9158, 0x471D,
	0x915A, 0x8DA3,
	0x915C, 0x02E7,
	0x915E, 0x0141,
	0x9160, 0x8082,
	0x9162, 0x4783,
	0x9164, 0x2090,
	0x9166, 0xF793,
	0x9168, 0x0FF7,
	0x916A, 0x4501,
	0x916C, 0xEB89,
	0x916E, 0x67B1,
	0x9170, 0xA783,
	0x9172, 0x22C7,
	0x9174, 0x479C,
	0x9176, 0xC503,
	0x9178, 0x0017,
	0x917A, 0x3533,
	0x917C, 0x00A0,
	0x917E, 0x8082,
	0x9180, 0x1141,
	0x9182, 0xC606,
	0x9184, 0x6799,
	0x9186, 0x8793,
	0x9188, 0x0287,
	0x918A, 0x6719,
	0x918C, 0x0713,
	0x918E, 0x0A87,
	0x9190, 0xA023,
	0x9192, 0x0007,
	0x9194, 0x0791,
	0x9196, 0x9DE3,
	0x9198, 0xFEE7,
	0x919A, 0x67B1,
	0x919C, 0xA783,
	0x919E, 0x22C7,
	0x91A0, 0xD683,
	0x91A2, 0x0007,
	0x91A4, 0x6709,
	0x91A6, 0x1023,
	0x91A8, 0x80D7,
	0x91AA, 0xD683,
	0x91AC, 0x0027,
	0x91AE, 0x6705,
	0x91B0, 0x1023,
	0x91B2, 0x30D7,
	0x91B4, 0xD783,
	0x91B6, 0x0047,
	0x91B8, 0x1023,
	0x91BA, 0x60F0,
	0x91BC, 0x375D,
	0x91BE, 0xC519,
	0x91C0, 0x67B1,
	0x91C2, 0xA783,
	0x91C4, 0x5B07,
	0x91C6, 0xC783,
	0x91C8, 0x18C7,
	0x91CA, 0xCF81,
	0x91CC, 0x67B1,
	0x91CE, 0xA783,
	0x91D0, 0x5B07,
	0x91D2, 0xA783,
	0x91D4, 0x1807,
	0x91D6, 0xA783,
	0x91D8, 0x0847,
	0x91DA, 0x9782,
	0x91DC, 0x40B2,
	0x91DE, 0x0141,
	0x91E0, 0x8082,
	0x91E2, 0x3F01,
	0x91E4, 0x67B1,
	0x91E6, 0xA783,
	0x91E8, 0x5B07,
	0x91EA, 0xA783,
	0x91EC, 0x1807,
	0x91EE, 0xA783,
	0x91F0, 0x0847,
	0x91F2, 0x9782,
	0x91F4, 0xBFE1,
	0x91F6, 0x4218,
	0x91F8, 0x429C,
	0x91FA, 0x8F99,
	0x91FC, 0x0813,
	0x91FE, 0x03F0,
	0x9200, 0x6363,
	0x9202, 0x00F8,
	0x9204, 0x8082,
	0x9206, 0x1141,
	0x9208, 0xC606,
	0x920A, 0x8385,
	0x920C, 0x97BA,
	0x920E, 0x9713,
	0x9210, 0x0027,
	0x9212, 0x972A,
	0x9214, 0x4318,
	0x9216, 0x7763,
	0x9218, 0x00B7,
	0x921A, 0xC21C,
	0x921C, 0x3FE9,
	0x921E, 0x40B2,
	0x9220, 0x0141,
	0x9222, 0x8082,
	0x9224, 0xC29C,
	0x9226, 0xBFDD,
	0x9228, 0x1101,
	0x922A, 0xCE06,
	0x922C, 0xCC22,
	0x922E, 0x882E,
	0x9230, 0x85B2,
	0x9232, 0x8436,
	0x9234, 0xC602,
	0x9236, 0xC436,
	0x9238, 0xCE89,
	0x923A, 0x4114,
	0x923C, 0x87AA,
	0x923E, 0x4701,
	0x9240, 0x4605,
	0x9242, 0xEA81,
	0x9244, 0xC390,
	0x9246, 0x0705,
	0x9248, 0x0563,
	0x924A, 0x00E4,
	0x924C, 0x0791,
	0x924E, 0x4394,
	0x9250, 0xDAF5,
	0x9252, 0x67B1,
	0x9254, 0xC783,
	0x9256, 0x1C17,
	0x9258, 0xE391,
	0x925A, 0x85C2,
	0x925C, 0x0034,
	0x925E, 0x0070,
	0x9260, 0x3F59,
	0x9262, 0x67B1,
	0x9264, 0xC783,
	0x9266, 0x1C17,
	0x9268, 0xEF81,
	0x926A, 0xC422,
	0x926C, 0x5783,
	0x926E, 0x0081,
	0x9270, 0x07C2,
	0x9272, 0x5503,
	0x9274, 0x00C1,
	0x9276, 0x8D5D,
	0x9278, 0x40F2,
	0x927A, 0x4462,
	0x927C, 0x6105,
	0x927E, 0x8082,
	0x9280, 0xC602,
	0x9282, 0xB7ED,
	0x9284, 0x9328,
	0x9286, 0x0000,
	0x9288, 0x9480,
	0x928A, 0x0000,
	0x928C, 0x93D8,
	0x928E, 0x0000,
	0x9290, 0x30B0,
	0x9292, 0x0008,
	0x9294, 0x01E7,
	0x9296, 0x2267,
	0x9298, 0x0227,
	0x929A, 0x0188,
	0x929C, 0x7001,
	0x929E, 0x19C6,
	0x92A0, 0x5E00,
	0x92A2, 0x6780,
	0x92A4, 0x01F2,
	0x92A6, 0x209F,
	0x92A8, 0x701A,
	0x92AA, 0x8070,
	0x92AC, 0xF3E6,
	0x92AE, 0x0001,
	0x92B0, 0x0000,
	0x92B2, 0x7025,
	0x92B4, 0x19CF,
	0x92B6, 0x7008,
	0x92B8, 0x19CF,
	0x92BA, 0x0A16,
	0x92BC, 0x0017,
	0x92BE, 0x001C,
	0x92C0, 0x0000,
	0x92C2, 0x03D9,
	0x92C4, 0x0B64,
	0x92C6, 0x00E2,
	0x92C8, 0x00E2,
	0x92CA, 0x24CF,
	0x92CC, 0x00E4,
	0x92CE, 0x00E4,
	0x92D0, 0x001D,
	0x92D2, 0x0001,
	0x92D4, 0x20CC,
	0x92D6, 0x7077,
	0x92D8, 0x2FCF,
	0x92DA, 0x701D,
	0x92DC, 0x19CF,
	0x92DE, 0x00A4,
	0x92E0, 0x0080,
	0x92E2, 0x5640,
	0x92E4, 0x6701,
	0x92E6, 0x03D4,
	0x92E8, 0x7001,
	0x92EA, 0x19CF,
	0x92EC, 0x7000,
	0x92EE, 0x4EB2,
	0x92F0, 0x0804,
	0x92F2, 0x7003,
	0x92F4, 0x1289,
	0x92F6, 0x004B,
	0x92F8, 0x2123,
	0x92FA, 0x7011,
	0x92FC, 0x0FE4,
	0x92FE, 0x00E2,
	0x9300, 0x00E2,
	0x9302, 0x0121,
	0x9304, 0x00E4,
	0x9306, 0x00E4,
	0x9308, 0x051D,
	0x930A, 0x2004,
	0x930C, 0x20CC,
	0x930E, 0x7157,
	0x9310, 0x2FCF,
	0x9312, 0x704B,
	0x9314, 0x0FE4,
	0x9316, 0x0082,
	0x9318, 0xC7A0,
	0x931A, 0xC061,
	0x931C, 0x0301,
	0x931E, 0x0000,
	0x9320, 0x0045,
	0x9322, 0x0003,
	0x9324, 0x4080,
	0x9326, 0x0000,
	0x9328, 0x9334,
	0x932A, 0x0000,
	0x932C, 0x3818,
	0x932E, 0x0001,
	0x9330, 0x0001,
	0x9332, 0x0000,
	0x9334, 0x0000,
	0x9336, 0x004B,
	0x9338, 0x9290,
	0x933A, 0x0000,
	0x933C, 0x30B0,
	0x933E, 0x0008,
	0x9340, 0x01E7,
	0x9342, 0x2267,
	0x9344, 0x0227,
	0x9346, 0x0188,
	0x9348, 0x7001,
	0x934A, 0x19C6,
	0x934C, 0x5E00,
	0x934E, 0x6780,
	0x9350, 0x01F2,
	0x9352, 0x209F,
	0x9354, 0x701A,
	0x9356, 0x8070,
	0x9358, 0xF3E6,
	0x935A, 0x0001,
	0x935C, 0x0000,
	0x935E, 0x7025,
	0x9360, 0x19CF,
	0x9362, 0x7008,
	0x9364, 0x19CF,
	0x9366, 0x05DC,
	0x9368, 0x0196,
	0x936A, 0x0017,
	0x936C, 0x0040,
	0x936E, 0x0419,
	0x9370, 0x0664,
	0x9372, 0x00E2,
	0x9374, 0x00E2,
	0x9376, 0x24CF,
	0x9378, 0x00E4,
	0x937A, 0x00E4,
	0x937C, 0x001D,
	0x937E, 0x0001,
	0x9380, 0x20CC,
	0x9382, 0x7083,
	0x9384, 0x19CF,
	0x9386, 0x5E62,
	0x9388, 0x6700,
	0x938A, 0x0796,
	0x938C, 0x0080,
	0x938E, 0x5640,
	0x9390, 0x6701,
	0x9392, 0x03D4,
	0x9394, 0x7001,
	0x9396, 0x19CF,
	0x9398, 0x7000,
	0x939A, 0x4EB2,
	0x939C, 0x0804,
	0x939E, 0x7003,
	0x93A0, 0x1289,
	0x93A2, 0x004B,
	0x93A4, 0x2123,
	0x93A6, 0x7003,
	0x93A8, 0x0FE4,
	0x93AA, 0x00E2,
	0x93AC, 0x00E2,
	0x93AE, 0x0121,
	0x93B0, 0x00E4,
	0x93B2, 0x00E4,
	0x93B4, 0x08DD,
	0x93B6, 0x2004,
	0x93B8, 0x20CC,
	0x93BA, 0x7166,
	0x93BC, 0xC064,
	0x93BE, 0x9467,
	0x93C0, 0x0327,
	0x93C2, 0x0000,
	0x93C4, 0x0164,
	0x93C6, 0x0082,
	0x93C8, 0x5400,
	0x93CA, 0x618F,
	0x93CC, 0x01C0,
	0x93CE, 0x0045,
	0x93D0, 0x7000,
	0x93D2, 0x4088,
	0x93D4, 0x0003,
	0x93D6, 0x0000,
	0x93D8, 0x93E4,
	0x93DA, 0x0000,
	0x93DC, 0x3818,
	0x93DE, 0x0001,
	0x93E0, 0x0001,
	0x93E2, 0x0000,
	0x93E4, 0x0000,
	0x93E6, 0x004D,
	0x93E8, 0x933C,
	0x93EA, 0x0000,
	0x93EC, 0x30B0,
	0x93EE, 0x0008,
	0x93F0, 0x01E7,
	0x93F2, 0x2267,
	0x93F4, 0x0227,
	0x93F6, 0x0188,
	0x93F8, 0x7001,
	0x93FA, 0x19C6,
	0x93FC, 0x5E00,
	0x93FE, 0x6780,
	0x9400, 0x01F2,
	0x9402, 0x209F,
	0x9404, 0x701A,
	0x9406, 0x8070,
	0x9408, 0xF3E6,
	0x940A, 0x0001,
	0x940C, 0x0000,
	0x940E, 0x7025,
	0x9410, 0x19CF,
	0x9412, 0x7008,
	0x9414, 0x19CF,
	0x9416, 0x0A16,
	0x9418, 0x0017,
	0x941A, 0x001C,
	0x941C, 0x0000,
	0x941E, 0x03D9,
	0x9420, 0x0B64,
	0x9422, 0x00E2,
	0x9424, 0x00E2,
	0x9426, 0x24CF,
	0x9428, 0x00E4,
	0x942A, 0x00E4,
	0x942C, 0x001D,
	0x942E, 0x0001,
	0x9430, 0x20CC,
	0x9432, 0x7050,
	0x9434, 0x19CF,
	0x9436, 0x21CF,
	0x9438, 0x0024,
	0x943A, 0x0080,
	0x943C, 0x5640,
	0x943E, 0x6701,
	0x9440, 0x03D4,
	0x9442, 0x7001,
	0x9444, 0x19CF,
	0x9446, 0x7000,
	0x9448, 0x4EB2,
	0x944A, 0x0804,
	0x944C, 0x7003,
	0x944E, 0x1289,
	0x9450, 0x004B,
	0x9452, 0x2123,
	0x9454, 0x7011,
	0x9456, 0x0FE4,
	0x9458, 0x00E2,
	0x945A, 0x00E2,
	0x945C, 0x0121,
	0x945E, 0x00E4,
	0x9460, 0x00E4,
	0x9462, 0x04DD,
	0x9464, 0x2004,
	0x9466, 0x20CC,
	0x9468, 0x70CF,
	0x946A, 0x2FCF,
	0x946C, 0x0064,
	0x946E, 0x0082,
	0x9470, 0xC7A0,
	0x9472, 0xC061,
	0x9474, 0x0301,
	0x9476, 0x0000,
	0x9478, 0x0045,
	0x947A, 0x0003,
	0x947C, 0x4080,
	0x947E, 0x0000,
	0x9480, 0x948C,
	0x9482, 0x0000,
	0x9484, 0x3818,
	0x9486, 0x0001,
	0x9488, 0x0001,
	0x948A, 0x0000,
	0x948C, 0x0000,
	0x948E, 0x0049,
	0x9490, 0x93EC,
	0x9492, 0x0000,
	0xC290, 0xAA55,
	0xC4B0, 0x0204,
	0xC4B2, 0x0206,
	0xC4B4, 0x0226,
	0xC4B6, 0x022A,
	0xC4B8, 0x022C,
	0xC4BA, 0x022E,
	0xC4BC, 0x0244,
	0xC4BE, 0x0246,
	0xC4C0, 0x0248,
	0xC4C2, 0x024A,
	0xC4C4, 0x024C,
	0xC4C6, 0x024E,
	0xC4C8, 0x0250,
	0xC4CA, 0x0252,
	0xC4CC, 0x0254,
	0xC4CE, 0x0256,
	0xC4D0, 0x0258,
	0xC4D2, 0x025A,
	0xC4D4, 0x025C,
	0xC4D6, 0x025E,
	0xC4D8, 0x0262,
	0xC4DA, 0x0266,
	0xC4DC, 0x026C,
	0xC4DE, 0x0400,
	0xC4E0, 0x0402,
	0xC4E2, 0x040C,
	0xC4E4, 0x040E,
	0xC4E6, 0x0410,
	0xC4E8, 0x041E,
	0xC4EA, 0x0422,
	0xC4EC, 0x0426,
	0xC4EE, 0x042C,
	0xC4F0, 0x042E,
	0xC4F2, 0x0442,
	0xC4F4, 0x044C,
	0xC4F6, 0x0452,
	0xC4F8, 0x0454,
	0xC4FA, 0x045C,
	0xC4FC, 0x045E,
	0xC4FE, 0x046C,
	0xC500, 0x046E,
	0xC502, 0x0470,
	0xC504, 0x0472,
	0xC506, 0x04A4,
	0xC508, 0x0600,
	0xC50A, 0x0602,
	0xC50C, 0x0604,
	0xC50E, 0x0624,
	0xC510, 0x06D0,
	0xC512, 0x06D2,
	0xC514, 0x06E4,
	0xC516, 0x06E6,
	0xC518, 0x06E8,
	0xC51A, 0x06EA,
	0xC51C, 0x06F6,
	0xC51E, 0x0A0A,
	0xC520, 0x0A10,
	0xC522, 0x0B30,
	0xC524, 0x0C00,
	0xC526, 0x0C02,
	0xC528, 0x0C04,
	0xC52A, 0x0C14,
	0xC52C, 0x0C16,
	0xC52E, 0x0C18,
	0xC530, 0x0C1A,
	0xC532, 0x0D00,
	0xC534, 0x0D28,
	0xC536, 0x0D2A,
	0xC538, 0x0D50,
	0xC53A, 0x0D52,
	0xC53C, 0x0F00,
	0xC53E, 0x0F08,
	0xC540, 0x0F0A,
	0xC542, 0x0F0C,
	0xC544, 0x0F0E,
	0xC546, 0x1004,
	0xC548, 0x101C,
	0xC54A, 0x1038,
	0xC54C, 0x103E,
	0xC54E, 0x1042,
	0xC550, 0x1044,
	0xC552, 0x1048,
	0xC554, 0x1100,
	0xC556, 0x1108,
	0xC558, 0x1116,
	0xC55A, 0x1118,
	0xC55C, 0x1130,
	0xC55E, 0x1206,
	0xC560, 0x1410,
	0xC562, 0x1414,
	0xC564, 0x1416,
	0xC566, 0x1418,
	0xC568, 0x141A,
	0xC56A, 0x141C,
	0xC56C, 0x141E,
	0xC56E, 0x1420,
	0xC570, 0x1422,
	0xC572, 0x1600,
	0xC574, 0x1608,
	0xC576, 0x160A,
	0xC578, 0x160C,
	0xC57A, 0x160E,
	0xC57C, 0x1614,
	0xC57E, 0x1702,
	0xC580, 0x1704,
	0xC582, 0x1706,
	0xC584, 0x1708,
	0xC586, 0x170A,
	0xC588, 0x170C,
	0xC58A, 0x1718,
	0xC58C, 0x1740,
	0xC58E, 0x1C00,
	0xC590, 0x1D06,
	0xC592, 0x1F04,
	0xC594, 0x1F06,
	0xC596, 0x1F08,
	0xC598, 0x1F0A,
	0x0368, 0x0075,

	0xC294, 0x0000,
	0xC296, 0x0001,
	0xC298, 0x0042,
	0xC29A, 0x0061,
	0xC29C, 0x0163,
	0xC29E, 0x0232,
	0xC2A0, 0x07FF,
	0xC2A2, 0x0042,
	0xC2A4, 0x0061,
	0xC2A6, 0x0163,
	0xC2A8, 0x0233,
	0xC2AA, 0x07FF,
	0xC2AC, 0x0042,
	0xC2AE, 0x0058,
	0xC2B0, 0x0163,
	0xC2B2, 0x0230,
	0xC2B4, 0x07FF,
	0xC2B6, 0x0042,
	0xC2B8, 0x005D,
	0xC2BA, 0x015F,
	0xC2BC, 0x0232,
	0xC2BE, 0x07FF,
	0xC2C0, 0x201B,
	0xC2C2, 0x200B,
	0xC2C4, 0x0009,
	0xC2C6, 0x0025,
	0xC2C8, 0x017E,
	0xC2CA, 0x0002,
	0xC2CC, 0x0391,
	0xC2CE, 0x0002,
	0xC2D0, 0x1B41,
	0xC2D2, 0x0002,
	0xC2D4, 0x5247,
	0xC2D6, 0x0002,
	0xC2D8, 0x2018,
	0xC2DA, 0x200D,
	0xC2DC, 0x0012,
	0xC2DE, 0x002A,
	0xC2E0, 0x002A,
	0xC2E2, 0x0002,
	0xC2E4, 0x0189,
	0xC2E6, 0x0002,
	0xC2E8, 0x2647,
	0xC2EA, 0x0002,
	0xC2EC, 0x55F1,
	0xC2EE, 0x0002,
	0xC2F0, 0x202C,
	0xC2F2, 0x201D,
	0xC2F4, 0x0011,
	0xC2F6, 0x003E,
	0xC2F8, 0x01A4,
	0xC2FA, 0x0002,
	0xC2FC, 0x0317,
	0xC2FE, 0x0002,
	0xC300, 0x3873,
	0xC302, 0x0002,
	0xC304, 0x8F0D,
	0xC306, 0x0002,
	0xC308, 0x0063,
	0xC30A, 0x000E,
	0xC30C, 0x2007,
	0xC30E, 0x0024,
	0xC310, 0x043C,
	0xC312, 0x0002,
	0xC314, 0x0532,
	0xC316, 0x0000,
	0xC318, 0x1DA1,
	0xC31A, 0x0000,
	0xC31C, 0x368D,
	0xC31E, 0x0002,
	0xC320, 0x0400,
	0xC322, 0x0000,
	0xC324, 0x0041,
	0xC326, 0x009A,
	0xC328, 0x01BA,
	0xC32A, 0x0237,
	0xC32C, 0x0356,
	0xC32E, 0x0041,
	0xC330, 0x009A,
	0xC332, 0x01BA,
	0xC334, 0x0237,
	0xC336, 0x0356,
	0xC338, 0x0041,
	0xC33A, 0x009A,
	0xC33C, 0x01BA,
	0xC33E, 0x0237,
	0xC340, 0x0356,
	0xC342, 0x0041,
	0xC344, 0x0096,
	0xC346, 0x01B8,
	0xC348, 0x0242,
	0xC34A, 0x035C,
	0xC34C, 0x000C,
	0xC34E, 0x2015,
	0xC350, 0x0026,
	0xC352, 0x000C,
	0xC354, 0x01B4,
	0xC356, 0x0000,
	0xC358, 0x0D84,
	0xC35A, 0x0000,
	0xC35C, 0x4AE3,
	0xC35E, 0x0002,
	0xC360, 0x1752,
	0xC362, 0x0002,
	0xC364, 0x0005,
	0xC366, 0x2012,
	0xC368, 0x002D,
	0xC36A, 0x2000,
	0xC36C, 0x00F3,
	0xC36E, 0x0000,
	0xC370, 0x0937,
	0xC372, 0x0000,
	0xC374, 0x5571,
	0xC376, 0x0002,
	0xC378, 0x05FD,
	0xC37A, 0x0000,
	0xC37C, 0x2005,
	0xC37E, 0x000F,
	0xC380, 0x0033,
	0xC382, 0x2034,
	0xC384, 0x0402,
	0xC386, 0x0000,
	0xC388, 0x032B,
	0xC38A, 0x0002,
	0xC38C, 0x3862,
	0xC38E, 0x0002,
	0xC390, 0x993F,
	0xC392, 0x0000,
	0xC394, 0x0059,
	0xC396, 0x2023,
	0xC398, 0x0013,
	0xC39A, 0x000E,
	0xC39C, 0x029A,
	0xC39E, 0x0000,
	0xC3A0, 0x2E74,
	0xC3A2, 0x0000,
	0xC3A4, 0x23DB,
	0xC3A6, 0x0002,
	0xC3A8, 0x1AE6,
	0xC3AA, 0x0002,
	0x8000, 0x0040,
	0x8040, 0x00AC,
	0x8042, 0x00AE,
	0x8044, 0x00B0,
	0x8046, 0x00B4,
	0x8048, 0x00B9,
	0x804A, 0x00BE,
	0x804C, 0x00C3,
	0x804E, 0x00C8,
	0x8050, 0x00CD,
	0x8052, 0x00D2,
	0x8054, 0x00D7,
	0x8056, 0x00DC,
	0x8058, 0x00E1,
	0x805A, 0x00E6,
	0x805C, 0x00EB,
	0x805E, 0x00F0,
	0x8060, 0x00F5,
	0x8062, 0x00FA,
	0x8064, 0x00FF,
	0x8066, 0x0104,
	0x8068, 0x0109,
	0x806A, 0x010E,
	0x806C, 0x0113,
	0x806E, 0x0118,
	0x8070, 0x011D,
	0x8072, 0x0122,
	0x8074, 0x0127,
	0x8076, 0x012C,
	0x8078, 0x0134,
	0x807A, 0x010E,
	0x807C, 0x0129,
	0x807E, 0x0128,
	0x8080, 0x0127,
	0x8082, 0x012B,
	0x8084, 0x0136,
	0x8086, 0x0134,
	0x8088, 0x0158,
	0x808A, 0x015A,
	0x808C, 0x0169,
	0x808E, 0x0186,
	0x8090, 0x0151,
	0x8092, 0x018B,
	0x8094, 0x0190,
	0x8096, 0x01A7,
	0x8098, 0x019F,
	0x809A, 0x016E,
	0x809C, 0x01C9,
	0x809E, 0x01B8,
	0x80A0, 0x01AE,
	0x80A2, 0x01CA,
	0x80A4, 0x01DD,
	0x80A6, 0x01B5,
	0x80A8, 0x0217,
	0x80AA, 0x01F7,
	0x80AC, 0x020F,
	0x80AE, 0x0208,
	0x80B0, 0x0230,
	0x80B2, 0x0205,
	0x80B4, 0x0264,
	0x80B6, 0x024A,
	0x80B8, 0x0250,
	0x80BA, 0x0250,
	0x80BC, 0x023B,
	0x80BE, 0x0281,
	0x80C0, 0x027D,
	0x80C2, 0x0288,
	0x80C4, 0x02A7,
	0x80C6, 0x02AD,
	0x80C8, 0x02B4,
	0x80CA, 0x02C0,
	0x80CC, 0x02C2,
	0x80CE, 0x02C8,
	0x80D0, 0x02E1,
	0x80D2, 0x02DC,
	0x80D4, 0x02E3,
	0x80D6, 0x0305,
	0x80D8, 0x031B,
	0x80DA, 0x0354,
	0x80DC, 0x0364,
	0x80DE, 0x0342,
	0x80E0, 0x033F,
	0x80E2, 0x037C,
	0x80E4, 0x0382,
	0x80E6, 0x0367,
	0x80E8, 0x039B,
	0x80EA, 0x0385,
	0x80EC, 0x036E,
	0x80EE, 0x03E2,
	0x80F0, 0x03F2,
	0x80F2, 0x03F8,
	0x80F4, 0x0425,
	0x80F6, 0x0433,
	0x80F8, 0x0443,
	0x80FA, 0x0463,
	0x80FC, 0x04A5,
	0x80FE, 0x0AF8,
	0x8100, 0x0008,
	0x8102, 0x12DE,
	0x8104, 0x25BC,
	0x8106, 0xBCAD,
	0x8108, 0x0000,
	0x810A, 0x2000,
	0x810C, 0x0000,
	0x810E, 0x2000,
	0x8110, 0x0000,
	0x8112, 0x2000,
	0x8114, 0x0000,
	0x8116, 0x2000,
	0x8118, 0x0000,
	0x811A, 0x0030,
	0x811C, 0x0070,
	0x811E, 0x00F0,
	0x8120, 0x0000,
	0x8122, 0x2000,
	0x8124, 0x0000,
	0x8128, 0x0000,
	0x812C, 0x0000,
	0x8130, 0x019A,
	0x8132, 0x01B1,
	0x8134, 0x01C9,
	0x8136, 0x01E2,
	0x8138, 0x0000,
	0x813A, 0x1600,
	0x813C, 0x0000,
	0x813E, 0x1600,
	0x8140, 0x0000,
	0x8142, 0x2000,
	0x8144, 0x0000,
	0x8146, 0x2000,
	0x8148, 0x0000,
	0x814A, 0x0030,
	0x814C, 0x0070,
	0x814E, 0x00F0,
	0x8150, 0x0000,
	0x8152, 0x2000,
	0x8154, 0x0000,
	0x8156, 0x2000,
	0x8158, 0x0000,
	0x815A, 0x2000,
	0x815C, 0x0000,
	0x815E, 0x2000,
	0x8160, 0x0000,
	0x8162, 0x2000,
	0x8164, 0x0000,
	0x8166, 0x0000,
	0x301C, 0x0000,
	0x5068, 0x12FE,
	0x506A, 0x30B7,
	0x506C, 0x11D0,
	0x506E, 0x3726,
	0x5070, 0x11FE,
	0x5072, 0x3869,
	0x5074, 0x1241,
	0x5076, 0x39B1,
	0x5078, 0x12C6,
	0x507A, 0x3BE7,
	0x507C, 0x12C4,
	0x507E, 0x3E2E,
	0x5080, 0x12A1,
	0x5082, 0x3F54,
	0x5084, 0x11DA,
	0x5086, 0x3E84,
	0x5088, 0x10B5,
	0x508A, 0x3DBA,
	0x508C, 0x0FD7,
	0x508E, 0x3CF8,
	0x5090, 0x0F1F,
	0x5092, 0x3B70,
	0x5094, 0x0E56,
	0x5096, 0x3BF1,
	0x5098, 0x0CB6,
	0x509A, 0x3FAA,
	0x509C, 0x119E,
	0x509E, 0x3405,
	0x50A0, 0x1127,
	0x50A2, 0x356F,
	0x50A4, 0x1134,
	0x50A6, 0x3718,
	0x50A8, 0x118A,
	0x50AA, 0x38B0,
	0x50AC, 0x1203,
	0x50AE, 0x3B52,
	0x50B0, 0x1250,
	0x50B2, 0x3D61,
	0x50B4, 0x1218,
	0x50B6, 0x3E47,
	0x50B8, 0x1169,
	0x50BA, 0x3CF5,
	0x50BC, 0x1059,
	0x50BE, 0x3C62,
	0x50C0, 0x0F17,
	0x50C2, 0x3B84,
	0x50C4, 0x0E6F,
	0x50C6, 0x3A40,
	0x50C8, 0x0E04,
	0x50CA, 0x3945,
	0x50CC, 0x0CD3,
	0x50CE, 0x3C5A,
	0x50D0, 0x10EF,
	0x50D2, 0x335A,
	0x50D4, 0x1077,
	0x50D6, 0x340C,
	0x50D8, 0x10C7,
	0x50DA, 0x34FB,
	0x50DC, 0x1109,
	0x50DE, 0x37AB,
	0x50E0, 0x11A3,
	0x50E2, 0x39FF,
	0x50E4, 0x11E6,
	0x50E6, 0x3C34,
	0x50E8, 0x11A0,
	0x50EA, 0x3D9C,
	0x50EC, 0x10C4,
	0x50EE, 0x3CDE,
	0x50F0, 0x0FC7,
	0x50F2, 0x3B46,
	0x50F4, 0x0EA7,
	0x50F6, 0x39AA,
	0x50F8, 0x0DA9,
	0x50FA, 0x3915,
	0x50FC, 0x0D7F,
	0x50FE, 0x37F7,
	0x5100, 0x0CCF,
	0x5102, 0x396E,
	0x5104, 0x1042,
	0x5106, 0x32DE,
	0x5108, 0x1023,
	0x510A, 0x335C,
	0x510C, 0x1093,
	0x510E, 0x3391,
	0x5110, 0x10D9,
	0x5112, 0x3655,
	0x5114, 0x11A7,
	0x5116, 0x3853,
	0x5118, 0x11E3,
	0x511A, 0x3AC4,
	0x511C, 0x113A,
	0x511E, 0x3D64,
	0x5120, 0x100A,
	0x5122, 0x3D19,
	0x5124, 0x0EF0,
	0x5126, 0x3BB3,
	0x5128, 0x0E3A,
	0x512A, 0x38F2,
	0x512C, 0x0D1F,
	0x512E, 0x37DD,
	0x5130, 0x0CE3,
	0x5132, 0x376A,
	0x5134, 0x0C9E,
	0x5136, 0x3810,
	0x5138, 0x1008,
	0x513A, 0x32BD,
	0x513C, 0x0FFB,
	0x513E, 0x3254,
	0x5140, 0x101D,
	0x5142, 0x3394,
	0x5144, 0x10EC,
	0x5146, 0x350A,
	0x5148, 0x11EB,
	0x514A, 0x36EB,
	0x514C, 0x11AD,
	0x514E, 0x3A7B,
	0x5150, 0x10FD,
	0x5152, 0x3D4E,
	0x5154, 0x0F97,
	0x5156, 0x3E26,
	0x5158, 0x0E2B,
	0x515A, 0x3D2C,
	0x515C, 0x0DAB,
	0x515E, 0x390C,
	0x5160, 0x0CE2,
	0x5162, 0x37A0,
	0x5164, 0x0C71,
	0x5166, 0x372E,
	0x5168, 0x0C3D,
	0x516A, 0x37C0,
	0x516C, 0x0FAD,
	0x516E, 0x325D,
	0x5170, 0x0FEB,
	0x5172, 0x3262,
	0x5174, 0x1024,
	0x5176, 0x33E8,
	0x5178, 0x10E6,
	0x517A, 0x34F0,
	0x517C, 0x121E,
	0x517E, 0x3677,
	0x5180, 0x11A0,
	0x5182, 0x3B29,
	0x5184, 0x111E,
	0x5186, 0x3D3B,
	0x5188, 0x0F9E,
	0x518A, 0x3D75,
	0x518C, 0x0DDF,
	0x518E, 0x3D36,
	0x5190, 0x0D7D,
	0x5192, 0x3861,
	0x5194, 0x0CE8,
	0x5196, 0x3682,
	0x5198, 0x0C7E,
	0x519A, 0x365D,
	0x519C, 0x0C6D,
	0x519E, 0x3732,
	0x51A0, 0x0FB4,
	0x51A2, 0x3342,
	0x51A4, 0x0FBF,
	0x51A6, 0x3307,
	0x51A8, 0x1029,
	0x51AA, 0x344B,
	0x51AC, 0x10CB,
	0x51AE, 0x3618,
	0x51B0, 0x11E2,
	0x51B2, 0x3747,
	0x51B4, 0x11BB,
	0x51B6, 0x3A7D,
	0x51B8, 0x10FA,
	0x51BA, 0x3D5F,
	0x51BC, 0x0F8D,
	0x51BE, 0x3DAD,
	0x51C0, 0x0E53,
	0x51C2, 0x3C22,
	0x51C4, 0x0DE3,
	0x51C6, 0x37C9,
	0x51C8, 0x0D2B,
	0x51CA, 0x35DE,
	0x51CC, 0x0CD3,
	0x51CE, 0x35C3,
	0x51D0, 0x0C95,
	0x51D2, 0x363E,
	0x51D4, 0x1008,
	0x51D6, 0x3428,
	0x51D8, 0x1008,
	0x51DA, 0x3499,
	0x51DC, 0x1049,
	0x51DE, 0x355F,
	0x51E0, 0x10E0,
	0x51E2, 0x379E,
	0x51E4, 0x118A,
	0x51E6, 0x3956,
	0x51E8, 0x11CD,
	0x51EA, 0x3B1B,
	0x51EC, 0x112B,
	0x51EE, 0x3D51,
	0x51F0, 0x1026,
	0x51F2, 0x3CA5,
	0x51F4, 0x0F5D,
	0x51F6, 0x3A97,
	0x51F8, 0x0EB0,
	0x51FA, 0x3776,
	0x51FC, 0x0DE5,
	0x51FE, 0x359B,
	0x5200, 0x0D95,
	0x5202, 0x353C,
	0x5204, 0x0D09,
	0x5206, 0x3645,
	0x5208, 0x1094,
	0x520A, 0x3551,
	0x520C, 0x1056,
	0x520E, 0x369E,
	0x5210, 0x10AB,
	0x5212, 0x3676,
	0x5214, 0x10FE,
	0x5216, 0x3917,
	0x5218, 0x1187,
	0x521A, 0x3B4C,
	0x521C, 0x11C5,
	0x521E, 0x3CA0,
	0x5220, 0x11B4,
	0x5222, 0x3D7E,
	0x5224, 0x112B,
	0x5226, 0x3BE7,
	0x5228, 0x1063,
	0x522A, 0x39EB,
	0x522C, 0x0F73,
	0x522E, 0x37CC,
	0x5230, 0x0E9B,
	0x5232, 0x362F,
	0x5234, 0x0E66,
	0x5236, 0x3593,
	0x5238, 0x0D56,
	0x523A, 0x37EC,
	0x523C, 0x1133,
	0x523E, 0x35E6,
	0x5240, 0x1092,
	0x5242, 0x386D,
	0x5244, 0x10E7,
	0x5246, 0x39A9,
	0x5248, 0x115D,
	0x524A, 0x3AC5,
	0x524C, 0x11EA,
	0x524E, 0x3CD8,
	0x5250, 0x122B,
	0x5252, 0x3E62,
	0x5254, 0x123D,
	0x5256, 0x3EA4,
	0x5258, 0x11D2,
	0x525A, 0x3CB0,
	0x525C, 0x10FC,
	0x525E, 0x3AB0,
	0x5260, 0x1024,
	0x5262, 0x3929,
	0x5264, 0x0F78,
	0x5266, 0x376D,
	0x5268, 0x0EA3,
	0x526A, 0x3769,
	0x526C, 0x0D67,
	0x526E, 0x39D9,
	0x5270, 0x1202,
	0x5272, 0x359F,
	0x5274, 0x1161,
	0x5276, 0x38D6,
	0x5278, 0x1173,
	0x527A, 0x3B45,
	0x527C, 0x11C9,
	0x527E, 0x3C28,
	0x5280, 0x1259,
	0x5282, 0x3D58,
	0x5284, 0x12B7,
	0x5286, 0x3EF7,
	0x5288, 0x12CA,
	0x528A, 0x3F96,
	0x528C, 0x121E,
	0x528E, 0x3E0F,
	0x5290, 0x117F,
	0x5292, 0x3C7E,
	0x5294, 0x10D3,
	0x5296, 0x3A79,
	0x5298, 0x102D,
	0x529A, 0x3946,
	0x529C, 0x0EE2,
	0x529E, 0x3A94,
	0x52A0, 0x0D8A,
	0x52A2, 0x3D1D,
	0x0318, 0x0019,
	0x0344, 0x0002,
	0x0348, 0x468E,
	0x034A, 0xEAA9,
	0x0364, 0x3200,
	0x0366, 0x0380,
	0x036C, 0x8391,
	0x036E, 0x00C4,
	0x0370, 0x00C4,
	0x06A6, 0x6700,
	0x06D4, 0x0041,
	0x613C, 0x7070,
	0x613E, 0x5858,
	0x6154, 0x0300,
	0x1102, 0x0008,
	0x11C6, 0x0009,
	0x1742, 0xFFFF,
	0x3006, 0x0003,
	0x0A04, 0xB401,
	0x0A06, 0xC400,
	0x0A08, 0xC88F,
	0x0A0C, 0xF055,
	0x0A0E, 0xEE10,
	0x0A12, 0x0002,
	0x0A18, 0x0014,
	0x0A24, 0x00C4,
	0x070A, 0x1F80,
	0x07A0, 0x0001,
	0x07C6, 0x1A00,
	0x07C8, 0x3400,
	0x07CA, 0x4E00,
	0x07CC, 0x6800,
	0x07CE, 0x0100,
	0x07D0, 0x0100,
	0x07D2, 0x0100,
	0x07D4, 0x0100,
	0x07D6, 0x0100,
	0x07D8, 0x0100,
	0x1000, 0x0300,
	0x1030, 0x1F01,
	0x1032, 0x0008,
	0x1034, 0x0300,
	0x1036, 0x0103,
	0x1064, 0x0300,
	0x027E, 0x0100,
};
#endif

static void sensor_init(void)
{
	LOG_INF("E\n");
#if MULTI_WRITE
	hi4821q_table_write_cmos_sensor(
		addr_data_pair_init_hi4821q,
		sizeof(addr_data_pair_init_hi4821q) /
		sizeof(kal_uint16));
#else


#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_preview_hi4821q[] = {
	0x0B00, 0x0000,
	0x8002, 0x0044,
	0x8004, 0x0047,
	0x8006, 0x004A,
	0x8008, 0x004D,
	0x800A, 0x0052,
	0x800C, 0x0056,
	0x800E, 0x005A,
	0x8010, 0x005F,
	0x8012, 0x0060,
	0x8014, 0x0065,
	0x8016, 0x006B,
	0x8018, 0x006E,
	0x801A, 0x0071,
	0x801C, 0x0076,
	0x801E, 0x007C,
	0x8020, 0x007C,
	0x8022, 0x007C,
	0x8024, 0x0083,
	0x8026, 0x008D,
	0x8028, 0x008F,
	0x802A, 0x0099,
	0x802C, 0x009A,
	0x802E, 0x009B,
	0x8030, 0x00A3,
	0x8032, 0x00A8,
	0x8034, 0x00AC,
	0x8036, 0x00B0,
	0x8038, 0x00B7,
	0x803A, 0x00B8,
	0x803C, 0x00AF,
	0x803E, 0x00AF,
	0x8126, 0x1A00,
	0x812A, 0x1700,
	0x812E, 0x15C0,
	0x813A, 0x1800,
	0x813E, 0x1C00,
	0x0204, 0x4408,
	0x0206, 0x0386,
	0x020C, 0x0F8F,
	0x0210, 0x0F97,
	0x0226, 0x0046,
	0x022A, 0x1B66,
	0x022C, 0x0090,
	0x022E, 0x1816,
	0x0244, 0x0103,
	0x0246, 0x0301,
	0x0248, 0x8013,
	0x024A, 0x0901,
	0x024C, 0x0103,
	0x024E, 0x0301,
	0x0250, 0x8013,
	0x0252, 0x0901,
	0x0254, 0x0103,
	0x0256, 0x0301,
	0x0258, 0x8013,
	0x025A, 0x0901,
	0x025C, 0x0088,
	0x025E, 0x0088,
	0x0262, 0x0010,
	0x0266, 0x0010,
	0x026C, 0x0223,
	0x0400, 0x3047,
	0x0402, 0x1133,
	0x040C, 0x0000,
	0x040E, 0x0001,
	0x0410, 0x01F5,
	0x041E, 0x8540,
	0x0420, 0x1400,
	0x0422, 0x00E1,
	0x0426, 0x0404,
	0x042C, 0x0000,
	0x042E, 0x0BC8,
	0x0442, 0x0508,
	0x044C, 0x0100,
	0x0452, 0x8584,
	0x0454, 0x0205,
	0x0456, 0xFFFF,
	0x045C, 0x020F,
	0x045E, 0x1FFE,
	0x046C, 0x0002,
	0x046E, 0x03EB,
	0x0470, 0x0002,
	0x0472, 0x0002,
	0x04A4, 0x0001,
	0x0D00, 0x4000,
	0x0D28, 0x0008,
	0x0D2A, 0x0FAF,
	0x0D2C, 0x0000,
	0x0D50, 0x81F7,
	0x0D52, 0x0010,
	0x0600, 0x1131,
	0x0602, 0x7112,
	0x0604, 0x8C4B,
	0x0624, 0x0004,
	0x06D0, 0x0064,
	0x06D2, 0x0230,
	0x06E4, 0x8040,
	0x06E6, 0x0040,
	0x06E8, 0x0010,
	0x06EA, 0x8040,
	0x06F6, 0xFF00,
	0x238C, 0x0000,
	0x238E, 0x0000,
	0x2390, 0x0303,
	0x2392, 0x0303,
	0x2394, 0x0303,
	0x2396, 0x0303,
	0x2398, 0x0303,
	0x239A, 0x0303,
	0x239C, 0x0303,
	0x239E, 0x0303,
	0x23A0, 0x0303,
	0x23A2, 0x0303,
	0x23A4, 0x0303,
	0x23A6, 0x0303,
	0x23A8, 0x0303,
	0x23AA, 0x0303,
	0x23AC, 0x0303,
	0x23AE, 0x0303,
	0x0F00, 0x0000,
	0x0F04, 0x0004,
	0x0F06, 0x0008,
	0x0F08, 0x0011,
	0x0F0A, 0x2233,
	0x0F0C, 0x4455,
	0x0F0E, 0x6677,
	0x0B04, 0x90E0,
	0x0B20, 0x0FA0,
	0x0B22, 0x0BB8,
	0x0B30, 0x0000,
	0x1108, 0x1001,
	0x1118, 0x0000,
	0x1702, 0x1C9C,
	0x1704, 0x0000,
	0x1706, 0x0000,
	0x1708, 0x0058,
	0x170A, 0x0038,
	0x170C, 0x8144,
	0x1718, 0x0000,
	0x1740, 0x0000,
	0x1F04, 0x0000,
	0x1F06, 0x5838,
	0x1F08, 0x03FE,
	0x1F0A, 0x0054,
	0x1D06, 0x0010,
	0x0A0A, 0xC372,
	0x0A10, 0xD840,
	0x0C00, 0x0221,
	0x0C02, 0x0FA8,
	0x0C04, 0x0BC8,
	0x0C14, 0x0004,
	0x0C16, 0x0008,
	0x0C18, 0x0FA0,
	0x0C1A, 0x0BB8,
	0x0736, 0x0087,
	0x0738, 0x0302,
	0x073C, 0x0900,
	0x0746, 0x00AF,
	0x0748, 0x0002,
	0x0766, 0x0032,
	0x1206, 0x0860,
	0x1C00, 0x0140,
	0x1600, 0xF080,
	0x1608, 0x0014,
	0x160A, 0x0F80,
	0x160C, 0x0014,
	0x160E, 0x0BA0,
	0x1614, 0x0305,
	0x1410, 0x0001,
	0x1410, 0x0001,
	0x1004, 0x2BB0,
	0x101C, 0x021A,
	0x1020, 0xC107,
	0x1022, 0x081A,
	0x1024, 0x0506,
	0x1026, 0x0909,
	0x1028, 0x120A,
	0x102A, 0x0A0A,
	0x102C, 0x110A,
	0x1038, 0x2100,
	0x1042, 0x0108,
	0x1044, 0x01F0,
	0x1048, 0x01F0,
};
#endif


static void preview_setting(void)
{
	LOG_INF("E\n");
#if MULTI_WRITE
	hi4821q_table_write_cmos_sensor(
		addr_data_pair_preview_hi4821q,
		sizeof(addr_data_pair_preview_hi4821q) /
		sizeof(kal_uint16));
#else

#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_capture_30fps_hi4821q[] = {
	0x0B00, 0x0000,
	0x8002, 0x0044,
	0x8004, 0x0047,
	0x8006, 0x004A,
	0x8008, 0x004D,
	0x800A, 0x0052,
	0x800C, 0x0056,
	0x800E, 0x005A,
	0x8010, 0x005F,
	0x8012, 0x0060,
	0x8014, 0x0065,
	0x8016, 0x006B,
	0x8018, 0x006E,
	0x801A, 0x0071,
	0x801C, 0x0076,
	0x801E, 0x007C,
	0x8020, 0x007C,
	0x8022, 0x007C,
	0x8024, 0x0083,
	0x8026, 0x008D,
	0x8028, 0x008F,
	0x802A, 0x0099,
	0x802C, 0x009A,
	0x802E, 0x009B,
	0x8030, 0x00A3,
	0x8032, 0x00A8,
	0x8034, 0x00AC,
	0x8036, 0x00B0,
	0x8038, 0x00B7,
	0x803A, 0x00B8,
	0x803C, 0x00AF,
	0x803E, 0x00AF,
	0x8126, 0x1A00,
	0x812A, 0x1700,
	0x812E, 0x15C0,
	0x813A, 0x1800,
	0x813E, 0x1C00,
	0x0204, 0x4408,
	0x0206, 0x0386,
	0x020C, 0x0F8F,
	0x0210, 0x0F97,
	0x0226, 0x0046,
	0x022A, 0x1B66,
	0x022C, 0x0090,
	0x022E, 0x1816,
	0x0244, 0x0103,
	0x0246, 0x0301,
	0x0248, 0x8013,
	0x024A, 0x0901,
	0x024C, 0x0103,
	0x024E, 0x0301,
	0x0250, 0x8013,
	0x0252, 0x0901,
	0x0254, 0x0103,
	0x0256, 0x0301,
	0x0258, 0x8013,
	0x025A, 0x0901,
	0x025C, 0x0088,
	0x025E, 0x0088,
	0x0262, 0x0010,
	0x0266, 0x0010,
	0x026C, 0x0223,
	0x0400, 0x3047,
	0x0402, 0x1133,
	0x040C, 0x0000,
	0x040E, 0x0001,
	0x0410, 0x01F5,
	0x041E, 0x8540,
	0x0420, 0x1400,
	0x0422, 0x00E1,
	0x0426, 0x0404,
	0x042C, 0x0000,
	0x042E, 0x0BC8,
	0x0442, 0x0508,
	0x044C, 0x0100,
	0x0452, 0x8584,
	0x0454, 0x0205,
	0x0456, 0xFFFF,
	0x045C, 0x020F,
	0x045E, 0x1FFE,
	0x046C, 0x0002,
	0x046E, 0x03EB,
	0x0470, 0x0002,
	0x0472, 0x0002,
	0x04A4, 0x0001,
	0x0D00, 0x4000,
	0x0D28, 0x0008,
	0x0D2A, 0x0FAF,
	0x0D2C, 0x0000,
	0x0D50, 0x81F7,
	0x0D52, 0x0010,
	0x0600, 0x1131,
	0x0602, 0x7112,
	0x0604, 0x8C4B,
	0x0624, 0x0004,
	0x06D0, 0x0064,
	0x06D2, 0x0230,
	0x06E4, 0x8040,
	0x06E6, 0x0040,
	0x06E8, 0x0010,
	0x06EA, 0x8040,
	0x06F6, 0xFF00,
	0x238C, 0x0000,
	0x238E, 0x0000,
	0x2390, 0x0303,
	0x2392, 0x0303,
	0x2394, 0x0303,
	0x2396, 0x0303,
	0x2398, 0x0303,
	0x239A, 0x0303,
	0x239C, 0x0303,
	0x239E, 0x0303,
	0x23A0, 0x0303,
	0x23A2, 0x0303,
	0x23A4, 0x0303,
	0x23A6, 0x0303,
	0x23A8, 0x0303,
	0x23AA, 0x0303,
	0x23AC, 0x0303,
	0x23AE, 0x0303,
	0x0F00, 0x0000,
	0x0F04, 0x0004,
	0x0F06, 0x0008,
	0x0F08, 0x0011,
	0x0F0A, 0x2233,
	0x0F0C, 0x4455,
	0x0F0E, 0x6677,
	0x0B04, 0x90E0,
	0x0B20, 0x0FA0,
	0x0B22, 0x0BB8,
	0x0B30, 0x0000,
	0x1108, 0x1001,
	0x1118, 0x0000,
	0x1702, 0x1C9C,
	0x1704, 0x0000,
	0x1706, 0x0000,
	0x1708, 0x0058,
	0x170A, 0x0038,
	0x170C, 0x8144,
	0x1718, 0x0000,
	0x1740, 0x0000,
	0x1F04, 0x0000,
	0x1F06, 0x5838,
	0x1F08, 0x03FE,
	0x1F0A, 0x0054,
	0x1D06, 0x0010,
	0x0A0A, 0xC372,
	0x0A10, 0xD840,
	0x0C00, 0x0221,
	0x0C02, 0x0FA8,
	0x0C04, 0x0BC8,
	0x0C14, 0x0004,
	0x0C16, 0x0008,
	0x0C18, 0x0FA0,
	0x0C1A, 0x0BB8,
	0x0736, 0x0087,
	0x0738, 0x0302,
	0x073C, 0x0900,
	0x0746, 0x00AF,
	0x0748, 0x0002,
	0x0766, 0x0032,
	0x1206, 0x0860,
	0x1C00, 0x0140,
	0x1600, 0xF080,
	0x1608, 0x0014,
	0x160A, 0x0F80,
	0x160C, 0x0014,
	0x160E, 0x0BA0,
	0x1614, 0x0305,
	0x1410, 0x0001,
	0x1410, 0x0001,
	0x1004, 0x2BB0,
	0x101C, 0x021A,
	0x1020, 0xC107,
	0x1022, 0x081A,
	0x1024, 0x0506,
	0x1026, 0x0909,
	0x1028, 0x120A,
	0x102A, 0x0A0A,
	0x102C, 0x110A,
	0x1038, 0x2100,
	0x1042, 0x0108,
	0x1044, 0x01F0,
	0x1048, 0x01F0,
};

kal_uint16 addr_data_pair_capture_15fps_hi4821q[] = {

     };
#endif


static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E\n");
#if MULTI_WRITE

	hi4821q_table_write_cmos_sensor(
		addr_data_pair_capture_30fps_hi4821q,
		sizeof(addr_data_pair_capture_30fps_hi4821q) /
		sizeof(kal_uint16));

#else

#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_video_hi4821q[] = {
	0x0B00, 0x0000,
	0x8002, 0x0044,
	0x8004, 0x0047,
	0x8006, 0x004A,
	0x8008, 0x004D,
	0x800A, 0x0052,
	0x800C, 0x0056,
	0x800E, 0x005A,
	0x8010, 0x005F,
	0x8012, 0x0060,
	0x8014, 0x0065,
	0x8016, 0x006B,
	0x8018, 0x006E,
	0x801A, 0x0071,
	0x801C, 0x0076,
	0x801E, 0x007C,
	0x8020, 0x007C,
	0x8022, 0x007C,
	0x8024, 0x0083,
	0x8026, 0x008D,
	0x8028, 0x008F,
	0x802A, 0x0099,
	0x802C, 0x009A,
	0x802E, 0x009B,
	0x8030, 0x00A3,
	0x8032, 0x00A8,
	0x8034, 0x00AC,
	0x8036, 0x00B0,
	0x8038, 0x00B7,
	0x803A, 0x00B8,
	0x803C, 0x00AF,
	0x803E, 0x00AF,
	0x8126, 0x1A00,
	0x812A, 0x1700,
	0x812E, 0x15C0,
	0x813A, 0x1800,
	0x813E, 0x1C00,
	0x0204, 0x0408,
	0x0206, 0x0450,
	0x020C, 0x0CB4,
	0x0210, 0x0CBC,
	0x0226, 0x0046,
	0x022A, 0x1B66,
	0x022C, 0x0090,
	0x022E, 0x1816,
	0x0244, 0x0301,
	0x0246, 0x0301,
	0x0248, 0x0301,
	0x024A, 0x1301,
	0x024C, 0x0301,
	0x024E, 0x0301,
	0x0250, 0x0301,
	0x0252, 0x1301,
	0x0254, 0x0301,
	0x0256, 0x0301,
	0x0258, 0x0301,
	0x025A, 0x1301,
	0x025C, 0x0088,
	0x025E, 0x0044,
	0x0262, 0x0010,
	0x0266, 0x0010,
	0x026C, 0x0221,
	0x0400, 0x3007,
	0x0402, 0x1111,
	0x040C, 0x0000,
	0x040E, 0x0001,
	0x0410, 0x01F5,
	0x041E, 0x2131,
	0x0420, 0x1400,
	0x0422, 0x0450,
	0x0426, 0x0404,
	0x042C, 0x0000,
	0x042E, 0x0BC8,
	0x0442, 0x0508,
	0x044C, 0x0100,
	0x0452, 0x8584,
	0x0454, 0x0205,
	0x0456, 0xFFFF,
	0x045C, 0x020F,
	0x045E, 0x1FFE,
	0x046C, 0x0002,
	0x046E, 0x03EB,
	0x0470, 0x0002,
	0x0472, 0x0002,
	0x04A4, 0x0001,
	0x0D00, 0x4000,
	0x0D28, 0x0008,
	0x0D2A, 0x0FAF,
	0x0D2C, 0x0000,
	0x0D50, 0x81F7,
	0x0D52, 0x0010,
	0x0600, 0x1131,
	0x0602, 0x7112,
	0x0604, 0x884B,
	0x0624, 0x0004,
	0x06D0, 0x0064,
	0x06D2, 0x0230,
	0x06E4, 0x8040,
	0x06E6, 0x0040,
	0x06E8, 0x0010,
	0x06EA, 0x8040,
	0x06F6, 0x0000,
	0x238C, 0x0000,
	0x238E, 0x0000,
	0x2390, 0x0303,
	0x2392, 0x0303,
	0x2394, 0x0303,
	0x2396, 0x0303,
	0x2398, 0x0303,
	0x239A, 0x0303,
	0x239C, 0x0303,
	0x239E, 0x0303,
	0x23A0, 0x0303,
	0x23A2, 0x0303,
	0x23A4, 0x0303,
	0x23A6, 0x0303,
	0x23A8, 0x0303,
	0x23AA, 0x0303,
	0x23AC, 0x0303,
	0x23AE, 0x0303,
	0x0F00, 0x0000,
	0x0F04, 0x0004,
	0x0F06, 0x0008,
	0x0F08, 0x0011,
	0x0F0A, 0x2233,
	0x0F0C, 0x4455,
	0x0F0E, 0x6677,
	0x0B04, 0x90E0,
	0x0B20, 0x0FA0,
	0x0B22, 0x0BB8,
	0x0B30, 0x0000,
	0x1108, 0x1001,
	0x1118, 0x0000,
	0x1702, 0x1C9C,
	0x1704, 0x0000,
	0x1706, 0x0000,
	0x1708, 0x0058,
	0x170A, 0x0038,
	0x170C, 0x8144,
	0x1718, 0x0000,
	0x1740, 0x0000,
	0x1F04, 0x0000,
	0x1F06, 0x5838,
	0x1F08, 0x03FE,
	0x1F0A, 0x0054,
	0x1D06, 0x0010,
	0x0A0A, 0xC372,
	0x0A10, 0xD840,
	0x0C00, 0x0221,
	0x0C02, 0x0FA8,
	0x0C04, 0x0BC8,
	0x0C14, 0x0004,
	0x0C16, 0x0008,
	0x0C18, 0x0FA0,
	0x0C1A, 0x0BB8,
	0x0736, 0x0087,
	0x0738, 0x0302,
	0x073C, 0x0900,
	0x0746, 0x00AF,
	0x0748, 0x0002,
	0x0766, 0x0034,
	0x1206, 0x0860,
	0x1C00, 0x0140,
	0x1600, 0x1080,
	0x1608, 0x0030,
	0x160A, 0x1F00,
	0x160C, 0x0028,
	0x160E, 0x1740,
	0x1614, 0x0305,
	0x1410, 0x0001,
	0x1410, 0x0001,
	0x1004, 0x2BB0,
	0x101C, 0x0201,
	0x1020, 0xC107,
	0x1022, 0x081A,
	0x1024, 0x0506,
	0x1026, 0x0909,
	0x1028, 0x120A,
	0x102A, 0x0A0A,
	0x102C, 0x110A,
	0x1038, 0x0000,
	0x1042, 0x0008,
	0x1044, 0x01F0,
	0x1048, 0x00F8,
};
#endif

static void video_setting(void)
{
	LOG_INF("E\n");
#if MULTI_WRITE
	hi4821q_table_write_cmos_sensor(
		addr_data_pair_video_hi4821q,
		sizeof(addr_data_pair_video_hi4821q) /
		sizeof(kal_uint16));
#else

#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_hs_video_hi4821q[] = {
	0x0B00, 0x0000,
	0x8002, 0x004C,
	0x8004, 0x0053,
	0x8006, 0x0056,
	0x8008, 0x0060,
	0x800A, 0x0069,
	0x800C, 0x006A,
	0x800E, 0x0072,
	0x8010, 0x007A,
	0x8012, 0x007E,
	0x8014, 0x0090,
	0x8016, 0x0093,
	0x8018, 0x009B,
	0x801A, 0x00A5,
	0x801C, 0x00A6,
	0x801E, 0x00AC,
	0x8020, 0x00B4,
	0x8022, 0x00BF,
	0x8024, 0x00C3,
	0x8026, 0x00C3,
	0x8028, 0x00D2,
	0x802A, 0x00DC,
	0x802C, 0x00DB,
	0x802E, 0x00EF,
	0x8030, 0x00EC,
	0x8032, 0x00F6,
	0x8034, 0x0108,
	0x8036, 0x0109,
	0x8038, 0x0109,
	0x803A, 0x0119,
	0x803C, 0x011B,
	0x803E, 0x011B,
	0x8126, 0x1900,
	0x812A, 0x1700,
	0x812E, 0x1600,
	0x813A, 0x1B00,
	0x813E, 0x1E00,
	0x0204, 0x0908,
	0x0206, 0x02E9,
	0x020C, 0x04B0,
	0x0210, 0x04B8,
	0x0226, 0x0042,
	0x022A, 0x1B62,
	0x022C, 0x03C0,
	0x022E, 0x14E2,
	0x0244, 0x1C04,
	0x0246, 0x0180,
	0x0248, 0x0101,
	0x024A, 0x0901,
	0x024C, 0x1C04,
	0x024E, 0x0180,
	0x0250, 0x0101,
	0x0252, 0x0901,
	0x0254, 0x1C04,
	0x0256, 0x0180,
	0x0258, 0x0101,
	0x025A, 0x0901,
	0x025C, 0x0044,
	0x025E, 0x0044,
	0x0262, 0x0010,
	0x0266, 0x0020,
	0x026C, 0x0112,
	0x0400, 0x380B,
	0x0402, 0x1455,
	0x040C, 0x0000,
	0x040E, 0x0000,
	0x0410, 0x00FA,
	0x041E, 0x2130,
	0x0420, 0x0400,
	0x0422, 0x0174,
	0x0426, 0x0404,
	0x042C, 0x0002,
	0x042E, 0x0448,
	0x0442, 0x4508,
	0x044C, 0x0000,
	0x0452, 0x8557,
	0x0454, 0x1405,
	0x0456, 0xC03F,
	0x045C, 0x030F,
	0x045E, 0x1FFE,
	0x046C, 0x0002,
	0x046E, 0x03EB,
	0x0470, 0x0004,
	0x0472, 0x0002,
	0x04A4, 0x0001,
	0x0D00, 0x4000,
	0x0D28, 0x0000,
	0x0D2A, 0x07D7,
	0x0D2C, 0x6004,
	0x0D50, 0x81F7,
	0x0D52, 0x0010,
	0x0600, 0x1131,
	0x0602, 0x7112,
	0x0604, 0x884B,
	0x0624, 0x0004,
	0x06D0, 0x0300,
	0x06D2, 0x0100,
	0x06E4, 0x8080,
	0x06E6, 0x0080,
	0x06E8, 0x8040,
	0x06EA, 0x8160,
	0x06F6, 0x0000,
	0x238C, 0x0000,
	0x238E, 0x0000,
	0x2390, 0x0D0D,
	0x2392, 0x0D0D,
	0x2394, 0x0D0D,
	0x2396, 0x0D0D,
	0x2398, 0x0D0D,
	0x239A, 0x0D0D,
	0x239C, 0x0D0D,
	0x239E, 0x0D0D,
	0x23A0, 0x0D0D,
	0x23A2, 0x0D0D,
	0x23A4, 0x0D0D,
	0x23A6, 0x0D0D,
	0x23A8, 0x0D0D,
	0x23AA, 0x0D0D,
	0x23AC, 0x0D0D,
	0x23AE, 0x0D0D,
	0x0F00, 0x0000,
	0x0F04, 0x002C,
	0x0F06, 0x0008,
	0x0F08, 0x0011,
	0x0F0A, 0x2233,
	0x0F0C, 0x4455,
	0x0F0E, 0x6677,
	0x0B04, 0x90E0,
	0x0B20, 0x0780,
	0x0B22, 0x0438,
	0x0B30, 0x0000,
	0x1108, 0x0002,
	0x1118, 0x04D0,
	0x1702, 0x1C9C,
	0x1704, 0x0000,
	0x1706, 0x0000,
	0x1708, 0x0050,
	0x170A, 0x0370,
	0x170C, 0x8144,
	0x1718, 0x0000,
	0x1740, 0x0000,
	0x1F04, 0x0003,
	0x1F06, 0x5070,
	0x1F08, 0x03FE,
	0x1F0A, 0x0054,
	0x1D06, 0x0010,
	0x0A0A, 0xC772,
	0x0A10, 0xD840,
	0x0C00, 0x0221,
	0x0C02, 0x0FA8,
	0x0C04, 0x0BC8,
	0x0C14, 0x002C,
	0x0C16, 0x0008,
	0x0C18, 0x0780,
	0x0C1A, 0x0438,
	0x0736, 0x006C,
	0x0738, 0x0102,
	0x073C, 0x0700,
	0x0746, 0x0158,
	0x0748, 0x0004,
	0x0766, 0x0064,
	0x1206, 0x0860,
	0x1C00, 0x0140,
	0x1600, 0x1080,
	0x1608, 0x0030,
	0x160A, 0x1F00,
	0x160C, 0x0028,
	0x160E, 0x1740,
	0x1614, 0x0305,
	0x1410, 0x0001,
	0x1410, 0x0001,
	0x1004, 0x2BB0,
	0x101C, 0x0201,
	0x1020, 0xC107,
	0x1022, 0x081A,
	0x1024, 0x0506,
	0x1026, 0x0909,
	0x1028, 0x120A,
	0x102A, 0x0A0A,
	0x102C, 0x110A,
	0x1038, 0x0000,
	0x1042, 0x0008,
	0x1044, 0x01F0,
	0x1048, 0x00F8,
};
#endif

static void hs_video_setting(void)
{
	LOG_INF("E\n");
#if MULTI_WRITE
	hi4821q_table_write_cmos_sensor(
		addr_data_pair_hs_video_hi4821q,
		sizeof(addr_data_pair_hs_video_hi4821q) /
		sizeof(kal_uint16));
#else

#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_slim_video_hi4821q[] = {
	0x0B00, 0x0000,
	0x8002, 0x004C,
	0x8004, 0x0053,
	0x8006, 0x0056,
	0x8008, 0x0060,
	0x800A, 0x0069,
	0x800C, 0x006A,
	0x800E, 0x0072,
	0x8010, 0x007A,
	0x8012, 0x007E,
	0x8014, 0x0090,
	0x8016, 0x0093,
	0x8018, 0x009B,
	0x801A, 0x00A5,
	0x801C, 0x00A6,
	0x801E, 0x00AC,
	0x8020, 0x00B4,
	0x8022, 0x00BF,
	0x8024, 0x00C3,
	0x8026, 0x00C3,
	0x8028, 0x00D2,
	0x802A, 0x00DC,
	0x802C, 0x00DB,
	0x802E, 0x00EF,
	0x8030, 0x00EC,
	0x8032, 0x00F6,
	0x8034, 0x0108,
	0x8036, 0x0109,
	0x8038, 0x0109,
	0x803A, 0x0119,
	0x803C, 0x011B,
	0x803E, 0x011B,
	0x8126, 0x1900,
	0x812A, 0x1700,
	0x812E, 0x1600,
	0x813A, 0x1B00,
	0x813E, 0x1E00,
	0x0204, 0x0908,
	0x0206, 0x02E9,
	0x020C, 0x0254,
	0x0210, 0x025C,
	0x0226, 0x0042,
	0x022A, 0x1B62,
	0x022C, 0x0690,
	0x022E, 0x1212,
	0x0244, 0x1C04,
	0x0246, 0x0180,
	0x0248, 0x0101,
	0x024A, 0x0901,
	0x024C, 0x1C04,
	0x024E, 0x0180,
	0x0250, 0x0101,
	0x0252, 0x0901,
	0x0254, 0x1C04,
	0x0256, 0x0180,
	0x0258, 0x0101,
	0x025A, 0x0901,
	0x025C, 0x0044,
	0x025E, 0x0044,
	0x0262, 0x0010,
	0x0266, 0x0020,
	0x026C, 0x0112,
	0x0400, 0x380B,
	0x0402, 0x1455,
	0x040C, 0x0000,
	0x040E, 0x0000,
	0x0410, 0x00FA,
	0x041E, 0x2130,
	0x0420, 0x0400,
	0x0422, 0x0174,
	0x0426, 0x0404,
	0x042C, 0x0002,
	0x042E, 0x02E0,
	0x0442, 0x4508,
	0x044C, 0x0000,
	0x0452, 0x8557,
	0x0454, 0x1405,
	0x0456, 0xC03F,
	0x045C, 0x030F,
	0x045E, 0x1FFE,
	0x046C, 0x0002,
	0x046E, 0x03EB,
	0x0470, 0x0004,
	0x0472, 0x0002,
	0x04A4, 0x0001,
	0x0D00, 0x4000,
	0x0D28, 0x0000,
	0x0D2A, 0x07D7,
	0x0D2C, 0x6004,
	0x0D50, 0x81F7,
	0x0D52, 0x0010,
	0x0600, 0x1131,
	0x0602, 0x7112,
	0x0604, 0x884B,
	0x0624, 0x0004,
	0x06D0, 0x0300,
	0x06D2, 0x0100,
	0x06E4, 0x8080,
	0x06E6, 0x0080,
	0x06E8, 0x8040,
	0x06EA, 0x8160,
	0x06F6, 0x0000,
	0x238C, 0x0000,
	0x238E, 0x0000,
	0x2390, 0x0D0D,
	0x2392, 0x0D0D,
	0x2394, 0x0D0D,
	0x2396, 0x0D0D,
	0x2398, 0x0D0D,
	0x239A, 0x0D0D,
	0x239C, 0x0D0D,
	0x239E, 0x0D0D,
	0x23A0, 0x0D0D,
	0x23A2, 0x0D0D,
	0x23A4, 0x0D0D,
	0x23A6, 0x0D0D,
	0x23A8, 0x0D0D,
	0x23AA, 0x0D0D,
	0x23AC, 0x0D0D,
	0x23AE, 0x0D0D,
	0x0F00, 0x0000,
	0x0F04, 0x016C,
	0x0F06, 0x0008,
	0x0F08, 0x0011,
	0x0F0A, 0x2233,
	0x0F0C, 0x4455,
	0x0F0E, 0x6677,
	0x0B04, 0x90E0,
	0x0B20, 0x0500,
	0x0B22, 0x02D0,
	0x0B30, 0x0000,
	0x1108, 0x0002,
	0x1118, 0x0A1C,
	0x1702, 0x1C9C,
	0x1704, 0x0000,
	0x1706, 0x0001,
	0x1708, 0x0050,
	0x170A, 0x0240,
	0x170C, 0x8144,
	0x1718, 0x0000,
	0x1740, 0x0000,
	0x1F04, 0x0006,
	0x1F06, 0x5040,
	0x1F08, 0x03FE,
	0x1F0A, 0x0054,
	0x1D06, 0x0010,
	0x0A0A, 0xC772,
	0x0A10, 0xD840,
	0x0C00, 0x0221,
	0x0C02, 0x0FA8,
	0x0C04, 0x0BC8,
	0x0C14, 0x016C,
	0x0C16, 0x0008,
	0x0C18, 0x0500,
	0x0C1A, 0x02D0,
	0x0736, 0x006C,
	0x0738, 0x0102,
	0x073C, 0x0700,
	0x0746, 0x01DC,
	0x0748, 0x0004,
	0x0766, 0x0064,
	0x1206, 0x0860,
	0x1C00, 0x0140,
	0x1600, 0x1080,
	0x1608, 0x0030,
	0x160A, 0x1F00,
	0x160C, 0x0028,
	0x160E, 0x1740,
	0x1614, 0x0305,
	0x1410, 0x0001,
	0x1410, 0x0001,
	0x1004, 0x2BB0,
	0x101C, 0x0201,
	0x1020, 0xC10A,
	0x1022, 0x0A2A,
	0x1024, 0x050A,
	0x1026, 0x0F0E,
	0x1028, 0x170E,
	0x102A, 0x100A,
	0x102C, 0x1A0A,
	0x1038, 0x0000,
	0x1042, 0x0008,
	0x1044, 0x01F0,
	0x1048, 0x00F8,
};
#endif


static void slim_video_setting(void)
{
	LOG_INF("E\n");
#if MULTI_WRITE
	hi4821q_table_write_cmos_sensor(
		addr_data_pair_slim_video_hi4821q,
		sizeof(addr_data_pair_slim_video_hi4821q) /
		sizeof(kal_uint16));
#else



#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_custom1_hi4821q[] = {
	0x0B00, 0x0000,
	0x8002, 0x0044,
	0x8004, 0x0047,
	0x8006, 0x004A,
	0x8008, 0x004D,
	0x800A, 0x0052,
	0x800C, 0x0056,
	0x800E, 0x005A,
	0x8010, 0x005F,
	0x8012, 0x0060,
	0x8014, 0x0065,
	0x8016, 0x006B,
	0x8018, 0x006E,
	0x801A, 0x0071,
	0x801C, 0x0076,
	0x801E, 0x007C,
	0x8020, 0x007C,
	0x8022, 0x007C,
	0x8024, 0x0083,
	0x8026, 0x008D,
	0x8028, 0x008F,
	0x802A, 0x0099,
	0x802C, 0x009A,
	0x802E, 0x009B,
	0x8030, 0x00A3,
	0x8032, 0x00A8,
	0x8034, 0x00AC,
	0x8036, 0x00B0,
	0x8038, 0x00B7,
	0x803A, 0x00B8,
	0x803C, 0x00AF,
	0x803E, 0x00AF,
	0x8126, 0x1A00,
	0x812A, 0x1700,
	0x812E, 0x15C0,
	0x813A, 0x1800,
	0x813E, 0x1C00,
	0x0204, 0x4408,
	0x0206, 0x03EA,
	0x020C, 0x0F18,
	0x0210, 0x0F20,
	0x0226, 0x0046,
	0x022A, 0x1B66,
	0x022C, 0x0090,
	0x022E, 0x1816,
	0x0244, 0x0103,
	0x0246, 0x0301,
	0x0248, 0x8013,
	0x024A, 0x0901,
	0x024C, 0x0103,
	0x024E, 0x0301,
	0x0250, 0x8013,
	0x0252, 0x0901,
	0x0254, 0x0103,
	0x0256, 0x0301,
	0x0258, 0x8013,
	0x025A, 0x0901,
	0x025C, 0x0088,
	0x025E, 0x0088,
	0x0262, 0x0010,
	0x0266, 0x0010,
	0x026C, 0x0223,
	0x0400, 0x3047,
	0x0402, 0x1133,
	0x040C, 0x0000,
	0x040E, 0x0001,
	0x0410, 0x01F5,
	0x041E, 0x8540,
	0x0420, 0x1400,
	0x0422, 0x00FA,
	0x0426, 0x0404,
	0x042C, 0x0000,
	0x042E, 0x0BC8,
	0x0442, 0x0508,
	0x044C, 0x0100,
	0x0452, 0x8584,
	0x0454, 0x0205,
	0x0456, 0xFFFF,
	0x045C, 0x020F,
	0x045E, 0x1FFE,
	0x046C, 0x0002,
	0x046E, 0x03EB,
	0x0470, 0x0002,
	0x0472, 0x0002,
	0x04A4, 0x0001,
	0x0D00, 0x4000,
	0x0D28, 0x0008,
	0x0D2A, 0x0FAF,
	0x0D2C, 0x0000,
	0x0D50, 0x81F7,
	0x0D52, 0x0010,
	0x0600, 0x1131,
	0x0602, 0x7112,
	0x0604, 0x8C4B,
	0x0624, 0x0004,
	0x06D0, 0x0064,
	0x06D2, 0x0230,
	0x06E4, 0x8040,
	0x06E6, 0x0040,
	0x06E8, 0x0010,
	0x06EA, 0x8040,
	0x06F6, 0xFF00,
	0x238C, 0x0000,
	0x238E, 0x0000,
	0x2390, 0x0303,
	0x2392, 0x0303,
	0x2394, 0x0303,
	0x2396, 0x0303,
	0x2398, 0x0303,
	0x239A, 0x0303,
	0x239C, 0x0303,
	0x239E, 0x0303,
	0x23A0, 0x0303,
	0x23A2, 0x0303,
	0x23A4, 0x0303,
	0x23A6, 0x0303,
	0x23A8, 0x0303,
	0x23AA, 0x0303,
	0x23AC, 0x0303,
	0x23AE, 0x0303,
	0x0F00, 0x0000,
	0x0F04, 0x0004,
	0x0F06, 0x0008,
	0x0F08, 0x0011,
	0x0F0A, 0x2233,
	0x0F0C, 0x4455,
	0x0F0E, 0x6677,
	0x0B04, 0x90E0,
	0x0B20, 0x0FA0,
	0x0B22, 0x0BB8,
	0x0B30, 0x0000,
	0x1108, 0x1001,
	0x1118, 0x0000,
	0x1702, 0x1C9C,
	0x1704, 0x0000,
	0x1706, 0x0000,
	0x1708, 0x0058,
	0x170A, 0x0038,
	0x170C, 0x8144,
	0x1718, 0x0000,
	0x1740, 0x0000,
	0x1F04, 0x0000,
	0x1F06, 0x5838,
	0x1F08, 0x03FE,
	0x1F0A, 0x0054,
	0x1D06, 0x0010,
	0x0A0A, 0xC372,
	0x0A10, 0xD840,
	0x0C00, 0x0221,
	0x0C02, 0x0FA8,
	0x0C04, 0x0BC8,
	0x0C14, 0x0004,
	0x0C16, 0x0008,
	0x0C18, 0x0FA0,
	0x0C1A, 0x0BB8,
	0x0736, 0x0087,
	0x0738, 0x0302,
	0x073C, 0x0900,
	0x0746, 0x02CA,
	0x0748, 0x000E,
	0x0766, 0x0032,
	0x1206, 0x0860,
	0x1C00, 0x0140,
	0x1600, 0xF080,
	0x1608, 0x0014,
	0x160A, 0x0F80,
	0x160C, 0x0014,
	0x160E, 0x0BA0,
	0x1614, 0x0305,
	0x1410, 0x0001,
	0x1410, 0x0001,
	0x1004, 0x2BAB,
	0x101C, 0x021A,
	0x1020, 0xC105,
	0x1022, 0x0615,
	0x1024, 0x0506,
	0x1026, 0x0708,
	0x1028, 0x1108,
	0x102A, 0x090A,
	0x102C, 0x1A0A,
	0x1038, 0x2100,
	0x1042, 0x0108,
	0x1044, 0x01F0,
	0x1048, 0x01F0,
};
#endif


static void custom1_setting(void)
{
	LOG_INF("E\n");
#if MULTI_WRITE
	hi4821q_table_write_cmos_sensor(
		addr_data_pair_custom1_hi4821q,
		sizeof(addr_data_pair_custom1_hi4821q) /
		sizeof(kal_uint16));
#else

#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_custom2_hi4821q[] = {
	0x0B00, 0x0000,
	0x8002, 0x0044,
	0x8004, 0x0047,
	0x8006, 0x004A,
	0x8008, 0x004D,
	0x800A, 0x0052,
	0x800C, 0x0056,
	0x800E, 0x005A,
	0x8010, 0x005F,
	0x8012, 0x0060,
	0x8014, 0x0065,
	0x8016, 0x006B,
	0x8018, 0x006E,
	0x801A, 0x0071,
	0x801C, 0x0076,
	0x801E, 0x007C,
	0x8020, 0x007C,
	0x8022, 0x007C,
	0x8024, 0x0083,
	0x8026, 0x008D,
	0x8028, 0x008F,
	0x802A, 0x0099,
	0x802C, 0x009A,
	0x802E, 0x009B,
	0x8030, 0x00A3,
	0x8032, 0x00A8,
	0x8034, 0x00AC,
	0x8036, 0x00B0,
	0x8038, 0x00B7,
	0x803A, 0x00B8,
	0x803C, 0x00AF,
	0x803E, 0x00AF,
	0x8126, 0x1B80,
	0x812A, 0x1A00,
	0x812E, 0x1900,
	0x813A, 0x1800,
	0x813E, 0x1C00,
	0x0204, 0x4008,
	0x0206, 0x045E,
	0x020C, 0x1920,
	0x0210, 0x1928,
	0x0226, 0x004B,
	0x022A, 0x1B6B,
	0x022C, 0x0670,
	0x022E, 0x1243,
	0x0244, 0x0101,
	0x0246, 0x0101,
	0x0248, 0x0101,
	0x024A, 0x0901,
	0x024C, 0x0101,
	0x024E, 0x0101,
	0x0250, 0x0101,
	0x0252, 0x0901,
	0x0254, 0x0101,
	0x0256, 0x0101,
	0x0258, 0x0101,
	0x025A, 0x0901,
	0x025C, 0x0044,
	0x025E, 0x0044,
	0x0262, 0x0008,
	0x0266, 0x0808,
	0x026C, 0x0000,
	0x0400, 0x3003,
	0x0402, 0x0000,
	0x040C, 0x0100,
	0x040E, 0x0001,
	0x0410, 0x03EC,
	0x041E, 0x2131,
	0x0420, 0x1400,
	0x0422, 0x045E,
	0x0426, 0x0000,
	0x042C, 0x0000,
	0x042E, 0x0BD8,
	0x0442, 0x0508,
	0x044C, 0x0000,
	0x0452, 0x8500,
	0x0454, 0x0005,
	0x0456, 0xFFFF,
	0x045C, 0x000F,
	0x045E, 0x1FFF,
	0x046C, 0x0000,
	0x046E, 0x03ED,
	0x0470, 0x0001,
	0x0472, 0x0003,
	0x04A4, 0x0001,
	0x0D00, 0x400B,
	0x0D28, 0x0008,
	0x0D2A, 0x1F67,
	0x0D2C, 0x0000,
	0x0D50, 0x83EE,
	0x0D52, 0x0000,
	0x0600, 0x1130,
	0x0602, 0x7712,
	0x0604, 0x884B,
	0x0624, 0x0000,
	0x06D0, 0x0300,
	0x06D2, 0x0100,
	0x06E4, 0x8018,
	0x06E6, 0x8018,
	0x06E8, 0x8018,
	0x06EA, 0x8018,
	0x06F6, 0x0000,
	0x238C, 0x0108,
	0x238E, 0x0108,
	0x2390, 0x0000,
	0x2392, 0x0080,
	0x2394, 0x0000,
	0x2396, 0x0000,
	0x2398, 0x8000,
	0x239A, 0x0000,
	0x239C, 0x0000,
	0x239E, 0x0000,
	0x23A0, 0x0000,
	0x23A2, 0x0080,
	0x23A4, 0x0000,
	0x23A6, 0x0000,
	0x23A8, 0x8000,
	0x23AA, 0x0000,
	0x23AC, 0x0000,
	0x23AE, 0x0000,
	0x0F00, 0x0000,
	0x0F04, 0x07E0,
	0x0F06, 0x0010,
	0x0F08, 0x0011,
	0x0F0A, 0x2233,
	0x0F0C, 0x4455,
	0x0F0E, 0x6677,
	0x0B04, 0xD560,
	0x0B20, 0x0FA0,
	0x0B22, 0x0BB8,
	0x0B30, 0x0001,
	0x1108, 0x0002,
	0x1118, 0x091C,
	0x1702, 0x1C3C,
	0x1704, 0x0001,
	0x1706, 0x0001,
	0x1708, 0x0050,
	0x170A, 0x0214,
	0x170C, 0x8141,
	0x1718, 0x0014,
	0x1740, 0x0001,
	0x1F04, 0x0006,
	0x1F06, 0x5014,
	0x1F08, 0x0200,
	0x1F0A, 0x0014,
	0x1D06, 0x0030,
	0x0A0A, 0xC372,
	0x0A10, 0x8470,
	0x0C00, 0x0223,
	0x0C02, 0x1F60,
	0x0C04, 0x1790,
	0x0C14, 0x07E0,
	0x0C16, 0x0010,
	0x0C18, 0x0FA0,
	0x0C1A, 0x0BB8,
	0x0736, 0x0087,
	0x0738, 0x0302,
	0x073C, 0x0900,
	0x0746, 0x01DC,
	0x0748, 0x0004,
	0x0766, 0x0064,
	0x1206, 0xC860,
	0x1C00, 0x0140,
	0x1600, 0xF080,
	0x1608, 0x07E0,
	0x160A, 0x0FA0,
	0x160C, 0x002C,
	0x160E, 0x0B80,
	0x1614, 0x0D11,
	0x1410, 0x0000,
	0x1004, 0x2BAB,
	0x101C, 0x021A,
	0x1020, 0xC107,
	0x1022, 0x081A,
	0x1024, 0x0506,
	0x1026, 0x0909,
	0x1028, 0x120A,
	0x102A, 0x0A0A,
	0x102C, 0x110A,
	0x1038, 0x2100,
	0x1042, 0x0108,
	0x1044, 0x00FA,
	0x1048, 0x00FA,
};
#endif

static void custom2_setting(void)
{
	LOG_INF("E\n");
#if MULTI_WRITE
	hi4821q_table_write_cmos_sensor(
		addr_data_pair_custom2_hi4821q,
		sizeof(addr_data_pair_custom2_hi4821q) /
		sizeof(kal_uint16));
#else

#endif
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_custom3_hi4821q[] = {
	0x0B00, 0x0000,
	0x8126, 0x1B80,
	0x812A, 0x1A00,
	0x812E, 0x1900,
	0x0204, 0x4008,
	0x0206, 0x045E,
	0x020C, 0x1920,
	0x0210, 0x1928,
	0x0226, 0x004B,
	0x022A, 0x1B6B,
	0x022C, 0x0090,
	0x022E, 0x1823,
	0x0244, 0x0101,
	0x0246, 0x0101,
	0x0248, 0x0101,
	0x024A, 0x0901,
	0x024C, 0x0101,
	0x024E, 0x0101,
	0x0250, 0x0101,
	0x0252, 0x0901,
	0x0254, 0x0101,
	0x0256, 0x0101,
	0x0258, 0x0101,
	0x025A, 0x0901,
	0x025C, 0x0044,
	0x025E, 0x0044,
	0x0262, 0x0008,
	0x0266, 0x0808,
	0x026C, 0x0000,
	0x0400, 0x3003,
	0x0402, 0x0000,
	0x040C, 0x0100,
	0x0410, 0x03EC,
	0x041E, 0x2131,
	0x0422, 0x045E,
	0x0426, 0x0000,
	0x042C, 0x0004,
	0x042E, 0x1790,
	0x0442, 0x0508,
	0x044C, 0x0000,
	0x0452, 0x8500,
	0x0454, 0x0005,
	0x045C, 0x000F,
	0x045E, 0x1FFF,
	0x046C, 0x0000,
	0x046E, 0x03ED,
	0x0470, 0x0001,
	0x0472, 0x0003,
	0x04A4, 0x0001,
	0x0D00, 0x400B,
	0x0D2A, 0x1F67,
	0x0D50, 0x83EE,
	0x0D52, 0x0000,
	0x0600, 0x1130,
	0x0602, 0x7712,
	0x0604, 0x804B,
	0x0624, 0x0000,
	0x06E4, 0x8018,
	0x06E6, 0x8018,
	0x06E8, 0x8018,
	0x06EA, 0x8018,
	0x06F6, 0x0000,
	0x238C, 0x0108,
	0x238E, 0x0108,
	0x2390, 0x0000,
	0x2392, 0x0080,
	0x2394, 0x0000,
	0x2396, 0x0000,
	0x2398, 0x8000,
	0x239A, 0x0000,
	0x239C, 0x0000,
	0x239E, 0x0000,
	0x23A0, 0x0000,
	0x23A2, 0x0080,
	0x23A4, 0x0000,
	0x23A6, 0x0000,
	0x23A8, 0x8000,
	0x23AA, 0x0000,
	0x23AC, 0x0000,
	0x23AE, 0x0000,
	0x0F00, 0x0000,
	0x0F04, 0x0010,
	0x0F06, 0x0010,
	0x0F08, 0x0011,
	0x0F0A, 0x2233,
	0x0F0C, 0x4455,
	0x0F0E, 0x6677,
	0x0B04, 0xD560,
	0x0B0E, 0x0101,
	0x0B20, 0x1F40,
	0x0B22, 0x1770,
	0x0B30, 0x0001,
	0x1108, 0x1002,
	0x1118, 0x0000,
	0x1702, 0x1A3A,
	0x1704, 0x0001,
	0x1706, 0x0000,
	0x1708, 0x0050,
	0x170A, 0x0038,
	0x170C, 0x8141,
	0x1718, 0x0014,
	0x1740, 0x0001,
	0x1F04, 0x0000,
	0x1F06, 0x5038,
	0x1F08, 0x0200,
	0x1F0A, 0x0014,
	0x1D06, 0x0030,
	0x0A0A, 0xC372,
	0x0A10, 0x8470,
	0x0C00, 0x0223,
	0x0C02, 0x1F60,
	0x0C04, 0x1790,
	0x0C14, 0x0010,
	0x0C16, 0x0010,
	0x0C18, 0x1F40,
	0x0C1A, 0x1770,
	0x0736, 0x0087,
	0x0738, 0x0302,
	0x073C, 0x0900,
	0x0746, 0x0125,
	0x0766, 0x0064,
	0x1206, 0xC860,
	0x1C00, 0x0140,
	0x1600, 0xF080,
	0x1608, 0x0030,
	0x160A, 0x1F00,
	0x160C, 0x0028,
	0x160E, 0x1740,
	0x1614, 0x0D11,
	0x1410, 0x0000,
	0x1004, 0x2BAB,
	0x101C, 0x021A,
	0x1020, 0xC10A,
	0x1022, 0x0A2B,
	0x1024, 0x050B,
	0x1026, 0x0F0F,
	0x1028, 0x180E,
	0x102A, 0x100A,
	0x102C, 0x1B0A,
	0x1038, 0x2100,
	0x103E, 0x0001,
	0x1042, 0x0108,
	0x1048, 0x01F0,
};
#endif


static void custom3_setting(void)
{
	LOG_INF("E\n");
#if MULTI_WRITE
	hi4821q_table_write_cmos_sensor(
		addr_data_pair_custom3_hi4821q,
		sizeof(addr_data_pair_custom3_hi4821q) /
		sizeof(kal_uint16));
#else

#endif
}

#if HI4821Q_OTP_ENABLE
//+add main camera hi4821q otp code
#include "cam_cal_define.h"
#include <linux/slab.h>
static struct stCAM_CAL_DATAINFO_STRUCT st_rear_hi4821q_eeprom_data ={
	.sensorID= HI4821Q_SENSOR_ID,
	.deviceID = 0x01,
	.dataLength = 0x1A35,
	.sensorVendorid = 0x17000100,
	.vendorByte = {1,2,3,4},
	.dataBuffer = NULL,
};

static struct stCAM_CAL_CHECKSUM_STRUCT st_rear_hi4821q_Checksum[11] =
{
	{MODULE_ITEM,0x0000,0x0000,0x0007,0x0008,0x55},
	{AWB_ITEM,0x0009,0x0009,0x0019,0x001A,0x55},
	{AF_ITEM,0x001B,0x001B,0x0020,0x0021,0x55},
	{LSC_ITEM,0x0022,0x0022,0x076E,0x076F,0x55},
	{PDAF_ITEM,0x0770,0x0770,0x0960,0x0961,0x55},
	{PDAF_PROC2_ITEM,0x0962,0x0962,0x0D4E,0x0D4F,0x55},
	{HI4821Q_PGC,0x0D50,0x0D50,0x13C4,0x13C5,0x55},
	{HI4821Q_QGC,0x13C6,0x13C6,0x17B8,0x17B9,0x55},
	{HI4821Q_XGC,0x17BA,0x17BA,0x1A32,0x1A33,0x55},
	{TOTAL_ITEM,0x0000,0x0000,0x1A33,0x1A34,0x55},
	{MAX_ITEM,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x55},  // this line must haved
};
extern int imgSensorReadEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData,
	struct stCAM_CAL_CHECKSUM_STRUCT* checkData);
extern int imgSensorSetEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData);

//-add main camera hi4821q otp code
#endif

static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	//kal_int32 size = 0;

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
    			pr_info("i2c write id : 0x%x, sensor id: 0x%x\n",
    			imgsensor.i2c_write_id, *sensor_id);

#if HI4821Q_OTP_ENABLE
			//+add main camera hi4821q otp code
			size = imgSensorReadEepromData(&st_rear_hi4821q_eeprom_data,st_rear_hi4821q_Checksum);
			if(size != st_rear_hi4821q_eeprom_data.dataLength || (st_rear_hi4821q_eeprom_data.sensorVendorid >> 24)!= hi4821q_read_eeprom(0x0001)) {
				pr_err("get eeprom data failed\n");
				if(st_rear_hi4821q_eeprom_data.dataBuffer != NULL) {
					kfree(st_rear_hi4821q_eeprom_data.dataBuffer);
					st_rear_hi4821q_eeprom_data.dataBuffer = NULL;
				}
				*sensor_id = 0xFFFFFFFF;
				return ERROR_SENSOR_CONNECT_FAIL;
			} else {
				pr_info("get eeprom data success\n");
				imgSensorSetEepromData(&st_rear_hi4821q_eeprom_data);

				#if HI4821Q_XGC_QGC_PGC_CALIB
					//get the pgc qgc xgc buffer
					pgc_data_buffer = &(st_rear_hi4821q_eeprom_data.dataBuffer[st_rear_hi4821q_Checksum[HI4821Q_PGC - 1].startAdress + 3]);
					qgc_data_buffer = &(st_rear_hi4821q_eeprom_data.dataBuffer[st_rear_hi4821q_Checksum[HI4821Q_QGC - 1].startAdress + 3]);
					xgc_data_buffer = &(st_rear_hi4821q_eeprom_data.dataBuffer[st_rear_hi4821q_Checksum[HI4821Q_XGC - 1].startAdress + 3]);

					#if HI4821Q_OTP_DUMP

						pr_info("hi4821q_eeprom:pgc_addr:0x%x\n",st_rear_hi4821q_Checksum[HI4821Q_PGC - 1].startAdress + 3);
						pr_info("hi4821q_eeprom:qgc_addr:0x%x\n",st_rear_hi4821q_Checksum[HI4821Q_QGC - 1].startAdress + 3);
						pr_info("hi4821q_eeprom:xgc_addr:0x%x\n",st_rear_hi4821q_Checksum[HI4821Q_XGC - 1].startAdress + 3);

						pr_info("=====================hi4821q dump pgc eeprom data start====================\n");
						dumpEEPROMData(PGC_DATA_SIZE,pgc_data_buffer);
						pr_info("=====================hi4821q dump pgc eeprom data end======================\n");

						pr_info("=====================hi4821q dump qgc eeprom data start====================\n");
						dumpEEPROMData(QGC_DATA_SIZE,qgc_data_buffer);
						pr_info("=====================hi4821q dump qgc eeprom data end======================\n");

						pr_info("=====================hi4821q dump xgc eeprom data start====================\n");
						dumpEEPROMData(XGC_DATA_SIZE,xgc_data_buffer);
						pr_info("=====================hi4821q dump xgc eeprom data end======================\n");
					#endif
				#endif
			}
			//-add main camera hi4821q otp code
#endif
			return ERROR_NONE;
			}

			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		pr_err("Read id fail,sensor id: 0x%x\n", *sensor_id);
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
				pr_info("i2c write id: 0x%x, sensor id: 0x%x\n",
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
		pr_err("open sensor id fail: 0x%x\n", sensor_id);
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
	set_mirror_flip(imgsensor.mirror);
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
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;

}	/* capture() */
//	#if 0 //normal video, hs video, slim video to be added

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
	video_setting();
	set_mirror_flip(imgsensor.mirror);
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
	set_mirror_flip(imgsensor.mirror);
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
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;
}    /*    slim_video     */
//#endif

static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
								MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom1_setting();
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;
}

static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT
*image_window,
               MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	imgsensor.pclk = imgsensor_info.custom2.pclk;
	imgsensor.line_length = imgsensor_info.custom2.linelength;
	imgsensor.frame_length = imgsensor_info.custom2.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	#if HI4821Q_XGC_QGC_PGC_CALIB
		apply_sensor_Cali();
	#endif
	custom2_setting();
	set_mirror_flip(imgsensor.mirror);
     return ERROR_NONE;
}

static kal_uint32 Custom3(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT
*image_window,
               MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
	imgsensor.pclk = imgsensor_info.custom3.pclk;
	imgsensor.line_length = imgsensor_info.custom3.linelength;
	imgsensor.frame_length = imgsensor_info.custom3.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	#if HI4821Q_XGC_QGC_PGC_CALIB
		apply_sensor_Cali();
	#endif
	custom3_setting();
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;
}   /*  Custom1	*/

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

//#if 0
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

	sensor_resolution->SensorCustom1Width  =
		imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height  =
		imgsensor_info.custom1.grabwindow_height;
//#endif
	sensor_resolution->SensorCustom2Width =
		imgsensor_info.custom2.grabwindow_width;
	sensor_resolution->SensorCustom2Height =
		imgsensor_info.custom2.grabwindow_height;

	sensor_resolution->SensorCustom3Width =
		imgsensor_info.custom3.grabwindow_width;
	sensor_resolution->SensorCustom3Height =
		imgsensor_info.custom3.grabwindow_height;
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

	sensor_info->Custom1DelayFrame =
		imgsensor_info.custom1_delay_frame;
	sensor_info->Custom2DelayFrame =
		imgsensor_info.custom2_delay_frame;
	sensor_info->Custom3DelayFrame =
		imgsensor_info.custom3_delay_frame;
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
	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom2.mipi_data_lp2hs_settle_dc;

		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		sensor_info->SensorGrabStartX = imgsensor_info.custom3.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom3.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom3.mipi_data_lp2hs_settle_dc;
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
	case MSDK_SCENARIO_ID_CUSTOM1:
		Custom1(image_window, sensor_config_data); // Custom1
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		Custom2(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		Custom3(image_window, sensor_config_data);
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
	case MSDK_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		frame_length = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.custom2.framelength) ? (frame_length - imgsensor_info.custom2.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		frame_length = imgsensor_info.custom3.pclk / framerate * 10 / imgsensor_info.custom3.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.custom3.framelength) ? (frame_length - imgsensor_info.custom3.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.custom3.framelength + imgsensor.dummy_line;
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
	case MSDK_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		*framerate = imgsensor_info.custom2.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		*framerate = imgsensor_info.custom3.max_framerate;
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
		write_cmos_sensor(0x1038, 0x0000); //mipi_virtual_channel_ctrl
		write_cmos_sensor(0x1042, 0x0008); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
		write_cmos_sensor(0x0b04, 0x0141);
		write_cmos_sensor(0x0C0A, 0x0200);

	} else {
		write_cmos_sensor(0x1038, 0x4100); //mipi_virtual_channel_ctrl
		write_cmos_sensor(0x1042, 0x0108); //mipi_pd_sep_ctrl_h, mipi_pd_sep_ctrl_l
		write_cmos_sensor(0x0b04, 0x0349);
		write_cmos_sensor(0x0C0A, 0x0000);

	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	pr_debug("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable)
		write_cmos_sensor(0x0b00, 0x0100); // stream on
	else
		write_cmos_sensor(0x0b00, 0x0000); // stream off

	mdelay(10);
	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
			UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
#ifdef ENABLE_PDAF
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
#endif
	if (!((feature_id == 3040) || (feature_id == 3058)))
		LOG_INF("feature_id = %d\n", feature_id);

	switch (feature_id) {
	//+for factory mode of photo black screen
		case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		*(feature_data + 2) = imgsensor_info.max_gain;
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_shutter;
		*(feature_data + 2) = imgsensor_info.exp_step;
		break;
	//-for factory mode of photo black screen
	//+ for mipi rate is 0
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
		//+for main camera hw remosaic
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom2.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom3.pclk;
			break;
		//-for main camera hw remosaic
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
		//+for main camera hw remosaic
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom1.framelength << 16)
				+ imgsensor_info.custom1.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom2.framelength << 16)
				+ imgsensor_info.custom2.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom3.framelength << 16)
				+ imgsensor_info.custom3.linelength;
			break;
		//-for main camera hw remosaic
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
	//-for mipi rate is 0
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length(
			(UINT16) *feature_data, (UINT16) *(feature_data + 1));
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
	    spin_lock(&imgsensor_drv_lock);
	    imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		LOG_INF("current fps :%d\n", imgsensor.current_fps);
	break;
	case SENSOR_FEATURE_GET_CROP_INFO:
	    LOG_INF("GET_CROP_INFO scenarioId:%d\n",
			*feature_data_32);

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
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[5],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[6],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[7],
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
	#if 0
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
	    LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
	    ihdr_write_shutter_gain((UINT16)*feature_data,
			(UINT16)*(feature_data+1),
				(UINT16)*(feature_data+2));
	break;
	#endif
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
			case MSDK_SCENARIO_ID_CUSTOM1:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom1.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CUSTOM2:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom2.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CUSTOM3:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom3.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.pre.mipi_pixel_rate;
				break;
			}
	break;
#ifdef ENABLE_PDAF
/******************** PDAF START ********************/
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
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
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		pr_debug("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16) *feature_data);
		pvcinfo =
	    (struct SENSOR_VC_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			pr_debug("Jesse+ CAPTURE_JPEG \n");
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pr_debug("Jesse+ VIDEO_PREVIEW \n");
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			pr_debug("Jesse+ CAMERA_PREVIEW \n");
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		break;
	case SENSOR_FEATURE_SET_PDAF:
			imgsensor.pdaf_mode = *feature_data_16;
		break;
/******************** PDAF END ********************/
	//+for factory mode of photo black screen
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
		/*
		* 1, if driver support new sw frame sync
		* set_shutter_frame_length() support third para auto_extend_en
		*/
		*(feature_data + 1) = 1;
		/* margin info by scenario */
		*(feature_data + 2) = imgsensor_info.margin;
		break;
	//-for factory mode of photo black screen
#endif
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		streaming_control(KAL_FALSE);
		break;

	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	//+for factory mode of photo black screen
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case MSDK_SCENARIO_ID_CUSTOM3:
			*feature_return_para_32 = 1; /*BINNING_NONE*/
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			//+for main camera hw remosaic
			*feature_return_para_32 = 2; /*BINNING_AVERAGED*/
			//-for main camera hw remosaic
			break;
		}
		pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
			*feature_para_len = 4;
		break;
	//-for factory mode of photo black screen
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

UINT32 HI4821Q_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc =  &sensor_func;
	return ERROR_NONE;
}	/*	hi4821q_MIPI_RAW_SensorInit	*/
