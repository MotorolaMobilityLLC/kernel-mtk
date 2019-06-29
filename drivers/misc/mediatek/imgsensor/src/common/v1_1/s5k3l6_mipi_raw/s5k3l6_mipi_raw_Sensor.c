/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 S5K3L6mipiraw_sensor.c
 *
 * Project:
 * --------
 *	 ALPS MT6739
 *
 * Description:
 * ------------
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k3l6_mipi_raw_Sensor.h"

#define FPT_PDAF_SUPPORT   //for pdaf switch

/****************************Modify Following Strings for Debug****************************/
#define PFX "S5K3L6"
#define LOG_INF(format, args...)    pr_err(PFX "[%s] " format, __FUNCTION__, ##args)
//#define LOG_INF printk

#define LOG_1 LOG_INF("S5K3L6,MIPI 4LANE\n")
#define S5K3L6_EEPROM_SIZE 3871
#define EEPROM_DATA_PATH "/data/vendor/camera_dump/s5k3l6_eeprom_data.bin"
//#define SENSORDB LOG_INF
/****************************   Modify end    *******************************************/

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static bool bIsLongExposure = KAL_FALSE;

static MUINT32 g_sync_mode = SENSOR_MASTER_SYNC_MODE;

static imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5K3L6_SENSOR_ID,		//Sensor ID Value: 0x30C8//record sensor id defined in Kd_imgsensor.h
	.checksum_value =  0x143d0c73,		/* checksum value for Camera Auto Test */
	.pre = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4896,				//record different mode's linelength
		.framelength = 3260,			//record different mode's framelength
		.startx= 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 480000000,
	},
	.cap = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4896,//5808,				//record different mode's linelength
		.framelength = 3260,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 480000000,
	},
	.normal_video = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4896,//5808,				//record different mode's linelength
		.framelength = 3260,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 4208,		//record different mode's width of grabwindow
		.grabwindow_height = 3120,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 480000000,
	},
	.hs_video = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4896,				//record different mode's linelength
		.framelength = 816,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1280,		//record different mode's width of grabwindow
		.grabwindow_height = 720,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 1200,
		.mipi_pixel_rate = 480000000,
	},
	.slim_video = {
		.pclk = 480000000,				//record different mode's pclk
		.linelength  = 4896,				//record different mode's linelength
		.framelength = 3260,			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width  = 1920,		//record different mode's width of grabwindow
		.grabwindow_height = 1080,		//record different mode's height of grabwindow
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 300,
		.mipi_pixel_rate = 208000000,
	},

	.margin = 5,			//sensor framelength & shutter margin
	.min_shutter = 4,		//min shutter
	.max_frame_length = 0xFFFF,//REG0x0202 <=REG0x0340-5//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,	//shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
	.frame_time_delay_frame = 1, // The delay frame of setting frame length
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num ,don't support Slow motion

	.cap_delay_frame = 3,		//enter capture delay frame num
	.pre_delay_frame = 3, 		//enter preview delay frame num
	.video_delay_frame = 3,		//enter video delay frame num
	.hs_video_delay_frame = 3,	//enter high speed video  delay frame num
	.slim_video_delay_frame = 3,//enter slim video delay frame num

	.isp_driving_current = ISP_DRIVING_4MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = 1,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
	.i2c_addr_table = {0x5a, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
	.i2c_speed = 400, /* i2c read/write speed */
};

static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x200,					//current shutter
	.gain = 0x200,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x5a,//record current sensor's i2c write id
	.current_ae_effective_frame = 1, //number of frames in effect for long exposure。if N+1 take effect，the value is 1；
};

