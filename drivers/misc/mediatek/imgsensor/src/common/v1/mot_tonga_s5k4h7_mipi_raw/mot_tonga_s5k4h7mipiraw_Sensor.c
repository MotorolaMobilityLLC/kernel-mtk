/* *****************************************************************************
 *
 * Filename:
 * ---------
 *	 s5k4h7mipi_Sensor.c
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
/* #include <asm/system.h> */
/* #include <linux/xlog.h> */
#include "kd_camera_typedef.h"
//#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "mot_tonga_s5k4h7mipiraw_Sensor.h"

#define OTP_4H7 1

#if OTP_4H7
#define FLAG_VALUE_PAGE 0x40
#define INCLUDE_NO_OTP_4H7 1
#define S5K4H7_MODULE_ID_START_ADD 0x0a0e
#define S5K4H7_MODULE_ID_LENGTH 0x02
#define getbit(x,y)   ((x) >> (y)&1)

//Use mot part number to distinguish module
#define S5K4H7_MODULE_ID_BINGO 0x3136
#define S5K4H7_MODULE_ID_LIMA 0x3138
#endif
/****************************Modify following Strings for debug****************************/
#define PFX "s5k4h7_camera_sensor"

#define LOG_1 LOG_INF("s5k4h7,MIPI 4LANE\n")
#define LOG_INF(format, args...)    pr_err(PFX "[%s] " format, __func__, ##args)
#define LOGE(format, args...)    pr_err(PFX "[%s] " format, __func__, ##args)

#define S5K4H7YX_EEPROM_SIZE 715
#define S5K4H7YX_SERIAL_NUM_SIZE 8
#define EEPROM_DATA_PATH "/data/vendor/camera_dump/s5k4h7_otp_data.bin"
#define WIDE_SERIAL_NUM_DATA_PATH "/data/vendor/camera_dump/serial_number_wide.bin"

static DEFINE_SPINLOCK(imgsensor_drv_lock);
static bool bIsLongExposure = KAL_FALSE;
static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_TONGA_S5K4H7_SENSOR_ID,

	.checksum_value = 0xf16e8197,

	.pre = {
	.pclk = 280000000,              /* record different mode's pclk */
	.linelength = 3688,				/* record different mode's linelength */
	.framelength = 2530,         /* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width = 3264,		/* record different mode's width of grabwindow */
		.grabwindow_height = 2448,		/* record different mode's height of grabwindow */
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 292800000,
	},
	.cap = {
		.pclk = 280000000,
		.linelength = 3688,
		.framelength = 2530,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 292800000,
	},
	.normal_video = {	/* Full 30fps */
		.pclk = 280000000,
		.linelength = 3688,
		.framelength = 2530,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 292800000,
	},
	.hs_video = {	/* Full 30fps */
		.pclk = 280000000,
		.linelength = 3688,
		.framelength = 2530,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 292800000,
	},
	.slim_video = {
		.pclk = 280000000,				/* record different mode's pclk */
		.linelength = 3688,			/* record different mode's linelength */
		.framelength = 2530,		   /* record different mode's framelength */
		.startx = 0,					/* record different mode's startx of grabwindow */
		.starty = 0,					/* record different mode's starty of grabwindow */
		.grabwindow_width = 1280,		/* record different mode's width of grabwindow */
		.grabwindow_height = 720,		/* record different mode's height of grabwindow */
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 292800000,
	},
	.margin = 4,
	.min_shutter = 4,
	.max_frame_length = 0xffff-5,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  /* 1, support; 0,not support */
	.ihdr_le_firstline = 0,  /* 1,le first ; 0, se first */
	.sensor_mode_num = 5,	  /* support sensor mode num */

	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_6MA,     /* mclk driving current */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,    /* sensor_interface_type */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,        /* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_settle_delay_mode = 0,                /* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x5A,0x20,0xff},
	.i2c_speed = 400,
};


static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				/* mirrorflip information */
	.sensor_mode = IMGSENSOR_MODE_INIT, /* IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video */
	.shutter = 0x3D0,					/* current shutter */
	.gain = 0x100,						/* current gain */
	.dummy_pixel = 0,					/* current dummypixel */
	.dummy_line = 0,					/* current dummyline */
	.current_fps = 300,  /* full size current fps : 24fps for PIP, 30fps for Normal or ZSD */
	.autoflicker_en = KAL_FALSE,  /* auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker */
	.test_pattern = KAL_FALSE,		/* test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output */
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,/* current scenario id */
	.ihdr_en = 0, /* sensor need support LE, SE with HDR feature */
	.i2c_write_id = 0x5A,
	.current_ae_effective_frame = 0x3372CB48,//0x13DE38C8, //number of frames in effect for long exposure?if N+1 take effect?the value is 1?
};


/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {

 { 3264, 2448,	  0,	0, 3264, 2448, 3264,  2448, 0000, 0000, 3264, 2448,	    0,	0, 3264, 2448}, /* Preview */
 { 3264, 2448,	  0,	0, 3264, 2448, 3264,  2448, 0000, 0000, 3264, 2448,	    0,	0, 3264, 2448}, /* capture */
 { 3264, 2448,	  0,	0, 3264, 2448, 3264,  2448, 0000, 0000, 3264, 2448,	    0,	0, 3264, 2448}, /* video */
 { 3264, 2448,	  0,	0, 3264, 2448, 3264,  2448, 0000, 0000, 3264, 2448,	    0,	0, 3264, 2448}, /* video */
 { 3264, 2448,  352,504, 2560, 1440, 1280,   720, 0000, 0000, 1280,  720,     0,  0, 1280,  720}, /* slim video */
};/* slim video */


static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}



