// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 mot_tonga_s5kjn1sqmipiraw_Sensor.c
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

#define PFX "MOT_TONGA_S5KJN1SQ_camera_sensor"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


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

#include "mot_tonga_s5kjn1sqmipiraw_Sensor.h"

#define MULTI_WRITE 1
#define EEPROM_READY 0
#define LOG_INF(format, args...)    \
    pr_err(PFX "[%s] " format, __func__, ##args)

#define LOG_ERR(format, args...)    \
    pr_err(PFX "[%s] " format, __func__, ##args)

#define S5KJN1_SERIAL_NUM_SIZE 16
#define MAIN_SERIAL_NUM_DATA_PATH "/data/vendor/camera_dump/serial_number_main.bin"

#if MULTI_WRITE
static const int I2C_BUFFER_LEN = 1020; /*trans# max is 255, each 4 bytes*/
#else
static const int I2C_BUFFER_LEN = 4;
#endif

/*
 * #define LOG_INF(format, args...) pr_debug(
 * PFX "[%s] " format, __func__, ##args)
 */
static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_TONGA_S5KJN1SQ_SENSOR_ID,
	.checksum_value = 0x67b95889,

	.pre = {
		.pclk = 560000000, /*//30fps case*/
		.linelength = 4584, /*//0x2330*/
		.framelength = 4064, /*//0x09F0*/
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080, /*//0x0A20*/
		.grabwindow_height = 3072, /*//0x0794*/
		//grabwindow_height should be 16's N times
		.mipi_data_lp2hs_settle_dc = 0x22,
		.max_framerate = 300,
		.mipi_pixel_rate = 662400000,
	},
	.cap = {
		.pclk = 560000000, /*//30fps case*/
		.linelength = 4584, /*//0x2330*/
		.framelength = 4064, /*//0x09F0*/
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080, /*//0x0A20*/
		.grabwindow_height = 3072, /*//0x0794*/
		//grabwindow_height should be 16's N times
		.mipi_data_lp2hs_settle_dc = 0x22,
		.max_framerate = 300,
		.mipi_pixel_rate = 662400000,
	},
	.normal_video = {
		.pclk = 560000000, /*//30fps case*/
		.linelength = 5910, /*//0x2330*/
		.framelength = 3156, /*//0x09F0*/
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080, /*//0x0A20*/
		.grabwindow_height = 3072, /*//0x0794*/
		//grabwindow_height should be 16's N times
		.mipi_data_lp2hs_settle_dc = 0x22,
		.max_framerate = 300,
		.mipi_pixel_rate = 480000000,
	},
	.margin = 10,		/* sensor framelength & shutter margin */
	.min_shutter = 4,	/* min shutter */

	/* max framelength by sensor register's limitation */
	.max_frame_length = 0xffff,

	/* shutter delay frame for AE cycle,
	 * 2 frame with ispGain_delay-shut_delay=2-0=2
	 */
	.ae_shut_delay_frame = 0,

	/* sensor gain delay frame for AE cycle,
	 * 2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	 */
	.ae_sensor_gain_delay_frame = 0,

	.ae_ispGain_delay_frame = 2,	/* isp gain delay frame for AE cycle */
	.ihdr_support = 0,	/* 1, support; 0,not support */
	.ihdr_le_firstline = 0,	/* 1,le first ; 0, se first */
	.sensor_mode_num = 3,	/* support sensor mode num */

	.cap_delay_frame = 2,	/* enter capture delay frame num */
	.pre_delay_frame = 2,	/* enter preview delay frame num */
	.video_delay_frame = 2,	/* enter video delay frame num */

	/* enter high speed video  delay frame num */
	.hs_video_delay_frame = 2,

	.slim_video_delay_frame = 2,	/* enter slim video delay frame num */

	.isp_driving_current = ISP_DRIVING_6MA,	/* mclk driving current */

	/* sensor_interface_type */
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,

	/* 0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2 */
	.mipi_sensor_type = MIPI_OPHY_NCSI2,

	/* 0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL */
	.mipi_settle_delay_mode = 0,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	/*.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_BAYER_Gr,*/
	.mclk = 24,	/* mclk value, suggest 24 or 26 for 24Mhz or 26Mhz */
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_speed = 1000, /*support 1MHz write*/
	/* record sensor support all write id addr,
	 * only supprt 4 must end with 0xff
	 */
	.i2c_addr_table = {0xac, 0xff},
};


static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,	/* mirrorflip information */

	/* IMGSENSOR_MODE enum value,record current sensor mode,such as:
	 * INIT, Preview, Capture, Video,High Speed Video, Slim Video
	 */
	.sensor_mode = IMGSENSOR_MODE_INIT,

	.shutter = 0x3D0,	/* current shutter */
	.gain = 0x100,		/* current gain */
	.dummy_pixel = 0,	/* current dummypixel */
	.dummy_line = 0,	/* current dummyline */
	.current_fps = 0,	/* full size current fps : 24fps for PIP,
				 * 30fps for Normal or ZSD
				 */

	/* auto flicker enable: KAL_FALSE for disable auto flicker,
	 * KAL_TRUE for enable auto flicker
	 */
	.autoflicker_en = KAL_FALSE,

		/* test pattern mode or not.
		 * KAL_FALSE for in test pattern mode,
		 * KAL_TRUE for normal output
		 */
	.test_pattern = KAL_FALSE,

	/* current scenario id */
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,

	/* sensor need support LE, SE with HDR feature */
	.ihdr_mode = KAL_FALSE,
	.i2c_write_id = 0xAC,	/* record current sensor's i2c write id */

};

/* VC_Num, VC_PixelNum, ModeSelect, EXPO_Ratio, ODValue, RG_STATSMODE */
/* VC0_ID, VC0_DataType, VC0_SIZEH, VC0_SIZE,
 * VC1_ID, VC1_DataType, VC1_SIZEH, VC1_SIZEV
 */
/* VC2_ID, VC2_DataType, VC2_SIZEH, VC2_SIZE,
 * VC3_ID, VC3_DataType, VC3_SIZEH, VC3_SIZEV
 */

/*static SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
 *  {// Preview mode setting
 *  {0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
 *  0x00, 0x2B, 0x0910, 0x06D0, 0x01, 0x00, 0x0000, 0x0000,
 *  0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000},
 * // Video mode setting
 *{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
 *0x00, 0x2B, 0x1220, 0x0DA0, 0x01, 0x00, 0x0000, 0x0000,
 *0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000},
 * // Capture mode setting
 *{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
 *0x00, 0x2B, 0x1220, 0x0DA0, 0x01, 0x00, 0x0000, 0x0000,
 *0x02, 0x30, 0x00B4, 0x0360, 0x03, 0x00, 0x0000, 0x0000}};
 */

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[3] = {
	{ 8192,	6176,	  16,	  16,	4080,	3072,	4080,	3072,	0,	0,	4080,	3072,	0,	0,	4080,	3072}, // Preview
	{ 8192,	6176,	  16,	  16,	4080,	3072,	4080,	3072,	0,	0,	4080,	3072,	0,	0,	4080,	3072}, // capture
	{ 8192,	6176,	  16,	  16,	4080,	3072,	4080,	3072,	0,	0,	4080,	3072,	0,	0,	4080,	3072}, // video
};


/* no mirror flip, and no binning -revised by dj */
/* static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
 * .i4OffsetX = 16,
 * .i4OffsetY = 16,
 * .i4PitchX  = 64,
 * .i4PitchY  = 64,
 * .i4PairNum  =16,
 * .i4SubBlkW  =16,
 * .i4SubBlkH  =16,
 * .i4PosL = {{20,23},{72,23},{36,27},{56,27},{24,43},{68,43},{40,47},
 * {52,47},{40,55},{52,55},{24,59},{68,59},{36,75},{56,75},{20,79},{72,79}},
 * .i4PosR = {{20,27},{72,27},{36,31},{56,31},{24,39},{68,39},{40,43},{52,43},
 * {40,59},{52,59},{24,63},{68,63},{36,71},{56,71},{20,75},{72,75}},
 * .iMirrorFlip = 0,
 * .i4BlockNumX = 72,
 * .i4BlockNumY = 54,
 * };
 */

#if EEPROM_READY //stan
#define RWB_ID_OFFSET 0x0F73
#define EEPROM_READ_ID  0xA4
#define EEPROM_WRITE_ID   0xA5
#endif

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {
		(char)(addr >> 8), (char)(addr & 0xFF),
		(char)(para >> 8), (char)(para & 0xFF) };
	iWriteRegI2C(pusendcmd, 4, imgsensor.i2c_write_id);
}

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *) &get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[3] = {
		(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF) };

	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	pr_debug("dummyline = %d, dummypixels = %d\n",
		imgsensor.dummy_line, imgsensor.dummy_pixel);

	/* return; //for test */
	write_cmos_sensor(0x0340, imgsensor.frame_length);
	write_cmos_sensor(0x0342, imgsensor.line_length);
}				/*      set_dummy  */


static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{

	kal_uint32 frame_length = imgsensor.frame_length;

	pr_debug("framerate = %d, min framelength should enable %d\n",
		framerate, min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	if (frame_length >= imgsensor.min_frame_length)
		imgsensor.frame_length = frame_length;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;

	imgsensor.dummy_line =
		imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;

		imgsensor.dummy_line =
			imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}				/*      set_max_framerate  */

static void write_shutter(kal_uint16 shutter)
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
	if (shutter < imgsensor_info.min_shutter)
		shutter = imgsensor_info.min_shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk
			/ imgsensor.line_length * 10 / imgsensor.frame_length;

		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x0340, imgsensor.frame_length);

		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length);
		pr_debug("(else)imgsensor.frame_length = %d\n",
			imgsensor.frame_length);

	}
	/* Update Shutter */
	write_cmos_sensor(0x0202, shutter);
	pr_debug("shutter =%d, framelength =%d\n",
		shutter, imgsensor.frame_length);

}				/*      write_shutter  */



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
}				/*      set_shutter */



