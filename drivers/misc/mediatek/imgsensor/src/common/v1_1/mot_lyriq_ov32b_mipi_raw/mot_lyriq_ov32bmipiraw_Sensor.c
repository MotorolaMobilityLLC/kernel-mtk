/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
  ****************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
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
#include "mot_lyriq_ov32bmipiraw_Sensor.h"
#include "mot_lyriq_ov32b_eeprom.h"
#define PFX "[imgsensor] mot_lyriq_ov32bmipiraw"

static int mot_sensor_debug = 0;
module_param(mot_sensor_debug, int, S_IRWXU);

#define LOG_INF(format, args...)        do { if (mot_sensor_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)
#define LOG_DEBUG(format, args...)        do { if (mot_sensor_debug   ) { pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args); } } while(0)

#define LOG_INF_N(format, args...) LOG_ERR(PFX "[%s %d] " format, __func__, __LINE__, ##args)
#define LOG_ERR(format, args...) pr_err(PFX "[%s %d] " format, __func__, __LINE__, ##args)

#define I2C_ADDR (0x20)
#define MULTI_WRITE_REGISTER_VALUE  (8)
#if MULTI_WRITE_REGISTER_VALUE == 8
#define I2C_BUFFER_LEN 765
#elif MULTI_WRITE_REGISTER_VALUE == 16
#define I2C_BUFFER_LEN 1020
#endif

#if 0
#define EEPROM_DATA_PATH "/data/vendor/camera_dump/lyriq_ov32b_eeprom_data.bin"
#define SERIAL_FRONT_DATA_PATH "/data/vendor/camera_dump/serial_number_front.bin"

static const char *lyriq_ov32b_dump_file[2] = {EEPROM_DATA_PATH, SERIAL_FRONT_DATA_PATH};

static ov_cross_talk_cal_t ov_cross_talk_data = {{0}};

static mot_calibration_info_t lyriq_ov32b_cal_info = {0};

int imgread_cam_cal_data(int sensorid, const char **dump_file, mot_calibration_info_t *mot_cal_info);

int get_ov_cross_talk_data(uint8_t eeprom_i2c_addr, uint32_t cross_talk_data_start_addr,
				uint32_t cross_talk_data_size, ov_cross_talk_cal_t *p_cross_talk_data);
#endif
static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_LYRIQ_OV32B_SENSOR_ID,
	.checksum_value = 0x30a07776,
	.pre = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.max_framerate = 300,
		.mipi_pixel_rate = 331740000,
		.mipi_data_lp2hs_settle_dc = 85,
	},
	.cap = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.max_framerate = 300,
		.mipi_pixel_rate = 331740000,
		.mipi_data_lp2hs_settle_dc = 85,
	},
	.normal_video = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.max_framerate = 300,
		.mipi_pixel_rate = 331740000,
		.mipi_data_lp2hs_settle_dc = 85,
	},
	.hs_video = {
		.pclk = 115200000,
		.linelength = 360,
		.framelength = 2666,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1632,
		.grabwindow_height = 1224,
		.mipi_pixel_rate = 658100000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3264,
		.grabwindow_height = 2448,
		.max_framerate = 300,
		.mipi_pixel_rate = 331740000,
		.mipi_data_lp2hs_settle_dc = 85,
	},
	.custom1 = {
		.pclk = 115200000,
		.linelength = 720,
		.framelength = 2664,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.max_framerate = 600,
		.mipi_pixel_rate = 662400000,
		.mipi_data_lp2hs_settle_dc = 85,
	},
	.custom2 = {
		.pclk = 115200000,
		.linelength = 360,
		.framelength = 1332,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1632,
		.grabwindow_height = 1224,
		.mipi_pixel_rate = 663490000,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 2400,
	},
	.custom3 = {
		.pclk = 115200000,
		.linelength = 1440,
		.framelength = 5332,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 6528,
		.grabwindow_height = 4896,
		.max_framerate = 150,
		.mipi_pixel_rate = 662400000,
		.mipi_data_lp2hs_settle_dc = 85,
	},

	.margin = 31,
	.min_shutter = 16, //16,
	.min_gain = 64, //1x
	.max_gain = 1020, //15.9xxx
	.min_gain_iso = 100,
	.gain_step = 1,  //type1 step=1
	.gain_type = 1, //ov
	.max_frame_length = 0xffffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 8,
	.cap_delay_frame = 2,
	.pre_delay_frame = 2,
	.video_delay_frame = 2,
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,
	.custom1_delay_frame = 2,
	.custom2_delay_frame = 2,
	.custom3_delay_frame = 2,
	.frame_time_delay_frame = 1,
	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = 0,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {I2C_ADDR,0xff},
	.i2c_speed = 1000,
};