/* Sensor output window information*/
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{
 { 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // Preview
 { 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // capture
 { 4208, 3120,	  0,  	0, 4208, 3120, 4208, 3120,   0,	0, 4208, 3120, 	 0, 0, 4208, 3120}, // video
 { 4208, 3120,	184,  480, 3840, 2160, 1280,  720,   0,	0, 1280,  720, 	 0, 0, 1280,  720}, //hight speed video
 { 4208, 3120,	184,  480, 3840, 2160, 1920, 1080,   0,	0, 1920, 1080, 	 0, 0, 1920, 1080},// slim video
};

#ifdef FPT_PDAF_SUPPORT
static SET_PD_BLOCK_INFO_T imgsensor_pd_info =
 //for 3L6
{
    .i4OffsetX = 24,
    .i4OffsetY = 24,
    .i4PitchX = 64,
    .i4PitchY = 64,
    .i4PairNum =16,
    .i4SubBlkW =16,
    .i4SubBlkH =16,
    .i4BlockNumX = 65,
    .i4BlockNumY = 48,
    .iMirrorFlip = 0,
    .i4PosL = {{28,31},{80,31},{44,35},{64,35},{32,51},{76,51},{48,55},{60,55},{48,63},{60,63},{32,67},{76,67},{44,83},{64,83},{28,87},{80,87}},
    .i4PosR = {{28,35},{80,35},{44,39},{64,39},{32,47},{76,47},{48,51},{60,51},{48,67},{60,67},{32,71},{76,71},{44,79},{64,79},{28,83},{80,83}},
};
#endif

static kal_uint16 read_cmos_sensor_byte(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

    //kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd , 2, (u8*)&get_byte,1,imgsensor.i2c_write_id);
    return get_byte;
}

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

    //kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
    return get_byte;
}

static void write_cmos_sensor_byte(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

    //kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

    //kdSetI2CSpeed(imgsensor_info.i2c_speed); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pusendcmd , 4, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	/* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	write_cmos_sensor(0x0342, imgsensor.line_length & 0xFFFF);
}	/*	set_dummy  */

static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

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



static kal_uint32 streaming_control(kal_bool enable)
{
    int timeout = 200;//(10000 / imgsensor.current_fps) + 1;
    int i = 0;
    int framecnt = 0;

    LOG_INF("streaming_enable(0= Sw Standby,1= streaming): %d\n", enable);
    if (enable) {
        write_cmos_sensor_byte(0x3C1E, 0x01);
        write_cmos_sensor_byte(0x0100, 0x01);
        write_cmos_sensor_byte(0x3C1E, 0x00);
        mdelay(10);
    } else {
        write_cmos_sensor_byte(0x0100, 0x00);
        for (i = 0; i < timeout; i++) {
            mdelay(10);
            framecnt = read_cmos_sensor_byte(0x0005);
            if ( framecnt == 0xFF) {
                LOG_INF(" Stream Off OK at i=%d.\n", i);
                return ERROR_NONE;
            }
        }
        LOG_INF("Stream Off Fail! framecnt= %d.\n", framecnt);
    }
    return ERROR_NONE;
}

static void set_shutter(kal_uint32 shutter)
{
	unsigned long flags;
	int i = 0;
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

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296,0);
		else if(realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146,0);
		else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	if(shutter > 65530) {  //linetime=10160/960000000<< maxshutter=3023622-line=32s
		/*enter long exposure mode */
		kal_uint32 exposure_time;
		kal_uint32 new_framelength;
		kal_uint32 long_shutter;
		kal_uint32 temp1_0342 = 0, temp2_0343 = 0;
		kal_uint32 long_shutter_linelenght = 0;
		int timeout = 200;
		int framecnt = 0;

		bIsLongExposure = KAL_TRUE;
		LOG_INF(" enter long exposure mode\n");

		/* Calculate value need by long exposure setting*/
		exposure_time = shutter/100;//ms
		if(exposure_time < 6500) {
			temp1_0342 = 0x13;
			temp2_0343 = 0x20;
			long_shutter_linelenght = 4896;
			LOG_INF("  1s~7s exposure_time = %d\n",exposure_time);
		} else if (6500 <= exposure_time  && exposure_time <= 22200) {
			temp1_0342 = 0x3f;
			temp2_0343 = 0x90;
			long_shutter_linelenght = 16272;
			LOG_INF("  7s~22.2s exposure_time = %d\n",exposure_time);
		} else if (22200 < exposure_time  && exposure_time <= 23500) {
			temp1_0342 = 0x3f;
			temp2_0343 = 0x90;
			long_shutter_linelenght = 17216;
			LOG_INF("  22.2s~23.5s exposure_time = %d\n",exposure_time);
		}
		 //shutter unit is S.used by 0x202 and 0x20
		long_shutter = (shutter*48)/(long_shutter_linelenght/10);
		new_framelength = long_shutter+2; //used by 0x340 and 0x341
		LOG_INF(" Calc long exposure_time=%dms,long_shutter=%d, framelength=%d.\n",\
			      exposure_time,long_shutter, new_framelength);

		/*stream off */
		streaming_control(KAL_FALSE);

		/*setting for long exposure*/
		write_cmos_sensor_byte(0x0307, 0x60);
		write_cmos_sensor_byte(0x3C1F, 0x03);
		write_cmos_sensor_byte(0x030D, 0x03);
		write_cmos_sensor_byte(0x030E, 0x00);
		write_cmos_sensor_byte(0x030F, 0x78);
		write_cmos_sensor_byte(0x3C17, 0x04);
		write_cmos_sensor_byte(0x0820, 0x00);
		write_cmos_sensor_byte(0x0821, 0x78);
		write_cmos_sensor_byte(0x38C5, 0x03);
		write_cmos_sensor_byte(0x38D9, 0x00);
		write_cmos_sensor_byte(0x38DB, 0x08);
		write_cmos_sensor_byte(0x38DD, 0x13);
		write_cmos_sensor_byte(0x38C3, 0x06);
		write_cmos_sensor_byte(0x38C1, 0x00);
		write_cmos_sensor_byte(0x38D7, 0x0F);
		write_cmos_sensor_byte(0x38D5, 0x03);
		write_cmos_sensor_byte(0x38B1, 0x01);
		write_cmos_sensor_byte(0x3932, 0x20);
		write_cmos_sensor_byte(0x3938, 0x20);
		write_cmos_sensor_byte(0x0340, (new_framelength&0xFF00)>>8);
		write_cmos_sensor_byte(0x0341, (new_framelength&0x00FF));
		write_cmos_sensor_byte(0x0342, temp1_0342);
		write_cmos_sensor_byte(0x0343, temp2_0343);
		write_cmos_sensor_byte(0x0202, (long_shutter&0xFF00)>>8);
		write_cmos_sensor_byte(0x0203, (long_shutter&0x00FF));

		/*stream on*/
		write_cmos_sensor_byte(0x3C1E, 0x01);
		write_cmos_sensor_byte(0x0100, 0x01);
		write_cmos_sensor_byte(0x3C1E, 0x00);
		for (i = 0; i < timeout; i++) {
			mdelay(10);
			framecnt = read_cmos_sensor_byte(0x0005);
			if ( framecnt == 0xFF) {
				LOG_INF(" Stream On OK at i=%d.\n", i);
				break;
			}
		}

		/* Frame exposure mode customization for LE*/
		imgsensor.ae_frm_mode.frame_mode_1 = IMGSENSOR_AE_MODE_SE;
		imgsensor.ae_frm_mode.frame_mode_2 = IMGSENSOR_AE_MODE_SE;
		imgsensor.current_ae_effective_frame = 1;
		LOG_INF(" long exposure stream on-\n");
	} else {
		/*normal mode*/
		if (bIsLongExposure == KAL_TRUE) {
			bIsLongExposure = KAL_FALSE;
			LOG_INF("[Exit long shutter + ]  shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
			/*stream off*/
			streaming_control(KAL_FALSE);
			/*setting for normal*/
			write_cmos_sensor_byte(0x0307, 0x78);
			write_cmos_sensor_byte(0x3C1F, 0x00);
			write_cmos_sensor_byte(0x030D, 0x03);
			write_cmos_sensor_byte(0x030E, 0x00);
			write_cmos_sensor_byte(0x030F, 0x4B);
			write_cmos_sensor_byte(0x3C17, 0x00);
			write_cmos_sensor_byte(0x0820, 0x04);
			write_cmos_sensor_byte(0x0821, 0xB0);
			write_cmos_sensor_byte(0x38C5, 0x09);
			write_cmos_sensor_byte(0x38D9, 0x2A);
			write_cmos_sensor_byte(0x38DB, 0x0A);
			write_cmos_sensor_byte(0x38DD, 0x0B);
			write_cmos_sensor_byte(0x38C3, 0x0A);
			write_cmos_sensor_byte(0x38C1, 0x0F);
			write_cmos_sensor_byte(0x38D7, 0x0A);
			write_cmos_sensor_byte(0x38D5, 0x09);
			write_cmos_sensor_byte(0x38B1, 0x0F);
			write_cmos_sensor_byte(0x3932, 0x18);
			write_cmos_sensor_byte(0x3938, 0x00);

			write_cmos_sensor_byte(0x0340, 0x0C);
			write_cmos_sensor_byte(0x0341, 0xBC);
			write_cmos_sensor_byte(0x0342, 0x13);
			write_cmos_sensor_byte(0x0343, 0x20);
			write_cmos_sensor_byte(0x0202, 0x03);
			write_cmos_sensor_byte(0x0203, 0xDE);
			/*stream on*/
			streaming_control(KAL_TRUE);
			LOG_INF("[Exit long shutter - ] shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

		} else {
			shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;
			write_cmos_sensor(0x0202, shutter & 0xFFFF);
			LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
		}
		imgsensor.current_ae_effective_frame = 1;
	}
}

static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	/* LOG_INF("shutter =%d, frame_time =%d\n", shutter, frame_time); */

	/* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
	/* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */

	/* OV Recommend Solution */
	/* if shutter bigger than frame_length, should extend frame length first */
	spin_lock(&imgsensor_drv_lock);
	/*Change frame time */
	if (frame_length > 1)
		dummy_line = frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;

	/*  */
	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length */
			write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	/* Update Shutter */
	write_cmos_sensor(0X0202, shutter & 0xFFFF);

	LOG_INF("Add for N3D! shutterlzl =%d, framelength =%d\n", shutter, imgsensor.frame_length);
}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0000;
	//gain = 64 = 1x real gain.
    reg_gain = gain/2;
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
  //gain = 64 = 1x real gain.
	kal_uint16 reg_gain;
	LOG_INF("set_gain %d \n", gain);
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

	write_cmos_sensor(0x0204, (reg_gain&0xFFFF));
	return gain;
}	/*	set_gain  */