static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0;

	reg_gain = gain / 2;
	return (kal_uint16) reg_gain;
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

	/* gain=1024;//for test */
	/* return; //for test */

	if (gain < BASEGAIN || gain > 16 * BASEGAIN) {
		pr_debug("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 16 * BASEGAIN)
			gain = 16 * BASEGAIN;
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	pr_debug("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x0204, reg_gain);
	/* write_cmos_sensor_8(0x0204,(reg_gain>>8)); */
	/* write_cmos_sensor_8(0x0205,(reg_gain&0xff)); */

	return gain;
}				/*      set_gain  */

static void set_mirror_flip(kal_uint8 image_mirror)
{

	kal_uint8 itemp;

	pr_debug("image_mirror = %d\n", image_mirror);
	itemp = read_cmos_sensor_8(0x0101);
	itemp &= ~0x03;

	switch (image_mirror) {

	case IMAGE_NORMAL:
		write_cmos_sensor_8(0x0101, itemp);
		break;

	case IMAGE_V_MIRROR:
		write_cmos_sensor_8(0x0101, itemp | 0x02);
		break;

	case IMAGE_H_MIRROR:
		write_cmos_sensor_8(0x0101, itemp | 0x01);
		break;

	case IMAGE_HV_MIRROR:
		write_cmos_sensor_8(0x0101, itemp | 0x03);
		break;
	}
}

/*************************************************************************
 * FUNCTION
 *	check_stremoff
 *
 * DESCRIPTION
 *	waiting function until sensor streaming finish.
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

static void check_streamoff(void)
{
	unsigned int i = 0;
	int timeout = (10000 / imgsensor.current_fps) + 1;

	mdelay(3);
	for (i = 0; i < timeout; i++) {
		if (read_cmos_sensor_8(0x0005) != 0xFF)
			mdelay(1);
		else
			break;
	}
	pr_debug("%s exit! %d\n", __func__, i);
}

static kal_uint32 streaming_control(kal_bool enable)
{
	pr_debug("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable) {
		write_cmos_sensor_8(0x0100, 0x01);
	} else {
		write_cmos_sensor_8(0x0100, 0x00);
		check_streamoff();
	}
	return ERROR_NONE;
}

static kal_uint16 table_write_cmos_sensor(kal_uint16 *para, kal_uint32 len)
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

#if MULTI_WRITE
	if ((I2C_BUFFER_LEN - tosend) < 4 || IDX == len || addr != addr_last) {
		iBurstWriteReg_multi(puSendCmd, tosend,
			imgsensor.i2c_write_id, 4, imgsensor_info.i2c_speed);
			tosend = 0;
}
#else
		iWriteRegI2CTiming(puSendCmd, 4,
			imgsensor.i2c_write_id, imgsensor_info.i2c_speed);

		tosend = 0;
#endif

	}
	return 0;
}


static kal_uint16 addr_data_pair_init_jn1sq[] = {
	/*MOT_TONGA_S5KJN1SQ_EVT0_ReferenceSetfile_v0.1b_2020510*/
	/*SW Page*/
	0x6028, 0x2400,
	0x602A, 0x7700,
	0x6F12, 0x1753,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0x6385,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x9386,
	0x6F12, 0xC701,
	0x6F12, 0xB7B7,
	0x6F12, 0x0024,
	0x6F12, 0x3777,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0xC77F,
	0x6F12, 0x1307,
	0x6F12, 0x07C7,
	0x6F12, 0x958F,
	0x6F12, 0x2328,
	0x6F12, 0xD774,
	0x6F12, 0x231A,
	0x6F12, 0xF774,
	0x6F12, 0x012F,
	0x6F12, 0xB777,
	0x6F12, 0x0024,
	0x6F12, 0x1307,
	0x6F12, 0xD003,
	0x6F12, 0x23A0,
	0x6F12, 0xE774,
	0x6F12, 0x1753,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0x2384,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x8547,
	0x6F12, 0x6310,
	0x6F12, 0xF506,
	0x6F12, 0x3786,
	0x6F12, 0x0024,
	0x6F12, 0x9306,
	0x6F12, 0x4601,
	0x6F12, 0x83C7,
	0x6F12, 0x4600,
	0x6F12, 0xA1CB,
	0x6F12, 0xB737,
	0x6F12, 0x0024,
	0x6F12, 0x83A7,
	0x6F12, 0x875C,
	0x6F12, 0x83D6,
	0x6F12, 0x2600,
	0x6F12, 0x83D7,
	0x6F12, 0x271E,
	0x6F12, 0x13D7,
	0x6F12, 0x2700,
	0x6F12, 0xB707,
	0x6F12, 0x0140,
	0x6F12, 0x83D5,
	0x6F12, 0x27F0,
	0x6F12, 0x8357,
	0x6F12, 0x4601,
	0x6F12, 0x1306,
	0x6F12, 0xE7FF,
	0x6F12, 0xB697,
	0x6F12, 0x8D8F,
	0x6F12, 0xC207,
	0x6F12, 0xC183,
	0x6F12, 0x9396,
	0x6F12, 0x0701,
	0x6F12, 0xC186,
	0x6F12, 0x635F,
	0x6F12, 0xD600,
	0x6F12, 0x8907,
	0x6F12, 0x998F,
	0x6F12, 0x9396,
	0x6F12, 0x0701,
	0x6F12, 0xC186,
	0x6F12, 0x9397,
	0x6F12, 0x0601,
	0x6F12, 0xC183,
	0x6F12, 0x37B7,
	0x6F12, 0x0040,
	0x6F12, 0x2311,
	0x6F12, 0xF7A0,
	0x6F12, 0x8280,
	0x6F12, 0xE3D8,
	0x6F12, 0x06FE,
	0x6F12, 0xBA97,
	0x6F12, 0xF917,
	0x6F12, 0xCDB7,
	0x6F12, 0xB717,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x07CA,
	0x6F12, 0xAA97,
	0x6F12, 0x3387,
	0x6F12, 0xB700,
	0x6F12, 0x8D8F,
	0x6F12, 0x83C5,
	0x6F12, 0xD705,
	0x6F12, 0xB747,
	0x6F12, 0x0024,
	0x6F12, 0x9386,
	0x6F12, 0x078F,
	0x6F12, 0x83D7,
	0x6F12, 0xE670,
	0x6F12, 0x0905,
	0x6F12, 0x0347,
	0x6F12, 0xC705,
	0x6F12, 0x630B,
	0x6F12, 0xF500,
	0x6F12, 0x8567,
	0x6F12, 0xB697,
	0x6F12, 0x03A6,
	0x6F12, 0x8794,
	0x6F12, 0xB306,
	0x6F12, 0xB700,
	0x6F12, 0xB296,
	0x6F12, 0x23A4,
	0x6F12, 0xD794,
	0x6F12, 0x2207,
	0x6F12, 0x3305,
	0x6F12, 0xB700,
	0x6F12, 0x4205,
	0x6F12, 0x4181,
	0x6F12, 0x8280,
	0x6F12, 0x5D71,
	0x6F12, 0xA2C6,
	0x6F12, 0xA6C4,
	0x6F12, 0x7324,
	0x6F12, 0x2034,
	0x6F12, 0xF324,
	0x6F12, 0x1034,
	0x6F12, 0x7360,
	0x6F12, 0x0430,
	0x6F12, 0x2AD8,
	0x6F12, 0x2ED6,
	0x6F12, 0x3545,
	0x6F12, 0x9305,
	0x6F12, 0x8008,
	0x6F12, 0x22DA,
	0x6F12, 0x3ECE,
	0x6F12, 0x86C2,
	0x6F12, 0x96C0,
	0x6F12, 0x1ADE,
	0x6F12, 0x1EDC,
	0x6F12, 0x32D4,
	0x6F12, 0x36D2,
	0x6F12, 0x3AD0,
	0x6F12, 0x42CC,
	0x6F12, 0x46CA,
	0x6F12, 0x72C8,
	0x6F12, 0x76C6,
	0x6F12, 0x7AC4,
	0x6F12, 0x7EC2,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x40F5,
	0x6F12, 0x9377,
	0x6F12, 0x8500,
	0x6F12, 0x2A84,
	0x6F12, 0x85C3,
	0x6F12, 0xB737,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x077C,
	0x6F12, 0x03D7,
	0x6F12, 0x6702,
	0x6F12, 0x0507,
	0x6F12, 0x2393,
	0x6F12, 0xE702,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x00ED,
	0x6F12, 0x0545,
	0x6F12, 0xD535,
	0x6F12, 0x1374,
	0x6F12, 0x0408,
	0x6F12, 0x11CC,
	0x6F12, 0xB737,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x077C,
	0x6F12, 0x03D7,
	0x6F12, 0x8705,
	0x6F12, 0x0507,
	0x6F12, 0x239C,
	0x6F12, 0xE704,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x6065,
	0x6F12, 0x9640,
	0x6F12, 0x8642,
	0x6F12, 0x7253,
	0x6F12, 0xE253,
	0x6F12, 0x5254,
	0x6F12, 0x4255,
	0x6F12, 0xB255,
	0x6F12, 0x2256,
	0x6F12, 0x9256,
	0x6F12, 0x0257,
	0x6F12, 0xF247,
	0x6F12, 0x6248,
	0x6F12, 0xD248,
	0x6F12, 0x424E,
	0x6F12, 0xB24E,
	0x6F12, 0x224F,
	0x6F12, 0x924F,
	0x6F12, 0x7370,
	0x6F12, 0x0430,
	0x6F12, 0x7390,
	0x6F12, 0x1434,
	0x6F12, 0x7310,
	0x6F12, 0x2434,
	0x6F12, 0x3644,
	0x6F12, 0xA644,
	0x6F12, 0x6161,
	0x6F12, 0x7300,
	0x6F12, 0x2030,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0xC369,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x83A4,
	0x6F12, 0xC700,
	0x6F12, 0x2A84,
	0x6F12, 0x0146,
	0x6F12, 0xA685,
	0x6F12, 0x1145,
	0x6F12, 0x9790,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x60AD,
	0x6F12, 0x2285,
	0x6F12, 0x97E0,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x40A9,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x87F5,
	0x6F12, 0x03C7,
	0x6F12, 0x0700,
	0x6F12, 0x8546,
	0x6F12, 0x6315,
	0x6F12, 0xD706,
	0x6F12, 0xB776,
	0x6F12, 0x0024,
	0x6F12, 0x03A6,
	0x6F12, 0x468D,
	0x6F12, 0x8945,
	0x6F12, 0x9386,
	0x6F12, 0xA700,
	0x6F12, 0x630F,
	0x6F12, 0xB600,
	0x6F12, 0x9386,
	0x6F12, 0x2700,
	0x6F12, 0x630B,
	0x6F12, 0xE600,
	0x6F12, 0x3717,
	0x6F12, 0x0024,
	0x6F12, 0x0356,
	0x6F12, 0x6738,
	0x6F12, 0x2D47,
	0x6F12, 0x6314,
	0x6F12, 0xE600,
	0x6F12, 0x9386,
	0x6F12, 0xA700,
	0x6F12, 0xB747,
	0x6F12, 0x0024,
	0x6F12, 0x83A5,
	0x6F12, 0xC786,
	0x6F12, 0x2946,
	0x6F12, 0x1305,
	0x6F12, 0x0404,
	0x6F12, 0x83C7,
	0x6F12, 0xC52F,
	0x6F12, 0x2148,
	0x6F12, 0x1D8E,
	0x6F12, 0x8147,
	0x6F12, 0x3387,
	0x6F12, 0xF500,
	0x6F12, 0xB388,
	0x6F12, 0xF600,
	0x6F12, 0x0317,
	0x6F12, 0xE73C,
	0x6F12, 0x8398,
	0x6F12, 0x0800,
	0x6F12, 0x8907,
	0x6F12, 0x1105,
	0x6F12, 0x4697,
	0x6F12, 0x3317,
	0x6F12, 0xC700,
	0x6F12, 0x232E,
	0x6F12, 0xE5FE,
	0x6F12, 0xE391,
	0x6F12, 0x07FF,
	0x6F12, 0x0546,
	0x6F12, 0xA685,
	0x6F12, 0x1145,
	0x6F12, 0x9790,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x60A4,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0x0361,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0xA35C,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x83A4,
	0x6F12, 0x8700,
	0x6F12, 0x4111,
	0x6F12, 0xAA89,
	0x6F12, 0x2E8A,
	0x6F12, 0xB28A,
	0x6F12, 0xA685,
	0x6F12, 0x0146,
	0x6F12, 0x1145,
	0x6F12, 0x36C6,
	0x6F12, 0x9790,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x60A1,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x1387,
	0x6F12, 0x87F5,
	0x6F12, 0x0347,
	0x6F12, 0x2701,
	0x6F12, 0x1384,
	0x6F12, 0x87F5,
	0x6F12, 0xB246,
	0x6F12, 0x0149,
	0x6F12, 0x11CF,
	0x6F12, 0x3767,
	0x6F12, 0x0024,
	0x6F12, 0x0357,
	0x6F12, 0x2777,
	0x6F12, 0xB777,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x07C7,
	0x6F12, 0x0E07,
	0x6F12, 0x03D9,
	0x6F12, 0x871C,
	0x6F12, 0x2394,
	0x6F12, 0xE71C,
	0x6F12, 0x5686,
	0x6F12, 0xD285,
	0x6F12, 0x4E85,
	0x6F12, 0x97E0,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x0088,
	0x6F12, 0x8347,
	0x6F12, 0x2401,
	0x6F12, 0x89C7,
	0x6F12, 0xB777,
	0x6F12, 0x0024,
	0x6F12, 0x239C,
	0x6F12, 0x27E3,
	0x6F12, 0x0546,
	0x6F12, 0xA685,
	0x6F12, 0x1145,
	0x6F12, 0x9790,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0xC09B,
	0x6F12, 0x4101,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0xA357,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0x0353,
	0x6F12, 0x3784,
	0x6F12, 0x0024,
	0x6F12, 0x9307,
	0x6F12, 0x04F4,
	0x6F12, 0x83C7,
	0x6F12, 0x4701,
	0x6F12, 0x5D71,
	0x6F12, 0x2A89,
	0x6F12, 0x8DEF,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x03A4,
	0x6F12, 0x4700,
	0x6F12, 0x0146,
	0x6F12, 0x1145,
	0x6F12, 0xA285,
	0x6F12, 0x9790,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x2098,
	0x6F12, 0x4A85,
	0x6F12, 0x97E0,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x400F,
	0x6F12, 0x0546,
	0x6F12, 0xA285,
	0x6F12, 0x1145,
	0x6F12, 0x9790,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0xA096,
	0x6F12, 0x6161,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0xE351,
	0x6F12, 0x1304,
	0x6F12, 0x04F4,
	0x6F12, 0x0347,
	0x6F12, 0x5401,
	0x6F12, 0x8547,
	0x6F12, 0x6310,
	0x6F12, 0xF716,
	0x6F12, 0xB785,
	0x6F12, 0x0024,
	0x6F12, 0x9384,
	0x6F12, 0x05F8,
	0x6F12, 0x4146,
	0x6F12, 0x9385,
	0x6F12, 0x05F8,
	0x6F12, 0x0A85,
	0x6F12, 0x9740,
	0x6F12, 0x01FC,
	0x6F12, 0xE780,
	0x6F12, 0x6058,
	0x6F12, 0x4146,
	0x6F12, 0x9385,
	0x6F12, 0x0401,
	0x6F12, 0x0808,
	0x6F12, 0x9740,
	0x6F12, 0x01FC,
	0x6F12, 0xE780,
	0x6F12, 0x6057,
	0x6F12, 0x4146,
	0x6F12, 0x9385,
	0x6F12, 0x0402,
	0x6F12, 0x0810,
	0x6F12, 0x9740,
	0x6F12, 0x01FC,
	0x6F12, 0xE780,
	0x6F12, 0x6056,
	0x6F12, 0x4146,
	0x6F12, 0x9385,
	0x6F12, 0x0403,
	0x6F12, 0x0818,
	0x6F12, 0x9740,
	0x6F12, 0x01FC,
	0x6F12, 0xE780,
	0x6F12, 0x6055,
	0x6F12, 0x1C40,
	0x6F12, 0xB7DB,
	0x6F12, 0x0040,
	0x6F12, 0x014B,
	0x6F12, 0xBEC0,
	0x6F12, 0x5C40,
	0x6F12, 0x8149,
	0x6F12, 0x854A,
	0x6F12, 0xBEC2,
	0x6F12, 0x5C44,
	0x6F12, 0x096D,
	0x6F12, 0x370C,
	0x6F12, 0x0040,
	0x6F12, 0xBEC4,
	0x6F12, 0x1C48,
	0x6F12, 0x938B,
	0x6F12, 0x0B03,
	0x6F12, 0x930C,
	0x6F12, 0x0004,
	0x6F12, 0xBEC6,
	0x6F12, 0x0347,
	0x6F12, 0x7401,
	0x6F12, 0x8967,
	0x6F12, 0x631B,
	0x6F12, 0x5701,
	0x6F12, 0x93F7,
	0x6F12, 0xF900,
	0x6F12, 0x9808,
	0x6F12, 0xBA97,
	0x6F12, 0x83C7,
	0x6F12, 0x07FB,
	0x6F12, 0x8A07,
	0x6F12, 0xCA97,
	0x6F12, 0x9C43,
	0x6F12, 0x63DE,
	0x6F12, 0x3A01,
	0x6F12, 0x1387,
	0x6F12, 0x69FE,
	0x6F12, 0x63FA,
	0x6F12, 0xEA00,
	0x6F12, 0x1387,
	0x6F12, 0xA9FD,
	0x6F12, 0x63F6,
	0x6F12, 0xEA00,
	0x6F12, 0x1387,
	0x6F12, 0x49FC,
	0x6F12, 0x63E3,
	0x6F12, 0xEA0A,
	0x6F12, 0x131A,
	0x6F12, 0x1B00,
	0x6F12, 0x9808,
	0x6F12, 0x5297,
	0x6F12, 0x8354,
	0x6F12, 0x07FF,
	0x6F12, 0x0145,
	0x6F12, 0xB384,
	0x6F12, 0xF402,
	0x6F12, 0xB580,
	0x6F12, 0x637A,
	0x6F12, 0x9D00,
	0x6F12, 0x2685,
	0x6F12, 0x9780,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x6052,
	0x6F12, 0x4915,
	0x6F12, 0x1375,
	0x6F12, 0xF50F,
	0x6F12, 0x9C08,
	0x6F12, 0xD297,
	0x6F12, 0x03D7,
	0x6F12, 0x07FE,
	0x6F12, 0xB3D4,
	0x6F12, 0xA400,
	0x6F12, 0xC204,
	0x6F12, 0x6297,
	0x6F12, 0xC180,
	0x6F12, 0x2310,
	0x6F12, 0x9700,
	0x6F12, 0x03D7,
	0x6F12, 0x07FC,
	0x6F12, 0x83D7,
	0x6F12, 0x07FD,
	0x6F12, 0x050B,
	0x6F12, 0x6297,
	0x6F12, 0x8356,
	0x6F12, 0x0700,
	0x6F12, 0x3315,
	0x6F12, 0xF500,
	0x6F12, 0x558D,
	0x6F12, 0x4205,
	0x6F12, 0x4181,
	0x6F12, 0x2310,
	0x6F12, 0xA700,
	0x6F12, 0x8509,
	0x6F12, 0xE395,
	0x6F12, 0x99F7,
	0x6F12, 0x8D47,
	0x6F12, 0x37D4,
	0x6F12, 0x0040,
	0x6F12, 0x2319,
	0x6F12, 0xF40A,
	0x6F12, 0x2316,
	0x6F12, 0x040C,
	0x6F12, 0x9790,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x6093,
	0x6F12, 0xAA84,
	0x6F12, 0x9790,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x2092,
	0x6F12, 0x9307,
	0x6F12, 0x0008,
	0x6F12, 0x33D5,
	0x6F12, 0xA700,
	0x6F12, 0x9307,
	0x6F12, 0x0004,
	0x6F12, 0x1205,
	0x6F12, 0xB3D7,
	0x6F12, 0x9700,
	0x6F12, 0x1375,
	0x6F12, 0x0503,
	0x6F12, 0x8D8B,
	0x6F12, 0x5D8D,
	0x6F12, 0x2319,
	0x6F12, 0xA40C,
	0x6F12, 0x45B5,
	0x6F12, 0x1397,
	0x6F12, 0x1900,
	0x6F12, 0x9394,
	0x6F12, 0x0701,
	0x6F12, 0x5E97,
	0x6F12, 0xC180,
	0x6F12, 0x2310,
	0x6F12, 0x9700,
	0x6F12, 0x6DB7,
	0x6F12, 0x0347,
	0x6F12, 0x6401,
	0x6F12, 0xE315,
	0x6F12, 0xF7FA,
	0x6F12, 0xB784,
	0x6F12, 0x0024,
	0x6F12, 0x9384,
	0x6F12, 0x04F8,
	0x6F12, 0x4146,
	0x6F12, 0x9385,
	0x6F12, 0x0404,
	0x6F12, 0x0A85,
	0x6F12, 0x9740,
	0x6F12, 0x01FC,
	0x6F12, 0xE780,
	0x6F12, 0x2042,
	0x6F12, 0x4146,
	0x6F12, 0x9385,
	0x6F12, 0x0405,
	0x6F12, 0x0808,
	0x6F12, 0x9740,
	0x6F12, 0x01FC,
	0x6F12, 0xE780,
	0x6F12, 0x2041,
	0x6F12, 0x4146,
	0x6F12, 0x9385,
	0x6F12, 0x0406,
	0x6F12, 0x0810,
	0x6F12, 0x9740,
	0x6F12, 0x01FC,
	0x6F12, 0xE780,
	0x6F12, 0x2040,
	0x6F12, 0x4146,
	0x6F12, 0x9385,
	0x6F12, 0x0407,
	0x6F12, 0x0818,
	0x6F12, 0x9740,
	0x6F12, 0x01FC,
	0x6F12, 0xE780,
	0x6F12, 0x203F,
	0x6F12, 0x0357,
	0x6F12, 0x0400,
	0x6F12, 0x8357,
	0x6F12, 0x2400,
	0x6F12, 0x37DB,
	0x6F12, 0x0040,
	0x6F12, 0x2310,
	0x6F12, 0xE104,
	0x6F12, 0x2311,
	0x6F12, 0xF104,
	0x6F12, 0x2312,
	0x6F12, 0xE104,
	0x6F12, 0x2313,
	0x6F12, 0xF104,
	0x6F12, 0x0357,
	0x6F12, 0x8400,
	0x6F12, 0x8357,
	0x6F12, 0xA400,
	0x6F12, 0x814A,
	0x6F12, 0x2314,
	0x6F12, 0xE104,
	0x6F12, 0x2315,
	0x6F12, 0xF104,
	0x6F12, 0x2316,
	0x6F12, 0xE104,
	0x6F12, 0x2317,
	0x6F12, 0xF104,
	0x6F12, 0x8149,
	0x6F12, 0x854B,
	0x6F12, 0x096D,
	0x6F12, 0x130B,
	0x6F12, 0x0B03,
	0x6F12, 0x370C,
	0x6F12, 0x0040,
	0x6F12, 0x930C,
	0x6F12, 0x0004,
	0x6F12, 0x0347,
	0x6F12, 0x7401,
	0x6F12, 0x8967,
	0x6F12, 0x631B,
	0x6F12, 0x7701,
	0x6F12, 0x93F7,
	0x6F12, 0xF900,
	0x6F12, 0x9808,
	0x6F12, 0xBA97,
	0x6F12, 0x83C7,
	0x6F12, 0x07FB,
	0x6F12, 0x8A07,
	0x6F12, 0xCA97,
	0x6F12, 0x9C43,
	0x6F12, 0x63C4,
	0x6F12, 0x3B07,
	0x6F12, 0x139A,
	0x6F12, 0x1A00,
	0x6F12, 0x9808,
	0x6F12, 0x5297,
	0x6F12, 0x8354,
	0x6F12, 0x07FF,
	0x6F12, 0x0145,
	0x6F12, 0xB384,
	0x6F12, 0xF402,
	0x6F12, 0xB580,
	0x6F12, 0x637A,
	0x6F12, 0x9D00,
	0x6F12, 0x2685,
	0x6F12, 0x9780,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0xA03B,
	0x6F12, 0x4915,
	0x6F12, 0x1375,
	0x6F12, 0xF50F,
	0x6F12, 0x9C08,
	0x6F12, 0xD297,
	0x6F12, 0x03D7,
	0x6F12, 0x07FE,
	0x6F12, 0xB3D4,
	0x6F12, 0xA400,
	0x6F12, 0xC204,
	0x6F12, 0x6297,
	0x6F12, 0xC180,
	0x6F12, 0x2310,
	0x6F12, 0x9700,
	0x6F12, 0x03D7,
	0x6F12, 0x07FC,
	0x6F12, 0x83D7,
	0x6F12, 0x07FD,
	0x6F12, 0x850A,
	0x6F12, 0x6297,
	0x6F12, 0x8356,
	0x6F12, 0x0700,
	0x6F12, 0x3315,
	0x6F12, 0xF500,
	0x6F12, 0x558D,
	0x6F12, 0x4205,
	0x6F12, 0x4181,
	0x6F12, 0x2310,
	0x6F12, 0xA700,
	0x6F12, 0x8509,
	0x6F12, 0xE391,
	0x6F12, 0x99F9,
	0x6F12, 0x51BD,
	0x6F12, 0x1397,
	0x6F12, 0x1900,
	0x6F12, 0x9394,
	0x6F12, 0x0701,
	0x6F12, 0x5A97,
	0x6F12, 0xC180,
	0x6F12, 0x2310,
	0x6F12, 0x9700,
	0x6F12, 0xE5B7,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0xE326,
	0x6F12, 0xB737,
	0x6F12, 0x0024,
	0x6F12, 0x83A7,
	0x6F12, 0x0761,
	0x6F12, 0xAA84,
	0x6F12, 0x2E89,
	0x6F12, 0x8297,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x03A4,
	0x6F12, 0x0700,
	0x6F12, 0x0146,
	0x6F12, 0x1145,
	0x6F12, 0xA285,
	0x6F12, 0x9780,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0xC069,
	0x6F12, 0xCA85,
	0x6F12, 0x2685,
	0x6F12, 0x9730,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x20F5,
	0x6F12, 0x0546,
	0x6F12, 0xA285,
	0x6F12, 0x1145,
	0x6F12, 0x9780,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0x2068,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0xC324,
	0x6F12, 0xB717,
	0x6F12, 0x0024,
	0x6F12, 0x83C7,
	0x6F12, 0x0734,
	0x6F12, 0xEDCF,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0x6321,
	0x6F12, 0x9780,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x803E,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x1387,
	0x6F12, 0x87F5,
	0x6F12, 0x0347,
	0x6F12, 0xF701,
	0x6F12, 0x9387,
	0x6F12, 0x87F5,
	0x6F12, 0x19EB,
	0x6F12, 0x37F7,
	0x6F12, 0x0040,
	0x6F12, 0x8356,
	0x6F12, 0x6772,
	0x6F12, 0x2391,
	0x6F12, 0xD702,
	0x6F12, 0x0357,
	0x6F12, 0xA772,
	0x6F12, 0x2392,
	0x6F12, 0xE702,
	0x6F12, 0xB776,
	0x6F12, 0x0024,
	0x6F12, 0x83C6,
	0x6F12, 0xB6F1,
	0x6F12, 0x0547,
	0x6F12, 0xA38F,
	0x6F12, 0xE700,
	0x6F12, 0x99C6,
	0x6F12, 0x83D6,
	0x6F12, 0x4701,
	0x6F12, 0x238F,
	0x6F12, 0xE700,
	0x6F12, 0x2380,
	0x6F12, 0xD702,
	0x6F12, 0x83C6,
	0x6F12, 0x0702,
	0x6F12, 0x03C7,
	0x6F12, 0xE701,
	0x6F12, 0xB9CE,
	0x6F12, 0x0DC3,
	0x6F12, 0x03D7,
	0x6F12, 0x6701,
	0x6F12, 0x0DCF,
	0x6F12, 0xB7F6,
	0x6F12, 0x0040,
	0x6F12, 0x2393,
	0x6F12, 0xE672,
	0x6F12, 0x03D7,
	0x6F12, 0x8701,
	0x6F12, 0x0DCF,
	0x6F12, 0xB7F6,
	0x6F12, 0x0040,
	0x6F12, 0x2395,
	0x6F12, 0xE672,
	0x6F12, 0x238F,
	0x6F12, 0x0700,
	0x6F12, 0x03C7,
	0x6F12, 0x0702,
	0x6F12, 0x7D17,
	0x6F12, 0x1377,
	0x6F12, 0xF70F,
	0x6F12, 0x2380,
	0x6F12, 0xE702,
	0x6F12, 0x01E7,
	0x6F12, 0x0547,
	0x6F12, 0x238F,
	0x6F12, 0xE700,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0x631A,
	0x6F12, 0x83D6,
	0x6F12, 0x2702,
	0x6F12, 0x37F7,
	0x6F12, 0x0040,
	0x6F12, 0x2313,
	0x6F12, 0xD772,
	0x6F12, 0xD1B7,
	0x6F12, 0x83D6,
	0x6F12, 0x4702,
	0x6F12, 0x37F7,
	0x6F12, 0x0040,
	0x6F12, 0x2315,
	0x6F12, 0xD772,
	0x6F12, 0xD1B7,
	0x6F12, 0x71DF,
	0x6F12, 0x03D7,
	0x6F12, 0xA701,
	0x6F12, 0x19CF,
	0x6F12, 0xB7F6,
	0x6F12, 0x0040,
	0x6F12, 0x2393,
	0x6F12, 0xE672,
	0x6F12, 0x03D7,
	0x6F12, 0xC701,
	0x6F12, 0x19CF,
	0x6F12, 0xB7F6,
	0x6F12, 0x0040,
	0x6F12, 0x2395,
	0x6F12, 0xE672,
	0x6F12, 0x238F,
	0x6F12, 0x0700,
	0x6F12, 0x6DBF,
	0x6F12, 0x83D6,
	0x6F12, 0x2702,
	0x6F12, 0x37F7,
	0x6F12, 0x0040,
	0x6F12, 0x2313,
	0x6F12, 0xD772,
	0x6F12, 0xC5B7,
	0x6F12, 0x83D6,
	0x6F12, 0x4702,
	0x6F12, 0x37F7,
	0x6F12, 0x0040,
	0x6F12, 0x2315,
	0x6F12, 0xD772,
	0x6F12, 0xC5B7,
	0x6F12, 0x8280,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0xC311,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0xA166,
	0x6F12, 0xB775,
	0x6F12, 0x0024,
	0x6F12, 0x9386,
	0x6F12, 0x76F7,
	0x6F12, 0x3777,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0xC701,
	0x6F12, 0x2946,
	0x6F12, 0x9385,
	0x6F12, 0xA57F,
	0x6F12, 0x3545,
	0x6F12, 0x2320,
	0x6F12, 0xF73C,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0xA01F,
	0x6F12, 0xB777,
	0x6F12, 0x0024,
	0x6F12, 0xB785,
	0x6F12, 0x0024,
	0x6F12, 0x3755,
	0x6F12, 0x0020,
	0x6F12, 0x3737,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x4774,
	0x6F12, 0x0146,
	0x6F12, 0x9385,
	0x6F12, 0xA58B,
	0x6F12, 0x1305,
	0x6F12, 0x0537,
	0x6F12, 0x232C,
	0x6F12, 0xF75E,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x4058,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x23A6,
	0x6F12, 0xA700,
	0x6F12, 0xB785,
	0x6F12, 0x0024,
	0x6F12, 0x3765,
	0x6F12, 0x0020,
	0x6F12, 0x0146,
	0x6F12, 0x9385,
	0x6F12, 0xE59F,
	0x6F12, 0x1305,
	0x6F12, 0x45B2,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x2056,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x23A2,
	0x6F12, 0xA700,
	0x6F12, 0xB785,
	0x6F12, 0x0024,
	0x6F12, 0x3755,
	0x6F12, 0x0020,
	0x6F12, 0x0146,
	0x6F12, 0x9385,
	0x6F12, 0x2597,
	0x6F12, 0x1305,
	0x6F12, 0x0525,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x0054,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x23A4,
	0x6F12, 0xA700,
	0x6F12, 0xB775,
	0x6F12, 0x0024,
	0x6F12, 0x3775,
	0x6F12, 0x0020,
	0x6F12, 0x0146,
	0x6F12, 0x9385,
	0x6F12, 0x257B,
	0x6F12, 0x1305,
	0x6F12, 0x85D3,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0xE051,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x23A8,
	0x6F12, 0xA700,
	0x6F12, 0xB785,
	0x6F12, 0x0024,
	0x6F12, 0x37B5,
	0x6F12, 0x0020,
	0x6F12, 0x0146,
	0x6F12, 0x9385,
	0x6F12, 0x85CE,
	0x6F12, 0x1305,
	0x6F12, 0xA5C6,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0xC04F,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x23A0,
	0x6F12, 0xA700,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x3737,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x67D3,
	0x6F12, 0x2320,
	0x6F12, 0xF768,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0x4304,
	0x6F12, 0x0000,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0020,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0001,
	0x6F12, 0x0203,
	0x6F12, 0x0001,
	0x6F12, 0x0203,
	0x6F12, 0x0405,
	0x6F12, 0x0607,
	0x6F12, 0x0405,
	0x6F12, 0x0607,
	0x6F12, 0x10D0,
	0x6F12, 0x10D0,
	0x6F12, 0x1CD0,
	0x6F12, 0x1CD0,
	0x6F12, 0x22D0,
	0x6F12, 0x22D0,
	0x6F12, 0x2ED0,
	0x6F12, 0x2ED0,
	0x6F12, 0x0000,
	0x6F12, 0x0400,
	0x6F12, 0x0800,
	0x6F12, 0x0C00,
	0x6F12, 0x0800,
	0x6F12, 0x0C00,
	0x6F12, 0x0000,
	0x6F12, 0x0400,
	0x6F12, 0x30D0,
	0x6F12, 0x32D0,
	0x6F12, 0x64D0,
	0x6F12, 0x66D0,
	0x6F12, 0x7CD0,
	0x6F12, 0x7ED0,
	0x6F12, 0xA8D0,
	0x6F12, 0xAAD0,
	0x6F12, 0x0001,
	0x6F12, 0x0001,
	0x6F12, 0x0001,
	0x6F12, 0x0001,
	0x6F12, 0x0405,
	0x6F12, 0x0405,
	0x6F12, 0x0405,
	0x6F12, 0x0405,
	0x6F12, 0x10D0,
	0x6F12, 0x10D0,
	0x6F12, 0x12D0,
	0x6F12, 0x12D0,
	0x6F12, 0x20D0,
	0x6F12, 0x20D0,
	0x6F12, 0x22D0,
	0x6F12, 0x22D0,
	0x6F12, 0x0000,
	0x6F12, 0x0400,
	0x6F12, 0x0000,
	0x6F12, 0x0400,
	0x6F12, 0x0000,
	0x6F12, 0x0400,
	0x6F12, 0x0000,
	0x6F12, 0x0400,
	0x6F12, 0x30D0,
	0x6F12, 0x32D0,
	0x6F12, 0x38D0,
	0x6F12, 0x3AD0,
	0x6F12, 0x70D0,
	0x6F12, 0x72D0,
	0x6F12, 0x78D0,
	0x6F12, 0x7AD0,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0xA3F3,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x1307,
	0x6F12, 0xC00E,
	0x6F12, 0x9387,
	0x6F12, 0xC701,
	0x6F12, 0xB785,
	0x6F12, 0x0024,
	0x6F12, 0x3755,
	0x6F12, 0x0020,
	0x6F12, 0xBA97,
	0x6F12, 0x0146,
	0x6F12, 0x3777,
	0x6F12, 0x0024,
	0x6F12, 0x9385,
	0x6F12, 0x4507,
	0x6F12, 0x1305,
	0x6F12, 0xC504,
	0x6F12, 0x2320,
	0x6F12, 0xF73C,
	0x6F12, 0x9710,
	0x6F12, 0x00FC,
	0x6F12, 0xE780,
	0x6F12, 0x603C,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x23A0,
	0x6F12, 0xA710,
	0x6F12, 0x4928,
	0x6F12, 0xE177,
	0x6F12, 0x3747,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x5776,
	0x6F12, 0x2317,
	0x6F12, 0xF782,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0xE3F0,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0xE702,
	0x6F12, 0x83EC,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x83A4,
	0x6F12, 0x0710,
	0x6F12, 0xAA89,
	0x6F12, 0x2E8A,
	0x6F12, 0x0146,
	0x6F12, 0xA685,
	0x6F12, 0x1145,
	0x6F12, 0x9780,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0xA031,
	0x6F12, 0xB787,
	0x6F12, 0x0024,
	0x6F12, 0x03C7,
	0x6F12, 0x4710,
	0x6F12, 0x3E84,
	0x6F12, 0x0149,
	0x6F12, 0x11CF,
	0x6F12, 0x3767,
	0x6F12, 0x0024,
	0x6F12, 0x0357,
	0x6F12, 0x2777,
	0x6F12, 0xB777,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x07C7,
	0x6F12, 0x0E07,
	0x6F12, 0x03D9,
	0x6F12, 0x871C,
	0x6F12, 0x2394,
	0x6F12, 0xE71C,
	0x6F12, 0xD285,
	0x6F12, 0x4E85,
	0x6F12, 0x97D0,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0xA0F8,
	0x6F12, 0x8347,
	0x6F12, 0x4410,
	0x6F12, 0x89C7,
	0x6F12, 0xB777,
	0x6F12, 0x0024,
	0x6F12, 0x239C,
	0x6F12, 0x27E3,
	0x6F12, 0x0546,
	0x6F12, 0xA685,
	0x6F12, 0x1145,
	0x6F12, 0x9780,
	0x6F12, 0xFFFB,
	0x6F12, 0xE780,
	0x6F12, 0xA02C,
	0x6F12, 0x1743,
	0x6F12, 0x01FC,
	0x6F12, 0x6700,
	0x6F12, 0xA3E8,
	0x6F12, 0xE177,
	0x6F12, 0x3747,
	0x6F12, 0x0024,
	0x6F12, 0x9387,
	0x6F12, 0x5776,
	0x6F12, 0x2318,
	0x6F12, 0xF782,
	0x6F12, 0x8280,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x602A, 0x35CC,
	0x6F12, 0x1C80,
	0x6F12, 0x0024,
	/*Global*/
	0x6028, 0x2400,
	0x602A, 0x1354,
	0x6F12, 0x0100,
	0x6F12, 0x7017,
	0x602A, 0x13B2,
	0x6F12, 0x0000,
	0x602A, 0x1236,
	0x6F12, 0x0000,
	0x602A, 0x1A0A,
	0x6F12, 0x4C0A,
	0x602A, 0x2210,
	0x6F12, 0x3401,
	0x602A, 0x2176,
	0x6F12, 0x6400,
	0x602A, 0x222E,
	0x6F12, 0x0001,
	0x602A, 0x06B6,
	0x6F12, 0x0A00,
	0x602A, 0x06BC,
	0x6F12, 0x1001,
	0x602A, 0x2140,
	0x6F12, 0x0101,
	0x602A, 0x218E,
	0x6F12, 0x0000,
	0x602A, 0x1A0E,
	0x6F12, 0x9600,
	0x6028, 0x4000,
	0xF44E, 0x0011,
	0xF44C, 0x0B0B,
	0xF44A, 0x0006,
	0x0118, 0x0002,
	0x011A, 0x0001
};

static void sensor_init(void)
{
	/* initial sequence */

	pr_debug("sensor_init\n");

	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0000, 0x0001);
	write_cmos_sensor(0x0000, 0x38E1);
	write_cmos_sensor(0x001E, 0x0005);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6010, 0x0001);
	mdelay(5);
	write_cmos_sensor(0x6226, 0x0001);
	mdelay(10);

	table_write_cmos_sensor(addr_data_pair_init_jn1sq,
		sizeof(addr_data_pair_init_jn1sq) / sizeof(kal_uint16));
}				/*      sensor_init  */



