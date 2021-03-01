/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 OV16A1Qmipiraw_sensor.c
 *
 * Project:
 * --------
 *	 ALPS MT6795
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 * sensor setting : AM11a
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
#ifdef CONFIG_RLK_CAM_PERFORMANCE_IMPROVE
#include <linux/dma-mapping.h>
#endif
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include <linux/slab.h>
#include "ov16a1qmipiraw_Sensor.h"

#define MULTI_WRITE 1

#if MULTI_WRITE
static const int I2C_BUFFER_LEN = 1020; /*trans# max is 255, each 4 bytes*/
#else
static const int I2C_BUFFER_LEN = 4;
#endif

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = OV16A1Q_SENSOR_ID,		//record sensor id defined in Kd_imgsensor.h

	.checksum_value = 0x47a75476,		//checksum value for Camera Auto Test

	.pre = {
		.pclk = 100000000,				//record different mode's pclk
		.linelength  = 1700,				//record different mode's linelength
		.framelength = 1960,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2304,		//record different mode's width of grabwindow
		.grabwindow_height = 1728,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 290400000,
	},
	.cap = {
		.pclk = 100000000,
		.linelength  = 1700,
		.framelength = 1960,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 2304,
		.grabwindow_height = 1728,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 290400000,
	},
	.cap1 = {							//capture for 15fps
		.pclk = 150000000,
		.linelength  = 1700,
		.framelength = 2582,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 3264,
		.grabwindow_height = 2448,
		.mipi_data_lp2hs_settle_dc = 15,
		.max_framerate = 150,
	},
	.normal_video = { // cap
		.pclk = 100000000,
		.linelength  = 1700,
		.framelength = 1960,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 2304,
		.grabwindow_height = 1728,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 290400000,
	},
	.hs_video = {
		.pclk = 144000000,				//record different mode's pclk
		.linelength  = 1932,				//record different mode's linelength
		.framelength = 620,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 640,		//record different mode's width of grabwindow
		.grabwindow_height = 480,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 15,
		.max_framerate = 1200,
		.mipi_pixel_rate = 144000000,
	},
	.slim_video = {//pre
		.pclk = 144000000,				//record different mode's pclk
		.linelength  = 1932,				//record different mode's linelength
		.framelength = 2482,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1632,		//record different mode's width of grabwindow
		.grabwindow_height = 1224,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 15,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 144000000,
	},
	.margin = 6,			//sensor framelength & shutter margin
	.min_shutter = 6,		//min shutter
	.min_gain = 64,
	.max_gain = 4096,
	.min_gain_iso = 100,
	.gain_step = 1,
	.gain_type = 0,
	.max_frame_length = 0x90f7,//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,	//shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 3,	  //support sensor mode num ,don't support Slow motion
	.cap_delay_frame = 3,		//enter capture delay frame num
	.pre_delay_frame = 3, 		//enter preview delay frame num
	.video_delay_frame = 3,		//enter video delay frame num
	.hs_video_delay_frame = 3,	//enter high speed video  delay frame num
	.slim_video_delay_frame = 3,//enter slim video delay frame num

	.isp_driving_current = ISP_DRIVING_8MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_MANUAL,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
	.i2c_addr_table = {0x20, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff

};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x4C00,					//current shutter
	.gain = 0x200,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 30,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = 0, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x20,//record current sensor's i2c write id
	.current_ae_effective_frame = 2,
	.vendor_id = 0,
};

/* Sensor output window information*/
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] = {
 { 4656, 3496, 0, 0,  	   4656, 3496, 2328, 1748,   12, 10,  2304, 1728, 0, 0, 2304, 1728 },// Preview
 { 4656, 3496, 0, 0,  	   4656, 3496, 2328, 1748,   12, 10,  2304, 1728, 0, 0, 2304, 1728 },//capture
 { 4656, 3496, 0, 0,  	   4656, 3496, 2328, 1748,   12, 10,  2304, 1728, 0, 0, 2304, 1728 },//normal-video
 { 3296, 2480,	  336,	272, 2624, 1936,  656,  484,   8,	2,  640,  480,	 0, 0,  640,  480}, //hight speed video  //8 2
 { 3296, 2480,	  0,	12, 3296, 2456, 1648, 1228,   8,	2, 1632, 1224,	 0, 0, 1632, 1224}, // slim video
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static kal_uint32 return_sensor_id(void)
{
	kal_uint32 sensor_id = 0;

	sensor_id = ((read_cmos_sensor(0x300B) << 8) | read_cmos_sensor(0x300C));

	return sensor_id;
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	write_cmos_sensor(0x380c, imgsensor.line_length >> 8);
	write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);
}	/*	set_dummy  */

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	//kal_int16 dummy_line;
	kal_uint32 frame_length = imgsensor.frame_length;
	//unsigned long flags;

	LOG_INF("framerate = %d, min framelength =%d, should enable? \n", framerate, min_framelength_en);

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
	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
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
	kal_uint32 frame_length = 0;

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
    // Framelength should be an even number
	   imgsensor.frame_length = ((imgsensor.frame_length + 1) >> 1) << 1;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
	} else {
		// Extend frame length
		write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	}

	// Update Shutter
	write_cmos_sensor(0x3500, (shutter>>12) & 0x0F);
	write_cmos_sensor(0x3501, (shutter>>4) & 0xFF);
	write_cmos_sensor(0x3502, (shutter<<4) & 0xF0);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

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
static void set_shutter(kal_uint32 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;

	kal_uint32 shutter_us = 0;
	kal_uint32 LL = 0;
	kal_uint32 fix_shutter = 0xfe500;

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

//	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
//brad comment this tp prevent bad long shutter
	    // Framelength should be an even number
	imgsensor.frame_length = ((imgsensor.frame_length + 1) >> 1) << 1;

	if (shutter > 42507) {
		printk("brad long exp %d %d \n", shutter, imgsensor.frame_length);

		//write_cmos_sensor(0x0100, 0x00);  /*stream on*/

		//write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		//write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);

		shutter_us = (shutter << 8) / (imgsensor.pclk / imgsensor.line_length);
		//LL = shutter_us * 0x8FF / 1000000;
		LL = shutter_us * 9;

		printk("shutter_us: %d, SHUTTER:x%x pck:x%x ll:x%x L: 0x%x\n",
			shutter_us, shutter, imgsensor.pclk, imgsensor.line_length, LL);

		//shutter=0xfe500;

		write_cmos_sensor(0x380e, 0xff);
		write_cmos_sensor(0x380f, 0xff);
		write_cmos_sensor(0x380c, LL >> 8);
		write_cmos_sensor(0x380d, LL & 0XFF);


		write_cmos_sensor(0x3502, (fix_shutter << 4) & 0xFF);
		write_cmos_sensor(0x3501, (fix_shutter >> 4) & 0xFF);
		write_cmos_sensor(0x3500, (fix_shutter >> 12) & 0x0F);

		//write_cmos_sensor(0x0100, 0x01);  /*stream on*/

		imgsensor.ae_frm_mode.frame_mode_1 = IMGSENSOR_AE_MODE_SKIP;
		imgsensor.ae_frm_mode.frame_mode_2 = IMGSENSOR_AE_MODE_SKIP;
		imgsensor.current_ae_effective_frame = 2;
	} else {
		if (imgsensor.autoflicker_en) {
			realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
			if (realtime_fps >= 297 && realtime_fps <= 305)
				set_max_framerate(296, 0);
			else if (realtime_fps >= 147 && realtime_fps <= 150)
				set_max_framerate(146, 0);
			else {
				// Extend frame length
				write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
				write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
			}
		} else {
			// Extend frame length
			write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
			write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		}

		// Update Shutter
		write_cmos_sensor(0x3502, (shutter << 4) & 0xFF);
		write_cmos_sensor(0x3501, (shutter >> 4) & 0xFF);
		write_cmos_sensor(0x3500, (shutter >> 12) & 0x0F);
		write_cmos_sensor(0x380c, imgsensor.line_length>>8);
		write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);

		imgsensor.current_ae_effective_frame = 2;

	}
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);
}

