/*
* Copyright (C) 2016 MediaTek Inc.
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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *      IMX362mipi_Sensor.c
 *
 * Project:
 * --------
 *      ALPS
 *
 * Description:
 * ------------
 *      Source code of Sensor driver
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
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx362mipi_Sensor.h"

/****************************Modify Following Strings for Debug****************************/
#define PFX "IMX362_camera_sensor"
#define LOG_1 LOG_INF("IMX362,MIPI 1LANE\n")
#define LOG_2 LOG_INF("preview 1600*1200@30fps,600Mbps/lane; video 1600*1200@30fps,600Mbps/lane; 1600*1200@30fps,600Mbps/lane\n")
/****************************   Modify end    *******************************************/
#define LOG_INF(format, args...) pr_err(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOGE(format, args...)   pr_err(PFX "[%s] " format, __FUNCTION__, ##args)


static DEFINE_SPINLOCK(imgsensor_drv_lock);
typedef struct tag_i2c_write_array{
	kal_uint32 addr;
	kal_uint32 para;
}i2c_write_array;

static i2c_write_array init_setting_array[]={
	{0x0136, 0x18},
	{0x0137, 0x00},
	{0x31A3, 0x00},
	{0x5812, 0x04},
	{0x5813, 0x04},
	{0x58D0, 0x08},
	{0x5F20, 0x01},
	{0x5FF0, 0x00},
	{0x5FF1, 0xFE},
	{0x5FF2, 0x00},
	{0x5FF3, 0x52},
	{0x72E8, 0x96},
	{0x72E9, 0x59},
	{0x72EA, 0x65},
	{0x72FB, 0x2C},
	{0x737E, 0x02},
	{0x737F, 0x30},
	{0x7380, 0x28},
	{0x7381, 0x00},
	{0x7383, 0x02},
	{0x7384, 0x00},
	{0x7385, 0x00},
	{0x74CC, 0x00},
	{0x74CD, 0x55},
	{0x74D2, 0x00},
	{0x74D3, 0x52},
	{0x74DA, 0x00},
	{0x74DB, 0xFE},
	{0x9333, 0x03},
	{0x9334, 0x04},
	{0x9335, 0x05},
	{0x9346, 0x96},
	{0x934A, 0x8C},
	{0x9352, 0xAA},
	{0xB0B6, 0x05},
	{0xB0B7, 0x05},
	{0xB0B9, 0x05},
	{0xBC88, 0x06},
	{0xBC89, 0xD8},
};
//write_cmos_sensor(0x0340,0x0C);//FRM_LENGTH_LINES[15:8]
//write_cmos_sensor(0x0341,0xE0);//FRM_LENGTH_LINES[7:0]0x0CE0=3296
//write_cmos_sensor(0x0342,0x21);//LINE_LENGTH_PCK[15:8]
//write_cmos_sensor(0x0343,0x28);//LINE_LENGTH_PCK[7:0]0x2128=8488
static i2c_write_array preview_setting_array[]={
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0114, 0x03},
	{0x0220, 0x00},
	{0x0221, 0x11},
	{0x0340, 0x09},
	{0x0341, 0x98},
	{0x0342, 0x2C},
	{0x0343, 0x88},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x00},
	{0x0901, 0x11},
	{0x30F4, 0x02},
	{0x30F5, 0x6C},
	{0x30F6, 0x00},
	{0x30F7, 0x14},
	{0x31A0, 0x01},
	{0x31A5, 0x01},
	{0x31A6, 0x00},
	{0x560F, 0x46},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x01},
	{0x0347, 0x78},
	{0x0348, 0x0F},
	{0x0349, 0xBF},
	{0x034A, 0x0A},
	{0x034B, 0x57},
	{0x034C, 0x1F},
	{0x034D, 0x80},
	{0x034E, 0x08},
	{0x034F, 0xE0},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x0F},
	{0x040D, 0xC0},
	{0x040E, 0x08},
	{0x040F, 0xE0},
	{0x0301, 0x03},
	{0x0303, 0x02},
	{0x0305, 0x04},
	{0x0306, 0x00},
	{0x0307, 0xD2},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x04},
	{0x030E, 0x01},
	{0x030F, 0x0A},
	{0x0310, 0x01},

};

static i2c_write_array capture_setting_array[]={
	{0x0112,0x0A},
	{0x0113,0x0A},
	{0x0114,0x03},
	{0x0220,0x00},
	{0x0221,0x11},
	{0x0340,0x0C},
	{0x0341,0x30},
	{0x0342,0x22},
	{0x0343,0xF8},
	{0x0381,0x01},
	{0x0383,0x01},
	{0x0385,0x01},
	{0x0387,0x01},
	{0x0900,0x00},
	{0x0901,0x11},
	{0x30F4,0x01},
	{0x30F5,0xA4},
	{0x30F6,0x01},
	{0x30F7,0x90},
	{0x31A0,0x01},
	{0x31A5,0x01},
	{0x31A6,0x00},
	{0x560F,0x46},
	{0x0344,0x00},
	{0x0345,0x00},
	{0x0346,0x00},
	{0x0347,0x00},
	{0x0348,0x0F},
	{0x0349,0xBF},
	{0x034A,0x0B},
	{0x034B,0xCF},
	{0x034C,0x1F},
	{0x034D,0x80},
	{0x034E,0x0B},
	{0x034F,0xD0},
	{0x0408,0x00},
	{0x0409,0x00},
	{0x040A,0x00},
	{0x040B,0x00},
	{0x040C,0x0F},
	{0x040D,0xC0},
	{0x040E,0x0B},
	{0x040F,0xD0},
	{0x0301,0x03},
	{0x0303,0x02},
	{0x0305,0x04},
	{0x0306,0x00},
	{0x0307,0xD2},
	{0x0309,0x0A},
	{0x030B,0x01},
	{0x030D,0x04},
	{0x030E,0x01},
	{0x030F,0x4F},
	{0x0310,0x01},
};