static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
    	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};

    	iReadRegI2C(pusendcmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
    char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

    iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

#if OTP_4H7
static void write_cmos_sensor_16(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

    //kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pusendcmd, 4, imgsensor.i2c_write_id);
}
#endif

unsigned char S5K4H7_read_cmos_sensor(u32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

void S5K4H7_write_cmos_sensor(u16 addr, u32 para)
{
    char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

    iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n", imgsensor.dummy_line, imgsensor.dummy_pixel);

	write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
	write_cmos_sensor_8(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor_8(0x0343, imgsensor.line_length & 0xFF);
}	/*	set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	LOG_INF("get here11\n");
	LOG_INF("get here22  0x%x\n", (read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
	return (((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001)));
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{

	kal_uint32 frame_length = imgsensor.frame_length;

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
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
	set_dummy();
}	/*	set_max_framerate  */

static kal_uint32 streaming_control(kal_bool enable)
{
    int timeout = 200;//(10000 / imgsensor.current_fps) + 1;
    int i = 0;
    int framecnt = 0;

    LOG_INF("streaming_enable(0= Sw Standby,1= streaming): %d\n", enable);
    if (enable) {
        write_cmos_sensor_8(0x0100, 0x01);
        mdelay(10);
    } else {
        write_cmos_sensor_8(0x0100, 0x00);
        for (i = 0; i < timeout; i++) {
            mdelay(10);
            framecnt = read_cmos_sensor_8(0x0005);
            if ( framecnt == 0xFF) {
                LOG_INF(" Stream Off OK at i=%d.\n", i);
                return ERROR_NONE;
            }
        }
        LOG_INF("Stream Off Fail! framecnt= %d.\n", framecnt);
    }
    return ERROR_NONE;
}

static void write_shutter(kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;
	kal_uint32 temp_0200 = 0, temp_0340 = 0;
	kal_uint32 temp_0202 = 0, temp_0342 = 0;

	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;

	if (shutter > 65530) {
		temp_0342 = 0xFFFC;
		temp_0200 = 0xFF6C;
		temp_0202 = ((shutter*3688-temp_0200))/temp_0342 +1;
		temp_0340 = temp_0202 + 5;
		LOG_INF("temp_0202 =%d, temp_0340 =%d \n", temp_0202, temp_0340);
		streaming_control(KAL_FALSE);
		write_cmos_sensor_8(0x0340, (temp_0340&0xFF00)>>8);
		write_cmos_sensor_8(0x0341, (temp_0340&0x00FF));
		write_cmos_sensor_8(0x0342, 0xFF);
		write_cmos_sensor_8(0x0343, 0xFC);
		write_cmos_sensor_8(0x0200, 0xFF);
		write_cmos_sensor_8(0x0201, 0x6C);
		write_cmos_sensor_8(0x0202, (temp_0202&0xFF00)>>8);
		write_cmos_sensor_8(0x0203, (temp_0202&0x00FF));
		streaming_control(KAL_TRUE);
		bIsLongExposure = KAL_TRUE;
	} else {

		shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
				(imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
		if (imgsensor.autoflicker_en) {
			realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
			if (realtime_fps >= 297 && realtime_fps <= 305)
				set_max_framerate(296, 0);
			else if (realtime_fps >= 147 && realtime_fps <= 150)
				set_max_framerate(146, 0);
			else {
				write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
				write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
			}
		} else {
			write_cmos_sensor_8(0x0340, imgsensor.frame_length >> 8);
			write_cmos_sensor_8(0x0341, imgsensor.frame_length & 0xFF);
		}

		if (bIsLongExposure == KAL_TRUE) {
			streaming_control(KAL_FALSE);
			write_cmos_sensor_8(0x0342, 0x0E);
			write_cmos_sensor_8(0x0343, 0x68);
			write_cmos_sensor_8(0x0200, 0x0D);
			write_cmos_sensor_8(0x0201, 0xD8);
			streaming_control(KAL_TRUE);
			bIsLongExposure = KAL_FALSE;
		}
		write_cmos_sensor_8(0x0202, shutter >> 8);
		write_cmos_sensor_8(0x0203, shutter & 0xFF);
	}

	LOG_INF("shutter =%d, framelength =%d bIsLongExposure = %d\n", shutter, imgsensor.frame_length,bIsLongExposure);
}

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
	/* [0:3] = N meams N /16 X	*/
	/* [4:9] = M meams M X		 */
	/* Total gain = M + N /16 X   */

	/*  */
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

    write_cmos_sensor_8(0x0204, (reg_gain>>8));
    write_cmos_sensor_8(0x0205, (reg_gain&0xff));

    return gain;
}	/*	set_gain  */

/* defined but not used */
static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	LOG_INF("le:0x%x, se:0x%x, gain:0x%x\n", le, se, gain);
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
				/* Extend frame length first */
				write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);

		set_gain(gain);
	}
}

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

	switch (image_mirror) {
		case IMAGE_NORMAL:
			write_cmos_sensor_8(0x0101, 0x00);
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor_8(0x0101, 0x01);
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor_8(0x0101, 0x02);
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor_8(0x0101, 0x03);
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

/* #define LSC_cal	1 */
static void sensor_init(void)
{
	LOG_INF("sensor_init() E\n");
	/* Base on S5K4H7_EVT0_Reference_setfile_v0.91 */
	write_cmos_sensor_8(0X0100, 0X00);
	write_cmos_sensor_8(0x0000, 0X12);
	write_cmos_sensor_8(0x0000, 0X48);
	write_cmos_sensor_8(0x0A02, 0X15);
	write_cmos_sensor_8(0x0100, 0X00);
	write_cmos_sensor_8(0X0B05, 0X01);
	write_cmos_sensor_8(0X3074, 0X06);
	write_cmos_sensor_8(0X3075, 0X2F);
	write_cmos_sensor_8(0X308A, 0X20);
	write_cmos_sensor_8(0X308B, 0X08);
	write_cmos_sensor_8(0X308C, 0X0B);
	write_cmos_sensor_8(0X3081, 0X07);
	write_cmos_sensor_8(0X307B, 0X85);
	write_cmos_sensor_8(0X307A, 0X0A);
	write_cmos_sensor_8(0X3079, 0X0A);
	write_cmos_sensor_8(0X306E, 0X71);
	write_cmos_sensor_8(0X306F, 0X28);
	write_cmos_sensor_8(0X301F, 0X20);
	write_cmos_sensor_8(0X306B, 0X9A);
	write_cmos_sensor_8(0X3091, 0X1F);
	write_cmos_sensor_8(0X30C4, 0X06);
	write_cmos_sensor_8(0X3200, 0X09);
	write_cmos_sensor_8(0X306A, 0X79);
	write_cmos_sensor_8(0X30B0, 0XFF);
	write_cmos_sensor_8(0X306D, 0X08);
	write_cmos_sensor_8(0X3080, 0X00);
	write_cmos_sensor_8(0X3929, 0X3F);
	write_cmos_sensor_8(0X3084, 0X16);
	write_cmos_sensor_8(0X3070, 0X0F);
	write_cmos_sensor_8(0X3B45, 0X01);
	write_cmos_sensor_8(0X30C2, 0X05);
	write_cmos_sensor_8(0X3069, 0X87);
	write_cmos_sensor_8(0X3924, 0X7F);
	write_cmos_sensor_8(0X3925, 0XFD);
	write_cmos_sensor_8(0X3C08, 0XFF);
	write_cmos_sensor_8(0X3C09, 0XFF);
	write_cmos_sensor_8(0X3C31, 0XFF);
	write_cmos_sensor_8(0X3C32, 0XFF);
	write_cmos_sensor_8(0X0100, 0X00);
}	/*	sensor_init  */

static void preview_setting(void)
{
	write_cmos_sensor_8(0X0136, 0X18);
	write_cmos_sensor_8(0X0137, 0X00);
	write_cmos_sensor_8(0X0305, 0X06);
	write_cmos_sensor_8(0X0306, 0X00);
	write_cmos_sensor_8(0X0307, 0X8C);
	write_cmos_sensor_8(0X030D, 0X06);
	write_cmos_sensor_8(0X030E, 0X00);
	write_cmos_sensor_8(0X030F, 0XAF);
	write_cmos_sensor_8(0X3C1F, 0X00);
	write_cmos_sensor_8(0X3C17, 0X00);
	write_cmos_sensor_8(0X3C1C, 0X05);
	write_cmos_sensor_8(0X3C1D, 0X15);
	write_cmos_sensor_8(0X0301, 0X04);
	write_cmos_sensor_8(0X0820, 0X02);
	write_cmos_sensor_8(0X0821, 0XBC);
	write_cmos_sensor_8(0X0822, 0X00);
	write_cmos_sensor_8(0X0823, 0X00);
	write_cmos_sensor_8(0X0112, 0X0A);
	write_cmos_sensor_8(0X0113, 0X0A);
	write_cmos_sensor_8(0X0114, 0X03);
	write_cmos_sensor_8(0X3906, 0X04);
	write_cmos_sensor_8(0X0344, 0X00);
	write_cmos_sensor_8(0X0345, 0X08);
	write_cmos_sensor_8(0X0346, 0X00);
	write_cmos_sensor_8(0X0347, 0X08);
	write_cmos_sensor_8(0X0348, 0X0C);
	write_cmos_sensor_8(0X0349, 0XC7);
	write_cmos_sensor_8(0X034A, 0X09);
	write_cmos_sensor_8(0X034B, 0X97);
	write_cmos_sensor_8(0X034C, 0X0C);
	write_cmos_sensor_8(0X034D, 0XC0);
	write_cmos_sensor_8(0X034E, 0X09);
	write_cmos_sensor_8(0X034F, 0X90);
	write_cmos_sensor_8(0X0900, 0X00);
	write_cmos_sensor_8(0X0901, 0X00);
	write_cmos_sensor_8(0X0381, 0X01);
	write_cmos_sensor_8(0X0383, 0X01);
	write_cmos_sensor_8(0X0385, 0X01);
	write_cmos_sensor_8(0X0387, 0X01);
	write_cmos_sensor_8(0X0101, 0X00);
	write_cmos_sensor_8(0X0340, 0X09);
	write_cmos_sensor_8(0X0341, 0XE2);
	write_cmos_sensor_8(0X0342, 0X0E);
	write_cmos_sensor_8(0X0343, 0X68);
	write_cmos_sensor_8(0X0200, 0X0D);
	write_cmos_sensor_8(0X0201, 0XD8);
	write_cmos_sensor_8(0X0202, 0X02);
	write_cmos_sensor_8(0X0203, 0X08);
	write_cmos_sensor_8(0x3400, 0x01);
	write_cmos_sensor_8(0x0100, 0x01);

}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	write_cmos_sensor_8(0X0136, 0X18);
	write_cmos_sensor_8(0X0137, 0X00);
	write_cmos_sensor_8(0X0305, 0X06);
	write_cmos_sensor_8(0X0306, 0X00);
	write_cmos_sensor_8(0X0307, 0X8C);
	write_cmos_sensor_8(0X030D, 0X06);
	write_cmos_sensor_8(0X030E, 0X00);
	write_cmos_sensor_8(0X030F, 0XAF);
	write_cmos_sensor_8(0X3C1F, 0X00);
	write_cmos_sensor_8(0X3C17, 0X00);
	write_cmos_sensor_8(0X3C1C, 0X05);
	write_cmos_sensor_8(0X3C1D, 0X15);
	write_cmos_sensor_8(0X0301, 0X04);
	write_cmos_sensor_8(0X0820, 0X02);
	write_cmos_sensor_8(0X0821, 0XBC);
	write_cmos_sensor_8(0X0822, 0X00);
	write_cmos_sensor_8(0X0823, 0X00);
	write_cmos_sensor_8(0X0112, 0X0A);
	write_cmos_sensor_8(0X0113, 0X0A);
	write_cmos_sensor_8(0X0114, 0X03);
	write_cmos_sensor_8(0X3906, 0X04);
	write_cmos_sensor_8(0X0344, 0X00);
	write_cmos_sensor_8(0X0345, 0X08);
	write_cmos_sensor_8(0X0346, 0X00);
	write_cmos_sensor_8(0X0347, 0X08);
	write_cmos_sensor_8(0X0348, 0X0C);
	write_cmos_sensor_8(0X0349, 0XC7);
	write_cmos_sensor_8(0X034A, 0X09);
	write_cmos_sensor_8(0X034B, 0X97);
	write_cmos_sensor_8(0X034C, 0X0C);
	write_cmos_sensor_8(0X034D, 0XC0);
	write_cmos_sensor_8(0X034E, 0X09);
	write_cmos_sensor_8(0X034F, 0X90);
	write_cmos_sensor_8(0X0900, 0X00);
	write_cmos_sensor_8(0X0901, 0X00);
	write_cmos_sensor_8(0X0381, 0X01);
	write_cmos_sensor_8(0X0383, 0X01);
	write_cmos_sensor_8(0X0385, 0X01);
	write_cmos_sensor_8(0X0387, 0X01);
	write_cmos_sensor_8(0X0101, 0X00);
	write_cmos_sensor_8(0X0340, 0X09);
	write_cmos_sensor_8(0X0341, 0XE2);
	write_cmos_sensor_8(0X0342, 0X0E);
	write_cmos_sensor_8(0X0343, 0X68);
	write_cmos_sensor_8(0X0200, 0X0D);
	write_cmos_sensor_8(0X0201, 0XD8);
	write_cmos_sensor_8(0X0202, 0X02);
	write_cmos_sensor_8(0X0203, 0X08);
	write_cmos_sensor_8(0x3400, 0x01);
	write_cmos_sensor_8(0x0100, 0x01);
}

