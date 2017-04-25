/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 hi556mipiraw_sensor.c
 *
 * Project:
 * --------
 *	 ALPS MT6735
 *	20150820: mirror flip not setting,so preview fuction can can use it ,to resolv it.
  *	20150914: 送测完了fae 要求修改setting，重新合setting.
  -------------------------------
  @DateTime:    20151021163817
  modify the winsize info,last time foroget to modity it.
 * Description:
 -------------------------------
 @DateTime:    20150925173104 move to mt6735
 modify winsizeinfo,add pip setting ,add high speed video setting-------------------------------
 @DateTime:    20151119155951
 add group hold fuction at shutter gain setting, for ae peak
 * ------------
 *	 Source code of Sensor driver
 *
 *	PengtaoFan

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
#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "hi556mipiraw_Sensor.h"
/*===FEATURE SWITH===*/
 // #define FPTPDAFSUPPORT   //for pdaf switch
 // #define FANPENGTAO   //for debug log
 #define LOG_INF LOG_INF_NEW
 //#define NONCONTINUEMODE
/*===FEATURE SWITH===*/


/****************************Modify Following Strings for Debug****************************/
#define PFX "hi556"
#define LOG_INF_NEW(format, args...)    pr_err(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_INF_LOD(format, args...)	xlog_printk(ANDROID_LOG_INFO   , PFX, "[%s] " format, __FUNCTION__, ##args)
#define LOG_1 LOG_INF("hi556,MIPI 2LANE\n")
#define SENSORDB LOG_INF
/****************************   Modify end    *******************************************/
#define HI556_OTP_FUNCTION




static DEFINE_SPINLOCK(imgsensor_drv_lock);




static imgsensor_info_struct imgsensor_info = { 
	.sensor_id = HI556_SENSOR_ID,		//Sensor ID Value: 0x30C8//record sensor id defined in Kd_imgsensor.h
	
	.checksum_value = 0xe48556a,		//checksum value for Camera Auto Test

	.pre = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,				//record different mode's linelength
		.framelength = 2049,			//record different mode's framelength
		.startx= 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1296,		//record different mode's width of grabwindow
		.grabwindow_height = 972,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
#ifdef NONCONTINUEMODE
	.cap = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,//5808,				//record different mode's linelength
		.framelength = 2083,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2592,		//record different mode's width of grabwindow
		.grabwindow_height = 1944,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
#else //CONTINUEMODE
	.cap = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,//5808,				//record different mode's linelength
		.framelength = 2049,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2592,		//record different mode's width of grabwindow
		.grabwindow_height = 1944,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
#endif
#if 0 //fps =24
	.cap1 = {							//capture for PIP 24fps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 448000000,				//record different mode's pclk
		.linelength  = 5808,				//record different mode's linelength
		.framelength = 3206,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 240,	
	},
#endif
#if 1 //fps 15
	.cap1 = {							//capture for PIP 15ps relative information, capture1 mode must use same framelength, linelength with Capture mode for shutter calculate
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,//,				//record different mode's linelength
		.framelength = 4166,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2592,		//record different mode's width of grabwindow
		.grabwindow_height = 1944,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 150,	
	},
#endif
	.normal_video = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,//5808,				//record different mode's linelength
		.framelength = 2083,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2592,		//record different mode's width of grabwindow
		.grabwindow_height = 1944,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
	.hs_video = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,				//record different mode's linelength
		.framelength = 520,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 640,		//record different mode's width of grabwindow
		.grabwindow_height = 480,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 19,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 1200,	
	},
	.slim_video = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength  = 2816,				//record different mode's linelength
		.framelength = 2049,			//record different mode's framelength
		.startx= 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1296,		//record different mode's width of grabwindow
		.grabwindow_height = 972,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 14,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
  .custom1 = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
  .custom2 = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
  .custom3 = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
  .custom4 = {
		.pclk = 176000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},
  .custom5 = {
		.pclk = 440000000,				//record different mode's pclk
		.linelength = 4592,				//record different mode's linelength
		.framelength =3188, //3168,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2096,		//record different mode's width of grabwindow
		.grabwindow_height = 1552,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,	
	},

	.margin = 6,			//sensor framelength & shutter margin
	.min_shutter = 6,		//min shutter
	.max_frame_length = 0x7FFF,//REG0x0202 <=REG0x0340-5//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,	//shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 3,	  //support sensor mode num ,don't support Slow motion
	
	.cap_delay_frame = 2,		//enter capture delay frame num
	.pre_delay_frame = 2, 		//enter preview delay frame num
	.video_delay_frame = 2,		//enter video delay frame num
	.hs_video_delay_frame = 3,	//enter high speed video  delay frame num
	.slim_video_delay_frame = 3,//enter slim video delay frame num
    .custom1_delay_frame = 2,
    .custom2_delay_frame = 2, 
    .custom3_delay_frame = 2, 
    .custom4_delay_frame = 2, 
    .custom5_delay_frame = 2,
	
	.isp_driving_current = ISP_DRIVING_8MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = 1,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_2_LANE,//mipi lane num
	.i2c_addr_table = {0x40,0x50,0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
    .i2c_speed = 400, // i2c read/write speed
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_V_MIRROR,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x200,					//current shutter
	.gain = 0x200,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0,//record current sensor's i2c write id
};


/* Sensor output window information*/

static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =	 
{
 { 2592, 1944,	  0,    0, 2592, 1944, 1296,  972,	 0, 0, 1296,  972,	 0, 0, 1296,  972}, // preview 
 { 2592, 1944,	  0,  	0, 2592, 1944, 2592, 1944,   0,	0, 2592, 1944, 	 0, 0, 2592, 1944}, // capture 
 { 2592, 1944,	  0,  	0, 2592, 1944, 2592, 1944,   0,	0, 2592, 1944, 	 0, 0, 2592, 1944}, // video 
 { 2592, 1944,	  0,  	0, 2592, 1944, 1296,  972,   0,	0,  640,  480, 	 0, 0,  640,  480}, // hight speed video
 { 2592, 1944,	  0,  	0, 2592, 1944, 1296,  972,   0,	0, 1296,  972, 	 0, 0, 1296,  972}, // slim
};



static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
    return ((get_byte<<8)&0xFF00)|((get_byte>>8)&0x00FF);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor(0x0006, imgsensor.frame_length & 0xFFFF);	  
	write_cmos_sensor(0x0008, imgsensor.line_length & 0xFFFF);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	LOG_INF("framerate = %d, min framelength should enable(%d) \n", framerate,min_framelength_en);
   
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

#if 0
static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;
	//kal_uint32 frame_length = 0;
	   
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */
	
	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
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
	
	if (imgsensor.autoflicker_en) { 
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);	
	} else {
		// Extend frame length
		write_cmos_sensor(0X0006, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0x0004, (shutter) & 0xFFFF);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

	//LOG_INF("frame_length = %d ", frame_length);
	
}	/*	write_shutter  */
#endif


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
	kal_uint16 realtime_fps = 0;
	//kal_uint32 frame_length = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	
	//write_shutter(shutter);
	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */
	
	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
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
	
	if (imgsensor.autoflicker_en) { 
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);	
		else {
		// Extend frame length
		write_cmos_sensor(0x0046, 0x0100); //group para hold on
		write_cmos_sensor(0x0006, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		write_cmos_sensor(0x0046, 0x0100); //group para hold on
		// Extend frame length
		write_cmos_sensor(0x0006, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0X0074, shutter & 0xFFFF);
	write_cmos_sensor(0x0046, 0x0000); //group para hold off
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;
	//gain = 64 = 1x real gain.
	reg_gain = gain / 4 - 16;
	//reg_gain = reg_gain & 0xFFFF;
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
	
	LOG_INF("set_gain %d \n", gain);
  //gain = 64 = 1x real gain.
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
	write_cmos_sensor(0x0046, 0x0100); //group para hold o
	write_cmos_sensor(0x0076, (reg_gain&0xFFFF)); 
	write_cmos_sensor(0x0046, 0x0000); //group para hold off   
	return gain;
}	/*	set_gain  */

//[TODO]ihdr_write_shutter_gain not support for hi556
static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
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
		write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);

		write_cmos_sensor(0x3502, (le << 4) & 0xFF);
		write_cmos_sensor(0x3501, (le >> 4) & 0xFF);	 
		write_cmos_sensor(0x3500, (le >> 12) & 0x0F);
		
		write_cmos_sensor(0x3512, (se << 4) & 0xFF); 
		write_cmos_sensor(0x3511, (se >> 4) & 0xFF);
		write_cmos_sensor(0x3510, (se >> 12) & 0x0F); 

		set_gain(gain);
	}

}