static kal_uint16 addr_data_pair_pre_jn1sq[] = {
	0x6028, 0x2400,
	0x602A, 0x1A28,
	0x6F12, 0x4C00,
	0x602A, 0x065A,
	0x6F12, 0x0000,
	0x602A, 0x139E,
	0x6F12, 0x0100,
	0x602A, 0x139C,
	0x6F12, 0x0000,
	0x602A, 0x13A0,
	0x6F12, 0x0A00,
	0x6F12, 0x0120,
	0x602A, 0x2072,
	0x6F12, 0x0000,
	0x602A, 0x1A64,
	0x6F12, 0x0301,
	0x6F12, 0xFF00,
	0x602A, 0x19E6,
	0x6F12, 0x0200,
	0x602A, 0x1A30,
	0x6F12, 0x3401,
	0x602A, 0x19FC,
	0x6F12, 0x0B00,
	0x602A, 0x19F4,
	0x6F12, 0x0606,
	0x602A, 0x19F8,
	0x6F12, 0x1010,
	0x602A, 0x1B26,
	0x6F12, 0x6F80,
	0x6F12, 0xA060,
	0x602A, 0x1A3C,
	0x6F12, 0x6207,
	0x602A, 0x1A48,
	0x6F12, 0x6207,
	0x602A, 0x1444,
	0x6F12, 0x2000,
	0x6F12, 0x2000,
	0x602A, 0x144C,
	0x6F12, 0x3F00,
	0x6F12, 0x3F00,
	0x602A, 0x7F6C,
	0x6F12, 0x0100,
	0x6F12, 0x2F00,
	0x6F12, 0xFA00,
	0x6F12, 0x2400,
	0x6F12, 0xE500,
	0x602A, 0x0650,
	0x6F12, 0x0600,
	0x602A, 0x0654,
	0x6F12, 0x0000,
	0x602A, 0x1A46,
	0x6F12, 0xB000,
	0x602A, 0x1A52,
	0x6F12, 0xBF00,
	0x602A, 0x0674,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x0668,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x602A, 0x0684,
	0x6F12, 0x4001,
	0x602A, 0x0688,
	0x6F12, 0x4001,
	0x602A, 0x147C,
	0x6F12, 0x1000,
	0x602A, 0x1480,
	0x6F12, 0x1000,
	0x602A, 0x19F6,
	0x6F12, 0x0904,
	0x602A, 0x0812,
	0x6F12, 0x0010,
	0x602A, 0x2148,
	0x6F12, 0x0100,
	0x602A, 0x2042,
	0x6F12, 0x1A00,
	0x602A, 0x0874,
	0x6F12, 0x0100,
	0x602A, 0x09C0,
	0x6F12, 0x2008,
	0x602A, 0x09C4,
	0x6F12, 0x2000,
	0x602A, 0x19FE,
	0x6F12, 0x0E1C,
	0x602A, 0x4D92,
	0x6F12, 0x0100,
	0x602A, 0x8104,
	0x6F12, 0x0100,
	0x602A, 0x4D94,
	0x6F12, 0x0005,
	0x6F12, 0x000A,
	0x6F12, 0x0010,
	0x6F12, 0x1510,
	0x6F12, 0x000A,
	0x6F12, 0x0040,
	0x6F12, 0x1510,
	0x6F12, 0x1510,
	0x602A, 0x3570,
	0x6F12, 0x0000,
	0x602A, 0x3574,
	0x6F12, 0xD803,
	0x602A, 0x21E4,
	0x6F12, 0x0400,
	0x602A, 0x21EC,
	0x6F12, 0x2A01,
	0x602A, 0x2080,
	0x6F12, 0x0100,
	0x6F12, 0xFF00,
	0x602A, 0x2086,
	0x6F12, 0x0001,
	0x602A, 0x208E,
	0x6F12, 0x14F4,
	0x602A, 0x208A,
	0x6F12, 0xD244,
	0x6F12, 0xD244,
	0x602A, 0x120E,
	0x6F12, 0x1000,
	0x602A, 0x212E,
	0x6F12, 0x0200,
	0x602A, 0x13AE,
	0x6F12, 0x0101,
	0x602A, 0x0718,
	0x6F12, 0x0001,
	0x602A, 0x0710,
	0x6F12, 0x0002,
	0x6F12, 0x0804,
	0x6F12, 0x0100,
	0x602A, 0x1B5C,
	0x6F12, 0x0000,
	0x602A, 0x0786,
	0x6F12, 0x7701,
	0x602A, 0x2022,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x1360,
	0x6F12, 0x0100,
	0x602A, 0x1376,
	0x6F12, 0x0100,
	0x6F12, 0x6038,
	0x6F12, 0x7038,
	0x6F12, 0x8038,
	0x602A, 0x1386,
	0x6F12, 0x0B00,
	0x602A, 0x06FA,
	0x6F12, 0x1000,
	0x602A, 0x4A94,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x602A, 0x0A76,
	0x6F12, 0x1000,
	0x602A, 0x0AEE,
	0x6F12, 0x1000,
	0x602A, 0x0B66,
	0x6F12, 0x1000,
	0x602A, 0x0BDE,
	0x6F12, 0x1000,
	0x602A, 0x0C56,
	0x6F12, 0x1000,
	0x602A, 0x0CF2,
	0x6F12, 0x0001,
	0x602A, 0x0CF0,
	0x6F12, 0x0101,
	0x602A, 0x11B8,
	0x6F12, 0x0100,
	0x602A, 0x11F6,
	0x6F12, 0x0020,
	0x602A, 0x4A74,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0xD8FF,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0xD8FF,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0xF46A, 0xAE80,
	0x0344, 0x0000,
	0x0346, 0x0000,
	0x0348, 0x1FFF,
	0x034A, 0x181F,
	0x034C, 0x0FF0,
	0x034E, 0x0C00,
	0x0350, 0x0008,
	0x0352, 0x0008,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0110, 0x1002,
	0x0114, 0x0301,
	0x0116, 0x3000,
	0x0136, 0x1800,
	0x013E, 0x0000,
	0x0300, 0x0006,
	0x0302, 0x0001,
	0x0304, 0x0004,
	0x0306, 0x008C,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x008A,
	0x0312, 0x0000,
	0x0340, 0x0FE0,
	0x0342, 0x11E8,
	0x0702, 0x0000,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x0D00, 0x0100,
	0x0D02, 0x0001,
	0x0D04, 0x0102,
	0x6226, 0x0000
};