#if 0
static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;

	reg_gain = gain * 2;
	//reg_gain = reg_gain & 0xFFFF;
	return (kal_uint16)reg_gain;
}
#endif
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
	if (gain < BASEGAIN || gain > 16 * BASEGAIN) {
		LOG_INF("Error gain setting");

		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 16 * BASEGAIN)
			gain = 16 * BASEGAIN;
	}

	//reg_gain = gain2reg(gain);
	reg_gain = gain*2;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x3508, (reg_gain>>8));
	write_cmos_sensor(0x3509, (reg_gain&0xFF));
	return gain;
}	/*	set_gain  */

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
		if (le < imgsensor_info.min_shutter)
			le = imgsensor_info.min_shutter;

		if (se < imgsensor_info.min_shutter)
			se = imgsensor_info.min_shutter;

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



static kal_uint32 streaming_control(kal_bool enable)
{
	printk("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable) {
		mdelay(5);
		write_cmos_sensor(0x0100, 0x01);
		mdelay(5);
	} else {
		write_cmos_sensor(0x0100, 0x00);
	}
	return ERROR_NONE;
}

#if 0
static kal_uint16 table_write_cmos_sensor(kal_uint16 *para,
					  kal_uint32 len)
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
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;

		}
#if MULTI_WRITE
		/* Write when remain buffer size is less than 3 bytes
		 * or reach end of data
		 */
		if ((I2C_BUFFER_LEN - tosend) < 3
			|| IDX == len || addr != addr_last) {
			iBurstWriteReg_multi(puSendCmd,
						tosend,
						imgsensor.i2c_write_id,
						3,
						400);
			tosend = 0;
		}
#else
		iWriteRegI2C(puSendCmd, 3, imgsensor.i2c_write_id);
		tosend = 0;
#endif
	}

#if 0 /*for debug*/
	for (int i = 0; i < len/2; i++)
		LOG_INF("readback addr(0x%x)=0x%x\n",
			para[2*i], read_cmos_sensor_8(para[2*i]));
#endif
	return 0;
}
#endif