static imgsensor_info_struct imgsensor_info = {
	.sensor_id = IMX362_SENSOR_ID, //IMX362MIPI_SENSOR_ID,  /*sensor_id = 0x326*/ //record sensor id defined in Kd_imgsensor.h
	.checksum_value = 0xfa71879b, //checksum value for Camera Auto Test
	.pre = {
		.pclk = 840000000,                           //record different mode's pclk
		.linelength = 11400,                            //record different mode's linelength
		.framelength = 2456,                       //record different mode's framelength
		.startx = 0,                                          //record different mode's startx of grabwindow
		.starty = 0,                                          //record different mode's starty of grabwindow
		.grabwindow_width = 4032,          //record different mode's width of grabwindow
		.grabwindow_height =2272,          //record different mode's height of grabwindow
		/*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario         */
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*     following for GetDefaultFramerateByScenario()         */
		.max_framerate = 300,
	},
	.cap = {/*normal capture*/
		.pclk = 840000000,//OPPXCLK
		.linelength = 8952,
		.framelength = 3120,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 3024,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,//300,
	},
	.cap1 = {/*PIP capture*/ //capture for PIP 24fps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 840000000,
		.linelength = 11400,
		.framelength = 2456,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 2272,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 240, //less than 13M(include 13M),cap1 max framerate is 24fps,16M max framerate is 20fps, 20M max framerate is 15fps
	},
	.normal_video = {
		.pclk = 840000000,
		.linelength = 11400,
		.framelength = 2456,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 2272,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 1200,//modify
	},
	.hs_video = {/*slow motion*/
		.pclk = 840000000,
		.linelength = 11400,
		.framelength = 2456,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 2272,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 1200,//modify
	},
	.slim_video = {/*VT Call*/
		.pclk = 840000000,
		.linelength = 11400,
		.framelength = 2456,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4032,
		.grabwindow_height = 2272,
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 300,
	},
	.margin = 10,                    //sensor framelength & shutter margin
	.min_shutter = 1, //1,              //min shutter
	.max_frame_length = 0x7fff,//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,    //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.ihdr_support = 0,     //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,           //support sensor mode num
	.cap_delay_frame = 2,            //enter capture delay frame num
	.pre_delay_frame = 2,            //enter preview delay frame num
	.video_delay_frame = 2,        //enter video delay frame num
	.hs_video_delay_frame = 2,  //enter high speed video  delay frame num
	.slim_video_delay_frame = 2,//enter slim video delay frame num
	.isp_driving_current = ISP_DRIVING_4MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_R, //SENSOR_OUTPUT_FORMAT_RAW_Gr,//SENSOR_OUTPUT_FORMAT_RAW_R,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
	.i2c_addr_table = {0x21,0x34,0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_HV_MIRROR,                                //mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x14d,                                       //current shutter
	.gain = 0xe000,                                                   //current gain
	.dummy_pixel = 0,                                     //current dummypixel
	.dummy_line = 0,                                       //current dummyline
	.current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,            //test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.hdr_mode = 0, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x20,//record current sensor's i2c write id
};

#define IMX362_HDR_TYPE (0x00)
#define IMX362_BINNING_TYPE (0x10)
static kal_uint16 imx362_type = 0;/*0x00=HDR type, 0x10=binning type*/
/* Sensor output window information */
/*according toIMX362 datasheet p53 image cropping*/
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] ={
	{ 4032, 2272,     0,   0, 4032, 2272, 4032,  2272, 0000, 0000, 4032,  2272,   0,   0, 4032, 2272}, // Preview
	{ 4032, 3024,     0,   0, 4032, 3024, 4032,  3024, 0000, 0000, 4032,  3024,   0,   0, 4032, 3024}, // capture
	{ 4032, 2272,     0,   0, 4032, 2272, 4032,  2272, 0000, 0000, 4032,  2272,   0,   0, 4032, 2272}, // video
	//{ 4032, 3024,    0,   0, 4208, 3120, 4208,  3120, 0000, 0000, 4208,  3120,   0,   0, 4208, 3120}, // video
	{ 4032, 2272,     0,   0, 4032, 2272, 4032,  2272, 0000, 0000, 4032,  2272,   0,   0, 4032, 2272}, //hight speed video
	//{ 4208, 2688,         0,  432, 4208, 2256, 1400,  752 , 0000, 0000, 1400,  752,   0,   0, 1400,  752}, //hight speed video
	{ 4032, 2272,     0,   0, 4032, 2272, 4032,  2272, 0000, 0000, 4032,  2272,   0,   0, 4032, 2272}};// slim video
//{ 4208, 2688,         0,  432, 4208, 2256, 1400,  752 , 0000, 0000, 1400,  752,   0,   0, 1400,  752}};// slim video
#if 0
/*VC1 None , VC2 for PDAF(DT=0X36), unit : 8bit*/
static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
{
	/* Preview mode setting */
	{0x03, 0x0a,    0x00,       0x08, 0x40, 0x00,
		0x00, 0x2b, 0x0834, 0x0618, 0x00, 0x35, 0x0280, 0x0001,
		0x00, 0x2f, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x03, 0x0a,   0x00,   0x08, 0x40, 0x00,
		0x00, 0x2b, 0x1070, 0x0C30, 0x00, 0x35, 0x0280, 0x0001,
		0x00, 0x2f, 0x00A0, 0x0780, 0x03, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0a,   0x00,   0x08, 0x40, 0x00,
		0x00, 0x2b, 0x1070, 0x0C30, 0x01, 0x00, 0x0000, 0x0000,
		0x02, 0x2f, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000}
};

static SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX = 24,
	.i4OffsetY = 24,
	.i4PitchX = 32,
	.i4PitchY = 32,
	.i4PairNum =4,
	.i4SubBlkW =16,
	.i4SubBlkH =16,
	.i4PosL = {{26,29},{42,29},{33,48},{49,48}},
	.i4PosR = {{25,32},{41,32},{34,45},{50,45}},
	.iMirrorFlip = 3, /* 0:IMAGE_NORMAL,1:IMAGE_H_MIRROR,2:IMAGE_V_MIRROR,3:IMAGE_HV_MIRROR*/
};

/* Binning Type VC information*/
static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO_Binning[3]=
{
	/* Preview mode setting */
	{0x03, 0x0a,    0x00,       0x08, 0x40, 0x00,
		0x00, 0x2b, 0x0834, 0x0618, 0x00, 0x35, 0x0280, 0x0001,
		0x00, 0x2f, 0x00A0, 0x0602, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x03, 0x0a,   0x00,   0x08, 0x40, 0x00,
		0x00, 0x2b, 0x1070, 0x0C30, 0x00, 0x35, 0x0280, 0x0001,
		0x00, 0x2f, 0x00A0, 0x0C02, 0x03, 0x00, 0x0000, 0x0000},//0x0780
	/* Video mode setting */
	{0x02, 0x0a,   0x00,   0x08, 0x40, 0x00,
		0x00, 0x2b, 0x1070, 0x0C30, 0x01, 0x00, 0x0000, 0x0000,
		0x02, 0x2f, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000}
};
/*HDR mode PD position information*/
static SET_PD_BLOCK_INFO_T imgsensor_pd_info_Binning =
{
	.i4OffsetX = 24,
	.i4OffsetY = 24,
	.i4PitchX = 32,
	.i4PitchY = 32,
	.i4PairNum = 4,//8,//4,
	.i4SubBlkW =16,
	.i4SubBlkH =16,
	.i4PosL = {{30,31},{46,31},{37,52},{53,52}},//{{30,29},{46,29},{30,31},{46,31},{37,52},{53,52},{37,54},{53,54}},//{{27,30},{43,30},{34,49},{50,49}},
	.i4PosR = {{29,36},{45,36},{38,47},{54,47}},//{{29,36},{45,36},{29,38},{45,38},{38,45},{54,45},{38,47},{54,47}},//{{26,33},{42,33},{35,46},{51,46}},
};
#endif
//add for imx362 pdaf
static SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX = 0,
	.i4OffsetY = 0,
	.i4PitchX  = 0,
	.i4PitchY  = 0,
	.i4PairNum	=0,
	.i4SubBlkW	=0,
	.i4SubBlkH	=0,
	.i4PosL = {{0,0}},
	.i4PosR = {{0,0}},
	.iMirrorFlip = 0,
	.i4BlockNumX = 0,
	.i4BlockNumY = 0,
	.i4LeFirst = 0,
	.i4Crop = {{0,0},{0,0},{0,0},{0,0},{0,0}, \
		{0,0},{0,0},{0,0},{0,0},{0,0}},
};