static void preview_setting(void)
{
	/* Convert from : "MOT_TONGA_S5KJN1SQ_EVT0_ReferenceSetfile_v0.1b_2020510 -- (Set)Shine"*/

	/*ExtClk :	24	MHz
	  Vt_pix_clk :	70 	MHz 70*8 = 560Mhz
	  MIPI_output_speed :	1656.0 	Mbps/lane
	  Crop_Width :	8192	px
	  Crop_Height :	6176	px
	  Output_Width :	4080	px
	  Output_Height :	3072	px
	  Frame rate :	30.06	fps
	  Output format :	Raw10
	  H-size :	4584 	px
	  H-blank :	504	px
	  V-size :	4064	line
	  V-blank :	992	line
	  Tail X :	508
	  Tail Y :	3056
	  Lane :	4	lane
	  First Pixel :	Gr	First*/

	pr_debug("preview_setting\n");

	table_write_cmos_sensor(addr_data_pair_pre_jn1sq,
			sizeof(addr_data_pair_pre_jn1sq) / sizeof(kal_uint16));

}				/*      preview_setting  */

static kal_uint16 addr_data_pair_video_jn1sq[] = {
	0x6028, 0x2400,
	0x602A, 0x1A28,
	0x6F12, 0x4C00,
	0x602A, 0x065A,
	0x6F12, 0x0000,
	0x602A, 0x139E,
	0x6F12, 0x0100,
	0x602A, 0x139C,
	0x6F12, 0x0000,
	0x602A, 0x13A0,
	0x6F12, 0x0A00,
	0x6F12, 0x0120,
	0x602A, 0x2072,
	0x6F12, 0x0000,
	0x602A, 0x1A64,
	0x6F12, 0x0301,
	0x6F12, 0xFF00,
	0x602A, 0x19E6,
	0x6F12, 0x0200,
	0x602A, 0x1A30,
	0x6F12, 0x3401,
	0x602A, 0x19FC,
	0x6F12, 0x0B00,
	0x602A, 0x19F4,
	0x6F12, 0x0606,
	0x602A, 0x19F8,
	0x6F12, 0x1010,
	0x602A, 0x1B26,
	0x6F12, 0x6F80,
	0x6F12, 0xA060,
	0x602A, 0x1A3C,
	0x6F12, 0x6207,
	0x602A, 0x1A48,
	0x6F12, 0x6207,
	0x602A, 0x1444,
	0x6F12, 0x2000,
	0x6F12, 0x2000,
	0x602A, 0x144C,
	0x6F12, 0x3F00,
	0x6F12, 0x3F00,
	0x602A, 0x7F6C,
	0x6F12, 0x0100,
	0x6F12, 0x2F00,
	0x6F12, 0xFA00,
	0x6F12, 0x2400,
	0x6F12, 0xE500,
	0x602A, 0x0650,
	0x6F12, 0x0600,
	0x602A, 0x0654,
	0x6F12, 0x0000,
	0x602A, 0x1A46,
	0x6F12, 0xB000,
	0x602A, 0x1A52,
	0x6F12, 0xBF00,
	0x602A, 0x0674,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x0668,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x6F12, 0x0800,
	0x602A, 0x0684,
	0x6F12, 0x4001,
	0x602A, 0x0688,
	0x6F12, 0x4001,
	0x602A, 0x147C,
	0x6F12, 0x1000,
	0x602A, 0x1480,
	0x6F12, 0x1000,
	0x602A, 0x19F6,
	0x6F12, 0x0904,
	0x602A, 0x0812,
	0x6F12, 0x0010,
	0x602A, 0x2148,
	0x6F12, 0x0100,
	0x602A, 0x2042,
	0x6F12, 0x1A00,
	0x602A, 0x0874,
	0x6F12, 0x0100,
	0x602A, 0x09C0,
	0x6F12, 0x2008,
	0x602A, 0x09C4,
	0x6F12, 0x2000,
	0x602A, 0x19FE,
	0x6F12, 0x0E1C,
	0x602A, 0x4D92,
	0x6F12, 0x0100,
	0x602A, 0x8104,
	0x6F12, 0x0100,
	0x602A, 0x4D94,
	0x6F12, 0x0005,
	0x6F12, 0x000A,
	0x6F12, 0x0010,
	0x6F12, 0x1510,
	0x6F12, 0x000A,
	0x6F12, 0x0040,
	0x6F12, 0x1510,
	0x6F12, 0x1510,
	0x602A, 0x3570,
	0x6F12, 0x0000,
	0x602A, 0x3574,
	0x6F12, 0x1304,
	0x602A, 0x21E4,
	0x6F12, 0x0400,
	0x602A, 0x21EC,
	0x6F12, 0x1D02,
	0x602A, 0x2080,
	0x6F12, 0x0100,
	0x6F12, 0xFF00,
	0x602A, 0x2086,
	0x6F12, 0x0001,
	0x602A, 0x208E,
	0x6F12, 0x14F4,
	0x602A, 0x208A,
	0x6F12, 0xD244,
	0x6F12, 0xD244,
	0x602A, 0x120E,
	0x6F12, 0x1000,
	0x602A, 0x212E,
	0x6F12, 0x0200,
	0x602A, 0x13AE,
	0x6F12, 0x0101,
	0x602A, 0x0718,
	0x6F12, 0x0001,
	0x602A, 0x0710,
	0x6F12, 0x0002,
	0x6F12, 0x0804,
	0x6F12, 0x0100,
	0x602A, 0x1B5C,
	0x6F12, 0x0000,
	0x602A, 0x0786,
	0x6F12, 0x7701,
	0x602A, 0x2022,
	0x6F12, 0x0500,
	0x6F12, 0x0500,
	0x602A, 0x1360,
	0x6F12, 0x0100,
	0x602A, 0x1376,
	0x6F12, 0x0100,
	0x6F12, 0x6038,
	0x6F12, 0x7038,
	0x6F12, 0x8038,
	0x602A, 0x1386,
	0x6F12, 0x0B00,
	0x602A, 0x06FA,
	0x6F12, 0x1000,
	0x602A, 0x4A94,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x6F12, 0x0600,
	0x602A, 0x0A76,
	0x6F12, 0x1000,
	0x602A, 0x0AEE,
	0x6F12, 0x1000,
	0x602A, 0x0B66,
	0x6F12, 0x1000,
	0x602A, 0x0BDE,
	0x6F12, 0x1000,
	0x602A, 0x0C56,
	0x6F12, 0x1000,
	0x602A, 0x0CF2,
	0x6F12, 0x0001,
	0x602A, 0x0CF0,
	0x6F12, 0x0101,
	0x602A, 0x11B8,
	0x6F12, 0x0100,
	0x602A, 0x11F6,
	0x6F12, 0x0020,
	0x602A, 0x4A74,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0xD8FF,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0xD8FF,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6F12, 0x0000,
	0x6028, 0x4000,
	0xF46A, 0xAE80,
	0x0344, 0x0000,
	0x0346, 0x0000,
	0x0348, 0x1FFF,
	0x034A, 0x181F,
	0x034C, 0x0FF0,
	0x034E, 0x0C00,
	0x0350, 0x0008,
	0x0352, 0x0008,
	0x0900, 0x0122,
	0x0380, 0x0002,
	0x0382, 0x0002,
	0x0384, 0x0002,
	0x0386, 0x0002,
	0x0110, 0x1002,
	0x0114, 0x0301,
	0x0116, 0x3000,
	0x0136, 0x1800,
	0x013E, 0x0000,
	0x0300, 0x0006,
	0x0302, 0x0001,
	0x0304, 0x0004,
	0x0306, 0x008C,
	0x0308, 0x0008,
	0x030A, 0x0001,
	0x030C, 0x0000,
	0x030E, 0x0004,
	0x0310, 0x0064,
	0x0312, 0x0000,
	0x080E, 0x0000,
	0x0340, 0x0C54,
	0x0342, 0x1716,
	0x0702, 0x0000,
	0x0202, 0x0100,
	0x0200, 0x0100,
	0x0D00, 0x0100,
	0x0D02, 0x0001,
	0x0D04, 0x0102,
	0x6226, 0x0000
};

