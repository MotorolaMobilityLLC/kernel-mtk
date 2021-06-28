/*****************************************************************************
 *
 *Filename:
 *---------
 *   mot_coful_s5kjn1_qtech.c
 *
 *Project:
 *--------
 *   ALPS MT6768
 *
 *Description:
 *------------
 *   Source code of Sensor driver
 *------------------------------------------------------------------------------
 *Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
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

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_typedef.h"
#include "mot_coful_s5kjn1_qtech.h"

#define FPTPDAFSUPPORT

#define MULTI_WRITE 1

#define PFX "mot_coful_s5kjn1_qtech"

#define LOG_INF(format, args...)    pr_err(PFX "[%s] " format, __func__, ##args)

#define EEPROM_DATA_PATH "/data/vendor/camera_dump/s5kjn1_eeprom_data.bin"
#define SERIAL_MAIN_DATA_PATH "/data/vendor/camera_dump/serial_number_main.bin"

static const char *s5kjn1_dump_file[2] = {EEPROM_DATA_PATH, SERIAL_MAIN_DATA_PATH};
static mot_calibration_info_t s5kjn1_cal_info = {0};
int imgread_cam_cal_data(int sensorid, const char **dump_file, mot_calibration_info_t *mot_cal_info);
int flash_mode = 0;

static DEFINE_SPINLOCK(imgsensor_drv_lock);
static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = MOT_COFUL_S5KJN1_QTECH_ID,
	.checksum_value = 0x42d95d37,
	.pre = {
		.pclk = 560000000,
		.linelength = 5910,
		.framelength = 3156,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 494400000,
		.max_framerate = 300,
	},
	.cap = {
		.pclk = 560000000,
		.linelength = 5910,
		.framelength = 3156,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 494400000,
		.max_framerate = 300,
	},
	.normal_video = {
		.pclk = 560000000,
		.linelength = 5910,
		.framelength = 3156,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 494400000,
		.max_framerate = 300,
	},
	.hs_video = {
		.pclk = 600000000,
		.linelength = 2096,
		.framelength = 2380,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1280,
		.grabwindow_height = 720,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 640000000,
		.max_framerate = 1200,
	},
	.slim_video = {
		.pclk = 560000000,
		.linelength = 5910,
		.framelength = 3156,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 494400000,
		.max_framerate = 300,
	},
	.custom1 = {
		.pclk = 600000000,
		.linelength = 2116,
		.framelength = 4720,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1920,
		.grabwindow_height = 1080,
		.mipi_data_lp2hs_settle_dc = 85,
		.mipi_pixel_rate = 640000000,
		.max_framerate = 600,
	},
	.margin = 10,
	.min_shutter = 2,
	.max_frame_length = 0xFFFF,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.frame_time_delay_frame = 1,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 7,
	.cap_delay_frame = 2,
	.pre_delay_frame = 2,
	.video_delay_frame = 2,
	.hs_video_delay_frame = 2,
	.slim_video_delay_frame = 2,
	.custom1_delay_frame = 2,
	.isp_driving_current = ISP_DRIVING_4MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = 1,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = {0xAC, 0xff},
	.i2c_speed = 1000,
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_HV_MIRROR,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x0200,
	.gain = 0x0100,
	.dummy_pixel = 0,
	.dummy_line = 0,
	.current_fps = 0,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = KAL_FALSE,
	.i2c_write_id = 0xAC,
//cxc long exposure >
	.current_ae_effective_frame = 2,
//cxc long exposure <
};

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {

	{
		0x02, 0x0A, 0x00, 0x08, 0x40, 0x00,
		0x00, 0x2B, 0x07D0, 0x05DC, 0x01, 0x00, 0x0000, 0x0000,
		0x01, 0x30, 0x027C, 0x0BF0, 0x03, 0x00, 0x0000, 0x0000
	},

	{
		0x02, 0x0A, 0x00, 0x08, 0x40, 0x00,
		0x00, 0x2B, 0x07D0, 0x05DC, 0x01, 0x00, 0x0000, 0x0000,
		0x01, 0x30, 0x027C, 0x0BF0, 0x03, 0x00, 0x0000, 0x0000
	},

	{
		0x02, 0x0A, 0x00, 0x08, 0x40, 0x00,
		0x00, 0x2B, 0x0FA0, 0x0BB8, 0x01, 0x00, 0x0000, 0x0000,
		0x01, 0x30, 0x027C, 0x0BF0, 0x03, 0x00, 0x0000, 0x0000
	}
};

static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[6] = {
	{8160, 6144, 0, 0, 8160, 6144, 4080, 3072, 0, 0, 4080, 3072,0, 0, 4080, 3072},
	{8160, 6144, 0, 0, 8160, 6144, 4080, 3072, 0, 0, 4080, 3072,0, 0, 4080, 3072},
	{8160, 6144, 0, 0, 8160, 6144, 4080, 3072, 0, 0, 4080, 3072,0, 0, 4080, 3072},
	{8160, 6144, 240, 912, 7680, 4320, 1280, 720, 0, 0, 1280, 720,0, 0, 1280, 720},
	{8160, 6144, 0, 0, 8160, 6144, 4080, 3072, 0, 0, 4080, 3072,0, 0, 4080, 3072},
	{8160, 6144, 240, 912, 7680, 4320, 1920, 1080, 0, 0, 1920, 1080,0, 0, 1920, 1080},

};

static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	.i4OffsetX = 8,
	.i4OffsetY = 8,
	.i4PitchX = 8,
	.i4PitchY = 8,
	.i4PairNum = 4,
	.i4SubBlkW = 8,
	.i4SubBlkH = 2,
	.i4PosL = {{11, 8}, {9, 11}, {13, 12}, {15, 15}},
	.i4PosR = {{10, 8}, {8, 11}, {12, 12}, {14, 15}},
	.iMirrorFlip = IMAGE_HV_MIRROR,
	.i4BlockNumX = 508,
	.i4BlockNumY = 382,
};

#if MULTI_WRITE
#define I2C_BUFFER_LEN 765
#else
#define I2C_BUFFER_LEN 3
#endif
static kal_uint16 mot_coful_s5kjn1_qtech_table_write_cmos_sensor(kal_uint16 *para, kal_uint32 len)
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
                if ((I2C_BUFFER_LEN - tosend) < 4 ||
                        len == IDX ||
                        addr != addr_last) {
                        iBurstWriteReg_multi(puSendCmd, tosend,
                                imgsensor.i2c_write_id,
                                4, imgsensor_info.i2c_speed);

                        tosend = 0;
                }
#else
                iWriteRegI2C(puSendCmd, 4, imgsensor.i2c_write_id);
                tosend = 0;

#endif
     }
            return 0;
}

static kal_uint16 read_cmos_sensor_8(kal_uint16 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *) &get_byte, 1,
				imgsensor.i2c_write_id);
	return get_byte;
}

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *) &get_byte, 1,
				imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor_byte(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {
	 (char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF) };

	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {
		(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8),
		(char)(para & 0xFF)
	};

	iWriteRegI2C(pusendcmd, 4, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d\n", imgsensor.dummy_line,
			imgsensor.dummy_pixel);

	write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	write_cmos_sensor(0x0342, imgsensor.line_length & 0xFFFF);
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable(%d)\n",
			framerate, min_framelength_en);
	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length =
		(frame_length >
		 imgsensor.min_frame_length)
		  ? frame_length : imgsensor.min_frame_length;
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
}

static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable) {
		write_cmos_sensor(0x3C1E, 0x0100);
		write_cmos_sensor_byte(0x0100, 0x01);
		write_cmos_sensor(0x3C1E, 0x0000);
	} else {
		write_cmos_sensor_byte(0x0100, 0x00);
	}
	return ERROR_NONE;
}

//cxc long exposure >
static bool bNeedSetNormalMode = KAL_FALSE;
#define SHUTTER_1		95932//30396
#define SHUTTER_2		191864//60792
#define SHUTTER_4		383729//56049
#define SHUTTER_8		767459//46563
#define SHUTTER_16		1534919//27591
#define SHUTTER_32		3069838

//cxc long exposure <

/*************************************************************************
*FUNCTION
*  set_shutter
*
*DESCRIPTION
*  This function set e-shutter of sensor to change exposure time.
*
*PARAMETERS
*  iShutter : exposured lines
*
*RETURNS
*  None
*
*GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint32 shutter)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_uint32 pre_shutter = 2877;

	LOG_INF("cxc enter  shutter = %d\n", shutter);

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	if (shutter < 94755) {
		if (bNeedSetNormalMode) {
			LOG_INF("exit long shutter\n");
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000); //stream off
			write_cmos_sensor(0x0340, 0x0c54);
			write_cmos_sensor(0x0342, 0x0716);
			write_cmos_sensor(0x0202, 0x0100);
			write_cmos_sensor(0x0702, 0x0000);
			write_cmos_sensor(0x0704, 0x0000);
			write_cmos_sensor(0x0100, 0x0100);

			bNeedSetNormalMode = KAL_FALSE;
			imgsensor.current_ae_effective_frame = 2;
		}
		pre_shutter = shutter;

		spin_lock(&imgsensor_drv_lock);
		if (shutter > imgsensor.min_frame_length -
				imgsensor_info.margin)
			imgsensor.frame_length = shutter +
				imgsensor_info.margin;
		else
			imgsensor.frame_length = imgsensor.min_frame_length;
		if (imgsensor.frame_length > imgsensor_info.max_frame_length)
			imgsensor.frame_length =
				imgsensor_info.max_frame_length;
		spin_unlock(&imgsensor_drv_lock);
		shutter =
			(shutter < imgsensor_info.min_shutter)
			 ? imgsensor_info.min_shutter : shutter;
		shutter =
			(shutter >
			 (imgsensor_info.max_frame_length -
			 imgsensor_info.margin)) ?
			 (imgsensor_info.max_frame_length -
			 imgsensor_info.margin) : shutter;
		if (imgsensor.autoflicker_en) {
			realtime_fps =
				imgsensor.pclk / imgsensor.line_length * 10 /
				imgsensor.frame_length;
			if (realtime_fps >= 297 && realtime_fps <= 305)
				set_max_framerate(296, 0);
			else if (realtime_fps >= 147 && realtime_fps <= 150)
				set_max_framerate(146, 0);
			else {
				write_cmos_sensor(0x0340,
					  imgsensor.frame_length & 0xFFFF);
			}
		} else {

			write_cmos_sensor(0x0340,
					imgsensor.frame_length & 0xFFFF);
		}

		write_cmos_sensor(0X0202, shutter & 0xFFFF);
		LOG_INF("cxc 2 enter  shutter = %d\n", shutter);

	} else {

		LOG_INF("cxc enter long shutter\n");
		bNeedSetNormalMode = KAL_TRUE;
		imgsensor.ae_frm_mode.frame_mode_1 = IMGSENSOR_AE_MODE_SE;
		imgsensor.ae_frm_mode.frame_mode_2 = IMGSENSOR_AE_MODE_SE;
		imgsensor.current_ae_effective_frame = 2;
		write_cmos_sensor(0x6028, 0x4000);
		write_cmos_sensor(0x0100, 0x0000); //stream off
		write_cmos_sensor(0x0340, 0xFBD4);
		write_cmos_sensor(0x0342, 0x21F0);
		write_cmos_sensor(0x0202, 0xFBC8);
		write_cmos_sensor(0x0702, 0x0500);
		write_cmos_sensor(0x0704, 0x0500);
		write_cmos_sensor(0x0100, 0x0100); //stream on

		write_cmos_sensor(0x0E10, pre_shutter); //1st frame
		write_cmos_sensor(0x0E12, imgsensor.gain); //aGain 1st frame

		switch (shutter) {
		case SHUTTER_1:
			LOG_INF("cxc shutter 1\n");
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000); //stream off
			write_cmos_sensor(0x0202, 0xFBC8);
			write_cmos_sensor(0x0340, 0xFBD4);
			write_cmos_sensor(0x0702, 0x0000);
			write_cmos_sensor(0x0704, 0x0000);
			write_cmos_sensor(0x0100, 0x0100); //stream on
			break;
		case SHUTTER_2:
			LOG_INF("cxc shutter 2\n");
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000); //stream off
			write_cmos_sensor(0x0202, 0xFBC8);
			write_cmos_sensor(0x0340, 0xFBD4);
			write_cmos_sensor(0x0702, 0x0100);
			write_cmos_sensor(0x0704, 0x0100);
			write_cmos_sensor(0x0100, 0x0100); //stream on
			break;
		case SHUTTER_4:
			LOG_INF("cxc shutter 4\n");
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000); //stream off
			write_cmos_sensor(0x0202, 0xFBC8);
			write_cmos_sensor(0x0340, 0xFBD4);
			write_cmos_sensor(0x0702, 0x0200);
			write_cmos_sensor(0x0704, 0x0200);
			write_cmos_sensor(0x0100, 0x0100); //stream on
			break;
		case SHUTTER_8:
			LOG_INF("cxc shutter 8\n");
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000); //stream off
			write_cmos_sensor(0x0202, 0xFBC8);
			write_cmos_sensor(0x0340, 0xFBD4);
			write_cmos_sensor(0x0702, 0x0300);
			write_cmos_sensor(0x0704, 0x0300);
			write_cmos_sensor(0x0100, 0x0100); //stream on
			break;
		case SHUTTER_16:
			LOG_INF("cxc shutter 16\n");
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000); //stream off
			write_cmos_sensor(0x0202, 0xFBC8);
			write_cmos_sensor(0x0340, 0xFBD4);
			write_cmos_sensor(0x0702, 0x0400);
			write_cmos_sensor(0x0704, 0x0400);
			write_cmos_sensor(0x0100, 0x0100); //stream on
			break;
		case SHUTTER_32:
			LOG_INF("cxc shutter 32\n");
			write_cmos_sensor(0x6028, 0x4000);
			write_cmos_sensor(0x0100, 0x0000); //stream off
			write_cmos_sensor(0x0202, 0xFBC8);
			write_cmos_sensor(0x0340, 0xFBD4);
			write_cmos_sensor(0x0702, 0x0500);
			write_cmos_sensor(0x0704, 0x0500);
			write_cmos_sensor(0x0100, 0x0100); //stream on
			break;
		default:
			LOG_INF("cxc shutter > 1\n");
			write_cmos_sensor(0x0E14, ((shutter * 0x05DB) /
				SHUTTER_1)); //2nd frame
			break;
		}
		write_cmos_sensor(0x0E16, 0x0050); //aGain 2nd frame
		write_cmos_sensor(0x0704, 0x0600); //shifter for shutter

		write_cmos_sensor(0x0100, 0x0100); //stream on
		pre_shutter = 0;
	}

	//write_cmos_sensor(0X0202, shutter & 0xFFFF);

	LOG_INF("cxc  Exit! shutter =%d, framelength =%d\n", shutter,
			imgsensor.frame_length);
}

/*************************************************************************
*FUNCTION
*  set_shutter_frame_length
*
*DESCRIPTION
*  for frame &3A sync
*
*************************************************************************/