static void normal_video_setting(kal_uint16 currefps)
{
	write_cmos_sensor_8(0X0100, 0X00);
	write_cmos_sensor_8(0X0136, 0X18);
	write_cmos_sensor_8(0X0137, 0X00);
	write_cmos_sensor_8(0X0305, 0X06);
	write_cmos_sensor_8(0X0306, 0X00);
	write_cmos_sensor_8(0X0307, 0X8C);
	write_cmos_sensor_8(0X030D, 0X06);
	write_cmos_sensor_8(0X030E, 0X00);
	write_cmos_sensor_8(0X030F, 0XAF);
	write_cmos_sensor_8(0X3C1F, 0X00);
	write_cmos_sensor_8(0X3C17, 0X00);
	write_cmos_sensor_8(0X3C1C, 0X05);
	write_cmos_sensor_8(0X3C1D, 0X15);
	write_cmos_sensor_8(0X0301, 0X04);
	write_cmos_sensor_8(0X0820, 0X02);
	write_cmos_sensor_8(0X0821, 0XBC);
	write_cmos_sensor_8(0X0822, 0X00);
	write_cmos_sensor_8(0X0823, 0X00);
	write_cmos_sensor_8(0X0112, 0X0A);
	write_cmos_sensor_8(0X0113, 0X0A);
	write_cmos_sensor_8(0X0114, 0X03);
	write_cmos_sensor_8(0X3906, 0X04);
	write_cmos_sensor_8(0X0344, 0X00);
	write_cmos_sensor_8(0X0345, 0X08);
	write_cmos_sensor_8(0X0346, 0X00);
	write_cmos_sensor_8(0X0347, 0X08);
	write_cmos_sensor_8(0X0348, 0X0C);
	write_cmos_sensor_8(0X0349, 0XC7);
	write_cmos_sensor_8(0X034A, 0X09);
	write_cmos_sensor_8(0X034B, 0X97);
	write_cmos_sensor_8(0X034C, 0X0C);
	write_cmos_sensor_8(0X034D, 0XC0);
	write_cmos_sensor_8(0X034E, 0X09);
	write_cmos_sensor_8(0X034F, 0X90);
	write_cmos_sensor_8(0X0900, 0X00);
	write_cmos_sensor_8(0X0901, 0X00);
	write_cmos_sensor_8(0X0381, 0X01);
	write_cmos_sensor_8(0X0383, 0X01);
	write_cmos_sensor_8(0X0385, 0X01);
	write_cmos_sensor_8(0X0387, 0X01);
	write_cmos_sensor_8(0X0101, 0X00);
	write_cmos_sensor_8(0X0340, 0X09);
	write_cmos_sensor_8(0X0341, 0XE2);
	write_cmos_sensor_8(0X0342, 0X0E);
	write_cmos_sensor_8(0X0343, 0X68);
	write_cmos_sensor_8(0X0200, 0X0D);
	write_cmos_sensor_8(0X0201, 0XD8);
	write_cmos_sensor_8(0X0202, 0X02);
	write_cmos_sensor_8(0X0203, 0X08);
	write_cmos_sensor_8(0x3400, 0x01);
	write_cmos_sensor_8(0x0100, 0x01);
}

static void hs_video_setting()
{
	write_cmos_sensor_8(0X0100, 0X00);
	write_cmos_sensor_8(0X0136, 0X18);
	write_cmos_sensor_8(0X0137, 0X00);
	write_cmos_sensor_8(0X0305, 0X06);
	write_cmos_sensor_8(0X0306, 0X00);
	write_cmos_sensor_8(0X0307, 0X8C);
	write_cmos_sensor_8(0X030D, 0X06);
	write_cmos_sensor_8(0X030E, 0X00);
	write_cmos_sensor_8(0X030F, 0XAF);
	write_cmos_sensor_8(0X3C1F, 0X00);
	write_cmos_sensor_8(0X3C17, 0X00);
	write_cmos_sensor_8(0X3C1C, 0X05);
	write_cmos_sensor_8(0X3C1D, 0X15);
	write_cmos_sensor_8(0X0301, 0X04);
	write_cmos_sensor_8(0X0820, 0X02);
	write_cmos_sensor_8(0X0821, 0XBC);
	write_cmos_sensor_8(0X0822, 0X00);
	write_cmos_sensor_8(0X0823, 0X00);
	write_cmos_sensor_8(0X0112, 0X0A);
	write_cmos_sensor_8(0X0113, 0X0A);
	write_cmos_sensor_8(0X0114, 0X03);
	write_cmos_sensor_8(0X3906, 0X04);
	write_cmos_sensor_8(0X0344, 0X00);
	write_cmos_sensor_8(0X0345, 0X08);
	write_cmos_sensor_8(0X0346, 0X00);
	write_cmos_sensor_8(0X0347, 0X08);
	write_cmos_sensor_8(0X0348, 0X0C);
	write_cmos_sensor_8(0X0349, 0XC7);
	write_cmos_sensor_8(0X034A, 0X09);
	write_cmos_sensor_8(0X034B, 0X97);
	write_cmos_sensor_8(0X034C, 0X0C);
	write_cmos_sensor_8(0X034D, 0XC0);
	write_cmos_sensor_8(0X034E, 0X09);
	write_cmos_sensor_8(0X034F, 0X90);
	write_cmos_sensor_8(0X0900, 0X00);
	write_cmos_sensor_8(0X0901, 0X00);
	write_cmos_sensor_8(0X0381, 0X01);
	write_cmos_sensor_8(0X0383, 0X01);
	write_cmos_sensor_8(0X0385, 0X01);
	write_cmos_sensor_8(0X0387, 0X01);
	write_cmos_sensor_8(0X0101, 0X00);
	write_cmos_sensor_8(0X0340, 0X09);
	write_cmos_sensor_8(0X0341, 0XE2);
	write_cmos_sensor_8(0X0342, 0X0E);
	write_cmos_sensor_8(0X0343, 0X68);
	write_cmos_sensor_8(0X0200, 0X0D);
	write_cmos_sensor_8(0X0201, 0XD8);
	write_cmos_sensor_8(0X0202, 0X02);
	write_cmos_sensor_8(0X0203, 0X08);
	write_cmos_sensor_8(0x3400, 0x01);
	write_cmos_sensor_8(0x0100, 0x01);
}