//ihdr_write_shutter_gain not support for s5k3L6
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
	spin_lock(&imgsensor_drv_lock);
	imgsensor.mirror= image_mirror;
	spin_unlock(&imgsensor_drv_lock);
	switch (image_mirror) {
		case IMAGE_NORMAL:
			write_cmos_sensor(0x0101,0X00); //GR
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor(0x0101,0X01); //R
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor(0x0101,0X02); //B
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor(0x0101,0X03); //GB
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
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}	/*	night_mode	*/

static void sensor_init(void)
{
	LOG_INF("E\n");

	write_cmos_sensor(0x0100,0x0000);
	mdelay(3);
	write_cmos_sensor(0x3084,0x1314);
	write_cmos_sensor(0x3266,0x0001);
	write_cmos_sensor(0x3242,0x2020);
	write_cmos_sensor(0x306A,0x2F4C);
	write_cmos_sensor(0x306C,0xCA01);
	write_cmos_sensor(0x307A,0x0D20);
	write_cmos_sensor(0x309E,0x002D);
	write_cmos_sensor(0x3072,0x0013);
	write_cmos_sensor(0x3074,0x0977);
	write_cmos_sensor(0x3076,0x9411);
	write_cmos_sensor(0x3024,0x0016);
	write_cmos_sensor(0x3070,0x3D00);
	write_cmos_sensor(0x3002,0x0E00);
	write_cmos_sensor(0x3006,0x1000);
	write_cmos_sensor(0x300A,0x0C00);
	write_cmos_sensor(0x3010,0x0400);
	write_cmos_sensor(0x3018,0xC500);
	write_cmos_sensor(0x303A,0x0204);
	write_cmos_sensor(0x3452,0x0001);
	write_cmos_sensor(0x3454,0x0001);
	write_cmos_sensor(0x3456,0x0001);
	write_cmos_sensor(0x3458,0x0001);
	write_cmos_sensor(0x345a,0x0002);
	write_cmos_sensor(0x345C,0x0014);
	write_cmos_sensor(0x345E,0x0002);
	write_cmos_sensor(0x3460,0x0014);
	write_cmos_sensor(0x3464,0x0006);
	write_cmos_sensor(0x3466,0x0012);
	write_cmos_sensor(0x3468,0x0012);
	write_cmos_sensor(0x346A,0x0012);
	write_cmos_sensor(0x346C,0x0012);
	write_cmos_sensor(0x346E,0x0012);
	write_cmos_sensor(0x3470,0x0012);
	write_cmos_sensor(0x3472,0x0008);
	write_cmos_sensor(0x3474,0x0004);
	write_cmos_sensor(0x3476,0x0044);
	write_cmos_sensor(0x3478,0x0004);
	write_cmos_sensor(0x347A,0x0044);
	write_cmos_sensor(0x347E,0x0006);
	write_cmos_sensor(0x3480,0x0010);
	write_cmos_sensor(0x3482,0x0010);
	write_cmos_sensor(0x3484,0x0010);
	write_cmos_sensor(0x3486,0x0010);
	write_cmos_sensor(0x3488,0x0010);
	write_cmos_sensor(0x348A,0x0010);
	write_cmos_sensor(0x348E,0x000C);
	write_cmos_sensor(0x3490,0x004C);
	write_cmos_sensor(0x3492,0x000C);
	write_cmos_sensor(0x3494,0x004C);
	write_cmos_sensor(0x3496,0x0020);
	write_cmos_sensor(0x3498,0x0006);
	write_cmos_sensor(0x349A,0x0008);
	write_cmos_sensor(0x349C,0x0008);
	write_cmos_sensor(0x349E,0x0008);
	write_cmos_sensor(0x34A0,0x0008);
	write_cmos_sensor(0x34A2,0x0008);
	write_cmos_sensor(0x34A4,0x0008);
	write_cmos_sensor(0x34A8,0x001A);
	write_cmos_sensor(0x34AA,0x002A);
	write_cmos_sensor(0x34AC,0x001A);
	write_cmos_sensor(0x34AE,0x002A);
	write_cmos_sensor(0x34B0,0x0080);
	write_cmos_sensor(0x34B2,0x0006);
	write_cmos_sensor(0x32A2,0x0000);
	write_cmos_sensor(0x32A4,0x0000);
	write_cmos_sensor(0x32A6,0x0000);
	write_cmos_sensor(0x32A8,0x0000);
	write_cmos_sensor(0x3066,0x7E00);
	write_cmos_sensor(0x3004,0x0800);
	write_cmos_sensor(0x0100,0x0000);
}	/*	sensor_init  */


