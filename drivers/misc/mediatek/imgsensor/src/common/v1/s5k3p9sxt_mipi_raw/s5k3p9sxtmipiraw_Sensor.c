/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 S5K3P9SXmipi_Sensor.c
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
#include <linux/atomic.h>
#include <linux/types.h>

//#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_typedef.h"
#include "s5k3p9sxtmipiraw_Sensor.h"
#ifndef VENDOR_EDIT
#define VENDOR_EDIT 1
#endif
#if VENDOR_EDIT
/*xxxx@Camera.Driver  add for multi project using one build*/
//#include <soc/xxxx/xxxx_project.h>
#endif

#ifndef USE_TNP_BURST
#define USE_TNP_BURST
#endif


#define PFX "S5K3P9SXT_camera_sensor"
//#define LOG_WRN(format, args...) xlog_printk(ANDROID_LOG_WARN ,PFX, "[%S] " format, __FUNCTION__, ##args)
//#defineLOG_INF(format, args...) xlog_printk(ANDROID_LOG_INFO ,PFX, "[%s] " format, __FUNCTION__, ##args)
//#define LOG_DBG(format, args...) xlog_printk(ANDROID_LOG_DEBUG ,PFX, "[%S] " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)	pr_debug(PFX "[%s] " format, __func__, ##args)

#if VENDOR_EDIT
/*xxxx ,modify for different module*/
#define MODULE_ID_OFFSET 0x0001
static kal_uint32 streaming_control(kal_bool enable);
#endif

#if VENDOR_EDIT
/* Add by xxxx for register device info  */
//#define DEVICE_VERSION_S5K3P9SXT    "s5k3p9sxt"
//extern void register_imgsensor_deviceinfo(char *name, char *version, u8 module_id);
//static kal_uint8 deviceInfo_register_value = 0x00;
static bool bNeedSetNormalMode = KAL_FALSE;

#endif
static DEFINE_SPINLOCK(imgsensor_drv_lock);
extern char back_cam_efuse_id[64];
extern char back_cam_name[64];
extern u32 dual_main_sensorid;

static imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K3P9SX_SENSOR_ID,
	#if VENDOR_EDIT
	/*xxxx add */
	.module_id = 0x01,	//
	#endif

	.checksum_value = 0xB1F1B3CC,

	.pre = {
		.pclk = 560000000,              //record different mode's pclk
		.linelength = 8496,	         /* record different mode's linelength */
		.framelength = 2196,         //record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2320,	/* record different mode's width of grabwindow */
		.grabwindow_height = 1744,	/* record different mode's height of grabwindow */
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 216000000,
	},
	.cap = {
		.pclk = 560000000,
		.linelength = 5088,
		.framelength = 3668,
		.startx =0,
		.starty = 0,
		.grabwindow_width = 4640,
		.grabwindow_height = 3488,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 585600000,
	},
	#if 0
	.cap1 = {
		.pclk = 560000000,
		.linelength = 6400,
		.framelength = 3643,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4640,
		.grabwindow_height = 3488,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 240,
	},
	.cap2 = {
		.pclk = 560000000,
		.linelength = 10240,
		.framelength = 3643,
		.startx = 0,
		.starty =0,
		.grabwindow_width = 4640,
		.grabwindow_height = 3488,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 150,
	},
	#endif
	.normal_video = {
		.pclk = 560000000,
		.linelength = 6784,
		.framelength = 2750,
		.startx =0,
		.starty = 0,
		.grabwindow_width = 4640,
		.grabwindow_height = 2608,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 456000000,
	},
	.hs_video = {
		.pclk = 560000000,
		.linelength = 5088,
		.framelength =917,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
		.mipi_pixel_rate = 216000000,
	},
	#if 0
	.slim_video = {
		.pclk = 560000000,              //record different mode's pclk
		.linelength = 5120,	/* record different mode's linelength */
		.framelength = 3644,	/* record different mode's framelength */
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 1920,	/* record different mode's width of grabwindow */
		.grabwindow_height = 1080,	/* record different mode's height of grabwindow */
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
	},
	#endif
	.custom1 = {
		.pclk = 560000000,
		.linelength = 6368,
		.framelength = 3662,
		.startx =0,
		.starty = 0,
		.grabwindow_width = 4640,
		.grabwindow_height = 3488,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 240,
		.mipi_pixel_rate = 585600000,
	},
	.custom2 = {
		.pclk = 560000000,
		.linelength = 6368,
		.framelength = 3662,
		.startx =0,
		.starty = 0,
		.grabwindow_width = 4640,
		.grabwindow_height = 3488,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 240,
		.mipi_pixel_rate = 585600000,
	},
	.margin = 4,
	.min_shutter = 4,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.frame_time_delay_frame = 2,
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 6,	  //support sensor mode num

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,
	.custom1_delay_frame = 3,   /* enter custom1 delay frame num */
	.custom2_delay_frame = 3,
	.custom3_delay_frame = 3,
	.custom4_delay_frame = 3,
	.custom5_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = 0, //0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x20, 0xff},
};

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] =
{// Preview mode setting
 {0x02, 0x0A, 0x00, 0x08, 0x40, 0x00,
  0x00, 0x2B, 0x0910, 0x06D0, 0x01, 0x00, 0x0000, 0x0000,
  0x01, 0x30, 0x0168, 0x0360, 0x03, 0x00, 0x0000, 0x0000},
// Capture mode setting
 {0x02, 0x0A, 0x00, 0x08, 0x40, 0x00,
  0x00, 0x2B, 0x1220, 0x0DA0, 0x01, 0x00, 0x0000, 0x0000,
  0x01, 0x30, 0x0168, 0x0360, 0x03, 0x00, 0x0000, 0x0000},
// Video mode setting
 {0x02, 0x0A, 0x00, 0x08, 0x40, 0x00,
  0x00, 0x2B, 0x1220, 0x0DA0, 0x01, 0x00, 0x0000, 0x0000,
  0x01, 0x30, 0x0168, 0x028C, 0x03, 0x00, 0x0000, 0x0000}};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_HV_MIRROR,				//mirrorflip information
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
	.i2c_write_id = 0x20,

};


/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[7] =
{	{4640, 3488, 0, 0, 4640, 3488, 2320, 1744, 0000, 0000, 2320, 1744, 0, 0, 2320, 1744},	/* Preview */
	{4640, 3488, 0, 0, 4640, 3488, 4640, 3488, 0000, 0000, 4640, 3488, 0, 0, 4640, 3488},	/* capture */
	{4640, 3488, 0, 440, 4640, 2608, 4640, 2608, 0000, 0000, 4640, 2608, 0, 0, 4640, 2608},	/* video */
	{4640, 3488, 400, 664, 3840, 2160, 1280, 720, 0000, 0000, 1280, 720, 0, 0, 1280, 720},	/* hight speed video */
	{4640, 3488, 400, 664, 3840, 2160, 1920, 1080, 0000, 0000, 1920, 1080, 0, 0, 1920, 1080},/* slim video */
	{4640, 3488, 0, 0, 4640, 3488, 4640, 3488, 0000, 0000, 4640, 3488, 0, 0, 4640, 3488},	/* custom1*/
	{4640, 3488, 0, 0, 4640, 3488, 4640, 3488, 0000, 0000, 4640, 3488, 0, 0, 4640, 3488},	/* custom2*/
};

//mirror flip
#if 1
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX = 16,
	.i4OffsetY = 16,
	.i4PitchX  = 32,
	.i4PitchY  = 16,
	.i4PairNum  = 4,
	.i4SubBlkW  = 16,
	.i4SubBlkH  = 8,
	.i4PosL = {{27, 20}, {43, 20}, {19, 24}, {35, 24}},
	.i4PosR = {{19, 16}, {35, 16}, {27, 28}, {43, 28}},
	.i4BlockNumX = 144,
	.i4BlockNumY = 216,
	.iMirrorFlip = 0,
	.i4Crop = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
};
#endif
#if 0
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
        .i4OffsetX = 16,
        .i4OffsetY = 16,
        .i4PitchX  = 32,
        .i4PitchY  = 16,
        .i4PairNum  = 4,
        .i4SubBlkW  = 16,
        .i4SubBlkH  = 8,
        .i4PosL = {{20, 19}, {36, 19}, {28, 31}, {44, 31}},
        .i4PosR = {{28, 23}, {44, 23}, {20, 27}, {36, 27}},
        .i4BlockNumX = 144,
        .i4BlockNumY = 216,
        .iMirrorFlip = 0,
        .i4Crop = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
};
#endif

#if 0
static SET_PD_BLOCK_INFO_T imgsensor_pd_info_16_9 =
{
	.i4OffsetX = 16,
	.i4OffsetY = 16,
	.i4PitchX  = 32,
	.i4PitchY  = 16,
	.i4PairNum  = 4,
	.i4SubBlkW  = 16,
	.i4SubBlkH  = 8,
	.i4PosL = {{19, 16}, {35, 16}, {27, 28}, {43, 28}},
	.i4PosR = {{27, 20}, {43, 20}, {19, 24}, {35, 24}},
	.i4BlockNumX = 144,
	.i4BlockNumY = 162,
	.iMirrorFlip = 0,
	.i4Crop = { {0, 0}, {0, 0}, {0, 440}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
};
#endif


extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);

/*
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	iReadReg((u16) addr ,(u8*)&get_byte, imgsensor.i2c_write_id);
	return get_byte;
}

#define write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1,  imgsensor.i2c_write_id)
*/

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(pusendcmd, 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
	return ((get_byte << 8) & 0xff00)| ((get_byte >> 8) & 0x00ff);
}


static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF),(char)(para >> 8), (char)(para & 0xFF)};
	iWriteRegI2C(pusendcmd, 4, imgsensor.i2c_write_id);
}

#if VENDOR_EDIT
static void write_cmos_sensor_byte(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	//kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}
#endif

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte=0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};
	iReadRegI2C(pusendcmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	//return; //for test
	write_cmos_sensor(0x0340, imgsensor.frame_length);
	write_cmos_sensor(0x0342, imgsensor.line_length);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable %d\n", framerate,
		min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	if (frame_length >= imgsensor.min_frame_length) {
		imgsensor.frame_length = frame_length;
	} else {
		imgsensor.frame_length = imgsensor.min_frame_length;
	}
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en) {
		imgsensor.min_frame_length = imgsensor.frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */

static void write_shutter(kal_uint32 shutter)
{

	kal_uint16 realtime_fps = 0;
	#if VENDOR_EDIT
	/*xxxx,,add for slow shutter */
	//kal_uint32 exp_time = 0;// shutter * 5120 * 1000 / 560000000;
	//kal_uint32 value_coarse_2frame = 0;// reg_0x303E
	//kal_uint32 value_coarse_1frame = 0;// reg_0x304A
	//kal_uint16 pow_value = 0;
	//char value_pck_shifter = 0;  //reg_0X305B
	/*xxxx,20180108, add for slow shutter mirror*/
	//kal_uint8 itemp_mirror;
	kal_uint32 CintR = 0;
	kal_uint32 Time_Farme = 0;
	kal_uint32 Frame_length_lines_on_2nd_Frame = 0;
	#endif
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
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if(realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
		// Extend frame length
			write_cmos_sensor(0x0340, imgsensor.frame_length);

		}
	} else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length);

	}
	// Update Shutter
	#if VENDOR_EDIT
	if (shutter >= 0xFFF0) {  // need to modify line_length & PCLK
		bNeedSetNormalMode = KAL_TRUE;
		if (shutter >= 1760000) {  //>16s
			shutter = 1760000;
		}
		if (shutter <= 120000) {
			CintR = shutter / 2;
			Time_Farme = CintR + 0x0010;
			//streaming_control(0);
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000);
			mdelay(100);
			write_cmos_sensor_byte(0x0104, 0x01);
			write_cmos_sensor(0x0340, Time_Farme & 0xFFFF);  //FLL
			write_cmos_sensor(0x0342, 0x13E0);  //LLP
			write_cmos_sensor(0x0202, CintR & 0xFFFF); //CintR
			write_cmos_sensor_byte(0x0104, 0x00);
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0100);
		} else {
			//CintR = 1719 * (shutter * 5080) / 560000000;
			CintR = (5461 * shutter) / 350000;
			Time_Farme = CintR + 0x0010;
			//Frame_length_lines_on_2nd_Frame= 110066 * (shutter * 5080) / 560000000;
			Frame_length_lines_on_2nd_Frame= (11 * shutter * 127) / 1400;
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000);
			write_cmos_sensor(0x0342, 0x13E0);  //LLP
			write_cmos_sensor(0x0E0A, 0x0003);
			write_cmos_sensor(0x0E0C, 0x0100);
			write_cmos_sensor(0x0E0E, 0x0001);
			write_cmos_sensor(0x0E10, 0x0004);
			write_cmos_sensor(0x0E12, Time_Farme & 0xFFFF);
			write_cmos_sensor(0x0E14, 0x0100);
			write_cmos_sensor(0x0E16, 0x0100);
			write_cmos_sensor(0x0E18, 0x0100);
			write_cmos_sensor(0x6028, 0x2000);
			write_cmos_sensor(0x602A, 0x4A7A);  //55c0
			write_cmos_sensor(0x6F12, 0x0006);
			write_cmos_sensor(0x602A, 0x4A7C);  //55C4
			write_cmos_sensor(0x6F12, (Frame_length_lines_on_2nd_Frame >> 16) & 0x00FF);
			write_cmos_sensor(0x6F12, Frame_length_lines_on_2nd_Frame & 0xFFFF);
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0100);
		}

	} else {
		if (bNeedSetNormalMode) {
			LOG_INF("exit long shutter\n");
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000);
			write_cmos_sensor(0x0342, 0x13E0);
			write_cmos_sensor(0x0E0A, 0x0000);
			write_cmos_sensor(0x0E0C, 0x0000);
			write_cmos_sensor(0x0E0E, 0x0000);
			write_cmos_sensor(0x0E10, 0x0000);
			write_cmos_sensor(0x0E12, 0x0000);
			write_cmos_sensor(0x0E14, 0x0000);
			write_cmos_sensor(0x0E16, 0x0000);
			write_cmos_sensor(0x0E18, 0x0000);
			write_cmos_sensor(0x6028, 0x2000);
			write_cmos_sensor(0x602A, 0x4A7A);  
			write_cmos_sensor(0x6F12, 0x0000);
			write_cmos_sensor(0x602A, 0x4A7C);  
			write_cmos_sensor(0x6F12, 0x0000);
			write_cmos_sensor(0x6F12, 0x0000);
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0100);
			bNeedSetNormalMode = KAL_FALSE;
		}

		write_cmos_sensor(0x0340, imgsensor.frame_length);
		write_cmos_sensor(0x0202, imgsensor.shutter);
	}
	#endif

	LOG_INF("shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

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
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}	/*	set_shutter */

/*************************************************************************
* FUNCTION
*	set_shutter_frame_length
*
* DESCRIPTION
*	for frame & 3A sync
*
*************************************************************************/

static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length, should extend frame length first */
	spin_lock(&imgsensor_drv_lock);
	/* Change frame time */
	dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;

	if (shutter > imgsensor.frame_length - imgsensor_info.margin) {
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	}
	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;


	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
			/* Extend frame length */
			write_cmos_sensor(0x0340, imgsensor.frame_length);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length);
	}

	/* Update Shutter */
	write_cmos_sensor(0x0202, imgsensor.shutter);
	LOG_INF("Exit! shutter =%d, framelength =%d/%d, dummy_line=%d, auto_extend=%d\n",
		shutter, imgsensor.frame_length, frame_length, dummy_line, read_cmos_sensor(0x0350));


}	/* set_shutter_frame_length */