static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[] = {
{6528, 4896,    0,    0, 6528, 4896, 3264, 2448, 0, 0, 3264, 2448, 0, 0, 3264, 2448},//pre
{6528, 4896,    0,    0, 6528, 4896, 3264, 2448, 0, 0, 3264, 2448, 0, 0, 3264, 2448},//cap
{6528, 4896,    0,    0, 6528, 4896, 3264, 2448, 0, 0, 3264, 2448, 0, 0, 3264, 2448},//video
{6528, 4896,    0,    0, 6528, 4896, 1632, 1224, 0, 0, 1632, 1224, 0, 0, 1632, 1224},//hs_video
{6528, 4896,    0,    0, 6528, 4896, 3264, 2448, 0, 0, 3264, 2448, 0, 0, 3264, 2448},//slim_video
{6528, 4896, 1344, 1368, 3840, 2160, 1920, 1080, 0, 0, 1920, 1080, 0, 0, 1920, 1080},//custom1
{6528, 4896,    0,    0, 6528, 4896, 1632, 1224, 0, 0, 1632, 1224, 0, 0, 1632, 1224},//custom2
{6528, 4896,    0,    0, 6528, 4896, 6528, 4896, 0, 0, 6528, 4896, 0, 0, 6528, 4896},//custom3
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,
	.gain = 0x100,
	.dummy_pixel = 0,
	.dummy_line = 0,
	.current_fps = 300,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_mode = 0,
	.i2c_write_id = I2C_ADDR,
	.current_ae_effective_frame = 2,
};

static kal_uint16 sensor_init_setting_array[] = {
#include "setting/OV32B40_4cell_dphy_24M_init.h"
};

static kal_uint16 preview_setting_array[] = {
#include "setting/OV32B40_3264X2448_MIPI828_30FPS_NORMAL_noPD.h"
};

static kal_uint16 capture_setting_array[] = {
#include "setting/OV32B40_3264X2448_MIPI828_30FPS_NORMAL_noPD.h"
};

static kal_uint16 normal_video_setting_array[] = {
#include "setting/OV32B40_3264X2448_MIPI828_30FPS_NORMAL_noPD.h"
};

static kal_uint16 hs_video_setting_array[] = {
#include "setting/OV32B40_1632X1224_MIPI1656_120FPS_1080P_noPD.h"
};

static kal_uint16 slim_video_setting_array[] = {
#include "setting/OV32B40_3264X2448_MIPI828_30FPS_NORMAL_noPD.h"
};

static kal_uint16 custom1_setting_array[] = {
#include "setting/OV32B40_1920X1080_MIPI1656_60FPS_1080P_noPD.h"
};

static kal_uint16 custom2_setting_array[] = {
#include "setting/OV32B40_1632X1224_MIPI1656_240FPS_720P_noPD.h"
};