static void slim_video_setting(void)
{
	write_cmos_sensor_8(0X0136, 0X18);
	write_cmos_sensor_8(0X0137, 0X00);
	write_cmos_sensor_8(0X0305, 0X06);
	write_cmos_sensor_8(0X0306, 0X00);
	write_cmos_sensor_8(0X0307, 0X8C);
	write_cmos_sensor_8(0X030D, 0X06);
	write_cmos_sensor_8(0X030E, 0X00);
	write_cmos_sensor_8(0X030F, 0XAF);
	write_cmos_sensor_8(0X3C1F, 0X00);
	write_cmos_sensor_8(0X3C17, 0X00);
	write_cmos_sensor_8(0X3C1C, 0X05);
	write_cmos_sensor_8(0X3C1D, 0X15);
	write_cmos_sensor_8(0X0301, 0X04);
	write_cmos_sensor_8(0X0820, 0X02);
	write_cmos_sensor_8(0X0821, 0XBC);
	write_cmos_sensor_8(0X0822, 0X00);
	write_cmos_sensor_8(0X0823, 0X00);
	write_cmos_sensor_8(0X0112, 0X0A);
	write_cmos_sensor_8(0X0113, 0X0A);
	write_cmos_sensor_8(0X0114, 0X03);
	write_cmos_sensor_8(0X3906, 0X00);
	write_cmos_sensor_8(0X0344, 0X01);
	write_cmos_sensor_8(0X0345, 0X68);
	write_cmos_sensor_8(0X0346, 0X02);
	write_cmos_sensor_8(0X0347, 0X00);
	write_cmos_sensor_8(0X0348, 0X0B);
	write_cmos_sensor_8(0X0349, 0X67);
	write_cmos_sensor_8(0X034A, 0X07);
	write_cmos_sensor_8(0X034B, 0X9F);
	write_cmos_sensor_8(0X034C, 0X05);
	write_cmos_sensor_8(0X034D, 0X00);
	write_cmos_sensor_8(0X034E, 0X02);
	write_cmos_sensor_8(0X034F, 0XD0);
	write_cmos_sensor_8(0X0900, 0X01);
	write_cmos_sensor_8(0X0901, 0X22);
	write_cmos_sensor_8(0X0381, 0X01);
	write_cmos_sensor_8(0X0383, 0X01);
	write_cmos_sensor_8(0X0385, 0X01);
	write_cmos_sensor_8(0X0387, 0X03);
	write_cmos_sensor_8(0X0101, 0X00);
	write_cmos_sensor_8(0X0340, 0X09);
	write_cmos_sensor_8(0X0341, 0XE2);
	write_cmos_sensor_8(0X0342, 0X0E);
	write_cmos_sensor_8(0X0343, 0X68);
	write_cmos_sensor_8(0X0200, 0X0D);
	write_cmos_sensor_8(0X0201, 0XD8);
	write_cmos_sensor_8(0X0202, 0X02);
	write_cmos_sensor_8(0X0203, 0X08);
	write_cmos_sensor_8(0x3400, 0x01);
	write_cmos_sensor_8(0x0100, 0x01);
}

#if OTP_4H7
typedef struct s5k4h7_otp_data {
	unsigned char page_flag;
	unsigned char module_id[2];
	unsigned short moduleid;
	unsigned char gloden[8];
	unsigned char unint[8];
	unsigned char data[128];
	unsigned char lscdata[1871];
} S5K4H7_OTP_DATA;

S5K4H7_OTP_DATA s5k4h7_otp_data;

static uint8_t s5k4h7_eeprom[S5K4H7YX_EEPROM_SIZE] = {0};


static calibration_status_t mnf_status;
static calibration_status_t af_status;
static calibration_status_t awb_status;
static calibration_status_t lsc_status;
static calibration_status_t pdaf_status;
static calibration_status_t dual_status;


static struct s5k4h7yx_basic_info_t *basic_info_group = NULL;
static struct s5k4h7yx_awb_info_t *awb_info_group = NULL;
static struct s5k4h7yx_light_source_t *light_source_group = NULL;

#define S5K4H7_MPN_NUM    2
#define S5K4H7_MPN_LENGTH 8
static const char mnf_part_num[S5K4H7_MPN_NUM][S5K4H7_MPN_LENGTH] = {"28D09473", "28D09807"};
#if 0
static struct s5k4h7yx_lsc_info_t *lsc_info_group = NULL;
#endif
static int s5k4h7_read_otp_page_data(int page, int start_add, unsigned char *Buff, int size)
{
	unsigned short stram_flag = 0;
	unsigned short val = 0;
	int i = 0;
	if (NULL == Buff) return 0;

	stram_flag = read_cmos_sensor_8(0x0100); //3
	if (stram_flag == 0) {
		write_cmos_sensor_8(0x0100,0x01);   //3
		mdelay(50);
	}
	write_cmos_sensor_8(0x0a02,page);    //3
	write_cmos_sensor_8(0x3b41,0x01);
	write_cmos_sensor_8(0x3b42,0x03);
	write_cmos_sensor_8(0x3b40,0x01);
	write_cmos_sensor_16(0x0a00,0x0100); //4 otp enable and read start

	for (i = 0; i <= 100; i++)
	{
		mdelay(1);
		val = read_cmos_sensor_8(0x0A01);
		if (val == 0x01)
			break;
	}

	for ( i = 0; i < size; i++ ) {
		Buff[i] = read_cmos_sensor_8(start_add+i); //3
		LOG_INF("+++4h7qt 1 cur page = %d, Buff[%d] = 0x%x\n",page,i,Buff[i]);
	}
	write_cmos_sensor_16(0x0a00,0x0400);
	write_cmos_sensor_16(0x0a00,0x0000); //4 //otp enable and read end

	return 0;
}

static bool s5k4h7_read_valid_data(int page, int start_add)
{
	unsigned char page_flag[2] = {0};
	s5k4h7_read_otp_page_data(page,start_add,&page_flag[0],1);
	s5k4h7_read_otp_page_data(page,start_add,&page_flag[1],1);
	LOG_INF("+++4h7 2 page = %d,page_flag0 = 0x%x,f1 = 0x%x\n",page,page_flag[0],page_flag[1]);
	if (page_flag[0] != 0 || page_flag[1] != 0) {
		//LOGE("+++4h7 3 get vaild page success = %d\n",page);
		s5k4h7_otp_data.page_flag = ((page_flag[0] > page_flag[1])? page_flag[0] : page_flag[1]);
		//LOGE("+++4h7====== s5k4h7_otp_data.page_flag = 0x%x\n",s5k4h7_otp_data.page_flag);
		if (!((getbit(s5k4h7_otp_data.page_flag,6)) && !(getbit(s5k4h7_otp_data.page_flag,7)))) {
			LOG_INF("valid bit 7 = %d,bit 6 = %d\n",getbit(s5k4h7_otp_data.page_flag,7),getbit(s5k4h7_otp_data.page_flag,6));
			LOGE("+++4h7 error data not valid\n");
			return false;
		}else{
			return true;
		}
	}else{
		s5k4h7_otp_data.page_flag = 0;
		return false;
	}
}