static kal_uint16 gain2reg(const kal_uint16 gain)
{
	 kal_uint16 reg_gain = 0x0;

	reg_gain = gain / 2;
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

	if (gain < BASEGAIN || gain > 32 * BASEGAIN) {
		LOG_INF("Error gain setting");

		if (gain < BASEGAIN) {
			gain = BASEGAIN;
		} else if (gain > 32 * BASEGAIN) {
			gain = 32 * BASEGAIN;
		}
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	//write_cmos_sensor(0x0204,reg_gain);
	write_cmos_sensor_8(0x0204, (reg_gain >> 8));
	write_cmos_sensor_8(0x0205, (reg_gain & 0xff));

	return gain;
}	/*	set_gain  */
#if 1
static void set_mirror_flip(kal_uint8 image_mirror)
{
	kal_uint8 itemp;
	LOG_INF("image_mirror = %d\n", image_mirror);
	itemp = read_cmos_sensor(0x0101);
	itemp &= ~0x03;

	switch (image_mirror) {

			case IMAGE_NORMAL:
				write_cmos_sensor_byte(0x0101, itemp);
				break;

			case IMAGE_V_MIRROR:
				write_cmos_sensor_byte(0x0101, itemp | 0x02);
				break;

			case IMAGE_H_MIRROR:
				write_cmos_sensor_byte(0x0101, itemp | 0x01);
				break;

			case IMAGE_HV_MIRROR:
				write_cmos_sensor_byte(0x0101, itemp | 0x03);
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
#if 0
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}	/*	night_mode	*/
#endif

static kal_uint32 streaming_control(kal_bool enable)
{
	int timeout = 100;
	int i = 0;
	int framecnt = 0;

	LOG_INF("streaming_enable(0=Sw Standby, 1=streaming): %d\n", enable);
	if (enable) {
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor_8(0x0100, 0X01);
		mDELAY(10);
	} else {
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor_8(0x0100, 0x00);
		for (i = 0; i < timeout; i++) {
			mDELAY(10);
			framecnt = read_cmos_sensor_8(0x0005);
			if (framecnt == 0xFF) {
				LOG_INF(" Stream Off OK at i=%d.\n", i);
				return ERROR_NONE;
			}
		}
		LOG_INF("Stream Off Fail! framecnt=%d.\n", framecnt);
	}
	return ERROR_NONE;
}

#ifdef USE_TNP_BURST
static const u16 uTnpArrayA[] = {
	0x126F,
	0x0000,
	0x0000,
	0x4906,
	0x4805,
	0xF8C1,
	0x05C4,
	0x4905,
	0x1A08,
	0x4903,
	0xF8A1,
	0x05C8,
	0xF000,
	0xBC65,
	0x0000,
	0x0020,
	0x844A,
	0x0020,
	0xD02E,
	0x0020,
	0x006C,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0xBA40,
	0x4770,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0xBAC0,
	0x4770,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0xE92D,
	0x47F0,
	0x461C,
	0x4690,
	0x4689,
	0x4607,
	0x48FE,
	0x2200,
	0x6800,
	0xB286,
	0x0C05,
	0x4631,
	0x4628,
	0xF000,
	0xFCC5,
	0x4623,
	0x4642,
	0x4649,
	0x4638,
	0xF000,
	0xFCC4,
	0x48F8,
	0xF890,
	0x028B,
	0xB188,
	0x8A78,
	0xF104,
	0x5400,
	0xEB04,
	0x0180,
	0xE009,
	0x6822,
	0xF3C2,
	0x60C3,
	0xFA90,
	0xF0A0,
	0xF022,
	0x4278,
	0xEA42,
	0x0050,
	0xC401,
	0x428C,
	0xD1F3,
	0x4631,
	0x4628,
	0xE8BD,
	0x47F0,
	0x2201,
	0xF000,
	0xBCA2,
	0xE92D,
	0x5FFC,
	0x4683,
	0x48E7,
	0x468A,
	0x2200,
	0x6840,
	0x0C01,
	0xB280,
	0xE9CD,
	0x0100,
	0x4601,
	0x9801,
	0xF000,
	0xFC93,
	0xFBAB,
	0x100A,
	0x4BE2,
	0x4DE0,
	0x4AE0,
	0xF893,
	0x6091,
	0xF505,
	0x69AA,
	0xFB06,
	0xF60B,
	0x2300,
	0x1B89,
	0x464D,
	0xEB60,
	0x0003,
	0xC503,
	0x461D,
	0xFBEB,
	0x650A,
	0xF502,
	0x67AB,
	0x463A,
	0x4CD6,
	0xC260,
	0xF8A4,
	0x3548,
	0xF504,
	0x62A9,
	0xF894,
	0xC4A0,
	0xF894,
	0x44A1,
	0xF44F,
	0x58F8,
	0xF1BC,
	0x0F01,
	0xD003,
	0xF1BC,
	0x0F02,
	0xD014,
	0xE029,
	0xEA08,
	0x2304,
	0xF043,
	0x0311,
	0x8013,
	0x4623,
	0x2200,
	0xF000,
	0xFC6A,
	0xE9C9,
	0x0100,
	0x4623,
	0x2200,
	0x4628,
	0x4631,
	0xF000,
	0xFC62,
	0xE9C7,
	0x0100,
	0xE015,
	0xEA08,
	0x2C04,
	0xF04C,
	0x0C01,
	0xF8A2,
	0xC000,
	0xFBA1,
	0x2C04,
	0xFB00,
	0xC004,
	0xFB01,
	0x0103,
	0xE9C9,
	0x1200,
	0xFBA6,
	0x0104,
	0xFB05,
	0x1104,
	0xFB06,
	0x1103,
	0xE9C7,
	0x1000,
	0x48B8,
	0x49B9,
	0xF8B0,
	0x0548,
	0x8008,
	0x48B8,
	0xC80C,
	0xF648,
	0x0022,
	0xF000,
	0xFC43,
	0x48B5,
	0x3008,
	0xC80C,
	0xF648,
	0x002A,
	0xF000,
	0xFC3C,
	0x4658,
	0xF000,
	0xFC3E,
	0x49AD,
	0x2201,
	0xF8C1,
	0xA568,
	0xE9DD,
	0x1000,
	0xB002,
	0xE8BD,
	0x5FF0,
	0xF000,
	0xBC1F,
	0x4AA8,
	0xF892,
	0x25D5,
	0xB12A,
	0x4AA6,
	0x4BA5,
	0xF8D2,
	0x2568,
	0xF8C3,
	0x2430,
	0x4AA3,
	0xF8D2,
	0x2430,
	0xF000,
	0xBC29,
	0xB510,
	0x49A0,
	0x4AA3,
	0x4BA4,
	0xF8D1,
	0x143C,
	0x7C94,
	0xB10C,
	0x8A90,
	0xE01B,
	0x4A9B,
	0xF892,
	0x20A2,
	0xF1C2,
	0x020C,
	0x40D1,
	0x4348,
	0x0A01,
	0x489D,
	0xF8D0,
	0x0084,
	0x7902,
	0x434A,
	0x7941,
	0x88C0,
	0x40CA,
	0xEB00,
	0x1012,
	0xF44F,
	0x2180,
	0xFBB1,
	0xF0F0,
	0x1109,
	0x4288,
	0xD204,
	0x2840,
	0xD800,
	0x2040,
	0x8058,
	0xBD10,
	0x4608,
	0xE7FB,
	0x6841,
	0x7B4A,
	0x4991,
	0xF8A1,
	0x2382,
	0x6842,
	0x7B53,
	0x2B00,
	0xD015,
	0xF501,
	0x7161,
	0x7B92,
	0x800A,
	0x6840,
	0x7BC0,
	0x8048,
	0x488B,
	0xF8B0,
	0x20C2,
	0x808A,
	0xF8B0,
	0x20C4,
	0x80CA,
	0xF810,
	0x2FC7,
	0x78C0,
	0x0852,
	0x0840,
	0xEA42,
	0x0080,
	0x8108,
	0x4770,
	0xE92D,
	0x4FFF,
	0x4883,
	0xB083,
	0x461D,
	0x79C0,
	0xF8DD,
	0xB044,
	0x4616,
	0x460F,
	0x2800,
	0xD06E,
	0xF8DF,
	0xA1F4,
	0xF10A,
	0x0ABA,
	0xF1AA,
	0x001C,
	0xF8B0,
	0x9000,
	0xF8B0,
	0x8004,
	0xF000,
	0xFBCC,
	0x9903,
	0x9C10,
	0x4308,
	0xF104,
	0x4480,
	0xD007,
	0x80A7,
	0x80E6,
	0xF1AA,
	0x001C,
	0x8801,
	0x8121,
	0x8880,
	0xE020,
	0x4868,
	0xF89A,
	0x100C,
	0xF8B0,
	0x01D8,
	0x4348,
	0x9002,
	0xF000,
	0xFBBA,
	0x2800,
	0x9802,
	0xD001,
	0x1A36,
	0xE000,
	0x4407,
	0x80A7,
	0x80E6,
	0x4860,
	0xF8B0,
	0x61DA,
	0xF890,
	0x028A,
	0x4346,
	0xF000,
	0xFBAF,
	0xB110,
	0xEBA8,
	0x0806,
	0xE000,
	0x44B1,
	0xF8A4,
	0x9008,
	0x4640,
	0x8160,
	0x9803,
	0xB128,
	0x4856,
	0xF890,
	0x114F,
	0xF890,
	0x0289,
	0xE003,
	0xF89A,
	0x100D,
	0xF89A,
	0x000C,
	0x010A,
	0x4951,
	0xF891,
	0x114E,
	0xEA42,
	0x2181,
	0xF041,
	0x0103,
	0x81A1,
	0x2101,
	0x22FF,
	0xEB02,
	0x0040,
	0xEA41,
	0x2000,
	0x81E0,
	0xA901,
	0x4668,
	0xF000,
	0xFB8B,
	0xF89D,
	0x0000,
	0xF89D,
	0x1004,
	0xEA40,
	0x2001,
	0x8220,
	0x465F,
	0x463E,
	0xF000,
	0xFB7B,
	0x1E79,
	0x2800,
	0x89A8,
	0xD004,
	0x1847,
	0xF646,
	0x10A4,
	0xE003,
	0xE03F,
	0x1846,
	0xF646,
	0x1024,
	0x80A8,
	0x8267,
	0x82E6,
	0x2000,
	0x82A0,
	0x88A8,
	0x8020,
	0xF000,
	0xFB70,
	0x2801,
	0xD10C,
	0xF000,
	0xFB71,
	0xB148,
	0xF000,
	0xFB73,
	0xB130,
	0xF240,
	0x4013,
	0x81A0,
	0xF240,
	0x1001,
	0x81E0,
	0x8220,
	0x6A2B,
	0x2100,
	0x2083,
	0x9A10,
	0xF000,
	0xFB6A,
	0x81E8,
	0xF000,
	0xFB58,
	0x2601,
	0x2801,
	0xD112,
	0xF000,
	0xFB58,
	0xB178,
	0xF000,
	0xFB5A,
	0xB160,
	0x8026,
	0x4830,
	0x2100,
	0xE004,
	0x8802,
	0x0852,
	0xF820,
	0x2B02,
	0x1C49,
	0x89EA,
	0xEBB1,
	0x0F42,
	0xDBF6,
	0x89E9,
	0x89A8,
	0x4281,
	0xD900,
	0x81E8,
	0x8026,
	0xB007,
	0xE8BD,
	0x8FF0,
	0xE92D,
	0x43F8,
	0x481A,
	0x2200,
	0x6940,
	0xB285,
	0xEA4F,
	0x4810,
	0x4629,
	0x4640,
	0xF000,
	0xFAFB,
	0xF000,
	0xFB3F,
	0x4F20,
	0xF897,
	0x0073,
	0xB130,
	0x4813,
	0xF890,
	0x028B,
	0xB110,
	0x491D,
	0x201B,
	0x8008,
	0x481C,
	0x4E0E,
	0x3634,
	0xF890,
	0x46C0,
	0x89B0,
	0xB998,
	0x2000,
	0xF8AD,
	0x0000,
	0x480A,
	0x2202,
	0x4669,
	0xF8B0,
	0x0600,
	0x302E,
	0xF000,
	0xFB27,
	0xB110,
	0xF8BD,
	0x0000,
	0x81B0,
	0x89B0,
	0xB910,
	0xF44F,
	0x6080,
	0x81B0,
	0xF897,
	0x0075,
	0xE01D,
	0x0020,
	0x404A,
	0x0020,
	0xD02E,
	0x0020,
	0x200E,
	0x0040,
	0x3288,
	0x0020,
	0x2034,
	0x0020,
	0xA021,
	0x0020,
	0x403F,
	0x0020,
	0x703E,
	0x0040,
	0x00A0,
	0x0020,
	0xC038,
	0x0020,
	0x1022,
	0x0020,
	0x0080,
	0x0020,
	0x5028,
	0x0040,
	0x7EF4,
	0x0020,
	0xE00F,
	0xB128,
	0x89B0,
	0xB118,
	0x4360,
	0xF500,
	0x7000,
	0x0A84,
	0x48FE,
	0xF44F,
	0x7280,
	0xF8B0,
	0x077C,
	0x4290,
	0xD901,
	0x4601,
	0xE000,
	0x4611,
	0x018B,
	0xF5A3,
	0x4380,
	0x4290,
	0xD901,
	0x4601,
	0xE000,
	0x4611,
	0xFB01,
	0x3104,
	0x23FF,
	0xEBB3,
	0x2F11,
	0xD90E,
	0x4290,
	0xD901,
	0x4601,
	0xE000,
	0x4611,
	0x0189,
	0xF5A1,
	0x4180,
	0x4290,
	0xD800,
	0x4610,
	0xFB00,
	0x1004,
	0x0A00,
	0xE000,
	0x20FF,
	0x49EB,
	0x8008,
	0x4629,
	0x4640,
	0xE8BD,
	0x43F8,
	0x2201,
	0xF000,
	0xBA7A,
	0xB570,
	0x48E7,
	0x2200,
	0x6981,
	0x0C0C,
	0xB28D,
	0x4629,
	0x4620,
	0xF000,
	0xFA70,
	0xF000,
	0xFABE,
	0x48E2,
	0xF890,
	0x1074,
	0xB111,
	0x2100,
	0xF880,
	0x1070,
	0x48E0,
	0xF44F,
	0x7180,
	0xF890,
	0x206F,
	0xF44F,
	0x4030,
	0xF000,
	0xFA5E,
	0x4629,
	0x4620,
	0xE8BD,
	0x4070,
	0x2201,
	0xF000,
	0xBA57,
	0xB570,
	0x4604,
	0x48D6,
	0x4DD7,
	0xF890,
	0x0408,
	0xB1C8,
	0x4628,
	0xF890,
	0x0609,
	0xB1A8,
	0x4628,
	0xF8D5,
	0x2384,
	0xF8C0,
	0x2414,
	0xF200,
	0x4114,
	0x462A,
	0xF8D5,
	0x0390,
	0xF8C2,
	0x0420,
	0xF8D5,
	0x43C0,
	0x4610,
	0xF8C5,
	0x42E4,
	0xF8C0,
	0x4430,
	0x4608,
	0xF000,
	0xFA8B,
	0x49C7,
	0xF8B5,
	0x22B0,
	0x8F08,
	0x8F49,
	0x1A20,
	0x1E40,
	0x4411,
	0x4281,
	0xD900,
	0x4608,
	0xF8A5,
	0x02B2,
	0xBD70,
	0xE92D,
	0x41F0,
	0x4606,
	0x48BD,
	0x2200,
	0x6A00,
	0xB285,
	0x0C04,
	0x4629,
	0x4620,
	0xF000,
	0xFA1C,
	0x4630,
	0xF000,
	0xFA73,
	0x48BB,
	0x4FBB,
	0x6800,
	0x683B,
	0x8B41,
	0x0A09,
	0xF883,
	0x1036,
	0x7EC1,
	0xF883,
	0x1038,
	0x49B4,
	0xF891,
	0x214C,
	0x2A00,
	0xF8D1,
	0x2134,
	0xD001,
	0x1C52,
	0x0852,
	0x33CE,
	0x0A16,
	0x711E,
	0x719A,
	0xF8B1,
	0x213C,
	0xF3C2,
	0x1257,
	0x701A,
	0xF891,
	0x213D,
	0x00D2,
	0x709A,
	0xF891,
	0x214D,
	0x3BCE,
	0xB122,
	0xF8D1,
	0x2138,
	0x1C52,
	0x0856,
	0xE001,
	0xF8D1,
	0x6138,
	0x687A,
	0xEA4F,
	0x2C16,
	0xF501,
	0x7190,
	0xF882,
	0xC016,
	0x7616,
	0x8BCE,
	0xF500,
	0x70BA,
	0xF3C6,
	0x1657,
	0x7496,
	0x7FCE,
	0x00F6,
	0x7516,
	0x8C0E,
	0x68CF,
	0x08F6,
	0x437E,
	0x0B36,
	0x0A37,
	0xF803,
	0x7FD6,
	0x3277,
	0x709E,
	0x8806,
	0x0A36,
	0xF802,
	0x6C27,
	0x7846,
	0xF802,
	0x6C25,
	0x8886,
	0x0A36,
	0xF802,
	0x6C1F,
	0x7946,
	0xF802,
	0x6C1D,
	0x4E8F,
	0xF896,
	0x6410,
	0x71D6,
	0x4E8D,
	0xF896,
	0x6411,
	0x7256,
	0x4E8B,
	0xF896,
	0x640B,
	0x72D6,
	0x4E89,
	0xF896,
	0x6409,
	0x7356,
	0xF890,
	0x6030,
	0x73D6,
	0xF890,
	0x00DE,
	0xF802,
	0x0F1F,
	0x4884,
	0xF200,
	0x4672,
	0xF890,
	0x0472,
	0x7490,
	0x7830,
	0x7510,
	0x22A5,
	0x70DA,
	0x200E,
	0x7118,
	0xF811,
	0x0C7E,
	0xF1C0,
	0x010C,
	0x487C,
	0xF8D0,
	0x043C,
	0x40C8,
	0x0A06,
	0x719E,
	0x7218,
	0x2001,
	0xF803,
	0x0C2C,
	0x4877,
	0xF8D0,
	0x044C,
	0x40C8,
	0x21AA,
	0xF803,
	0x1D57,
	0x2602,
	0x705E,
	0x709A,
	0x2230,
	0x70DA,
	0x225A,
	0x711A,
	0x0A06,
	0x715E,
	0x719A,
	0x71D8,
	0x7219,
	0x2000,
	0x7258,
	0x4629,
	0x4620,
	0xE8BD,
	0x41F0,
	0x2201,
	0xF000,
	0xB977,
	0xE92D,
	0x41F0,
	0x4607,
	0x4864,
	0x460C,
	0x2200,
	0x6A40,
	0xB286,
	0x0C05,
	0x4631,
	0x4628,
	0xF000,
	0xF96A,
	0x4621,
	0x4638,
	0xF000,
	0xF9C5,
	0x4860,
	0xF890,
	0x0297,
	0xB910,
	0xF000,
	0xF997,
	0xB120,
	0xF104,
	0x4480,
	0x8AA0,
	0x1C40,
	0x82A0,
	0x4631,
	0x4628,
	0xE8BD,
	0x41F0,
	0x2201,
	0xF000,
	0xB953,
	0xE92D,
	0x41F0,
	0x4607,
	0x4852,
	0x460E,
	0x2200,
	0x6A80,
	0xB285,
	0x0C04,
	0x4629,
	0x4620,
	0xF000,
	0xF946,
	0x4631,
	0x4638,
	0xF000,
	0xF9A6,
	0x4F4B,
	0xF24D,
	0x260C,
	0x3734,
	0xF44F,
	0x6180,
	0x783A,
	0x4630,
	0xF000,
	0xF938,
	0x7878,
	0xB3C8,
	0x2200,
	0xF44F,
	0x7100,
	0x4630,
	0xF000,
	0xF930,
	0x4848,
	0x8800,
	0x4B48,
	0xF8A3,
	0x0244,
	0x4846,
	0x1D00,
	0x8800,
	0xF8A3,
	0x0246,
	0xF8B3,
	0x0244,
	0xF8B3,
	0x1246,
	0x1842,
	0xD002,
	0x0280,
	0xFBB0,
	0xF2F2,
	0xB291,
	0x4A40,
	0xF8A3,
	0x1248,
	0x8850,
	0x8812,
	0x4B3D,
	0xF8A3,
	0x05A6,
	0xF8A3,
	0x25A8,
	0x1880,
	0xD005,
	0x0292,
	0xFBB2,
	0xF0F0,
	0x461A,
	0xF8A2,
	0x05AA,
	0x4836,
	0xF8B0,
	0x05AA,
	0x180A,
	0xFB01,
	0x2010,
	0xF340,
	0x1095,
	0x2810,
	0xDC06,
	0x2800,
	0xDA05,
	0x2000,
	0xE003,
	0xE7FF,
	0x2201,
	0xE7C3,
	0x2010,
	0x492F,
	0x8008,
	0x4629,
	0x4620,
	0xE8BD,
	0x41F0,
	0x2201,
	0xF000,
	0xB8EF,
	0xB570,
	0x4821,
	0x2200,
	0x6AC1,
	0x0C0C,
	0xB28D,
	0x4629,
	0x4620,
	0xF000,
	0xF8E5,
	0x4821,
	0x6802,
	0xF8B2,
	0x0262,
	0x0183,
	0xF892,
	0x0260,
	0xF010,
	0x0F02,
	0xD009,
	0x4818,
	0x3034,
	0x8881,
	0x4299,
	0xD806,
	0x8840,
	0xF5A0,
	0x4151,
	0x3923,
	0xD101,
	0xF000,
	0xF938,
	0x4629,
	0x4620,
	0xE8BD,
	0x4070,
	0x2201,
	0xF000,
	0xB8C8,
	0xB570,
	0x4606,
	0x480D,
	0x2200,
	0x6B01,
	0x0C0C,
	0xB28D,
	0x4629,
	0x4620,
	0xF000,
	0xF8BD,
	0x4630,
	0xF000,
	0xF928,
	0x4907,
	0x4A11,
	0x3134,
	0x79CB,
	0x68D0,
	0x4098,
	0x60D0,
	0x6810,
	0x4098,
	0x6010,
	0x6888,
	0xE019,
	0x0020,
	0xE00F,
	0x0040,
	0x74F4,
	0x0020,
	0x404A,
	0x0020,
	0x5028,
	0x0020,
	0x200E,
	0x0020,
	0xD02E,
	0x0020,
	0xD008,
	0x0020,
	0xE036,
	0x0040,
	0x0494,
	0x0020,
	0xC038,
	0x0040,
	0x14D2,
	0x0040,
	0x10A4,
	0x0020,
	0x5432,
	0x63D0,
	0x4629,
	0x4620,
	0xE8BD,
	0x4070,
	0x2201,
	0xF000,
	0xB88C,
	0xB510,
	0x2200,
	0xF6AF,
	0x0197,
	0x4833,
	0xF000,
	0xF8F8,
	0x4C33,
	0x2200,
	0xF6AF,
	0x013F,
	0x6020,
	0x4831,
	0xF000,
	0xF8F0,
	0x6060,
	0xF2AF,
	0x7049,
	0x492F,
	0x2200,
	0x61C8,
	0xF2AF,
	0x619F,
	0x482E,
	0xF000,
	0xF8E5,
	0x2200,
	0xF2AF,
	0x512D,
	0x6120,
	0x482B,
	0xF000,
	0xF8DE,
	0x2200,
	0xF2AF,
	0x4123,
	0x6160,
	0x4829,
	0xF000,
	0xF8D7,
	0x2200,
	0xF2AF,
	0x31E9,
	0x61A0,
	0x4826,
	0xF000,
	0xF8D0,
	0x2200,
	0xF2AF,
	0x716B,
	0x61E0,
	0x4824,
	0xF000,
	0xF8C9,
	0x2200,
	0xF2AF,
	0x7123,
	0x60A0,
	0x4821,
	0xF000,
	0xF8C2,
	0x2200,
	0xF2AF,
	0x31B7,
	0x60E0,
	0x481F,
	0xF000,
	0xF8BB,
	0x2200,
	0xF2AF,
	0x2161,
	0x6220,
	0x481C,
	0xF000,
	0xF8B4,
	0x6260,
	0x2000,
	0xF104,
	0x0134,
	0x4602,
	0x8188,
	0xF2AF,
	0x2131,
	0x4818,
	0xF000,
	0xF8A9,
	0x2200,
	0xF2AF,
	0x1175,
	0x62A0,
	0x4815,
	0xF000,
	0xF8A2,
	0x2200,
	0xF2AF,
	0x1137,
	0x62E0,
	0x4813,
	0xF000,
	0xF89B,
	0x4912,
	0x6320,
	0xF640,
	0x00F1,
	0x6809,
	0x8348,
	0xBD10,
	0x0000,
	0x0000,
	0x1FDE,
	0x0020,
	0x404A,
	0x0000,
	0x3B5F,
	0x0020,
	0x5008,
	0x0000,
	0x19D7,
	0x0000,
	0xFF27,
	0x0000,
	0xE339,
	0x0100,
	0xCF32,
	0x0100,
	0x3B1E,
	0x0000,
	0x45EC,
	0x0000,
	0xB967,
	0x0000,
	0x2BE6,
	0x0100,
	0x6522,
	0x0000,
	0x838C,
	0x0000,
	0x4954,
	0x0020,
	0xD008,
	0xF24A,
	0x1C2B,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF64D,
	0x6C1F,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF644,
	0x5C65,
	0xF2C0,
	0x0C01,
	0x4760,
	0xF645,
	0x3C43,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF645,
	0x6CE3,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF246,
	0x1C7B,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF644,
	0x0CD9,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF644,
	0x1C79,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF644,
	0x1C81,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF244,
	0x0CB5,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF644,
	0x0CE9,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF643,
	0x5C15,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF643,
	0x5C1D,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF24D,
	0x5CC9,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF242,
	0x7CFF,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF248,
	0x2C71,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF643,
	0x1CE3,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF243,
	0x4C37,
	0xF2C0,
	0x0C01,
	0x4760,
	0xF246,
	0x7CB9,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF24E,
	0x6C2B,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF242,
	0x2C65,
	0xF2C0,
	0x0C01,
	0x4760,
	0xF648,
	0x4C83,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF245,
	0x4C49,
	0xF2C0,
	0x0C00,
	0x4760,
	0xF24C,
	0x1C2D,
	0xF2C0,
	0x0C00,
	0x4760,
};
#endif

#if 0
/*xxxx  add for debug after burst mode*/
static void burst_read_to_check(void)
{
	u8 addr_0x6F12[2] = {0x6F,0x12};
	kal_uint16 get_byte = 0;
	int i;
	write_cmos_sensor(0x6028, 0x4000); ////tnp start
	write_cmos_sensor(0x6004, 0x0001);

	write_cmos_sensor(0x602C, 0x2000); ////tnp start
	write_cmos_sensor(0x602E, 0x3F4C);


	for(i = 0; i < (u16)sizeof(uTnpArrayA); i++)
	{
	  iReadRegI2C(addr_0x6F12, 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
	  LOG_INF("Burst read: 0x%04X\n",((get_byte << 8) & 0xff00)| ((get_byte >> 8) & 0x00ff));
	}

	write_cmos_sensor(0x6028, 0x4000); ////tnp start
	write_cmos_sensor(0x6004, 0x0000);
}

static void burst_read_to_check2(void)
{
	u8 addr_0x6F12[2] = {0x6F,0x12};
	kal_uint16 get_byte = 0;
	int i;

	for(i = 0; i < 20; i+=2)
	{
		write_cmos_sensor(0x602C, 0x2000); ////tnp start
		write_cmos_sensor(0x602E, 0x4A20+i);
		iReadRegI2C(addr_0x6F12, 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
		LOG_INF("Normal read2: 0x%04X\n",((get_byte << 8) & 0xff00)| ((get_byte >> 8) & 0x00ff));
    }
}
#endif

	static void sensor_init(void)
	{
		LOG_INF("E\n");
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x6010, 0x0001);
		mdelay(3);         // Wait value must be at least 20000 MCLKs
		write_cmos_sensor(0x6214,0x7970);
		write_cmos_sensor(0x6218,0x7150);
		//Tnp ,sw
		//TnP SVN16396	
		write_cmos_sensor(0x6028, 0x0000);
		write_cmos_sensor(0x602A, 0x0000);
		write_cmos_sensor(0x6F12, 0x0649);
		write_cmos_sensor(0x6F12, 0x0548);
		write_cmos_sensor(0x6F12, 0xC1F8);
		write_cmos_sensor(0x6F12, 0xC405);
		write_cmos_sensor(0x6F12, 0x0549);
		write_cmos_sensor(0x6F12, 0x081A);
		write_cmos_sensor(0x6F12, 0x0349);
		write_cmos_sensor(0x6F12, 0xA1F8);
		write_cmos_sensor(0x6F12, 0xC805);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD7BB);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x4930);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2ED0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x6C00);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x40BA);
		write_cmos_sensor(0x6F12, 0x7047);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0xC0BA);
		write_cmos_sensor(0x6F12, 0x7047);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF047);
		write_cmos_sensor(0x6F12, 0x1C46);
		write_cmos_sensor(0x6F12, 0x9046);
		write_cmos_sensor(0x6F12, 0x8946);
		write_cmos_sensor(0x6F12, 0x0746);
		write_cmos_sensor(0x6F12, 0xFE48);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x4068);
		write_cmos_sensor(0x6F12, 0x86B2);
		write_cmos_sensor(0x6F12, 0x050C);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x37FC);
		write_cmos_sensor(0x6F12, 0x2346);
		write_cmos_sensor(0x6F12, 0x4246);
		write_cmos_sensor(0x6F12, 0x4946);
		write_cmos_sensor(0x6F12, 0x3846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x36FC);
		write_cmos_sensor(0x6F12, 0xF848);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x8B02);
		write_cmos_sensor(0x6F12, 0x88B1);
		write_cmos_sensor(0x6F12, 0x788A);
		write_cmos_sensor(0x6F12, 0x04F1);
		write_cmos_sensor(0x6F12, 0x0054);
		write_cmos_sensor(0x6F12, 0x04EB);
		write_cmos_sensor(0x6F12, 0x8001);
		write_cmos_sensor(0x6F12, 0x09E0);
		write_cmos_sensor(0x6F12, 0x2268);
		write_cmos_sensor(0x6F12, 0xC2F3);
		write_cmos_sensor(0x6F12, 0xC360);
		write_cmos_sensor(0x6F12, 0x90FA);
		write_cmos_sensor(0x6F12, 0xA0F0);
		write_cmos_sensor(0x6F12, 0x22F0);
		write_cmos_sensor(0x6F12, 0x7842);
		write_cmos_sensor(0x6F12, 0x42EA);
		write_cmos_sensor(0x6F12, 0x5000);
		write_cmos_sensor(0x6F12, 0x01C4);
		write_cmos_sensor(0x6F12, 0x8C42);
		write_cmos_sensor(0x6F12, 0xF3D1);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF047);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x14BC);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xFC5F);
		write_cmos_sensor(0x6F12, 0x8346);
		write_cmos_sensor(0x6F12, 0xE748);
		write_cmos_sensor(0x6F12, 0x8A46);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x8068);
		write_cmos_sensor(0x6F12, 0x010C);
		write_cmos_sensor(0x6F12, 0x80B2);
		write_cmos_sensor(0x6F12, 0xCDE9);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x0198);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x05FC);
		write_cmos_sensor(0x6F12, 0xABFB);
		write_cmos_sensor(0x6F12, 0x0A10);
		write_cmos_sensor(0x6F12, 0xE24B);
		write_cmos_sensor(0x6F12, 0xE04D);
		write_cmos_sensor(0x6F12, 0xE04A);
		write_cmos_sensor(0x6F12, 0x93F8);
		write_cmos_sensor(0x6F12, 0x9160);
		write_cmos_sensor(0x6F12, 0x05F5);
		write_cmos_sensor(0x6F12, 0xAA69);
		write_cmos_sensor(0x6F12, 0x06FB);
		write_cmos_sensor(0x6F12, 0x0BF6);
		write_cmos_sensor(0x6F12, 0x0023);
		write_cmos_sensor(0x6F12, 0x891B);
		write_cmos_sensor(0x6F12, 0x4D46);
		write_cmos_sensor(0x6F12, 0x60EB);
		write_cmos_sensor(0x6F12, 0x0300);
		write_cmos_sensor(0x6F12, 0x03C5);
		write_cmos_sensor(0x6F12, 0x1D46);
		write_cmos_sensor(0x6F12, 0xEBFB);
		write_cmos_sensor(0x6F12, 0x0A65);
		write_cmos_sensor(0x6F12, 0x02F5);
		write_cmos_sensor(0x6F12, 0xAB67);
		write_cmos_sensor(0x6F12, 0x3A46);
		write_cmos_sensor(0x6F12, 0xD64C);
		write_cmos_sensor(0x6F12, 0x60C2);
		write_cmos_sensor(0x6F12, 0xA4F8);
		write_cmos_sensor(0x6F12, 0x4835);
		write_cmos_sensor(0x6F12, 0x04F5);
		write_cmos_sensor(0x6F12, 0xA962);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0xA0C4);
		write_cmos_sensor(0x6F12, 0x94F8);
		write_cmos_sensor(0x6F12, 0xA144);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0xF858);
		write_cmos_sensor(0x6F12, 0xBCF1);
		write_cmos_sensor(0x6F12, 0x010F);
		write_cmos_sensor(0x6F12, 0x03D0);
		write_cmos_sensor(0x6F12, 0xBCF1);
		write_cmos_sensor(0x6F12, 0x020F);
		write_cmos_sensor(0x6F12, 0x14D0);
		write_cmos_sensor(0x6F12, 0x29E0);
		write_cmos_sensor(0x6F12, 0x08EA);
		write_cmos_sensor(0x6F12, 0x0423);
		write_cmos_sensor(0x6F12, 0x43F0);
		write_cmos_sensor(0x6F12, 0x1103);
		write_cmos_sensor(0x6F12, 0x1380);
		write_cmos_sensor(0x6F12, 0x2346);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xDCFB);
		write_cmos_sensor(0x6F12, 0xC9E9);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x2346);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD4FB);
		write_cmos_sensor(0x6F12, 0xC7E9);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x15E0);
		write_cmos_sensor(0x6F12, 0x08EA);
		write_cmos_sensor(0x6F12, 0x042C);
		write_cmos_sensor(0x6F12, 0x4CF0);
		write_cmos_sensor(0x6F12, 0x010C);
		write_cmos_sensor(0x6F12, 0xA2F8);
		write_cmos_sensor(0x6F12, 0x00C0);
		write_cmos_sensor(0x6F12, 0xA1FB);
		write_cmos_sensor(0x6F12, 0x042C);
		write_cmos_sensor(0x6F12, 0x00FB);
		write_cmos_sensor(0x6F12, 0x04C0);
		write_cmos_sensor(0x6F12, 0x01FB);
		write_cmos_sensor(0x6F12, 0x0301);
		write_cmos_sensor(0x6F12, 0xC9E9);
		write_cmos_sensor(0x6F12, 0x0012);
		write_cmos_sensor(0x6F12, 0xA6FB);
		write_cmos_sensor(0x6F12, 0x0401);
		write_cmos_sensor(0x6F12, 0x05FB);
		write_cmos_sensor(0x6F12, 0x0411);
		write_cmos_sensor(0x6F12, 0x06FB);
		write_cmos_sensor(0x6F12, 0x0311);
		write_cmos_sensor(0x6F12, 0xC7E9);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6F12, 0xB848);
		write_cmos_sensor(0x6F12, 0xB949);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x4805);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0xB848);
		write_cmos_sensor(0x6F12, 0x0CC8);
		write_cmos_sensor(0x6F12, 0x48F6);
		write_cmos_sensor(0x6F12, 0x2200);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xB5FB);
		write_cmos_sensor(0x6F12, 0xB548);
		write_cmos_sensor(0x6F12, 0x0830);
		write_cmos_sensor(0x6F12, 0x0CC8);
		write_cmos_sensor(0x6F12, 0x48F6);
		write_cmos_sensor(0x6F12, 0x2A00);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xAEFB);
		write_cmos_sensor(0x6F12, 0x5846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xB0FB);
		write_cmos_sensor(0x6F12, 0xAD49);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0xC1F8);
		write_cmos_sensor(0x6F12, 0x68A5);
		write_cmos_sensor(0x6F12, 0xDDE9);
		write_cmos_sensor(0x6F12, 0x0010);
		write_cmos_sensor(0x6F12, 0x02B0);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF05F);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x91BB);
		write_cmos_sensor(0x6F12, 0xA84A);
		write_cmos_sensor(0x6F12, 0x92F8);
		write_cmos_sensor(0x6F12, 0xD525);
		write_cmos_sensor(0x6F12, 0x2AB1);
		write_cmos_sensor(0x6F12, 0xA64A);
		write_cmos_sensor(0x6F12, 0xA54B);
		write_cmos_sensor(0x6F12, 0xD2F8);
		write_cmos_sensor(0x6F12, 0x6825);
		write_cmos_sensor(0x6F12, 0xC3F8);
		write_cmos_sensor(0x6F12, 0x3024);
		write_cmos_sensor(0x6F12, 0xA34A);
		write_cmos_sensor(0x6F12, 0xD2F8);
		write_cmos_sensor(0x6F12, 0x3024);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x9BBB);
		write_cmos_sensor(0x6F12, 0x10B5);
		write_cmos_sensor(0x6F12, 0xA049);
		write_cmos_sensor(0x6F12, 0xA34A);
		write_cmos_sensor(0x6F12, 0xA44B);
		write_cmos_sensor(0x6F12, 0xD1F8);
		write_cmos_sensor(0x6F12, 0x3C14);
		write_cmos_sensor(0x6F12, 0x947C);
		write_cmos_sensor(0x6F12, 0x0CB1);
		write_cmos_sensor(0x6F12, 0x908A);
		write_cmos_sensor(0x6F12, 0x1BE0);
		write_cmos_sensor(0x6F12, 0x9B4A);
		write_cmos_sensor(0x6F12, 0x92F8);
		write_cmos_sensor(0x6F12, 0xA220);
		write_cmos_sensor(0x6F12, 0xC2F1);
		write_cmos_sensor(0x6F12, 0x0C02);
		write_cmos_sensor(0x6F12, 0xD140);
		write_cmos_sensor(0x6F12, 0x4843);
		write_cmos_sensor(0x6F12, 0x010A);
		write_cmos_sensor(0x6F12, 0x9D48);
		write_cmos_sensor(0x6F12, 0xD0F8);
		write_cmos_sensor(0x6F12, 0x8400);
		write_cmos_sensor(0x6F12, 0x0279);
		write_cmos_sensor(0x6F12, 0x4A43);
		write_cmos_sensor(0x6F12, 0x4179);
		write_cmos_sensor(0x6F12, 0xC088);
		write_cmos_sensor(0x6F12, 0xCA40);
		write_cmos_sensor(0x6F12, 0x00EB);
		write_cmos_sensor(0x6F12, 0x1210);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x8021);
		write_cmos_sensor(0x6F12, 0xB1FB);
		write_cmos_sensor(0x6F12, 0xF0F0);
		write_cmos_sensor(0x6F12, 0x0911);
		write_cmos_sensor(0x6F12, 0x8842);
		write_cmos_sensor(0x6F12, 0x04D2);
		write_cmos_sensor(0x6F12, 0x4028);
		write_cmos_sensor(0x6F12, 0x00D8);
		write_cmos_sensor(0x6F12, 0x4020);
		write_cmos_sensor(0x6F12, 0x5880);
		write_cmos_sensor(0x6F12, 0x10BD);
		write_cmos_sensor(0x6F12, 0x0846);
		write_cmos_sensor(0x6F12, 0xFBE7);
		write_cmos_sensor(0x6F12, 0x4168);
		write_cmos_sensor(0x6F12, 0x4A7B);
		write_cmos_sensor(0x6F12, 0x9149);
		write_cmos_sensor(0x6F12, 0xA1F8);
		write_cmos_sensor(0x6F12, 0x8223);
		write_cmos_sensor(0x6F12, 0x4268);
		write_cmos_sensor(0x6F12, 0x537B);
		write_cmos_sensor(0x6F12, 0x002B);
		write_cmos_sensor(0x6F12, 0x15D0);
		write_cmos_sensor(0x6F12, 0x01F5);
		write_cmos_sensor(0x6F12, 0x6171);
		write_cmos_sensor(0x6F12, 0x927B);
		write_cmos_sensor(0x6F12, 0x0A80);
		write_cmos_sensor(0x6F12, 0x4068);
		write_cmos_sensor(0x6F12, 0xC07B);
		write_cmos_sensor(0x6F12, 0x4880);
		write_cmos_sensor(0x6F12, 0x8B48);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0xC220);
		write_cmos_sensor(0x6F12, 0x8A80);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0xC420);
		write_cmos_sensor(0x6F12, 0xCA80);
		write_cmos_sensor(0x6F12, 0x10F8);
		write_cmos_sensor(0x6F12, 0xC72F);
		write_cmos_sensor(0x6F12, 0xC078);
		write_cmos_sensor(0x6F12, 0x5208);
		write_cmos_sensor(0x6F12, 0x4008);
		write_cmos_sensor(0x6F12, 0x42EA);
		write_cmos_sensor(0x6F12, 0x8000);
		write_cmos_sensor(0x6F12, 0x0881);
		write_cmos_sensor(0x6F12, 0x7047);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xFF4F);
		write_cmos_sensor(0x6F12, 0x8348);
		write_cmos_sensor(0x6F12, 0x83B0);
		write_cmos_sensor(0x6F12, 0x1D46);
		write_cmos_sensor(0x6F12, 0xC079);
		write_cmos_sensor(0x6F12, 0xDDF8);
		write_cmos_sensor(0x6F12, 0x44B0);
		write_cmos_sensor(0x6F12, 0x1646);
		write_cmos_sensor(0x6F12, 0x0F46);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x6ED0);
		write_cmos_sensor(0x6F12, 0xDFF8);
		write_cmos_sensor(0x6F12, 0xF4A1);
		write_cmos_sensor(0x6F12, 0x0AF1);
		write_cmos_sensor(0x6F12, 0xBA0A);
		write_cmos_sensor(0x6F12, 0xAAF1);
		write_cmos_sensor(0x6F12, 0x1C00);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x0090);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x0480);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x3EFB);
		write_cmos_sensor(0x6F12, 0x0399);
		write_cmos_sensor(0x6F12, 0x109C);
		write_cmos_sensor(0x6F12, 0x0843);
		write_cmos_sensor(0x6F12, 0x04F1);
		write_cmos_sensor(0x6F12, 0x8044);
		write_cmos_sensor(0x6F12, 0x07D0);
		write_cmos_sensor(0x6F12, 0xA780);
		write_cmos_sensor(0x6F12, 0xE680);
		write_cmos_sensor(0x6F12, 0xAAF1);
		write_cmos_sensor(0x6F12, 0x1C00);
		write_cmos_sensor(0x6F12, 0x0188);
		write_cmos_sensor(0x6F12, 0x2181);
		write_cmos_sensor(0x6F12, 0x8088);
		write_cmos_sensor(0x6F12, 0x20E0);
		write_cmos_sensor(0x6F12, 0x6848);
		write_cmos_sensor(0x6F12, 0x9AF8);
		write_cmos_sensor(0x6F12, 0x0C10);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0xD801);
		write_cmos_sensor(0x6F12, 0x4843);
		write_cmos_sensor(0x6F12, 0x0290);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x2CFB);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x0298);
		write_cmos_sensor(0x6F12, 0x01D0);
		write_cmos_sensor(0x6F12, 0x361A);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x0744);
		write_cmos_sensor(0x6F12, 0xA780);
		write_cmos_sensor(0x6F12, 0xE680);
		write_cmos_sensor(0x6F12, 0x6048);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0xDA61);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x8A02);
		write_cmos_sensor(0x6F12, 0x4643);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x21FB);
		write_cmos_sensor(0x6F12, 0x10B1);
		write_cmos_sensor(0x6F12, 0xA8EB);
		write_cmos_sensor(0x6F12, 0x0608);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0xB144);
		write_cmos_sensor(0x6F12, 0xA4F8);
		write_cmos_sensor(0x6F12, 0x0890);
		write_cmos_sensor(0x6F12, 0x4046);
		write_cmos_sensor(0x6F12, 0x6081);
		write_cmos_sensor(0x6F12, 0x0398);
		write_cmos_sensor(0x6F12, 0x28B1);
		write_cmos_sensor(0x6F12, 0x5648);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x4F11);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x8902);
		write_cmos_sensor(0x6F12, 0x03E0);
		write_cmos_sensor(0x6F12, 0x9AF8);
		write_cmos_sensor(0x6F12, 0x0D10);
		write_cmos_sensor(0x6F12, 0x9AF8);
		write_cmos_sensor(0x6F12, 0x0C00);
		write_cmos_sensor(0x6F12, 0x0A01);
		write_cmos_sensor(0x6F12, 0x5149);
		write_cmos_sensor(0x6F12, 0x91F8);
		write_cmos_sensor(0x6F12, 0x4E11);
		write_cmos_sensor(0x6F12, 0x42EA);
		write_cmos_sensor(0x6F12, 0x8121);
		write_cmos_sensor(0x6F12, 0x41F0);
		write_cmos_sensor(0x6F12, 0x0301);
		write_cmos_sensor(0x6F12, 0xA181);
		write_cmos_sensor(0x6F12, 0x0121);
		write_cmos_sensor(0x6F12, 0xFF22);
		write_cmos_sensor(0x6F12, 0x02EB);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x41EA);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0xE081);
		write_cmos_sensor(0x6F12, 0x01A9);
		write_cmos_sensor(0x6F12, 0x6846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xFDFA);
		write_cmos_sensor(0x6F12, 0x9DF8);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x9DF8);
		write_cmos_sensor(0x6F12, 0x0410);
		write_cmos_sensor(0x6F12, 0x40EA);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0x2082);
		write_cmos_sensor(0x6F12, 0x5F46);
		write_cmos_sensor(0x6F12, 0x3E46);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xEDFA);
		write_cmos_sensor(0x6F12, 0x791E);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0xA889);
		write_cmos_sensor(0x6F12, 0x04D0);
		write_cmos_sensor(0x6F12, 0x4718);
		write_cmos_sensor(0x6F12, 0x46F6);
		write_cmos_sensor(0x6F12, 0xA410);
		write_cmos_sensor(0x6F12, 0x03E0);
		write_cmos_sensor(0x6F12, 0x3FE0);
		write_cmos_sensor(0x6F12, 0x4618);
		write_cmos_sensor(0x6F12, 0x46F6);
		write_cmos_sensor(0x6F12, 0x2410);
		write_cmos_sensor(0x6F12, 0xA880);
		write_cmos_sensor(0x6F12, 0x6782);
		write_cmos_sensor(0x6F12, 0xE682);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0xA082);
		write_cmos_sensor(0x6F12, 0xA888);
		write_cmos_sensor(0x6F12, 0x2080);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xE2FA);
		write_cmos_sensor(0x6F12, 0x0128);
		write_cmos_sensor(0x6F12, 0x0CD1);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xE3FA);
		write_cmos_sensor(0x6F12, 0x48B1);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xE5FA);
		write_cmos_sensor(0x6F12, 0x30B1);
		write_cmos_sensor(0x6F12, 0x40F2);
		write_cmos_sensor(0x6F12, 0x1340);
		write_cmos_sensor(0x6F12, 0xA081);
		write_cmos_sensor(0x6F12, 0x40F2);
		write_cmos_sensor(0x6F12, 0x0110);
		write_cmos_sensor(0x6F12, 0xE081);
		write_cmos_sensor(0x6F12, 0x2082);
		write_cmos_sensor(0x6F12, 0x2B6A);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0x8320);
		write_cmos_sensor(0x6F12, 0x109A);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xDCFA);
		write_cmos_sensor(0x6F12, 0xE881);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xCAFA);
		write_cmos_sensor(0x6F12, 0x0126);
		write_cmos_sensor(0x6F12, 0x0128);
		write_cmos_sensor(0x6F12, 0x12D1);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xCAFA);
		write_cmos_sensor(0x6F12, 0x78B1);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xCCFA);
		write_cmos_sensor(0x6F12, 0x60B1);
		write_cmos_sensor(0x6F12, 0x2680);
		write_cmos_sensor(0x6F12, 0x3048);
		write_cmos_sensor(0x6F12, 0x0021);
		write_cmos_sensor(0x6F12, 0x04E0);
		write_cmos_sensor(0x6F12, 0x0288);
		write_cmos_sensor(0x6F12, 0x5208);
		write_cmos_sensor(0x6F12, 0x20F8);
		write_cmos_sensor(0x6F12, 0x022B);
		write_cmos_sensor(0x6F12, 0x491C);
		write_cmos_sensor(0x6F12, 0xEA89);
		write_cmos_sensor(0x6F12, 0xB1EB);
		write_cmos_sensor(0x6F12, 0x420F);
		write_cmos_sensor(0x6F12, 0xF6DB);
		write_cmos_sensor(0x6F12, 0xE989);
		write_cmos_sensor(0x6F12, 0xA889);
		write_cmos_sensor(0x6F12, 0x8142);
		write_cmos_sensor(0x6F12, 0x00D9);
		write_cmos_sensor(0x6F12, 0xE881);
		write_cmos_sensor(0x6F12, 0x2680);
		write_cmos_sensor(0x6F12, 0x07B0);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF08F);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF843);
		write_cmos_sensor(0x6F12, 0x1A48);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x8069);
		write_cmos_sensor(0x6F12, 0x85B2);
		write_cmos_sensor(0x6F12, 0x4FEA);
		write_cmos_sensor(0x6F12, 0x1048);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x4046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x6DFA);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xB1FA);
		write_cmos_sensor(0x6F12, 0x204F);
		write_cmos_sensor(0x6F12, 0x97F8);
		write_cmos_sensor(0x6F12, 0x7300);
		write_cmos_sensor(0x6F12, 0x30B1);
		write_cmos_sensor(0x6F12, 0x1348);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x8B02);
		write_cmos_sensor(0x6F12, 0x10B1);
		write_cmos_sensor(0x6F12, 0x1D49);
		write_cmos_sensor(0x6F12, 0x1B20);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x1C48);
		write_cmos_sensor(0x6F12, 0x0E4E);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0xC046);
		write_cmos_sensor(0x6F12, 0x7088);
		write_cmos_sensor(0x6F12, 0x98B9);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0xADF8);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0B48);
		write_cmos_sensor(0x6F12, 0x0222);
		write_cmos_sensor(0x6F12, 0x6946);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x0006);
		write_cmos_sensor(0x6F12, 0x2E30);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x9AFA);
		write_cmos_sensor(0x6F12, 0x10B1);
		write_cmos_sensor(0x6F12, 0xBDF8);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x7080);
		write_cmos_sensor(0x6F12, 0x7088);
		write_cmos_sensor(0x6F12, 0x10B9);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x8060);
		write_cmos_sensor(0x6F12, 0x7080);
		write_cmos_sensor(0x6F12, 0x97F8);
		write_cmos_sensor(0x6F12, 0x7500);
		write_cmos_sensor(0x6F12, 0x20B3);
		write_cmos_sensor(0x6F12, 0x1DE0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x4900);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2ED0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0E20);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x8832);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x3420);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x21A0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x3F40);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x3E70);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0xA000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x38C0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2210);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x8000);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2850);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0xF47E);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0FE0);
		write_cmos_sensor(0x6F12, 0x7088);
		write_cmos_sensor(0x6F12, 0x18B1);
		write_cmos_sensor(0x6F12, 0x6043);
		write_cmos_sensor(0x6F12, 0x00F5);
		write_cmos_sensor(0x6F12, 0x0070);
		write_cmos_sensor(0x6F12, 0x840A);
		write_cmos_sensor(0x6F12, 0xF648);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x8072);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0x7C07);
		write_cmos_sensor(0x6F12, 0x9042);
		write_cmos_sensor(0x6F12, 0x01D9);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x1146);
		write_cmos_sensor(0x6F12, 0x8B01);
		write_cmos_sensor(0x6F12, 0xA3F5);
		write_cmos_sensor(0x6F12, 0x8043);
		write_cmos_sensor(0x6F12, 0x9042);
		write_cmos_sensor(0x6F12, 0x01D9);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x1146);
		write_cmos_sensor(0x6F12, 0x01FB);
		write_cmos_sensor(0x6F12, 0x0431);
		write_cmos_sensor(0x6F12, 0xFF23);
		write_cmos_sensor(0x6F12, 0xB3EB);
		write_cmos_sensor(0x6F12, 0x112F);
		write_cmos_sensor(0x6F12, 0x0ED9);
		write_cmos_sensor(0x6F12, 0x9042);
		write_cmos_sensor(0x6F12, 0x01D9);
		write_cmos_sensor(0x6F12, 0x0146);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0x1146);
		write_cmos_sensor(0x6F12, 0x8901);
		write_cmos_sensor(0x6F12, 0xA1F5);
		write_cmos_sensor(0x6F12, 0x8041);
		write_cmos_sensor(0x6F12, 0x9042);
		write_cmos_sensor(0x6F12, 0x00D8);
		write_cmos_sensor(0x6F12, 0x1046);
		write_cmos_sensor(0x6F12, 0x00FB);
		write_cmos_sensor(0x6F12, 0x0410);
		write_cmos_sensor(0x6F12, 0x000A);
		write_cmos_sensor(0x6F12, 0x00E0);
		write_cmos_sensor(0x6F12, 0xFF20);
		write_cmos_sensor(0x6F12, 0xE349);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x4046);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF843);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xEDB9);
		write_cmos_sensor(0x6F12, 0x70B5);
		write_cmos_sensor(0x6F12, 0xDF48);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xC169);
		write_cmos_sensor(0x6F12, 0x0C0C);
		write_cmos_sensor(0x6F12, 0x8DB2);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xE3F9);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x31FA);
		write_cmos_sensor(0x6F12, 0xDB4A);
		write_cmos_sensor(0x6F12, 0x92F8);
		write_cmos_sensor(0x6F12, 0x7400);
		write_cmos_sensor(0x6F12, 0x10B1);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x82F8);
		write_cmos_sensor(0x6F12, 0x7000);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0x7040);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD3B9);
		write_cmos_sensor(0x6F12, 0xD549);
		write_cmos_sensor(0x6F12, 0x0A8F);
		write_cmos_sensor(0x6F12, 0x498F);
		write_cmos_sensor(0x6F12, 0x801A);
		write_cmos_sensor(0x6F12, 0xD44A);
		write_cmos_sensor(0x6F12, 0x401E);
		write_cmos_sensor(0x6F12, 0xB2F8);
		write_cmos_sensor(0x6F12, 0xB032);
		write_cmos_sensor(0x6F12, 0x1944);
		write_cmos_sensor(0x6F12, 0x8142);
		write_cmos_sensor(0x6F12, 0x00D9);
		write_cmos_sensor(0x6F12, 0x0846);
		write_cmos_sensor(0x6F12, 0xA2F8);
		write_cmos_sensor(0x6F12, 0xB202);
		write_cmos_sensor(0x6F12, 0x7047);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x0646);
		write_cmos_sensor(0x6F12, 0xCA48);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x406A);
		write_cmos_sensor(0x6F12, 0x85B2);
		write_cmos_sensor(0x6F12, 0x040C);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xB8F9);
		write_cmos_sensor(0x6F12, 0x3046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x0AFA);
		write_cmos_sensor(0x6F12, 0xC848);
		write_cmos_sensor(0x6F12, 0xC84F);
		write_cmos_sensor(0x6F12, 0x0068);
		write_cmos_sensor(0x6F12, 0x3B68);
		write_cmos_sensor(0x6F12, 0x418B);
		write_cmos_sensor(0x6F12, 0x090A);
		write_cmos_sensor(0x6F12, 0x83F8);
		write_cmos_sensor(0x6F12, 0x3610);
		write_cmos_sensor(0x6F12, 0xC17E);
		write_cmos_sensor(0x6F12, 0x83F8);
		write_cmos_sensor(0x6F12, 0x3810);
		write_cmos_sensor(0x6F12, 0xC149);
		write_cmos_sensor(0x6F12, 0x91F8);
		write_cmos_sensor(0x6F12, 0x4C21);
		write_cmos_sensor(0x6F12, 0x002A);
		write_cmos_sensor(0x6F12, 0xD1F8);
		write_cmos_sensor(0x6F12, 0x3421);
		write_cmos_sensor(0x6F12, 0x01D0);
		write_cmos_sensor(0x6F12, 0x521C);
		write_cmos_sensor(0x6F12, 0x5208);
		write_cmos_sensor(0x6F12, 0xCE33);
		write_cmos_sensor(0x6F12, 0x160A);
		write_cmos_sensor(0x6F12, 0x1E71);
		write_cmos_sensor(0x6F12, 0x9A71);
		write_cmos_sensor(0x6F12, 0xB1F8);
		write_cmos_sensor(0x6F12, 0x3C21);
		write_cmos_sensor(0x6F12, 0xC2F3);
		write_cmos_sensor(0x6F12, 0x5712);
		write_cmos_sensor(0x6F12, 0x1A70);
		write_cmos_sensor(0x6F12, 0x91F8);
		write_cmos_sensor(0x6F12, 0x3D21);
		write_cmos_sensor(0x6F12, 0xD200);
		write_cmos_sensor(0x6F12, 0x9A70);
		write_cmos_sensor(0x6F12, 0x91F8);
		write_cmos_sensor(0x6F12, 0x4D21);
		write_cmos_sensor(0x6F12, 0xCE3B);
		write_cmos_sensor(0x6F12, 0x22B1);
		write_cmos_sensor(0x6F12, 0xD1F8);
		write_cmos_sensor(0x6F12, 0x3821);
		write_cmos_sensor(0x6F12, 0x521C);
		write_cmos_sensor(0x6F12, 0x5608);
		write_cmos_sensor(0x6F12, 0x01E0);
		write_cmos_sensor(0x6F12, 0xD1F8);
		write_cmos_sensor(0x6F12, 0x3861);
		write_cmos_sensor(0x6F12, 0x7A68);
		write_cmos_sensor(0x6F12, 0x4FEA);
		write_cmos_sensor(0x6F12, 0x162C);
		write_cmos_sensor(0x6F12, 0x01F5);
		write_cmos_sensor(0x6F12, 0x9071);
		write_cmos_sensor(0x6F12, 0x82F8);
		write_cmos_sensor(0x6F12, 0x16C0);
		write_cmos_sensor(0x6F12, 0x1676);
		write_cmos_sensor(0x6F12, 0xCE8B);
		write_cmos_sensor(0x6F12, 0x00F5);
		write_cmos_sensor(0x6F12, 0xBA70);
		write_cmos_sensor(0x6F12, 0xC6F3);
		write_cmos_sensor(0x6F12, 0x5716);
		write_cmos_sensor(0x6F12, 0x9674);
		write_cmos_sensor(0x6F12, 0xCE7F);
		write_cmos_sensor(0x6F12, 0xF600);
		write_cmos_sensor(0x6F12, 0x1675);
		write_cmos_sensor(0x6F12, 0x0E8C);
		write_cmos_sensor(0x6F12, 0xCF68);
		write_cmos_sensor(0x6F12, 0xF608);
		write_cmos_sensor(0x6F12, 0x7E43);
		write_cmos_sensor(0x6F12, 0x360B);
		write_cmos_sensor(0x6F12, 0x370A);
		write_cmos_sensor(0x6F12, 0x03F8);
		write_cmos_sensor(0x6F12, 0xD67F);
		write_cmos_sensor(0x6F12, 0x7732);
		write_cmos_sensor(0x6F12, 0x9E70);
		write_cmos_sensor(0x6F12, 0x0688);
		write_cmos_sensor(0x6F12, 0x360A);
		write_cmos_sensor(0x6F12, 0x02F8);
		write_cmos_sensor(0x6F12, 0x276C);
		write_cmos_sensor(0x6F12, 0x4678);
		write_cmos_sensor(0x6F12, 0x02F8);
		write_cmos_sensor(0x6F12, 0x256C);
		write_cmos_sensor(0x6F12, 0x8688);
		write_cmos_sensor(0x6F12, 0x360A);
		write_cmos_sensor(0x6F12, 0x02F8);
		write_cmos_sensor(0x6F12, 0x1F6C);
		write_cmos_sensor(0x6F12, 0x4679);
		write_cmos_sensor(0x6F12, 0x02F8);
		write_cmos_sensor(0x6F12, 0x1D6C);
		write_cmos_sensor(0x6F12, 0x9C4E);
		write_cmos_sensor(0x6F12, 0x96F8);
		write_cmos_sensor(0x6F12, 0x1064);
		write_cmos_sensor(0x6F12, 0xD671);
		write_cmos_sensor(0x6F12, 0x9A4E);
		write_cmos_sensor(0x6F12, 0x96F8);
		write_cmos_sensor(0x6F12, 0x1164);
		write_cmos_sensor(0x6F12, 0x5672);
		write_cmos_sensor(0x6F12, 0x984E);
		write_cmos_sensor(0x6F12, 0x96F8);
		write_cmos_sensor(0x6F12, 0x0B64);
		write_cmos_sensor(0x6F12, 0xD672);
		write_cmos_sensor(0x6F12, 0x964E);
		write_cmos_sensor(0x6F12, 0x96F8);
		write_cmos_sensor(0x6F12, 0x0964);
		write_cmos_sensor(0x6F12, 0x5673);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x3060);
		write_cmos_sensor(0x6F12, 0xD673);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0xDE00);
		write_cmos_sensor(0x6F12, 0x02F8);
		write_cmos_sensor(0x6F12, 0x1F0F);
		write_cmos_sensor(0x6F12, 0x9148);
		write_cmos_sensor(0x6F12, 0x00F2);
		write_cmos_sensor(0x6F12, 0x7246);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x7204);
		write_cmos_sensor(0x6F12, 0x9074);
		write_cmos_sensor(0x6F12, 0x3078);
		write_cmos_sensor(0x6F12, 0x1075);
		write_cmos_sensor(0x6F12, 0xA522);
		write_cmos_sensor(0x6F12, 0xDA70);
		write_cmos_sensor(0x6F12, 0x0E20);
		write_cmos_sensor(0x6F12, 0x1871);
		write_cmos_sensor(0x6F12, 0x11F8);
		write_cmos_sensor(0x6F12, 0x7E0C);
		write_cmos_sensor(0x6F12, 0xC0F1);
		write_cmos_sensor(0x6F12, 0x0C01);
		write_cmos_sensor(0x6F12, 0x8948);
		write_cmos_sensor(0x6F12, 0xD0F8);
		write_cmos_sensor(0x6F12, 0x3C04);
		write_cmos_sensor(0x6F12, 0xC840);
		write_cmos_sensor(0x6F12, 0x060A);
		write_cmos_sensor(0x6F12, 0x9E71);
		write_cmos_sensor(0x6F12, 0x1872);
		write_cmos_sensor(0x6F12, 0x0120);
		write_cmos_sensor(0x6F12, 0x03F8);
		write_cmos_sensor(0x6F12, 0x2C0C);
		write_cmos_sensor(0x6F12, 0x8448);
		write_cmos_sensor(0x6F12, 0xD0F8);
		write_cmos_sensor(0x6F12, 0x4C04);
		write_cmos_sensor(0x6F12, 0xC840);
		write_cmos_sensor(0x6F12, 0xAA21);
		write_cmos_sensor(0x6F12, 0x03F8);
		write_cmos_sensor(0x6F12, 0x571D);
		write_cmos_sensor(0x6F12, 0x0226);
		write_cmos_sensor(0x6F12, 0x5E70);
		write_cmos_sensor(0x6F12, 0x9A70);
		write_cmos_sensor(0x6F12, 0x3022);
		write_cmos_sensor(0x6F12, 0xDA70);
		write_cmos_sensor(0x6F12, 0x5A22);
		write_cmos_sensor(0x6F12, 0x1A71);
		write_cmos_sensor(0x6F12, 0x060A);
		write_cmos_sensor(0x6F12, 0x5E71);
		write_cmos_sensor(0x6F12, 0x9A71);
		write_cmos_sensor(0x6F12, 0xD871);
		write_cmos_sensor(0x6F12, 0x1972);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x5872);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x13B9);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x0746);
		write_cmos_sensor(0x6F12, 0x7148);
		write_cmos_sensor(0x6F12, 0x0C46);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x806A);
		write_cmos_sensor(0x6F12, 0x86B2);
		write_cmos_sensor(0x6F12, 0x050C);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x06F9);
		write_cmos_sensor(0x6F12, 0x2146);
		write_cmos_sensor(0x6F12, 0x3846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x5CF9);
		write_cmos_sensor(0x6F12, 0x6D48);
		write_cmos_sensor(0x6F12, 0x90F8);
		write_cmos_sensor(0x6F12, 0x9702);
		write_cmos_sensor(0x6F12, 0x10B9);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x33F9);
		write_cmos_sensor(0x6F12, 0x20B1);
		write_cmos_sensor(0x6F12, 0x04F1);
		write_cmos_sensor(0x6F12, 0x8044);
		write_cmos_sensor(0x6F12, 0xA08A);
		write_cmos_sensor(0x6F12, 0x401C);
		write_cmos_sensor(0x6F12, 0xA082);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x2846);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xEFB8);
		write_cmos_sensor(0x6F12, 0x2DE9);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x0746);
		write_cmos_sensor(0x6F12, 0x5F48);
		write_cmos_sensor(0x6F12, 0x0E46);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xC06A);
		write_cmos_sensor(0x6F12, 0x85B2);
		write_cmos_sensor(0x6F12, 0x040C);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xE2F8);
		write_cmos_sensor(0x6F12, 0x3146);
		write_cmos_sensor(0x6F12, 0x3846);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x3DF9);
		write_cmos_sensor(0x6F12, 0x584F);
		write_cmos_sensor(0x6F12, 0x4DF2);
		write_cmos_sensor(0x6F12, 0x0C26);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x8061);
		write_cmos_sensor(0x6F12, 0x3A78);
		write_cmos_sensor(0x6F12, 0x3046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD5F8);
		write_cmos_sensor(0x6F12, 0x7878);
		write_cmos_sensor(0x6F12, 0xC8B3);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0x4FF4);
		write_cmos_sensor(0x6F12, 0x0071);
		write_cmos_sensor(0x6F12, 0x3046);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xCDF8);
		write_cmos_sensor(0x6F12, 0x5648);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor(0x6F12, 0x564B);
		write_cmos_sensor(0x6F12, 0xA3F8);
		write_cmos_sensor(0x6F12, 0x4402);
		write_cmos_sensor(0x6F12, 0x5348);
		write_cmos_sensor(0x6F12, 0x001D);
		write_cmos_sensor(0x6F12, 0x0088);
		write_cmos_sensor(0x6F12, 0xA3F8);
		write_cmos_sensor(0x6F12, 0x4602);
		write_cmos_sensor(0x6F12, 0xB3F8);
		write_cmos_sensor(0x6F12, 0x4402);
		write_cmos_sensor(0x6F12, 0xB3F8);
		write_cmos_sensor(0x6F12, 0x4612);
		write_cmos_sensor(0x6F12, 0x4218);
		write_cmos_sensor(0x6F12, 0x02D0);
		write_cmos_sensor(0x6F12, 0x8002);
		write_cmos_sensor(0x6F12, 0xB0FB);
		write_cmos_sensor(0x6F12, 0xF2F2);
		write_cmos_sensor(0x6F12, 0x91B2);
		write_cmos_sensor(0x6F12, 0x4E4A);
		write_cmos_sensor(0x6F12, 0xA3F8);
		write_cmos_sensor(0x6F12, 0x4812);
		write_cmos_sensor(0x6F12, 0x5088);
		write_cmos_sensor(0x6F12, 0x1288);
		write_cmos_sensor(0x6F12, 0x4A4B);
		write_cmos_sensor(0x6F12, 0xA3F8);
		write_cmos_sensor(0x6F12, 0xA605);
		write_cmos_sensor(0x6F12, 0xA3F8);
		write_cmos_sensor(0x6F12, 0xA825);
		write_cmos_sensor(0x6F12, 0x8018);
		write_cmos_sensor(0x6F12, 0x05D0);
		write_cmos_sensor(0x6F12, 0x9202);
		write_cmos_sensor(0x6F12, 0xB2FB);
		write_cmos_sensor(0x6F12, 0xF0F0);
		write_cmos_sensor(0x6F12, 0x1A46);
		write_cmos_sensor(0x6F12, 0xA2F8);
		write_cmos_sensor(0x6F12, 0xAA05);
		write_cmos_sensor(0x6F12, 0x4448);
		write_cmos_sensor(0x6F12, 0xB0F8);
		write_cmos_sensor(0x6F12, 0xAA05);
		write_cmos_sensor(0x6F12, 0x0A18);
		write_cmos_sensor(0x6F12, 0x01FB);
		write_cmos_sensor(0x6F12, 0x1020);
		write_cmos_sensor(0x6F12, 0x40F3);
		write_cmos_sensor(0x6F12, 0x9510);
		write_cmos_sensor(0x6F12, 0x1028);
		write_cmos_sensor(0x6F12, 0x06DC);
		write_cmos_sensor(0x6F12, 0x0028);
		write_cmos_sensor(0x6F12, 0x05DA);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x03E0);
		write_cmos_sensor(0x6F12, 0xFFE7);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0xC3E7);
		write_cmos_sensor(0x6F12, 0x1020);
		write_cmos_sensor(0x6F12, 0x3D49);
		write_cmos_sensor(0x6F12, 0x0880);
		write_cmos_sensor(0x6F12, 0x2946);
		write_cmos_sensor(0x6F12, 0x2046);
		write_cmos_sensor(0x6F12, 0xBDE8);
		write_cmos_sensor(0x6F12, 0xF041);
		write_cmos_sensor(0x6F12, 0x0122);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x8CB8);
		write_cmos_sensor(0x6F12, 0x10B5);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x7B71);
		write_cmos_sensor(0x6F12, 0x3748);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xE9F8);
		write_cmos_sensor(0x6F12, 0x2C4C);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x2371);
		write_cmos_sensor(0x6F12, 0x6060);
		write_cmos_sensor(0x6F12, 0x3448);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xE1F8);
		write_cmos_sensor(0x6F12, 0xA060);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x2D60);
		write_cmos_sensor(0x6F12, 0x3249);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xC861);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x8351);
		write_cmos_sensor(0x6F12, 0x3148);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xD6F8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x1141);
		write_cmos_sensor(0x6F12, 0x6061);
		write_cmos_sensor(0x6F12, 0x2E48);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xCFF8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x0931);
		write_cmos_sensor(0x6F12, 0xA061);
		write_cmos_sensor(0x6F12, 0x2C48);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xC8F8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0xE121);
		write_cmos_sensor(0x6F12, 0xE061);
		write_cmos_sensor(0x6F12, 0x2948);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xC1F8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x4F61);
		write_cmos_sensor(0x6F12, 0x2062);
		write_cmos_sensor(0x6F12, 0x2748);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xBAF8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x0761);
		write_cmos_sensor(0x6F12, 0xE060);
		write_cmos_sensor(0x6F12, 0x2448);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xB3F8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0xEF21);
		write_cmos_sensor(0x6F12, 0x2061);
		write_cmos_sensor(0x6F12, 0x2248);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xACF8);
		write_cmos_sensor(0x6F12, 0x0022);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x9911);
		write_cmos_sensor(0x6F12, 0x6062);
		write_cmos_sensor(0x6F12, 0x1F48);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0xA5F8);
		write_cmos_sensor(0x6F12, 0xA062);
		write_cmos_sensor(0x6F12, 0x0020);
		write_cmos_sensor(0x6F12, 0x2146);
		write_cmos_sensor(0x6F12, 0x0246);
		write_cmos_sensor(0x6F12, 0x4880);
		write_cmos_sensor(0x6F12, 0xAFF2);
		write_cmos_sensor(0x6F12, 0x6911);
		write_cmos_sensor(0x6F12, 0x1B48);
		write_cmos_sensor(0x6F12, 0x00F0);
		write_cmos_sensor(0x6F12, 0x9BF8);
		write_cmos_sensor(0x6F12, 0x0949);
		write_cmos_sensor(0x6F12, 0xE062);
		write_cmos_sensor(0x6F12, 0x40F2);
		write_cmos_sensor(0x6F12, 0xF170);
		write_cmos_sensor(0x6F12, 0x0968);
		write_cmos_sensor(0x6F12, 0x4883);
		write_cmos_sensor(0x6F12, 0x10BD);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0FE0);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0xF474);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x4900);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2850);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0E20);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x2ED0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x08D0);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x36E0);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0x9404);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x38C0);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0xD214);
		write_cmos_sensor(0x6F12, 0x4000);
		write_cmos_sensor(0x6F12, 0xA410);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0xDE1F);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x5F3B);
		write_cmos_sensor(0x6F12, 0x2000);
		write_cmos_sensor(0x6F12, 0x0850);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0xD719);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x27FF);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x39E3);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x32CF);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x1E3B);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0xEC45);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x67B9);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0xE62B);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x2265);
		write_cmos_sensor(0x6F12, 0x4AF2);
		write_cmos_sensor(0x6F12, 0x2B1C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x4DF6);
		write_cmos_sensor(0x6F12, 0x1F6C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x44F6);
		write_cmos_sensor(0x6F12, 0x655C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x010C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x45F6);
		write_cmos_sensor(0x6F12, 0x433C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x45F6);
		write_cmos_sensor(0x6F12, 0xE36C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x46F2);
		write_cmos_sensor(0x6F12, 0x7B1C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x44F6);
		write_cmos_sensor(0x6F12, 0xD90C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x44F6);
		write_cmos_sensor(0x6F12, 0x791C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x44F6);
		write_cmos_sensor(0x6F12, 0x811C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x44F2);
		write_cmos_sensor(0x6F12, 0xB50C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x44F6);
		write_cmos_sensor(0x6F12, 0xE90C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x43F6);
		write_cmos_sensor(0x6F12, 0x155C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x43F6);
		write_cmos_sensor(0x6F12, 0x1D5C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x4DF2);
		write_cmos_sensor(0x6F12, 0xC95C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x42F2);
		write_cmos_sensor(0x6F12, 0xFF7C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x48F2);
		write_cmos_sensor(0x6F12, 0x712C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x43F6);
		write_cmos_sensor(0x6F12, 0xE31C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x46F2);
		write_cmos_sensor(0x6F12, 0xB97C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x4EF2);
		write_cmos_sensor(0x6F12, 0x2B6C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x42F2);
		write_cmos_sensor(0x6F12, 0x652C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x010C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x4CF2);
		write_cmos_sensor(0x6F12, 0x2D1C);
		write_cmos_sensor(0x6F12, 0xC0F2);
		write_cmos_sensor(0x6F12, 0x000C);
		write_cmos_sensor(0x6F12, 0x6047);
		write_cmos_sensor(0x6F12, 0x0000);

		//globle
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x16F0);
		write_cmos_sensor(0x6F12, 0x2929);
		write_cmos_sensor(0x602A, 0x16F2);
		write_cmos_sensor(0x6F12, 0x2929);
		write_cmos_sensor(0x602A, 0x16FA);
		write_cmos_sensor(0x6F12, 0x0029);
		write_cmos_sensor(0x602A, 0x16FC);
		write_cmos_sensor(0x6F12, 0x0029);
		write_cmos_sensor(0x602A, 0x1708);
		write_cmos_sensor(0x6F12, 0x0029);
		write_cmos_sensor(0x602A, 0x170A);
		write_cmos_sensor(0x6F12, 0x0029);
		write_cmos_sensor(0x602A, 0x1712);
		write_cmos_sensor(0x6F12, 0x2929);
		write_cmos_sensor(0x602A, 0x1714);
		write_cmos_sensor(0x6F12, 0x2929);
		write_cmos_sensor(0x602A, 0x1716);
		write_cmos_sensor(0x6F12, 0x2929);
		write_cmos_sensor(0x602A, 0x1722);
		write_cmos_sensor(0x6F12, 0x152A);
		write_cmos_sensor(0x602A, 0x1724);
		write_cmos_sensor(0x6F12, 0x152A);
		write_cmos_sensor(0x602A, 0x172C);
		write_cmos_sensor(0x6F12, 0x002A);
		write_cmos_sensor(0x602A, 0x172E);
		write_cmos_sensor(0x6F12, 0x002A);
		write_cmos_sensor(0x602A, 0x1736);
		write_cmos_sensor(0x6F12, 0x1500);
		write_cmos_sensor(0x602A, 0x1738);
		write_cmos_sensor(0x6F12, 0x1500);
		write_cmos_sensor(0x602A, 0x1740);
		write_cmos_sensor(0x6F12, 0x152A);
		write_cmos_sensor(0x602A, 0x1742);
		write_cmos_sensor(0x6F12, 0x152A);
		write_cmos_sensor(0x602A, 0x16BE);
		write_cmos_sensor(0x6F12, 0x1515);
		write_cmos_sensor(0x6F12, 0x1515);
		write_cmos_sensor(0x602A, 0x16C8);
		write_cmos_sensor(0x6F12, 0x0029);
		write_cmos_sensor(0x6F12, 0x0029);
		write_cmos_sensor(0x602A, 0x16D6);
		write_cmos_sensor(0x6F12, 0x0015);
		write_cmos_sensor(0x6F12, 0x0015);
		write_cmos_sensor(0x602A, 0x16E0);
		write_cmos_sensor(0x6F12, 0x2929);
		write_cmos_sensor(0x6F12, 0x2929);
		write_cmos_sensor(0x6F12, 0x2929);
		write_cmos_sensor(0x602A, 0x19B8);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x602A, 0x2224);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x602A, 0x0DF8);
		write_cmos_sensor(0x6F12, 0x1001);
		write_cmos_sensor(0x602A, 0x1EDA);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x602A, 0x16A0);
		write_cmos_sensor(0x6F12, 0x3D09);
		write_cmos_sensor(0x602A, 0x10A8);
		write_cmos_sensor(0x6F12, 0x000E);
		write_cmos_sensor(0x602A, 0x1198);
		write_cmos_sensor(0x6F12, 0x002B);
		write_cmos_sensor(0x602A, 0x1002);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x602A, 0x0F70);
		write_cmos_sensor(0x6F12, 0x0100);
		write_cmos_sensor(0x6F12, 0x002F);
		write_cmos_sensor(0x602A, 0x0F76);
		write_cmos_sensor(0x6F12, 0x0030);
		write_cmos_sensor(0x602A, 0x0F7A);
		write_cmos_sensor(0x6F12, 0x000B);
		write_cmos_sensor(0x6F12, 0x0009);
		write_cmos_sensor(0x6F12, 0xF46E);
		write_cmos_sensor(0x602A, 0x1698);
		write_cmos_sensor(0x6F12, 0x0D05);
		write_cmos_sensor(0x602A, 0x20A0);
		write_cmos_sensor(0x6F12, 0x0001);
		write_cmos_sensor(0x6F12, 0x0203);
		write_cmos_sensor(0x602A, 0x4900);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x0FEA, 0x1440);
		write_cmos_sensor(0x0B06, 0x0101);
		write_cmos_sensor(0xF44A, 0x0007);
		write_cmos_sensor(0xF456, 0x000A);
		write_cmos_sensor(0xF46A, 0xBFA0);
		write_cmos_sensor(0x0D80, 0x1388);
		write_cmos_sensor(0x0A02, 0x007D);
		//pd 
		write_cmos_sensor(0x6028, 0x2000);
		write_cmos_sensor(0x602A, 0x25B8);
		write_cmos_sensor(0x6F12, 0x0403);
		write_cmos_sensor(0x6F12, 0x040B);
		write_cmos_sensor(0x6F12, 0x040F);
		write_cmos_sensor(0x6F12, 0x0407);
		write_cmos_sensor(0x6F12, 0x0403);
		write_cmos_sensor(0x6F12, 0x040B);
		write_cmos_sensor(0x6F12, 0x040F);
		write_cmos_sensor(0x6F12, 0x0407);
		write_cmos_sensor(0x602A, 0x25B0);
		write_cmos_sensor(0x6F12, 0x0008);
		write_cmos_sensor(0x6F12, 0x1004);
		write_cmos_sensor(0x602A, 0x25E8);
		write_cmos_sensor(0x6F12, 0x4444);
		write_cmos_sensor(0x6F12, 0x4444);
		write_cmos_sensor(0x602A, 0x25B6);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x17F8);
		write_cmos_sensor(0x6F12, 0x0040);
		write_cmos_sensor(0x602A, 0x17FC);
		write_cmos_sensor(0x6F12, 0x1101);
		write_cmos_sensor(0x6F12, 0x1105);
		write_cmos_sensor(0x6F12, 0x1109);
		write_cmos_sensor(0x6F12, 0x110D);
		write_cmos_sensor(0x6F12, 0x1111);
		write_cmos_sensor(0x6F12, 0x1115);
		write_cmos_sensor(0x6F12, 0x1119);
		write_cmos_sensor(0x6F12, 0x111D);
		write_cmos_sensor(0x6F12, 0x1121);
		write_cmos_sensor(0x6F12, 0x1125);
		write_cmos_sensor(0x6F12, 0x1129);
		write_cmos_sensor(0x6F12, 0x112D);
		write_cmos_sensor(0x6F12, 0x1131);
		write_cmos_sensor(0x6F12, 0x1135);
		write_cmos_sensor(0x6F12, 0x1139);
		write_cmos_sensor(0x6F12, 0x113D);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x6F12, 0x0000);
		write_cmos_sensor(0x602A, 0x25F0);
		write_cmos_sensor(0x6F12, 0x0410);
		write_cmos_sensor(0x602A, 0x25F6);
		write_cmos_sensor(0x6F12, 0x1010);
		write_cmos_sensor(0x6F12, 0x1010);
		write_cmos_sensor(0x6F12, 0x1010);
		write_cmos_sensor(0x6F12, 0x1010);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x1010);
		write_cmos_sensor(0x6F12, 0x1010);
		write_cmos_sensor(0x6F12, 0x1010);
		write_cmos_sensor(0x6F12, 0x1010);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x6F12, 0x0101);
		write_cmos_sensor(0x602A, 0x2666);
		write_cmos_sensor(0x6F12, 0x0000);
		LOG_INF(" out \n");
		}	/*      sensor_init  */