static void check_stream_is_off(void)
{
	int i = 0;
	UINT32 framecnt;

	for (i = 0; i < 100; i++)
	{
		framecnt = read_cmos_sensor(0x0005); /* waiting for sensor to  stop output  then  set the  setting */
		if (framecnt == 0xFF)
		{
			LOG_INF("stream is  off\\n");
			break;
		} else {
			LOG_INF("stream is not off\\n");
			mdelay(1);
		}
	}
}

static void preview_setting(void)
{
	LOG_INF("E\n");
	write_cmos_sensor(0x0100,0x0000);

	check_stream_is_off();
	write_cmos_sensor(0x0344,0x0008);
	write_cmos_sensor(0x0346,0x0008);
	write_cmos_sensor(0x0348,0x1077);
	write_cmos_sensor(0x034A,0x0C37);
	write_cmos_sensor(0x034C,0x1070);
	write_cmos_sensor(0x034E,0x0C30);
	write_cmos_sensor(0x0900,0x0000);
	write_cmos_sensor(0x0380,0x0001);
	write_cmos_sensor(0x0382,0x0001);
	write_cmos_sensor(0x0384,0x0001);
	write_cmos_sensor(0x0386,0x0001);
	write_cmos_sensor(0x0114,0x0330);
	write_cmos_sensor(0x0110,0x0002);
	write_cmos_sensor(0x0136,0x1800);
	write_cmos_sensor(0x0304,0x0004);
	write_cmos_sensor(0x0306,0x0078);
	write_cmos_sensor(0x3C1E,0x0000);
	write_cmos_sensor(0x030C,0x0003);
	write_cmos_sensor(0x030E,0x004B);
	write_cmos_sensor(0x3C16,0x0000);
	write_cmos_sensor(0x0300,0x0006);
	write_cmos_sensor(0x0342,0x1320);
	write_cmos_sensor(0x0340,0x0CBC);
	write_cmos_sensor(0x38C4,0x0009);
	write_cmos_sensor(0x38D8,0x002A);
	write_cmos_sensor(0x38DA,0x000A);
	write_cmos_sensor(0x38DC,0x000B);
	write_cmos_sensor(0x38C2,0x000A);
	write_cmos_sensor(0x38C0,0x000F);
	write_cmos_sensor(0x38D6,0x000A);
	write_cmos_sensor(0x38D4,0x0009);
	write_cmos_sensor(0x38B0,0x000F);
	write_cmos_sensor(0x3932,0x1800);
	write_cmos_sensor(0x3938,0x000C);
	write_cmos_sensor(0x0820,0x04B0);
	write_cmos_sensor(0x380C,0x0090);
	write_cmos_sensor(0x3064,0xFFCF);
	write_cmos_sensor(0x309C,0x0640);
	write_cmos_sensor(0x3090,0x8800);
	write_cmos_sensor(0x3238,0x000C);
	write_cmos_sensor(0x314A,0x5F00);
	//write_cmos_sensor(0x32B2,0x0000);
	//write_cmos_sensor(0x32B4,0x0000);
	//write_cmos_sensor(0x32B6,0x0000);
	//write_cmos_sensor(0x32B8,0x0000);
	write_cmos_sensor(0x3300,0x0000);
	write_cmos_sensor(0x3400,0x0000);
	write_cmos_sensor(0x3402,0x4E42);
	write_cmos_sensor(0x32B2,0x0006);
	write_cmos_sensor(0x32B4,0x0006);
	write_cmos_sensor(0x32B6,0x0006);
	write_cmos_sensor(0x32B8,0x0006);
	write_cmos_sensor(0x3C34,0x0048);
	write_cmos_sensor(0x3C36,0x3000);
	write_cmos_sensor(0x3C38,0x0020);
	write_cmos_sensor(0x393E,0x4000);
	write_cmos_sensor(0x3C1E,0x0100);
	write_cmos_sensor(0x0100,0x0100);
	write_cmos_sensor(0x3C1E,0x0000);
}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n", currefps);
        //$MV1[MCLK:24,Width:4208,Height:3120,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:1124,pvi_pclk_inverse:0]
        write_cmos_sensor(0x0100,0x0000);

        check_stream_is_off();

        write_cmos_sensor(0x0344,0x0008);
        write_cmos_sensor(0x0346,0x0008);
        write_cmos_sensor(0x0348,0x1077);
        write_cmos_sensor(0x034A,0x0C37);
        write_cmos_sensor(0x034C,0x1070);
        write_cmos_sensor(0x034E,0x0C30);
        write_cmos_sensor(0x0900,0x0000);
        write_cmos_sensor(0x0380,0x0001);
        write_cmos_sensor(0x0382,0x0001);
        write_cmos_sensor(0x0384,0x0001);
        write_cmos_sensor(0x0386,0x0001);
        write_cmos_sensor(0x0114,0x0330);
        write_cmos_sensor(0x0110,0x0002);
        write_cmos_sensor(0x0136,0x1800);
        write_cmos_sensor(0x0304,0x0004);
        write_cmos_sensor(0x0306,0x0078);
        write_cmos_sensor(0x3C1E,0x0000);
        write_cmos_sensor(0x030C,0x0003);
        write_cmos_sensor(0x030E,0x004B);
        write_cmos_sensor(0x3C16,0x0000);
        write_cmos_sensor(0x0300,0x0006);
        write_cmos_sensor(0x0342,0x1320);
        write_cmos_sensor(0x0340,0x0CBC);
        write_cmos_sensor(0x38C4,0x0009);
        write_cmos_sensor(0x38D8,0x002A);
        write_cmos_sensor(0x38DA,0x000A);
        write_cmos_sensor(0x38DC,0x000B);
        write_cmos_sensor(0x38C2,0x000A);
        write_cmos_sensor(0x38C0,0x000F);
        write_cmos_sensor(0x38D6,0x000A);
        write_cmos_sensor(0x38D4,0x0009);
        write_cmos_sensor(0x38B0,0x000F);
        write_cmos_sensor(0x3932,0x1800);
        write_cmos_sensor(0x3938,0x000C);
        write_cmos_sensor(0x0820,0x04B0);
        write_cmos_sensor(0x380C,0x0090);
        write_cmos_sensor(0x3064,0xFFCF);
        write_cmos_sensor(0x309C,0x0640);
        write_cmos_sensor(0x3090,0x8800);
        write_cmos_sensor(0x3238,0x000C);
        write_cmos_sensor(0x314A,0x5F00);
        //write_cmos_sensor(0x32B2,0x0000);
        //write_cmos_sensor(0x32B4,0x0000);
        //write_cmos_sensor(0x32B6,0x0000);
        //write_cmos_sensor(0x32B8,0x0000);
        write_cmos_sensor(0x3300,0x0000);
        write_cmos_sensor(0x3400,0x0000);
        write_cmos_sensor(0x3402,0x4E42);
        write_cmos_sensor(0x32B2,0x0006);
        write_cmos_sensor(0x32B4,0x0006);
        write_cmos_sensor(0x32B6,0x0006);
        write_cmos_sensor(0x32B8,0x0006);
        write_cmos_sensor(0x3C34,0x0048);
        write_cmos_sensor(0x3C36,0x3000);
        write_cmos_sensor(0x3C38,0x0020);
        write_cmos_sensor(0x393E,0x4000);
        write_cmos_sensor(0x3C1E,0x0100);
        write_cmos_sensor(0x0100,0x0100);
        write_cmos_sensor(0x3C1E,0x0000);

    write_cmos_sensor_byte(0x3C67,0x10);    //dual cam sync

}