static int s5k4h7_get_vaild_data_page(int start_add)
{
	unsigned short page = 21;
	LOG_INF("read flag .....");
	for (page = 21;page <= 29; page+=4) {
		if(s5k4h7_read_valid_data(page, start_add)){
			break;
		}else if (29 == page){
			return 0;
		}
	}

	return page;
}

static int s5k4h7_get_vaild_lsc_data_page(int start_add)
{
	unsigned short page = 33;
	LOG_INF("read flag .....");
	for (page = 33;page <= 93; page+=30) {
		if(s5k4h7_read_valid_data(page, start_add)){
			break;
		}else if (93 == page){
			return 0;
		}
	}

	return page;
}

static int s5k4h7_read_data_kernel(void)
{
	unsigned int page = 0;
	unsigned int i = 0;
	unsigned int sum = 0;

	page = s5k4h7_get_vaild_data_page(0x0a04);
	if(!page) return -1;

	//read module id

	s5k4h7_read_otp_page_data(page, S5K4H7_MODULE_ID_START_ADD, s5k4h7_otp_data.module_id, S5K4H7_MODULE_ID_LENGTH);
	for (i = 0;i < S5K4H7_MODULE_ID_LENGTH ; i++ ) {
		LOG_INF ("+++4h7 6 page = %d modulue_id[%d] = 0x%x",page,i,s5k4h7_otp_data.module_id[i]);
		sum += s5k4h7_otp_data.module_id[i];
	}
	if (!sum) return -1;

	s5k4h7_otp_data.moduleid = ((s5k4h7_otp_data.module_id[0] << 8)& 0xFF00) | (s5k4h7_otp_data.module_id[1] & 0x00FF);
	LOG_INF("s5k4h7_otp_data.moduleid= 0x%x",s5k4h7_otp_data.moduleid);

	page = s5k4h7_get_vaild_data_page(0x0a3c);
	if(!page) return -1;

	//read all awb
	LOGE("s5k4h7 dump all awb start ===\n");
	s5k4h7_read_otp_page_data(page, 0x0a3c, s5k4h7_otp_data.data,8);
	s5k4h7_read_otp_page_data(page + 1, 0x0a04, s5k4h7_otp_data.data + 8, 51);
	LOGE("s5k4h7 dump all awb end ===\n");
	//read awb
	s5k4h7_read_otp_page_data(page, 0x0a3d, s5k4h7_otp_data.gloden, 7);
	s5k4h7_read_otp_page_data(page + 1, 0x0a04, s5k4h7_otp_data.gloden + 7, 1);
	s5k4h7_read_otp_page_data(page + 1, 0x0a19, s5k4h7_otp_data.unint, 8);

	page = s5k4h7_get_vaild_lsc_data_page(0x0a04);
	if(!page) return -1;

	for (i = 0; i < 30 ; i++ ) {
		if (i == 29) {
			s5k4h7_read_otp_page_data(page + i, 0x0a04, s5k4h7_otp_data.lscdata + (64*i), 15);
		} else {
			s5k4h7_read_otp_page_data(page + i, 0x0a04, s5k4h7_otp_data.lscdata + (64*i), 64);
		}

	}

	return 0;
}

static int s5k4h7_read_data_from_otp(void)
{
	LOGE("s5k4h7yx_read_data_from_otp -E");
	/* Read otp data of three group*/
	s5k4h7_read_otp_page_data(21, 0x0a04, s5k4h7_eeprom, 64);
	s5k4h7_read_otp_page_data(22, 0x0a04, s5k4h7_eeprom + 64, 64);
	s5k4h7_read_otp_page_data(23, 0x0a04, s5k4h7_eeprom + 128, 64);
	s5k4h7_read_otp_page_data(24, 0x0a04, s5k4h7_eeprom + 192, 46);
	s5k4h7_read_otp_page_data(25, 0x0a04, s5k4h7_eeprom + 238, 64);
	s5k4h7_read_otp_page_data(26, 0x0a04, s5k4h7_eeprom + 302, 64);
	s5k4h7_read_otp_page_data(27, 0x0a04, s5k4h7_eeprom + 366, 64);
	s5k4h7_read_otp_page_data(28, 0x0a04, s5k4h7_eeprom + 430, 46);
	s5k4h7_read_otp_page_data(29, 0x0a04, s5k4h7_eeprom + 476, 64);
	s5k4h7_read_otp_page_data(30, 0x0a04, s5k4h7_eeprom + 540, 64);
	s5k4h7_read_otp_page_data(31, 0x0a04, s5k4h7_eeprom + 604, 64);
	s5k4h7_read_otp_page_data(32, 0x0a04, s5k4h7_eeprom + 668, 46);
	//TODO: Add LSC data
	LOGE("s5k4h7yx_read_data_from_otp -X");
	return 0;
}

unsigned int S5K4H7_OTP_Read_Data(unsigned int addr,unsigned char *data, unsigned int size)
{
	if (size == 4) { //read module id
		memcpy(data, s5k4h7_otp_data.module_id, size);
		LOGE("add = 0x%x,read module id\n",addr);
	} else if (size == 8) { //read single awb data
		if (addr >= 0x0a3D) {
			memcpy(data, (s5k4h7_otp_data.gloden), size);
			LOGE("add = 0x%x, read golden\n",addr);
		} else {
			memcpy(data,(s5k4h7_otp_data.unint), size);
			LOGE("add = 0x%x, read unint\n",addr);
		}
	} else if (size >=1868){
		if (addr == 0x0a04) {
			memcpy(data, (s5k4h7_otp_data.lscdata), size);
		}else {
			memcpy(data, (s5k4h7_otp_data.lscdata + 1), size);
		}
	} else { //read all awb checksum
		memcpy(data, (s5k4h7_otp_data.data), size);
	}

	return size;
}

static void s5k4h7_eeprom_dump_bin(const char *file_name, uint32_t size, const void *data)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0666);
	if (IS_ERR_OR_NULL(fp)) {
		ret = PTR_ERR(fp);
		LOG_INF("open file error(%s), error(%d)\n",  file_name, ret);
		goto p_err;
	}

	ret = vfs_write(fp, (const char *)data, size, &fp->f_pos);
	if (ret < 0) {
		LOG_INF("file write fail(%s) to EEPROM data(%d)", file_name, ret);
		goto p_err;
	}

	LOG_INF("wirte to file(%s)\n", file_name);
p_err:
	if (!IS_ERR_OR_NULL(fp))
		filp_close(fp, NULL);

	set_fs(old_fs);
	LOG_INF(" end writing file");
}

static uint32_t convert_crc(uint8_t *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
}

static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static uint16_t to_uint16_swap(uint8_t *data)
{
	uint16_t converted;
	memcpy(&converted, data, sizeof(uint16_t));
	return ntohs(converted);
}

static int32_t eeprom_util_check_crc16(uint8_t *data, uint32_t size, uint32_t ref_crc)
{
	int32_t crc_match = 0;
	uint16_t crc = 0x0000;
	uint16_t crc_reverse = 0x0000;
	uint32_t i, j;

	uint32_t tmp;
	uint32_t tmp_reverse;

	/* Calculate both methods of CRC since integrators differ on
	* how CRC should be calculated. */
	for (i = 0; i < size; i++) {
		tmp_reverse = crc_reverse_byte(data[i]);
		tmp = data[i] & 0xff;
		for (j = 0; j < 8; j++) {
			if (((crc & 0x8000) >> 8) ^ (tmp & 0x80))
				crc = (crc << 1) ^ 0x8005;
			else
				crc = crc << 1;
			tmp <<= 1;

			if (((crc_reverse & 0x8000) >> 8) ^ (tmp_reverse & 0x80))
				crc_reverse = (crc_reverse << 1) ^ 0x8005;
			else
				crc_reverse = crc_reverse << 1;

			tmp_reverse <<= 1;
		}
	}

	crc_reverse = (crc_reverse_byte(crc_reverse) << 8) |
		crc_reverse_byte(crc_reverse >> 8);

	if (crc == ref_crc || crc_reverse == ref_crc)
		crc_match = 1;

	LOG_INF("REF_CRC 0x%x CALC CRC 0x%x CALC Reverse CRC 0x%x matches? %d\n",
		ref_crc, crc, crc_reverse, crc_match);

	return crc_match;
}