static void normal_video_setting(kal_uint16 currefps)
{
	pr_debug("normal_video_setting\n");

	table_write_cmos_sensor(addr_data_pair_video_jn1sq,
		sizeof(addr_data_pair_video_jn1sq) / sizeof(kal_uint16));
}

#if EEPROM_READY //stan
#define FOUR_CELL_SIZE 3072/*size = 3072 = 0xc00*/
static int Is_Read_4Cell;
static char Four_Cell_Array[FOUR_CELL_SIZE + 2];
static void read_4cell_from_eeprom(char *data)
{
	int ret;
	int addr = 0x763;/*Start of 4 cell data*/
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };
	char temp;

	if (Is_Read_4Cell != 1) {
		pr_debug("Need to read i2C\n");

		pu_send_cmd[0] = (char)(addr >> 8);
		pu_send_cmd[1] = (char)(addr & 0xFF);

		/* Check I2C is normal */
		ret = iReadRegI2C(pu_send_cmd, 2, &temp, 1, EEPROM_READ_ID);
		if (ret != 0) {
			pr_debug("iReadRegI2C error\n");
			return;
		}

		Four_Cell_Array[0] = (FOUR_CELL_SIZE & 0xff);/*Low*/
		Four_Cell_Array[1] = ((FOUR_CELL_SIZE >> 8) & 0xff);/*High*/

		/*Multi-Read*/
		iReadRegI2C(pu_send_cmd, 2, &Four_Cell_Array[2],
					FOUR_CELL_SIZE, EEPROM_READ_ID);
		Is_Read_4Cell = 1;
	}

	if (data != NULL) {
		pr_debug("return data\n");
		memcpy(data, Four_Cell_Array, FOUR_CELL_SIZE);
	}
}
#endif