#define IMX362MIPI_MaxGainIndex (154)
kal_uint16 IMX362MIPI_sensorGainMapping[IMX362MIPI_MaxGainIndex][2] ={
	{64 ,0       },
	{65 ,8       },
	{66 ,16 },
	{67 ,25 },
	{68 ,30 },
	{69 ,37 },
	{70 ,45 },
	{71 ,51 },
	{72 ,57 },
	{73 ,63 },
	{74 ,67 },
	{75 ,75 },
	{76 ,81 },
	{77 ,85 },
	{78 ,92 },
	{79 ,96 },
	{80 ,103},
	{81 ,107},
	{82 ,112},
	{83 ,118},
	{84 ,122},
	{86 ,133},
	{88 ,140},
	{89 ,144},
	{90 ,148},
	{93 ,159},
	{96 ,171},
	{97 ,175},
	{99 ,182},
	{101,188},
	{102,192},
	{104,197},
	{106,202},
	{107,206},
	{109,211},
	{112,220},
	{113,222},
	{115,228},
	{118,235},
	{120,239},
	{125,250},
	{126,252},
	{128,256},
	{129,258},
	{130,260},
	{132,264},
	{133,266},
	{135,269},
	{136,271},
	{138,274},
	{139,276},
	{141,279},
	{142,282},
	{144,285},
	{145,286},
	{147,290},
	{149,292},
	{150,294},
	{155,300},
	{157,303},
	{158,305},
	{161,309},
	{163,311},
	{170,319},
	{172,322},
	{174,324},
	{176,326},
	{179,329},
	{181,331},
	{185,335},
	{189,339},
	{193,342},
	{195,344},
	{196,345},
	{200,348},
	{202,350},
	{205,352},
	{207,354},
	{210,356},
	{211,357},
	{214,359},
	{217,361},
	{218,362},
	{221,364},
	{224,366},
	{231,370},
	{237,374},
	{246,379},
	{250,381},
	{252,382},
	{256,384},
	{260,386},
	{262,387},
	{273,392},
	{275,393},
	{280,395},
	{290,399},
	{306,405},
	{312,407},
	{321,410},
	{331,413},
	{345,417},
	{352,419},
	{360,421},
	{364,422},
	{372,424},
	{386,427},
	{400,430},
	{410,432},
	{420,434},
	{431,436},
	{437,437},
	{449,439},
	{455,440},
	{461,441},
	{468,442},
	{475,443},
	{482,444},
	{489,445},
	{496,446},
	{504,447},
	{512,448},
	{520,449},
	{529,450},
	{537,451},
	{546,452},
	{555,453},
	{565,454},
	{575,455},
	{585,456},
	{596,457},
	{607,458},
	{618,459},
	{630,460},
	{642,461},
	{655,462},
	{669,463},
	{683,464},
	{697,465},
	{713,466},
	{728,467},
	{745,468},
	{762,469},
	{780,470},
	{799,471},
	{819,472},
	{840,473},
	{862,474},
	{886,475},
	{910,476},
	{936,477},
	{964,478},
	{993,479},
	{1024,480}
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static int write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	int ret = 0;
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	ret = iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);

	return ret;
}


#if 0
extern bool read_imx362_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size);
extern bool read_imx362_eeprom_SPC( kal_uint16 addr, BYTE* data, kal_uint32 size);
static kal_uint8 IMX362MIPI_SPC_Data[126];
static kal_uint8 SPC_data_done = false;
static void load_imx362_SPC_Data(void)
{
	kal_uint16 i;
	if ( SPC_data_done == false ) {
		if (!read_imx362_eeprom_SPC(0x0F73,IMX362MIPI_SPC_Data,126)) {
			LOG_INF("imx362 load spc fail\n");
			return;
		}
		SPC_data_done = true;
	}

	for(i=0; i<63; i++)
	{
		write_cmos_sensor(0xD04C+i, IMX362MIPI_SPC_Data[i]);
		//LOG_INF("SPC_Data[%d] = %d\n", i, IMX362MIPI_SPC_Data[i]);
	}
	for(i=0; i<63; i++)
	{
		write_cmos_sensor(0xD08C+i, IMX362MIPI_SPC_Data[i+63]);
		//LOG_INF("SPC_Data[%d] = %d\n", i+63, IMX362MIPI_SPC_Data[i+63]);
	}
}
#endif
static void set_dummy(void)
{
	//LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor(0x0104, 0x01);

	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
	write_cmos_sensor(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor(0x0343, imgsensor.line_length & 0xFF);

	write_cmos_sensor(0x0104, 0x00);
}

static kal_uint32 return_sensor_id(void)
{
	int tmp = 0;
	int retry = 10;
	tmp=read_cmos_sensor(0x0005);
	LOG_INF("liuzhixiong return sensor id read =%d \n",tmp);
	if(write_cmos_sensor(0x0A02, 0x7F)==0){
		write_cmos_sensor(0x0A00, 0x01);
		while(retry--)
		{
			if(read_cmos_sensor(0x0A01) == 0x01)
			{
				imx362_type = read_cmos_sensor(0x0A2E);
				LOG_INF("imx362 type = 0x%x(0x00=HDR,0x10=binning)", imx362_type);
				return (kal_uint16)((read_cmos_sensor(0x0A24) << 4) | (read_cmos_sensor(0x0A25) >> 4));
			}
		}
	}

	return 0x00;

}
static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	LOG_INF("framerate = %d, min framelength should enable %d \n", framerate,min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	//dummy_line = frame_length - imgsensor.min_frame_length;
	//if (dummy_line < 0)
	//     imgsensor.dummy_line = 0;
	//else
	//     imgsensor.dummy_line = dummy_line;
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
}       /*     set_max_framerate  */


static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	//write_shutter(shutter);
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
	if(!shutter) shutter = 1; /*avoid 0*/
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);

	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (0) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
		else {
			// Extend frame length
			write_cmos_sensor(0x0104, 0x01);
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
			write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor(0x0104, 0x00);
		}
	} else {
		// Extend frame length
		write_cmos_sensor(0x0104, 0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x0104, 0x00);
	}

	// Update Shutter
	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor(0x0203, shutter  & 0xFF);
	write_cmos_sensor(0x0104, 0x00);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}    /*    set_shutter */


static void set_shutter_frame_time(kal_uint16 shutter, kal_uint16 frame_time)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	//LOG_INF("shutter =%d, frame_time =%d\n", shutter, frame_time);

	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
	spin_lock(&imgsensor_drv_lock);
	/*Change frame time*/
	imgsensor.dummy_line = (frame_time > imgsensor.frame_length) ? (frame_time - imgsensor.frame_length) : 0;
	imgsensor.frame_length = imgsensor.frame_length + imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	//
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (0) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
		else {
			// Extend frame length
			write_cmos_sensor(0x0104, 0x01);
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
			write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor(0x0104, 0x00);
		}
	} else {
		// Extend frame length
		write_cmos_sensor(0x0104, 0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x0104, 0x00);
	}

	// Update Shutter
	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
	write_cmos_sensor(0x0203, shutter  & 0xFF);
	write_cmos_sensor(0x0104, 0x00);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}    /*    set_shutter */