static uint8_t mot_eeprom_util_check_awb_limits(awb_t unit, awb_t golden)
{
	uint8_t result = 0;

	if (unit.r < AWB_R_MIN || unit.r > AWB_R_MAX) {
		LOG_INF("unit r out of range! MIN: %d, r: %d, MAX: %d",
			AWB_R_MIN, unit.r, AWB_R_MAX);
		result = 1;
	}
	if (unit.gr < AWB_GR_MIN || unit.gr > AWB_GR_MAX) {
		LOG_INF("unit gr out of range! MIN: %d, gr: %d, MAX: %d",
			AWB_GR_MIN, unit.gr, AWB_GR_MAX);
		result = 1;
	}
	if (unit.gb < AWB_GB_MIN || unit.gb > AWB_GB_MAX) {
		LOG_INF("unit gb out of range! MIN: %d, gb: %d, MAX: %d",
			AWB_GB_MIN, unit.gb, AWB_GB_MAX);
		result = 1;
	}
	if (unit.b < AWB_B_MIN || unit.b > AWB_B_MAX) {
		LOG_INF("unit b out of range! MIN: %d, b: %d, MAX: %d",
			AWB_B_MIN, unit.b, AWB_B_MAX);
		result = 1;
	}

	if (golden.r < AWB_R_MIN || golden.r > AWB_R_MAX) {
		LOG_INF("golden r out of range! MIN: %d, r: %d, MAX: %d",
			AWB_R_MIN, golden.r, AWB_R_MAX);
		result = 1;
	}
	if (golden.gr < AWB_GR_MIN || golden.gr > AWB_GR_MAX) {
		LOG_INF("golden gr out of range! MIN: %d, gr: %d, MAX: %d",
			AWB_GR_MIN, golden.gr, AWB_GR_MAX);
		result = 1;
	}
	if (golden.gb < AWB_GB_MIN || golden.gb > AWB_GB_MAX) {
		LOG_INF("golden gb out of range! MIN: %d, gb: %d, MAX: %d",
			AWB_GB_MIN, golden.gb, AWB_GB_MAX);
		result = 1;
	}
	if (golden.b < AWB_B_MIN || golden.b > AWB_B_MAX) {
		LOG_INF("golden b out of range! MIN: %d, b: %d, MAX: %d",
			AWB_B_MIN, golden.b, AWB_B_MAX);
		result = 1;
	}

	return result;
}

static uint8_t mot_eeprom_util_calculate_awb_factors_limit(awb_t unit, awb_t golden,
		awb_limit_t limit)
{
	uint32_t r_g;
	uint32_t b_g;
	uint32_t golden_rg, golden_bg;
	uint32_t gr_gb;
	uint32_t golden_gr_gb;
	uint32_t r_g_golden_min;
	uint32_t r_g_golden_max;
	uint32_t b_g_golden_min;
	uint32_t b_g_golden_max;

	LOG_INF("unit.r_g = 0x%x, unit.b_g=0x%x,unit.gr_gb=0x%x \n",unit.r_g,unit.b_g,unit.gr_gb);
	LOG_INF("golden.r_g = 0x%x, golden.b_g=0x%x,golden.gr_gb=0x%x \n",golden.r_g,golden.b_g,golden.gr_gb);


	LOG_INF("limit golden  0x%x, 0x%x ,0x%x 0x%x \n",limit.r_g_golden_min,limit.r_g_golden_max,
		limit.b_g_golden_min,limit.b_g_golden_max);


	r_g = unit.r_g *1000;
	b_g = unit.b_g*1000;
	gr_gb = unit.gr_gb*100;

	golden_rg = golden.r_g*1000;
	golden_bg = golden.b_g*1000;
	golden_gr_gb = golden.gr_gb*100;

	r_g_golden_min = limit.r_g_golden_min*16384;
	r_g_golden_max = limit.r_g_golden_max*16384;
	b_g_golden_min = limit.b_g_golden_min*16384;
	b_g_golden_max = limit.b_g_golden_max*16384;

	LOG_INF("rg = %d, bg=%d,rgmin=%d,bgmax =%d\n",r_g,b_g,r_g_golden_min,r_g_golden_max);
	LOG_INF("grg = %d, gbg=%d,bgmin=%d,bgmax =%d\n",golden_rg,golden_bg,b_g_golden_min,b_g_golden_max);

	if (r_g < (golden_rg - r_g_golden_min) || r_g > (golden_rg + r_g_golden_max)) {
		LOG_INF("Final RG calibration factors out of range!");
		return 1;
	}

	if (b_g < (golden_bg - b_g_golden_min) || b_g > (golden_bg + b_g_golden_max)) {
		LOG_INF("Final BG calibration factors out of range!");
		return 1;
	}

	LOG_INF("gr_gb = %d, golden_gr_gb=%d \n",gr_gb,golden_gr_gb);

	if (gr_gb < AWB_GR_GB_MIN || gr_gb > AWB_GR_GB_MAX) {
		LOG_INF("Final gr_gb calibration factors out of range!!!");
		return 1;
	}

	if (golden_gr_gb < AWB_GR_GB_MIN || golden_gr_gb > AWB_GR_GB_MAX) {
		LOG_INF("Final golden_gr_gb calibration factors out of range!!!");
		return 1;
	}

	return 0;
}