static kal_uint16 custom3_setting_array[] = {
#include "setting/OV32B40_6528X4896_MIPI1656_15FPS_FULLSIZE_noPD.h"
};
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pusendcmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static kal_uint16 read_cmos_sensor_8(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2CTiming(pusendcmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
	return get_byte;
}

static void write_cmos_sensor_8(kal_uint32 addr, kal_uint8 para)
{
	char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF),
			(char)(para & 0xFF)};

	iWriteRegI2CTiming(pusendcmd, 3, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint8 para)
{
	char pusendcmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF),
			(char)(para & 0xFF)};

	iWriteRegI2CTiming(pusendcmd, 3, imgsensor.i2c_write_id,imgsensor_info.i2c_speed);
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
			#if MULTI_WRITE_REGISTER_VALUE == 16
				puSendCmd[tosend++] = (char)(data >> 8);
			#endif
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;
		}

		#if MULTI_WRITE_REGISTER_VALUE == 8
			/* Write when remain buffer size is less than 3 bytes
			 * or reach end of data
			 */
			if ((I2C_BUFFER_LEN - tosend) < 3 || IDX == len || addr != addr_last) {
				iBurstWriteReg_multi(puSendCmd,
							tosend,
							imgsensor.i2c_write_id,
							3,
							imgsensor_info.i2c_speed);
				tosend = 0;
			}

		#elif MULTI_WRITE_REGISTER_VALUE == 16
			if ((I2C_BUFFER_LEN - tosend) < 4 || len == IDX || addr != addr_last) {
				iBurstWriteReg_multi(puSendCmd,
					tosend,
					imgsensor.i2c_write_id,
					4, imgsensor_info.i2c_speed);

				tosend = 0;
			}

		#else
		/*just for debug*/
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

static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF_N("image_mirror = %d\n", image_mirror);
	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor_8(0x3820,
			((read_cmos_sensor_8(0x3820) & 0xF9) | 0x00));
		write_cmos_sensor_8(0x3821,
			((read_cmos_sensor_8(0x3821) & 0xF9) | 0x00));
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor_8(0x3820,
			((read_cmos_sensor_8(0x3820) & 0xF9) | 0x04));
		write_cmos_sensor_8(0x3821,
			((read_cmos_sensor_8(0x3821) & 0xF9) | 0x00));
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor_8(0x3820,
			((read_cmos_sensor_8(0x3820) & 0xF9) | 0x00));
		write_cmos_sensor_8(0x3821,
			((read_cmos_sensor_8(0x3821) & 0xF9) | 0x04));
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor_8(0x3820,
			((read_cmos_sensor_8(0x3820) & 0xF9) | 0x04));
		write_cmos_sensor_8(0x3821,
			((read_cmos_sensor_8(0x3821) & 0xF9) | 0x04));
		break;
	}
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);
	return ERROR_NONE;
}

static kal_int32 get_sensor_temperature(void)
{

	return 0;
}

static void set_dummy(void)
{
	LOG_DEBUG("dummyline = %d, dummypixels = %d\n",	imgsensor.dummy_line, imgsensor.dummy_pixel);
	write_cmos_sensor_8(0x380c, imgsensor.line_length >> 8);
	write_cmos_sensor_8(0x380d, imgsensor.line_length & 0xFF);
	write_cmos_sensor_8(0x380e, imgsensor.frame_length >> 8);
	write_cmos_sensor_8(0x380f, imgsensor.frame_length & 0xFF);

}

static inline void extend_frame_length(kal_uint32 frame_length)
{
	/* Extend frame length*/
	write_cmos_sensor_8(0x3840,(frame_length >> 16) & 0xFF); //0x7f
	write_cmos_sensor_8(0x380e,(frame_length >> 8) & 0xFF); //0x7f
	write_cmos_sensor_8(0x380f,frame_length & 0xFF);
}

static inline void updat_shutter(kal_uint32 shutter)
{

	kal_uint32 long_shutter;
	/*1s=1000000000  tline=12500  1000000000/12500=80000*/
        if(shutter > 70000)
	{
		/* long exposure
		  *ov32b  default binning=2,
		  *In the tuning file camera_AE_custom_transfotm.cpp:
		  *u4Eposuretime = u4Eposuretime / u4BinningSumRatio
		  */
		long_shutter= shutter;
		write_cmos_sensor_8(0x3500, (long_shutter >> 16) & 0xFF);
		write_cmos_sensor_8(0x3501, (long_shutter >> 8) & 0xFF);
		write_cmos_sensor_8(0x3502, (long_shutter) & 0xFF);
        }else{
	    /*undate shutter*/
		write_cmos_sensor_8(0x3500, (shutter >> 16) & 0xFF);
		write_cmos_sensor_8(0x3501, (shutter >> 8) & 0xFF);
		write_cmos_sensor_8(0x3502, (shutter) & 0xFF);
       }
}