static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint8 i;

	for (i = 0; i < IMX362MIPI_MaxGainIndex; i++) {
		if(gain <= IMX362MIPI_sensorGainMapping[i][0]){
			break;
		}
	}
	if(gain != IMX362MIPI_sensorGainMapping[i][0])
		LOG_INF("Gain mapping don't correctly:%d %d \n", gain, IMX362MIPI_sensorGainMapping[i][0]);
	return IMX362MIPI_sensorGainMapping[i][1];
}

/*************************************************************************
 * FUNCTION
 *       set_gain
 *
 * DESCRIPTION
 *       This function is to set global gain to sensor.
 *
 * PARAMETERS
 *       iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *       the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	/* 0x350A[0:1], 0x350B[0:7] AGC real gain */
	/* [0:3] = N meams N /16 X     */
	/* [4:9] = M meams M X                    */
	/* Total gain = M + N /16 X   */

	//
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

	write_cmos_sensor(0x0104, 0x01);
	write_cmos_sensor(0x0204, (reg_gain>>8)& 0xFF);
	write_cmos_sensor(0x0205, reg_gain & 0xFF);
	write_cmos_sensor(0x0104, 0x00);

	return gain;
}       /*     set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{

	kal_uint16 realtime_fps = 0;
	kal_uint16 reg_gain;
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n",le,se,gain);
	spin_lock(&imgsensor_drv_lock);
	if (le > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = le + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	if (le < imgsensor_info.min_shutter) le = imgsensor_info.min_shutter;
	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
		else {
			write_cmos_sensor(0x0104, 0x01);
			write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
			write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
			write_cmos_sensor(0x0104, 0x00);
		}
	} else {
		write_cmos_sensor(0x0104, 0x01);
		write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x0104, 0x00);
	}
	write_cmos_sensor(0x0104, 0x01);
	/* Long exposure */
	write_cmos_sensor(0x0202, (le >> 8) & 0xFF);
	write_cmos_sensor(0x0203, le  & 0xFF);
	/* Short exposure */
	write_cmos_sensor(0x0224, (se >> 8) & 0xFF);
	write_cmos_sensor(0x0225, se  & 0xFF);
	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	/* Global analog Gain for Long expo*/
	write_cmos_sensor(0x0204, (reg_gain>>8)& 0xFF);
	write_cmos_sensor(0x0205, reg_gain & 0xFF);
	/* Global analog Gain for Short expo*/
	//write_cmos_sensor(0x0216, (reg_gain>>8)& 0xFF);
	//write_cmos_sensor(0x0217, reg_gain & 0xFF);
	write_cmos_sensor(0x0104, 0x00);
}

static void set_mirror_flip(kal_uint8 image_mirror)
{
	kal_uint8  iTemp;
	LOG_INF("image_mirror = %d\n", image_mirror);
	iTemp = read_cmos_sensor(0x0101);
	iTemp&= ~0x03; //Clear the mirror and flip bits.

	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor(0x0101, iTemp|0x00);
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor(0x0101, iTemp|0x01);
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor(0x0101, iTemp|0x02);
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor(0x0101, iTemp|0x03);
		break;
	default:
		LOG_INF("Error image_mirror setting\n");
	}

}

/*************************************************************************
 * FUNCTION
 *       night_mode
 *
 * DESCRIPTION
 *       This function night mode of sensor.
 *
 * PARAMETERS
 *       bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *       None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void night_mode(kal_bool enable)
{
	/*No Need to implement this function*/
}       /*     night_mode    */

/*     preview_setting  */
static void imx362_ImageQuality_Setting(void)
{
	int i=0;
	for(i = 0 ; i < sizeof(preview_setting_array)/sizeof(i2c_write_array);i++){
		write_cmos_sensor(preview_setting_array[i].addr,preview_setting_array[i].para);
	}
#if 0
	/*Mode2 HD(2X1bin)(HDR on)H:4032 V:1136*/
	//Mode Setting,as same as preview setting
	write_cmos_sensor(0x0112,0x0A);//CSI_DT_FMT_H
	write_cmos_sensor(0x0113,0x0A);//CSI_DT_FMT_L
	write_cmos_sensor(0x0114,0x03);//CSI_LANE_MODE:4LANE
	write_cmos_sensor(0x0220,0x61);//HDR_MODE:HDR Enable,separate gain used
	write_cmos_sensor(0x0221,0x12);//HDR_RESO_REDU_H,HDR_RESO_REDU_V:HDR V2 binning
	write_cmos_sensor(0x0340,0x05);//FRM_LENGTH_LINES[15:8]
	write_cmos_sensor(0x0341,0x20);//FRM_LENGTH_LINES[7:0]0x0520=1312
	write_cmos_sensor(0x0342,0x14);//LINE_LENGTH_PCK[15:8]
	write_cmos_sensor(0x0343,0xB8);//LINE_LENGTH_PCK[7:0]0x14B8=5304
	write_cmos_sensor(0x0381,0x01);//X_EVN_INC
	write_cmos_sensor(0x0383,0x01);//X_ODD_INC
	write_cmos_sensor(0x0385,0x01);//Y_EVN_INC
	write_cmos_sensor(0x0387,0x01);//Y_ODD_INC
	write_cmos_sensor(0x0900,0x00);//BINNING_MODE
	write_cmos_sensor(0x0901,0x11);//BINNING_TYPE_H,BINNING_TYPE_V
	write_cmos_sensor(0x30F4,0x00);
	write_cmos_sensor(0x30F5,0x14);
	write_cmos_sensor(0x30F6,0x00);
	write_cmos_sensor(0x30F7,0x14);
	write_cmos_sensor(0x31A0,0x02);
	write_cmos_sensor(0x31A5,0x00);
	write_cmos_sensor(0x31A6,0x00);
	write_cmos_sensor(0x560F,0x14);
	//Output Size Setting
	write_cmos_sensor(0x0344,0x00);//X_ADD_STA[11:8]
	write_cmos_sensor(0x0345,0x00);//X_ADD_STA[7:0]
	write_cmos_sensor(0x0346,0x01);//Y_ADD_STA[11:8]
	write_cmos_sensor(0x0347,0x78);//Y_ADD_STA[7:0]
	write_cmos_sensor(0x0348,0x0F);//X_ADD_END[11:8]
	write_cmos_sensor(0x0349,0xBF);//X_ADD_END[7:0]
	write_cmos_sensor(0x034A,0x0A);//Y_ADD_END[11:8]
	write_cmos_sensor(0x034B,0x57);//Y_ADD_END[7:0]
	write_cmos_sensor(0x034C,0x0F);//X_OUT_SIZE[12:8]
	write_cmos_sensor(0x034D,0xC0);//X_OUT_SIZE[7:0]
	write_cmos_sensor(0x034E,0x04);//Y_OUT_SIZE[11:8]
	write_cmos_sensor(0x034F,0x70);//Y_OUT_SIZE[7:0]
	write_cmos_sensor(0x0408,0x00);//DIG_CROP_X_OFFSET[11:8]
	write_cmos_sensor(0x0409,0x00);//DIG_CROP_X_OFFSET[7:0]
	write_cmos_sensor(0x040A,0x00);//DIG_CROP_Y_OFFSET[11:8]
	write_cmos_sensor(0x040B,0x00);//DIG_CROP_Y_OFFSET[7:0]
	write_cmos_sensor(0x040C,0x0F);//DIG_CROP_IMAGE_WIDTH[11:8]
	write_cmos_sensor(0x040D,0xC0);//DIG_CROP_IMAGE_WIDTH[7:0]
	write_cmos_sensor(0x040E,0x04);//DIG_CROP_IMAGE_HEIGHT[11:8]
	write_cmos_sensor(0x040F,0x70);//DIG_CROP_IMAGE_HEIGHT[7:0]
	//Clock Setting
	write_cmos_sensor(0x0301,0x03);//IVTPXCK_DIV
	write_cmos_sensor(0x0303,0x02);//IVTSYCK_DIV
	write_cmos_sensor(0x0305,0x04);//PREPLLCK_IVT_DIV
	write_cmos_sensor(0x0306,0x00);//PLL_IVT_MPY[10:8]
	write_cmos_sensor(0x0307,0xD2);//PLL_IVT_MPY[7:0]
	write_cmos_sensor(0x0309,0x0A);//IOPPXCK_DIV
	write_cmos_sensor(0x030B,0x01);//IOPSYCK_DIV
	write_cmos_sensor(0x030D,0x04);//PREPLLCK_IOP_DIV
	write_cmos_sensor(0x030E,0x01);//PLL_IOP_MPY[10:8]
	write_cmos_sensor(0x030F,0x5E);//PLL_IOP_MPY[7:0]
	write_cmos_sensor(0x0310,0x01);//PLL_MULT_DRIV
#endif
}