#define TONGA_S5KJN1_EEPROM_SLAVE_ADDR 0xA0
#define TONGA_S5KJN1_SENSOR_IIC_SLAVE_ADDR 0xAC
#define TONGA_S5KJN1_EEPROM_SIZE  0x191d
#define EEPROM_DATA_PATH "/data/vendor/camera_dump/s5kjn1_eeprom_data.bin"
#define TONGA_S5KJN1_EEPROM_CRC_AF_CAL_SIZE 24
#define TONGA_S5KJN1_EEPROM_CRC_AWB_CAL_SIZE 43
#define TONGA_S5KJN1_EEPROM_CRC_LSC_SIZE 1868
#define TONGA_S5KJN1_EEPROM_CRC_PDAF_OUTPUT1_SIZE 496
#define TONGA_S5KJN1_EEPROM_CRC_PDAF_OUTPUT2_SIZE 1004
#define TONGA_S5KJN1_EEPROM_CRC_MANUFACTURING_SIZE 37
#define TONGA_S5KJN1_MANUFACTURE_PART_NUMBER "28D14866"
#define TONGA_S5KJN1_MPN_LENGTH 8

static uint8_t TONGA_S5KJN1_eeprom[TONGA_S5KJN1_EEPROM_SIZE] = {0};
static calibration_status_t mnf_status = CRC_FAILURE;
static calibration_status_t af_status = CRC_FAILURE;
static calibration_status_t awb_status = CRC_FAILURE;
static calibration_status_t lsc_status = CRC_FAILURE;
static calibration_status_t pdaf_status = CRC_FAILURE;
static calibration_status_t dual_status = CRC_FAILURE;

