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
#include "mot_s5k4h7mipiraw_otp.h"
#include "mot_tonga_s5k4h7mipiraw_Sensor.h"


/****************************Modify following Strings for debug****************************/
#define PFX "s5k4h7_camera_sensor"

#define LOG_1 LOG_INF("s5k4h7,MIPI 4LANE\n")
#define LOG_INF(format, args...)    pr_err(PFX "[%s] " format, __func__, ##args)
#define LOGE(format, args...)    pr_err(PFX "[%s] " format, __func__, ##args)

#define S5K4H7YX_EEPROM_SIZE 715
#define S5K4H7YX_SERIAL_NUM_SIZE 8


static DEFINE_SPINLOCK(imgsensor_drv_lock);
static bool bIsLongExposure = KAL_FALSE;
static mot_calibration_info_t s5k4h7_cal_info = {0};
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
		.mipi_pixel_rate = 291200000,
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

	.isp_driving_current = ISP_DRIVING_2MA,     /* mclk driving current */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,    /* sensor_interface_type */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,        /* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_settle_delay_mode = 0,                /* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0x5A,0xff},
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
	write_cmos_sensor_8(0X030F, 0XB6);
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
				s5k4h7_otp_data();
				s5k4h7_get_cal_info(&s5k4h7_cal_info);
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
	sensor_info->calibration_status.mnf   = s5k4h7_cal_info.mnf_status;
	sensor_info->calibration_status.af    = s5k4h7_cal_info.af_status;
	sensor_info->calibration_status.awb   = s5k4h7_cal_info.awb_status;
	sensor_info->calibration_status.lsc   = s5k4h7_cal_info.lsc_status;
	sensor_info->calibration_status.pdaf  = s5k4h7_cal_info.pdaf_status;
	sensor_info->calibration_status.dual  = s5k4h7_cal_info.dual_status;

	memcpy(&sensor_info->mnf_calibration, &s5k4h7_cal_info.mnf_cal_data, sizeof(mot_calibration_mnf_t));
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

