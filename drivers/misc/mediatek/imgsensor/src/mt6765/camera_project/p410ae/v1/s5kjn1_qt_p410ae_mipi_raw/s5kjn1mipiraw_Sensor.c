/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 S5Kjn1mipiraw_sensor.c
 *
 * Project:
 * --------
 *	 ALPS MT6735
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *  20150624: the first driver from 
 *  20150706: add pip 15fps setting
 *  20150716: 更新log的打印方法
 *  20150720: use non - continue mode
 *  15072011511229: add pdaf, the pdaf old has be delete by recovery
 *  15072011511229: add 旧的log兼容，新的log在这个版本不能打印log？？
 *  15072209190629: non - continue mode bandwith limited , has <tiaowen> , modify to continue mode
 *  15072209201129: modify not enter init_setting bug
 *  15072718000000: crc addd 0x49c09f86
 *  15072718000001: MODIFY LOG SWITCH
 *  15072811330000: ADD NON-CONTIUE MODE ,PREVIEW 29FPS,CAPTURE 29FPS
 					([TODO]REG0304 0786->0780  PREVEIW INCREASE TO 30FPS)
 *  15072813000000: modify a wrong setting at pip reg030e 0x119->0xc8
 *  15080409230000: pass!
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
#include "s5kjn1mipiraw_Sensor.h"

#ifdef CONFIG_TINNO_PRODUCT_INFO
#include "dev_info.h"
#endif

#define MULTI_WRITE 1

#if MULTI_WRITE
#define I2C_BUFFER_LEN 1020	/* trans# max is 255, each 3 bytes */
#else
#define I2C_BUFFER_LEN 4
#endif

//add camera info for v851
//#ifdef CONFIG_TINNO_PRODUCT_INFO
//#include <dev_info.h>
//#endif

/*===FEATURE SWITH===*/
#define FPTPDAFSUPPORT 1  //for pdaf switch
//#define FANPENGTAO  1  //for debug log
//#define USE_TNP_BURST 1
//#define LOG_INF LOG_INF_LOD
//#define NONCONTINUEMODE
/*===FEATURE SWITH===*/

#define EEPROM_WRITE_ID   0xa0
#define PFX "s5kjn1_qt_camera_sensor"
#define LOG_INF(format, args...)	pr_err(PFX "[%s] " format, __func__, ##args)

#ifndef AGOLD_S5KJN1_AWB_DISABLE
//extern void s5kjn1kw_MIPI_update_wb_register_from_otp(void);
#endif
//[agold][chenwei][20190313][end]

static MUINT8 pOtp_data[16];

static DEFINE_SPINLOCK(imgsensor_drv_lock);

static imgsensor_info_struct imgsensor_info = {
	.sensor_id = S5KJN1_QT_P410AE_SENSOR_ID,		//Sensor ID Value: 0xf8d1+1

	.checksum_value = 0x40631970,		//checksum value for Camera Auto Test

	.pre = {
		.pclk = 560000000,
		.linelength = 4584,             //5910,
		.framelength = 4064,            //3156,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 504000000,
	},

	.cap = {
		.pclk = 560000000,
		.linelength = 4584,
		.framelength = 4064,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 504000000,
	},
	.normal_video = {
		.pclk = 560000000,
		.linelength = 4584,
		.framelength = 4064,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 504000000,
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
		.max_framerate = 1200,
		.mipi_pixel_rate = 640000000,
	},
	.slim_video = {
		.pclk = 560000000,
		.linelength = 5910,
		.framelength = 3156,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 2296,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 430000000,   //The .mipi_pixel_rate value corresponding to the PIP-feature
	},
	/*.custom1 = {
		.pclk = 560000000,
		.linelength = 4584,
		.framelength = 4064,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4080,
		.grabwindow_height = 3072,
		.mipi_data_lp2hs_settle_dc = 85,
		.max_framerate = 300,
		.mipi_pixel_rate = 662400000,
	},*/
	
	.margin = 5,
	.min_shutter = 4,
	.max_frame_length = 0xFFFF,       //REG0x0202 <=REG0x0340-5//max framelength by sensor register's limitation
	.ae_shut_delay_frame = 0,	        //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
	.ae_sensor_gain_delay_frame = 0,  //sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
	.ae_ispGain_delay_frame = 2,      //isp gain delay frame for AE cycle
	.frame_time_delay_frame = 2,
	.ihdr_support = 0,	              //1, support; 0,not support
	.ihdr_le_firstline = 0,           //1,le first ; 0, se first
	.sensor_mode_num = 6,	            //support sensor mode num ,don't support Slow motion
	
	.cap_delay_frame = 3,		//enter capture delay frame num
	.pre_delay_frame = 3, 		//enter preview delay frame num
	.video_delay_frame = 3,		//enter video delay frame num
	.hs_video_delay_frame = 3,	//enter high speed video  delay frame num
	.slim_video_delay_frame = 3,//enter slim video delay frame num
	.custom1_delay_frame = 2,	/* add new mode */
	.isp_driving_current = ISP_DRIVING_8MA, //mclk driving current
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = 1,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_Gb,//sensor output first pixel color
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
	.i2c_addr_table = {0xAC,0xff},//0x20, record sensor support all write id addr, only supprt 4must end with 0xff
    .i2c_speed = 1000, // i2c read/write speed
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x0200,					//current shutter
	.gain = 0x200,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.ihdr_en = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0xAC,//record current sensor's i2c write id
};

/* VC_Num, VC_PixelNum, ModeSelect, EXPO_Ratio, ODValue, RG_STATSMODE*/
/* VC0_ID, VC0_DataType, VC0_SIZEH, VC0_SIZE,
 * VC1_ID, VC1_DataType, VC1_SIZEH, VC1_SIZEV
 */

/* VC2_ID, VC2_DataType, VC2_SIZEH, VC2_SIZE,
 * VC3_ID, VC3_DataType, VC3_SIZEH, VC3_SIZEV
 */


static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3] = {
	/* Preview mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x0ff0, 0x0c00, 0x01, 0x30, 0x027c, 0x0bf0,
	0x01, 0x30, 0x027c, 0x0bf0, 0x03, 0x00, 0x0000, 0x0000},
	/* Video mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x0ff0, 0x0c00, 0x01, 0x30, 0x027c, 0x0bf0,
	0x01, 0x30, 0x027c, 0x0bf0, 0x03, 0x00, 0x0000, 0x0000},
	/* Capture mode setting */
	{0x02, 0x0A,   0x00,   0x08, 0x40, 0x00,
	0x00, 0x2B, 0x0ff0, 0x0c00, 0x01, 0x30, 0x027c, 0x0bf0,
	0x01, 0x30, 0x027c, 0x0bf0, 0x03, 0x00, 0x0000, 0x0000}
	};

/* Sensor output window information*/
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[6] =	 
{
	{ 8160, 6144,    0,    0, 8160, 6144, 4080, 3072,  0,   0, 4080, 3072,    0,    0, 4080, 3072}, // preveiw
	{ 8160, 6144,    0,    0, 8160, 6144, 4080, 3072,  0,   0, 4080, 3072,    0,    0, 4080, 3072}, // capture
	{ 8160, 6144,    0,    0, 8160, 6144, 4080, 3072,  0,   0, 4080, 3072,    0,    0, 4080, 3072}, // video
	{ 8160, 6144,    0,    0, 8160, 6144, 2040, 1536,380, 408, 1280,  720,    0,    0, 1280,  720}, // high speed
	{ 8160, 6144,    0,    0, 8160, 6144, 4080, 3072,  0, 388, 4080, 2296,    0,    0, 4080, 2296}, // slim video
	{ 8160, 6144,    0,    0, 8160, 6144, 4080, 3072,  0,   0, 4080, 3072,    0,    0, 4080, 3072},	/* custom1 */
};


static struct  SET_PD_BLOCK_INFO_T imgsensor_pd_info =
//for 3m5, need to modify
{
 
    .i4OffsetX = 8,

    .i4OffsetY = 8,
 
    .i4PitchX = 8,
 
    .i4PitchY = 8,
 
    .i4PairNum =4,

    .i4SubBlkW =8,
 
    .i4SubBlkH =2,

    .i4PosL = {{11,8},{9,11},{13,12},{15,15}},
 
    .i4PosR = {{10,8},{8,11},{12,12},{14,15}},
	.i4BlockNumX = 508,
    .i4BlockNumY = 382,
	.iMirrorFlip = 3,
 
};