static void sensor_init(void)
{
	int i=0;
	LOG_INF("E\n");
	//init setting
	//IMX362
	for(i = 0; i< sizeof(init_setting_array)/sizeof(i2c_write_array);i++){
		write_cmos_sensor(init_setting_array[i].addr,init_setting_array[i].para);
	}

#if 0
	write_cmos_sensor(0x0136,0x18);
	write_cmos_sensor(0x0137,0x00);

	write_cmos_sensor(0x31A3,0x00);
	write_cmos_sensor(0x5812,0x04);
	write_cmos_sensor(0x5813,0x04);
	write_cmos_sensor(0x5F20,0x01);
	write_cmos_sensor(0x72E8,0x96);
	write_cmos_sensor(0x72E9,0x59);
	write_cmos_sensor(0x72EA,0x65);
	write_cmos_sensor(0x72FB,0x2C);
	write_cmos_sensor(0x9333,0x03);
	write_cmos_sensor(0x9334,0x04);
	write_cmos_sensor(0x9335,0x05);
	write_cmos_sensor(0x9346,0x96);
	write_cmos_sensor(0x934A,0x8C);
	write_cmos_sensor(0x9352,0xAA);
	write_cmos_sensor(0xB0B6,0x05);
	write_cmos_sensor(0xB0B7,0x05);
	write_cmos_sensor(0xB0B9,0x05);
#endif

	imx362_ImageQuality_Setting();
	/*Need Mirror/Flip*/
	set_mirror_flip(0);

	//load_IMX362_SPC_Data();
	//write_cmos_sensor(0x7BC8,0x01);

	write_cmos_sensor(0x0100,0x00);//stream off
}       /*     sensor_init  */

static void preview_setting(void)
{
	int i=0;
	LOG_INF("preview E %ld %ld %ld\n", sizeof(preview_setting_array), sizeof(i2c_write_array), sizeof(preview_setting_array)/sizeof(i2c_write_array));
	mdelay(10);
	for(i = 0 ; i < sizeof(preview_setting_array)/sizeof(i2c_write_array);i++){
		write_cmos_sensor(preview_setting_array[i].addr,preview_setting_array[i].para);
	}
#if 0
	/*Mode2 Full(?)(HDR off)H:4032 V:3024*/
	//Mode Setting,as same as preview setting
	write_cmos_sensor(0x0112,0x0A);//CSI_DT_FMT_H
	write_cmos_sensor(0x0113,0x0A);//CSI_DT_FMT_L
	write_cmos_sensor(0x0114,0x03);//CSI_LANE_MODE:4LANE
	write_cmos_sensor(0x0220,0x00);//HDR_MODE:HDR Disable
	write_cmos_sensor(0x0221,0x11);//HDR_RESO_REDU_H,HDR_RESO_REDU_V:0x11=Fullresolution
	write_cmos_sensor(0x0340,0x0C);//FRM_LENGTH_LINES[15:8]
	write_cmos_sensor(0x0341,0xE0);//FRM_LENGTH_LINES[7:0]0x0CE0=3296
	write_cmos_sensor(0x0342,0x21);//LINE_LENGTH_PCK[15:8]
	write_cmos_sensor(0x0343,0x28);//LINE_LENGTH_PCK[7:0]0x2128=8488
	write_cmos_sensor(0x0381,0x01);//X_EVN_INC
	write_cmos_sensor(0x0383,0x01);//X_ODD_INC
	write_cmos_sensor(0x0385,0x01);//Y_EVN_INC
	write_cmos_sensor(0x0387,0x03);//Y_ODD_INC
	write_cmos_sensor(0x0900,0x00);//BINNING_MODE
	write_cmos_sensor(0x0901,0x11);//BINNING_TYPE_H,BINNING_TYPE_V
	write_cmos_sensor(0x30F4,0x00);
	write_cmos_sensor(0x30F5,0x14);
	write_cmos_sensor(0x30F6,0x00);
	write_cmos_sensor(0x30F7,0x14);
	write_cmos_sensor(0x31A0,0x02);
	write_cmos_sensor(0x31A5,0x00);
	write_cmos_sensor(0x31A6,0x00);
	write_cmos_sensor(0x560F,0x14);
	//Output Size Setting
	write_cmos_sensor(0x0344,0x00);//X_ADD_STA[11:8]
	write_cmos_sensor(0x0345,0x00);//X_ADD_STA[7:0]
	write_cmos_sensor(0x0346,0x00);//Y_ADD_STA[11:8]
	write_cmos_sensor(0x0347,0x00);//Y_ADD_STA[7:0]
	write_cmos_sensor(0x0348,0x0F);//X_ADD_END[11:8]
	write_cmos_sensor(0x0349,0xBF);//X_ADD_END[7:0]
	write_cmos_sensor(0x034A,0x0B);//Y_ADD_END[11:8]
	write_cmos_sensor(0x034B,0xCF);//Y_ADD_END[7:0]
	write_cmos_sensor(0x034C,0x0F);//X_OUT_SIZE[12:8]
	write_cmos_sensor(0x034D,0xC0);//X_OUT_SIZE[7:0]
	write_cmos_sensor(0x034E,0x0B);//Y_OUT_SIZE[11:8]
	write_cmos_sensor(0x034F,0xD0);//Y_OUT_SIZE[7:0]
	write_cmos_sensor(0x0408,0x00);//DIG_CROP_X_OFFSET[11:8]
	write_cmos_sensor(0x0409,0x00);//DIG_CROP_X_OFFSET[7:0]
	write_cmos_sensor(0x040A,0x00);//DIG_CROP_Y_OFFSET[11:8]
	write_cmos_sensor(0x040B,0x00);//DIG_CROP_Y_OFFSET[7:0]
	write_cmos_sensor(0x040C,0x0F);//DIG_CROP_IMAGE_WIDTH[11:8]
	write_cmos_sensor(0x040D,0xC0);//DIG_CROP_IMAGE_WIDTH[7:0]
	write_cmos_sensor(0x040E,0x0B);//DIG_CROP_IMAGE_HEIGHT[11:8]
	write_cmos_sensor(0x040F,0xD0);//DIG_CROP_IMAGE_HEIGHT[7:0]
	//Clock Setting
	write_cmos_sensor(0x0301,0x03);//IVTPXCK_DIV
	write_cmos_sensor(0x0303,0x02);//IVTSYCK_DIV
	write_cmos_sensor(0x0305,0x04);//PREPLLCK_IVT_DIV
	write_cmos_sensor(0x0306,0x00);//PLL_IVT_MPY[10:8]
	write_cmos_sensor(0x0307,0xD2);//PLL_IVT_MPY[7:0]
	write_cmos_sensor(0x0309,0x0A);//IOPPXCK_DIV
	write_cmos_sensor(0x030B,0x01);//IOPSYCK_DIV
	write_cmos_sensor(0x030D,0x04);//PREPLLCK_IOP_DIV
	write_cmos_sensor(0x030E,0x00);//PLL_IOP_MPY[10:8]
	write_cmos_sensor(0x030F,0xC8);//PLL_IOP_MPY[7:0]
	write_cmos_sensor(0x0310,0x01);//PLL_MULT_DRIV
#endif
	//     write_cmos_sensor(0x0220,0x00);//HDR mode disable
}

