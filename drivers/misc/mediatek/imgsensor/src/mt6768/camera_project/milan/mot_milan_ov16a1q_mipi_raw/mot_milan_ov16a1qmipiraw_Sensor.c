/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 mot_milan_ov16a1qmipiraw_Sensor.c
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
#include "mot_milan_ov16a1qmipiraw_Sensor.h"

#define MULTI_WRITE 1

#if MULTI_WRITE
static const int I2C_BUFFER_LEN = 765; /*trans# max is 255, each 4 bytes*/
#else
static const int I2C_BUFFER_LEN = 4;
#endif

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_MILAN_OV16A1Q_SENSOR_ID,		//record sensor id defined in Kd_imgsensor.h

	.checksum_value = 0x47a75476,		//checksum value for Camera Auto Test

	.pre = {
		.pclk = 100000000,				//record different mode's pclk
		.linelength  = 850,				//record different mode's linelength
		.framelength = 3920,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2304,		//record different mode's width of grabwindow
		.grabwindow_height = 1728,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 298000000,
	},
	.cap = {
		.pclk = 100000000,
		.linelength  = 850,
		.framelength = 3920,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 2304,
		.grabwindow_height = 1728,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 298000000,
	},
	.normal_video = { // cap
		.pclk = 100000000,
		.linelength  = 850,
		.framelength = 3920,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 2304,
		.grabwindow_height = 1728,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 298000000,
	},
	.hs_video = {
		.pclk = 100000000,               //record different mode's pclk
		.linelength  = 425,              //record different mode's linelength
		.framelength = 1960,              //record different mode's framelength
		.startx = 0,                     //record different mode's startx of grabwindow
		.starty = 0,                     //record different mode's starty of grabwindow
		.grabwindow_width  = 1164,       //record different mode's width of grabwindow
		.grabwindow_height = 874,        //record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
		.mipi_pixel_rate = 290400000,
	},
	.slim_video = {//pre
		.pclk = 100000000,				//record different mode's pclk
		.linelength  = 850,				//record different mode's linelength
		.framelength = 3920,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 2304,		//record different mode's width of grabwindow
		.grabwindow_height = 1728,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 15,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 298000000,
	},
	.custom1 = {//pre
		.pclk = 100000000,				//record different mode's pclk
		.linelength  = 425,				//record different mode's linelength
		.framelength = 980,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1280,		//record different mode's width of grabwindow
		.grabwindow_height = 720,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 2400,
		.mipi_pixel_rate = 602000000,
	},
	.custom3 = {//full size capture
		.pclk = 100000000,
		.linelength  = 850,
		.framelength = 3920,
		.startx = 0,
		.starty = 0,
		.grabwindow_width  = 4608,
		.grabwindow_height = 3456,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate =594000000,
	},
	.margin = 8,			//sensor framelength & shutter margin
	.min_shutter = 8,		//min shutter
	.min_gain = 64,
	.max_gain = 992,
	.min_gain_iso = 100,
	.gain_step = 1,
	.gain_type = 1,
	.max_frame_length = 0x7FFE,//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,	//shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.frame_time_delay_frame = 2, /* The delay frame of setting frame length  */
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 8,	  //support sensor mode num ,don't support Slow motion
	.cap_delay_frame = 2,		//enter capture delay frame num
	.pre_delay_frame = 2, 		//enter preview delay frame num
	.video_delay_frame = 2,		//enter video delay frame num
	.hs_video_delay_frame = 2,	//enter high speed video  delay frame num
	.slim_video_delay_frame = 2,//enter slim video delay frame num
	.custom1_delay_frame = 2,//enter slim video delay frame num
	.custom3_delay_frame = 2,	//enter custom3 delay frame num

	.isp_driving_current = ISP_DRIVING_8MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_B,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
	.i2c_addr_table = {0x20, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
	.i2c_speed = 1000,

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
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[7] = {
 { 4656, 3496, 0, 0,  	   4656, 3496, 2328, 1748,   12, 10,  2304, 1728, 0, 0, 2304, 1728 }, // Preview
 { 4656, 3496, 0, 0,  	   4656, 3496, 2328, 1748,   12, 10,  2304, 1728, 0, 0, 2304, 1728 }, // capture
 { 4656, 3496, 0, 0,  	   4656, 3496, 2328, 1748,   12, 10,  2304, 1728, 0, 0, 2304, 1728 }, // normal-video
 { 4656, 3496, 0, 0,  	   4656, 3496, 1164,  874,    0,  0,  1164,  874, 0, 0, 1164,  874 }, // hight speed video
 { 4656, 3496,    0,    0, 4656, 3496, 2328, 1748,   12, 10,  2304, 1728, 0, 0, 2304, 1728 }, // slim video
 { 4656, 3496, 1066, 1044, 2560, 1440, 1280,  720,    0,  0,  1280,  720, 0, 0, 1280,  720 }, // custom1
 { 4656, 3496, 0, 0,  	   4656, 3496, 4656, 3496,   24, 20,  4608, 3456, 0, 0, 4608, 3456 }, // custom3
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

	write_cmos_sensor(0x3208, 0x00);
	write_cmos_sensor(0x380c, (imgsensor.line_length >> 8) & 0xFF);
	write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);
	write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0x7F);
	write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFE);
	write_cmos_sensor(0x3208, 0x10);
	write_cmos_sensor(0x3208, 0xa0);
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