static void sensor_init(void)
{
write_cmos_sensor(0x0103, 0x01);
mdelay(5);
write_cmos_sensor(0x0102, 0x00);
write_cmos_sensor(0x0301, 0x48);
write_cmos_sensor(0x0302, 0x31);
write_cmos_sensor(0x0303, 0x04);
write_cmos_sensor(0x0305, 0xc2);
write_cmos_sensor(0x0306, 0x00);
write_cmos_sensor(0x0320, 0x02);
write_cmos_sensor(0x0323, 0x04);
write_cmos_sensor(0x0326, 0xd8);
write_cmos_sensor(0x0327, 0x0b);
write_cmos_sensor(0x0329, 0x01);
write_cmos_sensor(0x0343, 0x04);
write_cmos_sensor(0x0344, 0x01);
write_cmos_sensor(0x0345, 0x2c);
write_cmos_sensor(0x0346, 0xc0);
write_cmos_sensor(0x034a, 0x07);
write_cmos_sensor(0x300e, 0x22);
write_cmos_sensor(0x3012, 0x41);
write_cmos_sensor(0x3016, 0xd2);
write_cmos_sensor(0x3018, 0x70);
write_cmos_sensor(0x301e, 0x98);
write_cmos_sensor(0x3025, 0x03);
write_cmos_sensor(0x3026, 0x10);
write_cmos_sensor(0x3027, 0x08);
write_cmos_sensor(0x3102, 0x00);
write_cmos_sensor(0x3400, 0x04);
write_cmos_sensor(0x3406, 0x04);
write_cmos_sensor(0x3408, 0x04);
write_cmos_sensor(0x3421, 0x09);
write_cmos_sensor(0x3422, 0x20);
write_cmos_sensor(0x3423, 0x15);
write_cmos_sensor(0x3424, 0x40);
write_cmos_sensor(0x3425, 0x14);
write_cmos_sensor(0x3426, 0x04);
write_cmos_sensor(0x3504, 0x08);
write_cmos_sensor(0x3508, 0x01);
write_cmos_sensor(0x3509, 0x00);
write_cmos_sensor(0x350a, 0x01);
write_cmos_sensor(0x350b, 0x00);
write_cmos_sensor(0x350c, 0x00);
write_cmos_sensor(0x3548, 0x01);
write_cmos_sensor(0x3549, 0x00);
write_cmos_sensor(0x354a, 0x01);
write_cmos_sensor(0x354b, 0x00);
write_cmos_sensor(0x354c, 0x00);
write_cmos_sensor(0x3600, 0xff);
write_cmos_sensor(0x3602, 0x42);
write_cmos_sensor(0x3603, 0x7b);
write_cmos_sensor(0x3608, 0x9b);
write_cmos_sensor(0x360a, 0x69);
write_cmos_sensor(0x360b, 0x53);
write_cmos_sensor(0x3618, 0xc0);
write_cmos_sensor(0x361a, 0x8b);
write_cmos_sensor(0x361d, 0x20);
write_cmos_sensor(0x361e, 0x30);
write_cmos_sensor(0x361f, 0x01);
write_cmos_sensor(0x3620, 0x89);
write_cmos_sensor(0x3624, 0x8f);
write_cmos_sensor(0x3629, 0x09);
write_cmos_sensor(0x362e, 0x50);
write_cmos_sensor(0x3631, 0xe2);
write_cmos_sensor(0x3632, 0xe2);
write_cmos_sensor(0x3634, 0x10);
write_cmos_sensor(0x3635, 0x10);
write_cmos_sensor(0x3636, 0x10);
write_cmos_sensor(0x3639, 0xa6);
write_cmos_sensor(0x363a, 0xaa);
write_cmos_sensor(0x363b, 0x0c);
write_cmos_sensor(0x363c, 0x16);
write_cmos_sensor(0x363d, 0x29);
write_cmos_sensor(0x363e, 0x4f);
write_cmos_sensor(0x3642, 0xa8);
write_cmos_sensor(0x3652, 0x00);
write_cmos_sensor(0x3653, 0x00);
write_cmos_sensor(0x3654, 0x8a);
write_cmos_sensor(0x3656, 0x0c);
write_cmos_sensor(0x3657, 0x8e);
write_cmos_sensor(0x3660, 0x80);
write_cmos_sensor(0x3663, 0x00);
write_cmos_sensor(0x3664, 0x00);
write_cmos_sensor(0x3668, 0x05);
write_cmos_sensor(0x3669, 0x05);
write_cmos_sensor(0x370d, 0x10);
write_cmos_sensor(0x370e, 0x05);
write_cmos_sensor(0x370f, 0x10);
write_cmos_sensor(0x3711, 0x01);
write_cmos_sensor(0x3712, 0x09);
write_cmos_sensor(0x3713, 0x40);
write_cmos_sensor(0x3714, 0xe4);
write_cmos_sensor(0x3716, 0x04);
write_cmos_sensor(0x3717, 0x01);
write_cmos_sensor(0x3718, 0x02);
write_cmos_sensor(0x3719, 0x01);
write_cmos_sensor(0x371a, 0x02);
write_cmos_sensor(0x371b, 0x02);
write_cmos_sensor(0x371c, 0x01);
write_cmos_sensor(0x371d, 0x02);
write_cmos_sensor(0x371e, 0x12);
write_cmos_sensor(0x371f, 0x02);
write_cmos_sensor(0x3720, 0x14);
write_cmos_sensor(0x3721, 0x12);
write_cmos_sensor(0x3722, 0x44);
write_cmos_sensor(0x3723, 0x60);
write_cmos_sensor(0x372f, 0x34);
write_cmos_sensor(0x3726, 0x21);
write_cmos_sensor(0x37d0, 0x02);
write_cmos_sensor(0x37d1, 0x10);
write_cmos_sensor(0x37db, 0x08);
write_cmos_sensor(0x3808, 0x12);
write_cmos_sensor(0x3809, 0x30);
write_cmos_sensor(0x380a, 0x0d);
write_cmos_sensor(0x380b, 0xa8);
write_cmos_sensor(0x380c, 0x03);
write_cmos_sensor(0x380d, 0x52);
write_cmos_sensor(0x380e, 0x0f);
write_cmos_sensor(0x380f, 0x51);
write_cmos_sensor(0x3814, 0x11);
write_cmos_sensor(0x3815, 0x11);
write_cmos_sensor(0x3820, 0x00);
write_cmos_sensor(0x3821, 0x06);
write_cmos_sensor(0x3822, 0x00);
write_cmos_sensor(0x3823, 0x04);
write_cmos_sensor(0x3837, 0x10);
write_cmos_sensor(0x383c, 0x34);
write_cmos_sensor(0x383d, 0xff);
write_cmos_sensor(0x383e, 0x0d);
write_cmos_sensor(0x383f, 0x22);
write_cmos_sensor(0x3857, 0x00);
write_cmos_sensor(0x388f, 0x00);
write_cmos_sensor(0x3890, 0x00);
write_cmos_sensor(0x3891, 0x00);
write_cmos_sensor(0x3d81, 0x10);
write_cmos_sensor(0x3d83, 0x0c);
write_cmos_sensor(0x3d84, 0x00);
write_cmos_sensor(0x3d85, 0x1b);
write_cmos_sensor(0x3d88, 0x00);
write_cmos_sensor(0x3d89, 0x00);
write_cmos_sensor(0x3d8a, 0x00);
write_cmos_sensor(0x3d8b, 0x01);
write_cmos_sensor(0x3d8c, 0x77);
write_cmos_sensor(0x3d8d, 0xa0);
write_cmos_sensor(0x3f00, 0x02);
write_cmos_sensor(0x3f0c, 0x07);
write_cmos_sensor(0x3f0d, 0x2f);
write_cmos_sensor(0x4012, 0x0d);
write_cmos_sensor(0x4015, 0x04);
write_cmos_sensor(0x4016, 0x1b);
write_cmos_sensor(0x4017, 0x04);
write_cmos_sensor(0x4018, 0x0b);
write_cmos_sensor(0x401b, 0x1f);
write_cmos_sensor(0x401e, 0x01);
write_cmos_sensor(0x401f, 0x38);
write_cmos_sensor(0x4500, 0x20);
write_cmos_sensor(0x4501, 0x6a);
write_cmos_sensor(0x4502, 0xb4);
write_cmos_sensor(0x4586, 0x00);
write_cmos_sensor(0x4588, 0x02);
write_cmos_sensor(0x4640, 0x01);
write_cmos_sensor(0x4641, 0x04);
write_cmos_sensor(0x4643, 0x00);
write_cmos_sensor(0x4645, 0x03);
write_cmos_sensor(0x4806, 0x40);
write_cmos_sensor(0x480e, 0x00);
write_cmos_sensor(0x4815, 0x2b);
write_cmos_sensor(0x4819, 0x70);//0x90
write_cmos_sensor(0x4826, 0x32);//0x50
write_cmos_sensor(0x4833, 0x18);
write_cmos_sensor(0x4837, 0x08);
write_cmos_sensor(0x484b, 0x07);
write_cmos_sensor(0x4850, 0x41);
write_cmos_sensor(0x4860, 0x00);
write_cmos_sensor(0x4861, 0xec);
write_cmos_sensor(0x4864, 0x00);
write_cmos_sensor(0x4883, 0x00);
//write_cmos_sensor(0x4885, 0x0f);
write_cmos_sensor(0x4888, 0x10);
write_cmos_sensor(0x4a00, 0x10);
write_cmos_sensor(0x4e00, 0x00);
write_cmos_sensor(0x4e01, 0x04);
write_cmos_sensor(0x4e02, 0x01);
write_cmos_sensor(0x4e03, 0x00);
write_cmos_sensor(0x4e04, 0x08);
write_cmos_sensor(0x4e05, 0x04);
write_cmos_sensor(0x4e06, 0x00);
write_cmos_sensor(0x4e07, 0x13);
write_cmos_sensor(0x4e08, 0x01);
write_cmos_sensor(0x4e09, 0x00);
write_cmos_sensor(0x4e0a, 0x15);
write_cmos_sensor(0x4e0b, 0x0e);
write_cmos_sensor(0x4e0c, 0x00);
write_cmos_sensor(0x4e0d, 0x17);
write_cmos_sensor(0x4e0e, 0x07);
write_cmos_sensor(0x4e0f, 0x00);
write_cmos_sensor(0x4e10, 0x19);
write_cmos_sensor(0x4e11, 0x06);
write_cmos_sensor(0x4e12, 0x00);
write_cmos_sensor(0x4e13, 0x1b);
write_cmos_sensor(0x4e14, 0x08);
write_cmos_sensor(0x4e15, 0x00);
write_cmos_sensor(0x4e16, 0x1f);
write_cmos_sensor(0x4e17, 0x08);
write_cmos_sensor(0x4e18, 0x00);
write_cmos_sensor(0x4e19, 0x21);
write_cmos_sensor(0x4e1a, 0x0e);
write_cmos_sensor(0x4e1b, 0x00);
write_cmos_sensor(0x4e1c, 0x2d);
write_cmos_sensor(0x4e1d, 0x30);
write_cmos_sensor(0x4e1e, 0x00);
write_cmos_sensor(0x4e1f, 0x6a);
write_cmos_sensor(0x4e20, 0x05);
write_cmos_sensor(0x4e21, 0x00);
write_cmos_sensor(0x4e22, 0x6c);
write_cmos_sensor(0x4e23, 0x05);
write_cmos_sensor(0x4e24, 0x00);
write_cmos_sensor(0x4e25, 0x6e);
write_cmos_sensor(0x4e26, 0x39);
write_cmos_sensor(0x4e27, 0x00);
write_cmos_sensor(0x4e28, 0x7a);
write_cmos_sensor(0x4e29, 0x6d);
write_cmos_sensor(0x4e2a, 0x00);
write_cmos_sensor(0x4e2b, 0x00);
write_cmos_sensor(0x4e2c, 0x00);
write_cmos_sensor(0x4e2d, 0x00);
write_cmos_sensor(0x4e2e, 0x00);
write_cmos_sensor(0x4e2f, 0x00);
write_cmos_sensor(0x4e30, 0x00);
write_cmos_sensor(0x4e31, 0x00);
write_cmos_sensor(0x4e32, 0x00);
write_cmos_sensor(0x4e33, 0x00);
write_cmos_sensor(0x4e34, 0x00);
write_cmos_sensor(0x4e35, 0x00);
write_cmos_sensor(0x4e36, 0x00);
write_cmos_sensor(0x4e37, 0x00);
write_cmos_sensor(0x4e38, 0x00);
write_cmos_sensor(0x4e39, 0x00);
write_cmos_sensor(0x4e3a, 0x00);
write_cmos_sensor(0x4e3b, 0x00);
write_cmos_sensor(0x4e3c, 0x00);
write_cmos_sensor(0x4e3d, 0x00);
write_cmos_sensor(0x4e3e, 0x00);
write_cmos_sensor(0x4e3f, 0x00);
write_cmos_sensor(0x4e40, 0x00);
write_cmos_sensor(0x4e41, 0x00);
write_cmos_sensor(0x4e42, 0x00);
write_cmos_sensor(0x4e43, 0x00);
write_cmos_sensor(0x4e44, 0x00);
write_cmos_sensor(0x4e45, 0x00);
write_cmos_sensor(0x4e46, 0x00);
write_cmos_sensor(0x4e47, 0x00);
write_cmos_sensor(0x4e48, 0x00);
write_cmos_sensor(0x4e49, 0x00);
write_cmos_sensor(0x4e4a, 0x00);
write_cmos_sensor(0x4e4b, 0x00);
write_cmos_sensor(0x4e4c, 0x00);
write_cmos_sensor(0x4e4d, 0x00);
write_cmos_sensor(0x4e4e, 0x00);
write_cmos_sensor(0x4e4f, 0x00);
write_cmos_sensor(0x4e50, 0x00);
write_cmos_sensor(0x4e51, 0x00);
write_cmos_sensor(0x4e52, 0x00);
write_cmos_sensor(0x4e53, 0x00);
write_cmos_sensor(0x4e54, 0x00);
write_cmos_sensor(0x4e55, 0x00);
write_cmos_sensor(0x4e56, 0x00);
write_cmos_sensor(0x4e57, 0x00);
write_cmos_sensor(0x4e58, 0x00);
write_cmos_sensor(0x4e59, 0x00);
write_cmos_sensor(0x4e5a, 0x00);
write_cmos_sensor(0x4e5b, 0x00);
write_cmos_sensor(0x4e5c, 0x00);
write_cmos_sensor(0x4e5d, 0x00);
write_cmos_sensor(0x4e5e, 0x00);
write_cmos_sensor(0x4e5f, 0x00);
write_cmos_sensor(0x4e60, 0x00);
write_cmos_sensor(0x4e61, 0x00);
write_cmos_sensor(0x4e62, 0x00);
write_cmos_sensor(0x4e63, 0x00);
write_cmos_sensor(0x4e64, 0x00);
write_cmos_sensor(0x4e65, 0x00);
write_cmos_sensor(0x4e66, 0x00);
write_cmos_sensor(0x4e67, 0x00);
write_cmos_sensor(0x4e68, 0x00);
write_cmos_sensor(0x4e69, 0x00);
write_cmos_sensor(0x4e6a, 0x00);
write_cmos_sensor(0x4e6b, 0x00);
write_cmos_sensor(0x4e6c, 0x00);
write_cmos_sensor(0x4e6d, 0x00);
write_cmos_sensor(0x4e6e, 0x00);
write_cmos_sensor(0x4e6f, 0x00);
write_cmos_sensor(0x4e70, 0x00);
write_cmos_sensor(0x4e71, 0x00);
write_cmos_sensor(0x4e72, 0x00);
write_cmos_sensor(0x4e73, 0x00);
write_cmos_sensor(0x4e74, 0x00);
write_cmos_sensor(0x4e75, 0x00);
write_cmos_sensor(0x4e76, 0x00);
write_cmos_sensor(0x4e77, 0x00);
write_cmos_sensor(0x4e78, 0x1c);
write_cmos_sensor(0x4e79, 0x1e);
write_cmos_sensor(0x4e7a, 0x00);
write_cmos_sensor(0x4e7b, 0x00);
write_cmos_sensor(0x4e7c, 0x2c);
write_cmos_sensor(0x4e7d, 0x2f);
write_cmos_sensor(0x4e7e, 0x79);
write_cmos_sensor(0x4e7f, 0x7b);
write_cmos_sensor(0x4e80, 0x0a);
write_cmos_sensor(0x4e81, 0x31);
write_cmos_sensor(0x4e82, 0x66);
write_cmos_sensor(0x4e83, 0x81);
write_cmos_sensor(0x4e84, 0x03);
write_cmos_sensor(0x4e85, 0x40);
write_cmos_sensor(0x4e86, 0x02);
write_cmos_sensor(0x4e87, 0x09);
write_cmos_sensor(0x4e88, 0x43);
write_cmos_sensor(0x4e89, 0x53);
write_cmos_sensor(0x4e8a, 0x32);
write_cmos_sensor(0x4e8b, 0x67);
write_cmos_sensor(0x4e8c, 0x05);
write_cmos_sensor(0x4e8d, 0x83);
write_cmos_sensor(0x4e8e, 0x00);
write_cmos_sensor(0x4e8f, 0x00);
write_cmos_sensor(0x4e90, 0x00);
write_cmos_sensor(0x4e91, 0x00);
write_cmos_sensor(0x4e92, 0x00);
write_cmos_sensor(0x4e93, 0x00);
write_cmos_sensor(0x4e94, 0x00);
write_cmos_sensor(0x4e95, 0x00);
write_cmos_sensor(0x4e96, 0x00);
write_cmos_sensor(0x4e97, 0x00);
write_cmos_sensor(0x4e98, 0x00);
write_cmos_sensor(0x4e99, 0x00);
write_cmos_sensor(0x4e9a, 0x00);
write_cmos_sensor(0x4e9b, 0x00);
write_cmos_sensor(0x4e9c, 0x00);
write_cmos_sensor(0x4e9d, 0x00);
write_cmos_sensor(0x4e9e, 0x00);
write_cmos_sensor(0x4e9f, 0x00);
write_cmos_sensor(0x4ea0, 0x00);
write_cmos_sensor(0x4ea1, 0x00);
write_cmos_sensor(0x4ea2, 0x00);
write_cmos_sensor(0x4ea3, 0x00);
write_cmos_sensor(0x4ea4, 0x00);
write_cmos_sensor(0x4ea5, 0x00);
write_cmos_sensor(0x4ea6, 0x1e);
write_cmos_sensor(0x4ea7, 0x20);
write_cmos_sensor(0x4ea8, 0x32);
write_cmos_sensor(0x4ea9, 0x6d);
write_cmos_sensor(0x4eaa, 0x18);
write_cmos_sensor(0x4eab, 0x7f);
write_cmos_sensor(0x4eac, 0x00);
write_cmos_sensor(0x4ead, 0x00);
write_cmos_sensor(0x4eae, 0x7c);
write_cmos_sensor(0x4eaf, 0x07);
write_cmos_sensor(0x4eb0, 0x7c);
write_cmos_sensor(0x4eb1, 0x07);
write_cmos_sensor(0x4eb2, 0x07);
write_cmos_sensor(0x4eb3, 0x1c);
write_cmos_sensor(0x4eb4, 0x07);
write_cmos_sensor(0x4eb5, 0x1c);
write_cmos_sensor(0x4eb6, 0x07);
write_cmos_sensor(0x4eb7, 0x1c);
write_cmos_sensor(0x4eb8, 0x07);
write_cmos_sensor(0x4eb9, 0x1c);
write_cmos_sensor(0x4eba, 0x07);
write_cmos_sensor(0x4ebb, 0x14);
write_cmos_sensor(0x4ebc, 0x07);
write_cmos_sensor(0x4ebd, 0x1c);
write_cmos_sensor(0x4ebe, 0x07);
write_cmos_sensor(0x4ebf, 0x1c);
write_cmos_sensor(0x4ec0, 0x07);
write_cmos_sensor(0x4ec1, 0x1c);
write_cmos_sensor(0x4ec2, 0x07);
write_cmos_sensor(0x4ec3, 0x1c);
write_cmos_sensor(0x4ec4, 0x2c);
write_cmos_sensor(0x4ec5, 0x2f);
write_cmos_sensor(0x4ec6, 0x79);
write_cmos_sensor(0x4ec7, 0x7b);
write_cmos_sensor(0x4ec8, 0x7c);
write_cmos_sensor(0x4ec9, 0x07);
write_cmos_sensor(0x4eca, 0x7c);
write_cmos_sensor(0x4ecb, 0x07);
write_cmos_sensor(0x4ecc, 0x00);
write_cmos_sensor(0x4ecd, 0x00);
write_cmos_sensor(0x4ece, 0x07);
write_cmos_sensor(0x4ecf, 0x31);
write_cmos_sensor(0x4ed0, 0x69);
write_cmos_sensor(0x4ed1, 0x7f);
write_cmos_sensor(0x4ed2, 0x67);
write_cmos_sensor(0x4ed3, 0x00);
write_cmos_sensor(0x4ed4, 0x00);
write_cmos_sensor(0x4ed5, 0x00);
write_cmos_sensor(0x4ed6, 0x7c);
write_cmos_sensor(0x4ed7, 0x07);
write_cmos_sensor(0x4ed8, 0x7c);
write_cmos_sensor(0x4ed9, 0x07);
write_cmos_sensor(0x4eda, 0x33);
write_cmos_sensor(0x4edb, 0x7f);
write_cmos_sensor(0x4edc, 0x00);
write_cmos_sensor(0x4edd, 0x16);
write_cmos_sensor(0x4ede, 0x00);
write_cmos_sensor(0x4edf, 0x00);
write_cmos_sensor(0x4ee0, 0x32);
write_cmos_sensor(0x4ee1, 0x70);
write_cmos_sensor(0x4ee2, 0x01);
write_cmos_sensor(0x4ee3, 0x30);
write_cmos_sensor(0x4ee4, 0x22);
write_cmos_sensor(0x4ee5, 0x28);
write_cmos_sensor(0x4ee6, 0x6f);
write_cmos_sensor(0x4ee7, 0x75);
write_cmos_sensor(0x4ee8, 0x00);
write_cmos_sensor(0x4ee9, 0x00);
write_cmos_sensor(0x4eea, 0x30);
write_cmos_sensor(0x4eeb, 0x7f);
write_cmos_sensor(0x4eec, 0x00);
write_cmos_sensor(0x4eed, 0x00);
write_cmos_sensor(0x4eee, 0x00);
write_cmos_sensor(0x4eef, 0x00);
write_cmos_sensor(0x4ef0, 0x69);
write_cmos_sensor(0x4ef1, 0x7f);
write_cmos_sensor(0x4ef2, 0x07);
write_cmos_sensor(0x4ef3, 0x30);
write_cmos_sensor(0x4ef4, 0x32);
write_cmos_sensor(0x4ef5, 0x09);
write_cmos_sensor(0x4ef6, 0x7d);
write_cmos_sensor(0x4ef7, 0x65);
write_cmos_sensor(0x4ef8, 0x00);
write_cmos_sensor(0x4ef9, 0x00);
write_cmos_sensor(0x4efa, 0x00);
write_cmos_sensor(0x4efb, 0x00);
write_cmos_sensor(0x4efc, 0x7f);
write_cmos_sensor(0x4efd, 0x09);
write_cmos_sensor(0x4efe, 0x7f);
write_cmos_sensor(0x4eff, 0x09);
write_cmos_sensor(0x4f00, 0x1e);
write_cmos_sensor(0x4f01, 0x7c);
write_cmos_sensor(0x4f02, 0x7f);
write_cmos_sensor(0x4f03, 0x09);
write_cmos_sensor(0x4f04, 0x7f);
write_cmos_sensor(0x4f05, 0x0b);
write_cmos_sensor(0x4f06, 0x7c);
write_cmos_sensor(0x4f07, 0x02);
write_cmos_sensor(0x4f08, 0x7c);
write_cmos_sensor(0x4f09, 0x02);
write_cmos_sensor(0x4f0a, 0x32);
write_cmos_sensor(0x4f0b, 0x64);
write_cmos_sensor(0x4f0c, 0x32);
write_cmos_sensor(0x4f0d, 0x64);
write_cmos_sensor(0x4f0e, 0x32);
write_cmos_sensor(0x4f0f, 0x64);
write_cmos_sensor(0x4f10, 0x32);
write_cmos_sensor(0x4f11, 0x64);
write_cmos_sensor(0x4f12, 0x31);
write_cmos_sensor(0x4f13, 0x4f);
write_cmos_sensor(0x4f14, 0x83);
write_cmos_sensor(0x4f15, 0x84);
write_cmos_sensor(0x4f16, 0x63);
write_cmos_sensor(0x4f17, 0x64);
write_cmos_sensor(0x4f18, 0x83);
write_cmos_sensor(0x4f19, 0x84);
write_cmos_sensor(0x4f1a, 0x31);
write_cmos_sensor(0x4f1b, 0x32);
write_cmos_sensor(0x4f1c, 0x7b);
write_cmos_sensor(0x4f1d, 0x7c);
write_cmos_sensor(0x4f1e, 0x2f);
write_cmos_sensor(0x4f1f, 0x30);
write_cmos_sensor(0x4f20, 0x30);
write_cmos_sensor(0x4f21, 0x69);
write_cmos_sensor(0x4d06, 0x08);
write_cmos_sensor(0x5000, 0x01);
write_cmos_sensor(0x5001, 0x40);
write_cmos_sensor(0x5002, 0x53);
write_cmos_sensor(0x5003, 0x42);
write_cmos_sensor(0x5005, 0x00);
write_cmos_sensor(0x5038, 0x00);
write_cmos_sensor(0x5081, 0x00);
write_cmos_sensor(0x5180, 0x00);
write_cmos_sensor(0x5181, 0x10);
write_cmos_sensor(0x5182, 0x07);
write_cmos_sensor(0x5183, 0x8f);
write_cmos_sensor(0x5820, 0xc5);
write_cmos_sensor(0x5854, 0x00);
write_cmos_sensor(0x58cb, 0x03);
write_cmos_sensor(0x5bd0, 0x15);
write_cmos_sensor(0x5bd1, 0x02);
write_cmos_sensor(0x5c0e, 0x11);
write_cmos_sensor(0x5c11, 0x00);
write_cmos_sensor(0x5c16, 0x02);
write_cmos_sensor(0x5c17, 0x01);
write_cmos_sensor(0x5c1a, 0x04);
write_cmos_sensor(0x5c1b, 0x03);
write_cmos_sensor(0x5c21, 0x10);
write_cmos_sensor(0x5c22, 0x10);
write_cmos_sensor(0x5c23, 0x04);
write_cmos_sensor(0x5c24, 0x0c);
write_cmos_sensor(0x5c25, 0x04);
write_cmos_sensor(0x5c26, 0x0c);
write_cmos_sensor(0x5c27, 0x04);
write_cmos_sensor(0x5c28, 0x0c);
write_cmos_sensor(0x5c29, 0x04);
write_cmos_sensor(0x5c2a, 0x0c);
write_cmos_sensor(0x5c2b, 0x01);
write_cmos_sensor(0x5c2c, 0x01);
write_cmos_sensor(0x5c2e, 0x08);
write_cmos_sensor(0x5c30, 0x04);
write_cmos_sensor(0x5c35, 0x03);
write_cmos_sensor(0x5c36, 0x03);
write_cmos_sensor(0x5c37, 0x03);
write_cmos_sensor(0x5c38, 0x03);
write_cmos_sensor(0x5d00, 0xff);
write_cmos_sensor(0x5d01, 0x0f);
write_cmos_sensor(0x5d02, 0x80);
write_cmos_sensor(0x5d03, 0x44);
write_cmos_sensor(0x5d05, 0xfc);
write_cmos_sensor(0x5d06, 0x0b);
write_cmos_sensor(0x5d08, 0x10);
write_cmos_sensor(0x5d09, 0x10);
write_cmos_sensor(0x5d0a, 0x04);
write_cmos_sensor(0x5d0b, 0x0c);
write_cmos_sensor(0x5d0c, 0x04);
write_cmos_sensor(0x5d0d, 0x0c);
write_cmos_sensor(0x5d0e, 0x04);
write_cmos_sensor(0x5d0f, 0x0c);
write_cmos_sensor(0x5d10, 0x04);
write_cmos_sensor(0x5d11, 0x0c);
write_cmos_sensor(0x5d12, 0x01);
write_cmos_sensor(0x5d13, 0x01);
write_cmos_sensor(0x5d15, 0x10);
write_cmos_sensor(0x5d16, 0x10);
write_cmos_sensor(0x5d17, 0x10);
write_cmos_sensor(0x5d18, 0x10);
write_cmos_sensor(0x5d1a, 0x10);
write_cmos_sensor(0x5d1b, 0x10);
write_cmos_sensor(0x5d1c, 0x10);
write_cmos_sensor(0x5d1d, 0x10);
write_cmos_sensor(0x5d1e, 0x04);
write_cmos_sensor(0x5d1f, 0x04);
write_cmos_sensor(0x5d20, 0x04);
write_cmos_sensor(0x5d27, 0x64);
write_cmos_sensor(0x5d28, 0xc8);
write_cmos_sensor(0x5d29, 0x96);
write_cmos_sensor(0x5d2a, 0xff);
write_cmos_sensor(0x5d2b, 0xc8);
write_cmos_sensor(0x5d2c, 0xff);
write_cmos_sensor(0x5d2d, 0x04);
write_cmos_sensor(0x5d34, 0x00);
write_cmos_sensor(0x5d35, 0x08);
write_cmos_sensor(0x5d36, 0x00);
write_cmos_sensor(0x5d37, 0x04);
write_cmos_sensor(0x5d4a, 0x00);
write_cmos_sensor(0x5d4c, 0x00);
/*
	table_write_cmos_sensor(ov16a1q_init_setting,
			sizeof(ov16a1q_init_setting) / sizeof(kal_uint16));
*/
}	/*	sensor_init  */