#if 0
//[TODO]
static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);

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
			write_cmos_sensor(0x000e,0X0000); //B
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x000e,0X0100); //Gb
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x000e,0X0200); //Gr	
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x000e,0X0300); //R
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
	}

}

#endif
#ifdef HI556_OTP_FUNCTION
//#if 0
#if 1
static kal_uint16 read_cmos_sensor_byte(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
    return get_byte;
}
#endif
#if 1
static void write_cmos_sensor_byte(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

    kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}
#endif
struct HI556_otp_struct 
{
	int info_Flag;	//bit[7]:info, bit[6]:wb
	int info_data[20];
	int WB_FLAG;
	int wb_data[13];
	int RG_ratio_unit;
	int BG_ratio_unit;
	int LSC_FLAG;/*guoameng add lSC-shading read flag*/
	char lsc_data[1869];/*add lsc-shading data struct-3739-lsc-all-data*/
};

struct HI556_otp_struct HI556_otp;


void HI556_OTPSetting(void)
{	
	LOG_INF("HI556OTPSetting begin:\n ");
	write_cmos_sensor_byte(0x0A02, 0x01); //fast sleep On
	write_cmos_sensor_byte(0x0A00, 0x00); //sleep On
	mDELAY(10);
	write_cmos_sensor_byte(0x0F02, 0x00); //pll disable
	write_cmos_sensor_byte(0x011A, 0x01); //CP TRI_H
	write_cmos_sensor_byte(0x011B, 0x09); //IPGM TRIM_H
	write_cmos_sensor_byte(0x0D04, 0x01); //Fsync Output enable
	write_cmos_sensor_byte(0x0D00, 0x07); //Fsync Output Drivability
	write_cmos_sensor_byte(0x003E, 0x10); //OTP R/W
	write_cmos_sensor_byte(0x0a00, 0x01); //sleep off
	LOG_INF("HI556OTPSetting exit :\n ");
}

static kal_uint16 OTP_read_cmos_sensor(kal_uint16 otp_addr)
{
    kal_uint16 data;

    write_cmos_sensor_byte(0x10a, (otp_addr & 0xFF00) >> 8 ); //start address H        
    write_cmos_sensor_byte(0x10b, otp_addr & 0xFF); //start address L
    write_cmos_sensor_byte(0x102, 0x01); //single read
    	//mDELAY(10);
    data = read_cmos_sensor_byte(0x108); //OTP data read
	return data;
}

static int HI556_otp_read(void)
{
	int k;
	int i = 0, info_start_addr = 0, wb_start_addr = 0;
	int  checksum = 0 ,info_check = 0, wb_check = 0;
	int lsc_start_addr=0;
	int checksum_lsc=0;
	LOG_INF("HI556 HI556_otp_read \n");
//module info read 
	printk("guoameng-OTP_read_cmos_sensor(0x0401) = 0x%x \n", OTP_read_cmos_sensor(0x0401));
	HI556_otp.info_Flag = OTP_read_cmos_sensor(0x0401);
	printk("guoameng-HI556 HI556_otp.info_Flag = 0x%x \n", HI556_otp.info_Flag);
	if (HI556_otp.info_Flag == 0x01)	//Base Info Group1 valid
		info_start_addr = 0x0402;
	else if (HI556_otp.info_Flag == 0x07)	//Base Info Group2 valid
		info_start_addr = 0x0416;
	else if (HI556_otp.info_Flag == 0x1f)	//Base Info Group3 valid
		info_start_addr = 0x042A;
	else
		info_start_addr = 0;
	
	LOG_INF("HI556 info_start_addr = 0x%x \n", info_start_addr);
	LOG_INF("HI556 info_Flag = 0x%x \n", HI556_otp.info_Flag);

	if (info_start_addr != 0) {
		write_cmos_sensor_byte(0x10a, (info_start_addr >> 8) & 0xff); //start addr H
		write_cmos_sensor_byte(0x10b, info_start_addr & 0xff);	//start addr L
		write_cmos_sensor_byte(0x102, 0x01);	//single mode
		for (i = 0; i < 20; i++) {
			HI556_otp.info_data[i] = read_cmos_sensor_byte(0x108); //otp data read
			//LOG_INF("guoameng-HI556 info_data[%d] = 0x%x \n ", i, HI556_otp.info_data[i]);
		}
	}

	for(i=0;i<19;i++)
	{
	checksum += HI556_otp.info_data[i];
	}
	checksum =checksum % 0xff;
	info_check = HI556_otp.info_data[19]&0xff;
	if (checksum == info_check)
		{
		LOG_INF("HI556_Sensor: Module information checksum PASS\n ");
		}
	else
		{
		LOG_INF("HI556_Sensor: Module information checksum Fail\n ");
		}
	LOG_INF("guoameng-HI556_module_info checksum = 0x%x ,info_check = 0x%x \n", checksum,info_check);

//wb read	
	printk("guoameng-OTP_read_cmos_sensor(0x043E) = 0x%x \n", OTP_read_cmos_sensor(0x043E));
	HI556_otp.WB_FLAG = OTP_read_cmos_sensor(0x043E);
	if (HI556_otp.WB_FLAG == 0x01)
		wb_start_addr = 0x043f;
	else if (HI556_otp.WB_FLAG == 0x07)
		wb_start_addr = 0x044c;
	else if (HI556_otp.WB_FLAG == 0x1f)
		wb_start_addr = 0x0459;
	else
		LOG_INF("HI556 WB data invalid \n");

LOG_INF("guoameng-HI556 WB_FLAG = 0x%x \n", HI556_otp.WB_FLAG);
LOG_INF("guoameng-HI556 wb_start_addr = 0x%x \n", wb_start_addr);

	if (wb_start_addr != 0) {
		write_cmos_sensor_byte(0x10a, (wb_start_addr >> 8) & 0xff);	//start addr H
		write_cmos_sensor_byte(0x10b, wb_start_addr & 0xff);	//start addr L
		write_cmos_sensor_byte(0x102, 0x01);	//single mode
		for (i = 0; i < 13; i++) {
			HI556_otp.wb_data[i] = read_cmos_sensor_byte(0x108);	//otp data read
			//LOG_INF("guoameng-wb_data[%d] = 0x%x \n ", i, HI556_otp.wb_data[i]);
		}
	}
	
	checksum = 0;
	for(i=0;i<12;i++)
	{
	checksum += HI556_otp.wb_data[i];
	}
	checksum =checksum % 0xff;
	wb_check =  HI556_otp.wb_data[12]&0xff;

	if (checksum == wb_check)
			{
			LOG_INF("HI556_Sensor: WB checksum PASS\n ");
			}
	else
			{
			LOG_INF("HI556_Sensor: WB checksum Fail\n ");
			}
	LOG_INF("guoameng-HI556_awb checksum = 0x%x ,wb_check = 0x%x \n", checksum,wb_check);
	//lsc start
	HI556_otp.LSC_FLAG = OTP_read_cmos_sensor(0x0466);
	if (HI556_otp.LSC_FLAG == 0x01)
	  lsc_start_addr = 0x467;
	else if (HI556_otp.LSC_FLAG == 0x07)
	  lsc_start_addr = 0xbb4;
	else if (HI556_otp.LSC_FLAG == 0x1F)
	  lsc_start_addr = 0x1301;
	else
	  LOG_INF("HI556 LSC data invalid \n");
	LOG_INF("HI556 lsc_FLAG = 0x%x \n", HI556_otp.LSC_FLAG);
	LOG_INF("HI556 lsc_start_addr = 0x%x \n", lsc_start_addr);
	if (lsc_start_addr != 0) {
		write_cmos_sensor_byte(0x10a, (lsc_start_addr >> 8) & 0xff);	//start addr H
		write_cmos_sensor_byte(0x10b, lsc_start_addr & 0xff);	//start addr L
		write_cmos_sensor_byte(0x102, 0x01);	//single mode
		for (k = 0; k < 1869; k++) {
			HI556_otp.lsc_data[k] = read_cmos_sensor_byte(0x108);	//otp data read
		//	printk("guoameng-845-HI556 lsc_data[%d] = 0x%x\n", k, HI556_otp.lsc_data[k]);
			checksum_lsc += HI556_otp.lsc_data[k];
		}

		printk("HI556 xljadd checksum_lsc = 0x%x\n", checksum_lsc);
		checksum_lsc = (checksum_lsc - HI556_otp.lsc_data[1868])%255;
		printk("HI556 xljadd after calculator checksum_lsc = 0x%x\n", checksum_lsc);
	}
//return HI556_otp.LSC_FLAG;
	//lsc end

	write_cmos_sensor_byte(0x0a00, 0x00); //sleep On
	mdelay(10);
    write_cmos_sensor_byte(0x003e, 0x00); //OTP mode off
    write_cmos_sensor_byte(0x0a00, 0x01); //sleep Off

	return HI556_otp.WB_FLAG;
}