static void preview_setting(void)
{
	//Preview 2320*1744 30fps 24M MCLK 4lane 732Mbps/lane
	// preview 30.01fps
	LOG_INF("E\n");
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7970);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x0ED6);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1CF0);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0E58);
	write_cmos_sensor(0x6F12, 0x0040);
	write_cmos_sensor(0x602A, 0x1694);
	write_cmos_sensor(0x6F12, 0x1B0F);
	write_cmos_sensor(0x602A, 0x16AA);
	write_cmos_sensor(0x6F12, 0x009F);
	write_cmos_sensor(0x6F12, 0x0007);
	write_cmos_sensor(0x602A, 0x1098);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x602A, 0x2690);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0055);
	write_cmos_sensor(0x602A, 0x16A8);
	write_cmos_sensor(0x6F12, 0x38CD);
	write_cmos_sensor(0x602A, 0x108C);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x602A, 0x10CC);
	write_cmos_sensor(0x6F12, 0x0008);
	write_cmos_sensor(0x602A, 0x10D0);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x602A, 0x0F50);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0C60);
	write_cmos_sensor(0x6F12, 0x0002);
	write_cmos_sensor(0x6F12, 0x0202);
	write_cmos_sensor(0x602A, 0x1758);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0344, 0x0000);
	write_cmos_sensor(0x0346, 0x0008);
	write_cmos_sensor(0x0348, 0x122F);
	write_cmos_sensor(0x034A, 0x0DA7);
	write_cmos_sensor(0x034C, 0x0910);
	write_cmos_sensor(0x034E, 0x06D0);
	write_cmos_sensor(0x0350, 0x0004);
	write_cmos_sensor(0x0900, 0x0112);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0003);
	write_cmos_sensor(0x0404, 0x2000);
	write_cmos_sensor(0x0402, 0x1010);
	write_cmos_sensor(0x0400, 0x1010);
	write_cmos_sensor(0x0114, 0x0301);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0007);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x00F5);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0004);
	write_cmos_sensor(0x0310, 0x005A);
	write_cmos_sensor(0x0312, 0x0001);
	write_cmos_sensor(0x0340, 0x0894);
	write_cmos_sensor(0x0342, 0x2130);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x021E, 0x0000);
	write_cmos_sensor(0x0D00, 0x0100);
	write_cmos_sensor(0x0D02, 0x0001);
	write_cmos_sensor(0x0D04, 0x0100);
	write_cmos_sensor(0x0D06, 0x0120);
	write_cmos_sensor(0x0D08, 0x0360);
}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("capture_setting() E! currefps:%d\n", currefps);

	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7970);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x0ED6);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1CF0);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0E58);
	write_cmos_sensor(0x6F12, 0x0040);
	write_cmos_sensor(0x602A, 0x1694);
	write_cmos_sensor(0x6F12, 0x1B0F);
	write_cmos_sensor(0x602A, 0x16AA);
	write_cmos_sensor(0x6F12, 0x009F);
	write_cmos_sensor(0x6F12, 0x0007);
	write_cmos_sensor(0x602A, 0x1098);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x602A, 0x2690);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0055);
	write_cmos_sensor(0x602A, 0x16A8);
	write_cmos_sensor(0x6F12, 0x38CD);
	write_cmos_sensor(0x602A, 0x108C);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x602A, 0x10CC);
	write_cmos_sensor(0x6F12, 0x0008);
	write_cmos_sensor(0x602A, 0x10D0);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x602A, 0x0F50);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0C60);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0x0202);
	write_cmos_sensor(0x602A, 0x1758);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0344, 0x0000);
	write_cmos_sensor(0x0346, 0x0008);
	write_cmos_sensor(0x0348, 0x122F);
	write_cmos_sensor(0x034A, 0x0DA7);
	write_cmos_sensor(0x034C, 0x1220);
	write_cmos_sensor(0x034E, 0x0DA0);
	write_cmos_sensor(0x0350, 0x0008);
	write_cmos_sensor(0x0900, 0x0011);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0001);
	write_cmos_sensor(0x0404, 0x1000);
	write_cmos_sensor(0x0402, 0x1010);
	write_cmos_sensor(0x0400, 0x1010);
	write_cmos_sensor(0x0114, 0x0301);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0007);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x00F5);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0004);
	write_cmos_sensor(0x0310, 0x007A);
	write_cmos_sensor(0x0312, 0x0000);
	write_cmos_sensor(0x0340, 0x0E54);
	write_cmos_sensor(0x0342, 0x13E0);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x021E, 0x0000);
	write_cmos_sensor(0x0D00, 0x0100);
	write_cmos_sensor(0x0D02, 0x0001);
	write_cmos_sensor(0x0D04, 0x0102);
	write_cmos_sensor(0x0D06, 0x0120);
	write_cmos_sensor(0x0D08, 0x0360);
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	// full size 30fps
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7970);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x0ED6);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1CF0);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0E58);
	write_cmos_sensor(0x6F12, 0x0040);
	write_cmos_sensor(0x602A, 0x1694);
	write_cmos_sensor(0x6F12, 0x1B0F);
	write_cmos_sensor(0x602A, 0x16AA);
	write_cmos_sensor(0x6F12, 0x009F);
	write_cmos_sensor(0x6F12, 0x0007);
	write_cmos_sensor(0x602A, 0x1098);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x602A, 0x2690);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0055);
	write_cmos_sensor(0x602A, 0x16A8);
	write_cmos_sensor(0x6F12, 0x38CD);
	write_cmos_sensor(0x602A, 0x108C);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x602A, 0x10CC);
	write_cmos_sensor(0x6F12, 0x0008);
	write_cmos_sensor(0x602A, 0x10D0);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x602A, 0x0F50);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0C60);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0x0202);
	write_cmos_sensor(0x602A, 0x1758);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0344, 0x0000);
	write_cmos_sensor(0x0346, 0x01C0);
	write_cmos_sensor(0x0348, 0x122F);
	write_cmos_sensor(0x034A, 0x0BEF);
	write_cmos_sensor(0x034C, 0x1220);
	write_cmos_sensor(0x034E, 0x0A30);
	write_cmos_sensor(0x0350, 0x0008);
	write_cmos_sensor(0x0900, 0x0011);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0001);
	write_cmos_sensor(0x0404, 0x1000);
	write_cmos_sensor(0x0402, 0x1010);
	write_cmos_sensor(0x0400, 0x1010);
	write_cmos_sensor(0x0114, 0x0301);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0007);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x00F5);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0004);
	write_cmos_sensor(0x0310, 0x005F);
	write_cmos_sensor(0x0312, 0x0000);
	write_cmos_sensor(0x0340, 0x0ABE);
	write_cmos_sensor(0x0342, 0x1A80);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x021E, 0x0000);
	write_cmos_sensor(0x0D00, 0x0100);
	write_cmos_sensor(0x0D02, 0x0001);
	write_cmos_sensor(0x0D04, 0x0102);
	write_cmos_sensor(0x0D06, 0x0120);
	write_cmos_sensor(0x0D08, 0x028C);
}
static void hs_video_setting(void)
{
	LOG_INF("E\n");
	//720p 120fps
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7970);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x0ED6);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1CF0);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0E58);
	write_cmos_sensor(0x6F12, 0x0040);
	write_cmos_sensor(0x602A, 0x1694);
	write_cmos_sensor(0x6F12, 0x1B0F);
	write_cmos_sensor(0x602A, 0x16AA);
	write_cmos_sensor(0x6F12, 0x009F);
	write_cmos_sensor(0x6F12, 0x0007);
	write_cmos_sensor(0x602A, 0x1098);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x602A, 0x2690);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0055);
	write_cmos_sensor(0x602A, 0x16A8);
	write_cmos_sensor(0x6F12, 0x38CD);
	write_cmos_sensor(0x602A, 0x108C);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x602A, 0x10CC);
	write_cmos_sensor(0x6F12, 0x0008);
	write_cmos_sensor(0x602A, 0x10D0);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x602A, 0x0F50);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0C60);
	write_cmos_sensor(0x6F12, 0x0002);
	write_cmos_sensor(0x6F12, 0x0202);
	write_cmos_sensor(0x602A, 0x1758);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0344, 0x0180);
	write_cmos_sensor(0x0346, 0x02A0);
	write_cmos_sensor(0x0348, 0x10AF);
	write_cmos_sensor(0x034A, 0x0B0F);
	write_cmos_sensor(0x034C, 0x0500);
	write_cmos_sensor(0x034E, 0x02D0);
	write_cmos_sensor(0x0350, 0x0008);
	write_cmos_sensor(0x0900, 0x0113);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0005);
	write_cmos_sensor(0x0404, 0x3000);
	write_cmos_sensor(0x0402, 0x1010);
	write_cmos_sensor(0x0400, 0x1010);
	write_cmos_sensor(0x0114, 0x0300);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0007);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x00F5);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0004);
	write_cmos_sensor(0x0310, 0x005A);
	write_cmos_sensor(0x0312, 0x0001);
	write_cmos_sensor(0x0340, 0x0395);
	write_cmos_sensor(0x0342, 0x13E0);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x021E, 0x0000);
	write_cmos_sensor(0x0D00, 0x0000);
	write_cmos_sensor(0x0D02, 0x0001);
	write_cmos_sensor(0x0D04, 0x0102);
	write_cmos_sensor(0x0D06, 0x0000);
	write_cmos_sensor(0x0D08, 0x0000);

}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
	preview_setting();
}