static void capture_setting(kal_uint16 curretfps, MUINT32 linelength)
{
	int i=0;
	LOG_INF("capture E, linelength:%d\n", linelength);

	mdelay(10);
	for(i = 0 ; i < sizeof(capture_setting_array)/sizeof(i2c_write_array);i++){
		write_cmos_sensor(capture_setting_array[i].addr,capture_setting_array[i].para);
	}

	if(linelength > 8000) {
		write_cmos_sensor(0x0342, linelength >> 8);
		write_cmos_sensor(0x0343, linelength & 0xFF);
	}
#if 0
	/*Mode2 Full(?)(HDR off)H:4032 V:3024*/
	//Mode Setting,as same as preview setting
	write_cmos_sensor(0x0112,0x0A);//CSI_DT_FMT_H
	write_cmos_sensor(0x0113,0x0A);//CSI_DT_FMT_L
	write_cmos_sensor(0x0114,0x03);//CSI_LANE_MODE:4LANE
	write_cmos_sensor(0x0220,0x00);//HDR_MODE:HDR Disable
	write_cmos_sensor(0x0221,0x11);//HDR_RESO_REDU_H,HDR_RESO_REDU_V:0x11=Fullresolution
	write_cmos_sensor(0x0340,0x0C);//FRM_LENGTH_LINES[15:8]
	write_cmos_sensor(0x0341,0xE0);//FRM_LENGTH_LINES[7:0]0x0CE0=3296
	write_cmos_sensor(0x0342,0x21);//LINE_LENGTH_PCK[15:8]
	write_cmos_sensor(0x0343,0x28);//LINE_LENGTH_PCK[7:0]0x2128=8488
	write_cmos_sensor(0x0381,0x01);//X_EVN_INC
	write_cmos_sensor(0x0383,0x01);//X_ODD_INC
	write_cmos_sensor(0x0385,0x01);//Y_EVN_INC
	write_cmos_sensor(0x0387,0x01);//Y_ODD_INC
	write_cmos_sensor(0x0900,0x00);//BINNING_MODE
	write_cmos_sensor(0x0901,0x11);//BINNING_TYPE_H,BINNING_TYPE_V
	write_cmos_sensor(0x30F4,0x00);
	write_cmos_sensor(0x30F5,0x14);
	write_cmos_sensor(0x30F6,0x00);
	write_cmos_sensor(0x30F7,0x14);
	write_cmos_sensor(0x31A0,0x02);
	write_cmos_sensor(0x31A5,0x00);
	write_cmos_sensor(0x31A6,0x00);
	write_cmos_sensor(0x560F,0x14);
	//Output Size Setting
	write_cmos_sensor(0x0344,0x00);//X_ADD_STA[11:8]
	write_cmos_sensor(0x0345,0x00);//X_ADD_STA[7:0]
	write_cmos_sensor(0x0346,0x00);//Y_ADD_STA[11:8]
	write_cmos_sensor(0x0347,0x00);//Y_ADD_STA[7:0]
	write_cmos_sensor(0x0348,0x0F);//X_ADD_END[11:8]
	write_cmos_sensor(0x0349,0xBF);//X_ADD_END[7:0]
	write_cmos_sensor(0x034A,0x0B);//Y_ADD_END[11:8]
	write_cmos_sensor(0x034B,0xCF);//Y_ADD_END[7:0]
	write_cmos_sensor(0x034C,0x0F);//X_OUT_SIZE[12:8]
	write_cmos_sensor(0x034D,0xC0);//X_OUT_SIZE[7:0]
	write_cmos_sensor(0x034E,0x0B);//Y_OUT_SIZE[11:8]
	write_cmos_sensor(0x034F,0xD0);//Y_OUT_SIZE[7:0]
	write_cmos_sensor(0x0408,0x00);//DIG_CROP_X_OFFSET[11:8]
	write_cmos_sensor(0x0409,0x00);//DIG_CROP_X_OFFSET[7:0]
	write_cmos_sensor(0x040A,0x00);//DIG_CROP_Y_OFFSET[11:8]
	write_cmos_sensor(0x040B,0x00);//DIG_CROP_Y_OFFSET[7:0]
	write_cmos_sensor(0x040C,0x0F);//DIG_CROP_IMAGE_WIDTH[11:8]
	write_cmos_sensor(0x040D,0xC0);//DIG_CROP_IMAGE_WIDTH[7:0]
	write_cmos_sensor(0x040E,0x0B);//DIG_CROP_IMAGE_HEIGHT[11:8]
	write_cmos_sensor(0x040F,0xD0);//DIG_CROP_IMAGE_HEIGHT[7:0]
	//Clock Setting
	write_cmos_sensor(0x0301,0x03);//IVTPXCK_DIV
	write_cmos_sensor(0x0303,0x02);//IVTSYCK_DIV
	write_cmos_sensor(0x0305,0x04);//PREPLLCK_IVT_DIV
	write_cmos_sensor(0x0306,0x00);//PLL_IVT_MPY[10:8]
	write_cmos_sensor(0x0307,0xD2);//PLL_IVT_MPY[7:0]
	write_cmos_sensor(0x0309,0x0A);//IOPPXCK_DIV
	write_cmos_sensor(0x030B,0x01);//IOPSYCK_DIV
	write_cmos_sensor(0x030D,0x04);//PREPLLCK_IOP_DIV
	write_cmos_sensor(0x030E,0x00);//PLL_IOP_MPY[10:8]
	write_cmos_sensor(0x030F,0xC8);//PLL_IOP_MPY[7:0]
	write_cmos_sensor(0x0310,0x01);//PLL_MULT_DRIV
#endif

	//write_cmos_sensor(0x0220,0x00);
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("normal video E\n");
	LOG_INF("E! currefps:%d\n",currefps);
	/*Mode2-03 FHD(HDR off)H:2016 V:1136*/
	write_cmos_sensor(0x0112,0x0A);
	write_cmos_sensor(0x0113,0x0A);
	write_cmos_sensor(0x0114,0x03);
	write_cmos_sensor(0x0220,0x00);
	write_cmos_sensor(0x0221,0x11);
	write_cmos_sensor(0x0340,0x06);
	write_cmos_sensor(0x0341,0x40);
	write_cmos_sensor(0x0342,0x11);
	write_cmos_sensor(0x0343,0x10);
	write_cmos_sensor(0x0381,0x01);
	write_cmos_sensor(0x0383,0x01);
	write_cmos_sensor(0x0385,0x01);
	write_cmos_sensor(0x0387,0x01);
	write_cmos_sensor(0x0900,0x01);
	write_cmos_sensor(0x0901,0x22);
	write_cmos_sensor(0x30F4,0x02);
	write_cmos_sensor(0x30F5,0xBC);
	write_cmos_sensor(0x30F6,0x01);
	write_cmos_sensor(0x30F7,0x18);
	write_cmos_sensor(0x31A0,0x02);
	write_cmos_sensor(0x31A5,0x00);
	write_cmos_sensor(0x31A6,0x01);
	write_cmos_sensor(0x560F,0x14);
	write_cmos_sensor(0x0344,0x00);
	write_cmos_sensor(0x0345,0x00);
	write_cmos_sensor(0x0346,0x01);
	write_cmos_sensor(0x0347,0x78);
	write_cmos_sensor(0x0348,0x0F);
	write_cmos_sensor(0x0349,0xBF);
	write_cmos_sensor(0x034A,0x0A);
	write_cmos_sensor(0x034B,0x57);
	write_cmos_sensor(0x034C,0x07);
	write_cmos_sensor(0x034D,0xE0);
	write_cmos_sensor(0x034E,0x04);
	write_cmos_sensor(0x034F,0x70);
	write_cmos_sensor(0x0408,0x00);
	write_cmos_sensor(0x0409,0x00);
	write_cmos_sensor(0x040A,0x00);
	write_cmos_sensor(0x040B,0x00);
	write_cmos_sensor(0x040C,0x07);
	write_cmos_sensor(0x040D,0xE0);
	write_cmos_sensor(0x040E,0x04);
	write_cmos_sensor(0x040F,0x70);
	write_cmos_sensor(0x0301,0x03);
	write_cmos_sensor(0x0303,0x02);
	write_cmos_sensor(0x0305,0x04);
	write_cmos_sensor(0x0306,0x00);
	write_cmos_sensor(0x0307,0xD2);
	write_cmos_sensor(0x0309,0x0A);
	write_cmos_sensor(0x030B,0x01);
	write_cmos_sensor(0x030D,0x04);
	write_cmos_sensor(0x030E,0x01);
	write_cmos_sensor(0x030F,0x57);
	write_cmos_sensor(0x0310,0x01);

	write_cmos_sensor(0x0220,0x00);
	LOG_INF("imgsensor.hdr_mode in video mode:%d\n",imgsensor.hdr_mode);

}
/*
   static void hs_video_setting(void)
   {
   LOG_INF("hs_video E\n");
   write_cmos_sensor(0x0100,0x00);
   mdelay(10);
   write_cmos_sensor(0x0112,0x0A);
   write_cmos_sensor(0x0113,0x0A);
   write_cmos_sensor(0x0114,0x03);
   write_cmos_sensor(0x0220,0x00);//HDR_MODE:HDR Disable
   write_cmos_sensor(0x0221,0x11);//HDR_RESO_REDU_H,HDR_RESO_REDU_V:0x11=Fullresolution
   write_cmos_sensor(0x0340,0x02);//FRM_LENGTH_LINES[15:8]
   write_cmos_sensor(0x0341,0xE8);//FRM_LENGTH_LINES[7:0]0x20E8=744
   write_cmos_sensor(0x0342,0x09);//LINE_LENGTH_PCK[15:8]
   write_cmos_sensor(0x0343,0x18);//LINE_LENGTH_PCK[7:0]0x0918=2328
   write_cmos_sensor(0x0381,0x01);//X_EVN_INC
   write_cmos_sensor(0x0383,0x01);//X_ODD_INC
   write_cmos_sensor(0x0385,0x01);//Y_EVN_INC
   write_cmos_sensor(0x0387,0x03);//Y_ODD_INC
   write_cmos_sensor(0x0900,0x00);//BINNING_MODE
   write_cmos_sensor(0x0901,0x42);//BINNING_TYPE_H,BINNING_TYPE_V
   write_cmos_sensor(0x30F4,0x00);
   write_cmos_sensor(0x30F5,0x14);
   write_cmos_sensor(0x30F6,0x00);
   write_cmos_sensor(0x30F7,0x14);
   write_cmos_sensor(0x31A0,0x02);
   write_cmos_sensor(0x31A5,0x00);
   write_cmos_sensor(0x31A6,0x00);
   write_cmos_sensor(0x560F,0x14);
//Output Size Setting
write_cmos_sensor(0x0344,0x00);//X_ADD_STA[11:8]
write_cmos_sensor(0x0345,0x00);//X_ADD_STA[7:0]
write_cmos_sensor(0x0346,0x01);//Y_ADD_STA[11:8]
write_cmos_sensor(0x0347,0x78);//Y_ADD_STA[7:0]
write_cmos_sensor(0x0348,0x0F);//X_ADD_END[11:8]
write_cmos_sensor(0x0349,0xBF);//X_ADD_END[7:0]
write_cmos_sensor(0x034A,0x0A);//Y_ADD_END[11:8]
write_cmos_sensor(0x034B,0x57);//Y_ADD_END[7:0]
write_cmos_sensor(0x034C,0x0F);//X_OUT_SIZE[12:8]
write_cmos_sensor(0x034D,0xC0);//X_OUT_SIZE[7:0]
write_cmos_sensor(0x034E,0x04);//Y_OUT_SIZE[11:8]
write_cmos_sensor(0x034F,0x70);//Y_OUT_SIZE[7:0]
write_cmos_sensor(0x0408,0x00);//DIG_CROP_X_OFFSET[11:8]
write_cmos_sensor(0x0409,0x00);//DIG_CROP_X_OFFSET[7:0]
write_cmos_sensor(0x040A,0x00);//DIG_CROP_Y_OFFSET[11:8]
write_cmos_sensor(0x040B,0x00);//DIG_CROP_Y_OFFSET[7:0]
write_cmos_sensor(0x040C,0x0F);//DIG_CROP_IMAGE_WIDTH[11:8]
write_cmos_sensor(0x040D,0xC0);//DIG_CROP_IMAGE_WIDTH[7:0]
write_cmos_sensor(0x040E,0x04);//DIG_CROP_IMAGE_HEIGHT[11:8]
write_cmos_sensor(0x040F,0x70);//DIG_CROP_IMAGE_HEIGHT[7:0]
//Clock Setting
write_cmos_sensor(0x0301,0x03);//IVTPXCK_DIV
write_cmos_sensor(0x0303,0x02);//IVTSYCK_DIV
write_cmos_sensor(0x0305,0x04);//PREPLLCK_IVT_DIV
write_cmos_sensor(0x0306,0x00);//PLL_IVT_MPY[10:8]
write_cmos_sensor(0x0307,0xD2);//PLL_IVT_MPY[7:0]
write_cmos_sensor(0x0309,0x0A);//IOPPXCK_DIV
write_cmos_sensor(0x030B,0x01);//IOPSYCK_DIV
write_cmos_sensor(0x030D,0x04);//PREPLLCK_IOP_DIV
write_cmos_sensor(0x030E,0x01);//PLL_IOP_MPY[10:8]
write_cmos_sensor(0x030F,0x5E);//PLL_IOP_MPY[7:0]
write_cmos_sensor(0x0310,0x01);//PLL_MULT_DRIV

write_cmos_sensor(0x0220,0x00);
write_cmos_sensor(0x0100,0x01);
mdelay(10);

}
*/
static void slim_video_setting(void)
{
	LOG_INF("slim video E\n");


	write_cmos_sensor(0x0220,0x00);


}