#if 0
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info_16_9 =
{
    .i4OffsetX = 16,

    .i4OffsetY = 60,
 
    .i4PitchX = 32,
 
    .i4PitchY = 32,
 
    .i4PairNum =16,

    .i4SubBlkW =8,
 
    .i4SubBlkH =8, 

    .i4PosL = {{18,61},{26,61},{34,61},{42,61},{22,73},{30,73},{38,73},{46,73},{18,81},{26,81},{34,81},{42,81},{22,85},{30,85},{38,85},{46,85}},
 
    .i4PosR = {{18,65},{26,65},{34,65},{42,65},{22,69},{30,69},{38,69},{46,69},{18,77},{26,77},{34,77},{42,77},{22,89},{30,89},{38,89},{46,89}},
	.i4BlockNumX = 124,
    .i4BlockNumY = 90,
	.iMirrorFlip = 0,
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

#if MULTI_WRITE
static kal_uint16 table_write_cmos_sensor(kal_uint16 *para, kal_uint32 len)
{
    char puSendCmd[I2C_BUFFER_LEN];
    kal_uint32 tosend, IDX;
    kal_uint16 addr = 0, addr_last = 0, data = 0;

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
        //LOG_INF("i2c_write_id start[0x%x] Addr[0x%x] Data[0x%x] addr_last[0x%x]\n", imgsensor.i2c_write_id, addr, data, addr_last);
    }

	/* Write when remain buffer size is less than 4 bytes or reach end of data */
	    if ((I2C_BUFFER_LEN - tosend) < 4 || IDX == len || addr != addr_last) {
	        
	        LOG_INF("i2c_write_id end[0x%x] Addr[0x%x] Data[0x%x] addr_last[0x%x], tosend[%d], IDX[%d], len[%d]\n", imgsensor.i2c_write_id, addr, data, addr_last,
	        tosend, IDX, len );

	        while(iBurstWriteReg_multi(puSendCmd, tosend,
	            imgsensor.i2c_write_id, 4, imgsensor_info.i2c_speed) != 0 )
	            {
	                LOG_INF("iBurstWriteReg_multi FAIL!, retry!");
	            }
	        tosend = 0;
	    }
    }
    return 0;
}
#endif

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

static void check_streamoff(void)
{
	unsigned int i = 0;
	int timeout = (10000 / imgsensor.current_fps) + 1;

	mdelay(3);
	for (i = 0; i < timeout; i++) {
		if (read_cmos_sensor_byte(0x0005) != 0xFF)
			mdelay(1);
		else
			break;
	}
	LOG_INF(" check_streamoff exit!\n");
}

static kal_uint32 streaming_control(kal_bool enable)
{
	LOG_INF("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);

	if (enable) {
		write_cmos_sensor(0x6028, 0x4000);//6028 4000
		//write_cmos_sensor(0x0100, 0X0100);//0100 0100
		write_cmos_sensor_byte(0x0100, 0x01);
		mdelay(10);
	} else {
		write_cmos_sensor_byte(0x0100, 0x00);
		//write_cmos_sensor(0x0100, 0x00);//0104 01
		check_streamoff();
	}
	return ERROR_NONE;
}

#if 0
static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;
	   
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
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0x0202, (shutter) & 0xFFFF);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

	//LOG_INF("frame_length = %d ", frame_length);
	
}	/*	write_shutter  */
#endif

static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	 spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	/*Change frame time*/
	if (frame_length > 1)
		imgsensor.frame_length = frame_length;
/* */
	if (shutter > imgsensor.frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
		
	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	if (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
		shutter = (imgsensor_info.max_frame_length - imgsensor_info.margin);

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305)
			set_max_framerate(296, 0);
		else if (realtime_fps >= 147 && realtime_fps <= 150)
			set_max_framerate(146, 0);
		else {
			/* Extend frame length*/
			write_cmos_sensor(0x0340, imgsensor.frame_length);
		}
	} else {
		/* Extend frame length*/
		 write_cmos_sensor(0x0340, imgsensor.frame_length);
	}
	/* Update Shutter*/
	write_cmos_sensor(0x0202, shutter);

	LOG_INF("Add for DUAL lzh! shutter =%d, framelength =%d\n", shutter, imgsensor.frame_length);

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
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		// Extend frame length
		write_cmos_sensor(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	// Update Shutter
	write_cmos_sensor(0X0202, shutter & 0xFFFF);
	LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

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

	write_cmos_sensor(0x0204, (reg_gain&0xFFFF));    
	return gain;
}	/*	set_gain  */