static void preview_setting(void)
{
	printk("loading preview_setting\n");
	write_cmos_sensor(0x0305, 0x7a);
	write_cmos_sensor(0x0307, 0x01);
	write_cmos_sensor(0x4837, 0x15);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x3608, 0x75);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x361e, 0x30);
	write_cmos_sensor(0x3639, 0x93);
	write_cmos_sensor(0x363a, 0x99);
	write_cmos_sensor(0x3642, 0x98);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x3663, 0x01);
	write_cmos_sensor(0x370e, 0x05);
	write_cmos_sensor(0x3712, 0x08);
	write_cmos_sensor(0x3713, 0xc0);
	write_cmos_sensor(0x3714, 0xe2);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x04);
	write_cmos_sensor(0x3808, 0x09);
	write_cmos_sensor(0x3809, 0x18);
	write_cmos_sensor(0x380a, 0x06);
	write_cmos_sensor(0x380b, 0xd4);
	write_cmos_sensor(0x380c, 0x06);
	write_cmos_sensor(0x380d, 0xa4);
	write_cmos_sensor(0x380e, 0x0f);
	write_cmos_sensor(0x380f, 0x50);
	write_cmos_sensor(0x3814, 0x22);
	write_cmos_sensor(0x3815, 0x22);
	write_cmos_sensor(0x3820, 0x01);
	write_cmos_sensor(0x3821, 0x0c);
	write_cmos_sensor(0x3822, 0x00);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x383f, 0x33);
	write_cmos_sensor(0x4015, 0x02);
	write_cmos_sensor(0x4016, 0x0d);
	write_cmos_sensor(0x4017, 0x00);
	write_cmos_sensor(0x4018, 0x07);
	write_cmos_sensor(0x401b, 0x1f);
	write_cmos_sensor(0x401f, 0xfe);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x6a);
	write_cmos_sensor(0x4502, 0xe4);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e1d, 0x25);
	write_cmos_sensor(0x4e26, 0x44);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x5000, 0x09);
	write_cmos_sensor(0x5001, 0x42);
	write_cmos_sensor(0x5003, 0x42);
	write_cmos_sensor(0x5820, 0xc5);
	write_cmos_sensor(0x5854, 0x00);
	write_cmos_sensor(0x5bd0, 0x19);
	write_cmos_sensor(0x5c0e, 0x13);
	write_cmos_sensor(0x5c11, 0x00);
	write_cmos_sensor(0x5c16, 0x01);
	write_cmos_sensor(0x5c17, 0x00);
	write_cmos_sensor(0x5c1a, 0x00);
	write_cmos_sensor(0x5c1b, 0x00);
	write_cmos_sensor(0x5c21, 0x08);
	write_cmos_sensor(0x5c22, 0x08);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x06);
	write_cmos_sensor(0x5c25, 0x02);
	write_cmos_sensor(0x5c26, 0x06);
	write_cmos_sensor(0x5c27, 0x02);
	write_cmos_sensor(0x5c28, 0x06);
	write_cmos_sensor(0x5c29, 0x02);
	write_cmos_sensor(0x5c2a, 0x06);
	write_cmos_sensor(0x5c2b, 0x00);
	write_cmos_sensor(0x5c2c, 0x00);
	write_cmos_sensor(0x5d01, 0x07);
	write_cmos_sensor(0x5d08, 0x08);
	write_cmos_sensor(0x5d09, 0x08);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x06);
	write_cmos_sensor(0x5d0c, 0x02);
	write_cmos_sensor(0x5d0d, 0x06);
	write_cmos_sensor(0x5d0e, 0x02);
	write_cmos_sensor(0x5d0f, 0x06);
	write_cmos_sensor(0x5d10, 0x02);
	write_cmos_sensor(0x5d11, 0x06);
	write_cmos_sensor(0x5d12, 0x00);
	write_cmos_sensor(0x5d13, 0x00);
	write_cmos_sensor(0x3500, 0x00);
	write_cmos_sensor(0x3501, 0x0f);
	write_cmos_sensor(0x3502, 0x48);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);