static void custom1_setting(void)
{
	LOG_INF("custom1_setting E!\n");
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7970);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x0ED6);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1CF0);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0E58);
	write_cmos_sensor(0x6F12, 0x0040);
	write_cmos_sensor(0x602A, 0x1694);
	write_cmos_sensor(0x6F12, 0x1B0F);
	write_cmos_sensor(0x602A, 0x16AA);
	write_cmos_sensor(0x6F12, 0x009F);
	write_cmos_sensor(0x6F12, 0x0007);
	write_cmos_sensor(0x602A, 0x1098);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x602A, 0x2690);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0055);
	write_cmos_sensor(0x602A, 0x16A8);
	write_cmos_sensor(0x6F12, 0x38CD);
	write_cmos_sensor(0x602A, 0x108C);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x602A, 0x10CC);
	write_cmos_sensor(0x6F12, 0x0008);
	write_cmos_sensor(0x602A, 0x10D0);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x602A, 0x0F50);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0C60);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0x0202);
	write_cmos_sensor(0x602A, 0x1758);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0344, 0x0000);
	write_cmos_sensor(0x0346, 0x0008);
	write_cmos_sensor(0x0348, 0x122F);
	write_cmos_sensor(0x034A, 0x0DA7);
	write_cmos_sensor(0x034C, 0x1220);
	write_cmos_sensor(0x034E, 0x0DA0);
	write_cmos_sensor(0x0350, 0x0008);
	write_cmos_sensor(0x0900, 0x0011);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0001);
	write_cmos_sensor(0x0404, 0x1000);
	write_cmos_sensor(0x0402, 0x1010);
	write_cmos_sensor(0x0400, 0x1010);
	write_cmos_sensor(0x0114, 0x0301);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0007);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x00F5);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0004);
	write_cmos_sensor(0x0310, 0x007A);
	write_cmos_sensor(0x0312, 0x0000);
	write_cmos_sensor(0x0340, 0x0E54);
	write_cmos_sensor(0x0342, 0x13E0);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x021E, 0x0000);
	write_cmos_sensor(0x0D00, 0x0100);
	write_cmos_sensor(0x0D02, 0x0001);
	write_cmos_sensor(0x0D04, 0x0102);
	write_cmos_sensor(0x0D06, 0x0120);
	write_cmos_sensor(0x0D08, 0x0360);
}