static int HI556_otp_apply(void)
{
	int R_gain = 1, G_gain = 1, B_gain = 1;
	int RG_ratio_golden = ((HI556_otp.wb_data[6] << 8) | HI556_otp.wb_data[7]);
	int BG_ratio_golden = ((HI556_otp.wb_data[8] << 8) | HI556_otp.wb_data[9]);
	//int RG_ratio_golden = 0x1fd; //module house provide
	//int BG_ratio_golden = 0x24e; //module house provide
	LOG_INF("HI556 HI556_otp_apply \n");
	printk("guoameng-HI556_otp_apply-778-RG_ratio_golden=0x%x,BG_ratio_golden=0x%x\n",RG_ratio_golden,BG_ratio_golden);
	HI556_otp.RG_ratio_unit = ((HI556_otp.wb_data[0] << 8) | HI556_otp.wb_data[1]) & 0x03FF;
	HI556_otp.BG_ratio_unit = ((HI556_otp.wb_data[2] << 8) | HI556_otp.wb_data[3]) & 0x03FF;


	R_gain = (0x100 * RG_ratio_golden / HI556_otp.RG_ratio_unit);
	B_gain = (0x100 * BG_ratio_golden / HI556_otp.BG_ratio_unit);
	G_gain = 0x100;
	printk("guoameng-HI556_otp_apply-782-R_gain=%d,B_gain=%d\n",R_gain,B_gain);

	if (R_gain < B_gain) {
		if(R_gain < 0x100) {
			B_gain =0x100 *  B_gain / R_gain;
			G_gain =0x100 *  G_gain / R_gain;
			R_gain = 0x100;
		printk("guoameng-HI556_otp_apply-789-R_gain=%d,B_gain=%d\n",R_gain,B_gain);
		}
	} else {
		if (B_gain < 0x100) {
			R_gain = 0x100 * R_gain / B_gain;
			G_gain = 0x100 * G_gain / B_gain;
			B_gain = 0x100;
			printk("guoameng-HI556_otp_apply-796-R_gain=%d,B_gain=%d\n",R_gain,B_gain);
		}
	}
	//R_gain = 0x11111111;


    
LOG_INF("HI556 Before apply otp G_gain = 0x%x, R_gain = 0x%x, B_gain = 0x%x \n",\
	read_cmos_sensor(0x0078) , read_cmos_sensor(0x007c), read_cmos_sensor(0x007e));
	write_cmos_sensor(0x0078, G_gain);
	write_cmos_sensor(0x007a, G_gain);
	write_cmos_sensor(0x007c, R_gain);
	write_cmos_sensor(0x007e, B_gain);
LOG_INF("HI556 After apply otp G_gain = 0x%x, R_gain = 0x%x, B_gain = 0x%x \n",\
	read_cmos_sensor(0x0078) , read_cmos_sensor(0x007c), read_cmos_sensor(0x007e)) ;

	return HI556_otp.WB_FLAG;
}

//add by ameng read lsc start
#if 0
static int hi556_otp_lsc_read(void)
{
	int lsc_start_addr=0;
	int  checksum_lsc = 0;
	hi556_otp.LSC_FLAG = OTP_read_cmos_sensor(0x0466);
	if (hi556_otp.LSC_FLAG == 0x01)
	  lsc_start_addr = 0x467;
	else if (hi556_otp.LSC_FLAG == 0x07)
	  lsc_start_addr = 0xbb4;
	else if (hi556_otp.LSC_FLAG == 0x1F)
	  lsc_start_addr = 0x1301;
	else
	  LOG_INF("HI556 LSC data invalid \n");
	LOG_INF("HI556 lsc_FLAG = 0x%x \n", hi556_otp.LSC_FLAG);
	LOG_INF("HI556 lsc_start_addr = 0x%x \n", lsc_start_addr);
	if (lsc_start_addr != 0) {
		write_cmos_sensor_byte(0x10a, (lsc_start_addr >> 8) & 0xff);	//start addr H
		write_cmos_sensor_byte(0x10b, lsc_start_addr & 0xff);	//start addr L
		write_cmos_sensor_byte(0x102, 0x01);	//single mode
		for (i = 0; i < 1869; i++) {
			hi556_otp.lsc_data[i] = read_cmos_sensor_byte(0x108);	//otp data read
			//printk("HI556 lsc_data[%d] = 0x%x\n", i, hi556_otp.lsc_data[i]);
			checksum_lsc += hi556_otp.lsc_data[i];
		}

		printk("HI556 xljadd checksum_lsc = 0x%x\n", checksum_lsc);
		checksum_lsc = (checksum_lsc - hi556_otp.lsc_data[1868])%255;
		printk("HI556 xljadd after calculator checksum_lsc = 0x%x\n", checksum_lsc);
	}
return hi556_otp.LSC_FLAG;
}
#endif
#if 1
unsigned int HI556_OTP_Read_Lsc(unsigned int addr,unsigned char *data, unsigned int size)
{
	LOG_INF(" start copy lsc_data size = %d\n",size);  

	LOG_INF(" hi553_otp.lsc_data[1868]=0x%x\n",HI556_otp.lsc_data[1868]);    		
	memcpy(data,HI556_otp.lsc_data,size);
	return size;
}
#endif
//add by ameng read lsc end