static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("E! currefps:%d\n",currefps);
	write_cmos_sensor(0x0100,0x0000);
	check_stream_is_off();
	write_cmos_sensor(0x0344,0x0008);
	write_cmos_sensor(0x0346,0x0008);
	write_cmos_sensor(0x0348,0x1077);
	write_cmos_sensor(0x034A,0x0C37);
	write_cmos_sensor(0x034C,0x1070);
	write_cmos_sensor(0x034E,0x0C30);
	write_cmos_sensor(0x0900,0x0000);
	write_cmos_sensor(0x0380,0x0001);
	write_cmos_sensor(0x0382,0x0001);
	write_cmos_sensor(0x0384,0x0001);
	write_cmos_sensor(0x0386,0x0001);
	write_cmos_sensor(0x0114,0x0330);
	write_cmos_sensor(0x0110,0x0002);
	write_cmos_sensor(0x0136,0x1800);
	write_cmos_sensor(0x0304,0x0004);
	write_cmos_sensor(0x0306,0x0078);
	write_cmos_sensor(0x3C1E,0x0000);
	write_cmos_sensor(0x030C,0x0004);
	write_cmos_sensor(0x030E,0x0064);
	write_cmos_sensor(0x3C16,0x0000);
	write_cmos_sensor(0x0300,0x0006);
	write_cmos_sensor(0x0342,0x1320);
	write_cmos_sensor(0x0340,0x0CBC);
	write_cmos_sensor(0x38C4,0x0009);
	write_cmos_sensor(0x38D8,0x002A);
	write_cmos_sensor(0x38DA,0x000A);
	write_cmos_sensor(0x38DC,0x000B);
	write_cmos_sensor(0x38C2,0x000A);
	write_cmos_sensor(0x38C0,0x000F);
	write_cmos_sensor(0x38D6,0x000A);
	write_cmos_sensor(0x38D4,0x0009);
	write_cmos_sensor(0x38B0,0x000F);
	write_cmos_sensor(0x3932,0x1800);
	write_cmos_sensor(0x3938,0x000C);
	write_cmos_sensor(0x0820,0x04B0);
	write_cmos_sensor(0x380C,0x0090);
	write_cmos_sensor(0x3064,0xEFCF);
	write_cmos_sensor(0x309C,0x0640);
	write_cmos_sensor(0x3090,0x8800);
	write_cmos_sensor(0x3238,0x000C);
	write_cmos_sensor(0x314A,0x5F00);
	write_cmos_sensor(0x32B2,0x0000);
	write_cmos_sensor(0x32B4,0x0000);
	write_cmos_sensor(0x32B6,0x0000);
	write_cmos_sensor(0x32B8,0x0000);
	write_cmos_sensor(0x3300,0x0000);
	write_cmos_sensor(0x3400,0x0000);
	write_cmos_sensor(0x3402,0x4E42);
	write_cmos_sensor(0x32B2,0x0006);
	write_cmos_sensor(0x32B4,0x0006);
	write_cmos_sensor(0x32B6,0x0006);
	write_cmos_sensor(0x32B8,0x0006);
	write_cmos_sensor(0x3C34,0x0008);
	write_cmos_sensor(0x3C36,0x0000);
	write_cmos_sensor(0x3C38,0x0000);
	write_cmos_sensor(0x393E,0x4000);
	write_cmos_sensor(0x3C1E,0x0100);
	write_cmos_sensor(0x0100,0x0100);
	write_cmos_sensor(0x3C1E,0x0000);
}