static calibration_status_t s5k4h7_check_manufacturing_data(void *data)
{
	struct s5k4h7yx_eeprom_t *eeprom = (struct s5k4h7yx_eeprom_t*)data;

	uint8_t *crc_start;
        uint8_t i = 0;
        calibration_status_t status = CRC_FAILURE;

	LOG_INF("Check Manufacturing Data Enter basic_info_flag1=%d", eeprom->basic_info_flag1);
	/*Get valid group and check data crc*/
	if(((eeprom->basic_info_flag1>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->basic_info_flag1;
		basic_info_group = &eeprom->basic_info_group1;
	}else if(((eeprom->basic_info_flag1>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->basic_info_flag2;
		basic_info_group = &eeprom->basic_info_group2;
	}else if(((eeprom->basic_info_flag1>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->basic_info_flag3;
		basic_info_group = &eeprom->basic_info_group3;
	}else{
		LOG_INF("failed: invalid Manufacturing data");
		return CRC_FAILURE;
	}

	if(!eeprom_util_check_crc16(crc_start,
		S5K4H7YX_EEPROM_CRC_MANUFACTURING_SIZE + 1,
		convert_crc(basic_info_group->manufacture_crc16))) {
		LOG_INF("Manufacturing CRC Fails!");
		return CRC_FAILURE;
	}

        for (i = 0; i < S5K4H7_MPN_NUM; i++) {
	     if(strncmp(basic_info_group->mpn, mnf_part_num[i], S5K4H7_MPN_LENGTH) == 0) {
	        status = NO_ERRORS;
                LOG_INF("Match manufacturing part number (%d - %s) !", i, basic_info_group->mpn);
                break;
	     }
        }

	LOG_INF("Manufacturing CRC status(%d)", status);
	return status;
}

static calibration_status_t s5k4h7_check_awb_data(void *data)
{
	struct s5k4h7yx_eeprom_t *eeprom = (struct s5k4h7yx_eeprom_t*)data;

	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;
	uint8_t *crc_start;

	LOG_INF("Check AWB Data Enter");
	/*Get valid group and check data crc*/
	if(((eeprom->awb_info_flag1>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->awb_info_flag1;
		awb_info_group = &eeprom->awb_info_group1;
	}else if(((eeprom->awb_info_flag2>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->awb_info_flag2;
		awb_info_group = &eeprom->awb_info_group2;
	}else if(((eeprom->awb_info_flag3>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->awb_info_flag3;
		awb_info_group = &eeprom->awb_info_group3;
	}else{
		LOG_INF("failed: invalid AWB info data");
		return CRC_FAILURE;
	}

	if(!eeprom_util_check_crc16(crc_start,
		S5K4H7YX_EEPROM_CRC_AWB_CAL_SIZE + 1,
		convert_crc(awb_info_group->awb_crc16))) {
		LOG_INF("AWB CRC Fails!");
		return CRC_FAILURE;
	}

	if(((eeprom->ls_info_flag1>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->ls_info_flag1;
		light_source_group = &eeprom->ls_info_group1;
	}else if(((eeprom->ls_info_flag2>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->ls_info_flag2;
		light_source_group = &eeprom->ls_info_group2;
	}else if(((eeprom->ls_info_flag3>>6)&0x03) == VALID_FLAG){
		crc_start = &eeprom->ls_info_flag3;
		light_source_group = &eeprom->ls_info_group3;
	}else{
		LOG_INF("failed: invalid light source info data");
		return CRC_FAILURE;
	}

	if(!eeprom_util_check_crc16(crc_start,
		S5K4H7YX_EEPROM_CRC_LIGHT_SOURCE_SIZE + 1,
		convert_crc(light_source_group->ls_crc16))) {
		LOG_INF("light source CRC Fails!");
		return CRC_FAILURE;
	}

	unit.r = to_uint16_swap(awb_info_group->awb_src_1_r);
	unit.gr = to_uint16_swap(awb_info_group->awb_src_1_gr);
	unit.gb = to_uint16_swap(awb_info_group->awb_src_1_gb);
	unit.b = to_uint16_swap(awb_info_group->awb_src_1_b);
	unit.r_g = to_uint16_swap(awb_info_group->awb_src_1_rg_ratio);
	unit.b_g = to_uint16_swap(awb_info_group->awb_src_1_bg_ratio);
	unit.gr_gb = to_uint16_swap(awb_info_group->awb_src_1_gr_gb_ratio);

	golden.r = to_uint16_swap(awb_info_group->awb_src_1_golden_r);
	golden.gr = to_uint16_swap(awb_info_group->awb_src_1_golden_gr);
	golden.gb = to_uint16_swap(awb_info_group->awb_src_1_golden_gb);
	golden.b = to_uint16_swap(awb_info_group->awb_src_1_golden_b);
	golden.r_g = to_uint16_swap(awb_info_group->awb_src_1_golden_rg_ratio);
	golden.b_g = to_uint16_swap(awb_info_group->awb_src_1_golden_bg_ratio);
	golden.gr_gb = to_uint16_swap(awb_info_group->awb_src_1_golden_gr_gb_ratio);
	if (mot_eeprom_util_check_awb_limits(unit, golden)) {
		LOG_INF("AWB CRC limit Fails!");
		return LIMIT_FAILURE;
	}

	golden_limit.r_g_golden_min = light_source_group->awb_r_g_golden_min_limit[0];
	golden_limit.r_g_golden_max = light_source_group->awb_r_g_golden_max_limit[0];
	golden_limit.b_g_golden_min = light_source_group->awb_b_g_golden_min_limit[0];
	golden_limit.b_g_golden_max = light_source_group->awb_b_g_golden_max_limit[0];

	if (mot_eeprom_util_calculate_awb_factors_limit(unit, golden,golden_limit)) {
		LOG_INF("AWB CRC factor limit Fails!");
		return LIMIT_FAILURE;
	}
	LOG_INF("AWB CRC Pass");
	return NO_ERRORS;
}

static void s5k4h7_eeprom_format_calibration_data(void *data)
{
	if (NULL == data) {
		LOG_INF("data is NULL");
		return;
	}

	mnf_status = s5k4h7_check_manufacturing_data(data);
	af_status = 0;
	awb_status = s5k4h7_check_awb_data(data);
	lsc_status = 0;
	pdaf_status = 0;
	dual_status = 0;

	LOG_INF("status mnf:%d, af:%d, awb:%d, lsc:%d, pdaf:%d, dual:%d",
		mnf_status, af_status, awb_status, lsc_status, pdaf_status, dual_status);
}

static void s5k4h7_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;

	if (basic_info_group == NULL) {
		LOG_INF("basic_info_group is NULL");
		return;
	}

	ret = snprintf(mnf->table_revision, MAX_CALIBRATION_STRING, "0x%x",
		basic_info_group->eeprom_table_version[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->table_revision failed");
		mnf->table_revision[0] = 0;
	}

	ret = snprintf(mnf->mot_part_number, MAX_CALIBRATION_STRING, "%c%c%c%c%c%c%c%c",
		basic_info_group->mpn[0], basic_info_group->mpn[1], basic_info_group->mpn[2],
		basic_info_group->mpn[3], basic_info_group->mpn[4], basic_info_group->mpn[5],
		basic_info_group->mpn[6], basic_info_group->mpn[7]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->mot_part_number failed");
		mnf->mot_part_number[0] = 0;
	}

	ret = snprintf(mnf->actuator_id, MAX_CALIBRATION_STRING, "0x%x", basic_info_group->actuator_id[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->actuator_id failed");
		mnf->actuator_id[0] = 0;
	}

	ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "0x%x", basic_info_group->lens_id[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->lens_id failed");
		mnf->lens_id[0] = 0;
	}

	if (basic_info_group->manufacturer_id[0] == 'S' && basic_info_group->manufacturer_id[1] == 'U') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Sunny");
	} else if (basic_info_group->manufacturer_id[0] == 'O' && basic_info_group->manufacturer_id[1] == 'F') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "OFilm");
	} else if (basic_info_group->manufacturer_id[0] == 'Q' && basic_info_group->manufacturer_id[1] == 'T') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Qtech");
	} else {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Unknown");
		LOG_INF("unknown manufacturer_id");
	}

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->integrator failed");
		mnf->integrator[0] = 0;
	}

	ret = snprintf(mnf->factory_id, MAX_CALIBRATION_STRING, "%c%c",
		basic_info_group->factory_id[0], basic_info_group->factory_id[1]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->factory_id failed");
		mnf->factory_id[0] = 0;
	}

	ret = snprintf(mnf->manufacture_line, MAX_CALIBRATION_STRING, "%u",
		basic_info_group->manufacture_line[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->manufacture_line failed");
		mnf->manufacture_line[0] = 0;
	}

	ret = snprintf(mnf->manufacture_date, MAX_CALIBRATION_STRING, "20%u/%u/%u",
		basic_info_group->manufacture_date[0], basic_info_group->manufacture_date[1],
		basic_info_group->manufacture_date[2]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->manufacture_date failed");
		mnf->manufacture_date[0] = 0;
	}

	ret = snprintf(mnf->serial_number, MAX_CALIBRATION_STRING, "%02x%02x%02x%02x%02x%02x%02x%02x",
		basic_info_group->serial_number[0], basic_info_group->serial_number[1],
		basic_info_group->serial_number[2], basic_info_group->serial_number[3],
		basic_info_group->serial_number[4], basic_info_group->serial_number[5],
		basic_info_group->serial_number[6], basic_info_group->serial_number[7]);

	s5k4h7_eeprom_dump_bin(WIDE_SERIAL_NUM_DATA_PATH,  S5K4H7YX_SERIAL_NUM_SIZE,  basic_info_group->serial_number);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->serial_number failed");
		mnf->serial_number[0] = 0;
	}
}
#endif

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
//extern void app_get_back_sensor_name(char *back_name);
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			LOG_INF("s5k4h7mipiraw_Sensor get_imgsensor_id *sensor_id = %x\r\n", *sensor_id);
			if (*sensor_id == imgsensor_info.sensor_id) {
#if OTP_4H7
				s5k4h7_read_data_kernel();
				s5k4h7_read_data_from_otp();
				s5k4h7_eeprom_dump_bin(EEPROM_DATA_PATH, S5K4H7YX_EEPROM_SIZE, (void *)s5k4h7_eeprom);
				s5k4h7_eeprom_format_calibration_data((void *)s5k4h7_eeprom);
#endif
#if 0
				s5k4h7_read_data_kernel();
#if INCLUDE_NO_OTP_4H7
				if ((s5k4h7_otp_data.moduleid > 0) && (s5k4h7_otp_data.moduleid < 0xFFFF)) {
#endif
					if ((s5k4h7_otp_data.moduleid != S5K4H7_MODULE_ID_BINGO) &&
						(s5k4h7_otp_data.moduleid != S5K4H7_MODULE_ID_LIMA)){
						*sensor_id = 0xFFFFFFFF;
						return ERROR_SENSOR_CONNECT_FAIL;
					} else {
						s5k4h7_read_data_from_otp();
						s5k4h7_eeprom_dump_bin(EEPROM_DATA_PATH, S5K4H7YX_EEPROM_SIZE, (void *)s5k4h7_eeprom);
						s5k4h7_eeprom_format_calibration_data((void *)s5k4h7_eeprom);
						LOG_INF("This is qtech --->s5k4h7 otp data vaild ...");
					}
#if INCLUDE_NO_OTP_4H7
				} else {
					LOG_INF("This is s5k4h7, but no otp data ...");
				}
#endif
#endif
				LOG_INF("s5k4h7 i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
				break;
			}
			LOG_INF("s5k4h7 read sensor id fail, id: 0x%x\n", *sensor_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	LOG_INF("S5K4H7_OTP_CheckID successfully...\n");
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

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			LOG_INF("s5k4h7mipiraw open sensor_id = %x\r\n", sensor_id);
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
#if 0
#if INCLUDE_NO_OTP_4H7
				if ((s5k4h7_otp_data.moduleid > 0) && (s5k4h7_otp_data.moduleid < 0xFFFF)) {
#endif
					if ((s5k4h7_otp_data.moduleid != S5K4H7_MODULE_ID_BINGO) &&
						(s5k4h7_otp_data.moduleid != S5K4H7_MODULE_ID_LIMA)){
						sensor_id = 0xFFFF;
						return ERROR_SENSOR_CONNECT_FAIL;
					} else
						LOG_INF("This is qtech --->s5k4h7 otp data vaild...");
#if INCLUDE_NO_OTP_4H7
				} else {
					LOG_INF("This is s5k4h7, but no otp data ...");
				}
#endif
#endif
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x\n", sensor_id);
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
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);


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
	imgsensor.sensor_mode = IMGSENSOR_MODE_HS_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
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
}	/*	hs_video	 */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

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

	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;
	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;
	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_INFO_STRUCT *sensor_info,
		MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; /* inverse with datasheet */
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
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;		 /* The frame of setting shutter default 0 for TG int */
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
	sensor_info->SensorWidthSampling = 0;  /* 0 is default 1x */
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x */
	sensor_info->SensorPacketECCOrder = 1;
#if OTP_4H7
	sensor_info->calibration_status.mnf = mnf_status;
	sensor_info->calibration_status.af = af_status;
	sensor_info->calibration_status.awb = awb_status;
	sensor_info->calibration_status.lsc = lsc_status;
	sensor_info->calibration_status.pdaf = pdaf_status;
	sensor_info->calibration_status.dual = dual_status;
	s5k4h7_eeprom_get_mnf_data((void *)s5k4h7_eeprom, &sensor_info->mnf_calibration);
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

	if (framerate == 0)
		return ERROR_NONE;
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
		imgsensor.current_fps = 146;
	else
		imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) /* enable auto flicker */
		imgsensor.autoflicker_en = KAL_TRUE;
	else /* Cancel Auto flick */
		imgsensor.autoflicker_en = KAL_FALSE;
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
			if (imgsensor.frame_length > imgsensor.shutter) set_dummy();
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			if (framerate == 0)
				return ERROR_NONE;
			frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) set_dummy();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:

			if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n", framerate, imgsensor_info.cap.max_framerate/10);
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) set_dummy();
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) set_dummy();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) set_dummy();
			break;
		default:  /* coding with  preview scenario by default */
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter) set_dummy();
			LOG_INF("error scenario_id = %d, we use preview scenario\n", scenario_id);
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
		default:
			break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);
	/* enable = false; */
	if (enable) {
		/* 0x5E00[8]: 1 enable,  0 disable */
		/* 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK */
		write_cmos_sensor_8(0x0601, 0x02);
	} else {
		/* 0x5E00[8]: 1 enable,  0 disable */
		/* 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK */
		write_cmos_sensor_8(0x0601, 0x00);
	}
	write_cmos_sensor_8(0x3200, 0x00);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
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
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
	LOG_INF("feature_id = %d", feature_id);
	switch (feature_id) {
		case SENSOR_FEATURE_GET_AE_EFFECTIVE_FRAME_FOR_LE:
			*feature_return_para_32 = imgsensor.current_ae_effective_frame;
			break;
		case SENSOR_FEATURE_GET_AE_FRAME_MODE_FOR_LE:
			memcpy(feature_return_para_32, &imgsensor.ae_frm_mode,
			sizeof(struct IMGSENSOR_AE_FRM_MODE));
			break;
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
		break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*feature_return_para_32 = 1; /*BINNING_NONE*/
			break;
		default:
			*feature_return_para_32 = 2; /*BINNING_AVERAGED*/
			break;
		}
		pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
		/*
		 * 1, if driver support new sw frame sync
		 * set_shutter_frame_length() support third para auto_extend_en
		 */
		*(feature_data + 1) = 1;
		/* margin info by scenario */
		*(feature_data + 2) = imgsensor_info.margin;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
			*feature_return_para_16++ = imgsensor.line_length;
			*feature_return_para_16 = imgsensor.frame_length;
			*feature_para_len = 4;
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
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= (imgsensor_info.pre.framelength << 16)
					+ imgsensor_info.pre.linelength;
				break;
			}
			break;

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
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
				break;
			}
			break;

		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk, imgsensor.current_fps);
			*feature_return_para_32 = imgsensor.pclk;
			*feature_para_len = 4;
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			set_shutter(*feature_data);
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			break;
		case SENSOR_FEATURE_SET_GAIN:
			set_gain((UINT16) *feature_data);
			break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
			break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			if ((sensor_reg_data->RegData>>8) > 0)
				write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			else
				write_cmos_sensor_8(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER:
			sensor_reg_data->RegData = read_cmos_sensor(sensor_reg_data->RegAddr);
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			/* get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE */
			/* if EEPROM does not exist in camera module. */
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
			set_auto_flicker_mode((BOOL)*feature_data_16, *(feature_data_16+1));
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			set_max_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*feature_data, *(feature_data+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*(feature_data), (MUINT32 *)(uintptr_t)(*(feature_data+1)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
			set_test_pattern_mode((BOOL)*feature_data);
			break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: /* for factory mode auto testing */
			*feature_return_para_32 = imgsensor_info.checksum_value;
			*feature_para_len = 4;
			break;
		case SENSOR_FEATURE_SET_FRAMERATE:
			spin_lock(&imgsensor_drv_lock);
			imgsensor.current_fps = *feature_data_32;
			spin_unlock(&imgsensor_drv_lock);
			break;
		case SENSOR_FEATURE_SET_HDR:
			LOG_INF("Warning! Not Support IHDR Feature");
			spin_lock(&imgsensor_drv_lock);
			imgsensor.ihdr_en = KAL_FALSE;
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
			LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n", (UINT16)*feature_data, (UINT16)*(feature_data+1), (UINT16)*(feature_data+2));
			ihdr_write_shutter_gain((UINT16)*feature_data, (UINT16)*(feature_data+1), (UINT16)*(feature_data+2));
			break;
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
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					 *(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.pre.mipi_pixel_rate;
					break;
			}
			break;
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

UINT32 MOT_TONGA_S5K4H7_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}