static inline void update_gain(kal_uint16 reg_gain)
{
	/*update gain reg*/
	write_cmos_sensor_8(0x3508, (reg_gain >> 8) & 0xFF);
	write_cmos_sensor_8(0x3509, reg_gain & 0xFF);
}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0;

	reg_gain = (gain*4);

	return reg_gain;
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable %d\n", framerate,
		min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	if (frame_length >= imgsensor.min_frame_length)
		imgsensor.frame_length = frame_length;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;

	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en)
		imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}

static void write_shutter(kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;

	LOG_DEBUG("s shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);
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
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			extend_frame_length(imgsensor.frame_length);
		}
	} else {
		extend_frame_length(imgsensor.frame_length);
		LOG_DEBUG("(else)imgsensor.frame_length = %d\n",
			imgsensor.frame_length);
	}

	updat_shutter(shutter);
	LOG_DEBUG("e shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

}

static void set_shutter(kal_uint32 shutter)
{
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}

static void set_shutter_frame_length(kal_uint32 shutter,
				     kal_uint32 frame_length,
				     kal_bool auto_extend_en)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);

	if (frame_length > 1)
	    imgsensor.frame_length = frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin)
		: shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			extend_frame_length(imgsensor.frame_length);
		}
	} else {
		extend_frame_length(imgsensor.frame_length);
	}

	updat_shutter(shutter);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

}

static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	LOG_DEBUG("gain setting");

	if (gain < imgsensor_info.min_gain)
		gain = imgsensor_info.min_gain;
	else if (gain > imgsensor_info.max_gain)
		gain = imgsensor_info.max_gain;

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_DEBUG("gain = %d, reg_gain = 0x%x\n ", gain, reg_gain);

	update_gain(reg_gain);
	return gain;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	unsigned int tmp;

	LOG_INF_N("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
		write_cmos_sensor_8(0x0100, 0x01);
	} else {
		tmp = read_cmos_sensor_8(0x0100);
		if (tmp & 0x01)
			write_cmos_sensor_8(0x0100, 0x00);
	}
	return ERROR_NONE;
}