static void write_shutter(kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;
	static bool islongexp = 0;

	kal_uint32 vts_register_val = 0;

	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;

	if(shutter <= 0x7FF6) {
		shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
			(imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
		if (imgsensor.autoflicker_en) {
			realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
			if (realtime_fps >= 297 && realtime_fps <= 305)
				set_max_framerate(296, 0);
			else if (realtime_fps >= 147 && realtime_fps <= 150)
				set_max_framerate(146, 0);
			else {
				write_cmos_sensor(0x3208, 0x00);

				if (shutter <= 0x2000) {
					write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0x7F);
					write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFE);
				}
				else { // 0x2000 < shutter <= 0x7FF6
					vts_register_val = imgsensor.frame_length - shutter + 0x2000;
					write_cmos_sensor(0x380e, (vts_register_val >> 8) & 0x7F);
					write_cmos_sensor(0x380f, vts_register_val & 0xFE);
				}

				write_cmos_sensor(0x3208, 0x10);
				write_cmos_sensor(0x3208, 0xa0);
			}
		} else {
			write_cmos_sensor(0x3208, 0x00);

			if (shutter <= 0x2000) {
				write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0x7F);
				write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFE);
			}
			else { // 0x2000 < shutter <= 0x7FF6
				vts_register_val = imgsensor.frame_length - shutter + 0x2000;
				write_cmos_sensor(0x380e, (vts_register_val >> 8) & 0x7F);
				write_cmos_sensor(0x380f, vts_register_val & 0xFE);
			}

			write_cmos_sensor(0x3208, 0x10);
			write_cmos_sensor(0x3208, 0xa0);
		}
		if(!islongexp) {
			write_cmos_sensor(0x3208, 0x01);
			write_cmos_sensor(0x3500, (shutter >> 16) & 0x7F);
			write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
			write_cmos_sensor(0x3502, (shutter) & 0xFE);
			write_cmos_sensor(0x3208, 0x11);
			write_cmos_sensor(0x3208, 0xa1);
		} else {
			write_cmos_sensor(0x3208, 0x03);
			write_cmos_sensor(0x3500, 0x00);
			write_cmos_sensor(0x3501, 0x0f);
			write_cmos_sensor(0x3502, 0x48);
			write_cmos_sensor(0x3208, 0x13);
			write_cmos_sensor(0x3208, 0xA3);
			islongexp = 0;
		}
	} else {
		write_cmos_sensor(0x3208, 0x03);
		write_cmos_sensor(0x3500, (shutter >> 16) & 0x7F);
		write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
		write_cmos_sensor(0x3502, (shutter) & 0xFE);
		write_cmos_sensor(0x3598, 0x01);
		write_cmos_sensor(0x3509, 0x00);
		write_cmos_sensor(0x350a, 0x01);
		write_cmos_sensor(0x350b, 0x00);
		write_cmos_sensor(0x380e, 0x20);
		write_cmos_sensor(0x380f, 0x08);
		write_cmos_sensor(0x5221, 0x00);
		write_cmos_sensor(0x5222, 0x5b);
		write_cmos_sensor(0x5223, 0xea);
		write_cmos_sensor(0x3208, 0x13);
		write_cmos_sensor(0x3208, 0xA3);
		islongexp = 1;
	}

	LOG_INF("shutter =%d, framelength =%d islongexp = %d \n", shutter, imgsensor.frame_length,islongexp);
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

	if (gain < imgsensor_info.min_gain || gain > imgsensor_info.max_gain) {
		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else
			gain = imgsensor_info.max_gain;
	}

	reg_gain = gain*4;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d, reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x3508, (reg_gain>>8) & 0x7f);
	write_cmos_sensor(0x3509, (reg_gain&0xFF));
	return gain;
}

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