static void hs_video_setting(void)
{
	LOG_INF("E\n");
  //$MV1[MCLK:24,Width:1280,Height:720,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:186,pvi_pclk_inverse:0]
	write_cmos_sensor(0x0100, 0x0000);

    check_stream_is_off();

	write_cmos_sensor(0x0344,0x0340);
	write_cmos_sensor(0x0346,0x0350);
	write_cmos_sensor(0x0348,0x0D3F);
	write_cmos_sensor(0x034A,0x08EF);
	write_cmos_sensor(0x034C,0x0500);
	write_cmos_sensor(0x034E,0x02D0);
	write_cmos_sensor(0x0900,0x0122);
	write_cmos_sensor(0x0380,0x0001);
	write_cmos_sensor(0x0382,0x0001);
	write_cmos_sensor(0x0384,0x0001);
	write_cmos_sensor(0x0386,0x0003);
	write_cmos_sensor(0x0114,0x0330);
	write_cmos_sensor(0x0110,0x0002);
	write_cmos_sensor(0x0136,0x1800);
	write_cmos_sensor(0x0304,0x0004);
	write_cmos_sensor(0x0306,0x0078);
	write_cmos_sensor(0x3C1E,0x0000);
	write_cmos_sensor(0x030C,0x0003);
	write_cmos_sensor(0x030E,0x0059);
	write_cmos_sensor(0x3C16,0x0002);
	write_cmos_sensor(0x0300,0x0006);
	write_cmos_sensor(0x0342,0x1320);
	write_cmos_sensor(0x0340,0x0330);
	write_cmos_sensor(0x38C4,0x0002);
	write_cmos_sensor(0x38D8,0x0008);
	write_cmos_sensor(0x38DA,0x0003);
	write_cmos_sensor(0x38DC,0x0004);
	write_cmos_sensor(0x38C2,0x0003);
	write_cmos_sensor(0x38C0,0x0001);
	write_cmos_sensor(0x38D6,0x0003);
	write_cmos_sensor(0x38D4,0x0002);
	write_cmos_sensor(0x38B0,0x0004);
	write_cmos_sensor(0x3932,0x1800);
	write_cmos_sensor(0x3938,0x000C);
	write_cmos_sensor(0x0820,0x0162);
	write_cmos_sensor(0x380C,0x003D);
	write_cmos_sensor(0x3064,0xFFCF);
	write_cmos_sensor(0x309C,0x0640);
	write_cmos_sensor(0x3090,0x8000);
	write_cmos_sensor(0x3238,0x000B);
	write_cmos_sensor(0x314A,0x5F02);
	//write_cmos_sensor(0x32B2,0x0006);
	//write_cmos_sensor(0x32B4,0x0006);
	//write_cmos_sensor(0x32B6,0x0006);
	//write_cmos_sensor(0x32B8,0x0006);
	write_cmos_sensor(0x3300,0x0000);
	write_cmos_sensor(0x3400,0x0000);
	write_cmos_sensor(0x3402,0x4E46);
	write_cmos_sensor(0x32B2,0x0008);
	write_cmos_sensor(0x32B4,0x0008);
	write_cmos_sensor(0x32B6,0x0008);
	write_cmos_sensor(0x32B8,0x0008);
	write_cmos_sensor(0x3C34,0x0048);
	write_cmos_sensor(0x3C36,0x3000);
	write_cmos_sensor(0x3C38,0x0024);
	write_cmos_sensor(0x393E,0x4000);
	write_cmos_sensor(0x303A,0x0204);
	write_cmos_sensor(0x3034,0x4B01);
	write_cmos_sensor(0x3036,0x0029);
	write_cmos_sensor(0x3032,0x4800);
	write_cmos_sensor(0x320E,0x049E);
	write_cmos_sensor(0x3C1E, 0x0100);
	write_cmos_sensor(0x0100, 0x0100);
	write_cmos_sensor(0x3C1E, 0x0000);
}

static void slim_video_setting(void)
{
	LOG_INF("E\n");
	//$MV1[MCLK:24,Width:1920,Height:1080,Format:MIPI_Raw10,mipi_lane:4,mipi_datarate:520,pvi_pclk_inverse:0]
	write_cmos_sensor(0x0100, 0x0000);
	check_stream_is_off();
	write_cmos_sensor(0x0344,0x00C0);
	write_cmos_sensor(0x0346,0x01E8);
	write_cmos_sensor(0x0348,0x0FBF);
	write_cmos_sensor(0x034A,0x0A57);
	write_cmos_sensor(0x034C,0x0780);
	write_cmos_sensor(0x034E,0x0438);
	write_cmos_sensor(0x0900,0x0122);
	write_cmos_sensor(0x0380,0x0001);
	write_cmos_sensor(0x0382,0x0001);
	write_cmos_sensor(0x0384,0x0001);
	write_cmos_sensor(0x0386,0x0003);
	write_cmos_sensor(0x0114,0x0330);
	write_cmos_sensor(0x0110,0x0002);
	write_cmos_sensor(0x0136,0x1800);
	write_cmos_sensor(0x0304,0x0004);
	write_cmos_sensor(0x0306,0x0078);
	write_cmos_sensor(0x3C1E,0x0000);
	write_cmos_sensor(0x030C,0x0003);
	write_cmos_sensor(0x030E,0x0082);
	write_cmos_sensor(0x3C16,0x0002);
	write_cmos_sensor(0x0300,0x0006);
	write_cmos_sensor(0x0342,0x1320);
	write_cmos_sensor(0x0340,0x0CBC);
	write_cmos_sensor(0x38C4,0x0004);
	write_cmos_sensor(0x38D8,0x000F);
	write_cmos_sensor(0x38DA,0x0005);
	write_cmos_sensor(0x38DC,0x0005);
	write_cmos_sensor(0x38C2,0x0004);
	write_cmos_sensor(0x38C0,0x0003);
	write_cmos_sensor(0x38D6,0x0004);
	write_cmos_sensor(0x38D4,0x0003);
	write_cmos_sensor(0x38B0,0x0006);
	write_cmos_sensor(0x3932,0x2000);
	write_cmos_sensor(0x3938,0x000C);
	write_cmos_sensor(0x0820,0x0208);
	write_cmos_sensor(0x380C,0x0049);
	write_cmos_sensor(0x3064,0xEFCF);
	write_cmos_sensor(0x309C,0x0640);
	write_cmos_sensor(0x3090,0x8000);
	write_cmos_sensor(0x3238,0x000B);
	write_cmos_sensor(0x314A,0x5F02);
	write_cmos_sensor(0x32B2,0x0003);
	write_cmos_sensor(0x32B4,0x0003);
	write_cmos_sensor(0x32B6,0x0003);
	write_cmos_sensor(0x32B8,0x0003);
	write_cmos_sensor(0x3300,0x0000);
	write_cmos_sensor(0x3400,0x0000);
	write_cmos_sensor(0x3402,0x4E46);
	write_cmos_sensor(0x32B2,0x0008);
	write_cmos_sensor(0x32B4,0x0008);
	write_cmos_sensor(0x32B6,0x0008);
	write_cmos_sensor(0x32B8,0x0008);
	write_cmos_sensor(0x3C34,0x0008);
	write_cmos_sensor(0x3C36,0x0000);
	write_cmos_sensor(0x3C38,0x0000);
	write_cmos_sensor(0x393E,0x4000);
	write_cmos_sensor(0x3C1E, 0x0100);
	write_cmos_sensor(0x0100, 0x0100);
	write_cmos_sensor(0x3C1E, 0x0000);

}

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor_byte(0x0000) << 8) | read_cmos_sensor_byte(0x0001));
}