static void custom2_setting(void)
{
	LOG_INF(" E!\n");
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6214, 0x7970);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x0ED6);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1CF0);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0E58);
	write_cmos_sensor(0x6F12, 0x0040);
	write_cmos_sensor(0x602A, 0x1694);
	write_cmos_sensor(0x6F12, 0x1B0F);
	write_cmos_sensor(0x602A, 0x16AA);
	write_cmos_sensor(0x6F12, 0x009F);
	write_cmos_sensor(0x6F12, 0x0007);
	write_cmos_sensor(0x602A, 0x1098);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x602A, 0x2690);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0055);
	write_cmos_sensor(0x602A, 0x16A8);
	write_cmos_sensor(0x6F12, 0x38CD);
	write_cmos_sensor(0x602A, 0x108C);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x602A, 0x10CC);
	write_cmos_sensor(0x6F12, 0x0008);
	write_cmos_sensor(0x602A, 0x10D0);
	write_cmos_sensor(0x6F12, 0x000F);
	write_cmos_sensor(0x602A, 0x0F50);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0C60);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0x0202);
	write_cmos_sensor(0x602A, 0x1758);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0344, 0x0000);
	write_cmos_sensor(0x0346, 0x0008);
	write_cmos_sensor(0x0348, 0x122F);
	write_cmos_sensor(0x034A, 0x0DA7);
	write_cmos_sensor(0x034C, 0x1220);
	write_cmos_sensor(0x034E, 0x0DA0);
	write_cmos_sensor(0x0350, 0x0008);
	write_cmos_sensor(0x0900, 0x0011);
	write_cmos_sensor(0x0380, 0x0001);
	write_cmos_sensor(0x0382, 0x0001);
	write_cmos_sensor(0x0384, 0x0001);
	write_cmos_sensor(0x0386, 0x0001);
	write_cmos_sensor(0x0404, 0x1000);
	write_cmos_sensor(0x0402, 0x1010);
	write_cmos_sensor(0x0400, 0x1010);
	write_cmos_sensor(0x0114, 0x0301);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x0300, 0x0007);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0006);
	write_cmos_sensor(0x0306, 0x00F5);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0004);
	write_cmos_sensor(0x0310, 0x0063);
	write_cmos_sensor(0x0312, 0x0000);
	write_cmos_sensor(0x0340, 0x0E4E);
	write_cmos_sensor(0x0342, 0x18E0);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x021E, 0x0000);
	write_cmos_sensor(0x0D00, 0x0100);
	write_cmos_sensor(0x0D02, 0x0001);
	write_cmos_sensor(0x0D04, 0x0102);
	write_cmos_sensor(0x0D06, 0x0120);
	write_cmos_sensor(0x0D08, 0x0360);
}