static void sensor_init(void)
{
	unsigned int tmp;
	LOG_INF_N("E \n");
	tmp = read_cmos_sensor_8(0x0100);
	if (tmp & 0x01)
		write_cmos_sensor_8(0x0100, 0x00);
	table_write_cmos_sensor(sensor_init_setting_array,
		sizeof(sensor_init_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

static void preview_setting(void)
{
	LOG_INF_N("E\n");
	table_write_cmos_sensor(preview_setting_array,
		sizeof(preview_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF_N(" E! currefps:%d\n",  currefps);
	table_write_cmos_sensor(capture_setting_array,
		sizeof(capture_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF_N(" E! currefps:%d\n",  currefps);
	table_write_cmos_sensor(normal_video_setting_array,
		sizeof(normal_video_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

static void hs_video_setting(void)
{
	LOG_INF_N("E\n");
	table_write_cmos_sensor(hs_video_setting_array,
		sizeof(hs_video_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

static void slim_video_setting(void)
{
	LOG_INF_N("E\n");
	table_write_cmos_sensor(slim_video_setting_array,
		sizeof(slim_video_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

static void custom1_setting(void)
{
	LOG_INF_N("E\n");
	table_write_cmos_sensor(custom1_setting_array,
		sizeof(custom1_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

static void custom2_setting(void)
{
	LOG_INF_N("E\n");
	table_write_cmos_sensor(custom2_setting_array,
		sizeof(custom2_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

#if 0
static void write_cross_talk_data(void)
{
	uint16_t write_table[(OV_CROSS_TALK_GROUP_SIZE + 6)*2] = {0};

	LOG_INF_N("E\n");

	memcpy(write_table, &ov_cross_talk_data.height[0].addr, sizeof(write_table));

#if 0 //for debug
	for (i = 0; i < sizeof(write_table)/sizeof(uint16_t); i+=2)
		LOG_INF("0x%04x: 0x%02x", write_table[i], write_table[i+1]);

	LOG_INF("write_table[586]=0x%04x: write_table[587]=0x%02x", write_table[586], write_table[587]);
#endif

	if (ov_cross_talk_data.is_valid != 1)
	{
		LOG_ERR("cross talk data is not valid, applied fail.");
		return;
	}

	table_write_cmos_sensor(write_table,
		sizeof(write_table)/sizeof(uint16_t));

	LOG_INF("apply cross talk calibration data success.");
	LOG_INF_N("X");
}

#endif
static void custom3_setting(void)
{
	LOG_INF_N("E\n");
	table_write_cmos_sensor(custom3_setting_array,
		sizeof(custom3_setting_array)/sizeof(kal_uint16));
	LOG_INF_N("X");
}

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor_8(0x300B) << 8) | read_cmos_sensor_8(0x300C));
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
			LOG_INF("i2c write id: 0x%x, sid:rid 0x%x:0x%x match:%d\n",
				imgsensor.i2c_write_id, imgsensor_info.sensor_id, *sensor_id,
				imgsensor_info.sensor_id == *sensor_id);
			if (*sensor_id == imgsensor_info.sensor_id) {
				// get calibration status and mnf data.
			LYRIQ_OV32B_read_data_from_eeprom(LYRIQ_OV32B_EEPROM_SLAVE_ADDR, 0x0000, LYRIQ_OV32B_EEPROM_SIZE);
			LYRIQ_OV32B_eeprom_format_calibration_data((void *)LYRIQ_OV32B_eeprom);
				return ERROR_NONE;
			}
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

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
			LOG_INF("i2c write id: 0x%x, sid:rid 0x%x:0x%x match:%d\n",
				imgsensor.i2c_write_id, imgsensor_info.sensor_id, sensor_id,
				imgsensor_info.sensor_id == sensor_id);
			if (sensor_id == imgsensor_info.sensor_id) {
				break;
			}
			retry--;
		} while (retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id)
			break;
		retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id)
		return ERROR_SENSOR_CONNECT_FAIL;

	sensor_init();
	set_mirror_flip(imgsensor.mirror);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x200;
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
}

static kal_uint32 close(void)
{
	LOG_INF_N("E\n");

	streaming_control(0);
	return ERROR_NONE;
}

static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("%s E\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor_info.margin = 12;
	imgsensor_info.min_shutter = 4;
	spin_unlock(&imgsensor_drv_lock);

	preview_setting();

	return ERROR_NONE;
}

static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("%s E\n", __func__);

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor_info.margin = 12;
	imgsensor_info.min_shutter = 4;
	spin_unlock(&imgsensor_drv_lock);

	capture_setting(imgsensor.current_fps);

	return ERROR_NONE;
}

static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor_info.margin = 12;
	imgsensor_info.min_shutter = 4;
	spin_unlock(&imgsensor_drv_lock);

	normal_video_setting(imgsensor.current_fps);

	return ERROR_NONE;
}

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor_info.margin = 12;
	imgsensor_info.min_shutter = 4;
	spin_unlock(&imgsensor_drv_lock);

	hs_video_setting();

	return ERROR_NONE;
}

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor_info.margin = 12;
	imgsensor_info.min_shutter = 4;
	spin_unlock(&imgsensor_drv_lock);

	slim_video_setting();

	return ERROR_NONE;
}

static kal_uint32 custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;
	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor_info.margin = 12;
	imgsensor_info.min_shutter = 4;
	spin_unlock(&imgsensor_drv_lock);

	custom1_setting();

	return ERROR_NONE;
}

static kal_uint32 custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
	imgsensor.pclk = imgsensor_info.custom2.pclk;
	imgsensor.line_length = imgsensor_info.custom2.linelength;
	imgsensor.frame_length = imgsensor_info.custom2.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor_info.margin = 8;
	imgsensor_info.min_shutter = 4;
	spin_unlock(&imgsensor_drv_lock);

	custom2_setting();

	return ERROR_NONE;
}

static kal_uint32 custom3(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
	imgsensor.pclk = imgsensor_info.custom3.pclk;
	imgsensor.line_length = imgsensor_info.custom3.linelength;
	imgsensor.frame_length = imgsensor_info.custom3.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor_info.margin = 24;
	imgsensor_info.min_shutter = 8;
	spin_unlock(&imgsensor_drv_lock);

	custom3_setting();

	// apply cross talk calibration data after resolution settings.
//	write_cross_talk_data();
	mdelay(100);

	return ERROR_NONE;
}

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF_N("E\n");

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

	sensor_resolution->SensorHighSpeedVideoWidth =
		imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight =
		imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth =
		imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight =
		imgsensor_info.slim_video.grabwindow_height;

	sensor_resolution->SensorCustom1Width =
		imgsensor_info.custom1.grabwindow_width;
	sensor_resolution->SensorCustom1Height =
		imgsensor_info.custom1.grabwindow_height;

	sensor_resolution->SensorCustom2Width =
		imgsensor_info.custom2.grabwindow_width;
	sensor_resolution->SensorCustom2Height =
		imgsensor_info.custom2.grabwindow_height;

	sensor_resolution->SensorCustom3Width =
		imgsensor_info.custom3.grabwindow_width;
	sensor_resolution->SensorCustom3Height =
		imgsensor_info.custom3.grabwindow_height;

	return ERROR_NONE;
}

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			   MSDK_SENSOR_INFO_STRUCT *sensor_info,
			   MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame =
		imgsensor_info.slim_video_delay_frame;
	sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;
	sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame;
	sensor_info->Custom3DelayFrame = imgsensor_info.custom3_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->FrameTimeDelayFrame =
		imgsensor_info.frame_time_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->TEMPERATURE_SUPPORT = imgsensor_info.temperature_support;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 0;
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0; /* 0 is default 1x */
	sensor_info->SensorHightSampling = 0; /* 0 is default 1x */
	sensor_info->SensorPacketECCOrder = 1;
#if 1
	sensor_info->calibration_status.mnf   = mnf_status;
	sensor_info->calibration_status.af    = af_status;
	sensor_info->calibration_status.awb   = awb_status;
	sensor_info->calibration_status.lsc   = lsc_status;
	sensor_info->calibration_status.pdaf  = pdaf_status;
	sensor_info->calibration_status.dual  = dual_status;

	LYRIQ_OV32B_eeprom_get_mnf_data((void *) LYRIQ_OV32B_eeprom, &sensor_info->mnf_calibration);
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

		sensor_info->SensorGrabStartX =
			imgsensor_info.normal_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.normal_video.starty;

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
		sensor_info->SensorGrabStartX =
			imgsensor_info.slim_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.slim_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

		break;

	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
		break;

	case MSDK_SCENARIO_ID_CUSTOM2:
		sensor_info->SensorGrabStartX = imgsensor_info.custom2.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom2.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom2.mipi_data_lp2hs_settle_dc;
		break;

	case MSDK_SCENARIO_ID_CUSTOM3:
		sensor_info->SensorGrabStartX = imgsensor_info.custom3.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.custom3.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom3.mipi_data_lp2hs_settle_dc;
		break;

	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}

	return ERROR_NONE;
}

static const char *mot_lyriq_ov32b_scenario_to_name(enum MSDK_SCENARIO_ID_ENUM scenario_id)
{
	const char *pScenarioName[] = {
		"MSDK_SCENARIO_ID_CAMERA_PREVIEW",
		"MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG",
		"MSDK_SCENARIO_ID_VIDEO_PREVIEW",
		"MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO",
		"MSDK_SCENARIO_ID_SLIM_VIDEO",
		"MSDK_SCENARIO_ID_CUSTOM1",
		"MSDK_SCENARIO_ID_CUSTOM2",
		"MSDK_SCENARIO_ID_CUSTOM3",
		"MSDK_SCENARIO_ID_CUSTOM4",
		"MSDK_SCENARIO_ID_CUSTOM5",
		"MSDK_SCENARIO_ID_MAX",
	};

	if (scenario_id >= MSDK_SCENARIO_ID_CAMERA_PREVIEW && scenario_id <= MSDK_SCENARIO_ID_MAX)
		return pScenarioName[scenario_id];
	else
		return "SCENARIO_UNKONWN";
}

static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF_N("scenario(%d): %s\n", scenario_id, mot_lyriq_ov32b_scenario_to_name(scenario_id));
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
	case MSDK_SCENARIO_ID_CUSTOM2:
		custom2(image_window, sensor_config_data);
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
}

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
	LOG_DEBUG("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)
		imgsensor.autoflicker_en = KAL_TRUE;
	else
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk / framerate * 10
				/ imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.pre.framelength)
		? (frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk /
				framerate * 10 /
				imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.normal_video.framelength)
		? (frame_length - imgsensor_info.normal_video.framelength)
		: 0;
		imgsensor.frame_length =
			imgsensor_info.normal_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		frame_length = imgsensor_info.cap.pclk / framerate * 10
				/ imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap.framelength)
			  ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length =
				imgsensor_info.cap.framelength
				+ imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);

		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10
				/ imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.hs_video.framelength)
			  ? (frame_length - imgsensor_info.hs_video.framelength)
			  : 0;
		imgsensor.frame_length =
			imgsensor_info.hs_video.framelength
				+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10
			/ imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.slim_video.framelength)
			? (frame_length - imgsensor_info.slim_video.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.slim_video.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM1:
		frame_length = imgsensor_info.custom1.pclk / framerate * 10
				/ imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom1.framelength)
			? (frame_length - imgsensor_info.custom1.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.custom1.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		frame_length = imgsensor_info.custom2.pclk / framerate * 10
				/ imgsensor_info.custom2.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom2.framelength)
			? (frame_length - imgsensor_info.custom2.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.custom2.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		frame_length = imgsensor_info.custom3.pclk / framerate * 10
				/ imgsensor_info.custom3.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.custom3.framelength)
			? (frame_length - imgsensor_info.custom3.framelength)
			: 0;
		imgsensor.frame_length =
			imgsensor_info.custom3.framelength
			+ imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	default:
		frame_length = imgsensor_info.pre.pclk / framerate * 10
			/ imgsensor_info.pre.linelength;
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
		LOG_INF("error scenario_id = %d, we use preview scenario\n",
			scenario_id);
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(
		enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{

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

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				 UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data
		= (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_DEBUG("feature_id = %d\n", feature_id);
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
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom2.pclk;
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
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_OFFSET_TO_START_OF_EXPOSURE:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 3000000;
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
		 /* night_mode((BOOL) *feature_data); */
		break;
	#ifdef VENDOR_EDIT
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
		write_cmos_sensor(sensor_reg_data->RegAddr,
				    sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/*get the lens driver ID from EEPROM
		 * or just return LENS_DRIVER_ID_DO_NOT_CARE
		 * if EEPROM does not exist in camera module.
		 */
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
		/* for factory mode auto testing */
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
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
	#if 0
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
			(UINT32)*feature_data);
	#endif
		wininfo =
	(struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

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

	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_32 = get_sensor_temperature();
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
		LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data,
			(UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) (*feature_data),
					(UINT16) (*(feature_data + 1)),
					(BOOL) (*(feature_data + 2)));
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
	case SENSOR_FEATURE_SET_AWB_GAIN:
		break;
	case SENSOR_FEATURE_SET_HDR_SHUTTER:
		LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1));
		#if 0
		ihdr_write_shutter((UINT16)*feature_data,
				   (UINT16)*(feature_data+1));
		#endif
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n",
			*feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
			case MSDK_SCENARIO_ID_CUSTOM3:
				*feature_return_para_32 = 1000; /*BINNING_NONE*/
				break;
			default:
				*feature_return_para_32 = 1310; /*BINNING_AVERAGED*/
				break;
		}
		pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_AE_EFFECTIVE_FRAME_FOR_LE:
		*feature_return_para_32 = imgsensor.current_ae_effective_frame;
		break;
	case SENSOR_FEATURE_GET_AE_FRAME_MODE_FOR_LE:
		memcpy(feature_return_para_32,
		&imgsensor.ae_frm_mode, sizeof(struct IMGSENSOR_AE_FRM_MODE));
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
	{
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom2.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom3.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.mipi_pixel_rate;
			break;
		}
	}
	break;
	default:
		break;
	}

	return ERROR_NONE;
}

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 MOT_LYRIQ_OV32B_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}