/*	table_write_cmos_sensor(ov16a1q_preview_setting,
		sizeof(ov16a1q_preview_setting) / sizeof(kal_uint16));*/
}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	printk("loading capture_setting\n");
	write_cmos_sensor(0x0305, 0x7a);
	write_cmos_sensor(0x0307, 0x01);
	write_cmos_sensor(0x4837, 0x15);
	write_cmos_sensor(0x0329, 0x01);
	write_cmos_sensor(0x0344, 0x01);
	write_cmos_sensor(0x0345, 0x2c);
	write_cmos_sensor(0x034a, 0x07);
	write_cmos_sensor(0x3608, 0x75);
	write_cmos_sensor(0x360a, 0x69);
	write_cmos_sensor(0x361a, 0x8b);
	write_cmos_sensor(0x361e, 0x30);
	write_cmos_sensor(0x3639, 0x93);
	write_cmos_sensor(0x363a, 0x99);
	write_cmos_sensor(0x3642, 0x98);
	write_cmos_sensor(0x3654, 0x8a);
	write_cmos_sensor(0x3656, 0x0c);
	write_cmos_sensor(0x3663, 0x01);
	write_cmos_sensor(0x370e, 0x05);
	write_cmos_sensor(0x3712, 0x08);
	write_cmos_sensor(0x3713, 0xc0);
	write_cmos_sensor(0x3714, 0xe2);
	write_cmos_sensor(0x37d0, 0x02);
	write_cmos_sensor(0x37d1, 0x10);
	write_cmos_sensor(0x37db, 0x04);
	write_cmos_sensor(0x3808, 0x09);
	write_cmos_sensor(0x3809, 0x18);
	write_cmos_sensor(0x380a, 0x06);
	write_cmos_sensor(0x380b, 0xd4);
	write_cmos_sensor(0x380c, 0x06);
	write_cmos_sensor(0x380d, 0xa4);
	write_cmos_sensor(0x380e, 0x0f);
	write_cmos_sensor(0x380f, 0x50);
	write_cmos_sensor(0x3814, 0x22);
	write_cmos_sensor(0x3815, 0x22);
	write_cmos_sensor(0x3820, 0x01);
	write_cmos_sensor(0x3821, 0x0c);
	write_cmos_sensor(0x3822, 0x00);
	write_cmos_sensor(0x383c, 0x22);
	write_cmos_sensor(0x383f, 0x33);
	write_cmos_sensor(0x4015, 0x02);
	write_cmos_sensor(0x4016, 0x0d);
	write_cmos_sensor(0x4017, 0x00);
	write_cmos_sensor(0x4018, 0x07);
	write_cmos_sensor(0x401b, 0x1f);
	write_cmos_sensor(0x401f, 0xfe);
	write_cmos_sensor(0x4500, 0x20);
	write_cmos_sensor(0x4501, 0x6a);
	write_cmos_sensor(0x4502, 0xe4);
	write_cmos_sensor(0x4e05, 0x04);
	write_cmos_sensor(0x4e11, 0x06);
	write_cmos_sensor(0x4e1d, 0x25);
	write_cmos_sensor(0x4e26, 0x44);
	write_cmos_sensor(0x4e29, 0x6d);
	write_cmos_sensor(0x5000, 0x09);
	write_cmos_sensor(0x5001, 0x42);
	write_cmos_sensor(0x5003, 0x42);
	write_cmos_sensor(0x5820, 0xc5);
	write_cmos_sensor(0x5854, 0x00);
	write_cmos_sensor(0x5bd0, 0x19);
	write_cmos_sensor(0x5c0e, 0x13);
	write_cmos_sensor(0x5c11, 0x00);
	write_cmos_sensor(0x5c16, 0x01);
	write_cmos_sensor(0x5c17, 0x00);
	write_cmos_sensor(0x5c1a, 0x00);
	write_cmos_sensor(0x5c1b, 0x00);
	write_cmos_sensor(0x5c21, 0x08);
	write_cmos_sensor(0x5c22, 0x08);
	write_cmos_sensor(0x5c23, 0x02);
	write_cmos_sensor(0x5c24, 0x06);
	write_cmos_sensor(0x5c25, 0x02);
	write_cmos_sensor(0x5c26, 0x06);
	write_cmos_sensor(0x5c27, 0x02);
	write_cmos_sensor(0x5c28, 0x06);
	write_cmos_sensor(0x5c29, 0x02);
	write_cmos_sensor(0x5c2a, 0x06);
	write_cmos_sensor(0x5c2b, 0x00);
	write_cmos_sensor(0x5c2c, 0x00);
	write_cmos_sensor(0x5d01, 0x07);
	write_cmos_sensor(0x5d08, 0x08);
	write_cmos_sensor(0x5d09, 0x08);
	write_cmos_sensor(0x5d0a, 0x02);
	write_cmos_sensor(0x5d0b, 0x06);
	write_cmos_sensor(0x5d0c, 0x02);
	write_cmos_sensor(0x5d0d, 0x06);
	write_cmos_sensor(0x5d0e, 0x02);
	write_cmos_sensor(0x5d0f, 0x06);
	write_cmos_sensor(0x5d10, 0x02);
	write_cmos_sensor(0x5d11, 0x06);
	write_cmos_sensor(0x5d12, 0x00);
	write_cmos_sensor(0x5d13, 0x00);
	write_cmos_sensor(0x3500, 0x00);
	write_cmos_sensor(0x3501, 0x0f);
	write_cmos_sensor(0x3502, 0x48);
	write_cmos_sensor(0x3508, 0x01);
	write_cmos_sensor(0x3509, 0x00);