static void HI556_otp_cali(void)
{
	LOG_INF("HI556 otp_cali \n");
	
		HI556_OTPSetting();

        HI556_otp_read();
    
        HI556_otp_apply();

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
	//Sensor Information////////////////////////////
	//Sensor	  : Hi-556                            
	//Date		  : 2017-02-20                        
	//AP chipset      : MTK                                       
	//MCLK	          : 24MHz                   
	//MIPI            : 2 Lane                                   
	//Pixel order 	  : B 1st            
	//BLC offset	  : 64code                       
	//Setting Ver.    : v06                         
	////////////////////////////////////////////////

	write_cmos_sensor(0x0a00, 0x0000); //stream off
	write_cmos_sensor(0x0e00, 0x0102); 
	write_cmos_sensor(0x0e02, 0x0102); 
	write_cmos_sensor(0x0e0c, 0x0100); 
	write_cmos_sensor(0x2000, 0x7400);
	write_cmos_sensor(0x2002, 0x001c);
	write_cmos_sensor(0x2004, 0x0242);
	write_cmos_sensor(0x2006, 0x0942);
	write_cmos_sensor(0x2008, 0x7007);
	write_cmos_sensor(0x200a, 0x0fd9);
	write_cmos_sensor(0x200c, 0x0259);
	write_cmos_sensor(0x200e, 0x7008);
	write_cmos_sensor(0x2010, 0x160e);
	write_cmos_sensor(0x2012, 0x0047);
	write_cmos_sensor(0x2014, 0x2118);
	write_cmos_sensor(0x2016, 0x0041);
	write_cmos_sensor(0x2018, 0x00d8);
	write_cmos_sensor(0x201a, 0x0145);
	write_cmos_sensor(0x201c, 0x0006);
	write_cmos_sensor(0x201e, 0x0181);
	write_cmos_sensor(0x2020, 0x13cc);
	write_cmos_sensor(0x2022, 0x2057);
	write_cmos_sensor(0x2024, 0x7001);
	write_cmos_sensor(0x2026, 0x0fca);
	write_cmos_sensor(0x2028, 0x00cb);
	write_cmos_sensor(0x202a, 0x009f);
	write_cmos_sensor(0x202c, 0x7002);
	write_cmos_sensor(0x202e, 0x13cc);
	write_cmos_sensor(0x2030, 0x019b);
	write_cmos_sensor(0x2032, 0x014d);
	write_cmos_sensor(0x2034, 0x2987);
	write_cmos_sensor(0x2036, 0x2766);
	write_cmos_sensor(0x2038, 0x0020);
	write_cmos_sensor(0x203a, 0x2060);
	write_cmos_sensor(0x203c, 0x0e5d);
	write_cmos_sensor(0x203e, 0x181d);
	write_cmos_sensor(0x2040, 0x2066);
	write_cmos_sensor(0x2042, 0x20c4);
	write_cmos_sensor(0x2044, 0x5000);
	write_cmos_sensor(0x2046, 0x0005);
	write_cmos_sensor(0x2048, 0x0000);
	write_cmos_sensor(0x204a, 0x01db);
	write_cmos_sensor(0x204c, 0x025a);
	write_cmos_sensor(0x204e, 0x00c0);
	write_cmos_sensor(0x2050, 0x0005);
	write_cmos_sensor(0x2052, 0x0006);
	write_cmos_sensor(0x2054, 0x0ad9);
	write_cmos_sensor(0x2056, 0x0259);
	write_cmos_sensor(0x2058, 0x0618);
	write_cmos_sensor(0x205a, 0x0258);
	write_cmos_sensor(0x205c, 0x2266);
	write_cmos_sensor(0x205e, 0x20c8);
	write_cmos_sensor(0x2060, 0x2060);
	write_cmos_sensor(0x2062, 0x707b);
	write_cmos_sensor(0x2064, 0x0fdd);
	write_cmos_sensor(0x2066, 0x81b8);
	write_cmos_sensor(0x2068, 0x5040);
	write_cmos_sensor(0x206a, 0x0020);
	write_cmos_sensor(0x206c, 0x5060);
	write_cmos_sensor(0x206e, 0x3143);
	write_cmos_sensor(0x2070, 0x5081);
	write_cmos_sensor(0x2072, 0x025c);
	write_cmos_sensor(0x2074, 0x7800);
	write_cmos_sensor(0x2076, 0x7400);
	write_cmos_sensor(0x2078, 0x001c);
	write_cmos_sensor(0x207a, 0x0242);
	write_cmos_sensor(0x207c, 0x0942);
	write_cmos_sensor(0x207e, 0x0bd9);
	write_cmos_sensor(0x2080, 0x0259);
	write_cmos_sensor(0x2082, 0x7008);
	write_cmos_sensor(0x2084, 0x160e);
	write_cmos_sensor(0x2086, 0x0047);
	write_cmos_sensor(0x2088, 0x2118);
	write_cmos_sensor(0x208a, 0x0041);
	write_cmos_sensor(0x208c, 0x00d8);
	write_cmos_sensor(0x208e, 0x0145);
	write_cmos_sensor(0x2090, 0x0006);
	write_cmos_sensor(0x2092, 0x0181);
	write_cmos_sensor(0x2094, 0x13cc);
	write_cmos_sensor(0x2096, 0x2057);
	write_cmos_sensor(0x2098, 0x7001);
	write_cmos_sensor(0x209a, 0x0fca);
	write_cmos_sensor(0x209c, 0x00cb);
	write_cmos_sensor(0x209e, 0x009f);
	write_cmos_sensor(0x20a0, 0x7002);
	write_cmos_sensor(0x20a2, 0x13cc);
	write_cmos_sensor(0x20a4, 0x019b);
	write_cmos_sensor(0x20a6, 0x014d);
	write_cmos_sensor(0x20a8, 0x2987);
	write_cmos_sensor(0x20aa, 0x2766);
	write_cmos_sensor(0x20ac, 0x0020);
	write_cmos_sensor(0x20ae, 0x2060);
	write_cmos_sensor(0x20b0, 0x0e5d);
	write_cmos_sensor(0x20b2, 0x181d);
	write_cmos_sensor(0x20b4, 0x2066);
	write_cmos_sensor(0x20b6, 0x20c4);
	write_cmos_sensor(0x20b8, 0x50a0);
	write_cmos_sensor(0x20ba, 0x0005);
	write_cmos_sensor(0x20bc, 0x0000);
	write_cmos_sensor(0x20be, 0x01db);
	write_cmos_sensor(0x20c0, 0x025a);
	write_cmos_sensor(0x20c2, 0x00c0);
	write_cmos_sensor(0x20c4, 0x0005);
	write_cmos_sensor(0x20c6, 0x0006);
	write_cmos_sensor(0x20c8, 0x0ad9);
	write_cmos_sensor(0x20ca, 0x0259);
	write_cmos_sensor(0x20cc, 0x0618);
	write_cmos_sensor(0x20ce, 0x0258);
	write_cmos_sensor(0x20d0, 0x2266);
	write_cmos_sensor(0x20d2, 0x20c8);
	write_cmos_sensor(0x20d4, 0x2060);
	write_cmos_sensor(0x20d6, 0x707b);
	write_cmos_sensor(0x20d8, 0x0fdd);
	write_cmos_sensor(0x20da, 0x86b8);
	write_cmos_sensor(0x20dc, 0x50e0);
	write_cmos_sensor(0x20de, 0x0020);
	write_cmos_sensor(0x20e0, 0x5100);
	write_cmos_sensor(0x20e2, 0x3143);
	write_cmos_sensor(0x20e4, 0x5121);
	write_cmos_sensor(0x20e6, 0x7800);
	write_cmos_sensor(0x20e8, 0x3140);
	write_cmos_sensor(0x20ea, 0x01c4);
	write_cmos_sensor(0x20ec, 0x01c1);
	write_cmos_sensor(0x20ee, 0x01c0);
	write_cmos_sensor(0x20f0, 0x01c4);
	write_cmos_sensor(0x20f2, 0x2700);
	write_cmos_sensor(0x20f4, 0x3d40);
	write_cmos_sensor(0x20f6, 0x7800);
	write_cmos_sensor(0x20f8, 0xffff);
	write_cmos_sensor(0x27fe, 0xe000);
	write_cmos_sensor(0x3000, 0x60f8);
	write_cmos_sensor(0x3002, 0x187f);
	write_cmos_sensor(0x3004, 0x7060);
	write_cmos_sensor(0x3006, 0x0114);
	write_cmos_sensor(0x3008, 0x60b0);
	write_cmos_sensor(0x300a, 0x1473);
	write_cmos_sensor(0x300c, 0x0013);
	write_cmos_sensor(0x300e, 0x140f);
	write_cmos_sensor(0x3010, 0x0040);
	write_cmos_sensor(0x3012, 0x100f);
	write_cmos_sensor(0x3014, 0x60f8);
	write_cmos_sensor(0x3016, 0x187f);
	write_cmos_sensor(0x3018, 0x7060);
	write_cmos_sensor(0x301a, 0x0114);
	write_cmos_sensor(0x301c, 0x60b0);
	write_cmos_sensor(0x301e, 0x1473);
	write_cmos_sensor(0x3020, 0x0013);
	write_cmos_sensor(0x3022, 0x140f);
	write_cmos_sensor(0x3024, 0x0040);
	write_cmos_sensor(0x3026, 0x000f);

	write_cmos_sensor(0x0b00, 0x0000); 
	write_cmos_sensor(0x0b02, 0x0045); 
	write_cmos_sensor(0x0b04, 0xb405); 
	write_cmos_sensor(0x0b06, 0xc403); 
	write_cmos_sensor(0x0b08, 0x0081); 
	write_cmos_sensor(0x0b0a, 0x8252); 
	write_cmos_sensor(0x0b0c, 0xf814); 
	write_cmos_sensor(0x0b0e, 0xc618); 
	write_cmos_sensor(0x0b10, 0xa828); 
	write_cmos_sensor(0x0b12, 0x004c); 
	write_cmos_sensor(0x0b14, 0x4068); 
	write_cmos_sensor(0x0b16, 0x0000); 
	write_cmos_sensor(0x0f30, 0x6e25); //pll                                   
	write_cmos_sensor(0x0f32, 0x7067); //pll                                   
	write_cmos_sensor(0x0954, 0x0009);                                         
	write_cmos_sensor(0x0956, 0x1100);                                         
	write_cmos_sensor(0x0958, 0xcc80);                                         
	write_cmos_sensor(0x095a, 0x0000);                                         
	write_cmos_sensor(0x0c00, 0x1110);                                         
	write_cmos_sensor(0x0c02, 0x0011);                                         
	write_cmos_sensor(0x0c04, 0x0000);                                         
	write_cmos_sensor(0x0c06, 0x0200);                                         
	write_cmos_sensor(0x0c10, 0x0040); //OB                                    
	write_cmos_sensor(0x0c12, 0x0040); //OB                                    
	write_cmos_sensor(0x0c14, 0x0040); //OB                                    
	write_cmos_sensor(0x0c16, 0x0040); //OB                                    
	write_cmos_sensor(0x0a10, 0x4000); //pedestal_data                         
	write_cmos_sensor(0x3068, 0xf800);                                         
	write_cmos_sensor(0x306a, 0xf876);                                         
	write_cmos_sensor(0x006c, 0x0000);                                         
	write_cmos_sensor(0x005e, 0x0200);                                         
	write_cmos_sensor(0x000e, 0x0200); //image orient                          
	write_cmos_sensor(0x0e0a, 0x0001);                                         
	write_cmos_sensor(0x004a, 0x0100);                                         
	write_cmos_sensor(0x004c, 0x0000);                                         
	write_cmos_sensor(0x004e, 0x0100);                                         
	write_cmos_sensor(0x000c, 0x0022);                                         
	write_cmos_sensor(0x0008, 0x0b00); //line length pck 2816                  
	write_cmos_sensor(0x005a, 0x0202);                                         
	write_cmos_sensor(0x0012, 0x000e);                                         
	write_cmos_sensor(0x0018, 0x0a31);                                         
	write_cmos_sensor(0x0022, 0x0008);                                         
	write_cmos_sensor(0x0028, 0x0017);                                         
	write_cmos_sensor(0x0024, 0x0028);                                         
	write_cmos_sensor(0x002a, 0x002d);                                         
	write_cmos_sensor(0x0026, 0x0030);                                         
	write_cmos_sensor(0x002c, 0x07c7);                                         
	write_cmos_sensor(0x002e, 0x1111);                                         
	write_cmos_sensor(0x0030, 0x1111);                                         
	write_cmos_sensor(0x0032, 0x1111);                                         
	write_cmos_sensor(0x0006, 0x0801); //frame length lines min 0x07BC         
	write_cmos_sensor(0x0a22, 0x0000);                                         
	write_cmos_sensor(0x0a12, 0x0a20); //x output size 2592                    
	write_cmos_sensor(0x0a14, 0x0798); //y output size 1944                    
	write_cmos_sensor(0x003e, 0x0000);                                         
	write_cmos_sensor(0x0074, 0x07ff); //coarse integ time                     
	write_cmos_sensor(0x0070, 0x0411);                                         
	write_cmos_sensor(0x0002, 0x0000);                                         
	write_cmos_sensor(0x0a02, 0x0100);                                         
	write_cmos_sensor(0x0a24, 0x0100);                                         
	write_cmos_sensor(0x0076, 0x0000); //analog gain 1x                        
	write_cmos_sensor(0x0060, 0x0000);                                         
	write_cmos_sensor(0x0062, 0x0530);                                         
	write_cmos_sensor(0x0064, 0x0500);                                         
	write_cmos_sensor(0x0066, 0x0530);                                         
	write_cmos_sensor(0x0068, 0x0500);                                         
	write_cmos_sensor(0x0122, 0x0300);                                         
	write_cmos_sensor(0x015a, 0xff08);                                         
	write_cmos_sensor(0x0804, 0x0200);                                         
	write_cmos_sensor(0x005c, 0x0102);                                         
	write_cmos_sensor(0x0a1a, 0x0800); //DGain pedestal enable
	write_cmos_sensor(0x0a00, 0x0100); //stream off    
}