#if 1
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
						imgsensor_info.i2c_speed);
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
    table_write_cmos_sensor(ov16a1q_init_setting,
        sizeof(ov16a1q_init_setting) / sizeof(kal_uint16));
}	/*	sensor_init  */

static void preview_setting(void)
{
    LOG_INF("loading preview_setting  \n");
    table_write_cmos_sensor(ov16a1q_preview_setting,
        sizeof(ov16a1q_preview_setting) / sizeof(kal_uint16));
}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
    LOG_INF("loading capture_setting  \n");
    table_write_cmos_sensor(ov16a1q_capture_setting,
        sizeof(ov16a1q_capture_setting) / sizeof(kal_uint16));
}

static void normal_video_setting(void)
{
    LOG_INF("E\n");
    table_write_cmos_sensor(ov16a1q_normal_video_setting,
        sizeof(ov16a1q_normal_video_setting) / sizeof(kal_uint16));
}

static void hs_video_setting(void)
{
    LOG_INF("E\n");
    table_write_cmos_sensor(ov16a1q_hs_video_setting,
        sizeof(ov16a1q_hs_video_setting) / sizeof(kal_uint16));
}

static void slim_video_setting(void)
{
    LOG_INF("E\n");
    table_write_cmos_sensor(ov16a1q_slim_video_setting,
        sizeof(ov16a1q_slim_video_setting) / sizeof(kal_uint16));
}

static void custom1_setting(void)
{
    LOG_INF("E\n");
    table_write_cmos_sensor(ov16a1q_custom1_setting,
        sizeof(ov16a1q_custom1_setting) / sizeof(kal_uint16));
}

static void custom3_setting(void)
{
	LOG_INF("E\n");

	table_write_cmos_sensor(ov16a1q_custom3_setting,
			sizeof(ov16a1q_custom3_setting) / sizeof(kal_uint16));
}