/*
	table_write_cmos_sensor(ov16a1q_capture_setting,
		sizeof(ov16a1q_capture_setting) / sizeof(kal_uint16));*/
}	/*	preview_setting  */

static void vga_setting_120fps(void)
{
	write_cmos_sensor(0x0100, 0x00);

	write_cmos_sensor(0x0300, 0x00);
	write_cmos_sensor(0x0302, 0x3c);
	write_cmos_sensor(0x0303, 0x01);
	write_cmos_sensor(0x030b, 0x00);
	write_cmos_sensor(0x030c, 0x00);
	write_cmos_sensor(0x030d, 0x1e);
	write_cmos_sensor(0x3501, 0x25);
	write_cmos_sensor(0x3502, 0xc0);
	write_cmos_sensor(0x366e, 0x08);
	write_cmos_sensor(0x3714, 0x29);
	write_cmos_sensor(0x37c2, 0x34);
	write_cmos_sensor(0x3800, 0x01);
	write_cmos_sensor(0x3801, 0x50);
	write_cmos_sensor(0x3802, 0x01);
	write_cmos_sensor(0x3803, 0x10);
	write_cmos_sensor(0x3804, 0x0b);
	write_cmos_sensor(0x3805, 0x8f);
	write_cmos_sensor(0x3806, 0x08);
	write_cmos_sensor(0x3807, 0x9f);
	write_cmos_sensor(0x3808, 0x02);
	write_cmos_sensor(0x3809, 0x80);
	write_cmos_sensor(0x380a, 0x01);
	write_cmos_sensor(0x380b, 0xe0);
	write_cmos_sensor(0x380c, 0x07);
	write_cmos_sensor(0x380d, 0x8c);
	write_cmos_sensor(0x380e, 0x02);
	write_cmos_sensor(0x380f, 0x6c);
	write_cmos_sensor(0x3810, 0x00);
	write_cmos_sensor(0x3811, 0x08);
	write_cmos_sensor(0x3812, 0x00);
	write_cmos_sensor(0x3813, 0x02);
	write_cmos_sensor(0x3814, 0x03);
	write_cmos_sensor(0x3820, 0x90);
	write_cmos_sensor(0x3821, 0x67);
	write_cmos_sensor(0x382a, 0x07);
	write_cmos_sensor(0x4009, 0x05);
	write_cmos_sensor(0x4601, 0x40);
	write_cmos_sensor(0x4837, 0x16);
	write_cmos_sensor(0x5795, 0x00);
	write_cmos_sensor(0x5796, 0x10);
	write_cmos_sensor(0x5797, 0x10);
	write_cmos_sensor(0x5798, 0x73);
	write_cmos_sensor(0x5799, 0x73);
	write_cmos_sensor(0x579b, 0x00);
	write_cmos_sensor(0x579d, 0x00);
	write_cmos_sensor(0x579e, 0x05);
	write_cmos_sensor(0x579f, 0xa0);
	write_cmos_sensor(0x57a0, 0x03);
	write_cmos_sensor(0x57a1, 0x20);
	write_cmos_sensor(0x0100, 0x01);
}