#define OTP_3L6 1
#define INCLUDE_NO_OTP_3L6 1

#if OTP_3L6
#define S5K3L6_EEPROM_SLAVE_ADD 0xB0
#define S5K3L6_SENSOR_IIC_SLAVE_ADD 0x5a
#define S5K3L6_OFILM_MODULE_ID  0x4f46

static uint8_t s5k3l6_eeprom[S5K3L6_EEPROM_SIZE] = {0};

static calibration_status_t mnf_status;
static calibration_status_t af_status;
static calibration_status_t awb_status;
static calibration_status_t lsc_status;
static calibration_status_t pdaf_status;
static calibration_status_t dual_status;

static void s5k3l6_read_data_from_eeprom(kal_uint8 slave, kal_uint32 start_add, uint32_t size)
{
	int i = 0;
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = slave;
	spin_unlock(&imgsensor_drv_lock);

	//read eeprom data
	for (i = 0; i < size; i ++) {
		s5k3l6_eeprom[i] = read_cmos_sensor(start_add);
		start_add ++;
	}

	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = S5K3L6_SENSOR_IIC_SLAVE_ADD;
	spin_unlock(&imgsensor_drv_lock);
}

static void s5k3l6_eeprom_dump_bin(const char *file_name, uint32_t size, const void *data)
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