static void preview_setting(void)
{
	LOG_INF("E\n");
	
	//Sensor Information////////////////////////////
	//Sensor	  : Hi-556
	//Date		  : 2016-10-19
	//Customer		  : MTK_validation
	//Image size	  : 2592x1944
	//MCLK		  : 24MHz
	//MIPI speed(Mbps): 880Mbps x 2Lane
	//Frame Length	  : 4166
	//Line Length	  : 2816
	//Max Fps	  : 15.0fps
	//Pixel order	  : Green 1st (=GB)
	//X/Y-flip	  : X-flip
	//BLC offset	  : 64code
	////////////////////////////////////////////////

	write_cmos_sensor(0x0a00, 0x0000);
	write_cmos_sensor(0x0b0a, 0x8259);
	write_cmos_sensor(0x0f30, 0x6e25); //pll                                  
	write_cmos_sensor(0x0f32, 0x7167); //pll                                  
	write_cmos_sensor(0x004a, 0x0100);                                        
	write_cmos_sensor(0x004c, 0x0000);                                        
	write_cmos_sensor(0x004e, 0x0000); //per-frame control off, on 0x0100     
	write_cmos_sensor(0x000c, 0x0122);                                        
	write_cmos_sensor(0x0008, 0x0b00); //line length pck 2816                 
	write_cmos_sensor(0x005a, 0x0404);                                        
	write_cmos_sensor(0x0012, 0x000c);                                        
	write_cmos_sensor(0x0018, 0x0a33);                                        
	write_cmos_sensor(0x0022, 0x0008);                                        
	write_cmos_sensor(0x0028, 0x0017);                                        
	write_cmos_sensor(0x0024, 0x0022);                                        
	write_cmos_sensor(0x002a, 0x002b);                                        
	write_cmos_sensor(0x0026, 0x0030);                                        
	write_cmos_sensor(0x002c, 0x07c7);                                        
	write_cmos_sensor(0x002e, 0x3311);                                        
	write_cmos_sensor(0x0030, 0x3311);                                        
	write_cmos_sensor(0x0032, 0x3311);                                        
	write_cmos_sensor(0x0006, 0x0801); //frame length lines 2049              
	write_cmos_sensor(0x0a22, 0x0000);                                        
	write_cmos_sensor(0x0a12, 0x0510); //x output size 2592                   
	write_cmos_sensor(0x0a14, 0x03cc); //y output size 1944                   
	write_cmos_sensor(0x003e, 0x0000);                                        
	write_cmos_sensor(0x0804, 0x0200);                                        
	write_cmos_sensor(0x0a04, 0x016a); //isp_en                               
	write_cmos_sensor(0x090e, 0x0010); //mipi_vblank_delay                    
	write_cmos_sensor(0x090c, 0x09c0); //mipi_hblank_delay                    
	write_cmos_sensor(0x0902, 0x4319); //mipi_tx_op_mode1, mipi_tx_op_mode2   
	write_cmos_sensor(0x0914, 0xc106); //mipi_exit_seq, tlpx                  
	write_cmos_sensor(0x0916, 0x040e); //tclk_prepare, tclk_zero              
	write_cmos_sensor(0x0918, 0x0304); //tclk_pre, ths_prepare                
	write_cmos_sensor(0x091a, 0x0709); //ths_zero, ths_trail                  
	write_cmos_sensor(0x091c, 0x0e06); //tclk_post, tclk_trail                
	write_cmos_sensor(0x091e, 0x0300); //mipi_exit, null  
	write_cmos_sensor(0x0a00, 0x0100); //stream off



}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	if (currefps == 300) {
		
	//Sensor Information////////////////////////////
	//Sensor	  : Hi-556
	//Date		  : 2017-02-20
	//AP chipset      : MTK 
	//Image size	  : 2592x1944
	//MCLK		  : 24MHz
	//MIPI speed(Mbps): 880Mbps x 2Lane
	//Frame Length	  : 2049
	//Line Length	  : 2816
	//Max Fps	  : 30.5fps
	//Pixel order	  : Green 1st (=GB)
	//X/Y-flip	  : X-flip
	//BLC offset	  : 64code
	////////////////////////////////////////////////

	write_cmos_sensor(0x0a00, 0x0000);
	write_cmos_sensor(0x0b0a, 0x8252);
	write_cmos_sensor(0x0f30, 0x6e25); //pll                                  
	write_cmos_sensor(0x0f32, 0x7067); //pll                                  
	write_cmos_sensor(0x004a, 0x0100);                                        
	write_cmos_sensor(0x004c, 0x0000);                                        
	write_cmos_sensor(0x004e, 0x0000); //per-frame control off, on 0x0100      
	write_cmos_sensor(0x000c, 0x0022);                                        
	write_cmos_sensor(0x0008, 0x0b00); //line length pck 2816                 
	write_cmos_sensor(0x005a, 0x0202);                                        
	write_cmos_sensor(0x0012, 0x000e);                                        
	write_cmos_sensor(0x0018, 0x0a31);                                        
	write_cmos_sensor(0x0022, 0x0008);                                        
	write_cmos_sensor(0x0028, 0x0017);                                        
	write_cmos_sensor(0x0024, 0x0028);                                        
	write_cmos_sensor(0x002a, 0x002d);                                        
	write_cmos_sensor(0x0026, 0x0030);                                        
	write_cmos_sensor(0x002c, 0x07c7);                                        
	write_cmos_sensor(0x002e, 0x1111);                                        
	write_cmos_sensor(0x0030, 0x1111);                                        
	write_cmos_sensor(0x0032, 0x1111);                                        
	write_cmos_sensor(0x0006, 0x0801); //frame length lines 2049              
	write_cmos_sensor(0x0a22, 0x0000);                                        
	write_cmos_sensor(0x0a12, 0x0a20); //x output size 2592                   
	write_cmos_sensor(0x0a14, 0x0798); //y output size 1944                   
	write_cmos_sensor(0x003e, 0x0000);                                        
	write_cmos_sensor(0x0804, 0x0200);                                        
	write_cmos_sensor(0x0a04, 0x014a); //isp_en                               
	write_cmos_sensor(0x090c, 0x0fdc); //mipi_vblank_delay                    
	write_cmos_sensor(0x090e, 0x002d); //mipi_hblank_delay                    
	write_cmos_sensor(0x0902, 0x4319); //mipi_tx_op_mode1, mipi_tx_op_mode2   
	write_cmos_sensor(0x0914, 0xc10a); //mipi_exit_seq, tlpx                  
	write_cmos_sensor(0x0916, 0x071f); //tclk_prepare, tclk_zero              
	write_cmos_sensor(0x0918, 0x0408); //tclk_pre, ths_prepare                
	write_cmos_sensor(0x091a, 0x0c0d); //ths_zero, ths_trail                  
	write_cmos_sensor(0x091c, 0x0f09); //tclk_post, tclk_trail                
	write_cmos_sensor(0x091e, 0x0a00); //mipi_exit, null  
	write_cmos_sensor(0x0a00, 0x0100); //stream off

	}
	//else if (currefps == 240) {}
	else if (currefps == 150) {	

	//Sensor Information////////////////////////////
	//Sensor	  : Hi-556
	//Date		  : 2017-02-20
	//AP chipset      : MTK 
	//Image size	  : 2592x1944
	//MCLK		  : 24MHz
	//MIPI speed(Mbps): 880Mbps x 2Lane
	//Frame Length	  : 4166
	//Line Length 	  : 2816
	//Max Fps 	  : 15.0fps
	////////////////////////////////////////////////

	write_cmos_sensor(0x0a00, 0x0000);
	write_cmos_sensor(0x0b0a, 0x8252);
	write_cmos_sensor(0x0f30, 0x6e25); //pll                                  
	write_cmos_sensor(0x0f32, 0x7067); //pll                                  
	write_cmos_sensor(0x004a, 0x0100);                                        
	write_cmos_sensor(0x004c, 0x0000);                                        
	write_cmos_sensor(0x004e, 0x0000); //per-frame control off, on 0x0100     
	write_cmos_sensor(0x000c, 0x0022);                                        
	write_cmos_sensor(0x0008, 0x0b00); //line length pck 2816                 
	write_cmos_sensor(0x005a, 0x0202);                                        
	write_cmos_sensor(0x0012, 0x000e);                                        
	write_cmos_sensor(0x0018, 0x0a31);                                        
	write_cmos_sensor(0x0022, 0x0008);                                        
	write_cmos_sensor(0x0028, 0x0017);                                        
	write_cmos_sensor(0x0024, 0x0028);                                        
	write_cmos_sensor(0x002a, 0x002d);                                        
	write_cmos_sensor(0x0026, 0x0030);                                        
	write_cmos_sensor(0x002c, 0x07c7);                                        
	write_cmos_sensor(0x002e, 0x1111);                                        
	write_cmos_sensor(0x0030, 0x1111);                                        
	write_cmos_sensor(0x0032, 0x1111);                                        
	write_cmos_sensor(0x0006, 0x1046); //frame length lines 4166              
	write_cmos_sensor(0x0a22, 0x0000);                                        
	write_cmos_sensor(0x0a12, 0x0a20); //x output size 2592                   
	write_cmos_sensor(0x0a14, 0x0798); //y output size 1944                   
	write_cmos_sensor(0x003e, 0x0000);                                        
	write_cmos_sensor(0x0804, 0x0200);                                        
	write_cmos_sensor(0x0a04, 0x014a); //isp_en                               
	write_cmos_sensor(0x090c, 0x0fdc); //mipi_vblank_delay                    
	write_cmos_sensor(0x090e, 0x002d); //mipi_hblank_delay                    
	write_cmos_sensor(0x0902, 0x4319); //mipi_tx_op_mode1, mipi_tx_op_mode2   
	write_cmos_sensor(0x0914, 0xc10a); //mipi_exit_seq, tlpx                  
	write_cmos_sensor(0x0916, 0x071f); //tclk_prepare, tclk_zero              
	write_cmos_sensor(0x0918, 0x0408); //tclk_pre, ths_prepare                
	write_cmos_sensor(0x091a, 0x0c0d); //ths_zero, ths_trail                  
	write_cmos_sensor(0x091c, 0x0f09); //tclk_post, tclk_trail                
	write_cmos_sensor(0x091e, 0x0a00); //mipi_exit, null
	write_cmos_sensor(0x0a00, 0x0100); //stream off  									

		}

	else {

	//Sensor Information////////////////////////////
	//Sensor	  : Hi-556
	//Date		  : 2017-02-20
	//AP chipset      : MTK 
	//Image size	  : 2592x1944
	//MCLK		  : 24MHz
	//MIPI speed(Mbps): 880Mbps x 2Lane
	//Frame Length	  : 2049
	//Line Length 	  : 2816
	//Max Fps 	  : 30.5fps
	////////////////////////////////////////////////

	write_cmos_sensor(0x0a00, 0x0000);
	write_cmos_sensor(0x0b0a, 0x8252);
	write_cmos_sensor(0x0f30, 0x6e25); //pll                                  
	write_cmos_sensor(0x0f32, 0x7067); //pll                                  
	write_cmos_sensor(0x004a, 0x0100);                                        
	write_cmos_sensor(0x004c, 0x0000);                                        
	write_cmos_sensor(0x004e, 0x0000); //per-frame control off, on 0x0100      
	write_cmos_sensor(0x000c, 0x0022);                                        
	write_cmos_sensor(0x0008, 0x0b00); //line length pck 2816                 
	write_cmos_sensor(0x005a, 0x0202);                                        
	write_cmos_sensor(0x0012, 0x000e);                                        
	write_cmos_sensor(0x0018, 0x0a31);                                        
	write_cmos_sensor(0x0022, 0x0008);                                        
	write_cmos_sensor(0x0028, 0x0017);                                        
	write_cmos_sensor(0x0024, 0x0028);                                        
	write_cmos_sensor(0x002a, 0x002d);                                        
	write_cmos_sensor(0x0026, 0x0030);                                        
	write_cmos_sensor(0x002c, 0x07c7);                                        
	write_cmos_sensor(0x002e, 0x1111);                                        
	write_cmos_sensor(0x0030, 0x1111);                                        
	write_cmos_sensor(0x0032, 0x1111);                                        
	write_cmos_sensor(0x0006, 0x0801); //frame length lines 2049              
	write_cmos_sensor(0x0a22, 0x0000);                                        
	write_cmos_sensor(0x0a12, 0x0a20); //x output size 2592                   
	write_cmos_sensor(0x0a14, 0x0798); //y output size 1944                   
	write_cmos_sensor(0x003e, 0x0000);                                        
	write_cmos_sensor(0x0804, 0x0200);                                        
	write_cmos_sensor(0x0a04, 0x014a); //isp_en                               
	write_cmos_sensor(0x090c, 0x0fdc); //mipi_vblank_delay                    
	write_cmos_sensor(0x090e, 0x002d); //mipi_hblank_delay                    
	write_cmos_sensor(0x0902, 0x4319); //mipi_tx_op_mode1, mipi_tx_op_mode2   
	write_cmos_sensor(0x0914, 0xc10a); //mipi_exit_seq, tlpx                  
	write_cmos_sensor(0x0916, 0x071f); //tclk_prepare, tclk_zero              
	write_cmos_sensor(0x0918, 0x0408); //tclk_pre, ths_prepare                
	write_cmos_sensor(0x091a, 0x0c0d); //ths_zero, ths_trail                  
	write_cmos_sensor(0x091c, 0x0f09); //tclk_post, tclk_trail                
	write_cmos_sensor(0x091e, 0x0a00); //mipi_exit, null
	write_cmos_sensor(0x0a00, 0x0100); //stream off

	}
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
  capture_setting(currefps);
}