static uint8_t crc_reverse_byte(uint32_t data)
{
	return ((data * 0x0802LU & 0x22110LU) |
		(data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

static uint32_t convert_crc(uint8_t *crc_ptr)
{
	return (crc_ptr[0] << 8) | (crc_ptr[1]);
}

static uint16_t to_uint16_swap(uint8_t *data)
{
	uint16_t converted;
	memcpy(&converted, data, sizeof(uint16_t));
	return ntohs(converted);
}

static void TONGA_S5KJN1_read_data_from_eeprom(kal_uint8 slave, kal_uint32 start_add, uint32_t size)
{
	int i = 0;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = slave;
	spin_unlock(&imgsensor_drv_lock);

	//read eeprom data
	for (i = 0; i < size; i ++) {
		TONGA_S5KJN1_eeprom[i] = read_cmos_sensor_8(start_add);
		start_add ++;
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = TONGA_S5KJN1_SENSOR_IIC_SLAVE_ADDR;
	spin_unlock(&imgsensor_drv_lock);
}

static void TONGA_S5KJN1_eeprom_dump_bin(const char *file_name, uint32_t size, const void *data)
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
	uint32_t r_g_golden_min;
	uint32_t r_g_golden_max;
	uint32_t b_g_golden_min;
	uint32_t b_g_golden_max;

	r_g = unit.r_g * 1000;
	b_g = unit.b_g*1000;

	golden_rg = golden.r_g* 1000;
	golden_bg = golden.b_g* 1000;

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
	return 0;
}

static calibration_status_t TONGA_S5KJN1_check_awb_data(void *data)
{
	struct TONGA_S5KJN1_eeprom_t *eeprom = (struct TONGA_S5KJN1_eeprom_t*)data;
	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;

	if(!eeprom_util_check_crc16(eeprom->cie_src_1_ev,
		TONGA_S5KJN1_EEPROM_CRC_AWB_CAL_SIZE,
		convert_crc(eeprom->awb_crc16))) {
		LOG_INF("AWB CRC Fails!");
		return CRC_FAILURE;
	}

	unit.r = to_uint16_swap(eeprom->awb_src_1_r)/64;
	unit.gr = to_uint16_swap(eeprom->awb_src_1_gr)/64;
	unit.gb = to_uint16_swap(eeprom->awb_src_1_gb)/64;
	unit.b = to_uint16_swap(eeprom->awb_src_1_b)/64;
	unit.r_g = to_uint16_swap(eeprom->awb_src_1_rg_ratio);
	unit.b_g = to_uint16_swap(eeprom->awb_src_1_bg_ratio);
	unit.gr_gb = to_uint16_swap(eeprom->awb_src_1_gr_gb_ratio);

	golden.r = to_uint16_swap(eeprom->awb_src_1_golden_r)/64;
	golden.gr = to_uint16_swap(eeprom->awb_src_1_golden_gr)/64;
	golden.gb = to_uint16_swap(eeprom->awb_src_1_golden_gb)/64;
	golden.b = to_uint16_swap(eeprom->awb_src_1_golden_b)/64;
	golden.r_g = to_uint16_swap(eeprom->awb_src_1_golden_rg_ratio);
	golden.b_g = to_uint16_swap(eeprom->awb_src_1_golden_bg_ratio);
	golden.gr_gb = to_uint16_swap(eeprom->awb_src_1_golden_gr_gb_ratio);
	if (mot_eeprom_util_check_awb_limits(unit, golden)) {
		LOG_INF("AWB CRC limit Fails!");
		return LIMIT_FAILURE;
	}

	golden_limit.r_g_golden_min = eeprom->awb_r_g_golden_min_limit[0];
	golden_limit.r_g_golden_max = eeprom->awb_r_g_golden_max_limit[0];
	golden_limit.b_g_golden_min = eeprom->awb_b_g_golden_min_limit[0];
	golden_limit.b_g_golden_max = eeprom->awb_b_g_golden_max_limit[0];

	if (mot_eeprom_util_calculate_awb_factors_limit(unit, golden,golden_limit)) {
		LOG_INF("AWB CRC factor limit Fails!");
		return LIMIT_FAILURE;
	}
	LOG_INF("AWB CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t TONGA_S5KJN1_check_lsc_data_mtk(void *data)
{
	struct TONGA_S5KJN1_eeprom_t *eeprom = (struct TONGA_S5KJN1_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->lsc_data_mtk, TONGA_S5KJN1_EEPROM_CRC_LSC_SIZE,
		convert_crc(eeprom->lsc_crc16_mtk))) {
		LOG_INF("LSC CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("LSC CRC Pass");
	return NO_ERRORS;
}

static void TONGA_S5KJN1_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;
	struct TONGA_S5KJN1_eeprom_t *eeprom = (struct TONGA_S5KJN1_eeprom_t*)data;

	ret = snprintf(mnf->table_revision, MAX_CALIBRATION_STRING, "0x%x",
		eeprom->eeprom_table_version[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->table_revision failed");
		mnf->table_revision[0] = 0;
	}

	ret = snprintf(mnf->mot_part_number, MAX_CALIBRATION_STRING, "%c%c%c%c%c%c%c%c",
		eeprom->mpn[0], eeprom->mpn[1], eeprom->mpn[2], eeprom->mpn[3],
		eeprom->mpn[4], eeprom->mpn[5], eeprom->mpn[6], eeprom->mpn[7]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->mot_part_number failed");
		mnf->mot_part_number[0] = 0;
	}

	ret = snprintf(mnf->actuator_id, MAX_CALIBRATION_STRING, "0x%x", eeprom->actuator_id[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->actuator_id failed");
		mnf->actuator_id[0] = 0;
	}

	ret = snprintf(mnf->lens_id, MAX_CALIBRATION_STRING, "0x%x", eeprom->lens_id[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->lens_id failed");
		mnf->lens_id[0] = 0;
	}

	if (eeprom->manufacturer_id[0] == 'S' && eeprom->manufacturer_id[1] == 'U') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Sunny");
	} else if (eeprom->manufacturer_id[0] == 'O' && eeprom->manufacturer_id[1] == 'F') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "OFilm");
	} else if (eeprom->manufacturer_id[0] == 'Q' && eeprom->manufacturer_id[1] == 'T') {
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
		eeprom->factory_id[0], eeprom->factory_id[1]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->factory_id failed");
		mnf->factory_id[0] = 0;
	}

	ret = snprintf(mnf->manufacture_line, MAX_CALIBRATION_STRING, "%u",
		eeprom->manufacture_line[0]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->manufacture_line failed");
		mnf->manufacture_line[0] = 0;
	}

	ret = snprintf(mnf->manufacture_date, MAX_CALIBRATION_STRING, "20%u/%u/%u",
		eeprom->manufacture_date[0], eeprom->manufacture_date[1], eeprom->manufacture_date[2]);

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->manufacture_date failed");
		mnf->manufacture_date[0] = 0;
	}

	ret = snprintf(mnf->serial_number, MAX_CALIBRATION_STRING, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		eeprom->serial_number[0], eeprom->serial_number[1],
		eeprom->serial_number[2], eeprom->serial_number[3],
		eeprom->serial_number[4], eeprom->serial_number[5],
		eeprom->serial_number[6], eeprom->serial_number[7],
		eeprom->serial_number[8], eeprom->serial_number[9],
		eeprom->serial_number[10], eeprom->serial_number[11],
		eeprom->serial_number[12], eeprom->serial_number[13],
		eeprom->serial_number[14], eeprom->serial_number[15]);
	TONGA_S5KJN1_eeprom_dump_bin(MAIN_SERIAL_NUM_DATA_PATH,  S5KJN1_SERIAL_NUM_SIZE,  eeprom->serial_number);
	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->serial_number failed");
		mnf->serial_number[0] = 0;
	}
}

static calibration_status_t TONGA_S5KJN1_check_af_data(void *data)
{
	struct TONGA_S5KJN1_eeprom_t *eeprom = (struct TONGA_S5KJN1_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->af_data, TONGA_S5KJN1_EEPROM_CRC_AF_CAL_SIZE,
		convert_crc(eeprom->af_crc16))) {
		LOG_INF("Autofocus CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Autofocus CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t TONGA_S5KJN1_check_pdaf_data(void *data)
{
	struct TONGA_S5KJN1_eeprom_t *eeprom = (struct TONGA_S5KJN1_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->pdaf_out1_data_mtk, TONGA_S5KJN1_EEPROM_CRC_PDAF_OUTPUT1_SIZE,
		convert_crc(eeprom->pdaf_out1_crc16_mtk))) {
		LOG_INF("PDAF OUTPUT1 CRC Fails!");
		return CRC_FAILURE;
	}

	if (!eeprom_util_check_crc16(eeprom->pdaf_out2_data_mtk, TONGA_S5KJN1_EEPROM_CRC_PDAF_OUTPUT2_SIZE,
		convert_crc(eeprom->pdaf_out2_crc16_mtk))) {
		LOG_INF("PDAF OUTPUT2 CRC Fails!");
		return CRC_FAILURE;
	}

	LOG_INF("PDAF CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t TONGA_S5KJN1_check_manufacturing_data(void *data)
{
	struct TONGA_S5KJN1_eeprom_t *eeprom = (struct TONGA_S5KJN1_eeprom_t*)data;
LOG_INF("Manufacturing eeprom->mpn = %s !",eeprom->mpn);
#if 0
	if(strncmp(eeprom->mpn, TONGA_S5KJN1_MANUFACTURE_PART_NUMBER, TONGA_S5KJN1_MPN_LENGTH) != 0) {
		LOG_INF("Manufacturing part number (%s) check Fails!", eeprom->mpn);
		return CRC_FAILURE;
	}
#endif
	if (!eeprom_util_check_crc16(data, TONGA_S5KJN1_EEPROM_CRC_MANUFACTURING_SIZE,
		convert_crc(eeprom->manufacture_crc16))) {
		LOG_INF("Manufacturing CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Manufacturing CRC Pass");
	return NO_ERRORS;
}

static void TONGA_S5KJN1_eeprom_format_calibration_data(void *data)
{
	if (NULL == data) {
		LOG_INF("data is NULL");
		return;
	}

	mnf_status = TONGA_S5KJN1_check_manufacturing_data(data);
	af_status = TONGA_S5KJN1_check_af_data(data);
	awb_status = TONGA_S5KJN1_check_awb_data(data);;
	lsc_status = TONGA_S5KJN1_check_lsc_data_mtk(data);;
	pdaf_status = TONGA_S5KJN1_check_pdaf_data(data);
	dual_status = 0;

	LOG_INF("status mnf:%d, af:%d, awb:%d, lsc:%d, pdaf:%d, dual:%d",
		mnf_status, af_status, awb_status, lsc_status, pdaf_status, dual_status);
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
//	kal_uint16 sp8spFlag = 0;

	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 *we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = (
		(read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));

			if (*sensor_id == imgsensor_info.sensor_id) {
				TONGA_S5KJN1_read_data_from_eeprom(TONGA_S5KJN1_EEPROM_SLAVE_ADDR, 0x0000, TONGA_S5KJN1_EEPROM_SIZE);
				TONGA_S5KJN1_eeprom_dump_bin(EEPROM_DATA_PATH, TONGA_S5KJN1_EEPROM_SIZE, (void *)TONGA_S5KJN1_eeprom);
				TONGA_S5KJN1_eeprom_format_calibration_data((void *)TONGA_S5KJN1_eeprom);
				pr_debug("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
				/* preload 4cell data */
#if EEPROM_READY //stan
				read_4cell_from_eeprom(NULL);
#endif
				return ERROR_NONE;

		/* 4Cell version check, s5kjn1sq And s5kjn1sq's checking is differet
		 *	sp8spFlag = (((read_cmos_sensor(0x000C) & 0xFF) << 8)
		 *		|((read_cmos_sensor(0x000E) >> 8) & 0xFF));
		 *	pr_debug(
		 *	"sp8Flag(0x%x),0x5003 used by mot_tonga_s5kjn1sq\n", sp8spFlag);
		 *
		 *	if (sp8spFlag == 0x5003) {
		 *		pr_debug("it is mot_tonga_s5kjn1sq\n");
		 *		return ERROR_NONE;
		 *	}
		 *
		 *		pr_debug(
		 *	"s5kjn1sq type is 0x(%x),0x000C(0x%x),0x000E(0x%x)\n",
		 *		sp8spFlag,
		 *		read_cmos_sensor(0x000C),
		 *		read_cmos_sensor(0x000E));
		 *
		 *		*sensor_id = 0xFFFFFFFF;
		 *	return ERROR_SENSOR_CONNECT_FAIL;
		 */
			}
			pr_debug("Read sensor id fail, id: 0x%x\n",
				imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
	/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF */
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

	pr_debug("%s", __func__);

	/* sensor have two i2c address 0x6c 0x6d & 0x21 0x20,
	 * we should detect the module used i2c address
	 */
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = (
		(read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));

			if (sensor_id == imgsensor_info.sensor_id) {
				pr_debug("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, sensor_id);
				break;
			}

			pr_debug("Read sensor id fail, id: 0x%x\n",
				imgsensor.i2c_write_id);
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
	imgsensor.ihdr_mode = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}				/*      open  */



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
	pr_debug("E\n");

	/*No Need to implement this function */

	return ERROR_NONE;
}				/*      close  */


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
}				/*      preview   */

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
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
	/* PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M */
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
			pr_debug("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
				imgsensor.current_fps,
				imgsensor_info.cap.max_framerate / 10);
		}

		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	//capture_setting(imgsensor.current_fps);
	preview_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}				/* capture() */

static kal_uint32 normal_video(
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

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
}				/*      normal_video   */

static kal_uint32 get_resolution(
	MSDK_SENSOR_RESOLUTION_INFO_STRUCT(*sensor_resolution))
{
	pr_debug("get resolution E\n");
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

	return ERROR_NONE;
}				/*      get_resolution  */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	/*pr_debug("get_info -> scenario_id = %d\n", scenario_id);*/

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* not use */
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;

	/* inverse with datasheet */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4;	/* not use */
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
	sensor_info->SensorMasterClockSwitch = 0;	/* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	/* The frame of setting shutter default 0 for TG int */
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;

	/* The frame of setting sensor gain*/
	sensor_info->AESensorGainDelayFrame =
				imgsensor_info.ae_sensor_gain_delay_frame;

	sensor_info->AEISPGainDelayFrame =
				imgsensor_info.ae_ispGain_delay_frame;

	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;

	/* change pdaf support mode to pdaf VC mode */
	sensor_info->PDAF_Support = 0;
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
	sensor_info->calibration_status.mnf = mnf_status;
	sensor_info->calibration_status.af = af_status;
	sensor_info->calibration_status.awb = awb_status;
	sensor_info->calibration_status.lsc = lsc_status;
	sensor_info->calibration_status.pdaf = pdaf_status;
	sensor_info->calibration_status.dual = dual_status;
	TONGA_S5KJN1_eeprom_get_mnf_data((void *) TONGA_S5KJN1_eeprom, &sensor_info->mnf_calibration);
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
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}				/*      get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			  MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
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
	default:
		pr_debug("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
		return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}				/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	/* //pr_debug("framerate = %d\n ", framerate); */
	/* SetVideoMode Function should fix framerate */
	if (framerate == 0)
		/* Dynamic frame rate */
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

static kal_uint32 set_auto_flicker_mode(
	kal_bool enable, UINT16 framerate)
{
	pr_debug("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)		/* enable auto flicker */
		imgsensor.autoflicker_en = KAL_TRUE;
	else			/* Cancel Auto flick */
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id,	MUINT32 framerate)
{
	kal_uint32 frame_length;

	pr_debug("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk
			/ framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);

		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
			? (frame_length - imgsensor_info.pre.framelength) : 0;

		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		else {
			/*No need to set*/
			pr_debug("frame_length %d < shutter %d",
				imgsensor.frame_length, imgsensor.shutter);
		}
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk
		    / framerate * 10 / imgsensor_info.normal_video.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
	    (frame_length > imgsensor_info.normal_video.framelength)
	  ? (frame_length - imgsensor_info.normal_video.  framelength) : 0;

		imgsensor.frame_length =
		 imgsensor_info.normal_video.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		else {
			/*No need to set*/
			pr_debug("frame_length %d < shutter %d",
				imgsensor.frame_length, imgsensor.shutter);
		}
		break;

	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {

		frame_length = imgsensor_info.cap1.pclk
			/ framerate * 10 / imgsensor_info.cap1.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		      (frame_length > imgsensor_info.cap1.framelength)
		    ? (frame_length - imgsensor_info.cap1.  framelength) : 0;

		imgsensor.frame_length =
			imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
	} else if (imgsensor.current_fps == imgsensor_info.cap2.max_framerate) {
		frame_length = imgsensor_info.cap2.pclk
			/ framerate * 10 / imgsensor_info.cap2.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		      (frame_length > imgsensor_info.cap2.framelength)
		    ? (frame_length - imgsensor_info.cap2.  framelength) : 0;

		imgsensor.frame_length =
			imgsensor_info.cap2.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			pr_debug("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",
				framerate,
				imgsensor_info.cap.max_framerate / 10);

		frame_length = imgsensor_info.cap.pclk
			/ framerate * 10 / imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap.framelength)
			? (frame_length - imgsensor_info.cap.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.cap.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
	}
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		else {
			/*No need to set*/
			pr_debug("frame_length %d < shutter %d",
				imgsensor.frame_length, imgsensor.shutter);
		}
		break;

	default:		/* coding with  preview scenario by default */
		frame_length = imgsensor_info.pre.pclk
			/ framerate * 10 / imgsensor_info.pre.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
			? (frame_length - imgsensor_info.pre.framelength) : 0;

		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;

		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		else {
			/*No need to set*/
			pr_debug("frame_length %d < shutter %d",
				imgsensor.frame_length, imgsensor.shutter);
		}
		pr_debug("error scenario_id = %d, we use preview scenario\n",
		scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(
	enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	/*pr_debug("scenario_id = %d\n", scenario_id);*/

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
	default:
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	pr_debug("enable: %d\n", enable);

	if (enable) {
/* 0 : Normal, 1 : Solid Color, 2 : Color Bar, 3 : Shade Color Bar, 4 : PN9 */
		write_cmos_sensor(0x0600, 0x0002);
	} else {
		write_cmos_sensor(0x0600, 0x0000);
	}
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
	//INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;

	/* SET_PD_BLOCK_INFO_T *PDAFinfo; */
	/* SENSOR_VC_INFO_STRUCT *pvcinfo; */
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	/*pr_debug("feature_id = %d\n", feature_id);*/
	switch (feature_id) {
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
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
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
	/* night_mode((BOOL) *feature_data); no need to implement this mode */
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;

	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor_8(sensor_reg_data->RegAddr,
			sensor_reg_data->RegData);
		break;

	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor_8(sensor_reg_data->RegAddr);
		break;

	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM or
		 * just return LENS_DRIVER_ID_DO_NOT_CARE
		 */
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
		set_auto_flicker_mode((BOOL) (*feature_data_16),
					*(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(
	    (enum MSDK_SCENARIO_ID_ENUM) *feature_data, *(feature_data + 1));
		break;

	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM) *(feature_data),
			  (MUINT32 *) (uintptr_t) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL) (*feature_data));
		break;

	/* for factory mode auto testing */
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		pr_debug("current fps :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = (UINT16)*feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		pr_debug("ihdr enable :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_mode = (UINT8)*feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;

	case SENSOR_FEATURE_GET_CROP_INFO:
		/* pr_debug("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
		 *	(UINT32) *feature_data);
		 */

		wininfo =
	(struct SENSOR_WINSIZE_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

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
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
			       sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		pr_debug("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16) *feature_data,
			(UINT16) *(feature_data + 1),
			(UINT16) *(feature_data + 2));

/* ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),
 * (UINT16)*(feature_data+2));
 */
		break;
	case SENSOR_FEATURE_SET_AWB_GAIN:
		break;
	case SENSOR_FEATURE_SET_HDR_SHUTTER:
		pr_debug("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",
			(UINT16) *feature_data,
			(UINT16) *(feature_data + 1));
/* ihdr_write_shutter((UINT16)*feature_data,(UINT16)*(feature_data+1)); */
		break;

#if EEPROM_READY //stan
	case SENSOR_FEATURE_GET_4CELL_DATA:/*get 4 cell data from eeprom*/
	{
		int type = (kal_uint16)(*feature_data);
		char *data = (char *)(uintptr_t)(*(feature_data+1));

		if (type == FOUR_CELL_CAL_TYPE_XTALK_CAL) {
			pr_debug("Read Cross Talk Start");
			read_4cell_from_eeprom(data);
			pr_debug("Read Cross Talk = %02x %02x %02x %02x %02x %02x\n",
				(UINT16)data[0], (UINT16)data[1],
				(UINT16)data[2], (UINT16)data[3],
				(UINT16)data[4], (UINT16)data[5]);
		}
		break;
	}
#endif


		/******************** PDAF START >>> *********/
		/*
		 * case SENSOR_FEATURE_GET_PDAF_INFO:
		 * pr_debug("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n",
		 * (UINT16)*feature_data);
		 * PDAFinfo =
		 * (SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
		 * switch (*feature_data) {
		 * case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG: //full
		 * case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		 * case MSDK_SCENARIO_ID_CAMERA_PREVIEW: //2x2 binning
		 * memcpy((void *)PDAFinfo,
		 * (void *)&imgsensor_pd_info,
		 * sizeof(SET_PD_BLOCK_INFO_T)); //need to check
		 * break;
		 * case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		 * case MSDK_SCENARIO_ID_SLIM_VIDEO:
		 * default:
		 * break;
		 * }
		 * break;
		 * case SENSOR_FEATURE_GET_VC_INFO:
		 * pr_debug("SENSOR_FEATURE_GET_VC_INFO %d\n",
		 * (UINT16)*feature_data);
		 * pvcinfo =
		 * (SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		 * switch (*feature_data_32) {
		 * case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		 * memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[2],
		 * sizeof(SENSOR_VC_INFO_STRUCT));
		 * break;
		 * case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		 * memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[1],
		 * sizeof(SENSOR_VC_INFO_STRUCT));
		 * break;
		 * case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		 * default:
		 * memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[0],
		 * sizeof(SENSOR_VC_INFO_STRUCT));
		 * break;
		 * }
		 * break;
		 * case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		 * pr_debug(
		 * "SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n",
		 * (UINT16)*feature_data);
		 * //PDAF capacity enable or not
		 * switch (*feature_data) {
		 * case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		 * (MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
		 * break;
		 * case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		 * *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
		 * // video & capture use same setting
		 * break;
		 * case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		 * *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
		 * break;
		 * case MSDK_SCENARIO_ID_SLIM_VIDEO:
		 * //need to check
		 * *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
		 * break;
		 * case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		 * *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
		 * break;
		 * default:
		 * *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
		 * break;
		 * }
		 * break;
		 * case SENSOR_FEATURE_GET_PDAF_DATA: //get cal data from eeprom
		 * pr_debug("SENSOR_FEATURE_GET_PDAF_DATA\n");
		 * read_S5KJN1SQ_eeprom((kal_uint16 )(*feature_data),
		 * (char*)(uintptr_t)(*(feature_data+1)),
		 * (kal_uint32)(*(feature_data+2)));
		 * pr_debug("SENSOR_FEATURE_GET_PDAF_DATA success\n");
		 * break;
		 * case SENSOR_FEATURE_SET_PDAF:
		 * pr_debug("PDAF mode :%d\n", *feature_data_16);
		 * imgsensor.pdaf_mode= *feature_data_16;
		 * break;
		 */
		/******************** PDAF END   <<< *********/
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		pr_debug("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		pr_debug("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_PIXEL_RATE:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.cap.pclk /
			(imgsensor_info.cap.linelength - 80))*
			imgsensor_info.cap.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.normal_video.pclk /
			(imgsensor_info.normal_video.linelength - 80))*
			imgsensor_info.normal_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.pre.pclk /
			(imgsensor_info.pre.linelength - 80))*
			imgsensor_info.pre.grabwindow_width;
			break;
		}
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
}				/*      feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 MOT_TONGA_S5KJN1SQ_MIPI_RAW_SensorInit(
	struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}