static calibration_status_t s5k3l6_check_manufacturing_data(void *data)
{
	struct s5k3l6_eeprom_t *eeprom = (struct s5k3l6_eeprom_t*)data;

	if (!eeprom_util_check_crc16(data, S5K3L6_EEPROM_CRC_MANUFACTURING_SIZE,
		convert_crc(eeprom->manufacture_crc16))) {
			LOG_INF("Manufacturing CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Manufacturing CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t s5k3l6_check_af_data(void *data)
{
	struct s5k3l6_eeprom_t *eeprom = (struct s5k3l6_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->af_infinity_cal_dac, S5K3L6_EEPROM_CRC_AF_CAL_SIZE,
		convert_crc(eeprom->af_cal_crc16))) {
		LOG_INF("Autofocus CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("Autofocus CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t s5k3l6_check_awb_data(void *data)
{
	struct s5k3l6_eeprom_t *eeprom = (struct s5k3l6_eeprom_t*)data;
	awb_t unit;
	awb_t golden;
	awb_limit_t golden_limit;

	if(!eeprom_util_check_crc16(eeprom->cie_src_1_ev,
		S5K3L6_EEPROM_CRC_AWB_CAL_SIZE,
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

static calibration_status_t s5k3l6_check_lsc_data(void *data)
{
	struct s5k3l6_eeprom_t *eeprom = (struct s5k3l6_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->lsc_data, S5K3L6_EEPROM_CRC_LSC_SIZE,
		convert_crc(eeprom->lsc_crc16))) {
		LOG_INF("LSC CRC Fails!");
		return CRC_FAILURE;
	}
	LOG_INF("LSC CRC Pass");
	return NO_ERRORS;
}

static calibration_status_t s5k3l6_check_pdaf_data(void *data)
{
	struct s5k3l6_eeprom_t *eeprom = (struct s5k3l6_eeprom_t*)data;

	if (!eeprom_util_check_crc16(eeprom->pdaf_output1_data, S5K3L6_EEPROM_CRC_PDAF_OUTPUT1_SIZE,
		convert_crc(eeprom->pdaf_output1_crc16))) {
		LOG_INF("PDAF OUTPUT1 CRC Fails!");
		return CRC_FAILURE;
	}

	if (!eeprom_util_check_crc16(eeprom->pdaf_output2_data, S5K3L6_EEPROM_CRC_PDAF_OUTPUT2_SIZE,
		convert_crc(eeprom->pdaf_crc16))) {
		LOG_INF("PDAF OUTPUT2 CRC Fails!");
		return CRC_FAILURE;
	}

	LOG_INF("PDAF CRC Pass");
	return NO_ERRORS;
}

static void s5k3l6_eeprom_format_calibration_data(void *data)
{
	if (NULL == data) {
		LOG_INF("data is NULL");
		return;
	}

	mnf_status = s5k3l6_check_manufacturing_data(data);
	af_status = s5k3l6_check_af_data(data);
	awb_status = s5k3l6_check_awb_data(data);
	lsc_status = s5k3l6_check_lsc_data(data);
	pdaf_status = s5k3l6_check_pdaf_data(data);
	dual_status = 0;

	LOG_INF("status mnf:%d, af:%d, awb:%d, lsc:%d, pdaf:%d, dual:%d",
		mnf_status, af_status, awb_status, lsc_status, pdaf_status, dual_status);
}

static void s5k3l6_eeprom_get_mnf_data(void *data,
		mot_calibration_mnf_t *mnf)
{
	int ret;
	struct s5k3l6_eeprom_t *eeprom = (struct s5k3l6_eeprom_t*)data;

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
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	//sensor have two i2c address 0x5b 0x5a & 0x21 0x20, we should detect the module used i2c address
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = return_sensor_id();
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id,*sensor_id,imgsensor_info.sensor_id);
				s5k3l6_read_data_from_eeprom(S5K3L6_EEPROM_SLAVE_ADD, 0x0000, S5K3L6_EEPROM_SIZE);
				s5k3l6_eeprom_dump_bin(EEPROM_DATA_PATH, S5K3L6_EEPROM_SIZE, (void *)s5k3l6_eeprom);
				s5k3l6_eeprom_format_calibration_data((void *)s5k3l6_eeprom);
				LOG_INF("This is s5k3l6");
				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail, i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id,*sensor_id,imgsensor_info.sensor_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 1;
	}

	if (*sensor_id != imgsensor_info.sensor_id) {
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
				LOG_INF("This is s5k3l6, i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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
	if (imgsensor_info.sensor_id != sensor_id){
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	/* initail sequence write in  */
	sensor_init();

	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
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
	set_mirror_flip(IMAGE_NORMAL);
	mdelay(2);

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
	LOG_INF("capture30fps: use cap30FPS's setting: %d fps!\n",imgsensor.current_fps/10);
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);
	set_mirror_flip(IMAGE_NORMAL);
	mdelay(2);

#if 0
	if(imgsensor.test_pattern == KAL_TRUE)
	{
		//write_cmos_sensor(0x5002,0x00);
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
	set_mirror_flip(IMAGE_NORMAL);

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
	set_mirror_flip(IMAGE_NORMAL);

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
	set_mirror_flip(IMAGE_NORMAL);
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

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame;	/* The delay frame of setting frame length */
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
	s5k3l6_eeprom_get_mnf_data((void *)s5k3l6_eeprom, &sensor_info->mnf_calibration);

	//	#ifdef FPT_PDAF_SUPPORT
#ifdef FPT_PDAF_SUPPORT
	sensor_info->PDAF_Support = PDAF_SUPPORT_RAW;
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
			if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
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
			if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
			imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
			break;
	default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			if (imgsensor.frame_length > imgsensor.shutter)
				set_dummy();
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
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor(0x3202, 0x0080);
		write_cmos_sensor(0x3204, 0x0080);
		write_cmos_sensor(0x3206, 0x0080);
		write_cmos_sensor(0x3208, 0x0080);
		write_cmos_sensor(0x3232, 0x0000);
		write_cmos_sensor(0x3234, 0x0000);
		write_cmos_sensor(0x32a0, 0x0100);
		write_cmos_sensor(0x3300, 0x0001);
		write_cmos_sensor(0x3400, 0x0001);
		write_cmos_sensor(0x3402, 0x4e00);
		write_cmos_sensor(0x3268, 0x0000);
		write_cmos_sensor(0x0600, 0x0002);
	} else {
		// 0x5E00[8]: 1 enable,  0 disable
		// 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
		write_cmos_sensor(0x3202, 0x0000);
		write_cmos_sensor(0x3204, 0x0000);
		write_cmos_sensor(0x3206, 0x0000);
		write_cmos_sensor(0x3208, 0x0000);
		write_cmos_sensor(0x3232, 0x0000);
		write_cmos_sensor(0x3234, 0x0000);
		write_cmos_sensor(0x32a0, 0x0000);
		write_cmos_sensor(0x3300, 0x0000);
		write_cmos_sensor(0x3400, 0x0000);
		write_cmos_sensor(0x3402, 0x0000);
		write_cmos_sensor(0x3268, 0x0000);
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
	UINT16 *feature_return_para_16=(UINT16 *) feature_para;
	UINT16 *feature_data_16=(UINT16 *) feature_para;
	UINT32 *feature_return_para_32=(UINT32 *) feature_para;
	UINT32 *feature_data_32=(UINT32 *) feature_para;
	unsigned long long *feature_data=(unsigned long long *) feature_para;
	//unsigned long long *feature_return_para=(unsigned long long *) feature_para;

	SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
#ifdef FPT_PDAF_SUPPORT
	SET_PD_BLOCK_INFO_T *PDAFinfo;
#endif

	LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENOSR_FEATURE_GET_OFFSET_TO_START_OF_EXPOSURE:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 5500000;
		break;
	case SENSOR_FEATURE_GET_AE_EFFECTIVE_FRAME_FOR_LE:
		*feature_return_para_32 = imgsensor.current_ae_effective_frame;
		break;
	case SENSOR_FEATURE_GET_AE_FRAME_MODE_FOR_LE:
		memcpy(feature_return_para_32, &imgsensor.ae_frm_mode,
                         sizeof(struct IMGSENSOR_AE_FRM_MODE));
		break;
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
		LOG_INF("current fps :%d\n", (UINT32)*feature_data_32);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_SET_HDR:
		LOG_INF("ihdr enable :%d\n", (BOOL)*feature_data);
		spin_lock(&imgsensor_drv_lock);
		imgsensor.ihdr_en = (BOOL)*feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		//LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%lld\n", (UINT32)*feature_data);

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
#ifdef FPT_PDAF_SUPPORT
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n", *feature_data);

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
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%llu\n", *feature_data);
		PDAFinfo= (SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(SET_PD_BLOCK_INFO_T));
			LOG_INF("3L6 get pd data\n");
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		default:
			break;
		}
		break;
	case SENSOR_FEATURE_SET_PDAF:
		LOG_INF("PDAF mode :%d\n", *feature_data_16);
		/*imgsensor.pdaf_mode= *feature_data_16;*/
		break;
#endif

	case SENSOR_FEATURE_GET_SENSOR_SYNC_MODE_CAPACITY:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_SYNC_MODE_CAPACITY scenarioId:%lld\n", *feature_data);
		*feature_return_para_32 = SENSOR_MASTER_SYNC_MODE;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_GET_SENSOR_SYNC_MODE:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_SYNC_MODE scenarioId:%lld\n", *feature_data);
		*feature_return_para_32 = g_sync_mode;
		*feature_para_len = 4;
		break;
	case SENSOR_FEATURE_SET_SENSOR_SYNC_MODE:
		LOG_INF("SENSOR_FEATURE_SET_SENSOR_SYNC_MODE scenarioId:%lld\n", *feature_data);
		g_sync_mode = (MUINT32) (*feature_data_32);
		LOG_INF("[hwadd]mode = %d\n", g_sync_mode);
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) *feature_data, (UINT16) *(feature_data + 1));
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME\n");
		streaming_control(KAL_TRUE);
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
}    /*    feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K3L6_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	LOG_INF("S5K3L6_MIPI_RAW_SensorInit in\n");
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&sensor_func;
	return ERROR_NONE;
}	/*	S5K3L6_MIPI_RAW_SensorInit	*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<desuo.lu@reallytek.com>");
MODULE_DESCRIPTION("camera information");