#if VENDOR_EDIT
/*xxxx,modify for different module*/
static kal_uint16 read_module_id(void)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(MODULE_ID_OFFSET >> 8), (char)(MODULE_ID_OFFSET & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *)&get_byte, 1, 0xA0/*EEPROM_READ_ID*/);
	pr_err("the module id is %d\n", get_byte);
	return get_byte;
}
#endif
static void get_back_cam_efuse_id(void)
{
	int i = 0,temp = 0;
	kal_uint8 efuse_id;
	write_cmos_sensor(0x6010, 0x0001);
	mdelay(3);
	write_cmos_sensor(0x6214,0x7970);
	write_cmos_sensor(0x6218,0x7150);
	write_cmos_sensor(0x0136,0x1800);
	write_cmos_sensor(0x0304,0x0006);
	write_cmos_sensor(0x030C,0x0000);
	write_cmos_sensor(0x0306,0x00F5);
	write_cmos_sensor(0x0302,0x0001);
	write_cmos_sensor(0x0300,0x0007);
	write_cmos_sensor(0x030e,0x0004);
	write_cmos_sensor(0x0312,0x0000);
	write_cmos_sensor(0x0310,0x008B);
	write_cmos_sensor(0x030A,0x0001);
	write_cmos_sensor(0x0308,0x0008);

	write_cmos_sensor_byte(0x0100, 0x01);
	mdelay(50);
	write_cmos_sensor(0x0a02, 0x0000);
	write_cmos_sensor_byte(0x0a00, 0x01);
	mdelay(1);
	for(temp=0;temp<3;temp++)
	{
		if(0x00 == read_cmos_sensor_8(0x0a00))
		{
			for(i=0;i<6;i++)
			{
				efuse_id = read_cmos_sensor_8(0x0a24+i);
				sprintf(back_cam_efuse_id+2*i,"%02x",efuse_id);
				mdelay(1);
				LOG_INF("get_back_cam_efuse_id- efuse_id = 0x%02x\n", efuse_id);
			}
			break;
		}
		mdelay(1);
	}
	write_cmos_sensor_byte(0x0a00, 0x00);
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
extern char back_cam_name[64];
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
			*sensor_id = ((read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));
			pr_err("read_0x0000=0x%x, 0x0001=0x%x,0x0000_0001=0x%x\n", read_cmos_sensor_8(0x0000), read_cmos_sensor_8(0x0001), read_cmos_sensor(0x0000));
			if (*sensor_id == imgsensor_info.sensor_id) {
				//#if VENDOR_EDIT
				#if 0
				imgsensor_info.module_id = read_module_id();
				if (deviceInfo_register_value == 0x00) {
					register_imgsensor_deviceinfo("Cam_b", DEVICE_VERSION_S5K3P9SXT, imgsensor_info.module_id);
					deviceInfo_register_value = 0x01;
				}
				#endif
				imgsensor_info.module_id = read_module_id();
				if(0x60 == imgsensor_info.module_id)
				{
					*sensor_id = S5K3P9SXT_SENSOR_ID;
					//dual_main_sensorid = *sensor_id;
					get_back_cam_efuse_id();
					memset(back_cam_name, 0x00, sizeof(back_cam_name));
					memcpy(back_cam_name, "0_s5k3p9sx_TXD", 64);
					//ontim_get_otp_data(*sensor_id, NULL, 0);
					pr_err("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
					return ERROR_NONE;
				}
#if 0
				else if(0x50 != imgsensor_info.module_id)
				{
					*sensor_id = S5K3P9SXT_SENSOR_ID;
					dual_main_sensorid = *sensor_id;
					get_back_cam_efuse_id();
					memset(back_cam_name, 0x00, sizeof(back_cam_name));
					memcpy(back_cam_name, "0_s5k3p9sx_nootp", 64);
					pr_err("wrong module id, i2c write id: 0x%x, sensor id: 0x%x, module id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id,imgsensor_info.module_id);
					return ERROR_NONE;
				}
#endif
			}
			pr_err("Read sensor id fail, id: 0x%x ensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != S5K3P9SXT_SENSOR_ID) {
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
			sensor_id = ((read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id);
			retry--;
		} while(retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id) {
			break;
		}
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id) {
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	/* initail sequence write in  */
	sensor_init();

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
	streaming_control(KAL_FALSE);
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
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
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
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
		imgsensor.pclk = imgsensor_info.cap2.pclk;
		imgsensor.line_length = imgsensor_info.cap2.linelength;
		imgsensor.frame_length = imgsensor_info.cap2.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap2.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate) {
			LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
				imgsensor.current_fps, imgsensor_info.cap.max_framerate / 10);
		}
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);

	//burst_read_to_check();
	//burst_read_to_check2();

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
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);

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
	set_mirror_flip(imgsensor.mirror);

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
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	slim_video	 */