/*************************************************************************
 * FUNCTION
 *       get_imgsensor_id
 *
 * DESCRIPTION
 *       This function get the sensor ID
 *
 * PARAMETERS
 *       *sensorID : return the sensor ID
 *
 * RETURNS
 *       None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
//#define SLT_DEVINFO_CMM
#ifdef SLT_DEVINFO_CMM
#include  <linux/dev_info.h>
static struct devinfo_struct *s_DEVINFO_ccm;   //suppose 10 max lcm device
#endif
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry_total = 1;
	kal_uint8 retry_cnt = retry_total;
#ifdef SLT_DEVINFO_CMM
	s_DEVINFO_ccm =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);
	s_DEVINFO_ccm->device_type = "CCM";
	s_DEVINFO_ccm->device_module = "PC0FB0002B";//can change if got module id
	s_DEVINFO_ccm->device_vendor = "Sunrise";
	s_DEVINFO_ccm->device_ic = "IMX362";
	s_DEVINFO_ccm->device_version = "HI";
	s_DEVINFO_ccm->device_info = "200W";
#endif
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
#ifdef SLT_DEVINFO_CMM
				s_DEVINFO_ccm->device_used = DEVINFO_USED;
				devinfo_check_add_device(s_DEVINFO_ccm);
#endif
				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
			retry_cnt--;
		} while(retry_cnt > 0);
		i++;
		retry_cnt = retry_total;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		// if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF
		*sensor_id = 0xFFFFFFFF;
#ifdef SLT_DEVINFO_CMM
		s_DEVINFO_ccm->device_used = DEVINFO_UNUSED;
		devinfo_check_add_device(s_DEVINFO_ccm);
#endif
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
 * FUNCTION
 *       open
 *
 * DESCRIPTION
 *       This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *       None
 *
 * RETURNS
 *       None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 open(void)
{
	//const kal_uint8 i2c_addr[] = {IMGSENSOR_WRITE_ID_1, IMGSENSOR_WRITE_ID_2};
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint32 sensor_id = 0;
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
			LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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
	imgsensor.gain = 0xe000; //0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.hdr_mode = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}       /*     open  */