static void
set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;
	kal_bool autoflicker_closed = KAL_FALSE;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);

	dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;

	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter =
		(shutter < imgsensor_info.min_shutter)
		 ? imgsensor_info.min_shutter : shutter;
	shutter =
		(shutter >
		 (imgsensor_info.max_frame_length -
		  imgsensor_info.margin)) ? (imgsensor_info.max_frame_length -
			 imgsensor_info.margin) : shutter;

	if (autoflicker_closed) {
		realtime_fps =
			imgsensor.pclk / imgsensor.line_length * 10 /
			imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else
			write_cmos_sensor(0x0340, imgsensor.frame_length);
	} else
		write_cmos_sensor(0x0340, imgsensor.frame_length);


	write_cmos_sensor(0x0202, imgsensor.shutter);
	LOG_INF
	("Exit! shutter %d framelength %d/%d dummy_line=%d auto_extend=%d\n",
	 shutter, imgsensor.frame_length,
	  frame_length, dummy_line, read_cmos_sensor(0x0350));

}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;

	reg_gain = gain / 2;

	return (kal_uint16) reg_gain;
}

/*************************************************************************
*FUNCTION
*  set_gain
*
*DESCRIPTION
*  This function is to set global gain to sensor.
*
*PARAMETERS
*  iGain : sensor global gain(base: 0x40)
*
*RETURNS
*  the actually gain set to sensor.
*
*GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{

	kal_uint16 reg_gain;

	LOG_INF("set_gain %d\n", gain);
	if (gain < BASEGAIN || gain > 64 * BASEGAIN) {
		LOG_INF("Error gain setting");
		if (gain < BASEGAIN)
			gain = BASEGAIN;
		else if (gain > 64 * BASEGAIN)
			gain = 64 * BASEGAIN;
	}
	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);
	write_cmos_sensor(0x0204, (reg_gain & 0xFFFF));
	return gain;
}

static void set_mirror_flip(kal_uint8 image_mirror)
{
	LOG_INF("image_mirror = %d\n", image_mirror);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.mirror = image_mirror;
	spin_unlock(&imgsensor_drv_lock);
	switch (image_mirror) {
	case IMAGE_NORMAL:
		write_cmos_sensor_byte(0x0101, 0x00);
		break;
	case IMAGE_H_MIRROR:
		write_cmos_sensor_byte(0x0101, 0x01);
		break;
	case IMAGE_V_MIRROR:
		write_cmos_sensor_byte(0x0101, 0x02);
		break;
	case IMAGE_HV_MIRROR:
		write_cmos_sensor_byte(0x0101, 0x03);
		break;
	default:
		LOG_INF("Error image_mirror setting\n");
		break;
	}
}

#if MULTI_WRITE
kal_uint16 addr_data_pair_init_mot_coful_s5kjn1_qtech[] = {
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
	0x602A, 0x1A0E,
	0x6F12, 0x9600,
	0x6028, 0x4000,
	0xF44E, 0x0011,
	0xF44C, 0x0B0B,
	0xF44A, 0x0006,
	0x0118, 0x0002,
	0x011A, 0x0001,
};
#endif


static void sensor_init(void)
{
	LOG_INF("E\n");
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x0000, 0x0002);
	write_cmos_sensor(0x0000, 0x38E1);
	write_cmos_sensor(0x001E, 0x0007);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0x6010, 0x0001);
	mdelay(5);
	write_cmos_sensor(0x6226, 0x0001);
	mdelay(10);
#if MULTI_WRITE
	mot_coful_s5kjn1_qtech_table_write_cmos_sensor(
		addr_data_pair_init_mot_coful_s5kjn1_qtech,
		sizeof(addr_data_pair_init_mot_coful_s5kjn1_qtech) /
		sizeof(kal_uint16));
#else
	write_cmos_sensor(0x6214, 0x7971);
	write_cmos_sensor(0x6218, 0x7150);
	write_cmos_sensor(0x0A70, 0x0001);
	write_cmos_sensor(0x0A72, 0x0100);
	write_cmos_sensor(0x0A02, 0x0074);
	write_cmos_sensor(0x6028, 0x2000);
	write_cmos_sensor(0x602A, 0x106A);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x602A, 0x2BC2);
	write_cmos_sensor(0x6F12, 0x0003);
	write_cmos_sensor(0x602A, 0x3F5C);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0549);
	write_cmos_sensor(0x6F12, 0x0448);
	write_cmos_sensor(0x6F12, 0x054A);
	write_cmos_sensor(0x6F12, 0xC1F8);
	write_cmos_sensor(0x6F12, 0x5005);
	write_cmos_sensor(0x6F12, 0x101A);
	write_cmos_sensor(0x6F12, 0xA1F8);
	write_cmos_sensor(0x6F12, 0x5405);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xCFB9);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x4470);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2E30);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x6E00);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xFF5F);
	write_cmos_sensor(0x6F12, 0xF848);
	write_cmos_sensor(0x6F12, 0x8B46);
	write_cmos_sensor(0x6F12, 0x1746);
	write_cmos_sensor(0x6F12, 0x0068);
	write_cmos_sensor(0x6F12, 0x9A46);
	write_cmos_sensor(0x6F12, 0x4FEA);
	write_cmos_sensor(0x6F12, 0x1049);
	write_cmos_sensor(0x6F12, 0x80B2);
	write_cmos_sensor(0x6F12, 0x8046);
	write_cmos_sensor(0x6F12, 0x0146);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x4846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x02FA);
	write_cmos_sensor(0x6F12, 0xF24D);
	write_cmos_sensor(0x6F12, 0x95F8);
	write_cmos_sensor(0x6F12, 0x6D00);
	write_cmos_sensor(0x6F12, 0x0228);
	write_cmos_sensor(0x6F12, 0x35D0);
	write_cmos_sensor(0x6F12, 0x0224);
	write_cmos_sensor(0x6F12, 0xF04E);
	write_cmos_sensor(0x6F12, 0x5346);
	write_cmos_sensor(0x6F12, 0xB6F8);
	write_cmos_sensor(0x6F12, 0xB802);
	write_cmos_sensor(0x6F12, 0xB0FB);
	write_cmos_sensor(0x6F12, 0xF4F0);
	write_cmos_sensor(0x6F12, 0xA6F8);
	write_cmos_sensor(0x6F12, 0xB802);
	write_cmos_sensor(0x6F12, 0xD5F8);
	write_cmos_sensor(0x6F12, 0x1411);
	write_cmos_sensor(0x6F12, 0x06F5);
	write_cmos_sensor(0x6F12, 0x2E76);
	write_cmos_sensor(0x6F12, 0x6143);
	write_cmos_sensor(0x6F12, 0xC5F8);
	write_cmos_sensor(0x6F12, 0x1411);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0x8C11);
	write_cmos_sensor(0x6F12, 0x411A);
	write_cmos_sensor(0x6F12, 0x89B2);
	write_cmos_sensor(0x6F12, 0x25F8);
	write_cmos_sensor(0x6F12, 0x981B);
	write_cmos_sensor(0x6F12, 0x35F8);
	write_cmos_sensor(0x6F12, 0x142C);
	write_cmos_sensor(0x6F12, 0x6243);
	write_cmos_sensor(0x6F12, 0x521E);
	write_cmos_sensor(0x6F12, 0x00FB);
	write_cmos_sensor(0x6F12, 0x0210);
	write_cmos_sensor(0x6F12, 0xB5F8);
	write_cmos_sensor(0x6F12, 0xF210);
	write_cmos_sensor(0x6F12, 0x07FB);
	write_cmos_sensor(0x6F12, 0x04F2);
	write_cmos_sensor(0x6F12, 0x0844);
	write_cmos_sensor(0x6F12, 0xC5F8);
	write_cmos_sensor(0x6F12, 0xF800);
	write_cmos_sensor(0x6F12, 0x5946);
	write_cmos_sensor(0x6F12, 0x0098);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xDBF9);
	write_cmos_sensor(0x6F12, 0x3088);
	write_cmos_sensor(0x6F12, 0x4146);
	write_cmos_sensor(0x6F12, 0x6043);
	write_cmos_sensor(0x6F12, 0x3080);
	write_cmos_sensor(0x6F12, 0xE86F);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0xB0FB);
	write_cmos_sensor(0x6F12, 0xF4F0);
	write_cmos_sensor(0x6F12, 0xE867);
	write_cmos_sensor(0x6F12, 0x04B0);
	write_cmos_sensor(0x6F12, 0x4846);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF05F);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xC7B9);
	write_cmos_sensor(0x6F12, 0x0124);
	write_cmos_sensor(0x6F12, 0xC8E7);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x8046);
	write_cmos_sensor(0x6F12, 0xD148);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x4168);
	write_cmos_sensor(0x6F12, 0x0D0C);
	write_cmos_sensor(0x6F12, 0x8EB2);
	write_cmos_sensor(0x6F12, 0x3146);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xB9F9);
	write_cmos_sensor(0x6F12, 0xD04C);
	write_cmos_sensor(0x6F12, 0xCE4F);
	write_cmos_sensor(0x6F12, 0x2078);
	write_cmos_sensor(0x6F12, 0x97F8);
	write_cmos_sensor(0x6F12, 0x8B12);
	write_cmos_sensor(0x6F12, 0x10FB);
	write_cmos_sensor(0x6F12, 0x01F0);
	write_cmos_sensor(0x6F12, 0x2070);
	write_cmos_sensor(0x6F12, 0x4046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xB8F9);
	write_cmos_sensor(0x6F12, 0x2078);
	write_cmos_sensor(0x6F12, 0x97F8);
	write_cmos_sensor(0x6F12, 0x8B12);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0xB0FB);
	write_cmos_sensor(0x6F12, 0xF1F0);
	write_cmos_sensor(0x6F12, 0x2070);
	write_cmos_sensor(0x6F12, 0x3146);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xA1B9);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xFF47);
	write_cmos_sensor(0x6F12, 0x8146);
	write_cmos_sensor(0x6F12, 0xBF48);
	write_cmos_sensor(0x6F12, 0x1746);
	write_cmos_sensor(0x6F12, 0x8846);
	write_cmos_sensor(0x6F12, 0x8068);
	write_cmos_sensor(0x6F12, 0x1C46);
	write_cmos_sensor(0x6F12, 0x85B2);
	write_cmos_sensor(0x6F12, 0x060C);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x92F9);
	write_cmos_sensor(0x6F12, 0x2346);
	write_cmos_sensor(0x6F12, 0x3A46);
	write_cmos_sensor(0x6F12, 0x4146);
	write_cmos_sensor(0x6F12, 0x4846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x9BF9);
	write_cmos_sensor(0x6F12, 0xBA4A);
	write_cmos_sensor(0x6F12, 0x9088);
	write_cmos_sensor(0x6F12, 0xF0B3);
	write_cmos_sensor(0x6F12, 0xB748);
	write_cmos_sensor(0x6F12, 0x90F8);
	write_cmos_sensor(0x6F12, 0xBA10);
	write_cmos_sensor(0x6F12, 0xD1B3);
	write_cmos_sensor(0x6F12, 0xD0F8);
	write_cmos_sensor(0x6F12, 0x2801);
	write_cmos_sensor(0x6F12, 0x1168);
	write_cmos_sensor(0x6F12, 0x8842);
	write_cmos_sensor(0x6F12, 0x00D3);
	write_cmos_sensor(0x6F12, 0x0846);
	write_cmos_sensor(0x6F12, 0x010A);
	write_cmos_sensor(0x6F12, 0xB1FA);
	write_cmos_sensor(0x6F12, 0x81F0);
	write_cmos_sensor(0x6F12, 0xC0F1);
	write_cmos_sensor(0x6F12, 0x1700);
	write_cmos_sensor(0x6F12, 0xC140);
	write_cmos_sensor(0x6F12, 0x02EB);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0xC9B2);
	write_cmos_sensor(0x6F12, 0x0389);
	write_cmos_sensor(0x6F12, 0xC288);
	write_cmos_sensor(0x6F12, 0x9B1A);
	write_cmos_sensor(0x6F12, 0x4B43);
	write_cmos_sensor(0x6F12, 0x8033);
	write_cmos_sensor(0x6F12, 0x02EB);
	write_cmos_sensor(0x6F12, 0x2322);
	write_cmos_sensor(0x6F12, 0x0092);
	write_cmos_sensor(0x6F12, 0x438A);
	write_cmos_sensor(0x6F12, 0x028A);
	write_cmos_sensor(0x6F12, 0x9B1A);
	write_cmos_sensor(0x6F12, 0x4B43);
	write_cmos_sensor(0x6F12, 0x8033);
	write_cmos_sensor(0x6F12, 0x02EB);
	write_cmos_sensor(0x6F12, 0x2322);
	write_cmos_sensor(0x6F12, 0x0192);
	write_cmos_sensor(0x6F12, 0x838B);
	write_cmos_sensor(0x6F12, 0x428B);
	write_cmos_sensor(0x6F12, 0x9B1A);
	write_cmos_sensor(0x6F12, 0x4B43);
	write_cmos_sensor(0x6F12, 0x8033);
	write_cmos_sensor(0x6F12, 0x02EB);
	write_cmos_sensor(0x6F12, 0x2322);
	write_cmos_sensor(0x6F12, 0x0292);
	write_cmos_sensor(0x6F12, 0xC28C);
	write_cmos_sensor(0x6F12, 0x808C);
	write_cmos_sensor(0x6F12, 0x121A);
	write_cmos_sensor(0x6F12, 0x4A43);
	write_cmos_sensor(0x6F12, 0x8032);
	write_cmos_sensor(0x6F12, 0x00EB);
	write_cmos_sensor(0x6F12, 0x2220);
	write_cmos_sensor(0x6F12, 0x0390);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0x6846);
	write_cmos_sensor(0x6F12, 0x54F8);
	write_cmos_sensor(0x6F12, 0x2210);
	write_cmos_sensor(0x6F12, 0x50F8);
	write_cmos_sensor(0x6F12, 0x2230);
	write_cmos_sensor(0x6F12, 0x5943);
	write_cmos_sensor(0x6F12, 0x090B);
	write_cmos_sensor(0x6F12, 0x44F8);
	write_cmos_sensor(0x6F12, 0x2210);
	write_cmos_sensor(0x6F12, 0x521C);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x01E0);
	write_cmos_sensor(0x6F12, 0x042A);
	write_cmos_sensor(0x6F12, 0xF2D3);
	write_cmos_sensor(0x6F12, 0x04B0);
	write_cmos_sensor(0x6F12, 0x2946);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF047);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x3FB9);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x954C);
	write_cmos_sensor(0x6F12, 0x9349);
	write_cmos_sensor(0x6F12, 0x0646);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0x6970);
	write_cmos_sensor(0x6F12, 0x8988);
	write_cmos_sensor(0x6F12, 0x94F8);
	write_cmos_sensor(0x6F12, 0x8120);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0xC1B1);
	write_cmos_sensor(0x6F12, 0x2146);
	write_cmos_sensor(0x6F12, 0xD1F8);
	write_cmos_sensor(0x6F12, 0x9410);
	write_cmos_sensor(0x6F12, 0x72B1);
	write_cmos_sensor(0x6F12, 0x8FB1);
	write_cmos_sensor(0x6F12, 0x0846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x3FF9);
	write_cmos_sensor(0x6F12, 0x0546);
	write_cmos_sensor(0x6F12, 0xE06F);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x3BF9);
	write_cmos_sensor(0x6F12, 0x8542);
	write_cmos_sensor(0x6F12, 0x02D2);
	write_cmos_sensor(0x6F12, 0xD4F8);
	write_cmos_sensor(0x6F12, 0x9400);
	write_cmos_sensor(0x6F12, 0x26E0);
	write_cmos_sensor(0x6F12, 0xE06F);
	write_cmos_sensor(0x6F12, 0x24E0);
	write_cmos_sensor(0x6F12, 0x002F);
	write_cmos_sensor(0x6F12, 0xFBD1);
	write_cmos_sensor(0x6F12, 0x002A);
	write_cmos_sensor(0x6F12, 0x24D0);
	write_cmos_sensor(0x6F12, 0x0846);
	write_cmos_sensor(0x6F12, 0x1EE0);
	write_cmos_sensor(0x6F12, 0x8149);
	write_cmos_sensor(0x6F12, 0x0D8E);
	write_cmos_sensor(0x6F12, 0x496B);
	write_cmos_sensor(0x6F12, 0x4B42);
	write_cmos_sensor(0x6F12, 0x77B1);
	write_cmos_sensor(0x6F12, 0x8148);
	write_cmos_sensor(0x6F12, 0x806F);
	write_cmos_sensor(0x6F12, 0x10E0);
	write_cmos_sensor(0x6F12, 0x4242);
	write_cmos_sensor(0x6F12, 0x00E0);
	write_cmos_sensor(0x6F12, 0x0246);
	write_cmos_sensor(0x6F12, 0x0029);
	write_cmos_sensor(0x6F12, 0x0FDB);
	write_cmos_sensor(0x6F12, 0x8A42);
	write_cmos_sensor(0x6F12, 0x0FDD);
	write_cmos_sensor(0x6F12, 0x3046);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF041);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x1FB9);
	write_cmos_sensor(0x6F12, 0x002A);
	write_cmos_sensor(0x6F12, 0x0CD0);
	write_cmos_sensor(0x6F12, 0x7848);
	write_cmos_sensor(0x6F12, 0xD0F8);
	write_cmos_sensor(0x6F12, 0x8C00);
	write_cmos_sensor(0x6F12, 0x25B1);
	write_cmos_sensor(0x6F12, 0x0028);
	write_cmos_sensor(0x6F12, 0xEDDA);
	write_cmos_sensor(0x6F12, 0xEAE7);
	write_cmos_sensor(0x6F12, 0x1946);
	write_cmos_sensor(0x6F12, 0xEDE7);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x17F9);
	write_cmos_sensor(0x6F12, 0xE060);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xF081);
	write_cmos_sensor(0x6F12, 0x2DE9);
	write_cmos_sensor(0x6F12, 0xF35F);
	write_cmos_sensor(0x6F12, 0xDFF8);
	write_cmos_sensor(0x6F12, 0xB0A1);
	write_cmos_sensor(0x6F12, 0x0C46);
	write_cmos_sensor(0x6F12, 0xBAF8);
	write_cmos_sensor(0x6F12, 0xBE04);
	write_cmos_sensor(0x6F12, 0x08B1);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x0EF9);
	write_cmos_sensor(0x6F12, 0x6C4E);
	write_cmos_sensor(0x6F12, 0x3088);
	write_cmos_sensor(0x6F12, 0x0128);
	write_cmos_sensor(0x6F12, 0x06D1);
	write_cmos_sensor(0x6F12, 0x002C);
	write_cmos_sensor(0x6F12, 0x04D1);
	write_cmos_sensor(0x6F12, 0x684D);
	write_cmos_sensor(0x6F12, 0x2889);
	write_cmos_sensor(0x6F12, 0x18B1);
	write_cmos_sensor(0x6F12, 0x401E);
	write_cmos_sensor(0x6F12, 0x2881);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xFC9F);
	write_cmos_sensor(0x6F12, 0xDFF8);
	write_cmos_sensor(0x6F12, 0x9891);
	write_cmos_sensor(0x6F12, 0xD9F8);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0xD602);
	write_cmos_sensor(0x6F12, 0x38B1);
	write_cmos_sensor(0x6F12, 0x3089);
	write_cmos_sensor(0x6F12, 0x401C);
	write_cmos_sensor(0x6F12, 0x80B2);
	write_cmos_sensor(0x6F12, 0x3081);
	write_cmos_sensor(0x6F12, 0xFF28);
	write_cmos_sensor(0x6F12, 0x01D9);
	write_cmos_sensor(0x6F12, 0xE889);
	write_cmos_sensor(0x6F12, 0x3081);
	write_cmos_sensor(0x6F12, 0x6048);
	write_cmos_sensor(0x6F12, 0x4FF0);
	write_cmos_sensor(0x6F12, 0x0008);
	write_cmos_sensor(0x6F12, 0xC6F8);
	write_cmos_sensor(0x6F12, 0x0C80);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0x5EB0);
	write_cmos_sensor(0x6F12, 0x40F2);
	write_cmos_sensor(0x6F12, 0xFF31);
	write_cmos_sensor(0x6F12, 0x0B20);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xEBF8);
	write_cmos_sensor(0x6F12, 0xD9F8);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0027);
	write_cmos_sensor(0x6F12, 0x3C46);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0xD412);
	write_cmos_sensor(0x6F12, 0x21B1);
	write_cmos_sensor(0x6F12, 0x0098);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xD2F8);
	write_cmos_sensor(0x6F12, 0x0746);
	write_cmos_sensor(0x6F12, 0x0BE0);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0xD602);
	write_cmos_sensor(0x6F12, 0x40B1);
	write_cmos_sensor(0x6F12, 0x3089);
	write_cmos_sensor(0x6F12, 0xE989);
	write_cmos_sensor(0x6F12, 0x8842);
	write_cmos_sensor(0x6F12, 0x04D3);
	write_cmos_sensor(0x6F12, 0x0098);
	write_cmos_sensor(0x6F12, 0xFFF7);
	write_cmos_sensor(0x6F12, 0x6EFF);
	write_cmos_sensor(0x6F12, 0x0746);
	write_cmos_sensor(0x6F12, 0x0124);
	write_cmos_sensor(0x6F12, 0x3846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xD5F8);
	write_cmos_sensor(0x6F12, 0xD9F8);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xB0F8);
	write_cmos_sensor(0x6F12, 0xD602);
	write_cmos_sensor(0x6F12, 0x08B9);
	write_cmos_sensor(0x6F12, 0xA6F8);
	write_cmos_sensor(0x6F12, 0x0280);
	write_cmos_sensor(0x6F12, 0xC7B3);
	write_cmos_sensor(0x6F12, 0x4746);
	write_cmos_sensor(0x6F12, 0xA6F8);
	write_cmos_sensor(0x6F12, 0x0880);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xCDF8);
	write_cmos_sensor(0x6F12, 0xF068);
	write_cmos_sensor(0x6F12, 0x3061);
	write_cmos_sensor(0x6F12, 0x688D);
	write_cmos_sensor(0x6F12, 0x50B3);
	write_cmos_sensor(0x6F12, 0xA88D);
	write_cmos_sensor(0x6F12, 0x50BB);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0xCAF8);
	write_cmos_sensor(0x6F12, 0xA889);
	write_cmos_sensor(0x6F12, 0x20B3);
	write_cmos_sensor(0x6F12, 0x1CB3);
	write_cmos_sensor(0x6F12, 0x706B);
	write_cmos_sensor(0x6F12, 0xAA88);
	write_cmos_sensor(0x6F12, 0xDAF8);
	write_cmos_sensor(0x6F12, 0x0815);
	write_cmos_sensor(0x6F12, 0xCAB1);
	write_cmos_sensor(0x6F12, 0x8842);
	write_cmos_sensor(0x6F12, 0x0CDB);
	write_cmos_sensor(0x6F12, 0x90FB);
	write_cmos_sensor(0x6F12, 0xF1F3);
	write_cmos_sensor(0x6F12, 0x90FB);
	write_cmos_sensor(0x6F12, 0xF1F2);
	write_cmos_sensor(0x6F12, 0x01FB);
	write_cmos_sensor(0x6F12, 0x1303);
	write_cmos_sensor(0x6F12, 0xB3EB);
	write_cmos_sensor(0x6F12, 0x610F);
	write_cmos_sensor(0x6F12, 0x00DD);
	write_cmos_sensor(0x6F12, 0x521C);
	write_cmos_sensor(0x6F12, 0x01FB);
	write_cmos_sensor(0x6F12, 0x1200);
	write_cmos_sensor(0x6F12, 0x0BE0);
	write_cmos_sensor(0x6F12, 0x91FB);
	write_cmos_sensor(0x6F12, 0xF0F3);
	write_cmos_sensor(0x6F12, 0x91FB);
	write_cmos_sensor(0x6F12, 0xF0F2);
	write_cmos_sensor(0x6F12, 0x00FB);
	write_cmos_sensor(0x6F12, 0x1313);
	write_cmos_sensor(0x6F12, 0xB3EB);
	write_cmos_sensor(0x6F12, 0x600F);
	write_cmos_sensor(0x6F12, 0x00DD);
	write_cmos_sensor(0x6F12, 0x521C);
	write_cmos_sensor(0x6F12, 0x5043);
	write_cmos_sensor(0x6F12, 0x401A);
	write_cmos_sensor(0x6F12, 0xF168);
	write_cmos_sensor(0x6F12, 0x01EB);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0xF060);
	write_cmos_sensor(0x6F12, 0xA88D);
	write_cmos_sensor(0x6F12, 0x10B1);
	write_cmos_sensor(0x6F12, 0xF089);
	write_cmos_sensor(0x6F12, 0x3087);
	write_cmos_sensor(0x6F12, 0xAF85);
	write_cmos_sensor(0x6F12, 0x5846);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0xFC5F);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x9EB8);
	write_cmos_sensor(0x6F12, 0x70B5);
	write_cmos_sensor(0x6F12, 0x2349);
	write_cmos_sensor(0x6F12, 0x0446);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x6F12, 0xC1F8);
	write_cmos_sensor(0x6F12, 0x3005);
	write_cmos_sensor(0x6F12, 0x1E48);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xC168);
	write_cmos_sensor(0x6F12, 0x0D0C);
	write_cmos_sensor(0x6F12, 0x8EB2);
	write_cmos_sensor(0x6F12, 0x3146);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x53F8);
	write_cmos_sensor(0x6F12, 0x2046);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x91F8);
	write_cmos_sensor(0x6F12, 0x3146);
	write_cmos_sensor(0x6F12, 0x2846);
	write_cmos_sensor(0x6F12, 0xBDE8);
	write_cmos_sensor(0x6F12, 0x7040);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x49B8);
	write_cmos_sensor(0x6F12, 0x10B5);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x9731);
	write_cmos_sensor(0x6F12, 0x1C48);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x88F8);
	write_cmos_sensor(0x6F12, 0x114C);
	write_cmos_sensor(0x6F12, 0x0122);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x0D31);
	write_cmos_sensor(0x6F12, 0x2060);
	write_cmos_sensor(0x6F12, 0x1948);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x80F8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0xD121);
	write_cmos_sensor(0x6F12, 0x6060);
	write_cmos_sensor(0x6F12, 0x1648);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x79F8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x1D21);
	write_cmos_sensor(0x6F12, 0xA060);
	write_cmos_sensor(0x6F12, 0x1448);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x72F8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x9511);
	write_cmos_sensor(0x6F12, 0x1248);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x6CF8);
	write_cmos_sensor(0x6F12, 0x0022);
	write_cmos_sensor(0x6F12, 0xAFF2);
	write_cmos_sensor(0x6F12, 0x7B01);
	write_cmos_sensor(0x6F12, 0x1048);
	write_cmos_sensor(0x6F12, 0x00F0);
	write_cmos_sensor(0x6F12, 0x66F8);
	write_cmos_sensor(0x6F12, 0xE060);
	write_cmos_sensor(0x6F12, 0x10BD);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x4460);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2C30);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2E30);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2580);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x6000);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2BA0);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x3600);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x0890);
	write_cmos_sensor(0x6F12, 0x4000);
	write_cmos_sensor(0x6F12, 0x7000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x24A7);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0x1AF3);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x6F12, 0x09BD);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x576B);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x57ED);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xBF8D);
	write_cmos_sensor(0x6F12, 0x4AF6);
	write_cmos_sensor(0x6F12, 0x293C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x42F2);
	write_cmos_sensor(0x6F12, 0xA74C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x41F6);
	write_cmos_sensor(0x6F12, 0xF32C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x010C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F6);
	write_cmos_sensor(0x6F12, 0xBD1C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x010C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4AF6);
	write_cmos_sensor(0x6F12, 0x532C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x45F2);
	write_cmos_sensor(0x6F12, 0x377C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x45F2);
	write_cmos_sensor(0x6F12, 0xD56C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x45F2);
	write_cmos_sensor(0x6F12, 0xC91C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F2);
	write_cmos_sensor(0x6F12, 0xAB2C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x44F6);
	write_cmos_sensor(0x6F12, 0x897C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x45F2);
	write_cmos_sensor(0x6F12, 0xA56C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x45F2);
	write_cmos_sensor(0x6F12, 0xEF6C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x40F2);
	write_cmos_sensor(0x6F12, 0x6D7C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4BF6);
	write_cmos_sensor(0x6F12, 0x8D7C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x4BF2);
	write_cmos_sensor(0x6F12, 0xAB4C);
	write_cmos_sensor(0x6F12, 0xC0F2);
	write_cmos_sensor(0x6F12, 0x000C);
	write_cmos_sensor(0x6F12, 0x6047);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x08D1);
	write_cmos_sensor(0x6F12, 0x008B);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0067);
#endif
}
static void custom1_setting(void)
{

	LOG_INF("E\n");
	write_cmos_sensor(0x6028, 0x2400);
	write_cmos_sensor(0x602A, 0x1A28);
	write_cmos_sensor(0x6F12, 0x4C00);
	write_cmos_sensor(0x602A, 0x065A);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x139E);
	write_cmos_sensor(0x6F12, 0x0300);
	write_cmos_sensor(0x602A, 0x139C);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x13A0);
	write_cmos_sensor(0x6F12, 0x0A00);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x602A, 0x2072);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1A64);
	write_cmos_sensor(0x6F12, 0x0301);
	write_cmos_sensor(0x6F12, 0xFF00);
	write_cmos_sensor(0x602A, 0x19E6);
	write_cmos_sensor(0x6F12, 0x0201);
	write_cmos_sensor(0x602A, 0x1A30);
	write_cmos_sensor(0x6F12, 0x3401);
	write_cmos_sensor(0x602A, 0x19FC);
	write_cmos_sensor(0x6F12, 0x0B00);
	write_cmos_sensor(0x602A, 0x19F4);
	write_cmos_sensor(0x6F12, 0x0606);
	write_cmos_sensor(0x602A, 0x19F8);
	write_cmos_sensor(0x6F12, 0x1010);
	write_cmos_sensor(0x602A, 0x1B26);
	write_cmos_sensor(0x6F12, 0x6F80);
	write_cmos_sensor(0x6F12, 0xA020);
	write_cmos_sensor(0x602A, 0x1A3C);
	write_cmos_sensor(0x6F12, 0x5207);
	write_cmos_sensor(0x602A, 0x1A48);
	write_cmos_sensor(0x6F12, 0x5207);
	write_cmos_sensor(0x602A, 0x1444);
	write_cmos_sensor(0x6F12, 0x2100);
	write_cmos_sensor(0x6F12, 0x2100);
	write_cmos_sensor(0x602A, 0x144C);
	write_cmos_sensor(0x6F12, 0x4200);
	write_cmos_sensor(0x6F12, 0x4200);
	write_cmos_sensor(0x602A, 0x7F6C);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0x3100);
	write_cmos_sensor(0x6F12, 0xF700);
	write_cmos_sensor(0x6F12, 0x2600);
	write_cmos_sensor(0x6F12, 0xE100);
	write_cmos_sensor(0x602A, 0x0650);
	write_cmos_sensor(0x6F12, 0x0600);
	write_cmos_sensor(0x602A, 0x0654);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1A46);
	write_cmos_sensor(0x6F12, 0x8900);
	write_cmos_sensor(0x602A, 0x1A52);
	write_cmos_sensor(0x6F12, 0xBF00);
	write_cmos_sensor(0x602A, 0x0674);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x602A, 0x0668);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x602A, 0x0684);
	write_cmos_sensor(0x6F12, 0x4001);
	write_cmos_sensor(0x602A, 0x0688);
	write_cmos_sensor(0x6F12, 0x4001);
	write_cmos_sensor(0x602A, 0x147C);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x1480);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x19F6);
	write_cmos_sensor(0x6F12, 0x0904);
	write_cmos_sensor(0x602A, 0x0812);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x602A, 0x2148);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x2042);
	write_cmos_sensor(0x6F12, 0x1A00);
	write_cmos_sensor(0x602A, 0x0874);
	write_cmos_sensor(0x6F12, 0x1100);
	write_cmos_sensor(0x602A, 0x09C0);
	write_cmos_sensor(0x6F12, 0x9800);
	write_cmos_sensor(0x602A, 0x09C4);
	write_cmos_sensor(0x6F12, 0x9800);
	write_cmos_sensor(0x602A, 0x19FE);
	write_cmos_sensor(0x6F12, 0x0E1C);
	write_cmos_sensor(0x602A, 0x4D92);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x8104);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x4D94);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x3570);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x3574);
	write_cmos_sensor(0x6F12, 0x0502);
	write_cmos_sensor(0x602A, 0x21E4);
	write_cmos_sensor(0x6F12, 0x0400);
	write_cmos_sensor(0x602A, 0x21EC);
	write_cmos_sensor(0x6F12, 0x1400);
	write_cmos_sensor(0x602A, 0x2080);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0xFF01);
	write_cmos_sensor(0x602A, 0x2086);
	write_cmos_sensor(0x6F12, 0x0002);
	write_cmos_sensor(0x602A, 0x208E);
	write_cmos_sensor(0x6F12, 0x14F4);
	write_cmos_sensor(0x602A, 0x208A);
	write_cmos_sensor(0x6F12, 0xC244);
	write_cmos_sensor(0x6F12, 0xD244);
	write_cmos_sensor(0x602A, 0x120E);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x212E);
	write_cmos_sensor(0x6F12, 0x0A00);
	write_cmos_sensor(0x602A, 0x13AE);
	write_cmos_sensor(0x6F12, 0x0102);
	write_cmos_sensor(0x602A, 0x0718);
	write_cmos_sensor(0x6F12, 0x0005);
	write_cmos_sensor(0x602A, 0x0710);
	write_cmos_sensor(0x6F12, 0x0004);
	write_cmos_sensor(0x6F12, 0x0401);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1B5C);
	write_cmos_sensor(0x6F12, 0x0300);
	write_cmos_sensor(0x602A, 0x0786);
	write_cmos_sensor(0x6F12, 0x7701);
	write_cmos_sensor(0x602A, 0x2022);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x602A, 0x1360);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1376);
	write_cmos_sensor(0x6F12, 0x0200);
	write_cmos_sensor(0x6F12, 0x6038);
	write_cmos_sensor(0x6F12, 0x7038);
	write_cmos_sensor(0x6F12, 0x8038);
	write_cmos_sensor(0x602A, 0x1386);
	write_cmos_sensor(0x6F12, 0x0B00);
	write_cmos_sensor(0x602A, 0x06FA);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x4A94);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x6F12, 0xFAFF);
	write_cmos_sensor(0x602A, 0x0A76);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0AEE);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0B66);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0BDE);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0C56);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0CF2);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x0CF0);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x602A, 0x11B8);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x11F6);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x602A, 0x4A74);
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
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0xF46A, 0xAE80);
	write_cmos_sensor(0x0344, 0x00F0);
	write_cmos_sensor(0x0346, 0x0390);
	write_cmos_sensor(0x0348, 0x1F0F);
	write_cmos_sensor(0x034A, 0x148F);
	write_cmos_sensor(0x034C, 0x0780);
	write_cmos_sensor(0x034E, 0x0438);
	write_cmos_sensor(0x0350, 0x0004);
	write_cmos_sensor(0x0352, 0x0004);
	write_cmos_sensor(0x0900, 0x0144);
	write_cmos_sensor(0x0380, 0x0002);
	write_cmos_sensor(0x0382, 0x0006);
	write_cmos_sensor(0x0384, 0x0002);
	write_cmos_sensor(0x0386, 0x0006);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0114, 0x0300);
	write_cmos_sensor(0x0116, 0x3000);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x013E, 0x0000);
	write_cmos_sensor(0x0300, 0x0006);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0004);
	write_cmos_sensor(0x0306, 0x0096);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0003);
	write_cmos_sensor(0x0310, 0x0064);
	write_cmos_sensor(0x0312, 0x0000);
	write_cmos_sensor(0x080E, 0x0000);
	write_cmos_sensor(0x0340, 0x1270);
	write_cmos_sensor(0x0342, 0x0844);
	write_cmos_sensor(0x0702, 0x0000);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x0D00, 0x0101);
	write_cmos_sensor(0x0D02, 0x0001);
	write_cmos_sensor(0x0D04, 0x0002);
	write_cmos_sensor(0x6226, 0x0000);
}

static void capture_setting(kal_uint16 currefps)
{

	LOG_INF("start\n");
	write_cmos_sensor(0x6028, 0x2400);
	write_cmos_sensor(0x602A, 0x1A28);
	write_cmos_sensor(0x6F12, 0x4C00);
	write_cmos_sensor(0x602A, 0x065A);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x139E);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x139C);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x13A0);
	write_cmos_sensor(0x6F12, 0x0A00);
	write_cmos_sensor(0x6F12, 0x0120);
	write_cmos_sensor(0x602A, 0x2072);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1A64);
	write_cmos_sensor(0x6F12, 0x0301);
	write_cmos_sensor(0x6F12, 0xFF00);
	write_cmos_sensor(0x602A, 0x19E6);
	write_cmos_sensor(0x6F12, 0x0200);
	write_cmos_sensor(0x602A, 0x1A30);
	write_cmos_sensor(0x6F12, 0x3401);
	write_cmos_sensor(0x602A, 0x19FC);
	write_cmos_sensor(0x6F12, 0x0B00);
	write_cmos_sensor(0x602A, 0x19F4);
	write_cmos_sensor(0x6F12, 0x0606);
	write_cmos_sensor(0x602A, 0x19F8);
	write_cmos_sensor(0x6F12, 0x1010);
	write_cmos_sensor(0x602A, 0x1B26);
	write_cmos_sensor(0x6F12, 0x6F80);
	write_cmos_sensor(0x6F12, 0xA060);
	write_cmos_sensor(0x602A, 0x1A3C);
	write_cmos_sensor(0x6F12, 0x6207);
	write_cmos_sensor(0x602A, 0x1A48);
	write_cmos_sensor(0x6F12, 0x6207);
	write_cmos_sensor(0x602A, 0x1444);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x602A, 0x144C);
	write_cmos_sensor(0x6F12, 0x3F00);
	write_cmos_sensor(0x6F12, 0x3F00);
	write_cmos_sensor(0x602A, 0x7F6C);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0x2F00);
	write_cmos_sensor(0x6F12, 0xFA00);
	write_cmos_sensor(0x6F12, 0x2400);
	write_cmos_sensor(0x6F12, 0xE500);
	write_cmos_sensor(0x602A, 0x0650);
	write_cmos_sensor(0x6F12, 0x0600);
	write_cmos_sensor(0x602A, 0x0654);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1A46);
	write_cmos_sensor(0x6F12, 0x8A00);
	write_cmos_sensor(0x602A, 0x1A52);
	write_cmos_sensor(0x6F12, 0xBF00);
	write_cmos_sensor(0x602A, 0x0674);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x602A, 0x0668);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x602A, 0x0684);
	write_cmos_sensor(0x6F12, 0x4001);
	write_cmos_sensor(0x602A, 0x0688);
	write_cmos_sensor(0x6F12, 0x4001);
	write_cmos_sensor(0x602A, 0x147C);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x1480);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x19F6);
	write_cmos_sensor(0x6F12, 0x0904);
	write_cmos_sensor(0x602A, 0x0812);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1A02);
	write_cmos_sensor(0x6F12, 0x1800);
	write_cmos_sensor(0x602A, 0x2148);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x2042);
	write_cmos_sensor(0x6F12, 0x1A00);
	write_cmos_sensor(0x602A, 0x0874);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x09C0);
	write_cmos_sensor(0x6F12, 0x2008);
	write_cmos_sensor(0x602A, 0x09C4);
	write_cmos_sensor(0x6F12, 0x2000);
	write_cmos_sensor(0x602A, 0x19FE);
	write_cmos_sensor(0x6F12, 0x0E1C);
	write_cmos_sensor(0x602A, 0x4D92);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x84C8);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x4D94);
	write_cmos_sensor(0x6F12, 0x0005);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0x0810);
	write_cmos_sensor(0x6F12, 0x000A);
	write_cmos_sensor(0x6F12, 0x0040);
	write_cmos_sensor(0x6F12, 0x0810);
	write_cmos_sensor(0x6F12, 0x0810);
	write_cmos_sensor(0x6F12, 0x8002);
	write_cmos_sensor(0x6F12, 0xFD03);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0x1510);
	write_cmos_sensor(0x602A, 0x3570);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x3574);
	write_cmos_sensor(0x6F12, 0x1201);
	write_cmos_sensor(0x602A, 0x21E4);
	write_cmos_sensor(0x6F12, 0x0400);
	write_cmos_sensor(0x602A, 0x21EC);
	write_cmos_sensor(0x6F12, 0x1F04);
	write_cmos_sensor(0x602A, 0x2080);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0xFF00);
	write_cmos_sensor(0x602A, 0x2086);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x208E);
	write_cmos_sensor(0x6F12, 0x14F4);
	write_cmos_sensor(0x602A, 0x208A);
	write_cmos_sensor(0x6F12, 0xD244);
	write_cmos_sensor(0x6F12, 0xD244);
	write_cmos_sensor(0x602A, 0x120E);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x212E);
	write_cmos_sensor(0x6F12, 0x0200);
	write_cmos_sensor(0x602A, 0x13AE);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x602A, 0x0718);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x0710);
	write_cmos_sensor(0x6F12, 0x0002);
	write_cmos_sensor(0x6F12, 0x0804);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1B5C);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x0786);
	write_cmos_sensor(0x6F12, 0x7701);
	write_cmos_sensor(0x602A, 0x2022);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x602A, 0x1360);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1376);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0x6038);
	write_cmos_sensor(0x6F12, 0x7038);
	write_cmos_sensor(0x6F12, 0x8038);
	write_cmos_sensor(0x602A, 0x1386);
	write_cmos_sensor(0x6F12, 0x0B00);
	write_cmos_sensor(0x602A, 0x06FA);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x4A94);
	write_cmos_sensor(0x6F12, 0x0900);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0300);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0300);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0900);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x0A76);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0AEE);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0B66);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0BDE);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0C56);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0CB6);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x0CF2);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x0CF0);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x602A, 0x11B8);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x11F6);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x602A, 0x4A74);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xD8FF);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xD8FF);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x218E);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x2268);
	write_cmos_sensor(0x6F12, 0xF379);
	write_cmos_sensor(0x602A, 0x5006);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x500E);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x4E70);
	write_cmos_sensor(0x6F12, 0x2062);
	write_cmos_sensor(0x6F12, 0x5501);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0xF46A, 0xAE80);
	write_cmos_sensor(0x0344, 0x0000);
	write_cmos_sensor(0x0346, 0x0000);
	write_cmos_sensor(0x0348, 0x1FFF);
	write_cmos_sensor(0x034A, 0x181F);
	write_cmos_sensor(0x034C, 0x0FF0);
	write_cmos_sensor(0x034E, 0x0C00);
	write_cmos_sensor(0x0350, 0x0008);
	write_cmos_sensor(0x0352, 0x0008);
	write_cmos_sensor(0x0900, 0x0122);
	write_cmos_sensor(0x0380, 0x0002);
	write_cmos_sensor(0x0382, 0x0002);
	write_cmos_sensor(0x0384, 0x0002);
	write_cmos_sensor(0x0386, 0x0002);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0114, 0x0301);
	write_cmos_sensor(0x0116, 0x3000);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x013E, 0x0000);
	write_cmos_sensor(0x0300, 0x0006);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0004);
	write_cmos_sensor(0x0306, 0x008C);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0004);
	write_cmos_sensor(0x0310, 0x0067);
	write_cmos_sensor(0x0312, 0x0000);
	write_cmos_sensor(0x080E, 0x0000);
	write_cmos_sensor(0x0340, 0x0C54);
	write_cmos_sensor(0x0342, 0x1716);
	write_cmos_sensor(0x0702, 0x0000);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x0D00, 0x0101);
	write_cmos_sensor(0x0D02, 0x0101);
	write_cmos_sensor(0x0D04, 0x0102);
	write_cmos_sensor(0x6226, 0x0000);
}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("start\n");
//	preview_setting();
	capture_setting(currefps);
}

static void slim_video_setting(kal_uint16 currefps)
{
	LOG_INF("start\n");
	capture_setting(currefps);
}

static void hs_video_setting(void)
{

	LOG_INF("E\n");
	write_cmos_sensor(0x6028, 0x2400);
	write_cmos_sensor(0x602A, 0x1A28);
	write_cmos_sensor(0x6F12, 0x4C00);
	write_cmos_sensor(0x602A, 0x065A);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x139E);
	write_cmos_sensor(0x6F12, 0x0300);
	write_cmos_sensor(0x602A, 0x139C);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x13A0);
	write_cmos_sensor(0x6F12, 0x0A00);
	write_cmos_sensor(0x6F12, 0x0020);
	write_cmos_sensor(0x602A, 0x2072);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1A64);
	write_cmos_sensor(0x6F12, 0x0301);
	write_cmos_sensor(0x6F12, 0x3F00);
	write_cmos_sensor(0x602A, 0x19E6);
	write_cmos_sensor(0x6F12, 0x0201);
	write_cmos_sensor(0x602A, 0x1A30);
	write_cmos_sensor(0x6F12, 0x3401);
	write_cmos_sensor(0x602A, 0x19FC);
	write_cmos_sensor(0x6F12, 0x0B00);
	write_cmos_sensor(0x602A, 0x19F4);
	write_cmos_sensor(0x6F12, 0x0606);
	write_cmos_sensor(0x602A, 0x19F8);
	write_cmos_sensor(0x6F12, 0x1010);
	write_cmos_sensor(0x602A, 0x1B26);
	write_cmos_sensor(0x6F12, 0x6F80);
	write_cmos_sensor(0x6F12, 0xA020);
	write_cmos_sensor(0x602A, 0x1A3C);
	write_cmos_sensor(0x6F12, 0x5207);
	write_cmos_sensor(0x602A, 0x1A48);
	write_cmos_sensor(0x6F12, 0x5207);
	write_cmos_sensor(0x602A, 0x1444);
	write_cmos_sensor(0x6F12, 0x2100);
	write_cmos_sensor(0x6F12, 0x2100);
	write_cmos_sensor(0x602A, 0x144C);
	write_cmos_sensor(0x6F12, 0x4200);
	write_cmos_sensor(0x6F12, 0x4200);
	write_cmos_sensor(0x602A, 0x7F6C);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0x3100);
	write_cmos_sensor(0x6F12, 0xF700);
	write_cmos_sensor(0x6F12, 0x2600);
	write_cmos_sensor(0x6F12, 0xE100);
	write_cmos_sensor(0x602A, 0x0650);
	write_cmos_sensor(0x6F12, 0x0600);
	write_cmos_sensor(0x602A, 0x0654);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1A46);
	write_cmos_sensor(0x6F12, 0x8600);
	write_cmos_sensor(0x602A, 0x1A52);
	write_cmos_sensor(0x6F12, 0xBF00);
	write_cmos_sensor(0x602A, 0x0674);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x6F12, 0x0500);
	write_cmos_sensor(0x602A, 0x0668);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x602A, 0x0684);
	write_cmos_sensor(0x6F12, 0x4001);
	write_cmos_sensor(0x602A, 0x0688);
	write_cmos_sensor(0x6F12, 0x4001);
	write_cmos_sensor(0x602A, 0x147C);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x1480);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x19F6);
	write_cmos_sensor(0x6F12, 0x0904);
	write_cmos_sensor(0x602A, 0x0812);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1A02);
	write_cmos_sensor(0x6F12, 0x0800);
	write_cmos_sensor(0x602A, 0x2148);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x2042);
	write_cmos_sensor(0x6F12, 0x1A00);
	write_cmos_sensor(0x602A, 0x0874);
	write_cmos_sensor(0x6F12, 0x1100);
	write_cmos_sensor(0x602A, 0x09C0);
	write_cmos_sensor(0x6F12, 0x1803);
	write_cmos_sensor(0x602A, 0x09C4);
	write_cmos_sensor(0x6F12, 0x1803);
	write_cmos_sensor(0x602A, 0x19FE);
	write_cmos_sensor(0x6F12, 0x0E1C);
	write_cmos_sensor(0x602A, 0x4D92);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x84C8);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x4D94);
	write_cmos_sensor(0x6F12, 0x4001);
	write_cmos_sensor(0x6F12, 0x0004);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0x0810);
	write_cmos_sensor(0x6F12, 0x0004);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0x0810);
	write_cmos_sensor(0x6F12, 0x0810);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x602A, 0x3570);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x3574);
	write_cmos_sensor(0x6F12, 0x3801);
	write_cmos_sensor(0x602A, 0x21E4);
	write_cmos_sensor(0x6F12, 0x0400);
	write_cmos_sensor(0x602A, 0x21EC);
	write_cmos_sensor(0x6F12, 0x6801);
	write_cmos_sensor(0x602A, 0x2080);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x6F12, 0x7F00);
	write_cmos_sensor(0x602A, 0x2086);
	write_cmos_sensor(0x6F12, 0x8000);
	write_cmos_sensor(0x602A, 0x208E);
	write_cmos_sensor(0x6F12, 0x14F4);
	write_cmos_sensor(0x602A, 0x208A);
	write_cmos_sensor(0x6F12, 0xC244);
	write_cmos_sensor(0x6F12, 0xD244);
	write_cmos_sensor(0x602A, 0x120E);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x212E);
	write_cmos_sensor(0x6F12, 0x0A00);
	write_cmos_sensor(0x602A, 0x13AE);
	write_cmos_sensor(0x6F12, 0x0102);
	write_cmos_sensor(0x602A, 0x0718);
	write_cmos_sensor(0x6F12, 0x0005);
	write_cmos_sensor(0x602A, 0x0710);
	write_cmos_sensor(0x6F12, 0x0004);
	write_cmos_sensor(0x6F12, 0x0401);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x1B5C);
	write_cmos_sensor(0x6F12, 0x0300);
	write_cmos_sensor(0x602A, 0x0786);
	write_cmos_sensor(0x6F12, 0x7701);
	write_cmos_sensor(0x602A, 0x2022);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x602A, 0x1360);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x1376);
	write_cmos_sensor(0x6F12, 0x0200);
	write_cmos_sensor(0x6F12, 0x6038);
	write_cmos_sensor(0x6F12, 0x7038);
	write_cmos_sensor(0x6F12, 0x8038);
	write_cmos_sensor(0x602A, 0x1386);
	write_cmos_sensor(0x6F12, 0x0B00);
	write_cmos_sensor(0x602A, 0x06FA);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x4A94);
	write_cmos_sensor(0x6F12, 0x1600);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x1600);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x0A76);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0AEE);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0B66);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0BDE);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0C56);
	write_cmos_sensor(0x6F12, 0x1000);
	write_cmos_sensor(0x602A, 0x0CB6);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x0CF2);
	write_cmos_sensor(0x6F12, 0x0001);
	write_cmos_sensor(0x602A, 0x0CF0);
	write_cmos_sensor(0x6F12, 0x0101);
	write_cmos_sensor(0x602A, 0x11B8);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x11F6);
	write_cmos_sensor(0x6F12, 0x0010);
	write_cmos_sensor(0x602A, 0x4A74);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xD8FF);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0xD8FF);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x218E);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x2268);
	write_cmos_sensor(0x6F12, 0xF279);
	write_cmos_sensor(0x602A, 0x5006);
	write_cmos_sensor(0x6F12, 0x0000);
	write_cmos_sensor(0x602A, 0x500E);
	write_cmos_sensor(0x6F12, 0x0100);
	write_cmos_sensor(0x602A, 0x4E70);
	write_cmos_sensor(0x6F12, 0x2062);
	write_cmos_sensor(0x6F12, 0x5501);
	write_cmos_sensor(0x6028, 0x4000);
	write_cmos_sensor(0xF46A, 0xAE80);
	write_cmos_sensor(0x0344, 0x05F0);
	write_cmos_sensor(0x0346, 0x0660);
	write_cmos_sensor(0x0348, 0x1A0F);
	write_cmos_sensor(0x034A, 0x11BF);
	write_cmos_sensor(0x034C, 0x0500);
	write_cmos_sensor(0x034E, 0x02D0);
	write_cmos_sensor(0x0350, 0x0004);
	write_cmos_sensor(0x0352, 0x0004);
	write_cmos_sensor(0x0900, 0x0144);
	write_cmos_sensor(0x0380, 0x0002);
	write_cmos_sensor(0x0382, 0x0006);
	write_cmos_sensor(0x0384, 0x0002);
	write_cmos_sensor(0x0386, 0x0006);
	write_cmos_sensor(0x0110, 0x1002);
	write_cmos_sensor(0x0114, 0x0300);
	write_cmos_sensor(0x0116, 0x3000);
	write_cmos_sensor(0x0136, 0x1800);
	write_cmos_sensor(0x013E, 0x0000);
	write_cmos_sensor(0x0300, 0x0006);
	write_cmos_sensor(0x0302, 0x0001);
	write_cmos_sensor(0x0304, 0x0004);
	write_cmos_sensor(0x0306, 0x0096);
	write_cmos_sensor(0x0308, 0x0008);
	write_cmos_sensor(0x030A, 0x0001);
	write_cmos_sensor(0x030C, 0x0000);
	write_cmos_sensor(0x030E, 0x0003);
	write_cmos_sensor(0x0310, 0x0064);
	write_cmos_sensor(0x0312, 0x0000);
	write_cmos_sensor(0x080E, 0x0000);
	write_cmos_sensor(0x0340, 0x094C);
	write_cmos_sensor(0x0342, 0x0830);
	write_cmos_sensor(0x0702, 0x0000);
	write_cmos_sensor(0x0202, 0x0100);
	write_cmos_sensor(0x0200, 0x0100);
	write_cmos_sensor(0x0D00, 0x0101);
	write_cmos_sensor(0x0D02, 0x0001);
	write_cmos_sensor(0x0D04, 0x0002);
	write_cmos_sensor(0x6226, 0x0000);
}

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor_8(0x0000) << 8) | read_cmos_sensor_8(0x0001));
}

/*************************************************************************
*FUNCTION
*  get_imgsensor_id
*
*DESCRIPTION
*  This function get the sensor ID
*
*PARAMETERS
*  *sensorID : return the sensor ID
*
*RETURNS
*  None
*
*GLOBALS AFFECTED
*
*************************************************************************/
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

			if (*sensor_id == imgsensor_info.sensor_id) {
				// get calibration status and mnf data.
				imgread_cam_cal_data(*sensor_id, s5kjn1_dump_file, &s5kjn1_cal_info);

				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
					imgsensor.i2c_write_id, *sensor_id);
				return ERROR_NONE;
			}

			LOG_INF("Read sensor id fail, id: 0x%x\n",
				imgsensor.i2c_write_id);
			retry--;
		} while (retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id != imgsensor_info.sensor_id) {
		/*if Sensor ID is not correct,
		 *Must set *sensor_id to 0xFFFFFFFF
		 */
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

/*************************************************************************
*FUNCTION
*  open
*
*DESCRIPTION
*  This function initialize the registers of CMOS sensor
*
*PARAMETERS
*  None
*
*RETURNS
*  None
*
*GLOBALS AFFECTED
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
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n",
				 		imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x, sensor id: 0x%x\n",
			 		imgsensor.i2c_write_id, sensor_id);
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
	spin_lock(&imgsensor_drv_lock);
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = KAL_FALSE;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	flash_mode = 1;
	return ERROR_NONE;
}

/*************************************************************************
*FUNCTION
*  close
*
*DESCRIPTION
*
*
*PARAMETERS
*  None
*
*RETURNS
*  None
*
*GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
	LOG_INF("E\n");

	flash_mode = 0;
	return ERROR_NONE;
}

/*************************************************************************
*FUNCTION
*preview
*
*DESCRIPTION
*  This function start the sensor preview.
*
*PARAMETERS
*  *image_window : address pointer of pixel numbers in one period of HSYNC
**sensor_config_data : address pointer of line numbers in one period of VSYNC
*
*RETURNS
*  None
*
*GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32
preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;

	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);   //preview use capture setting
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;
}

/*************************************************************************
*FUNCTION
*  capture
*
*DESCRIPTION
*  This function setup the CMOS sensor in capture MY_OUTPUT mode
*
*PARAMETERS
*
*RETURNS
*  None
*
*GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32
capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
/*
#ifdef FANPENGTAO
	int i;
#endif
*/
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) {
		LOG_INF("capture30fps: use cap30FPS's setting: %d fps!\n",
				imgsensor.current_fps / 10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {

		LOG_INF("cap115fps: use cap1's setting: %d fps!\n",
				imgsensor.current_fps / 10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	} else {
		LOG_INF
		("Warning:current_fps %d is not support, use cap1\n",
		 imgsensor.current_fps / 10);
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);
	mdelay(10);
/*
#ifdef FANPENGTAO
	for (i = 0; i < 10; i++) {
		LOG_INF("delay time = %d, the frame no = %d\n", i * 10,
				read_cmos_sensor(0x0005));
		mdelay(10);
	}
#endif
*/
	return ERROR_NONE;
}

static kal_uint32
normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
	 image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;

	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;
}

static kal_uint32
hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		 image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
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
}