//ihdr_write_shutter_gain not support for S5K3m5
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
	printk("mirrow image");
	spin_lock(&imgsensor_drv_lock);
    imgsensor.mirror= image_mirror; 
    spin_unlock(&imgsensor_drv_lock);
	switch (image_mirror) {
		case IMAGE_NORMAL:
			write_cmos_sensor_byte(0x0101,0x00); //GR
			break;
		case IMAGE_H_MIRROR:
			write_cmos_sensor_byte(0x0101,0x01); //R
			break;
		case IMAGE_V_MIRROR:
			write_cmos_sensor_byte(0x0101,0x02); //B	
			break;
		case IMAGE_HV_MIRROR:
			write_cmos_sensor_byte(0x0101,0x03); //GB
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
			break;
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


static kal_uint16 addr_data_pair_init[] = {
	0x6028,0x2400,
	0x602A,0x1354,
	0x6F12,0x0100,
	0x6F12,0x7017,
	0x602A,0x13B2,
	0x6F12,0x0000,
	0x602A,0x1236,
	0x6F12,0x0000,
	0x602A,0x1A0A,
	0x6F12,0x4C0A,
	0x602A,0x2210,
	0x6F12,0x3401,
	0x602A,0x2176,
	0x6F12,0x6400,
	0x602A,0x222E,
	0x6F12,0x0001,
	0x602A,0x06B6,
	0x6F12,0x0A00,
	0x602A,0x06BC,
	0x6F12,0x1001,
	0x602A,0x2140,
	0x6F12,0x0101,
	0x602A,0x1A0E,
	0x6F12,0x9600,
	0x6028,0x4000,
	0xF44E,0x0011,
	0xF44C,0x0B0B,
	0xF44A,0x0006,
	0x0118,0x0002,
	0x011A,0x0001,
};

static kal_uint16 addr_data_pair_preview[] = {
	0x6028,0x2400,
	0x602A,0x1A28,
	0x6F12,0x4C00,
	0x602A,0x065A,
	0x6F12,0x0000,
	0x602A,0x139E,
	0x6F12,0x0100,
	0x602A,0x139C,
	0x6F12,0x0000,
	0x602A,0x13A0,
	0x6F12,0x0A00,
	0x6F12,0x0120,
	0x602A,0x2072,
	0x6F12,0x0000,
	0x602A,0x1A64,
	0x6F12,0x0301,
	0x6F12,0xFF00,
	0x602A,0x19E6,
	0x6F12,0x0200,
	0x602A,0x1A30,
	0x6F12,0x3401,
	0x602A,0x19FC,
	0x6F12,0x0B00,
	0x602A,0x19F4,
	0x6F12,0x0606,
	0x602A,0x19F8,
	0x6F12,0x1010,
	0x602A,0x1B26,
	0x6F12,0x6F80,
	0x6F12,0xA060,
	0x602A,0x1A3C,
	0x6F12,0x6207,
	0x602A,0x1A48,
	0x6F12,0x6207,
	0x602A,0x1444,
	0x6F12,0x2000,
	0x6F12,0x2000,
	0x602A,0x144C,
	0x6F12,0x3F00,
	0x6F12,0x3F00,
	0x602A,0x7F6C,
	0x6F12,0x0100,
	0x6F12,0x2F00,
	0x6F12,0xFA00,
	0x6F12,0x2400,
	0x6F12,0xE500,
	0x602A,0x0650,
	0x6F12,0x0600,
	0x602A,0x0654,
	0x6F12,0x0000,
	0x602A,0x1A46,
	0x6F12,0x8A00,
	0x602A,0x1A52,
	0x6F12,0xBF00,
	0x602A,0x0674,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x0668,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x602A,0x0684,
	0x6F12,0x4001,
	0x602A,0x0688,
	0x6F12,0x4001,
	0x602A,0x147C,
	0x6F12,0x1000,
	0x602A,0x1480,
	0x6F12,0x1000,
	0x602A,0x19F6,
	0x6F12,0x0904,
	0x602A,0x0812,
	0x6F12,0x0000,
	0x602A,0x1A02,
	0x6F12,0x1800,
	0x602A,0x2148,
	0x6F12,0x0100,
	0x602A,0x2042,
	0x6F12,0x1A00,
	0x602A,0x0874,
	0x6F12,0x0100,
	0x602A,0x09C0,
	0x6F12,0x2008,
	0x602A,0x09C4,
	0x6F12,0x2000,
	0x602A,0x19FE,
	0x6F12,0x0E1C,
	0x602A,0x4D92,
	0x6F12,0x0100,
	0x602A,0x84C8,
	0x6F12,0x0100,
	0x602A,0x4D94,
	0x6F12,0x0005,
	0x6F12,0x000A,
	0x6F12,0x0010,
	0x6F12,0x0810,
	0x6F12,0x000A,
	0x6F12,0x0040,
	0x6F12,0x0810,
	0x6F12,0x0810,
	0x6F12,0x8002,
	0x6F12,0xFD03,
	0x6F12,0x0010,
	0x6F12,0x1510,
	0x602A,0x3570,
	0x6F12,0x0000,
	0x602A,0x3574,
	0x6F12,0x1201,
	0x602A,0x21E4,
	0x6F12,0x0400,
	0x602A,0x21EC,
	0x6F12,0x1F04,
	0x602A,0x2080,
	0x6F12,0x0101,
	0x6F12,0xFF00,
	0x6F12,0x7F01,
	0x6F12,0x0001,
	0x6F12,0x8001,
	0x6F12,0xD244,
	0x6F12,0xD244,
	0x6F12,0x14F4,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x20BA,
	0x6F12,0x121C,
	0x6F12,0x111C,
	0x6F12,0x54F4,
	0x602A,0x120E,
	0x6F12,0x1000,
	0x602A,0x212E,
	0x6F12,0x0200,
	0x602A,0x13AE,
	0x6F12,0x0101,
	0x602A,0x0718,
	0x6F12,0x0001,
	0x602A,0x0710,
	0x6F12,0x0002,
	0x6F12,0x0804,
	0x6F12,0x0100,
	0x602A,0x1B5C,
	0x6F12,0x0000,
	0x602A,0x0786,
	0x6F12,0x7701,
	0x602A,0x2022,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x1360,
	0x6F12,0x0100,
	0x602A,0x1376,
	0x6F12,0x0100,
	0x6F12,0x6038,
	0x6F12,0x7038,
	0x6F12,0x8038,
	0x602A,0x1386,
	0x6F12,0x0B00,
	0x602A,0x06FA,
	0x6F12,0x0000,
	0x602A,0x4A94,
	0x6F12,0x0900,
	0x6F12,0x0000,
	0x6F12,0x0300,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0300,
	0x6F12,0x0000,
	0x6F12,0x0900,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x0A76,
	0x6F12,0x1000,
	0x602A,0x0AEE,
	0x6F12,0x1000,
	0x602A,0x0B66,
	0x6F12,0x1000,
	0x602A,0x0BDE,
	0x6F12,0x1000,
	0x602A,0x0BE8,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0C56,
	0x6F12,0x1000,
	0x602A,0x0C60,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0CB6,
	0x6F12,0x0100,
	0x602A,0x0CF2,
	0x6F12,0x0001,
	0x602A,0x0CF0,
	0x6F12,0x0101,
	0x602A,0x11B8,
	0x6F12,0x0100,
	0x602A,0x11F6,
	0x6F12,0x0020,
	0x602A,0x4A74,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x218E,
	0x6F12,0x0000,
	0x602A,0x2268,
	0x6F12,0xF279,
	0x602A,0x5006,
	0x6F12,0x0000,
	0x602A,0x500E,
	0x6F12,0x0100,
	0x602A,0x4E70,
	0x6F12,0x2062,
	0x6F12,0x5501,
	0x602A,0x06DC,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6028,0x4000,
	0xF46A,0xAE80,
	0x0344,0x0000,
	0x0346,0x0000,
	0x0348,0x1FFF,
	0x034A,0x181F,
	0x034C,0x0FF0,
	0x034E,0x0C00,
	0x0350,0x0008,
	0x0352,0x0008,
	0x0900,0x0122,
	0x0380,0x0002,
	0x0382,0x0002,
	0x0384,0x0002,
	0x0386,0x0002,
	0x0110,0x1002,
	0x0114,0x0301,
	0x0116,0x3000,
	0x0136,0x1800,
	0x013E,0x0000,
	0x0300,0x0006,
	0x0302,0x0001,
	0x0304,0x0004,
	0x0306,0x008C,
	0x0308,0x0008,
	0x030A,0x0001,
	0x030C,0x0000,
	0x030E,0x0004,
	0x0310,0x008A,
	0x0312,0x0000,
	0x080E,0x0000,
	0x0340,0x0FE0,
	0x0342,0x11E8,
	0x0702,0x0000,
	0x0202,0x0100,
	0x0200,0x0100,
	0x0D00,0x0101,
	0x0D02,0x0101,
	0x0D04,0x0102,
	0x6226,0x0000,
};


static kal_uint16 addr_data_pair_capture[] = {
	0x6028,0x2400,
	0x602A,0x1A28,
	0x6F12,0x4C00,
	0x602A,0x065A,
	0x6F12,0x0000,
	0x602A,0x139E,
	0x6F12,0x0100,
	0x602A,0x139C,
	0x6F12,0x0000,
	0x602A,0x13A0,
	0x6F12,0x0A00,
	0x6F12,0x0120,
	0x602A,0x2072,
	0x6F12,0x0000,
	0x602A,0x1A64,
	0x6F12,0x0301,
	0x6F12,0xFF00,
	0x602A,0x19E6,
	0x6F12,0x0200,
	0x602A,0x1A30,
	0x6F12,0x3401,
	0x602A,0x19FC,
	0x6F12,0x0B00,
	0x602A,0x19F4,
	0x6F12,0x0606,
	0x602A,0x19F8,
	0x6F12,0x1010,
	0x602A,0x1B26,
	0x6F12,0x6F80,
	0x6F12,0xA060,
	0x602A,0x1A3C,
	0x6F12,0x6207,
	0x602A,0x1A48,
	0x6F12,0x6207,
	0x602A,0x1444,
	0x6F12,0x2000,
	0x6F12,0x2000,
	0x602A,0x144C,
	0x6F12,0x3F00,
	0x6F12,0x3F00,
	0x602A,0x7F6C,
	0x6F12,0x0100,
	0x6F12,0x2F00,
	0x6F12,0xFA00,
	0x6F12,0x2400,
	0x6F12,0xE500,
	0x602A,0x0650,
	0x6F12,0x0600,
	0x602A,0x0654,
	0x6F12,0x0000,
	0x602A,0x1A46,
	0x6F12,0x8A00,
	0x602A,0x1A52,
	0x6F12,0xBF00,
	0x602A,0x0674,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x0668,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x602A,0x0684,
	0x6F12,0x4001,
	0x602A,0x0688,
	0x6F12,0x4001,
	0x602A,0x147C,
	0x6F12,0x1000,
	0x602A,0x1480,
	0x6F12,0x1000,
	0x602A,0x19F6,
	0x6F12,0x0904,
	0x602A,0x0812,
	0x6F12,0x0000,
	0x602A,0x1A02,
	0x6F12,0x1800,
	0x602A,0x2148,
	0x6F12,0x0100,
	0x602A,0x2042,
	0x6F12,0x1A00,
	0x602A,0x0874,
	0x6F12,0x0100,
	0x602A,0x09C0,
	0x6F12,0x2008,
	0x602A,0x09C4,
	0x6F12,0x2000,
	0x602A,0x19FE,
	0x6F12,0x0E1C,
	0x602A,0x4D92,
	0x6F12,0x0100,
	0x602A,0x84C8,
	0x6F12,0x0100,
	0x602A,0x4D94,
	0x6F12,0x0005,
	0x6F12,0x000A,
	0x6F12,0x0010,
	0x6F12,0x0810,
	0x6F12,0x000A,
	0x6F12,0x0040,
	0x6F12,0x0810,
	0x6F12,0x0810,
	0x6F12,0x8002,
	0x6F12,0xFD03,
	0x6F12,0x0010,
	0x6F12,0x1510,
	0x602A,0x3570,
	0x6F12,0x0000,
	0x602A,0x3574,
	0x6F12,0x1201,
	0x602A,0x21E4,
	0x6F12,0x0400,
	0x602A,0x21EC,
	0x6F12,0x1F04,
	0x602A,0x2080,
	0x6F12,0x0101,
	0x6F12,0xFF00,
	0x6F12,0x7F01,
	0x6F12,0x0001,
	0x6F12,0x8001,
	0x6F12,0xD244,
	0x6F12,0xD244,
	0x6F12,0x14F4,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x20BA,
	0x6F12,0x121C,
	0x6F12,0x111C,
	0x6F12,0x54F4,
	0x602A,0x120E,
	0x6F12,0x1000,
	0x602A,0x212E,
	0x6F12,0x0200,
	0x602A,0x13AE,
	0x6F12,0x0101,
	0x602A,0x0718,
	0x6F12,0x0001,
	0x602A,0x0710,
	0x6F12,0x0002,
	0x6F12,0x0804,
	0x6F12,0x0100,
	0x602A,0x1B5C,
	0x6F12,0x0000,
	0x602A,0x0786,
	0x6F12,0x7701,
	0x602A,0x2022,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x1360,
	0x6F12,0x0100,
	0x602A,0x1376,
	0x6F12,0x0100,
	0x6F12,0x6038,
	0x6F12,0x7038,
	0x6F12,0x8038,
	0x602A,0x1386,
	0x6F12,0x0B00,
	0x602A,0x06FA,
	0x6F12,0x0000,
	0x602A,0x4A94,
	0x6F12,0x0900,
	0x6F12,0x0000,
	0x6F12,0x0300,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0300,
	0x6F12,0x0000,
	0x6F12,0x0900,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x0A76,
	0x6F12,0x1000,
	0x602A,0x0AEE,
	0x6F12,0x1000,
	0x602A,0x0B66,
	0x6F12,0x1000,
	0x602A,0x0BDE,
	0x6F12,0x1000,
	0x602A,0x0BE8,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0C56,
	0x6F12,0x1000,
	0x602A,0x0C60,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0CB6,
	0x6F12,0x0100,
	0x602A,0x0CF2,
	0x6F12,0x0001,
	0x602A,0x0CF0,
	0x6F12,0x0101,
	0x602A,0x11B8,
	0x6F12,0x0100,
	0x602A,0x11F6,
	0x6F12,0x0020,
	0x602A,0x4A74,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x218E,
	0x6F12,0x0000,
	0x602A,0x2268,
	0x6F12,0xF279,
	0x602A,0x5006,
	0x6F12,0x0000,
	0x602A,0x500E,
	0x6F12,0x0100,
	0x602A,0x4E70,
	0x6F12,0x2062,
	0x6F12,0x5501,
	0x602A,0x06DC,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6028,0x4000,
	0xF46A,0xAE80,
	0x0344,0x0000,
	0x0346,0x0000,
	0x0348,0x1FFF,
	0x034A,0x181F,
	0x034C,0x0FF0,
	0x034E,0x0C00,
	0x0350,0x0008,
	0x0352,0x0008,
	0x0900,0x0122,
	0x0380,0x0002,
	0x0382,0x0002,
	0x0384,0x0002,
	0x0386,0x0002,
	0x0110,0x1002,
	0x0114,0x0301,
	0x0116,0x3000,
	0x0136,0x1800,
	0x013E,0x0000,
	0x0300,0x0006,
	0x0302,0x0001,
	0x0304,0x0004,
	0x0306,0x008C,
	0x0308,0x0008,
	0x030A,0x0001,
	0x030C,0x0000,
	0x030E,0x0004,
	0x0310,0x008A,
	0x0312,0x0000,
	0x080E,0x0000,
	0x0340,0x0FE0,
	0x0342,0x11E8,
	0x0702,0x0000,
	0x0202,0x0100,
	0x0200,0x0100,
	0x0D00,0x0101,
	0x0D02,0x0101,
	0x0D04,0x0102,
	0x6226,0x0000,
};


static kal_uint16 addr_data_pair_normal_video[] = {
	0x6028,0x2400,
	0x602A,0x1A28,
	0x6F12,0x4C00,
	0x602A,0x065A,
	0x6F12,0x0000,
	0x602A,0x139E,
	0x6F12,0x0100,
	0x602A,0x139C,
	0x6F12,0x0000,
	0x602A,0x13A0,
	0x6F12,0x0A00,
	0x6F12,0x0120,
	0x602A,0x2072,
	0x6F12,0x0000,
	0x602A,0x1A64,
	0x6F12,0x0301,
	0x6F12,0xFF00,
	0x602A,0x19E6,
	0x6F12,0x0200,
	0x602A,0x1A30,
	0x6F12,0x3401,
	0x602A,0x19FC,
	0x6F12,0x0B00,
	0x602A,0x19F4,
	0x6F12,0x0606,
	0x602A,0x19F8,
	0x6F12,0x1010,
	0x602A,0x1B26,
	0x6F12,0x6F80,
	0x6F12,0xA060,
	0x602A,0x1A3C,
	0x6F12,0x6207,
	0x602A,0x1A48,
	0x6F12,0x6207,
	0x602A,0x1444,
	0x6F12,0x2000,
	0x6F12,0x2000,
	0x602A,0x144C,
	0x6F12,0x3F00,
	0x6F12,0x3F00,
	0x602A,0x7F6C,
	0x6F12,0x0100,
	0x6F12,0x2F00,
	0x6F12,0xFA00,
	0x6F12,0x2400,
	0x6F12,0xE500,
	0x602A,0x0650,
	0x6F12,0x0600,
	0x602A,0x0654,
	0x6F12,0x0000,
	0x602A,0x1A46,
	0x6F12,0x8A00,
	0x602A,0x1A52,
	0x6F12,0xBF00,
	0x602A,0x0674,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x0668,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x602A,0x0684,
	0x6F12,0x4001,
	0x602A,0x0688,
	0x6F12,0x4001,
	0x602A,0x147C,
	0x6F12,0x1000,
	0x602A,0x1480,
	0x6F12,0x1000,
	0x602A,0x19F6,
	0x6F12,0x0904,
	0x602A,0x0812,
	0x6F12,0x0000,
	0x602A,0x1A02,
	0x6F12,0x1800,
	0x602A,0x2148,
	0x6F12,0x0100,
	0x602A,0x2042,
	0x6F12,0x1A00,
	0x602A,0x0874,
	0x6F12,0x0100,
	0x602A,0x09C0,
	0x6F12,0x2008,
	0x602A,0x09C4,
	0x6F12,0x2000,
	0x602A,0x19FE,
	0x6F12,0x0E1C,
	0x602A,0x4D92,
	0x6F12,0x0100,
	0x602A,0x84C8,
	0x6F12,0x0100,
	0x602A,0x4D94,
	0x6F12,0x0005,
	0x6F12,0x000A,
	0x6F12,0x0010,
	0x6F12,0x0810,
	0x6F12,0x000A,
	0x6F12,0x0040,
	0x6F12,0x0810,
	0x6F12,0x0810,
	0x6F12,0x8002,
	0x6F12,0xFD03,
	0x6F12,0x0010,
	0x6F12,0x1510,
	0x602A,0x3570,
	0x6F12,0x0000,
	0x602A,0x3574,
	0x6F12,0x1201,
	0x602A,0x21E4,
	0x6F12,0x0400,
	0x602A,0x21EC,
	0x6F12,0x1F04,
	0x602A,0x2080,
	0x6F12,0x0101,
	0x6F12,0xFF00,
	0x6F12,0x7F01,
	0x6F12,0x0001,
	0x6F12,0x8001,
	0x6F12,0xD244,
	0x6F12,0xD244,
	0x6F12,0x14F4,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x20BA,
	0x6F12,0x121C,
	0x6F12,0x111C,
	0x6F12,0x54F4,
	0x602A,0x120E,
	0x6F12,0x1000,
	0x602A,0x212E,
	0x6F12,0x0200,
	0x602A,0x13AE,
	0x6F12,0x0101,
	0x602A,0x0718,
	0x6F12,0x0001,
	0x602A,0x0710,
	0x6F12,0x0002,
	0x6F12,0x0804,
	0x6F12,0x0100,
	0x602A,0x1B5C,
	0x6F12,0x0000,
	0x602A,0x0786,
	0x6F12,0x7701,
	0x602A,0x2022,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x1360,
	0x6F12,0x0100,
	0x602A,0x1376,
	0x6F12,0x0100,
	0x6F12,0x6038,
	0x6F12,0x7038,
	0x6F12,0x8038,
	0x602A,0x1386,
	0x6F12,0x0B00,
	0x602A,0x06FA,
	0x6F12,0x0000,
	0x602A,0x4A94,
	0x6F12,0x0900,
	0x6F12,0x0000,
	0x6F12,0x0300,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0300,
	0x6F12,0x0000,
	0x6F12,0x0900,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x0A76,
	0x6F12,0x1000,
	0x602A,0x0AEE,
	0x6F12,0x1000,
	0x602A,0x0B66,
	0x6F12,0x1000,
	0x602A,0x0BDE,
	0x6F12,0x1000,
	0x602A,0x0BE8,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0C56,
	0x6F12,0x1000,
	0x602A,0x0C60,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0CB6,
	0x6F12,0x0100,
	0x602A,0x0CF2,
	0x6F12,0x0001,
	0x602A,0x0CF0,
	0x6F12,0x0101,
	0x602A,0x11B8,
	0x6F12,0x0100,
	0x602A,0x11F6,
	0x6F12,0x0020,
	0x602A,0x4A74,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x218E,
	0x6F12,0x0000,
	0x602A,0x2268,
	0x6F12,0xF279,
	0x602A,0x5006,
	0x6F12,0x0000,
	0x602A,0x500E,
	0x6F12,0x0100,
	0x602A,0x4E70,
	0x6F12,0x2062,
	0x6F12,0x5501,
	0x602A,0x06DC,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6028,0x4000,
	0xF46A,0xAE80,
	0x0344,0x0000,
	0x0346,0x0000,
	0x0348,0x1FFF,
	0x034A,0x181F,
	0x034C,0x0FF0,
	0x034E,0x0C00,
	0x0350,0x0008,
	0x0352,0x0008,
	0x0900,0x0122,
	0x0380,0x0002,
	0x0382,0x0002,
	0x0384,0x0002,
	0x0386,0x0002,
	0x0110,0x1002,
	0x0114,0x0301,
	0x0116,0x3000,
	0x0136,0x1800,
	0x013E,0x0000,
	0x0300,0x0006,
	0x0302,0x0001,
	0x0304,0x0004,
	0x0306,0x008C,
	0x0308,0x0008,
	0x030A,0x0001,
	0x030C,0x0000,
	0x030E,0x0004,
	0x0310,0x008A,
	0x0312,0x0000,
	0x080E,0x0000,
	0x0340,0x0FE0,
	0x0342,0x11E8,
	0x0702,0x0000,
	0x0202,0x0100,
	0x0200,0x0100,
	0x0D00,0x0101,
	0x0D02,0x0101,
	0x0D04,0x0102,
	0x6226,0x0000,
};


static kal_uint16 addr_data_pair_hs_video[] = {
	0x6028,0x2400,
	0x602A,0x1A28,
	0x6F12,0x4C00,
	0x602A,0x065A,
	0x6F12,0x0000,
	0x602A,0x139E,
	0x6F12,0x0300,
	0x602A,0x139C,
	0x6F12,0x0000,
	0x602A,0x13A0,
	0x6F12,0x0A00,
	0x6F12,0x0020,
	0x602A,0x2072,
	0x6F12,0x0000,
	0x602A,0x1A64,
	0x6F12,0x0301,
	0x6F12,0x3F00,
	0x602A,0x19E6,
	0x6F12,0x0201,
	0x602A,0x1A30,
	0x6F12,0x3401,
	0x602A,0x19FC,
	0x6F12,0x0B00,
	0x602A,0x19F4,
	0x6F12,0x0606,
	0x602A,0x19F8,
	0x6F12,0x1010,
	0x602A,0x1B26,
	0x6F12,0x6F80,
	0x6F12,0xA020,
	0x602A,0x1A3C,
	0x6F12,0x5207,
	0x602A,0x1A48,
	0x6F12,0x5207,
	0x602A,0x1444,
	0x6F12,0x2100,
	0x6F12,0x2100,
	0x602A,0x144C,
	0x6F12,0x4200,
	0x6F12,0x4200,
	0x602A,0x7F6C,
	0x6F12,0x0100,
	0x6F12,0x3100,
	0x6F12,0xF700,
	0x6F12,0x2600,
	0x6F12,0xE100,
	0x602A,0x0650,
	0x6F12,0x0600,
	0x602A,0x0654,
	0x6F12,0x0000,
	0x602A,0x1A46,
	0x6F12,0x8600,
	0x602A,0x1A52,
	0x6F12,0xBF00,
	0x602A,0x0674,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x0668,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x602A,0x0684,
	0x6F12,0x4001,
	0x602A,0x0688,
	0x6F12,0x4001,
	0x602A,0x147C,
	0x6F12,0x1000,
	0x602A,0x1480,
	0x6F12,0x1000,
	0x602A,0x19F6,
	0x6F12,0x0904,
	0x602A,0x0812,
	0x6F12,0x0000,
	0x602A,0x1A02,
	0x6F12,0x0800,
	0x602A,0x2148,
	0x6F12,0x0100,
	0x602A,0x2042,
	0x6F12,0x1A00,
	0x602A,0x0874,
	0x6F12,0x1100,
	0x602A,0x09C0,
	0x6F12,0x1803,
	0x602A,0x09C4,
	0x6F12,0x1803,
	0x602A,0x19FE,
	0x6F12,0x0E1C,
	0x602A,0x4D92,
	0x6F12,0x0100,
	0x602A,0x84C8,
	0x6F12,0x0100,
	0x602A,0x4D94,
	0x6F12,0x4001,
	0x6F12,0x0004,
	0x6F12,0x0010,
	0x6F12,0x0810,
	0x6F12,0x0004,
	0x6F12,0x0010,
	0x6F12,0x0810,
	0x6F12,0x0810,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0010,
	0x6F12,0x0010,
	0x602A,0x3570,
	0x6F12,0x0000,
	0x602A,0x3574,
	0x6F12,0x3801,
	0x602A,0x21E4,
	0x6F12,0x0400,
	0x602A,0x21EC,
	0x6F12,0x6801,
	0x602A,0x2080,
	0x6F12,0x0100,
	0x6F12,0x7F00,
	0x6F12,0x0002,
	0x6F12,0x8000,
	0x6F12,0x0002,
	0x6F12,0xC244,
	0x6F12,0xD244,
	0x6F12,0x14F4,
	0x6F12,0x141C,
	0x6F12,0x111C,
	0x6F12,0x54F4,
	0x602A,0x20BA,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x120E,
	0x6F12,0x1000,
	0x602A,0x212E,
	0x6F12,0x0A00,
	0x602A,0x13AE,
	0x6F12,0x0102,
	0x602A,0x0718,
	0x6F12,0x0005,
	0x602A,0x0710,
	0x6F12,0x0004,
	0x6F12,0x0401,
	0x6F12,0x0100,
	0x602A,0x1B5C,
	0x6F12,0x0300,
	0x602A,0x0786,
	0x6F12,0x7701,
	0x602A,0x2022,
	0x6F12,0x0101,
	0x6F12,0x0101,
	0x602A,0x1360,
	0x6F12,0x0000,
	0x602A,0x1376,
	0x6F12,0x0200,
	0x6F12,0x6038,
	0x6F12,0x7038,
	0x6F12,0x8038,
	0x602A,0x1386,
	0x6F12,0x0B00,
	0x602A,0x06FA,
	0x6F12,0x1000,
	0x602A,0x4A94,
	0x6F12,0x1600,
	0x6F12,0x0000,
	0x6F12,0x1000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x1000,
	0x6F12,0x0000,
	0x6F12,0x1600,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x0A76,
	0x6F12,0x1000,
	0x602A,0x0AEE,
	0x6F12,0x1000,
	0x602A,0x0B66,
	0x6F12,0x1000,
	0x602A,0x0BDE,
	0x6F12,0x1000,
	0x602A,0x0BE8,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0C56,
	0x6F12,0x1000,
	0x602A,0x0C60,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0CB6,
	0x6F12,0x0000,
	0x602A,0x0CF2,
	0x6F12,0x0001,
	0x602A,0x0CF0,
	0x6F12,0x0101,
	0x602A,0x11B8,
	0x6F12,0x0000,
	0x602A,0x11F6,
	0x6F12,0x0010,
	0x602A,0x4A74,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x218E,
	0x6F12,0x0000,
	0x602A,0x2268,
	0x6F12,0xF279,
	0x602A,0x5006,
	0x6F12,0x0000,
	0x602A,0x500E,
	0x6F12,0x0100,
	0x602A,0x4E70,
	0x6F12,0x2062,
	0x6F12,0x5501,
	0x602A,0x06DC,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6028,0x4000,
	0xF46A,0xAE80,
	0x0344,0x05F0,
	0x0346,0x0660,
	0x0348,0x1A0F,
	0x034A,0x11BF,
	0x034C,0x0500,
	0x034E,0x02D0,
	0x0350,0x0004,
	0x0352,0x0004,
	0x0900,0x0144,
	0x0380,0x0002,
	0x0382,0x0006,
	0x0384,0x0002,
	0x0386,0x0006,
	0x0110,0x1002,
	0x0114,0x0300,
	0x0116,0x3000,
	0x0136,0x1800,
	0x013E,0x0000,
	0x0300,0x0006,
	0x0302,0x0001,
	0x0304,0x0004,
	0x0306,0x0096,
	0x0308,0x0008,
	0x030A,0x0001,
	0x030C,0x0000,
	0x030E,0x0003,
	0x0310,0x0064,
	0x0312,0x0000,
	0x080E,0x0000,
	0x0340,0x094C,
	0x0342,0x0830,
	0x0702,0x0000,
	0x0202,0x0100,
	0x0200,0x0100,
	0x0D00,0x0101,
	0x0D02,0x0001,
	0x0D04,0x0002,
	0x6226,0x0000,
};

static kal_uint16 addr_data_pair_slim_video[] = {
	0x6028,0x2400,
	0x602A,0x1A28,
	0x6F12,0x4C00,
	0x602A,0x065A,
	0x6F12,0x0000,
	0x602A,0x139E,
	0x6F12,0x0100,
	0x602A,0x139C,
	0x6F12,0x0000,
	0x602A,0x13A0,
	0x6F12,0x0A00,
	0x6F12,0x0120,
	0x602A,0x2072,
	0x6F12,0x0000,
	0x602A,0x1A64,
	0x6F12,0x0301,
	0x6F12,0xFF00,
	0x602A,0x19E6,
	0x6F12,0x0200,
	0x602A,0x1A30,
	0x6F12,0x3401,
	0x602A,0x19FC,
	0x6F12,0x0B00,
	0x602A,0x19F4,
	0x6F12,0x0606,
	0x602A,0x19F8,
	0x6F12,0x1010,
	0x602A,0x1B26,
	0x6F12,0x6F80,
	0x6F12,0xA060,
	0x602A,0x1A3C,
	0x6F12,0x6207,
	0x602A,0x1A48,
	0x6F12,0x6207,
	0x602A,0x1444,
	0x6F12,0x2000,
	0x6F12,0x2000,
	0x602A,0x144C,
	0x6F12,0x3F00,
	0x6F12,0x3F00,
	0x602A,0x7F6C,
	0x6F12,0x0100,
	0x6F12,0x2F00,
	0x6F12,0xFA00,
	0x6F12,0x2400,
	0x6F12,0xE500,
	0x602A,0x0650,
	0x6F12,0x0600,
	0x602A,0x0654,
	0x6F12,0x0000,
	0x602A,0x1A46,
	0x6F12,0x8A00,
	0x602A,0x1A52,
	0x6F12,0xBF00,
	0x602A,0x0674,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x0668,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x602A,0x0684,
	0x6F12,0x4001,
	0x602A,0x0688,
	0x6F12,0x4001,
	0x602A,0x147C,
	0x6F12,0x1000,
	0x602A,0x1480,
	0x6F12,0x1000,
	0x602A,0x19F6,
	0x6F12,0x0904,
	0x602A,0x0812,
	0x6F12,0x0000,
	0x602A,0x1A02,
	0x6F12,0x1800,
	0x602A,0x2148,
	0x6F12,0x0100,
	0x602A,0x2042,
	0x6F12,0x1A00,
	0x602A,0x0874,
	0x6F12,0x0100,
	0x602A,0x09C0,
	0x6F12,0x2008,
	0x602A,0x09C4,
	0x6F12,0x2000,
	0x602A,0x19FE,
	0x6F12,0x0E1C,
	0x602A,0x4D92,
	0x6F12,0x0100,
	0x602A,0x84C8,
	0x6F12,0x0100,
	0x602A,0x4D94,
	0x6F12,0x0005,
	0x6F12,0x000A,
	0x6F12,0x0010,
	0x6F12,0x0810,
	0x6F12,0x000A,
	0x6F12,0x0040,
	0x6F12,0x0810,
	0x6F12,0x0810,
	0x6F12,0x8002,
	0x6F12,0xFD03,
	0x6F12,0x0010,
	0x6F12,0x1510,
	0x602A,0x3570,
	0x6F12,0x0000,
	0x602A,0x3574,
	0x6F12,0x1201,
	0x602A,0x21E4,
	0x6F12,0x0400,
	0x602A,0x21EC,
	0x6F12,0x1F04,
	0x602A,0x2080,
	0x6F12,0x0101,
	0x6F12,0xFF00,
	0x6F12,0x7F01,
	0x6F12,0x0001,
	0x6F12,0x8001,
	0x6F12,0xD244,
	0x6F12,0xD244,
	0x6F12,0x14F4,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x20BA,
	0x6F12,0x121C,
	0x6F12,0x111C,
	0x6F12,0x54F4,
	0x602A,0x120E,
	0x6F12,0x1000,
	0x602A,0x212E,
	0x6F12,0x0200,
	0x602A,0x13AE,
	0x6F12,0x0101,
	0x602A,0x0718,
	0x6F12,0x0001,
	0x602A,0x0710,
	0x6F12,0x0002,
	0x6F12,0x0804,
	0x6F12,0x0100,
	0x602A,0x1B5C,
	0x6F12,0x0000,
	0x602A,0x0786,
	0x6F12,0x7701,
	0x602A,0x2022,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x1360,
	0x6F12,0x0100,
	0x602A,0x1376,
	0x6F12,0x0100,
	0x6F12,0x6038,
	0x6F12,0x7038,
	0x6F12,0x8038,
	0x602A,0x1386,
	0x6F12,0x0B00,
	0x602A,0x06FA,
	0x6F12,0x0000,
	0x602A,0x4A94,
	0x6F12,0x0900,
	0x6F12,0x0000,
	0x6F12,0x0300,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0300,
	0x6F12,0x0000,
	0x6F12,0x0900,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x0A76,
	0x6F12,0x1000,
	0x602A,0x0AEE,
	0x6F12,0x1000,
	0x602A,0x0B66,
	0x6F12,0x1000,
	0x602A,0x0BDE,
	0x6F12,0x1000,
	0x602A,0x0BE8,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0C56,
	0x6F12,0x1000,
	0x602A,0x0C60,
	0x6F12,0x3000,
	0x6F12,0x3000,
	0x602A,0x0CB6,
	0x6F12,0x0100,
	0x602A,0x0CF2,
	0x6F12,0x0001,
	0x602A,0x0CF0,
	0x6F12,0x0101,
	0x602A,0x11B8,
	0x6F12,0x0100,
	0x602A,0x11F6,
	0x6F12,0x0020,
	0x602A,0x4A74,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x602A,0x218E,
	0x6F12,0x0000,
	0x602A,0x2268,
	0x6F12,0xF279,
	0x602A,0x5006,
	0x6F12,0x0000,
	0x602A,0x500E,
	0x6F12,0x0100,
	0x602A,0x4E70,
	0x6F12,0x2062,
	0x6F12,0x5501,
	0x602A,0x06DC,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6028,0x4000,
	0xF46A,0xAE80,
	0x0344,0x0000,
	0x0346,0x0308,
	0x0348,0x1FFF,
	0x034A,0x1517,
	0x034C,0x0FF0,
	0x034E,0x08F8,
	0x0350,0x0008,
	0x0352,0x0008,
	0x0900,0x0122,
	0x0380,0x0002,
	0x0382,0x0002,
	0x0384,0x0002,
	0x0386,0x0002,
	0x0110,0x1002,
	0x0114,0x0300,
	0x0116,0x3000,
	0x0136,0x1800,
	0x013E,0x0000,
	0x0300,0x0006,
	0x0302,0x0001,
	0x0304,0x0004,
	0x0306,0x008C,
	0x0308,0x0008,
	0x030A,0x0001,
	0x030C,0x0000,
	0x030E,0x0004,
	0x0310,0x0064,
	0x0312,0x0000,
	0x080E,0x0000,
	0x0340,0x0C54,
	0x0342,0x1716,
	0x0702,0x0000,
	0x0202,0x0100,
	0x0200,0x0100,
	0x0D00,0x0101,
	0x0D02,0x0001,
	0x0D04,0x0102,
	0x6226,0x0000,
};
/*
static kal_uint16 addr_data_pair_custom1[] = {
	0x602A,0x1A28,
	0x6028,0x2400,
	0x6F12,0x4C00,
	0x602A,0x065A,
	0x6F12,0x0000,
	0x602A,0x139E,
	0x6F12,0x0100,
	0x602A,0x139C,
	0x6F12,0x0000,
	0x602A,0x13A0,
	0x6F12,0x0A00,
	0x6F12,0x0120,
	0x602A,0x2072,
	0x6F12,0x0000,
	0x602A,0x1A64,
	0x6F12,0x0301,
	0x6F12,0xFF00,
	0x602A,0x19E6,
	0x6F12,0x0200,
	0x602A,0x1A30,
	0x6F12,0x3401,
	0x602A,0x19FC,
	0x6F12,0x0B00,
	0x602A,0x19F4,
	0x6F12,0x0606,
	0x602A,0x19F8,
	0x6F12,0x1010,
	0x602A,0x1B26,
	0x6F12,0x6F80,
	0x6F12,0xA060,
	0x602A,0x1A3C,
	0x6F12,0x6207,
	0x602A,0x1A48,
	0x6F12,0x6207,
	0x602A,0x1444,
	0x6F12,0x2000,
	0x6F12,0x2000,
	0x602A,0x144C,
	0x6F12,0x3F00,
	0x6F12,0x3F00,
	0x602A,0x7F6C,
	0x6F12,0x0100,
	0x6F12,0x2F00,
	0x6F12,0xFA00,
	0x6F12,0x2400,
	0x6F12,0xE500,
	0x602A,0x0650,
	0x6F12,0x0600,
	0x602A,0x0654,
	0x6F12,0x0000,
	0x602A,0x1A46,
	0x6F12,0xB000,
	0x602A,0x1A52,
	0x6F12,0xBF00,
	0x602A,0x0674,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x0668,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x6F12,0x0800,
	0x602A,0x0684,
	0x6F12,0x4001,
	0x602A,0x0688,
	0x6F12,0x4001,
	0x602A,0x147C,
	0x6F12,0x1000,
	0x602A,0x1480,
	0x6F12,0x1000,
	0x602A,0x19F6,
	0x6F12,0x0904,
	0x602A,0x0812,
	0x6F12,0x0010,
	0x602A,0x2148,
	0x6F12,0x0100,
	0x602A,0x2042,
	0x6F12,0x1A00,
	0x602A,0x0874,
	0x6F12,0x0100,
	0x602A,0x09C0,
	0x6F12,0x2008,
	0x602A,0x09C4,
	0x6F12,0x2000,
	0x602A,0x19FE,
	0x6F12,0x0E1C,
	0x602A,0x4D92,
	0x6F12,0x0100,
	0x602A,0x8104,
	0x6F12,0x0100,
	0x602A,0x4D94,
	0x6F12,0x0005,
	0x6F12,0x000A,
	0x6F12,0x0010,
	0x6F12,0x1510,
	0x6F12,0x000A,
	0x6F12,0x0040,
	0x6F12,0x1510,
	0x6F12,0x1510,
	0x602A,0x3570,
	0x6F12,0x0000,
	0x602A,0x3574,
	0x6F12,0xD803,
	0x602A,0x21E4,
	0x6F12,0x0400,
	0x602A,0x21EC,
	0x6F12,0x2A01,
	0x602A,0x2080,
	0x6F12,0x0100,
	0x6F12,0xFF00,
	0x602A,0x2086,
	0x6F12,0x0001,
	0x602A,0x208E,
	0x6F12,0x14F4,
	0x602A,0x208A,
	0x6F12,0xD244,
	0x6F12,0xD244,
	0x602A,0x120E,
	0x6F12,0x1000,
	0x602A,0x212E,
	0x6F12,0x0200,
	0x602A,0x13AE,
	0x6F12,0x0101,
	0x602A,0x0718,
	0x6F12,0x0001,
	0x602A,0x0710,
	0x6F12,0x0002,
	0x6F12,0x0804,
	0x6F12,0x0100,
	0x602A,0x1B5C,
	0x6F12,0x0000,
	0x602A,0x0786,
	0x6F12,0x7701,
	0x602A,0x2022,
	0x6F12,0x0500,
	0x6F12,0x0500,
	0x602A,0x1360,
	0x6F12,0x0100,
	0x602A,0x1376,
	0x6F12,0x0100,
	0x6F12,0x6038,
	0x6F12,0x7038,
	0x6F12,0x8038,
	0x602A,0x1386,
	0x6F12,0x0B00,
	0x602A,0x06FA,
	0x6F12,0x1000,
	0x602A,0x4A94,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x6F12,0x0600,
	0x602A,0x0A76,
	0x6F12,0x1000,
	0x602A,0x0AEE,
	0x6F12,0x1000,
	0x602A,0x0B66,
	0x6F12,0x1000,
	0x602A,0x0BDE,
	0x6F12,0x1000,
	0x602A,0x0C56,
	0x6F12,0x1000,
	0x602A,0x0CF2,
	0x6F12,0x0001,
	0x602A,0x0CF0,
	0x6F12,0x0101,
	0x602A,0x11B8,
	0x6F12,0x0100,
	0x602A,0x11F6,
	0x6F12,0x0020,
	0x602A,0x4A74,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0xD8FF,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6F12,0x0000,
	0x6028,0x4000,
	0xF46A,0xAE80,
	0x0344,0x0000,
	0x0346,0x0000,
	0x0348,0x1FFF,
	0x034A,0x181F,
	0x034C,0x0FF0,
	0x034E,0x0C00,
	0x0350,0x0008,
	0x0352,0x0008,
	0x0900,0x0122,
	0x0380,0x0002,
	0x0382,0x0002,
	0x0384,0x0002,
	0x0386,0x0002,
	0x0110,0x1002,
	0x0114,0x0300,
	0x0116,0x3000,
	0x0136,0x1800,
	0x013E,0x0000,
	0x0300,0x0006,
	0x0302,0x0001,
	0x0304,0x0004,
	0x0306,0x008C,
	0x0308,0x0008,
	0x030A,0x0001,
	0x030C,0x0000,
	0x030E,0x0004,
	0x0310,0x008A,
	0x0312,0x0000,
	0x0340,0x0FE0,
	0x0342,0x11E8,
	0x0702,0x0000,
	0x0202,0x0100,
	0x0200,0x0100,
	0x0D00,0x0101,
	0x0D02,0x0001,
	0x0D04,0x0102,
	0x6226,0x0000,
};*/


static void sensor_init(void)
{
	LOG_INF("[%s] +", __func__);
	
	write_cmos_sensor(0x6028,0x4000);
	write_cmos_sensor(0x0000,0x0001);
	write_cmos_sensor(0x0000,0x38E1);
	write_cmos_sensor(0x001E,0x0005);
	write_cmos_sensor(0x6028,0x4000);
	write_cmos_sensor(0x6010,0x0001);
	mdelay(5);
	write_cmos_sensor(0x6226, 0x0001);
	mdelay(10);

	pr_err("s5kjn1 mipi setting");
	write_cmos_sensor(0xB134,0x3800);
	write_cmos_sensor(0xB132,0x0180);
	write_cmos_sensor(0xB13A,0x0000);

	table_write_cmos_sensor(addr_data_pair_init,	sizeof(addr_data_pair_init)/sizeof(kal_uint16));
	LOG_INF("[%s] -", __func__);
}	/*	sensor_init  */


static void preview_setting(void)
{
	LOG_INF("[%s] +", __func__);
	table_write_cmos_sensor(addr_data_pair_preview,	sizeof(addr_data_pair_preview)/sizeof(kal_uint16));
	LOG_INF("[%s] -", __func__);
}	/*	preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
	LOG_INF("[%s] +", __func__);
	table_write_cmos_sensor(addr_data_pair_capture,	sizeof(addr_data_pair_capture)/sizeof(kal_uint16));
	LOG_INF("[%s] -", __func__);
}/* capture setting */
 
  //rm for tests
static void normal_video_setting(kal_uint16 currefps)
{
	LOG_INF("[%s] +", __func__);
	table_write_cmos_sensor(addr_data_pair_normal_video,	sizeof(addr_data_pair_normal_video)/sizeof(kal_uint16));
	LOG_INF("[%s] -", __func__);
}
 


static void hs_video_setting(void)
{
	LOG_INF("[%s] +", __func__);
	table_write_cmos_sensor(addr_data_pair_hs_video,	sizeof(addr_data_pair_hs_video)/sizeof(kal_uint16));
	LOG_INF("[%s] -", __func__);
}

static void slim_video_setting(void)
{
	LOG_INF("[%s] +", __func__);
	table_write_cmos_sensor(addr_data_pair_slim_video,	sizeof(addr_data_pair_slim_video)/sizeof(kal_uint16));
	LOG_INF("[%s] -", __func__);
}

/*static void custom1_setting(void)
{
	LOG_INF("[%s] start", __func__);
	LOG_INF("[%s] +", __func__);
	table_write_cmos_sensor(addr_data_pair_custom1,	sizeof(addr_data_pair_custom1)/sizeof(kal_uint16));
	LOG_INF("[%s] -", __func__);
    LOG_INF("[%s] end", __func__);
	
} *//* custom1 setting */



static kal_uint32 return_sensor_id(void)
{
    return ((read_cmos_sensor_byte(0x0000) << 8) | read_cmos_sensor_byte(0x0001));
}

/*************************************************************************
 * read sensor module id.
 ************************************************************************/
static kal_uint16 read_cmos_sensor_eeprom(kal_uint16 addr)
{
    kal_uint16 get_byte = 0;
    char pu_send_cmd[2] = { (char)(addr >> 8), (char)(addr & 0xFF) };

    iReadRegI2C(pu_send_cmd, 2, (u8 *) &get_byte, 1, EEPROM_WRITE_ID);
	return get_byte;
}

/*************************************************************************
 * FUNCTION
 * return_module_id
 * DESCRIPTION
 * This function get the sensor module ID
 * PARAMETERS
 * none
 * RETURNS
 * module id.
 *
 * GLOBALS AFFECTED
 *
 * **********************************************************************/
/*static kal_uint16 return_module_id(void)
{
   return read_cmos_sensor_eeprom(0x0001);
}*/

static int get_eeprom_data(MUINT8 *data)
{
    MUINT8 i =0x0;
    u8 *otp_data = (u8 *)data;

    for (i = 0x0015;i <= 0x0024; i++, otp_data++){
        *otp_data = read_cmos_sensor_eeprom(i);
        pr_err("wrt pOtp_data s5kjn1 otpdata[0x%x]=0x%x \n", i, *otp_data);
    }
    return 0;
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
    //sensor have two i2c address 0x5b 0x5a & 0x21 0x20, we should detect the module used i2c address

	LOG_INF("[%s] +", __func__);

    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
	spin_unlock(&imgsensor_drv_lock);
	do {
	    *sensor_id = return_sensor_id();
	if (*sensor_id == imgsensor_info.sensor_id) {
		get_eeprom_data(pOtp_data);
		LOG_INF("i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id, *sensor_id, imgsensor_info.sensor_id);
#ifdef CONFIG_TINNO_PRODUCT_INFO
		FULL_PRODUCT_DEVICE_INFO(ID_MAIN_CAM_SN, pOtp_data);
                if('Q'==read_cmos_sensor_eeprom(0x000D) &&'T'==read_cmos_sensor_eeprom(0x000E))
		FULL_PRODUCT_DEVICE_INFO_CAMERA(S5KJN1_QT_P410AE_SENSOR_ID,0,"s5kjn1_qt_p410ae_mipi_raw",8160,6144);
               else if('O'==read_cmos_sensor_eeprom(0x000D) &&'F'==read_cmos_sensor_eeprom(0x000E))
		FULL_PRODUCT_DEVICE_INFO_CAMERA(S5KJN1_QT_P410AE_SENSOR_ID,0,"s5kjn1_of_p410ae_mipi_raw",8160,6144);
#endif
	return ERROR_NONE;
	}
			LOG_INF("Read sensor id fail, i2c write id: 0x%x, ReadOut sensor id: 0x%x, imgsensor_info.sensor_id:0x%x.\n", imgsensor.i2c_write_id, *sensor_id, imgsensor_info.sensor_id);
	    retry--;
	} while (retry > 0);
	i++;
	retry = 1;
    }

    if (*sensor_id != imgsensor_info.sensor_id) {
	*sensor_id = 0xFFFFFFFF;
		LOG_INF("[%s] -error-", __func__);
	return ERROR_SENSOR_CONNECT_FAIL;
    }
	LOG_INF("[%s] -", __func__);
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
   printk("lihao+open-start");
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            sensor_id = return_sensor_id();
            if (sensor_id == imgsensor_info.sensor_id) {
             
                printk("lihao+open func i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
                break;
               
	     }
            printk("lihao++open func iRead sensor id fail,try time: 0x%x, id: 0x%x, sensor id: 0x%x\n", -(retry-2),imgsensor.i2c_write_id,sensor_id);
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

    //[agold][chenwei][20190313][start]
    #ifndef AGOLD_S5KJN1_AWB_DISABLE
    //s5kjn1kw_MIPI_update_wb_register_from_otp();
    #endif
    //[agold][chenwei][20190313][end]
	
    spin_lock(&imgsensor_drv_lock);
    imgsensor.autoflicker_en= KAL_FALSE;
    imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.dummy_pixel = 0;
    imgsensor.dummy_line = 0;
    imgsensor.ihdr_en = 0;//KAL_FALSE;
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
	LOG_INF("E  preview\n");
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
	set_mirror_flip(IMAGE_HV_MIRROR);	
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
	LOG_INF("E  capture\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	//if (imgsensor.current_fps == imgsensor_info.cap.max_framerate) {
		LOG_INF("capture30fps: use cap30FPS's setting: %d fps!\n",imgsensor.current_fps/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;  
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	//} 
	/*else  
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
	}*/
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps); 
	set_mirror_flip(IMAGE_HV_MIRROR);	
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
	set_mirror_flip(IMAGE_HV_MIRROR);	
	
	
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
	set_mirror_flip(IMAGE_HV_MIRROR);
	
	return ERROR_NONE;
}	/*	hs_video   */



static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
				  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	/*imgsensor.video_mode = KAL_TRUE;*/
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/*imgsensor.current_fps = 300;*/
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	set_mirror_flip(IMAGE_HV_MIRROR);
	return ERROR_NONE;
}	

/*static kal_uint32 custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
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
	//PIP24fps_capture_setting(imgsensor.current_fps,imgsensor.pdaf_mode);
	custom1_setting();
	set_mirror_flip(IMAGE_HV_MIRROR);
	return ERROR_NONE;
}*/



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
    //sensor_resolution->SensorCustom1Width  = imgsensor_info.custom1.grabwindow_width;
    //sensor_resolution->SensorCustom1Height     = imgsensor_info.custom1.grabwindow_height;
	
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
    sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;
	//strncpy(sensor_info->TinnoModuleSn,pOtp_data,sizeof(pOtp_data));
	memcpy((void *)sensor_info->TinnoModuleSn,(void *)pOtp_data,sizeof(pOtp_data));
    //sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame; 

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
	
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 		 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;	
	sensor_info->FrameTimeDelayFrame = imgsensor_info.frame_time_delay_frame; /* The delay frame of setting frame length  */
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
	sensor_info->PDAF_Support = 2;
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
        /*case MSDK_SCENARIO_ID_CUSTOM1:
            sensor_info->SensorGrabStartX = imgsensor_info.custom1.startx; 
            sensor_info->SensorGrabStartY = imgsensor_info.custom1.starty;   
            sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.custom1.mipi_data_lp2hs_settle_dc; 

            break;*/
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
        /*case MSDK_SCENARIO_ID_CUSTOM1:
		custom1(image_window, sensor_config_data);
            break;*/
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
				LOG_INF("LZH 1imgsensor.frame_length = %d, imgsensor.shutter = %d\n", imgsensor.frame_length, imgsensor.shutter);
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
			//if(framerate==300)
			{
			frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}
		/*	else
			{
			frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			}*/
				LOG_INF("LZH 3imgsensor.frame_length = %d, imgsensor.shutter = %d\n", imgsensor.frame_length, imgsensor.shutter);
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
				LOG_INF("LZH 4imgsensor.frame_length = %d, imgsensor.shutter = %d\n", imgsensor.frame_length, imgsensor.shutter);
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
				LOG_INF("LZH 5imgsensor.frame_length = %d, imgsensor.shutter = %d\n", imgsensor.frame_length, imgsensor.shutter);
			if (imgsensor.frame_length > imgsensor.shutter) 
				set_dummy();
		
		break;
        /*case MSDK_SCENARIO_ID_CUSTOM1:
            frame_length = imgsensor_info.custom1.pclk / framerate * 10 / imgsensor_info.custom1.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.custom1.framelength) ? (frame_length - imgsensor_info.custom1.framelength) : 0;
            if (imgsensor.dummy_line < 0)
                imgsensor.dummy_line = 0;
            imgsensor.frame_length = imgsensor_info.custom1.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
				LOG_INF("LZH 6imgsensor.frame_length = %d, imgsensor.shutter = %d\n", imgsensor.frame_length, imgsensor.shutter);
			if (imgsensor.frame_length > imgsensor.shutter) set_dummy();
		
            break;*/
		default:  //coding with  preview scenario by default
			frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
			spin_lock(&imgsensor_drv_lock);
			imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
			imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
			imgsensor.min_frame_length = imgsensor.frame_length;
			spin_unlock(&imgsensor_drv_lock);
			LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
			LOG_INF("LZH 7imgsensor.frame_length = %d, imgsensor.shutter = %d\n", imgsensor.frame_length, imgsensor.shutter);
			if (imgsensor.frame_length > imgsensor.shutter) 
				set_dummy();
			
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
        /*case MSDK_SCENARIO_ID_CUSTOM1:
            *framerate = imgsensor_info.custom1.max_framerate;
            break;*/
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
	 write_cmos_sensor(0x0600, 0x0001);
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
    kal_uint32 rate;
    struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;

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

            wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));

            switch (*feature_data_32) {
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
                    break;
                case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
                    break;
                case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
                    break;
                case MSDK_SCENARIO_ID_SLIM_VIDEO:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
                    break;
                /*case MSDK_SCENARIO_ID_CUSTOM1:
                    memcpy((void *)wininfo, (void *)&imgsensor_winsize_info[5],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
                    break;*/
                case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
                default:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
                    break;
            }
			break;
        case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
            LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            break;

	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME\n");
		streaming_control(KAL_TRUE);
		break;
        /******************** PDAF START >>> *********/
		case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
			LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%llu\n", *feature_data);
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
				default:
					*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
					break;
			}
			break;
		case SENSOR_FEATURE_GET_PDAF_INFO:
			LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%llu\n", *feature_data);
			PDAFinfo= (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));
		
			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(struct SET_PD_BLOCK_INFO_T));
					break;
				case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(struct SET_PD_BLOCK_INFO_T));
				  break;
				case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
				default:
					break;
			}
			break;
		case SENSOR_FEATURE_GET_VC_INFO:
		pr_debug("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16)*feature_data);
		pvcinfo =
		 (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO[2],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO[1],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy(
				(void *)pvcinfo,
				(void *)&SENSOR_VC_INFO[0],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		}
		break;
		case SENSOR_FEATURE_GET_PDAF_DATA:	
			LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
			//S5K3m5_read_eeprom((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
			break;	
        /******************** PDAF END   <<< *********/
		case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:/*lzl*/
				set_shutter_frame_length((UINT16)*feature_data, (UINT16)*(feature_data+1));
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

		case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
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
				case MSDK_SCENARIO_ID_SLIM_VIDEO:
					rate = imgsensor_info.slim_video.mipi_pixel_rate;
					break;
				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				default:
					rate = imgsensor_info.pre.mipi_pixel_rate;
					break;
			}
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
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

UINT32 S5KJN1_QT_P410AE_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL) {
		*pfFunc=&sensor_func;
	}
	return ERROR_NONE;
}	/*	S5Kjn1_MIPI_RAW_SensorInit	*/