/*************************************************************************
 * FUNCTION
 *       close
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *       None
 *
 * RETURNS
 *       None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	/*No Need to implement this function*/

	return ERROR_NONE;
}       /*     close  */


/*************************************************************************
 * FUNCTION
 * preview
 *
 * DESCRIPTION
 *       This function start the sensor preview.
 *
 * PARAMETERS
 *       *image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *       None
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
	return ERROR_NONE;
}       /*     preview   */

/*************************************************************************
 * FUNCTION
 *       capture
 *
 * DESCRIPTION
 *       This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *       None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E, linelength=%d\n", sensor_config_data->Pixels);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
#if 0
	else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
#endif
	else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps, sensor_config_data->Pixels);
	//     preview_setting();
	mdelay(100);
	return ERROR_NONE;
}       /* capture() */
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
	LOG_INF("ihdr enable :%d\n", imgsensor.hdr_mode);
	normal_video_setting(imgsensor.current_fps);
	return ERROR_NONE;
}       /*     normal_video   */

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
	//imgsensor.current_fps = 600;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);

	return ERROR_NONE;
}       /*     hs_video   */

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
	//imgsensor.current_fps = 1200;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();

	return ERROR_NONE;
}       /*     slim_video       */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth       = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight      = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight         = imgsensor_info.slim_video.grabwindow_height;
	return ERROR_NONE;
}       /*     get_resolution        */

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

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;               /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;     /* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

	if(imx362_type == IMX362_HDR_TYPE)
		sensor_info->PDAF_Support = PDAF_SUPPORT_RAW_DUALPD; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode(Full), 3:PDAF VC mode(Binning)*/
	else
		sensor_info->PDAF_Support = PDAF_SUPPORT_CAMSV_LEGACY; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode(Full), 3:PDAF VC mode(Binning)*/
	//sensor_info->PDAF_Support = 0; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode(Full), 3:PDAF VC mode(Binning)*/

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
	sensor_info->SensorHightSampling = 0;       // 0 is default 1x
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
}       /*     get_info  */


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
}       /* control() */



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
		write_cmos_sensor(0x0601, 0x02);
	} else {
		write_cmos_sensor(0x0601, 0x00);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(0x0100, 0X01);
	else
		write_cmos_sensor(0x0100, 0x00);
	mdelay(10);
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
	//    unsigned long long *feature_return_para=(unsigned long long *) feature_para;

	SET_PD_BLOCK_INFO_T *PDAFinfo;
	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	//SENSOR_VC_INFO_STRUCT *pvcinfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	if(!((feature_id == 3004) || (feature_id == 3006)))
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
		set_shutter((UINT16)*feature_data);
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
		get_default_framerate_by_scenario((MSDK_SCENARIO_ID_ENUM)*feature_data, (MUINT32 *)(uintptr_t)(*(feature_data+1)));
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
		//read_IMX362_eeprom((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len=4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		LOG_INF("current fps :%d\n",  (UINT32)*feature_data);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		/* HDR mODE : 0: disable HDR, 1:IHDR, 2:HDR, 9:ZHDR */
		LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.hdr_mode = (BOOL)*feature_data;
		LOG_INF("ihdr enable :%d\n", imgsensor.hdr_mode);
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
		break;

		//add for imx362 pdaf
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld\n", *feature_data);
		PDAFinfo= (SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			break;
		}
		break;

	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:

		LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n", *feature_data);
		//PDAF capacity enable or not, s5k2l7 only full size support PDAF
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
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
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1; ///preview support
			break;
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;


	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
		ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
		break;

	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_time((UINT16)*feature_data,(UINT16)*(feature_data+1));
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n", *feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	default:
		break;
	}

	return ERROR_NONE;
}       /*     feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 IMX362_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}       /*     IMX362_MIPI_RAW_SensorInit     */