static kal_uint32 custom3(
	MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT * image_window,
	MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	printk("loading custom3_setting\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
	imgsensor.pclk = imgsensor_info.custom3.pclk;
	/* imgsensor.video_mode = KAL_TRUE; */
	imgsensor.line_length = imgsensor_info.custom3.linelength;
	imgsensor.frame_length = imgsensor_info.custom3.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/* imgsensor.current_fps = 600; */
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom3_setting();

	return ERROR_NONE;
}	/*      custom3       */

#define OV16A1Q_EEPROM_SLAVE_ADDR 0xA2
#define OV16A1Q_SENSOR_IIC_SLAVE_ADDR 0x20
#define OV16A1Q_EEPROM_SIZE  0x18F3
#define EEPROM_DATA_PATH "/data/vendor/camera_dump/mot_ov16a1q_eeprom.bin"
#define OV16A1Q_EEPROM_CRC_AWB_CAL_SIZE 43
#define OV16A1Q_EEPROM_CRC_LSC_SIZE 1868
#define OV16A1Q_EEPROM_CRC_MANUFACTURING_SIZE 37

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
static void ov16a1q_eeprom_get_3aInfo_data(void *data,
mot_calibration_3aInfo_t *calibration_3aInfo)
{
	struct ov16a1q_eeprom_t *eeprom = (struct ov16a1q_eeprom_t*)data;
	calibration_3aInfo->cie_src_1_ev = to_uint16_swap(eeprom->cie_src_1_ev);
	calibration_3aInfo->cie_src_1_u = to_uint16_swap(eeprom->cie_src_1_u);
	calibration_3aInfo->cie_src_1_v = to_uint16_swap(eeprom->cie_src_1_v);
	calibration_3aInfo->awb_src_1_golden_r = to_uint16_swap(eeprom->awb_src_1_golden_r);
	calibration_3aInfo->awb_src_1_golden_gr = to_uint16_swap(eeprom->awb_src_1_golden_gr);
	calibration_3aInfo->awb_src_1_golden_gb = to_uint16_swap(eeprom->awb_src_1_golden_gb);
	calibration_3aInfo->awb_src_1_golden_b = to_uint16_swap(eeprom->awb_src_1_golden_b);
	calibration_3aInfo->awb_src_1_r = to_uint16_swap(eeprom->awb_src_1_r);
	calibration_3aInfo->awb_src_1_gr = to_uint16_swap(eeprom->awb_src_1_gr);
	calibration_3aInfo->awb_src_1_gb = to_uint16_swap(eeprom->awb_src_1_gb);
	calibration_3aInfo->awb_src_1_b = to_uint16_swap(eeprom->awb_src_1_b);
	calibration_3aInfo->awb_src_1_rg_ratio = to_uint16_swap(eeprom->awb_src_1_rg_ratio);
	calibration_3aInfo->awb_src_1_bg_ratio = to_uint16_swap(eeprom->awb_src_1_bg_ratio);
	calibration_3aInfo->awb_src_1_gr_gb_ratio = to_uint16_swap(eeprom->awb_src_1_gr_gb_ratio);
	calibration_3aInfo->awb_src_1_golden_rg_ratio = to_uint16_swap(eeprom->awb_src_1_golden_rg_ratio);
	calibration_3aInfo->awb_src_1_golden_bg_ratio = to_uint16_swap(eeprom->awb_src_1_golden_bg_ratio);
	calibration_3aInfo->awb_src_1_golden_gr_gb_ratio = to_uint16_swap(eeprom->awb_src_1_golden_gr_gb_ratio);
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
	} else if (eeprom->manufacturer_id[0] == 'T' && eeprom->manufacturer_id[1] == 'S') {
		ret = snprintf(mnf->integrator, MAX_CALIBRATION_STRING, "Tianshi");
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

	if (!eeprom_util_check_crc16(data, OV16A1Q_EEPROM_CRC_MANUFACTURING_SIZE,
		convert_crc(eeprom->manufacture_crc16))) {
		LOG_INF("Manufacturing CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Manufacturing CRC PASS");
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
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
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
	normal_video_setting();

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

static kal_uint32 custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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

	return ERROR_NONE;
}

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

	sensor_resolution->SensorCustom1Width	 = imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height	 = imgsensor_info.custom1.grabwindow_height;

	sensor_resolution->SensorCustom3Width = imgsensor_info.custom3.grabwindow_width;
	sensor_resolution->SensorCustom3Height = imgsensor_info.custom3.grabwindow_height;

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
	sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;
	sensor_info->Custom3DelayFrame = imgsensor_info.custom3_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame;
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
	ov16a1q_eeprom_get_3aInfo_data((void *)ov16a1q_eeprom, &sensor_info->calibration_3aInfo);

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
	case MSDK_SCENARIO_ID_CUSTOM3:
		sensor_info->SensorGrabStartX = imgsensor_info.custom3.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom3.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom3.mipi_data_lp2hs_settle_dc;

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
		custom1(image_window, sensor_config_data);
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		custom3(image_window, sensor_config_data);
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
	case MSDK_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		frame_length = imgsensor_info.custom3.pclk / framerate * 10 / imgsensor_info.custom3.linelength;

		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.custom3.framelength) ? (frame_length - imgsensor_info.custom3.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.custom3.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter) {
			set_dummy();
		} else {
			/*No need to set*/
			pr_debug("frame_length %d < shutter %d", imgsensor.frame_length, imgsensor.shutter);
		}
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
	case MSDK_SCENARIO_ID_CUSTOM1:
		*framerate = imgsensor_info.custom1.max_framerate;
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

static kal_uint32 get_sensor_temperature(void)
{
	UINT8 temperature = 0, temp;
	static kal_uint32 temp_c;
	UINT32 temperature_convert = 0;
	static DEFINE_MUTEX(get_temperature_mutex);

	/*enable temperature TEMP_SEN_CTL */
	mutex_lock(&get_temperature_mutex);
	temp = read_cmos_sensor(0x4d12);
	write_cmos_sensor(0x4d12, temp | 0x01);

	temperature = read_cmos_sensor(0x4d13);
	mutex_unlock(&get_temperature_mutex);

	if (temperature > 0 && temperature < 85) {
		temperature_convert = temperature;
		temp_c = temperature_convert;
	} else {
		temperature_convert = temp_c;//using last normal temperature
	}

	LOG_INF("temp_c(%d), read_reg(%d)\n", temperature_convert, temperature);
	return temperature_convert;
}

static void set_shutter_frame_length(kal_uint16 shutter,
			kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	/*Change frame time*/
       if (frame_length > 1)
	    dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;

	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length -
		imgsensor_info.margin)) ? (imgsensor_info.max_frame_length -
		imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk /
			imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			// Extend frame length
			write_cmos_sensor(0x3840, (imgsensor.frame_length >> 16) & 0xFF);
			write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
			write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		}
	} else {
		// Extend frame length
		write_cmos_sensor(0x3840, (imgsensor.frame_length >> 16) & 0xFF);
		write_cmos_sensor(0x380e, (imgsensor.frame_length >> 8) & 0xFF);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
	}

	// Update Shutter
	write_cmos_sensor(0x3500, (shutter >> 16) & 0xFF);
	write_cmos_sensor(0x3502, (shutter) & 0xFF);
	write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
	LOG_INF("Add for N3D! shutter =%d, framelength =%d\n",shutter, imgsensor.frame_length);
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
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom1.framelength << 16)
				+ imgsensor_info.custom1.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom3.framelength << 16)
				+ imgsensor_info.custom3.linelength;
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
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= imgsensor_info.custom1.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= imgsensor_info.custom3.pclk;
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
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_32 = get_sensor_temperature();
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
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[5], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[6], sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
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
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16)(*feature_data),
			(UINT16)(*(feature_data + 1)));
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
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*feature_return_para_32 = 2; /*BINNING_AVERAGED*/
			break;
		default:
			*feature_return_para_32 = 1; /*BINNING_AVERAGED*/
			break;
		}
		LOG_INF("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
		*feature_para_len = 4;

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
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.custom1.pclk /
			(imgsensor_info.custom1.linelength - 80))*
			imgsensor_info.custom1.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
			(imgsensor_info.custom3.pclk /
			(imgsensor_info.custom3.linelength - 80))*
			imgsensor_info.custom3.grabwindow_width;

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
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
				imgsensor_info.custom1.mipi_pixel_rate;
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

UINT32 MOT_MILAN_OV16A1Q_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
    LOG_INF("MOT_MILAN_OV16A1Q_MIPI_RAW_SensorInit in\n");
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}	/*	MOT_MILAN_OV16A1Q_MIPI_RAW_SensorInit	*/