static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom1_setting();
	return ERROR_NONE;
}   /*  Custom1   */
static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	pr_debug("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	imgsensor.pclk = imgsensor_info.custom2.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.custom2.linelength;
	imgsensor.frame_length = imgsensor_info.custom2.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom2_setting();
	return ERROR_NONE;
}   /*  Custom2   */


static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight = imgsensor_info.slim_video.grabwindow_height;

	sensor_resolution->SensorCustom1Width  = imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height     = imgsensor_info.custom1.grabwindow_height;

	sensor_resolution->SensorCustom2Width  = imgsensor_info.custom2.grabwindow_width;
	sensor_resolution->SensorCustom2Height = imgsensor_info.custom2.grabwindow_height;
	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

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

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame; /* The delay frame of setting frame length  */
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 1;
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

		default:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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
			Custom1(image_window, sensor_config_data);	// Custom1
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			Custom2(image_window, sensor_config_data);	// Custom2
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
	if (framerate == 0) {
		// Dynamic frame rate
		return ERROR_NONE;
	}
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE)) {
		imgsensor.current_fps = 296;
	} else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE)) {
		imgsensor.current_fps = 146;
	} else {
		imgsensor.current_fps = framerate;
	}
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) {//enable auto flicker
		imgsensor.autoflicker_en = KAL_TRUE;
	} else {//Cancel Auto flick
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
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
			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if(framerate == 0) {
				return ERROR_NONE;
			}
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
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
				if (imgsensor.current_fps != imgsensor_info.cap.max_framerate) {
					LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate / 10);
				}
				frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
				spin_lock(&imgsensor_drv_lock);
				imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
				imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
				imgsensor.min_frame_length = imgsensor.frame_length;
				spin_unlock(&imgsensor_drv_lock);
			}
			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
			if (imgsensor.dummy_line < 0) {
				imgsensor.dummy_line = 0;
			}
			imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			frame_length = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.custom2.framelength) ? (frame_length - imgsensor_info.custom2.framelength) : 0;
			if (imgsensor.dummy_line < 0) {
				imgsensor.dummy_line = 0;
			}
			imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
			break;

		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) {
				set_dummy();
			}
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
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
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
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
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;
	//unsigned long long *feature_return_para=(unsigned long long *) feature_para;
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
	//SET_SENSOR_AWB_GAIN *pSetSensorAWB=(SET_SENSOR_AWB_GAIN *)feature_para;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/*LOG_INF("feature_id = %d\n", feature_id);*/
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
			//night_mode((BOOL) *feature_data);
			break;
		//#if VENDOR_EDIT
		#if 0
		/*zhengjiang.zhu@Camera.driver, 2017/10/17	add  for  module id*/
		case SENSOR_FEATURE_CHECK_MODULE_ID:
			*feature_return_para_32 = imgsensor_info.module_id;
			break;
		#endif
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
			set_auto_flicker_mode((BOOL)*feature_data_16, *(feature_data_16 + 1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			set_max_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data + 1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data + 1)));
			break;
		case SENSOR_FEATURE_GET_PDAF_DATA:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
			s5k3p9sxt_read_eeprom((kal_uint16 )(*feature_data), (char*)(uintptr_t)(*(feature_data + 1)), (kal_uint32)(*(feature_data + 2)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len=4;
			break;
		case SENSOR_FEATURE_SET_FRAMERATE:
			LOG_INF("current fps :%d\n", (UINT32)*feature_data_32);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.current_fps = *feature_data_32;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_SET_HDR:
			LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data_32);
			spin_lock(&imgsensor_drv_lock);
			imgsensor.ihdr_mode = *feature_data_32;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_GET_CROP_INFO:
			LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data);
			wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

			switch (*feature_data_32) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[1], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[2], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[3], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[4], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
				case MSDK_SCENARIO_ID_CUSTOM1:
					memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[5], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;

				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
					break;
			}
						break;
		case SENSOR_FEATURE_GET_PDAF_INFO:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n", (UINT16) *feature_data);
			PDAFinfo= (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info, sizeof(struct SET_PD_BLOCK_INFO_T));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					//memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info_16_9,sizeof(SET_PD_BLOCK_INFO_T));
					break;
				case MSDK_SCENARIO_ID_CUSTOM1:
					memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info, sizeof(struct SET_PD_BLOCK_INFO_T));
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				case MSDK_SCENARIO_ID_SLIM_VIDEO:

				default:
					break;
			}
			break;
		case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
			LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n", (UINT16) *feature_data);
			//PDAF capacity enable or not, 2p8 only full size support PDAF
			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1; // video & capture use same setting
					break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
				case MSDK_SCENARIO_ID_CUSTOM1:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
					break;
				default:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
			}
			break;
		case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
			LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n", (UINT16)*feature_data, (UINT16)*(feature_data + 1), (UINT16)*(feature_data + 2));
			break;
		case SENSOR_FEATURE_SET_AWB_GAIN:
			break;
		case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
			set_shutter_frame_length((UINT16) (*feature_data), (UINT16) (*(feature_data + 1)));
			break;
		case SENSOR_FEATURE_SET_HDR_SHUTTER:
			LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n", (UINT16)*feature_data, (UINT16)*(feature_data + 1));
			//ihdr_write_shutter((UINT16)*feature_data,(UINT16)*(feature_data+1));
			break;
		case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
			LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
			streaming_control(KAL_FALSE);
			break;
		case SENSOR_FEATURE_SET_STREAMING_RESUME:
			LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n", *feature_data);
			if (*feature_data != 0) {
				set_shutter(*feature_data);
			}
			streaming_control(KAL_TRUE);
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
				case MSDK_SCENARIO_ID_CUSTOM1:
					rate = imgsensor_info.custom1.mipi_pixel_rate;
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					rate = imgsensor_info.pre.mipi_pixel_rate;
					break;
			}
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
		}
		break;
		case SENSOR_FEATURE_GET_VC_INFO:
			LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
			pvcinfo = (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data + 1));
			switch (*feature_data_32) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1], sizeof(struct SENSOR_VC_INFO_STRUCT));
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2], sizeof(struct SENSOR_VC_INFO_STRUCT));
				break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			case MSDK_SCENARIO_ID_CUSTOM1:
			default:
				memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0], sizeof(struct SENSOR_VC_INFO_STRUCT));
				break;
			}
		default:
			break;
	}

	return ERROR_NONE;
}	/*	feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

//kin0603
UINT32 S5K3P9SXT_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
//UINT32 xxxx_MIPI_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL) {
		*pfFunc=&sensor_func;
	}
	return ERROR_NONE;
}	/*	xxxx_MIPI_RAW_SensorInit	*/