static kal_uint32
slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		   image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
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
	slim_video_setting(imgsensor.current_fps);
	set_mirror_flip(imgsensor.mirror);
	return ERROR_NONE;
}
Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *
		image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
	imgsensor.pclk = imgsensor_info.custom1.pclk;

	imgsensor.line_length = imgsensor_info.custom1.linelength;
	imgsensor.frame_length = imgsensor_info.custom1.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom1_setting();
	return ERROR_NONE;
}
static kal_uint32
get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
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
	return ERROR_NONE;
}

static kal_uint32
get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
		 MSDK_SENSOR_INFO_STRUCT *sensor_info,
		 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	sensor_info->SensorInterruptDelayLines = 4;
	sensor_info->SensorResetActiveHigh = FALSE;
	sensor_info->SensorResetDelayCount = 5;
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
	sensor_info->FrameTimeDelayFrame =
		imgsensor_info.frame_time_delay_frame;
	sensor_info->SensorMasterClockSwitch = 0;
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3;
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2;
	sensor_info->SensorPixelClockCount = 3;
	sensor_info->SensorDataLatchCount = 2;
	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;
	sensor_info->SensorHightSampling = 0;
	sensor_info->SensorPacketECCOrder = 1;
#ifdef FPTPDAFSUPPORT
	sensor_info->PDAF_Support = 2;