static void hs_video_setting(void)
{
	LOG_INF("E\n");

	//Sensor Information////////////////////////////
	//Sensor	  : hi-556
	//Date		  : 2016-10-19
	//Customer		  : MTK_validation
	//Image size	  : 640x480
	//MCLK		  : 24MHz
	//MIPI speed(Mbps): 220Mbps x 2Lane
	//Frame Length	  : 520
	//Line Length	  : 2816
	//Max Fps	  : 120.19fps
	//Pixel order	  : Green 1st (=GB)
	//X/Y-flip	  : X-flip
	//BLC offset	  : 64code
	////////////////////////////////////////////////

	write_cmos_sensor(0x0a00, 0x0000);
	write_cmos_sensor(0x0b0a, 0x8252);
	write_cmos_sensor(0x0f30, 0x6e25); //pll                                  
	write_cmos_sensor(0x0f32, 0x7267); //pll                                  
	write_cmos_sensor(0x004a, 0x0100);                                        
	write_cmos_sensor(0x004c, 0x0000);                                        
	write_cmos_sensor(0x004e, 0x0000); //per-frame control off, on 0x0100     
	write_cmos_sensor(0x000c, 0x0022);                                        
	write_cmos_sensor(0x0008, 0x0b00); //line length pck 2816                 
	write_cmos_sensor(0x005a, 0x0208);                                        
	write_cmos_sensor(0x0012, 0x0018);                                        
	write_cmos_sensor(0x0018, 0x0a27);                                        
	write_cmos_sensor(0x0022, 0x0008);                                        
	write_cmos_sensor(0x0028, 0x0017);                                        
	write_cmos_sensor(0x0024, 0x002e);                                        
	write_cmos_sensor(0x002a, 0x0033);                                        
	write_cmos_sensor(0x0026, 0x003c);                                        
	write_cmos_sensor(0x002c, 0x07bb);                                        
	write_cmos_sensor(0x002e, 0x1111);                                        
	write_cmos_sensor(0x0030, 0x1111);                                        
	write_cmos_sensor(0x0032, 0x7711);                                        
	write_cmos_sensor(0x0006, 0x0208); //frame length lines 520              
	write_cmos_sensor(0x0a22, 0x0100);                                        
	write_cmos_sensor(0x0a12, 0x0280); //x output size 2592                   
	write_cmos_sensor(0x0a14, 0x01e0); //y output size 1944                   
	write_cmos_sensor(0x003e, 0x0000);                                        
	write_cmos_sensor(0x0804, 0x0200);                                        
	write_cmos_sensor(0x0a04, 0x016a); //isp_en                               
	write_cmos_sensor(0x090c, 0x0270); //mipi_vblank_delay                    
	write_cmos_sensor(0x090e, 0x000c); //mipi_hblank_delay                    
	write_cmos_sensor(0x0902, 0x4319); //mipi_tx_op_mode1, mipi_tx_op_mode2   
	write_cmos_sensor(0x0914, 0xc103); //mipi_exit_seq, tlpx                  
	write_cmos_sensor(0x0916, 0x0207); //tclk_prepare, tclk_zero              
	write_cmos_sensor(0x0918, 0x0302); //tclk_pre, ths_prepare                
	write_cmos_sensor(0x091a, 0x0406); //ths_zero, ths_trail                  
	write_cmos_sensor(0x091c, 0x0903); //tclk_post, tclk_trail                
	write_cmos_sensor(0x091e, 0x0300); //mipi_exit, null
	write_cmos_sensor(0x0a00, 0x0100); //stream off   


}