/*
static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n", currefps);
	capture_setting(currefps);
}*/

static void hs_video_setting(void)
{
	LOG_INF("E\n");
	vga_setting_120fps();
}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
	preview_setting();
}


#define OV16A1Q_EEPROM_SLAVE_ADDR 0xA2
#define OV16A1Q_SENSOR_IIC_SLAVE_ADDR 0x20
#define OV16A1Q_EEPROM_SIZE  0x18F3
#define EEPROM_DATA_PATH "/data/vendor/camera_dump/ov16a1q_eeprom_data.bin"
#define OV16A1Q_EEPROM_CRC_AWB_CAL_SIZE 43
#define OV16A1Q_EEPROM_CRC_LSC_SIZE 1868
#define OV16A1Q_EEPROM_CRC_MANUFACTURING_SIZE 37
#define OV16A1Q_MANUFACTURE_PART_NUMBER "28C85521"
#define OV16A1Q_MPN_LENGTH 8

static uint8_t ov16a1q_eeprom[OV16A1Q_EEPROM_SIZE] = {0};
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

static void ov16a1q_read_data_from_eeprom(kal_uint8 slave, kal_uint32 start_add, uint32_t size)
{
	int i = 0;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = slave;
	spin_unlock(&imgsensor_drv_lock);

	//read eeprom data
	for (i = 0; i < size; i ++) {
		ov16a1q_eeprom[i] = read_cmos_sensor(start_add);
		start_add ++;
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = OV16A1Q_SENSOR_IIC_SLAVE_ADDR;
	spin_unlock(&imgsensor_drv_lock);
}

static void ov16a1q_eeprom_dump_bin(const char *file_name, uint32_t size, const void *data)
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

static calibration_status_t ov16a1q_check_awb_data(void *data)
{
	struct ov16a1q_eeprom_t *eeprom = (struct ov16a1q_eeprom_t*)data;
	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;

	if(!eeprom_util_check_crc16(eeprom->cie_src_1_ev,
		OV16A1Q_EEPROM_CRC_AWB_CAL_SIZE,
		convert_crc(eeprom->awb_crc16))) {
		LOG_INF("AWB CRC Fails!");
		return CRC_FAILURE;
	}

	unit.r = to_uint16_swap(eeprom->awb_src_1_r);
	unit.gr = to_uint16_swap(eeprom->awb_src_1_gr);
	unit.gb = to_uint16_swap(eeprom->awb_src_1_gb);
	unit.b = to_uint16_swap(eeprom->awb_src_1_b);
	unit.r_g = to_uint16_swap(eeprom->awb_src_1_rg_ratio);
	unit.b_g = to_uint16_swap(eeprom->awb_src_1_bg_ratio);
	unit.gr_gb = to_uint16_swap(eeprom->awb_src_1_gr_gb_ratio);

	golden.r = to_uint16_swap(eeprom->awb_src_1_golden_r);
	golden.gr = to_uint16_swap(eeprom->awb_src_1_golden_gr);
	golden.gb = to_uint16_swap(eeprom->awb_src_1_golden_gb);
	golden.b = to_uint16_swap(eeprom->awb_src_1_golden_b);
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

static calibration_status_t ov16a1q_check_lsc_data_mtk(void *data)
{
	struct ov16a1q_eeprom_t *eeprom = (struct ov16a1q_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->lsc_data_mtk, OV16A1Q_EEPROM_CRC_LSC_SIZE,
		convert_crc(eeprom->lsc_crc16_mtk))) {
		LOG_INF("LSC CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("LSC CRC Pass");
	return NO_ERRORS;
}

static void ov16a1q_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;
	struct ov16a1q_eeprom_t *eeprom = (struct ov16a1q_eeprom_t*)data;

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

	if (ret < 0 || ret >= MAX_CALIBRATION_STRING) {
		LOG_INF("snprintf of mnf->serial_number failed");
		mnf->serial_number[0] = 0;
	}
}

static calibration_status_t ov16a1q_check_manufacturing_data(void *data)
{
	struct ov16a1q_eeprom_t *eeprom = (struct ov16a1q_eeprom_t*)data;

	if(strncmp(eeprom->mpn, OV16A1Q_MANUFACTURE_PART_NUMBER, OV16A1Q_MPN_LENGTH) != 0) {
		LOG_INF("Manufacturing part number (%s) check Fails!", eeprom->mpn);
		return CRC_FAILURE;
	}

	if (!eeprom_util_check_crc16(data, OV16A1Q_EEPROM_CRC_MANUFACTURING_SIZE,
		convert_crc(eeprom->manufacture_crc16))) {
		LOG_INF("Manufacturing CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Manufacturing CRC Pass");
	return NO_ERRORS;
}

static void ov16a1q_eeprom_format_calibration_data(void *data)
{
	if (NULL == data) {
		LOG_INF("data is NULL");
		return;
	}

	mnf_status = ov16a1q_check_manufacturing_data(data);
	af_status = 0;
	awb_status = ov16a1q_check_awb_data(data);;
	lsc_status = ov16a1q_check_lsc_data_mtk(data);;
	pdaf_status = 0;
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
//extern void app_get_front_sensor_name(char *back_name);
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	LOG_INF("OV16A1Q get_imgsensor_id in\n");
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				ov16a1q_read_data_from_eeprom(OV16A1Q_EEPROM_SLAVE_ADDR, 0x0000, OV16A1Q_EEPROM_SIZE);
				ov16a1q_eeprom_dump_bin(EEPROM_DATA_PATH, OV16A1Q_EEPROM_SIZE, (void *)ov16a1q_eeprom);
				ov16a1q_eeprom_format_calibration_data((void *)ov16a1q_eeprom);
				LOG_INF("probe success, i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
				return ERROR_NONE;
			} else {
				LOG_INF("Read sensor id fail,i2c write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
			}
			retry--;
		} while (retry > 0);
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
	LOG_INF("PLATFORM:MT6785,MIPI 2LANE\n");
	//LOG_INF("preview 1280*960@30fps,864Mbps/lane; video 1280*960@30fps,864Mbps/lane; capture 5M@30fps,864Mbps/lane\n");
	//sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			sensor_id = return_sensor_id();
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail,i2c write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
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

	mdelay(10);
	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x2D00;
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
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();

	mdelay(10);
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
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//15fps
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
			LOG_INF("Warning: current_fps  is not support, so use cap1's setting: %d fps!\n",
				imgsensor_info.cap1.max_framerate / 10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);
	mdelay(10);

	if (imgsensor.test_pattern == KAL_TRUE)	{
		write_cmos_sensor(0x5000, (read_cmos_sensor(0x5000) & 0xBF) | 0x00);
	}

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
	capture_setting(imgsensor.current_fps);

	mdelay(10);

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

	mdelay(10);

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

	mdelay(10);

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

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
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

	sensor_info->calibration_status.mnf = mnf_status;
	sensor_info->calibration_status.af = af_status;
	sensor_info->calibration_status.awb = awb_status;
	sensor_info->calibration_status.lsc = lsc_status;
	sensor_info->calibration_status.pdaf = pdaf_status;
	sensor_info->calibration_status.dual = dual_status;
	ov16a1q_eeprom_get_mnf_data((void *)ov16a1q_eeprom, &sensor_info->mnf_calibration);

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
	set_max_framerate(imgsensor.current_fps, 1);

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
		set_dummy();
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
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	//case MSDK_SCENARIO_ID_CAMERA_ZSD:
		frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	default:  //coding with  preview scenario by default
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
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
	default:
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		// 0x5081[0]: 1 enable,  0 disable
		// 0x5081[5:4]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor(0x5081, 0x09);



	} else {
		write_cmos_sensor(0x5081, 0x00);
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
	unsigned long long *feature_data = (unsigned long long *) feature_para;
	// unsigned long long *feature_return_para=(unsigned long long *) feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data = (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
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
		*feature_return_para_32 = imgsensor.pclk;
		*feature_para_len = 4;
		break;

	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter(*feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		night_mode((BOOL)*feature_data);
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
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
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
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[0], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n", (UINT16)*feature_data, (UINT16)*(feature_data + 1), (UINT16)*(feature_data + 2));
		ihdr_write_shutter_gain((UINT16)*feature_data, (UINT16)*(feature_data+1), (UINT16)*(feature_data+2));
		break;
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
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.hs_video.pclk /
			(imgsensor_info.hs_video.linelength - 80))*
			imgsensor_info.hs_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.slim_video.pclk /
			(imgsensor_info.slim_video.linelength - 80))*
			imgsensor_info.slim_video.grabwindow_width;

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
	case SENSOR_FEATURE_GET_AE_EFFECTIVE_FRAME_FOR_LE:
		*feature_return_para_32 = imgsensor.current_ae_effective_frame;
		LOG_INF("GET AE EFFECTIVE %d\n", *feature_return_para_32);
		break;
	case SENSOR_FEATURE_GET_AE_FRAME_MODE_FOR_LE:
		memcpy(feature_return_para_32, &imgsensor.ae_frm_mode, sizeof(struct IMGSENSOR_AE_FRM_MODE));
		LOG_INF("GET_AE_FRAME_MODE");
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
}    /*    feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 MOT_LISBON_OV16A1Q_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
    LOG_INF("MOT_LISBON_OV16A1Q_MIPI_RAW_SensorInit in\n");
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}	/*	MOT_LISBON_OV16A1Q_MIPI_RAW_SensorInit	*/