#else
	sensor_info->PDAF_Support = 0;
#endif

	sensor_info->calibration_status.mnf   = s5kjn1_cal_info.mnf_status;
	sensor_info->calibration_status.af    = s5kjn1_cal_info.af_status;
	sensor_info->calibration_status.awb   = s5kjn1_cal_info.awb_status;
	sensor_info->calibration_status.lsc   = s5kjn1_cal_info.lsc_status;
	sensor_info->calibration_status.pdaf  = s5kjn1_cal_info.pdaf_status;
	sensor_info->calibration_status.dual  = s5kjn1_cal_info.dual_status;

	memcpy(&sensor_info->mnf_calibration, &s5kjn1_cal_info.mnf_cal_data, sizeof(mot_calibration_mnf_t));

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
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
		break;
	}
	return ERROR_NONE;
}

static kal_uint32
control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
		MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
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
		Custom1(image_window, sensor_config_data);
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
	LOG_INF("enable = %d, framerate = %d\n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable)
		imgsensor.autoflicker_en = KAL_TRUE;
	else
		imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32
set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM
	  scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);
	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length =
			imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			 imgsensor_info.pre.framelength) ? (frame_length -
			imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);

		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
		frame_length =
			imgsensor_info.normal_video.pclk / framerate * 10 /
			imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
		(frame_length > imgsensor_info.normal_video.framelength)
		  ? (frame_length -
		   imgsensor_info.normal_video.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.normal_video.framelength +
			imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps ==
			 imgsensor_info.cap1.max_framerate) {
			frame_length =
				imgsensor_info.cap1.pclk / framerate * 10 /
				imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
			(frame_length > imgsensor_info.cap1.framelength)
			 ? (frame_length - imgsensor_info.cap1.framelength)
			  : 0;
			imgsensor.frame_length =
				imgsensor_info.cap1.framelength +
				imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
		} else {
			if (imgsensor.current_fps !=
					imgsensor_info.cap.max_framerate)
				LOG_INF
		("current_fps %d is not support, so use cap' %d fps!\n",
				 framerate,
				 imgsensor_info.cap.max_framerate / 10);
			frame_length =
				imgsensor_info.cap.pclk / framerate * 10 /
				imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line =
				(frame_length > imgsensor_info.cap.framelength)
				 ? (frame_length -
				 imgsensor_info.cap.framelength) : 0;
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
		frame_length =
			imgsensor_info.hs_video.pclk / framerate * 10 /
			imgsensor_info.hs_video.linelength;
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
		frame_length =
			imgsensor_info.slim_video.pclk / framerate * 10 /
			imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length > imgsensor_info.slim_video.framelength)
			  ? (frame_length -
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
		if (imgsensor.current_fps !=
				imgsensor_info.custom1.max_framerate)
			LOG_INF
			("%d fps is not support, so use cap: %d fps!\n",
			 framerate,
			 imgsensor_info.custom1.max_framerate / 10);
		frame_length =
			imgsensor_info.custom1.pclk / framerate * 10 /
			imgsensor_info.custom1.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			 imgsensor_info.custom1.framelength) ? (frame_length -
			 imgsensor_info.custom1.framelength) : 0;
		if (imgsensor.dummy_line < 0)
			imgsensor.dummy_line = 0;
		imgsensor.frame_length =
			imgsensor_info.custom1.framelength
			 + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
		break;
	default:
		frame_length =
			imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frame_length >
			 imgsensor_info.pre.framelength) ? (frame_length -
			imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length =
			imgsensor_info.pre.framelength
			 + imgsensor.dummy_line;
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

static kal_uint32 get_default_framerate_by_scenario(enum
		MSDK_SCENARIO_ID_ENUM
		scenario_id,
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
	default:
		break;
	}
	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);
	if (enable) {

		write_cmos_sensor(0x0200, 0x0002);
		write_cmos_sensor(0x0202, 0x0002);
		write_cmos_sensor(0x0204, 0x0020);
		write_cmos_sensor(0x020E, 0x0100);
		write_cmos_sensor(0x0210, 0x0100);
		write_cmos_sensor(0x0212, 0x0100);
		write_cmos_sensor(0x0214, 0x0100);
		write_cmos_sensor(0x0600, 0x0002);
	} else {

		write_cmos_sensor(0x0600, 0x0000);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

static kal_uint32
feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
				UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *)feature_para;
	UINT32 fps = 0;

	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	pr_debug("feature_id = %d, len=%d\n", feature_id, *feature_para_len);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = imgsensor.line_length;
		*feature_return_para_16 = imgsensor.frame_length;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		pr_debug("imgsensor.pclk = %d,current_fps = %d\n",
				 imgsensor.pclk, imgsensor.current_fps);
		*feature_return_para_32 = imgsensor.pclk;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
		set_shutter((kal_uint32)*feature_data);
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
		write_cmos_sensor(sensor_reg_data->RegAddr,
						  sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData =
			read_cmos_sensor(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/*get the lens driver ID from EEPROM or
		 *just return LENS_DRIVER_ID_DO_NOT_CARE
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
		set_auto_flicker_mode((BOOL) * feature_data_16,
			  *(feature_data_16 + 1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)
			  *feature_data, *(feature_data + 1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)
			  *(feature_data), (MUINT32 *) (uintptr_t) (*
			  (feature_data + 1)));
		break;

	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL) * feature_data);
		break;

	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len = 4;
		break;

	case SENSOR_FEATURE_SET_FRAMERATE:
		pr_debug("current fps :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = (UINT16) *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;

	case SENSOR_FEATURE_SET_HDR:
		pr_debug("hdr enable :%d\n", *feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_mode = (UINT8) *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;

	case SENSOR_FEATURE_GET_CROP_INFO:
		pr_debug("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n",
				 (UINT32) * feature_data_32);

		wininfo =
			(struct SENSOR_WINSIZE_INFO_STRUCT
			 *)(uintptr_t) (*(feature_data + 1));

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
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,
				   (void *)&imgsensor_winsize_info[0],
				   sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:

		break;
	case SENSOR_FEATURE_SET_AWB_GAIN:
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) (*feature_data),
			 (UINT16) (*(feature_data + 1)));
		break;
	case SENSOR_FEATURE_SET_HDR_SHUTTER:
		pr_debug("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",
				 (UINT16) *feature_data,
				 (UINT16) *(feature_data + 1));
		break;

	case SENSOR_FEATURE_GET_PDAF_INFO:
		pr_debug("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n",
				 (UINT16) *feature_data);
		PDAFinfo =
			(struct SET_PD_BLOCK_INFO_T
			 *)(uintptr_t) (*(feature_data + 1));
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CUSTOM1:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				   sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		pr_debug("SENSOR_FEATURE_GET_VC_INFO %d\n",
				 (UINT16) *feature_data);
		pvcinfo =
			(struct SENSOR_VC_INFO_STRUCT
			 *)(uintptr_t) (*(feature_data + 1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)pvcinfo,
				   (void *)&SENSOR_VC_INFO[2],
				   sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
		default:
			memcpy((void *)pvcinfo,
				   (void *)&SENSOR_VC_INFO[0],
				   sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		pr_debug
		("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n",
		 (UINT16) *feature_data);

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:

			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:

			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
			break;
		default:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) = 1;
			break;
		}
		break;
	case SENSOR_FEATURE_SET_PDAF:
		pr_debug("PDAF mode :%d\n", *feature_data_16);
		imgsensor.pdaf_mode = *feature_data_16;
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
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				(imgsensor_info.cap.pclk /
				 (imgsensor_info.cap.linelength - 80)) *
				imgsensor_info.cap.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				(imgsensor_info.normal_video.pclk /
				 (imgsensor_info.normal_video.linelength - 80))
				 *imgsensor_info.normal_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				(imgsensor_info.hs_video.pclk /
				 (imgsensor_info.hs_video.linelength - 80)) *
				imgsensor_info.hs_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				(imgsensor_info.slim_video.pclk /
				 (imgsensor_info.slim_video.linelength - 80)) *
				imgsensor_info.slim_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				(imgsensor_info.slim_video.pclk /
				 (imgsensor_info.slim_video.linelength - 80)) *
				imgsensor_info.slim_video.grabwindow_width;

			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				(imgsensor_info.pre.pclk /
				 (imgsensor_info.pre.linelength - 80)) *
				imgsensor_info.pre.grabwindow_width;
			break;
		}
		break;

	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
		fps = (MUINT32) (*(feature_data + 2));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.cap.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.normal_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.hs_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.slim_video.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.custom1.mipi_pixel_rate;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *) (uintptr_t) (*(feature_data + 1)) =
				imgsensor_info.pre.mipi_pixel_rate;
			break;
		}

		break;

//cxc long exposure >
		case SENSOR_FEATURE_GET_AE_EFFECTIVE_FRAME_FOR_LE:
			*feature_return_para_32 =
				imgsensor.current_ae_effective_frame;
			break;
		case SENSOR_FEATURE_GET_AE_FRAME_MODE_FOR_LE:
			memcpy(feature_return_para_32, &imgsensor.ae_frm_mode,
				sizeof(struct IMGSENSOR_AE_FRM_MODE));
			break;
//cxc long exposure <

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

UINT32
MOT_COFUL_S5KJN1_QTECH_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{

	if (pfFunc != NULL)
		*pfFunc = &sensor_func;
	return ERROR_NONE;
}