static void slim_video_setting(void)
{
	LOG_INF("E\n");
	preview_setting();
}


static kal_uint32 return_sensor_id(void)
{
    return read_cmos_sensor(0x0f16);
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
#ifdef CONFIG_LCT_DEVINFO_SUPPORT/* add for dev_info*/
#include  "dev_info.h"
static struct devinfo_struct s_DEVINFO_ccm;
#endif
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
	s_DEVINFO_ccm.device_type = "CCM-S";
	s_DEVINFO_ccm.device_module = "NULL";//can change if got module id
	s_DEVINFO_ccm.device_vendor = "qunhui";
	s_DEVINFO_ccm.device_ic = "hi556";
	s_DEVINFO_ccm.device_version = "hynix";
	s_DEVINFO_ccm.device_info = "500W-Gr";
	s_DEVINFO_ccm.device_used=DEVINFO_USED;
	// - 2016-11-17-10-53
#endif
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if (*sensor_id == imgsensor_info.sensor_id) {
			sensor_init();
			HI556_otp_cali();
#ifdef CONFIG_MTK_CAM_CAL
		//read_imx135_otp_mtk_fmt();
#endif
				LOG_INF("i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id,*sensor_id,imgsensor_info.sensor_id);	
                return ERROR_NONE;
            }
			LOG_INF("Read sensor id fail, i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id,*sensor_id,imgsensor_info.sensor_id);	
            retry--;
        } while(retry > 0);
        i++;
        retry = 1;
    }
    if (*sensor_id != imgsensor_info.sensor_id) {
        // if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF
	#ifdef CONFIG_LCT_DEVINFO_SUPPORT	
		s_DEVINFO_ccm.device_used=DEVINFO_UNUSED;	
		DEVINFO_CHECK_DECLARE(s_DEVINFO_ccm.device_type,s_DEVINFO_ccm.device_module,
					s_DEVINFO_ccm.device_vendor,s_DEVINFO_ccm.device_ic,s_DEVINFO_ccm.device_version,
					s_DEVINFO_ccm.device_info,s_DEVINFO_ccm.device_used);
	#endif
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
    kal_uint32 sensor_id = 0;
    LOG_1;
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
            LOG_INF("Read sensor id fail, id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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
	 HI556_otp_apply();

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
    spin_unlock(&imgsensor_drv_lock);

    return ERROR_NONE;
}   /*  open  */



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
	//set_mirror_flip(IMAGE_NORMAL);	
	#if 0
	mdelay(10);
	#ifdef FANPENGTAO
	int i=0;
	for(i=0; i<10; i++){
		LOG_INF("delay time = %d, the frame no = %d\n", i*10, read_cmos_sensor(0x0005));
		mdelay(10);
	}
	#endif
	#endif
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
	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) {
		LOG_INF("capture30fps: use cap30FPS's setting: %d fps!\n",imgsensor.current_fps/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} 
	else  
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
		//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		LOG_INF("cap115fps: use cap1's setting: %d fps!\n",imgsensor.current_fps/10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else  { //PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		LOG_INF("Warning:=== current_fps %d fps is not support, so use cap1's setting\n",imgsensor.current_fps/10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps); 
	//set_mirror_flip(IMAGE_NORMAL);	
	mdelay(5);

#if 0
	if(imgsensor.test_pattern == KAL_TRUE)
	{
		 write_cmos_sensor(0x0a04, 0x0141);
		 write_cmos_sensor(0x0200, 0x0001);
		 write_cmos_sensor(0x0206, 0x000a);
		 write_cmos_sensor(0x0208, 0x0a0a);
		 write_cmos_sensor(0x020a, 0x000a);
		 write_cmos_sensor(0x020c, 0x0a0a);
  	}
#endif

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
	//set_mirror_flip(IMAGE_NORMAL);	
	
	
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
	//set_mirror_flip(IMAGE_NORMAL);
	
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
	//set_mirror_flip(IMAGE_NORMAL);
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

	
	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;
	
	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;
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
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame; 
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame; 
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
    sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
    sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame; 
    sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame; 
    sensor_info->Custom3DelayFrame = imgsensor_info.custom3_delay_frame; 
    sensor_info->Custom4DelayFrame = imgsensor_info.custom4_delay_frame; 
    sensor_info->Custom5DelayFrame = imgsensor_info.custom5_delay_frame; 

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
	#ifdef FPTPDAFSUPPORT
	sensor_info->PDAF_Support = 1;
	#else 
	sensor_info->PDAF_Support = 0;
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
			break;	
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;	
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			break;
        case MSDK_SCENARIO_ID_CUSTOM1:
            frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case MSDK_SCENARIO_ID_CUSTOM2:
            frame_length = imgsensor_info.custom2.pclk / framerate * 10 / imgsensor_info.custom2.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom2.framelength) ? (frame_length - imgsensor_info.custom2.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom2.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            break; 
        case MSDK_SCENARIO_ID_CUSTOM3:
            frame_length = imgsensor_info.custom3.pclk / framerate * 10 / imgsensor_info.custom3.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom3.framelength) ? (frame_length - imgsensor_info.custom3.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom3.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            break; 
        case MSDK_SCENARIO_ID_CUSTOM4:
            frame_length = imgsensor_info.custom4.pclk / framerate * 10 / imgsensor_info.custom4.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom4.framelength) ? (frame_length - imgsensor_info.custom4.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom4.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            break; 
        case MSDK_SCENARIO_ID_CUSTOM5:
            frame_length = imgsensor_info.custom5.pclk / framerate * 10 / imgsensor_info.custom5.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom5.framelength) ? (frame_length - imgsensor_info.custom5.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			break;	
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
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
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		 write_cmos_sensor(0x0a04, 0x0141);
		 write_cmos_sensor(0x0200, 0x0001);
		 write_cmos_sensor(0x0206, 0x000a);
		 write_cmos_sensor(0x0208, 0x0a0a);
		 write_cmos_sensor(0x020a, 0x000a);
		 write_cmos_sensor(0x020c, 0x0a0a);
		 LOG_INF(">>crc enable>> reg: %d\n", enable);
	} else {
		 write_cmos_sensor(0x0a04, 0x0140);
		 write_cmos_sensor(0x0200, 0x0000);
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
	//SET_PD_BLOCK_INFO_T *PDAFinfo;

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
            imgsensor.ihdr_en = (BOOL)*feature_data;
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
        case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
            LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            break;
        default:
            break;
    }

    return ERROR_NONE;
}    /*    feature_control()  */


static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};


UINT32 HI556MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	hi556_MIPI_RAW_SensorInit	*/



