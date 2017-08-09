/*****************************************************************************
 *
 * Filename:
 * ---------
 *     s5k2x8mipi_Sensor.c
 *
 * Project:
 * --------
 *     ALPS
 *
 * Description:
 * ------------
 *     Source code of Sensor driver
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k2x8mipiraw_Sensor.h"


//#include "s5k2x8_otp.h"
/****************************Modify Following Strings for Debug****************************/
#define PFX "s5k2x8_camera_sensor"
#define LOG_1 LOG_INF("s5k2x8,MIPI 4LANE\n")
/****************************   Modify end    *******************************************/
#define LOG_INF(fmt, args...)   pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##args)
kal_uint32 chip_id = 0;

static DEFINE_SPINLOCK(imgsensor_drv_lock);
#define ORIGINAL_VERSION 1
//#define SLOW_MOTION_120FPS
static imgsensor_info_struct imgsensor_info = {
    .sensor_id = S5K2X8_SENSOR_ID,        //record sensor id defined in Kd_imgsensor.h

    .checksum_value = 0xafb5098f,      //checksum value for Camera Auto Test

	.pre = {
		.pclk = 720000000,				//record different mode's pclk
		.linelength = 6864,				//record different mode's linelength
		.framelength =2300, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 2816,		//record different mode's width of grabwindow
		.grabwindow_height = 2112,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		//.max_framerate = 450,
		.max_framerate = 450,
	},
	.cap = {
	    /*Mipi datarate 1.632 Gbps*/
		.pclk = 712320000,				//record different mode's pclk
		.linelength = 6860,				//record different mode's linelength
		.framelength =4324, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 5632,		//record different mode's width of grabwindow
		.grabwindow_height = 4224,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		.max_framerate = 240,
	},
	.cap1 = {
		.pclk = 720000000,				//record different mode's pclk
		.linelength = 7640, 			//record different mode's linelength
		.framelength =4350, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 5632,		//record different mode's width of grabwindow
		.grabwindow_height = 4224,		//record different mode's height of grabwindow
	
		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 214,
	},

	.normal_video = {
		.pclk = 720000000,				//record different mode's pclk
		.linelength = 7640,				//record different mode's linelength
		.framelength =3292, //			//record different mode's framelength
		.startx = 0,					//record different mode's startx of grabwindow
		.starty = 0,					//record different mode's starty of grabwindow
		.grabwindow_width = 5632,		//record different mode's width of grabwindow
		.grabwindow_height = 3168,		//record different mode's height of grabwindow

		/*	 following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario	*/
		.mipi_data_lp2hs_settle_dc = 85,  //unit , ns
		/*	 following for GetDefaultFramerateByScenario()	*/
		.max_framerate = 283,
	},

	.hs_video = {
		.pclk = 720000000,
		.linelength = 6864,
		.framelength = 1716,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2816,		//record different mode's width of grabwindow
		.grabwindow_height = 1584,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 600,
	},

	.slim_video = {
		.pclk = 720000000,
		.linelength = 9200,
		.framelength = 872,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 1408,		//record different mode's width of grabwindow
		.grabwindow_height = 792,		//record different mode's height of grabwindow
		.mipi_data_lp2hs_settle_dc = 85,//unit , ns
		.max_framerate = 890,
	},
	.margin = 16,
	.min_shutter = 1,
	.max_frame_length = 0xffff,
	.ae_shut_delay_frame = 0,
	.ae_sensor_gain_delay_frame = 0,
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,	  //1, support; 0,not support
	.ihdr_le_firstline = 0,  //1,le first ; 0, se first
	.sensor_mode_num = 5,	  //support sensor mode num
	.cap_delay_frame = 3,
	.pre_delay_frame = 3,
	.video_delay_frame = 3,
	.hs_video_delay_frame = 3,
	.slim_video_delay_frame = 3,
	.isp_driving_current = ISP_DRIVING_6MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
	.mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,
	.mclk = 24,
	.mipi_lane_num = SENSOR_MIPI_4_LANE,
	.i2c_addr_table = { 0x5A, 0xff},
};


static imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
	.shutter = 0x3D0,					//current shutter
	.gain = 0x100,						//current gain
	.dummy_pixel = 0,					//current dummypixel
	.dummy_line = 0,					//current dummyline
	.current_fps = 0,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
	.autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
	.test_pattern = KAL_FALSE,		//test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
	.hdr_mode = KAL_FALSE, //sensor need support LE, SE with HDR feature
	.i2c_write_id = 0x5A,
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
/* full_w; full_h; x0_offset; y0_offset; w0_size; h0_size; scale_w; scale_h; x1_offset;  y1_offset;  w1_size;  h1_size;
     x2_tg_offset;   y2_tg_offset;  w2_tg_size;  h2_tg_size;*/
{
 { 5664, 4256, 16, 16,  5632, 4224, 2816, 2112, 0, 0, 2816, 2112, 0, 0, 2816, 2112}, // Preview
 { 5664, 4256, 16, 16,  5632, 4224, 5632, 4224, 0, 0, 5632, 4224, 0, 0, 5632, 4224}, // capture
 { 5664, 4256, 16, 544, 5632, 3168, 5632, 3168, 0, 0, 5632, 3168, 0, 0, 5632, 3168}, // normal_video
 { 5664, 4256, 16, 544, 5632, 3168, 2816, 1584, 0, 0, 2816, 1584, 0, 0, 2816, 1584}, // hs_video
 { 5664, 4256, 16, 544, 5632, 3168, 1408,  792, 0, 0, 1408,  792, 0, 0, 1408,  792}, // slim_video
};

//#define USE_OIS
#ifdef USE_OIS
#define OIS_I2C_WRITE_ID 0x48
#define OIS_I2C_READ_ID 0x49

#define RUMBA_OIS_CTRL   0x0000
#define RUMBA_OIS_STATUS 0x0001
#define RUMBA_OIS_MODE   0x0002
#define CENTERING_MODE   0x05
#define RUMBA_OIS_OFF	 0x0030

#define RUMBA_OIS_SETTING_ADD 0x0002
#define RUMBA_OIS_PRE_SETTING 0x02
#define RUMBA_OIS_CAP_SETTING 0x01


#define RUMBA_OIS_PRE    0
#define RUMBA_OIS_CAP    1


static void OIS_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
    iWriteRegI2C(pusendcmd , 3, OIS_I2C_WRITE_ID);
}
static kal_uint16 OIS_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(pusendcmd , 2, (u8*)&get_byte,1,OIS_I2C_READ_ID);
    return get_byte;
}
static int OIS_on(int mode)
{
 	int ret = 0;
	if(mode == RUMBA_OIS_PRE_SETTING){
		OIS_write_cmos_sensor(RUMBA_OIS_SETTING_ADD,RUMBA_OIS_PRE_SETTING);
	}
	if(mode == RUMBA_OIS_CAP_SETTING){
		OIS_write_cmos_sensor(RUMBA_OIS_SETTING_ADD,RUMBA_OIS_CAP_SETTING);
	}
	OIS_write_cmos_sensor(RUMBA_OIS_MODE,CENTERING_MODE);
	ret = OIS_read_cmos_sensor(RUMBA_OIS_MODE);
	LOG_INF("pangfei OIS ret=%d %s %d\n",ret,__func__,__LINE__);

	if(ret != CENTERING_MODE){
		//return -1;
	}
	OIS_write_cmos_sensor(RUMBA_OIS_CTRL,0x01);
	ret = OIS_read_cmos_sensor(RUMBA_OIS_CTRL);
	LOG_INF("pangfei OIS ret=%d %s %d\n",ret,__func__,__LINE__);
	if(ret != 0x01){
		//return -1;
	}

}

static int OIS_off(void)
{
	int ret = 0;
	OIS_write_cmos_sensor(RUMBA_OIS_OFF,0x01);
	ret = OIS_read_cmos_sensor(RUMBA_OIS_OFF);
	LOG_INF("pangfei OIS ret=%d %s %d\n",ret,__func__,__LINE__);
}
#endif
//add for s5k2x8 pdaf
static SET_PD_BLOCK_INFO_T imgsensor_pd_info =

{

    .i4OffsetX = 64,
    .i4OffsetY = 65,
    .i4PitchX  = 64,
    .i4PitchY  = 64,
    .i4PairNum  =16,
    .i4SubBlkW  =16,
    .i4SubBlkH  =16,
         .i4PosL = {{64,65},{116,65},{80,69},{100,69},{68,85},{112,85},{84,89},{96,89},{84,97},{96,97},{68,101},{112,101},{80,117},{100,117},{64,121},{116,121}},
    .i4PosR = {{64,69},{116,69},{80,73},{100,73},{68,81},{112,81},{84,85},{96,85},{84,101},{96,101},{68,105},{112,105},{80,113},{100,113},{64,117},{116,117}},

};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;

    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

    return get_byte;
}
static kal_uint16 read_cmos_sensor_twobyte(kal_uint32 addr)
{
    kal_uint16 get_byte = 0;
	char get_word[2];
    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd, 2, get_word, 2, imgsensor.i2c_write_id);
	get_byte = (((int)get_word[0])<<8) | get_word[1];
    return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
}

static void write_cmos_sensor_twobyte(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8),(char)(para & 0xFF)};
    LOG_INF("write_cmos_sensor_twobyte is %x,%x,%x,%x\n", pu_send_cmd[0], pu_send_cmd[1], pu_send_cmd[2], pu_send_cmd[3]);
    iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
}
static void set_dummy(void)
{
#if 1
    LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
    //write_cmos_sensor(0x0104, 0x01);
	//write_cmos_sensor_twobyte(0x6028,0x4000);
	//write_cmos_sensor_twobyte(0x602A,0xC340 );
	write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);

	//write_cmos_sensor_twobyte(0x602A,0xC342 );
	write_cmos_sensor_twobyte(0x0342, imgsensor.line_length);

    //write_cmos_sensor(0x0104, 0x00);
#endif
#if 0
    LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
    write_cmos_sensor(0x0104, 0x01);
    write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);
    write_cmos_sensor_twobyte(0x0342, imgsensor.line_length);
    write_cmos_sensor(0x0104, 0x00);
#endif
}    /*    set_dummy  */

static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
    //kal_int16 dummy_line;
    kal_uint32 frame_length = imgsensor.frame_length;
    //unsigned long flags;

    LOG_INF("framerate = %d, min framelength should enable = %d \n", framerate,min_framelength_en);

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
}    /*    set_max_framerate  */


/*************************************************************************
* FUNCTION
*    set_shutter
*
* DESCRIPTION
*    This function set e-shutter of sensor to change exposure time.
*    The registers 0x3500 ,0x3501 and 0x3502 control exposure of s5k2x8.
*    The exposure value is in number of Tline, where Tline is the time of sensor one line.
*
*    Exposure = [reg 0x3500]<<12 + [reg 0x3501]<<4 + [reg 0x3502]>>4;
*    The maximum exposure value is limited by VTS defined by register 0x380e and 0x380f.
      Maximum Exposure <= VTS -4
*
* PARAMETERS
*    iShutter : exposured lines
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
    //LOG_INF("enter xxxx  set_shutter, shutter =%d\n", shutter);

    unsigned long flags;
    //kal_uint16 realtime_fps = 0;
    //kal_uint32 frame_length = 0;
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	
	LOG_INF("set_shutter =%d\n", shutter);
    // OV Recommend Solution
    // if shutter bigger than frame_length, should extend frame length first
	if(!shutter) shutter = 1; /*avoid 0*/
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

// Frame length :4000 C340
	//write_cmos_sensor_twobyte(0x6028,0x4000);
	//write_cmos_sensor_twobyte(0x602A,0xC340 );
	write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);

//Shutter reg : 4000 C202
	//write_cmos_sensor_twobyte(0x6028,0x4000);
	//write_cmos_sensor_twobyte(0x602A,0xC202 );
	write_cmos_sensor_twobyte(0x0202,shutter);
	write_cmos_sensor_twobyte(0x021e,shutter);


#if 0
    write_cmos_sensor(0x0104,0x01);  //\D5\E2\B8\F6reg\D3\C3;\CA\C7?    write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);
    write_cmos_sensor_twobyte(0x0202, shutter);
    write_cmos_sensor(0x0104,0x00);
    LOG_INF("Exit!JEFF shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);
#endif
}    /*    set_shutter */

static void hdr_write_shutter(kal_uint16 le, kal_uint16 se)
{
	//LOG_INF("enter xxxx  set_shutter, shutter =%d\n", shutter);

	unsigned long flags;
	//kal_uint16 realtime_fps = 0;
	//kal_uint32 frame_length = 0;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = le;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	LOG_INF("HDR set shutter =%d\n", le);
	if(!le) le = 1; /*avoid 0*/
	
	spin_lock(&imgsensor_drv_lock);
	if (le > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = le + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	
	spin_unlock(&imgsensor_drv_lock);

	le = (le < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : le;
	le = (le > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : le;

	// Frame length :4000 C340
	//write_cmos_sensor_twobyte(0x6028,0x4000);
	//write_cmos_sensor_twobyte(0x602A,0xC340 );
	write_cmos_sensor_twobyte(0x0340, imgsensor.frame_length);

	/*Short exposure */
	write_cmos_sensor_twobyte(0x0202,se);
	/*Log exposure ratio*/
	write_cmos_sensor_twobyte(0x021e,le);
	


}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 reg_gain = 0x0;
	reg_gain = gain/2;
	return (kal_uint16)reg_gain;
}

/*************************************************************************
* FUNCTION
*    set_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    iGain : sensor global gain(base: 0x80)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{

	  kal_uint16 reg_gain;
	  kal_uint32 sensor_gain1 = 0;
	  kal_uint32 sensor_gain2 = 0;
    /* 0x350A[0:1], 0x350B[0:7] AGC real gain */
    /* [0:3] = N meams N /16 X  */
    /* [4:9] = M meams M X       */
    /* Total gain = M + N /16 X   */

    //
    if (gain < BASEGAIN || gain > 32 * BASEGAIN) {
        LOG_INF("Error gain setting");

        if (gain < BASEGAIN)
            gain = BASEGAIN;
        else if (gain > 32 * BASEGAIN)
            gain = 32 * BASEGAIN;
    }

    reg_gain = gain2reg(gain);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.gain = reg_gain;
    spin_unlock(&imgsensor_drv_lock);
    LOG_INF("gain = %d , reg_gain = 0x%x ", gain, reg_gain);

//Analog gain HW reg : 4000 C204
	write_cmos_sensor_twobyte(0x6028,0x4000);
	write_cmos_sensor_twobyte(0x602A,0x0204 );
	write_cmos_sensor_twobyte(0x6F12,reg_gain);
	write_cmos_sensor_twobyte(0x6F12,reg_gain);
#if 0
    write_cmos_sensor(0x0104, 0x01);
    write_cmos_sensor(0x0204,(reg_gain>>8));
    write_cmos_sensor(0x0205,(reg_gain&0xff));
    write_cmos_sensor(0x0104, 0x00);
#endif
    write_cmos_sensor_twobyte(0x602C,0x4000);
    write_cmos_sensor_twobyte(0x602E, 0x0204);
    sensor_gain1 = read_cmos_sensor_twobyte(0x6F12);
    LOG_INF("JEFF get_imgsensor_gain1 (0x%x)\n", sensor_gain1 );

	write_cmos_sensor_twobyte(0x602C,0x4000);
    write_cmos_sensor_twobyte(0x602E, 0x0206);
	sensor_gain2 = read_cmos_sensor_twobyte(0x6F12);
	LOG_INF("JEFF get_imgsensor_gain2 (0x%x)\n", sensor_gain2 );

	return gain;

}    /*    set_gain  */

static void set_mirror_flip(kal_uint8 image_mirror)
{
    LOG_INF("image_mirror = %d\n", image_mirror);
#if 1
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
            write_cmos_sensor(0x0101,0x00);   // Gr
            break;
        case IMAGE_H_MIRROR:
            write_cmos_sensor(0x0101,0x01);
            break;
        case IMAGE_V_MIRROR:
            write_cmos_sensor(0x0101,0x02);
            break;
        case IMAGE_HV_MIRROR:
            write_cmos_sensor(0x0101,0x03);//Gb
            break;
        default:
			LOG_INF("Error image_mirror setting\n");

    }
#endif
}

/*************************************************************************
* FUNCTION
*    night_mode
*
* DESCRIPTION
*    This function night mode of sensor.
*
* PARAMETERS
*    bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}    /*    night_mode    */

// #define  S5K2X8FW



#ifdef S5K2X8FW
extern int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId);
#define S5K2X8XXMIPI_WRITE_ID 0x5A
#define I2C_BUFFER_LEN  254
static kal_uint16 S5K2X8S_burst_download_cmos_sensor(kal_uint16* para,  kal_uint32 len)
{
         unsigned char puSendCmd[I2C_BUFFER_LEN];
         kal_uint32 tosend = 0;
         kal_uint32 IDX = 0;
         kal_uint32 totalCount = 0;
         kal_uint32 addr, data;
         kal_uint32 old_page=0;
         kal_uint32 burst_count = 0;
         kal_uint32 page_count = 0;

         /**set page 0x0000*/
         write_cmos_sensor_twobyte(0x6028,0x0000);
         LOG_INF("JEFF:burst  file para[0]  (0x%x)\n", para[0]);

         while(IDX*2 < len)
         {
                   if(tosend == 0)
                   {
                            addr= IDX*2;
                            write_cmos_sensor_twobyte(0x602A,addr&0xFFFF);

                            puSendCmd[tosend++] = 0x6F;
                            //LOG_INF("JEFF:burst  file , puSendCmd(0x%x), [tosend ](%d)\n",puSendCmd[tosend - 1], tosend - 1);

                            puSendCmd[tosend++] = 0x12;
                            //LOG_INF("JEFF:burst  file , puSendCmd(0x%x), [tosend ](%d)\n",puSendCmd[tosend - 1], tosend - 1);
                   }
                   data = para[IDX++];

                   //puSendCmd[tosend++] = (u8)(data >> 8);//big endian
                   puSendCmd[tosend++] = (u8)(data & 0xFF);
                  //LOG_INF("JEFF:burst  file , puSendCmd(0x%x),[tosend ](%d)\n",puSendCmd[tosend - 1], tosend - 1);

                   //puSendCmd[tosend++] = (u8)(data & 0xFF);//big endian
                   puSendCmd[tosend++] = (u8)(data >> 8);
                   //LOG_INF("JEFF:burst  file , puSendCmd(0x%x),[tosend ](%d)\n",puSendCmd[tosend - 1], tosend - 1);

                   if ((tosend  >= I2C_BUFFER_LEN) ||( IDX*2 >= len) || (old_page!=((IDX)>>15)))
                   {
                            LOG_INF("JEFF:burst  file start, tosend(%d), IDX(%d), old_page(%d)\n",tosend,IDX,old_page);

                            iBurstWriteReg(puSendCmd , tosend, S5K2X8XXMIPI_WRITE_ID);
                            totalCount += tosend;
                            tosend = 0;

                            if(old_page!=((IDX)>>15))
                            {
                                     write_cmos_sensor_twobyte(0x6028,((IDX)>>15));  //15 pointer offset
                                     old_page=((IDX)>>15);
                                     ++page_count;
                            }
                            ++burst_count;
                   }
         }
         LOG_INF("JEFF:burst  file to sensor totalCount(%d), IDX(%d), old_page(%d)\n", totalCount, IDX, old_page);
         LOG_INF("JEFF:burst  file to sensor page_count(%d), burst_count(%d), old_page(%d)\n", page_count, burst_count);


         return totalCount;
}

static void Sensor_FW_read_downlaod(char*  filename)
{
         /**getting binary size for buffer allocation **/
         static kal_uint8* s5k2x8_SensorFirmware;
         loff_t lof = 0;
         kal_uint32  file_size = 0;
         kal_uint32 ret;
         struct file *filp;
         mm_segment_t oldfs;
         oldfs = get_fs();
         set_fs(KERNEL_DS);

         LOG_INF("Sensor_FW_read_downlaod.\n");
        // filp = filp_open(filename, O_RDWR, S_IRUSR | S_IWUSR);
         filp = filp_open(filename, O_RDONLY, 0);
         if (IS_ERR(filp))
         {
             LOG_INF(KERN_INFO "Unable to load '%s'.\n", filename);
             return;
         }
         else
         {
             LOG_INF("JEFF:open s5k2x8 bin file sucess \n");
         }

         lof = vfs_llseek(filp, lof , SEEK_END);
         file_size = lof;
         LOG_INF("JEFF:The bin file length (%d)\n", file_size);
         if (file_size <= 0 )
         {
                   LOG_INF(KERN_INFO "Invalid firmware '%s'\n",filename);
                   filp_close(filp,0);
                   return ;
         }

         s5k2x8_SensorFirmware = kmalloc(file_size, GFP_KERNEL);
         if(s5k2x8_SensorFirmware == 0)
          {
                LOG_INF("JEFF:kmalloc failed \n");
                   filp_close(filp,0);
                 return ;
           }
          memset(s5k2x8_SensorFirmware, 0, file_size);

         filp->f_pos = 0;
         ret = vfs_read(filp, s5k2x8_SensorFirmware, file_size , &(filp->f_pos));
         if (file_size!= ret)
         {
                   LOG_INF(KERN_INFO "Failed to read '%s'.\n", filename);
                 kfree(s5k2x8_SensorFirmware);
                   filp_close(filp, NULL);
                   return ;
         }

/*******************************************************************************/
         /**FW_download*/
         write_cmos_sensor_twobyte(0x6028, 0x4000);

         //Reset & Remap
         write_cmos_sensor_twobyte(0x6042, 0x0001);

         //Auto increment enable
         write_cmos_sensor_twobyte(0x6004, 0x0001);

         // Download Sensor FW
         write_cmos_sensor_twobyte(0x6028,0x0000);
         S5K2X8S_burst_download_cmos_sensor ((kal_uint16*)s5k2x8_SensorFirmware, file_size);

         write_cmos_sensor_twobyte(0x6028,0x4000);
         //Reset & Start FW
         write_cmos_sensor_twobyte(0x6040, 0x0001);

         // FW Init time
         mdelay(6);
         set_fs(oldfs);
         kfree(s5k2x8_SensorFirmware);
         filp_close(filp, NULL);
}


#endif

static void sensor_init_10(void)
{
         LOG_INF("Enter s5k2x8 sensor_init.\n");

// Clock Gen
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x6214);
write_cmos_sensor_twobyte(0x6F12,0xFFFF);
write_cmos_sensor_twobyte(0x6F12,0xFFFF);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// Start T&P part
// DO NOT DELETE T&P SECTION COMMENTS! They are required to debug T&P related issues.
// https://ssemi-fsrv0331:8443/svn/2X8S_FW_REP/branches/2X8_EVT0_r515_20150113_TnP/2X8S_OutputFiles/2X8S_FW_Release
// SVN Rev: 621-621
// ROM Rev: 2X8S_FW_Release
// Signature:
// md5 f4a41fde2c6f895e001e899e73da0efd .btp
// md5 217990b382580e0f1de442b392a079b1 .htp
// md5 dacde8f3268510626953235788e1cf1c .RegsMap.h
// md5 79d2180e2bda7e412e8b04cf9cbf4b5d .RegsMap.bin
//
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0x4DB0);
write_cmos_sensor_twobyte(0x6F12,0x0449);
write_cmos_sensor_twobyte(0x6F12,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x044A);
write_cmos_sensor_twobyte(0x6F12,0x0860);
write_cmos_sensor_twobyte(0x6F12,0x101A);
write_cmos_sensor_twobyte(0x6F12,0x4860);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x31BB);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x55F0);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xBE50);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0xAE00);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF041);
write_cmos_sensor_twobyte(0x6F12,0x0E46);
write_cmos_sensor_twobyte(0x6F12,0xF849);
write_cmos_sensor_twobyte(0x6F12,0x0746);
write_cmos_sensor_twobyte(0x6F12,0x0888);
write_cmos_sensor_twobyte(0x6F12,0x401C);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0x301D);
write_cmos_sensor_twobyte(0x6F12,0x00F5);
write_cmos_sensor_twobyte(0x6F12,0x9C70);
write_cmos_sensor_twobyte(0x6F12,0x0546);
write_cmos_sensor_twobyte(0x6F12,0x301D);
write_cmos_sensor_twobyte(0x6F12,0xC030);
write_cmos_sensor_twobyte(0x6F12,0x00BF);
write_cmos_sensor_twobyte(0x6F12,0x807A);
write_cmos_sensor_twobyte(0x6F12,0x6988);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x0C00);
write_cmos_sensor_twobyte(0x6F12,0x8140);
write_cmos_sensor_twobyte(0x6F12,0x8CB2);
write_cmos_sensor_twobyte(0x6F12,0x2246);
write_cmos_sensor_twobyte(0x6F12,0x0021);
write_cmos_sensor_twobyte(0x6F12,0x3046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x92FB);
write_cmos_sensor_twobyte(0x6F12,0x2878);
write_cmos_sensor_twobyte(0x6F12,0x0128);
write_cmos_sensor_twobyte(0x6F12,0x0AD0);
write_cmos_sensor_twobyte(0x6F12,0x0228);
write_cmos_sensor_twobyte(0x6F12,0x08D0);
write_cmos_sensor_twobyte(0x6F12,0x2246);
write_cmos_sensor_twobyte(0x6F12,0x0121);
write_cmos_sensor_twobyte(0x6F12,0x3046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x88FB);
write_cmos_sensor_twobyte(0x6F12,0x2878);
write_cmos_sensor_twobyte(0x6F12,0x0328);
write_cmos_sensor_twobyte(0x6F12,0x02D0);
write_cmos_sensor_twobyte(0x6F12,0x02E0);
write_cmos_sensor_twobyte(0x6F12,0x0024);
write_cmos_sensor_twobyte(0x6F12,0xF4E7);
write_cmos_sensor_twobyte(0x6F12,0x0024);
write_cmos_sensor_twobyte(0x6F12,0x2246);
write_cmos_sensor_twobyte(0x6F12,0x0221);
write_cmos_sensor_twobyte(0x6F12,0x3046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x7CFB);
write_cmos_sensor_twobyte(0x6F12,0xD7F8);
write_cmos_sensor_twobyte(0x6F12,0x3C01);
write_cmos_sensor_twobyte(0x6F12,0x40F0);
write_cmos_sensor_twobyte(0x6F12,0x8000);
write_cmos_sensor_twobyte(0x6F12,0xC7F8);
write_cmos_sensor_twobyte(0x6F12,0x3C01);
write_cmos_sensor_twobyte(0x6F12,0xBDE8);
write_cmos_sensor_twobyte(0x6F12,0xF081);
write_cmos_sensor_twobyte(0x6F12,0x10B5);
write_cmos_sensor_twobyte(0x6F12,0xDD4C);
write_cmos_sensor_twobyte(0x6F12,0xA41E);
write_cmos_sensor_twobyte(0x6F12,0x2388);
write_cmos_sensor_twobyte(0x6F12,0x5B1C);
write_cmos_sensor_twobyte(0x6F12,0x2380);
write_cmos_sensor_twobyte(0x6F12,0xC2F1);
write_cmos_sensor_twobyte(0x6F12,0x0C04);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x6FFB);
write_cmos_sensor_twobyte(0x6F12,0xE040);
write_cmos_sensor_twobyte(0x6F12,0x10BD);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF041);
write_cmos_sensor_twobyte(0x6F12,0x0D46);
write_cmos_sensor_twobyte(0x6F12,0xD649);
write_cmos_sensor_twobyte(0x6F12,0x0446);
write_cmos_sensor_twobyte(0x6F12,0x891C);
write_cmos_sensor_twobyte(0x6F12,0x1646);
write_cmos_sensor_twobyte(0x6F12,0x0888);
write_cmos_sensor_twobyte(0x6F12,0xD44F);
write_cmos_sensor_twobyte(0x6F12,0x401C);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0xB868);
write_cmos_sensor_twobyte(0x6F12,0x80B2);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x63FB);
write_cmos_sensor_twobyte(0x6F12,0x3246);
write_cmos_sensor_twobyte(0x6F12,0x2946);
write_cmos_sensor_twobyte(0x6F12,0x2046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x63FB);
write_cmos_sensor_twobyte(0x6F12,0x0646);
write_cmos_sensor_twobyte(0x6F12,0xB868);
write_cmos_sensor_twobyte(0x6F12,0x80B2);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x63FB);
write_cmos_sensor_twobyte(0x6F12,0xB6F5);
write_cmos_sensor_twobyte(0x6F12,0x804F);
write_cmos_sensor_twobyte(0x6F12,0x3BD2);
write_cmos_sensor_twobyte(0x6F12,0x281D);
write_cmos_sensor_twobyte(0x6F12,0xC030);
write_cmos_sensor_twobyte(0x6F12,0x00BF);
write_cmos_sensor_twobyte(0x6F12,0x0746);
write_cmos_sensor_twobyte(0x6F12,0x281D);
write_cmos_sensor_twobyte(0x6F12,0xEC30);
write_cmos_sensor_twobyte(0x6F12,0x00BF);
write_cmos_sensor_twobyte(0x6F12,0x8046);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0x0121);
write_cmos_sensor_twobyte(0x6F12,0x2846);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x58FB);
write_cmos_sensor_twobyte(0x6F12,0x0128);
write_cmos_sensor_twobyte(0x6F12,0x14D9);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0x0121);
write_cmos_sensor_twobyte(0x6F12,0x2846);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x51FB);
write_cmos_sensor_twobyte(0x6F12,0x0228);
write_cmos_sensor_twobyte(0x6F12,0x09D0);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0x0121);
write_cmos_sensor_twobyte(0x6F12,0x2846);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x4AFB);
write_cmos_sensor_twobyte(0x6F12,0x0428);
write_cmos_sensor_twobyte(0x6F12,0x04D0);
write_cmos_sensor_twobyte(0x6F12,0x0220);
write_cmos_sensor_twobyte(0x6F12,0x2080);
write_cmos_sensor_twobyte(0x6F12,0x0DE0);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0xFBE7);
write_cmos_sensor_twobyte(0x6F12,0x0120);
write_cmos_sensor_twobyte(0x6F12,0xF9E7);
write_cmos_sensor_twobyte(0x6F12,0xB88B);
write_cmos_sensor_twobyte(0x6F12,0x0321);
write_cmos_sensor_twobyte(0x6F12,0x01EB);
write_cmos_sensor_twobyte(0x6F12,0x4000);
write_cmos_sensor_twobyte(0x6F12,0x80B2);
write_cmos_sensor_twobyte(0x6F12,0x2080);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x3210);
write_cmos_sensor_twobyte(0x6F12,0x0129);
write_cmos_sensor_twobyte(0x6F12,0x0FD0);
write_cmos_sensor_twobyte(0x6F12,0x80B2);
write_cmos_sensor_twobyte(0x6F12,0x0821);
write_cmos_sensor_twobyte(0x6F12,0x0828);
write_cmos_sensor_twobyte(0x6F12,0x00D3);
write_cmos_sensor_twobyte(0x6F12,0x0846);
write_cmos_sensor_twobyte(0x6F12,0x2080);
write_cmos_sensor_twobyte(0x6F12,0x2946);
write_cmos_sensor_twobyte(0x6F12,0x2046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x32FB);
write_cmos_sensor_twobyte(0x6F12,0x2946);
write_cmos_sensor_twobyte(0x6F12,0x2046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x33FB);
write_cmos_sensor_twobyte(0x6F12,0x3046);
write_cmos_sensor_twobyte(0x6F12,0x97E7);
write_cmos_sensor_twobyte(0x6F12,0x401C);
write_cmos_sensor_twobyte(0x6F12,0xDDE7);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF05F);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0xA892);
write_cmos_sensor_twobyte(0x6F12,0x0446);
write_cmos_sensor_twobyte(0x6F12,0x0D46);
write_cmos_sensor_twobyte(0x6F12,0x99F8);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0xA0A2);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0xA082);
write_cmos_sensor_twobyte(0x6F12,0x0988);
write_cmos_sensor_twobyte(0x6F12,0x1646);
write_cmos_sensor_twobyte(0x6F12,0x1F46);
write_cmos_sensor_twobyte(0x6F12,0x4FF4);
write_cmos_sensor_twobyte(0x6F12,0x805B);
write_cmos_sensor_twobyte(0x6F12,0x90B3);
write_cmos_sensor_twobyte(0x6F12,0x6888);
write_cmos_sensor_twobyte(0x6F12,0x6143);
write_cmos_sensor_twobyte(0x6F12,0x01EB);
write_cmos_sensor_twobyte(0x6F12,0x9003);
write_cmos_sensor_twobyte(0x6F12,0xDAF8);
write_cmos_sensor_twobyte(0x6F12,0x7400);
write_cmos_sensor_twobyte(0x6F12,0xD446);
write_cmos_sensor_twobyte(0x6F12,0x9842);
write_cmos_sensor_twobyte(0x6F12,0x0BD9);
write_cmos_sensor_twobyte(0x6F12,0xB0FB);
write_cmos_sensor_twobyte(0x6F12,0xF4F0);
write_cmos_sensor_twobyte(0x6F12,0x80B2);
write_cmos_sensor_twobyte(0x6F12,0x2880);
write_cmos_sensor_twobyte(0x6F12,0xDCF8);
write_cmos_sensor_twobyte(0x6F12,0x7410);
write_cmos_sensor_twobyte(0x6F12,0x00FB);
write_cmos_sensor_twobyte(0x6F12,0x1410);
write_cmos_sensor_twobyte(0x6F12,0x8000);
write_cmos_sensor_twobyte(0x6F12,0x6880);
write_cmos_sensor_twobyte(0x6F12,0xDCF8);
write_cmos_sensor_twobyte(0x6F12,0x7430);
write_cmos_sensor_twobyte(0x6F12,0xBCF8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0x7A88);
write_cmos_sensor_twobyte(0x6F12,0x181A);
write_cmos_sensor_twobyte(0x6F12,0xB0FB);
write_cmos_sensor_twobyte(0x6F12,0xF4F1);
write_cmos_sensor_twobyte(0x6F12,0x01FB);
write_cmos_sensor_twobyte(0x6F12,0x1400);
write_cmos_sensor_twobyte(0x6F12,0xAAB1);
write_cmos_sensor_twobyte(0x6F12,0x8242);
write_cmos_sensor_twobyte(0x6F12,0x00D9);
write_cmos_sensor_twobyte(0x6F12,0x491E);
write_cmos_sensor_twobyte(0x6F12,0x1046);
write_cmos_sensor_twobyte(0x6F12,0x3980);
write_cmos_sensor_twobyte(0x6F12,0xBAF8);
write_cmos_sensor_twobyte(0x6F12,0x7820);
write_cmos_sensor_twobyte(0x6F12,0x01FB);
write_cmos_sensor_twobyte(0x6F12,0x0421);
write_cmos_sensor_twobyte(0x6F12,0x0144);
write_cmos_sensor_twobyte(0x6F12,0x3160);
write_cmos_sensor_twobyte(0x6F12,0x98F8);
write_cmos_sensor_twobyte(0x6F12,0x2800);
write_cmos_sensor_twobyte(0x6F12,0x28B3);
write_cmos_sensor_twobyte(0x6F12,0x0C22);
write_cmos_sensor_twobyte(0x6F12,0x1846);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xF2FA);
write_cmos_sensor_twobyte(0x6F12,0x7060);
write_cmos_sensor_twobyte(0x6F12,0xBDE8);
write_cmos_sensor_twobyte(0x6F12,0xF09F);
write_cmos_sensor_twobyte(0x6F12,0x20E0);
write_cmos_sensor_twobyte(0x6F12,0xBCF8);
write_cmos_sensor_twobyte(0x6F12,0x7020);
write_cmos_sensor_twobyte(0x6F12,0x8242);
write_cmos_sensor_twobyte(0x6F12,0x01D9);
write_cmos_sensor_twobyte(0x6F12,0x491E);
write_cmos_sensor_twobyte(0x6F12,0x601E);
write_cmos_sensor_twobyte(0x6F12,0x9CF8);
write_cmos_sensor_twobyte(0x6F12,0x6E20);
write_cmos_sensor_twobyte(0x6F12,0x8AB1);
write_cmos_sensor_twobyte(0x6F12,0xBCF8);
write_cmos_sensor_twobyte(0x6F12,0x6820);
write_cmos_sensor_twobyte(0x6F12,0x8242);
write_cmos_sensor_twobyte(0x6F12,0x0BD2);
write_cmos_sensor_twobyte(0x6F12,0x9CF8);
write_cmos_sensor_twobyte(0x6F12,0x6450);
write_cmos_sensor_twobyte(0x6F12,0x3DB1);
write_cmos_sensor_twobyte(0x6F12,0xBCF8);
write_cmos_sensor_twobyte(0x6F12,0x6A50);
write_cmos_sensor_twobyte(0x6F12,0x8542);
write_cmos_sensor_twobyte(0x6F12,0x03D8);
write_cmos_sensor_twobyte(0x6F12,0xBCF8);
write_cmos_sensor_twobyte(0x6F12,0x6C20);
write_cmos_sensor_twobyte(0x6F12,0x8242);
write_cmos_sensor_twobyte(0x6F12,0x00D2);
write_cmos_sensor_twobyte(0x6F12,0x1046);
write_cmos_sensor_twobyte(0x6F12,0x7880);
write_cmos_sensor_twobyte(0x6F12,0xD2E7);
write_cmos_sensor_twobyte(0x6F12,0xBCF8);
write_cmos_sensor_twobyte(0x6F12,0x6220);
write_cmos_sensor_twobyte(0x6F12,0xF7E7);
write_cmos_sensor_twobyte(0x6F12,0xC6F8);
write_cmos_sensor_twobyte(0x6F12,0x04B0);
write_cmos_sensor_twobyte(0x6F12,0xDBE7);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x2400);
write_cmos_sensor_twobyte(0x6F12,0x401C);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xCDFA);
write_cmos_sensor_twobyte(0x6F12,0x0146);
write_cmos_sensor_twobyte(0x6F12,0x6888);
write_cmos_sensor_twobyte(0x6F12,0x99F8);
write_cmos_sensor_twobyte(0x6F12,0x0130);
write_cmos_sensor_twobyte(0x6F12,0x99F8);
write_cmos_sensor_twobyte(0x6F12,0x0220);
write_cmos_sensor_twobyte(0x6F12,0xB0FB);
write_cmos_sensor_twobyte(0x6F12,0xF3F0);
write_cmos_sensor_twobyte(0x6F12,0x32B1);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x2420);
write_cmos_sensor_twobyte(0x6F12,0x2B88);
write_cmos_sensor_twobyte(0x6F12,0x521C);
write_cmos_sensor_twobyte(0x6F12,0x9342);
write_cmos_sensor_twobyte(0x6F12,0x00D2);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x3980);
write_cmos_sensor_twobyte(0x6F12,0x7880);
write_cmos_sensor_twobyte(0x6F12,0xBAF8);
write_cmos_sensor_twobyte(0x6F12,0x7820);
write_cmos_sensor_twobyte(0x6F12,0x01FB);
write_cmos_sensor_twobyte(0x6F12,0x0421);
write_cmos_sensor_twobyte(0x6F12,0x0844);
write_cmos_sensor_twobyte(0x6F12,0xC6E9);
write_cmos_sensor_twobyte(0x6F12,0x000B);
write_cmos_sensor_twobyte(0x6F12,0xBCE7);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF04F);
write_cmos_sensor_twobyte(0x6F12,0x0446);
write_cmos_sensor_twobyte(0x6F12,0x90F8);
write_cmos_sensor_twobyte(0x6F12,0x2500);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0xA4B1);
write_cmos_sensor_twobyte(0x6F12,0x8BB0);
write_cmos_sensor_twobyte(0x6F12,0x0D46);
write_cmos_sensor_twobyte(0x6F12,0x10B1);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x6F12,0x00B1);
write_cmos_sensor_twobyte(0x6F12,0x0120);
write_cmos_sensor_twobyte(0x6F12,0x2870);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7400);
write_cmos_sensor_twobyte(0x6F12,0x0228);
write_cmos_sensor_twobyte(0x6F12,0x14D0);
write_cmos_sensor_twobyte(0x6F12,0x0023);
write_cmos_sensor_twobyte(0x6F12,0xAB74);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7600);
write_cmos_sensor_twobyte(0x6F12,0x0228);
write_cmos_sensor_twobyte(0x6F12,0x11D0);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0xE874);
write_cmos_sensor_twobyte(0x6F12,0x0026);
write_cmos_sensor_twobyte(0x6F12,0x13B1);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0x68B1);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x0008);
write_cmos_sensor_twobyte(0x6F12,0x5B4F);
write_cmos_sensor_twobyte(0x6F12,0x002B);
write_cmos_sensor_twobyte(0x6F12,0x0BD0);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x0209);
write_cmos_sensor_twobyte(0x6F12,0x0AE0);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x0103);
write_cmos_sensor_twobyte(0x6F12,0xE8E7);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0xEBE7);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x0208);
write_cmos_sensor_twobyte(0x6F12,0xF0E7);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x0009);
write_cmos_sensor_twobyte(0x6F12,0x34D0);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6000);
write_cmos_sensor_twobyte(0x6F12,0xB7F8);
write_cmos_sensor_twobyte(0x6F12,0x1E13);
write_cmos_sensor_twobyte(0x6F12,0x6FF0);
write_cmos_sensor_twobyte(0x6F12,0x010A);
write_cmos_sensor_twobyte(0x6F12,0x8842);
write_cmos_sensor_twobyte(0x6F12,0x09D2);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6420);
write_cmos_sensor_twobyte(0x6F12,0xB7F8);
write_cmos_sensor_twobyte(0x6F12,0x22C3);
write_cmos_sensor_twobyte(0x6F12,0x6245);
write_cmos_sensor_twobyte(0x6F12,0x03D9);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0x00BB);
write_cmos_sensor_twobyte(0x6F12,0x0AE0);
write_cmos_sensor_twobyte(0x6F12,0x8842);
write_cmos_sensor_twobyte(0x6F12,0x0AD9);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6420);
write_cmos_sensor_twobyte(0x6F12,0xB7F8);
write_cmos_sensor_twobyte(0x6F12,0x22C3);
write_cmos_sensor_twobyte(0x6F12,0x6245);
write_cmos_sensor_twobyte(0x6F12,0x04D9);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0xB0B9);
write_cmos_sensor_twobyte(0x6F12,0x5646);
write_cmos_sensor_twobyte(0x6F12,0x15E0);
write_cmos_sensor_twobyte(0x6F12,0x8842);
write_cmos_sensor_twobyte(0x6F12,0x05D9);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6420);
write_cmos_sensor_twobyte(0x6F12,0xB7F8);
write_cmos_sensor_twobyte(0x6F12,0x22C3);
write_cmos_sensor_twobyte(0x6F12,0x6245);
write_cmos_sensor_twobyte(0x6F12,0x0DD3);
write_cmos_sensor_twobyte(0x6F12,0x8842);
write_cmos_sensor_twobyte(0x6F12,0x0BD2);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6400);
write_cmos_sensor_twobyte(0x6F12,0xB7F8);
write_cmos_sensor_twobyte(0x6F12,0x2213);
write_cmos_sensor_twobyte(0x6F12,0x8842);
write_cmos_sensor_twobyte(0x6F12,0x05D2);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0x08B1);
write_cmos_sensor_twobyte(0x6F12,0x0226);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x0026);
write_cmos_sensor_twobyte(0x6F12,0x09A9);
write_cmos_sensor_twobyte(0x6F12,0x05A8);
write_cmos_sensor_twobyte(0x6F12,0x07F2);
write_cmos_sensor_twobyte(0x6F12,0x1637);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0xF888);
write_cmos_sensor_twobyte(0x6F12,0x3989);
write_cmos_sensor_twobyte(0x6F12,0x00EB);
write_cmos_sensor_twobyte(0x6F12,0x0802);
write_cmos_sensor_twobyte(0x6F12,0x34F8);
write_cmos_sensor_twobyte(0x6F12,0x600F);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x41FA);
write_cmos_sensor_twobyte(0x6F12,0x08A9);
write_cmos_sensor_twobyte(0x6F12,0x04A8);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0xF888);
write_cmos_sensor_twobyte(0x6F12,0xAB7C);
write_cmos_sensor_twobyte(0x6F12,0x00EB);
write_cmos_sensor_twobyte(0x6F12,0x0802);
write_cmos_sensor_twobyte(0x6F12,0xA088);
write_cmos_sensor_twobyte(0x6F12,0x00EB);
write_cmos_sensor_twobyte(0x6F12,0x0901);
write_cmos_sensor_twobyte(0x6F12,0xB889);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x33FA);
write_cmos_sensor_twobyte(0x6F12,0x07A9);
write_cmos_sensor_twobyte(0x6F12,0x03A8);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0xEB7C);
write_cmos_sensor_twobyte(0x6F12,0x3A88);
write_cmos_sensor_twobyte(0x6F12,0x7989);
write_cmos_sensor_twobyte(0x6F12,0x6088);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x29FA);
write_cmos_sensor_twobyte(0x6F12,0x06A9);
write_cmos_sensor_twobyte(0x6F12,0x02A8);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0xEB7C);
write_cmos_sensor_twobyte(0x6F12,0x3A88);
write_cmos_sensor_twobyte(0x6F12,0xE188);
write_cmos_sensor_twobyte(0x6F12,0xF889);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x1FFA);
write_cmos_sensor_twobyte(0x6F12,0x207E);
write_cmos_sensor_twobyte(0x6F12,0xA4F1);
write_cmos_sensor_twobyte(0x6F12,0x6004);
write_cmos_sensor_twobyte(0x6F12,0x08B1);
write_cmos_sensor_twobyte(0x6F12,0x0498);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x0598);
write_cmos_sensor_twobyte(0x6F12,0x6880);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7900);
write_cmos_sensor_twobyte(0x6F12,0x08B1);
write_cmos_sensor_twobyte(0x6F12,0x0298);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x0398);
write_cmos_sensor_twobyte(0x6F12,0xA880);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6C00);
write_cmos_sensor_twobyte(0x6F12,0x0599);
write_cmos_sensor_twobyte(0x6F12,0x401A);
write_cmos_sensor_twobyte(0x6F12,0x0499);
write_cmos_sensor_twobyte(0x6F12,0x401A);
write_cmos_sensor_twobyte(0x6F12,0x3044);
write_cmos_sensor_twobyte(0x6F12,0x0028);
write_cmos_sensor_twobyte(0x6F12,0x00DC);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0xE881);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6E00);
write_cmos_sensor_twobyte(0x6F12,0x0399);
write_cmos_sensor_twobyte(0x6F12,0x401A);
write_cmos_sensor_twobyte(0x6F12,0x0299);
write_cmos_sensor_twobyte(0x6F12,0x401A);
write_cmos_sensor_twobyte(0x6F12,0x0028);
write_cmos_sensor_twobyte(0x6F12,0x00DC);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x2882);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0x08B1);
write_cmos_sensor_twobyte(0x6F12,0x0898);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x0998);
write_cmos_sensor_twobyte(0x6F12,0xC117);
write_cmos_sensor_twobyte(0x6F12,0x00EB);
write_cmos_sensor_twobyte(0x6F12,0x9161);
write_cmos_sensor_twobyte(0x6F12,0x21F0);
write_cmos_sensor_twobyte(0x6F12,0x3F01);
write_cmos_sensor_twobyte(0x6F12,0x411A);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7900);
write_cmos_sensor_twobyte(0x6F12,0x78B1);
write_cmos_sensor_twobyte(0x6F12,0x0698);
write_cmos_sensor_twobyte(0x6F12,0x0EE0);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xBDC8);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x55E0);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0xAB00);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xC5B0);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0x14B0);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0x2400);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xBEA0);
write_cmos_sensor_twobyte(0x6F12,0x0798);
write_cmos_sensor_twobyte(0x6F12,0xC217);
write_cmos_sensor_twobyte(0x6F12,0x00EB);
write_cmos_sensor_twobyte(0x6F12,0x9262);
write_cmos_sensor_twobyte(0x6F12,0x22F0);
write_cmos_sensor_twobyte(0x6F12,0x3F02);
write_cmos_sensor_twobyte(0x6F12,0x801A);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x1920);
write_cmos_sensor_twobyte(0x6F12,0x91FB);
write_cmos_sensor_twobyte(0x6F12,0xF2F2);
write_cmos_sensor_twobyte(0x6F12,0xEA80);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x1920);
write_cmos_sensor_twobyte(0x6F12,0x91FB);
write_cmos_sensor_twobyte(0x6F12,0xF2F4);
write_cmos_sensor_twobyte(0x6F12,0x02FB);
write_cmos_sensor_twobyte(0x6F12,0x1411);
write_cmos_sensor_twobyte(0x6F12,0x6981);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x1A10);
write_cmos_sensor_twobyte(0x6F12,0x90FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F1);
write_cmos_sensor_twobyte(0x6F12,0x2981);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x1A10);
write_cmos_sensor_twobyte(0x6F12,0x90FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F2);
write_cmos_sensor_twobyte(0x6F12,0x01FB);
write_cmos_sensor_twobyte(0x6F12,0x1200);
write_cmos_sensor_twobyte(0x6F12,0xA881);
write_cmos_sensor_twobyte(0x6F12,0x0BB0);
write_cmos_sensor_twobyte(0x6F12,0xBDE8);
write_cmos_sensor_twobyte(0x6F12,0xF08F);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF34F);
write_cmos_sensor_twobyte(0x6F12,0xB94E);
write_cmos_sensor_twobyte(0x6F12,0xBA4B);
write_cmos_sensor_twobyte(0x6F12,0x81B0);
write_cmos_sensor_twobyte(0x6F12,0x0021);
write_cmos_sensor_twobyte(0x6F12,0x8A5B);
write_cmos_sensor_twobyte(0x6F12,0x03EB);
write_cmos_sensor_twobyte(0x6F12,0x4104);
write_cmos_sensor_twobyte(0x6F12,0xD5B2);
write_cmos_sensor_twobyte(0x6F12,0x120A);
write_cmos_sensor_twobyte(0x6F12,0xA4F8);
write_cmos_sensor_twobyte(0x6F12,0x8451);
write_cmos_sensor_twobyte(0x6F12,0x891C);
write_cmos_sensor_twobyte(0x6F12,0xA4F8);
write_cmos_sensor_twobyte(0x6F12,0x8621);
write_cmos_sensor_twobyte(0x6F12,0x0829);
write_cmos_sensor_twobyte(0x6F12,0xF3D3);
write_cmos_sensor_twobyte(0x6F12,0x4FF4);
write_cmos_sensor_twobyte(0x6F12,0x8011);
write_cmos_sensor_twobyte(0x6F12,0xB1FB);
write_cmos_sensor_twobyte(0x6F12,0xF0FE);
write_cmos_sensor_twobyte(0x6F12,0xB149);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAEF5);
write_cmos_sensor_twobyte(0x6F12,0x807B);
write_cmos_sensor_twobyte(0x6F12,0xB1F8);
write_cmos_sensor_twobyte(0x6F12,0x8412);
write_cmos_sensor_twobyte(0x6F12,0xA0F5);
write_cmos_sensor_twobyte(0x6F12,0x8078);
write_cmos_sensor_twobyte(0x6F12,0x0091);
write_cmos_sensor_twobyte(0x6F12,0xAC4D);
write_cmos_sensor_twobyte(0x6F12,0x0024);
write_cmos_sensor_twobyte(0x6F12,0x05EB);
write_cmos_sensor_twobyte(0x6F12,0x8203);
write_cmos_sensor_twobyte(0x6F12,0x05EB);
write_cmos_sensor_twobyte(0x6F12,0x420A);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0xA441);
write_cmos_sensor_twobyte(0x6F12,0xAAF8);
write_cmos_sensor_twobyte(0x6F12,0x9441);
write_cmos_sensor_twobyte(0x6F12,0xA84C);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x0F01);
write_cmos_sensor_twobyte(0x6F12,0x04EB);
write_cmos_sensor_twobyte(0x6F12,0x4205);
write_cmos_sensor_twobyte(0x6F12,0x05F5);
write_cmos_sensor_twobyte(0x6F12,0x2675);
write_cmos_sensor_twobyte(0x6F12,0x2F8C);
write_cmos_sensor_twobyte(0x6F12,0x2C88);
write_cmos_sensor_twobyte(0x6F12,0xA7EB);
write_cmos_sensor_twobyte(0x6F12,0x0406);
write_cmos_sensor_twobyte(0x6F12,0x06FB);
write_cmos_sensor_twobyte(0x6F12,0x08F6);
write_cmos_sensor_twobyte(0x6F12,0x96FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F6);
write_cmos_sensor_twobyte(0x6F12,0x04EB);
write_cmos_sensor_twobyte(0x6F12,0x2624);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0x2441);
write_cmos_sensor_twobyte(0x6F12,0x2D8A);
write_cmos_sensor_twobyte(0x6F12,0xA7EB);
write_cmos_sensor_twobyte(0x6F12,0x050C);
write_cmos_sensor_twobyte(0x6F12,0x0BFB);
write_cmos_sensor_twobyte(0x6F12,0x0CF6);
write_cmos_sensor_twobyte(0x6F12,0x96FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F6);
write_cmos_sensor_twobyte(0x6F12,0xA7EB);
write_cmos_sensor_twobyte(0x6F12,0x2629);
write_cmos_sensor_twobyte(0x6F12,0x0CFB);
write_cmos_sensor_twobyte(0x6F12,0x08FC);
write_cmos_sensor_twobyte(0x6F12,0x9CFB);
write_cmos_sensor_twobyte(0x6F12,0xF1F6);
write_cmos_sensor_twobyte(0x6F12,0x05EB);
write_cmos_sensor_twobyte(0x6F12,0x2625);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x1006);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0x6491);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0x4451);
write_cmos_sensor_twobyte(0x6F12,0xB6EB);
write_cmos_sensor_twobyte(0x6F12,0x102F);
write_cmos_sensor_twobyte(0x6F12,0x03D8);
write_cmos_sensor_twobyte(0x6F12,0x3C46);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0xA471);
write_cmos_sensor_twobyte(0x6F12,0x19E0);
write_cmos_sensor_twobyte(0x6F12,0x042A);
write_cmos_sensor_twobyte(0x6F12,0x0BD2);
write_cmos_sensor_twobyte(0x6F12,0x0299);
write_cmos_sensor_twobyte(0x6F12,0x4FF4);
write_cmos_sensor_twobyte(0x6F12,0x8016);
write_cmos_sensor_twobyte(0x6F12,0xB6FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F6);
write_cmos_sensor_twobyte(0x6F12,0xA6EB);
write_cmos_sensor_twobyte(0x6F12,0x0E06);
write_cmos_sensor_twobyte(0x6F12,0x651B);
write_cmos_sensor_twobyte(0x6F12,0x6E43);
write_cmos_sensor_twobyte(0x6F12,0x96FB);
write_cmos_sensor_twobyte(0x6F12,0xFBF5);
write_cmos_sensor_twobyte(0x6F12,0x08E0);
write_cmos_sensor_twobyte(0x6F12,0x0299);
write_cmos_sensor_twobyte(0x6F12,0xA9EB);
write_cmos_sensor_twobyte(0x6F12,0x0405);
write_cmos_sensor_twobyte(0x6F12,0x0E1A);
write_cmos_sensor_twobyte(0x6F12,0x7543);
write_cmos_sensor_twobyte(0x6F12,0xC0F5);
write_cmos_sensor_twobyte(0x6F12,0x8056);
write_cmos_sensor_twobyte(0x6F12,0x95FB);
write_cmos_sensor_twobyte(0x6F12,0xF6F5);
write_cmos_sensor_twobyte(0x6F12,0x2C44);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0xA441);
write_cmos_sensor_twobyte(0x6F12,0x0099);
write_cmos_sensor_twobyte(0x6F12,0x521C);
write_cmos_sensor_twobyte(0x6F12,0xA4EB);
write_cmos_sensor_twobyte(0x6F12,0x0124);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0xA441);
write_cmos_sensor_twobyte(0x6F12,0xBAF8);
write_cmos_sensor_twobyte(0x6F12,0x8431);
write_cmos_sensor_twobyte(0x6F12,0x6343);
write_cmos_sensor_twobyte(0x6F12,0xC4EB);
write_cmos_sensor_twobyte(0x6F12,0xE311);
write_cmos_sensor_twobyte(0x6F12,0xAAF8);
write_cmos_sensor_twobyte(0x6F12,0x9411);
write_cmos_sensor_twobyte(0x6F12,0x082A);
write_cmos_sensor_twobyte(0x6F12,0x9DD3);
write_cmos_sensor_twobyte(0x6F12,0xBDE8);
write_cmos_sensor_twobyte(0x6F12,0xFE8F);
write_cmos_sensor_twobyte(0x6F12,0x70B5);
write_cmos_sensor_twobyte(0x6F12,0x0446);
write_cmos_sensor_twobyte(0x6F12,0xB0FA);
write_cmos_sensor_twobyte(0x6F12,0x80F0);
write_cmos_sensor_twobyte(0x6F12,0x0D46);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0x0821);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x30F9);
write_cmos_sensor_twobyte(0x6F12,0x8440);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0xC540);
write_cmos_sensor_twobyte(0x6F12,0xB4FB);
write_cmos_sensor_twobyte(0x6F12,0xF5F0);
write_cmos_sensor_twobyte(0x6F12,0x05FB);
write_cmos_sensor_twobyte(0x6F12,0x1041);
write_cmos_sensor_twobyte(0x6F12,0xB5EB);
write_cmos_sensor_twobyte(0x6F12,0x410F);
write_cmos_sensor_twobyte(0x6F12,0x00D2);
write_cmos_sensor_twobyte(0x6F12,0x401C);
write_cmos_sensor_twobyte(0x6F12,0x70BD);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xFE4F);
write_cmos_sensor_twobyte(0x6F12,0x0746);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0290);
write_cmos_sensor_twobyte(0x6F12,0x0C46);
write_cmos_sensor_twobyte(0x6F12,0x081D);
write_cmos_sensor_twobyte(0x6F12,0xC030);
write_cmos_sensor_twobyte(0x6F12,0x00BF);
write_cmos_sensor_twobyte(0x6F12,0x8246);
write_cmos_sensor_twobyte(0x6F12,0x201D);
write_cmos_sensor_twobyte(0x6F12,0xEC30);
write_cmos_sensor_twobyte(0x6F12,0x00BF);
write_cmos_sensor_twobyte(0x6F12,0xD0E9);
write_cmos_sensor_twobyte(0x6F12,0x0013);
write_cmos_sensor_twobyte(0x6F12,0x5943);
write_cmos_sensor_twobyte(0x6F12,0xD0E9);
write_cmos_sensor_twobyte(0x6F12,0x0236);
write_cmos_sensor_twobyte(0x6F12,0x090A);
write_cmos_sensor_twobyte(0x6F12,0x5943);
write_cmos_sensor_twobyte(0x6F12,0x4FEA);
write_cmos_sensor_twobyte(0x6F12,0x1129);
write_cmos_sensor_twobyte(0x6F12,0xD0E9);
write_cmos_sensor_twobyte(0x6F12,0x0413);
write_cmos_sensor_twobyte(0x6F12,0x1830);
write_cmos_sensor_twobyte(0x6F12,0x5943);
write_cmos_sensor_twobyte(0x6F12,0x21C8);
write_cmos_sensor_twobyte(0x6F12,0x090A);
write_cmos_sensor_twobyte(0x6F12,0x4143);
write_cmos_sensor_twobyte(0x6F12,0x4FEA);
write_cmos_sensor_twobyte(0x6F12,0x1128);
write_cmos_sensor_twobyte(0x6F12,0x04F5);
write_cmos_sensor_twobyte(0x6F12,0xF66B);
write_cmos_sensor_twobyte(0x6F12,0x4946);
write_cmos_sensor_twobyte(0x6F12,0x4046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xF4F8);
write_cmos_sensor_twobyte(0x6F12,0xB0FA);
write_cmos_sensor_twobyte(0x6F12,0x80F0);
write_cmos_sensor_twobyte(0x6F12,0xB5FA);
write_cmos_sensor_twobyte(0x6F12,0x85F1);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xC1F1);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x0844);
write_cmos_sensor_twobyte(0x6F12,0x0021);
write_cmos_sensor_twobyte(0x6F12,0x2038);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xE7F8);
write_cmos_sensor_twobyte(0x6F12,0x0828);
write_cmos_sensor_twobyte(0x6F12,0x26FA);
write_cmos_sensor_twobyte(0x6F12,0x00F6);
write_cmos_sensor_twobyte(0x6F12,0x06FB);
write_cmos_sensor_twobyte(0x6F12,0x09F6);
write_cmos_sensor_twobyte(0x6F12,0x07DC);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x0801);
write_cmos_sensor_twobyte(0x6F12,0xCE40);
write_cmos_sensor_twobyte(0x6F12,0xC540);
write_cmos_sensor_twobyte(0x6F12,0x05FB);
write_cmos_sensor_twobyte(0x6F12,0x08F5);
write_cmos_sensor_twobyte(0x6F12,0xCD40);
write_cmos_sensor_twobyte(0x6F12,0x06E0);
write_cmos_sensor_twobyte(0x6F12,0xA0F1);
write_cmos_sensor_twobyte(0x6F12,0x0801);
write_cmos_sensor_twobyte(0x6F12,0x8E40);
write_cmos_sensor_twobyte(0x6F12,0xC540);
write_cmos_sensor_twobyte(0x6F12,0x05FB);
write_cmos_sensor_twobyte(0x6F12,0x08F5);
write_cmos_sensor_twobyte(0x6F12,0x8D40);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0065);
write_cmos_sensor_twobyte(0x6F12,0x3869);
write_cmos_sensor_twobyte(0x6F12,0x5A46);
write_cmos_sensor_twobyte(0x6F12,0x0168);
write_cmos_sensor_twobyte(0x6F12,0x8B68);
write_cmos_sensor_twobyte(0x6F12,0x6946);
write_cmos_sensor_twobyte(0x6F12,0x9847);
write_cmos_sensor_twobyte(0x6F12,0x04F2);
write_cmos_sensor_twobyte(0x6F12,0xA474);
write_cmos_sensor_twobyte(0x6F12,0xDAF8);
write_cmos_sensor_twobyte(0x6F12,0x1810);
write_cmos_sensor_twobyte(0x6F12,0xA069);
write_cmos_sensor_twobyte(0x6F12,0xFFF7);
write_cmos_sensor_twobyte(0x6F12,0x91FF);
write_cmos_sensor_twobyte(0x6F12,0x2060);
write_cmos_sensor_twobyte(0x6F12,0xE069);
write_cmos_sensor_twobyte(0x6F12,0xDAF8);
write_cmos_sensor_twobyte(0x6F12,0x1810);
write_cmos_sensor_twobyte(0x6F12,0xFFF7);
write_cmos_sensor_twobyte(0x6F12,0x8BFF);
write_cmos_sensor_twobyte(0x6F12,0x6060);
write_cmos_sensor_twobyte(0x6F12,0xD7F8);
write_cmos_sensor_twobyte(0x6F12,0x3C01);
write_cmos_sensor_twobyte(0x6F12,0x40F4);
write_cmos_sensor_twobyte(0x6F12,0x0050);
write_cmos_sensor_twobyte(0x6F12,0xC7F8);
write_cmos_sensor_twobyte(0x6F12,0x3C01);
write_cmos_sensor_twobyte(0x6F12,0x0298);
write_cmos_sensor_twobyte(0x6F12,0x7FE7);
write_cmos_sensor_twobyte(0x6F12,0x0180);
write_cmos_sensor_twobyte(0x6F12,0x7047);
write_cmos_sensor_twobyte(0x6F12,0x0160);
write_cmos_sensor_twobyte(0x6F12,0x7047);
write_cmos_sensor_twobyte(0x6F12,0x70B5);
write_cmos_sensor_twobyte(0x6F12,0x3A4C);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0xD351);
write_cmos_sensor_twobyte(0x6F12,0x2068);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xB7F8);
write_cmos_sensor_twobyte(0x6F12,0x384D);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x2751);
write_cmos_sensor_twobyte(0x6F12,0xA860);
write_cmos_sensor_twobyte(0x6F12,0x3648);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xAFF8);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x2B41);
write_cmos_sensor_twobyte(0x6F12,0x3448);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xA9F8);
write_cmos_sensor_twobyte(0x6F12,0x0122);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x0311);
write_cmos_sensor_twobyte(0x6F12,0xA068);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xA3F8);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x4721);
write_cmos_sensor_twobyte(0x6F12,0xE860);
write_cmos_sensor_twobyte(0x6F12,0x2F48);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x9CF8);
write_cmos_sensor_twobyte(0x6F12,0x0122);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x9B61);
write_cmos_sensor_twobyte(0x6F12,0x2069);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x96F8);
write_cmos_sensor_twobyte(0x6F12,0x2860);
write_cmos_sensor_twobyte(0x6F12,0x0122);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x3B61);
write_cmos_sensor_twobyte(0x6F12,0xA069);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x8FF8);
write_cmos_sensor_twobyte(0x6F12,0x2849);
write_cmos_sensor_twobyte(0x6F12,0x6860);
write_cmos_sensor_twobyte(0x6F12,0x46F2);
write_cmos_sensor_twobyte(0x6F12,0x6800);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0x2548);
write_cmos_sensor_twobyte(0x6F12,0x44F2);
write_cmos_sensor_twobyte(0x6F12,0xF631);
write_cmos_sensor_twobyte(0x6F12,0x801C);
write_cmos_sensor_twobyte(0x6F12,0x0180);
write_cmos_sensor_twobyte(0x6F12,0x234A);
write_cmos_sensor_twobyte(0x6F12,0x4FF6);
write_cmos_sensor_twobyte(0x6F12,0x7170);
write_cmos_sensor_twobyte(0x6F12,0x121D);
write_cmos_sensor_twobyte(0x6F12,0x1080);
write_cmos_sensor_twobyte(0x6F12,0x204B);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0xEB12);
write_cmos_sensor_twobyte(0x6F12,0x9B1D);
write_cmos_sensor_twobyte(0x6F12,0x1A80);
write_cmos_sensor_twobyte(0x6F12,0x9B1C);
write_cmos_sensor_twobyte(0x6F12,0x42F2);
write_cmos_sensor_twobyte(0x6F12,0x2822);
write_cmos_sensor_twobyte(0x6F12,0x1A80);
write_cmos_sensor_twobyte(0x6F12,0x9B1C);
write_cmos_sensor_twobyte(0x6F12,0x4BF2);
write_cmos_sensor_twobyte(0x6F12,0xF902);
write_cmos_sensor_twobyte(0x6F12,0x1A80);
write_cmos_sensor_twobyte(0x6F12,0x9B1C);
write_cmos_sensor_twobyte(0x6F12,0x42F6);
write_cmos_sensor_twobyte(0x6F12,0x3022);
write_cmos_sensor_twobyte(0x6F12,0x1A80);
write_cmos_sensor_twobyte(0x6F12,0x9B1C);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0xEB32);
write_cmos_sensor_twobyte(0x6F12,0x1A80);
write_cmos_sensor_twobyte(0x6F12,0x9B1C);
write_cmos_sensor_twobyte(0x6F12,0x4FF4);
write_cmos_sensor_twobyte(0x6F12,0x8672);
write_cmos_sensor_twobyte(0x6F12,0x1A80);
write_cmos_sensor_twobyte(0x6F12,0x9A1C);
write_cmos_sensor_twobyte(0x6F12,0x1180);
write_cmos_sensor_twobyte(0x6F12,0x911C);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0x891C);
write_cmos_sensor_twobyte(0x6F12,0x48F6);
write_cmos_sensor_twobyte(0x6F12,0x4540);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0x891C);
write_cmos_sensor_twobyte(0x6F12,0xD320);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0x0E49);
write_cmos_sensor_twobyte(0x6F12,0x41F6);
write_cmos_sensor_twobyte(0x6F12,0x0A30);
write_cmos_sensor_twobyte(0x6F12,0x4C31);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0x881C);
write_cmos_sensor_twobyte(0x6F12,0x4CF2);
write_cmos_sensor_twobyte(0x6F12,0xEB31);
write_cmos_sensor_twobyte(0x6F12,0x0180);
write_cmos_sensor_twobyte(0x6F12,0x811C);
write_cmos_sensor_twobyte(0x6F12,0x48F2);
write_cmos_sensor_twobyte(0x6F12,0x3330);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0x70BD);
write_cmos_sensor_twobyte(0x6F12,0x4000);
write_cmos_sensor_twobyte(0x6F12,0x9526);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xCC60);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0x2400);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x55B0);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x55E0);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x729D);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0xC4A5);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0xB4B5);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0xDB14);
write_cmos_sensor_twobyte(0x6F12,0x44F2);
write_cmos_sensor_twobyte(0x6F12,0x814C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x010C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x44F2);
write_cmos_sensor_twobyte(0x6F12,0x8B4C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x010C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0xE12C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x43F2);
write_cmos_sensor_twobyte(0x6F12,0x255C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x010C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0xF12C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x44F2);
write_cmos_sensor_twobyte(0x6F12,0x134C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x010C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x42F2);
write_cmos_sensor_twobyte(0x6F12,0xD96C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x010C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x42F6);
write_cmos_sensor_twobyte(0x6F12,0xB90C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x010C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x41F6);
write_cmos_sensor_twobyte(0x6F12,0x497C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x41F6);
write_cmos_sensor_twobyte(0x6F12,0x8F7C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x4CF2);
write_cmos_sensor_twobyte(0x6F12,0x554C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x41F6);
write_cmos_sensor_twobyte(0x6F12,0x877C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0x013C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x3525);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0477);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0587);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x4495);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x3188);
write_cmos_sensor_twobyte(0x6F12,0x026D);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x003F);
//
// Warning: T&P Parameters Info Unavailable
//
// End T&P part

////////////////////////////////////////////////////////////
//////Analog Setting Start 20141216
////////////////////////////////////////////////////////////

//// Add mixer setting @ 20150111
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x2947);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x2948);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x2949);
write_cmos_sensor(0x6F12,0x00);

//// Add dadlc setting @ 20150118 , pbc
write_cmos_sensor_twobyte(0x602A,0x2664);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

//// Add WDR exposure setting @ 20150118 , pbc
// pjw ??? WRITE 4000F4FC 4D17

//// SHBN setting
write_cmos_sensor_twobyte(0x602A,0x0F50);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0F52);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);

write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0xF442);	// DBUS option (No bypass)

//////////////////////////////
//////////////////////////////

//// Main Setting
write_cmos_sensor_twobyte(0x602A,0x1CCA);
write_cmos_sensor_twobyte(0x6F12,0xFDE9);	// Bias Sampling EN & FDB Off &dabx on, [10] FX_EN (F40A)
write_cmos_sensor_twobyte(0x602A,0x1CC8);
write_cmos_sensor_twobyte(0x6F12,0x3EB8);	// F406 address

//// ATOP Setting (Option)
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0xF440);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// CDS option
write_cmos_sensor_twobyte(0x602A,0xF4AA);
write_cmos_sensor_twobyte(0x6F12,0x0040);	// RAMP option (150 ohm)
write_cmos_sensor_twobyte(0x602A,0xF486);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// DBR option
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x1CDC);
write_cmos_sensor_twobyte(0x6F12,0x0084);	// RDV option
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0xF442);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// DBUS option


//// DBUS setting
write_cmos_sensor_twobyte(0x602A,0x244E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// DBUS column offset off
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

//// Ramp Setting
write_cmos_sensor_twobyte(0x602A,0x1C77);
write_cmos_sensor(0x6F12,0x07);
write_cmos_sensor_twobyte(0x602A,0x1C78);
write_cmos_sensor(0x6F12,0x07);
write_cmos_sensor_twobyte(0x602A,0x1C79);
write_cmos_sensor(0x6F12,0x07);
write_cmos_sensor_twobyte(0x602A,0x1C7A);
write_cmos_sensor(0x6F12,0x07);

write_cmos_sensor_twobyte(0x602A,0x1C7B);
write_cmos_sensor(0x6F12,0x0F);
write_cmos_sensor_twobyte(0x602A,0x1C7C);
write_cmos_sensor(0x6F12,0x0F);
write_cmos_sensor_twobyte(0x602A,0x1C7D);
write_cmos_sensor(0x6F12,0x0F);
write_cmos_sensor_twobyte(0x602A,0x1C7E);
write_cmos_sensor(0x6F12,0x0F);

write_cmos_sensor_twobyte(0x602A,0x1C7F);
write_cmos_sensor(0x6F12,0x30);
write_cmos_sensor_twobyte(0x602A,0x1C80);
write_cmos_sensor(0x6F12,0x30);
write_cmos_sensor_twobyte(0x602A,0x1C81);
write_cmos_sensor(0x6F12,0x30);
write_cmos_sensor_twobyte(0x602A,0x1C82);
write_cmos_sensor(0x6F12,0x30);

write_cmos_sensor_twobyte(0x602A,0x1546);
write_cmos_sensor_twobyte(0x6F12,0x01C0);
write_cmos_sensor_twobyte(0x6F12,0x01C0);
write_cmos_sensor_twobyte(0x6F12,0x01C0);
write_cmos_sensor_twobyte(0x6F12,0x01C0);

//// APS setting
// WRITE 4000F4AC 005A  // ADCsat 720mV // 20150113 KTY
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0xF4AC);
write_cmos_sensor_twobyte(0x6F12,0x0062);	// ADCsat 760mV

/////////ADC Timing is updated below(20150102)
/////////////Type
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x1B34);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x1B35);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x1B66);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x1B67);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x1B68);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x1B69);
write_cmos_sensor(0x6F12,0x00);

////////////////////////////////////////////////////////////
//////Analog Setting End
////////////////////////////////////////////////////////////

// WDR
// pjw WRITE #smiaRegs_rw_wdr_multiple_exp_mode 00 // smiaRegs_rw_wdr_multiple_exp_mode
// pjw WRITE #smiaRegs_rw_wdr_exposure_order 00 // smiaRegs_rw_wdr_exposure_order

// Debug Path - PSP Bypass
// pjw WRITE 400070F2 0001

/////////////////////////////////////////////////
// PSP Normal (All PSP Blocks Bypass)- YJM
/////////////////////////////////////////////////
// TOP BPC DNS Bypass
write_cmos_sensor_twobyte(0x602A,0xB4C6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//[15:8] SpotRemoval_MainDenoise_DenoiseMode, Power_0_NoiseIndex_0[7:0]  SpotRemoval_MainDenoise_DenoiseModePower_0_NoiseIndex_1
write_cmos_sensor_twobyte(0x6F12,0x0000);	//[15:8] SpotRemoval_MainDenoise_DenoiseMode, Power_0_NoiseIndex_2[7:0]  SpotRemoval_MainDenoise_DenoiseModePower_0_NoiseIndex_3
write_cmos_sensor_twobyte(0x6F12,0x0000);	//[15:8] SpotRemoval_MainDenoise_DenoiseMode, Power_0_NoiseIndex_4[7:0]  SpotRemoval_MainDenoise_DenoiseModePower_0_NoiseIndex_5
write_cmos_sensor_twobyte(0x6F12,0x0000);	//[15:8] SpotRemoval_MainDenoise_DenoiseMode, Power_0_NoiseIndex_6[7:0]  SpotRemoval_MainDenoise_DenoiseModePower_1_NoiseIndex_0

write_cmos_sensor_twobyte(0x602A,0x6868);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// Despeckle static enable

write_cmos_sensor_twobyte(0x602A,0x58B0);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// GOS bypass
// WRITE #noiseNormTuningParams_bypass       0001   // Noise Norm

write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x30E6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// PDAF Disable
// WRITE #afStatisticsTuningParams_cg_bypass    0001  // PD Stat
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x74F0);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//First Gamma (4T)
// WRITE #thStatBayerTuningParams        0001  // ThStatBayer
write_cmos_sensor_twobyte(0x602A,0x78C0);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// Mixer
write_cmos_sensor_twobyte(0x602A,0x6830);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// Despeckle
write_cmos_sensor_twobyte(0x602A,0x5512);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// GRAS
write_cmos_sensor_twobyte(0x602A,0x7C90);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// DRC
write_cmos_sensor_twobyte(0x602A,0x6A10);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// all gamma block bypass
write_cmos_sensor_twobyte(0x602A,0x6C60);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// all gamma block bypass
write_cmos_sensor_twobyte(0x602A,0x6EB0);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// all gamma block bypass
write_cmos_sensor_twobyte(0x602A,0x8F20);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// ELG

write_cmos_sensor_twobyte(0x602A,0x10A4);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// ThStat
write_cmos_sensor_twobyte(0x602A,0x10AC);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// YSum
write_cmos_sensor_twobyte(0x602A,0x0F9C);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// RGBY Hist

write_cmos_sensor_twobyte(0x602A,0x6990);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// DTP

// Mixer
write_cmos_sensor_twobyte(0x602A,0xA8C8);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//[15:8] N/A, [7:0]  Misc_Bypass_DisableMixer
//WRITE #smiaRegs_vendor_psp_psp_config_info_removePedestalMode 0000   // r566 version¿¡´ {¿?°?!!!
write_cmos_sensor_twobyte(0x602A,0x0320);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// ISPShift
write_cmos_sensor_twobyte(0x602A,0x9000);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x9008);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x9010);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x9018);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// WRITE #afXtalkTuningParams_bypass             0000

// PDAF BPC Statics
write_cmos_sensor_twobyte(0x602A,0x90A0);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

write_cmos_sensor_twobyte(0x602A,0x5890);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// Noise Norm
write_cmos_sensor_twobyte(0x602A,0x6870);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// HQDNS

write_cmos_sensor_twobyte(0x602A,0x93A0);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x9190);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x51D4);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// pjw

// pjw WRITE #senHal_PspWaitLines 19
write_cmos_sensor_twobyte(0x602A,0x14C7);
write_cmos_sensor(0x6F12,0x05);

/////////////////////////////////////////////////
// PSP END
/////////////////////////////////////////////////

//Analog -tyk 141231
// pjw write_cmos_sensor_twobyte(0x0216,0x0101);
// pwj ?? WRITE 40000216 0101

write_cmos_sensor_twobyte(0x602A,0x156E);
write_cmos_sensor_twobyte(0x6F12,0x003D);	//aig_ld_ptr0
write_cmos_sensor_twobyte(0x602A,0x1572);
write_cmos_sensor_twobyte(0x6F12,0x000A);	//aig_ld_reg0

write_cmos_sensor_twobyte(0x602A,0x1576);
write_cmos_sensor_twobyte(0x6F12,0x002D);	//aig_sl_ptr0
write_cmos_sensor_twobyte(0x602A,0x157A);
write_cmos_sensor_twobyte(0x6F12,0x000A);	//aig_sl_reg0
write_cmos_sensor_twobyte(0x602A,0x1586);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_rx_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x158A);
write_cmos_sensor_twobyte(0x6F12,0x0012);	//aig_rx_p1_reg0
write_cmos_sensor_twobyte(0x602A,0x157E);
write_cmos_sensor_twobyte(0x6F12,0x0024);	//aig_rx_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x1582);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_rx_p0_reg0
write_cmos_sensor_twobyte(0x602A,0x158E);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_tx_ptr0
write_cmos_sensor_twobyte(0x602A,0x1592);
write_cmos_sensor_twobyte(0x6F12,0x02F8);	//aig_tx_ptr1
write_cmos_sensor_twobyte(0x602A,0x1596);
write_cmos_sensor_twobyte(0x6F12,0x0026);	//aig_fx_ptr0
write_cmos_sensor_twobyte(0x602A,0x159A);
write_cmos_sensor_twobyte(0x6F12,0x003A);	//aig_fx_ptr1
write_cmos_sensor_twobyte(0x602A,0x159E);
write_cmos_sensor_twobyte(0x6F12,0x002E);	//aig_top_sl_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x15A2);
write_cmos_sensor_twobyte(0x6F12,0x0035);	//aig_top_sl_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x15A6);
write_cmos_sensor_twobyte(0x6F12,0x0009);	//aig_top_sl_f_reg0
write_cmos_sensor_twobyte(0x602A,0x15AA);
write_cmos_sensor_twobyte(0x6F12,0x0002);	//aig_top_sl_f_reg1
write_cmos_sensor_twobyte(0x602A,0x15AE);
write_cmos_sensor_twobyte(0x6F12,0x0002);	//aig_top_rx_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x15B2);
write_cmos_sensor_twobyte(0x6F12,0x0009);	//aig_top_rx_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x15B6);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//aig_top_rx_f_ptr0
write_cmos_sensor_twobyte(0x602A,0x15BA);
write_cmos_sensor_twobyte(0x6F12,0x002C);	//aig_top_rx_f_ptr1
write_cmos_sensor_twobyte(0x602A,0x15BE);
write_cmos_sensor_twobyte(0x6F12,0x03F2);	//aig_top_rx_r_reg0
write_cmos_sensor_twobyte(0x602A,0x15C2);
write_cmos_sensor_twobyte(0x6F12,0x03EB);	//aig_top_rx_r_reg1
write_cmos_sensor_twobyte(0x602A,0x15C6);
write_cmos_sensor_twobyte(0x6F12,0x037B);	//aig_top_rx_f_reg0
write_cmos_sensor_twobyte(0x602A,0x15CA);
write_cmos_sensor_twobyte(0x6F12,0x0374);	//aig_top_rx_f_reg1
write_cmos_sensor_twobyte(0x602A,0x15CE);
write_cmos_sensor_twobyte(0x6F12,0x00C3);	//aig_top_rx_r_reg2
write_cmos_sensor_twobyte(0x602A,0x15D2);
write_cmos_sensor_twobyte(0x6F12,0x00BC);	//aig_top_rx_r_reg3
write_cmos_sensor_twobyte(0x602A,0x15D6);
write_cmos_sensor_twobyte(0x6F12,0x00C3);	//aig_top_rx_f_reg2
write_cmos_sensor_twobyte(0x602A,0x15DA);
write_cmos_sensor_twobyte(0x6F12,0x00BC);	//aig_top_rx_f_reg3
write_cmos_sensor_twobyte(0x602A,0x15DE);
write_cmos_sensor_twobyte(0x6F12,0x0015);	//aig_top_rx_r_reg4
write_cmos_sensor_twobyte(0x602A,0x15E2);
write_cmos_sensor_twobyte(0x6F12,0x000E);	//aig_top_rx_r_reg5
write_cmos_sensor_twobyte(0x602A,0x15E6);
write_cmos_sensor_twobyte(0x6F12,0x0015);	//aig_top_rx_f_reg4
write_cmos_sensor_twobyte(0x602A,0x15EA);
write_cmos_sensor_twobyte(0x6F12,0x000E);	//aig_top_rx_f_reg5
write_cmos_sensor_twobyte(0x602A,0x15EE);
write_cmos_sensor_twobyte(0x6F12,0x0031);	//aig_top_rx_r_reg6
write_cmos_sensor_twobyte(0x602A,0x15F2);
write_cmos_sensor_twobyte(0x6F12,0x002A);	//aig_top_rx_r_reg7
write_cmos_sensor_twobyte(0x602A,0x15F6);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//aig_top_rx_f_reg6
write_cmos_sensor_twobyte(0x602A,0x15FA);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_top_rx_f_reg7
write_cmos_sensor_twobyte(0x602A,0x15FE);
write_cmos_sensor_twobyte(0x6F12,0x0263);	//aig_top_tx_l_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x1602);
write_cmos_sensor_twobyte(0x6F12,0x026A);	//aig_top_tx_l_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x1606);
write_cmos_sensor_twobyte(0x6F12,0x02F9);	//aig_top_tx_l_f_ptr0
write_cmos_sensor_twobyte(0x602A,0x160A);
write_cmos_sensor_twobyte(0x6F12,0x0300);	//aig_top_tx_l_f_ptr1
write_cmos_sensor_twobyte(0x602A,0x160E);
write_cmos_sensor_twobyte(0x6F12,0x03E2);	//aig_top_tx_l_r_reg0
write_cmos_sensor_twobyte(0x602A,0x1612);
write_cmos_sensor_twobyte(0x6F12,0x03DB);	//aig_top_tx_l_r_reg1
write_cmos_sensor_twobyte(0x602A,0x1616);
write_cmos_sensor_twobyte(0x6F12,0x0397);	//aig_top_tx_l_f_reg0
write_cmos_sensor_twobyte(0x602A,0x161A);
write_cmos_sensor_twobyte(0x6F12,0x0390);	//aig_top_tx_l_f_reg1
write_cmos_sensor_twobyte(0x602A,0x161E);
write_cmos_sensor_twobyte(0x6F12,0x00B3);	//aig_top_tx_l_r_reg2
write_cmos_sensor_twobyte(0x602A,0x1622);
write_cmos_sensor_twobyte(0x6F12,0x00AC);	//aig_top_tx_l_r_reg3
write_cmos_sensor_twobyte(0x602A,0x1626);
write_cmos_sensor_twobyte(0x6F12,0x001D);	//aig_top_tx_l_f_reg2
write_cmos_sensor_twobyte(0x602A,0x162A);
write_cmos_sensor_twobyte(0x6F12,0x0016);	//aig_top_tx_l_f_reg3
write_cmos_sensor_twobyte(0x602A,0x162E);
write_cmos_sensor_twobyte(0x6F12,0x0263);	//aig_top_tx_s_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x1632);
write_cmos_sensor_twobyte(0x6F12,0x026A);	//aig_top_tx_s_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x1636);
write_cmos_sensor_twobyte(0x6F12,0x02F9);	//aig_top_tx_s_f_ptr0
write_cmos_sensor_twobyte(0x602A,0x163A);
write_cmos_sensor_twobyte(0x6F12,0x0300);	//aig_top_tx_s_f_ptr1
write_cmos_sensor_twobyte(0x602A,0x163E);
write_cmos_sensor_twobyte(0x6F12,0x03D6);	//aig_top_tx_s_r_reg0
write_cmos_sensor_twobyte(0x602A,0x1642);
write_cmos_sensor_twobyte(0x6F12,0x03CF);	//aig_top_tx_s_r_reg1
write_cmos_sensor_twobyte(0x602A,0x1646);
write_cmos_sensor_twobyte(0x6F12,0x038B);	//aig_top_tx_s_f_reg0
write_cmos_sensor_twobyte(0x602A,0x164A);
write_cmos_sensor_twobyte(0x6F12,0x0384);	//aig_top_tx_s_f_reg1
write_cmos_sensor_twobyte(0x602A,0x164E);
write_cmos_sensor_twobyte(0x6F12,0x00A7);	//aig_top_tx_s_r_reg2
write_cmos_sensor_twobyte(0x602A,0x1652);
write_cmos_sensor_twobyte(0x6F12,0x00A0);	//aig_top_tx_s_r_reg3
write_cmos_sensor_twobyte(0x602A,0x1656);
write_cmos_sensor_twobyte(0x6F12,0x0011);	//aig_top_tx_s_f_reg2
write_cmos_sensor_twobyte(0x602A,0x165A);
write_cmos_sensor_twobyte(0x6F12,0x000A);	//aig_top_tx_s_f_reg3
write_cmos_sensor_twobyte(0x602A,0x165E);
write_cmos_sensor_twobyte(0x6F12,0x0027);	//aig_top_fx_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x1662);
write_cmos_sensor_twobyte(0x6F12,0x002E);	//aig_top_fx_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x1666);
write_cmos_sensor_twobyte(0x6F12,0x003B);	//aig_top_fx_f_ptr0
write_cmos_sensor_twobyte(0x602A,0x166A);
write_cmos_sensor_twobyte(0x6F12,0x0042);	//aig_top_fx_f_ptr1
write_cmos_sensor_twobyte(0x602A,0x166E);
write_cmos_sensor_twobyte(0x6F12,0x00CB);	//aig_top_fx_r_reg0
write_cmos_sensor_twobyte(0x602A,0x1672);
write_cmos_sensor_twobyte(0x6F12,0x00C4);	//aig_top_fx_r_reg1
write_cmos_sensor_twobyte(0x602A,0x1676);
write_cmos_sensor_twobyte(0x6F12,0x0019);	//aig_top_fx_f_reg0
write_cmos_sensor_twobyte(0x602A,0x167A);
write_cmos_sensor_twobyte(0x6F12,0x0012);	//aig_top_fx_f_reg1
write_cmos_sensor_twobyte(0x602A,0x19DA);
write_cmos_sensor_twobyte(0x6F12,0x03F3);	//aig_srx_reg0
write_cmos_sensor_twobyte(0x602A,0x19DE);
write_cmos_sensor_twobyte(0x6F12,0x037C);	//aig_srx_reg1
write_cmos_sensor_twobyte(0x602A,0x19E2);
write_cmos_sensor_twobyte(0x6F12,0x00C4);	//aig_srx_reg2
write_cmos_sensor_twobyte(0x602A,0x19E6);
write_cmos_sensor_twobyte(0x6F12,0x0002);	//aig_srx_reg3
write_cmos_sensor_twobyte(0x602A,0x19EA);
write_cmos_sensor_twobyte(0x6F12,0x03E3);	//aig_dstx_l_reg0
write_cmos_sensor_twobyte(0x602A,0x19EE);
write_cmos_sensor_twobyte(0x6F12,0x0398);	//aig_dstx_l_reg1
write_cmos_sensor_twobyte(0x602A,0x19F2);
write_cmos_sensor_twobyte(0x6F12,0x00B4);	//aig_stx_l_reg0
write_cmos_sensor_twobyte(0x602A,0x19F6);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_stx_l_reg1
write_cmos_sensor_twobyte(0x602A,0x1A0A);
write_cmos_sensor_twobyte(0x6F12,0x03D7);	//aig_dstx_s_reg0
write_cmos_sensor_twobyte(0x602A,0x1A0E);
write_cmos_sensor_twobyte(0x6F12,0x038C);	//aig_dstx_s_reg1
write_cmos_sensor_twobyte(0x602A,0x1A12);
write_cmos_sensor_twobyte(0x6F12,0x00A8);	//aig_stx_s_reg0
write_cmos_sensor_twobyte(0x602A,0x1A16);
write_cmos_sensor_twobyte(0x6F12,0x0012);	//aig_stx_s_reg1
write_cmos_sensor_twobyte(0x602A,0x1A2A);
write_cmos_sensor_twobyte(0x6F12,0x00CC);	//aig_sfx_reg0
write_cmos_sensor_twobyte(0x602A,0x1A2E);
write_cmos_sensor_twobyte(0x6F12,0x0006);	//aig_sfx_reg1
write_cmos_sensor_twobyte(0x602A,0x19FA);
write_cmos_sensor_twobyte(0x6F12,0x03E3);	//aig_dabx_l_reg0
write_cmos_sensor_twobyte(0x602A,0x19FE);
write_cmos_sensor_twobyte(0x6F12,0x0398);	//aig_dabx_l_reg1
write_cmos_sensor_twobyte(0x602A,0x1A02);
write_cmos_sensor_twobyte(0x6F12,0x00B4);	//aig_abx_l_reg0
write_cmos_sensor_twobyte(0x602A,0x1A06);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_abx_l_reg1
write_cmos_sensor_twobyte(0x602A,0x1A1A);
write_cmos_sensor_twobyte(0x6F12,0x03D7);	//aig_dabx_s_reg0
write_cmos_sensor_twobyte(0x602A,0x1A1E);
write_cmos_sensor_twobyte(0x6F12,0x038C);	//aig_dabx_s_reg1
write_cmos_sensor_twobyte(0x602A,0x1A22);
write_cmos_sensor_twobyte(0x6F12,0x00A8);	//aig_abx_s_reg0
write_cmos_sensor_twobyte(0x602A,0x1A26);
write_cmos_sensor_twobyte(0x6F12,0x0012);	//aig_abx_s_reg1
write_cmos_sensor_twobyte(0x602A,0x1A32);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_offs_sh_reg0
write_cmos_sensor_twobyte(0x602A,0x167E);
write_cmos_sensor_twobyte(0x6F12,0x024E);	//aig_sr_ptr0
write_cmos_sensor_twobyte(0x602A,0x1682);
write_cmos_sensor_twobyte(0x6F12,0x0571);	//aig_sr_ptr1
write_cmos_sensor_twobyte(0x602A,0x1686);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_ss_ptr0
write_cmos_sensor_twobyte(0x602A,0x168A);
write_cmos_sensor_twobyte(0x6F12,0x056F);	//aig_ss_ptr1
write_cmos_sensor_twobyte(0x602A,0x168E);
write_cmos_sensor_twobyte(0x6F12,0x024E);	//aig_s1_ptr0
write_cmos_sensor_twobyte(0x602A,0x1692);
write_cmos_sensor_twobyte(0x6F12,0x0320);	//aig_s1_ptr1
write_cmos_sensor_twobyte(0x602A,0x1696);
write_cmos_sensor_twobyte(0x6F12,0x001A);	//aig_s3_ptr0
write_cmos_sensor_twobyte(0x602A,0x169A);
write_cmos_sensor_twobyte(0x6F12,0x0128);	//aig_s3_ptr1
write_cmos_sensor_twobyte(0x602A,0x169E);
write_cmos_sensor_twobyte(0x6F12,0x001A);	//aig_s4_ptr0
write_cmos_sensor_twobyte(0x602A,0x16A2);
write_cmos_sensor_twobyte(0x6F12,0x0114);	//aig_s4_ptr1
write_cmos_sensor_twobyte(0x602A,0x16A6);
write_cmos_sensor_twobyte(0x6F12,0x001A);	//aig_s4d_ptr0
write_cmos_sensor_twobyte(0x602A,0x16AA);
write_cmos_sensor_twobyte(0x6F12,0x0120);	//aig_s4d_ptr1
write_cmos_sensor_twobyte(0x602A,0x16AE);
write_cmos_sensor_twobyte(0x6F12,0x001A);	//aig_clp_sl_ptr0
write_cmos_sensor_twobyte(0x602A,0x16B2);
write_cmos_sensor_twobyte(0x6F12,0x024E);	//aig_clp_sl_ptr1
write_cmos_sensor_twobyte(0x602A,0x16B6);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_pxbst_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x16BA);
write_cmos_sensor_twobyte(0x6F12,0x005B);	//aig_pxbst_p0_ptr1
write_cmos_sensor_twobyte(0x602A,0x16BE);
write_cmos_sensor_twobyte(0x6F12,0x02F8);	//aig_pxbst_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x16C2);
write_cmos_sensor_twobyte(0x6F12,0x0320);	//aig_pxbst_p1_ptr1
write_cmos_sensor_twobyte(0x602A,0x16C6);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_pxbstob_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x16CA);
write_cmos_sensor_twobyte(0x6F12,0x005B);	//aig_pxbstob_p0_ptr1
write_cmos_sensor_twobyte(0x602A,0x16CE);
write_cmos_sensor_twobyte(0x6F12,0x02F8);	//aig_pxbstob_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x16D2);
write_cmos_sensor_twobyte(0x6F12,0x0300);	//aig_pxbstob_p1_ptr1

write_cmos_sensor_twobyte(0x602A,0x16EE);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_lp_hblk_cds_reg0
write_cmos_sensor_twobyte(0x602A,0x16F2);
write_cmos_sensor_twobyte(0x6F12,0x0005);	//aig_lp_hblk_cds_reg1
write_cmos_sensor_twobyte(0x602A,0x16F6);
write_cmos_sensor_twobyte(0x6F12,0x0042);	//aig_vref_smp_ptr0
write_cmos_sensor_twobyte(0x602A,0x16FA);
write_cmos_sensor_twobyte(0x6F12,0x0577);	//aig_vref_smp_ptr1
write_cmos_sensor_twobyte(0x602A,0x16FE);
write_cmos_sensor_twobyte(0x6F12,0x01F0);	//aig_cnt_en_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x1702);
write_cmos_sensor_twobyte(0x6F12,0x024E);	//aig_cnt_en_p0_ptr1
write_cmos_sensor_twobyte(0x602A,0x1706);
write_cmos_sensor_twobyte(0x6F12,0x043C);	//aig_cnt_en_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x170A);
write_cmos_sensor_twobyte(0x6F12,0x056D);	//aig_cnt_en_p1_ptr1
write_cmos_sensor_twobyte(0x602A,0x170E);
write_cmos_sensor_twobyte(0x6F12,0x0253);	//aig_conv_enb_ptr0
write_cmos_sensor_twobyte(0x602A,0x1712);
write_cmos_sensor_twobyte(0x6F12,0x026A);	//aig_conv_enb_ptr1
write_cmos_sensor_twobyte(0x602A,0x1716);
write_cmos_sensor_twobyte(0x6F12,0x025A);	//aig_conv1_ptr0
write_cmos_sensor_twobyte(0x602A,0x171A);
write_cmos_sensor_twobyte(0x6F12,0x0272);	//aig_conv1_ptr1
write_cmos_sensor_twobyte(0x602A,0x171E);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_conv2_ptr0
write_cmos_sensor_twobyte(0x602A,0x1722);
write_cmos_sensor_twobyte(0x6F12,0x0272);	//aig_conv2_ptr1
write_cmos_sensor_twobyte(0x602A,0x1726);
write_cmos_sensor_twobyte(0x6F12,0x0253);	//aig_lat_lsb_ptr0
write_cmos_sensor_twobyte(0x602A,0x172A);
write_cmos_sensor_twobyte(0x6F12,0x0258);	//aig_lat_lsb_ptr1
write_cmos_sensor_twobyte(0x602A,0x172E);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_conv_lsb_ptr0
write_cmos_sensor_twobyte(0x602A,0x1732);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_conv_lsb_ptr1
write_cmos_sensor_twobyte(0x602A,0x1736);
write_cmos_sensor_twobyte(0x6F12,0x025A);	//aig_conv_lsb_ptr2
write_cmos_sensor_twobyte(0x602A,0x173A);
write_cmos_sensor_twobyte(0x6F12,0x0272);	//aig_conv_lsb_ptr3
write_cmos_sensor_twobyte(0x602A,0x173E);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_rst_div_ptr0
write_cmos_sensor_twobyte(0x602A,0x1742);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_rst_div_ptr1
write_cmos_sensor_twobyte(0x602A,0x1746);
write_cmos_sensor_twobyte(0x6F12,0x0253);	//aig_rst_div_ptr2
write_cmos_sensor_twobyte(0x602A,0x174A);
write_cmos_sensor_twobyte(0x6F12,0x0258);	//aig_rst_div_ptr3
write_cmos_sensor_twobyte(0x602A,0x174E);
write_cmos_sensor_twobyte(0x6F12,0x01F0);	//aig_cnt_en_ms_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x1752);
write_cmos_sensor_twobyte(0x6F12,0x0216);	//aig_cnt_en_ms_p0_ptr1
write_cmos_sensor_twobyte(0x602A,0x1756);
write_cmos_sensor_twobyte(0x6F12,0x0226);	//aig_cnt_en_ms_p0_ptr2
write_cmos_sensor_twobyte(0x602A,0x175A);
write_cmos_sensor_twobyte(0x6F12,0x024E);	//aig_cnt_en_ms_p0_ptr3
write_cmos_sensor_twobyte(0x602A,0x175E);
write_cmos_sensor_twobyte(0x6F12,0x043C);	//aig_cnt_en_ms_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x1762);
write_cmos_sensor_twobyte(0x6F12,0x04CC);	//aig_cnt_en_ms_p1_ptr1
write_cmos_sensor_twobyte(0x602A,0x176E);
write_cmos_sensor_twobyte(0x6F12,0x04DC);	//aig_cnt_en_ms_p1_ptr2
write_cmos_sensor_twobyte(0x602A,0x1772);
write_cmos_sensor_twobyte(0x6F12,0x056D);	//aig_cnt_en_ms_p1_ptr3
write_cmos_sensor_twobyte(0x602A,0x1776);
write_cmos_sensor_twobyte(0x6F12,0x0218);	//aig_conv_enb_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x177A);
write_cmos_sensor_twobyte(0x6F12,0x021D);	//aig_conv_enb_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x177E);
write_cmos_sensor_twobyte(0x6F12,0x0254);	//aig_conv_enb_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x1782);
write_cmos_sensor_twobyte(0x6F12,0x025A);	//aig_conv_enb_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x1786);
write_cmos_sensor_twobyte(0x6F12,0x0265);	//aig_conv_enb_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x178A);
write_cmos_sensor_twobyte(0x6F12,0x026B);	//aig_conv_enb_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x178E);
write_cmos_sensor_twobyte(0x6F12,0x04CE);	//aig_conv_enb_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x1792);
write_cmos_sensor_twobyte(0x6F12,0x04D3);	//aig_conv_enb_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x1796);
write_cmos_sensor_twobyte(0x6F12,0x056F);	//aig_conv_enb_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x179A);
write_cmos_sensor_twobyte(0x6F12,0x0574);	//aig_conv_enb_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x179E);
write_cmos_sensor_twobyte(0x6F12,0x021A);	//aig_conv1_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x17A2);
write_cmos_sensor_twobyte(0x6F12,0x021E);	//aig_conv1_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x17A6);
write_cmos_sensor_twobyte(0x6F12,0x0256);	//aig_conv1_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x17AA);
write_cmos_sensor_twobyte(0x6F12,0x025B);	//aig_conv1_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x17AE);
write_cmos_sensor_twobyte(0x6F12,0x0267);	//aig_conv1_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x17B2);
write_cmos_sensor_twobyte(0x6F12,0x026C);	//aig_conv1_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x17B6);
write_cmos_sensor_twobyte(0x6F12,0x04D0);	//aig_conv1_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x17BA);
write_cmos_sensor_twobyte(0x6F12,0x04D4);	//aig_conv1_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x17BE);
write_cmos_sensor_twobyte(0x6F12,0x0571);	//aig_conv1_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x17C2);
write_cmos_sensor_twobyte(0x6F12,0x0575);	//aig_conv1_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x17C6);
write_cmos_sensor_twobyte(0x6F12,0x021B);	//aig_conv2_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x17CA);
write_cmos_sensor_twobyte(0x6F12,0x021E);	//aig_conv2_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x17CE);
write_cmos_sensor_twobyte(0x6F12,0x0257);	//aig_conv2_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x17D2);
write_cmos_sensor_twobyte(0x6F12,0x025B);	//aig_conv2_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x17D6);
write_cmos_sensor_twobyte(0x6F12,0x0268);	//aig_conv2_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x17DA);
write_cmos_sensor_twobyte(0x6F12,0x026C);	//aig_conv2_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x17DE);
write_cmos_sensor_twobyte(0x6F12,0x04D1);	//aig_conv2_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x17E2);
write_cmos_sensor_twobyte(0x6F12,0x04D4);	//aig_conv2_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x17E6);
write_cmos_sensor_twobyte(0x6F12,0x0572);	//aig_conv2_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x17EA);
write_cmos_sensor_twobyte(0x6F12,0x0575);	//aig_conv2_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x17EE);
write_cmos_sensor_twobyte(0x6F12,0x0219);	//aig_lat_lsb_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x17F2);
write_cmos_sensor_twobyte(0x6F12,0x021B);	//aig_lat_lsb_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x17F6);
write_cmos_sensor_twobyte(0x6F12,0x0251);	//aig_lat_lsb_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x17FA);
write_cmos_sensor_twobyte(0x6F12,0x0254);	//aig_lat_lsb_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x17FE);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_lat_lsb_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x1802);
write_cmos_sensor_twobyte(0x6F12,0x0265);	//aig_lat_lsb_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x1806);
write_cmos_sensor_twobyte(0x6F12,0x04CE);	//aig_lat_lsb_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x180A);
write_cmos_sensor_twobyte(0x6F12,0x04D1);	//aig_lat_lsb_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x180E);
write_cmos_sensor_twobyte(0x6F12,0x056F);	//aig_lat_lsb_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x1812);
write_cmos_sensor_twobyte(0x6F12,0x0572);	//aig_lat_lsb_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x1816);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_conv_lsb_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x181A);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_conv_lsb_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x181E);
write_cmos_sensor_twobyte(0x6F12,0x021D);	//aig_conv_lsb_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x1822);
write_cmos_sensor_twobyte(0x6F12,0x0220);	//aig_conv_lsb_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x1826);
write_cmos_sensor_twobyte(0x6F12,0x0256);	//aig_conv_lsb_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x182A);
write_cmos_sensor_twobyte(0x6F12,0x025B);	//aig_conv_lsb_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x182E);
write_cmos_sensor_twobyte(0x6F12,0x0267);	//aig_conv_lsb_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x1832);
write_cmos_sensor_twobyte(0x6F12,0x026C);	//aig_conv_lsb_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x1836);
write_cmos_sensor_twobyte(0x6F12,0x04D3);	//aig_conv_lsb_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x183A);
write_cmos_sensor_twobyte(0x6F12,0x04D6);	//aig_conv_lsb_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x183E);
write_cmos_sensor_twobyte(0x6F12,0x0574);	//aig_conv_lsb_ms_ptr10
write_cmos_sensor_twobyte(0x602A,0x1842);
write_cmos_sensor_twobyte(0x6F12,0x0577);	//aig_conv_lsb_ms_ptr11
write_cmos_sensor_twobyte(0x602A,0x1846);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_rst_div_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x184A);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_rst_div_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x184E);
write_cmos_sensor_twobyte(0x6F12,0x0218);	//aig_rst_div_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x1852);
write_cmos_sensor_twobyte(0x6F12,0x021B);	//aig_rst_div_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x1856);
write_cmos_sensor_twobyte(0x6F12,0x0251);	//aig_rst_div_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x185A);
write_cmos_sensor_twobyte(0x6F12,0x0254);	//aig_rst_div_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x185E);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_rst_div_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x1862);
write_cmos_sensor_twobyte(0x6F12,0x0265);	//aig_rst_div_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x1866);
write_cmos_sensor_twobyte(0x6F12,0x04CE);	//aig_rst_div_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x186A);
write_cmos_sensor_twobyte(0x6F12,0x04D1);	//aig_rst_div_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x186E);
write_cmos_sensor_twobyte(0x6F12,0x056F);	//aig_rst_div_ms_ptr10
write_cmos_sensor_twobyte(0x602A,0x1872);
write_cmos_sensor_twobyte(0x6F12,0x0572);	//aig_rst_div_ms_ptr11
write_cmos_sensor_twobyte(0x602A,0x1876);
write_cmos_sensor_twobyte(0x6F12,0x021D);	//aig_piv_en_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x187A);
write_cmos_sensor_twobyte(0x6F12,0x0256);	//aig_piv_en_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x187E);
write_cmos_sensor_twobyte(0x6F12,0x04D3);	//aig_piv_en_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x1882);
write_cmos_sensor_twobyte(0x6F12,0x0571);	//aig_piv_en_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x1886);
write_cmos_sensor_twobyte(0x6F12,0x01EF);	//aig_comp_en_ptr0
write_cmos_sensor_twobyte(0x602A,0x188A);
write_cmos_sensor_twobyte(0x6F12,0x024F);	//aig_comp_en_ptr1
write_cmos_sensor_twobyte(0x602A,0x188E);
write_cmos_sensor_twobyte(0x6F12,0x043B);	//aig_comp_en_ptr2
write_cmos_sensor_twobyte(0x602A,0x1892);
write_cmos_sensor_twobyte(0x6F12,0x056E);	//aig_comp_en_ptr3
write_cmos_sensor_twobyte(0x602A,0x1896);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_cnt_rst_ptr0
write_cmos_sensor_twobyte(0x602A,0x189A);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_cnt_rst_ptr1
write_cmos_sensor_twobyte(0x602A,0x189E);
write_cmos_sensor_twobyte(0x6F12,0x000C);	//aig_conv_en_offset_ptr0
write_cmos_sensor_twobyte(0x602A,0x18A2);
write_cmos_sensor_twobyte(0x6F12,0x0014);	//aig_conv_en_offset_ptr1
write_cmos_sensor_twobyte(0x602A,0x18A6);
write_cmos_sensor_twobyte(0x6F12,0x0253);	//aig_conv_en_offset_ptr2
write_cmos_sensor_twobyte(0x602A,0x18AA);
write_cmos_sensor_twobyte(0x6F12,0x026D);	//aig_conv_en_offset_ptr3
write_cmos_sensor_twobyte(0x602A,0x18AE);
write_cmos_sensor_twobyte(0x6F12,0x000C);	//aig_lat_lsb_offset_ptr0
write_cmos_sensor_twobyte(0x602A,0x18B2);
write_cmos_sensor_twobyte(0x6F12,0x0014);	//aig_lat_lsb_offset_ptr1
write_cmos_sensor_twobyte(0x602A,0x18B6);
write_cmos_sensor_twobyte(0x6F12,0x0253);	//aig_lat_lsb_offset_ptr2
write_cmos_sensor_twobyte(0x602A,0x18BA);
write_cmos_sensor_twobyte(0x6F12,0x0258);	//aig_lat_lsb_offset_ptr3
write_cmos_sensor_twobyte(0x602A,0x18BE);
write_cmos_sensor_twobyte(0x6F12,0x003C);	//aig_lp_hblk_dbs_ptr0
write_cmos_sensor_twobyte(0x602A,0x18C2);
write_cmos_sensor_twobyte(0x6F12,0x000A);	//aig_lp_hblk_dbs_reg1
write_cmos_sensor_twobyte(0x602A,0x18C6);
write_cmos_sensor_twobyte(0x6F12,0x0150);	//aig_off_rst_en_ptr0
write_cmos_sensor_twobyte(0x602A,0x18CA);
write_cmos_sensor_twobyte(0x6F12,0x0573);	//aig_off_rst_en_ptr1
write_cmos_sensor_twobyte(0x602A,0x18CE);
write_cmos_sensor_twobyte(0x6F12,0x0151);	//aig_rmp_rst_ptr0
write_cmos_sensor_twobyte(0x602A,0x18D2);
write_cmos_sensor_twobyte(0x6F12,0x0153);	//aig_rmp_rst_ptr1
write_cmos_sensor_twobyte(0x602A,0x18D6);
write_cmos_sensor_twobyte(0x6F12,0x0250);	//aig_rmp_rst_ptr2
write_cmos_sensor_twobyte(0x602A,0x18DA);
write_cmos_sensor_twobyte(0x6F12,0x0253);	//aig_rmp_rst_ptr3
write_cmos_sensor_twobyte(0x602A,0x18DE);
write_cmos_sensor_twobyte(0x6F12,0x056F);	//aig_rmp_rst_ptr4
write_cmos_sensor_twobyte(0x602A,0x18E2);
write_cmos_sensor_twobyte(0x6F12,0x0572);	//aig_rmp_rst_ptr5
write_cmos_sensor_twobyte(0x602A,0x18E6);
write_cmos_sensor_twobyte(0x6F12,0x0250);	//aig_rmp_mode_ptr0
write_cmos_sensor_twobyte(0x602A,0x18EA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr0
write_cmos_sensor_twobyte(0x602A,0x18EE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr1
write_cmos_sensor_twobyte(0x602A,0x18F2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr2
write_cmos_sensor_twobyte(0x602A,0x18F6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr3
write_cmos_sensor_twobyte(0x602A,0x18FA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr4
write_cmos_sensor_twobyte(0x602A,0x18FE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr5
write_cmos_sensor_twobyte(0x602A,0x1902);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr6
write_cmos_sensor_twobyte(0x602A,0x1906);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr7
write_cmos_sensor_twobyte(0x602A,0x190A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr0
write_cmos_sensor_twobyte(0x602A,0x190E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr1
write_cmos_sensor_twobyte(0x602A,0x1912);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr2
write_cmos_sensor_twobyte(0x602A,0x1916);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr3
write_cmos_sensor_twobyte(0x602A,0x191A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr4
write_cmos_sensor_twobyte(0x602A,0x191E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr5
write_cmos_sensor_twobyte(0x602A,0x1922);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr6
write_cmos_sensor_twobyte(0x602A,0x1926);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr7
write_cmos_sensor_twobyte(0x602A,0x192A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr0
write_cmos_sensor_twobyte(0x602A,0x192E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr1
write_cmos_sensor_twobyte(0x602A,0x1932);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr2
write_cmos_sensor_twobyte(0x602A,0x1936);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr3
write_cmos_sensor_twobyte(0x602A,0x193A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr4
write_cmos_sensor_twobyte(0x602A,0x193E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr5
write_cmos_sensor_twobyte(0x602A,0x1942);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr6
write_cmos_sensor_twobyte(0x602A,0x1946);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr7
write_cmos_sensor_twobyte(0x602A,0x1992);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr0
write_cmos_sensor_twobyte(0x602A,0x1996);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr1
write_cmos_sensor_twobyte(0x602A,0x199A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr2
write_cmos_sensor_twobyte(0x602A,0x199E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr3
write_cmos_sensor_twobyte(0x602A,0x19A2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr4
write_cmos_sensor_twobyte(0x602A,0x19A6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr5
write_cmos_sensor_twobyte(0x602A,0x19AA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr6
write_cmos_sensor_twobyte(0x602A,0x19AE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr7
write_cmos_sensor_twobyte(0x602A,0x19B2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr0
write_cmos_sensor_twobyte(0x602A,0x19B6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr1
write_cmos_sensor_twobyte(0x602A,0x19BA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr2
write_cmos_sensor_twobyte(0x602A,0x19BE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr3
write_cmos_sensor_twobyte(0x602A,0x19C2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr4
write_cmos_sensor_twobyte(0x602A,0x19C6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr5
write_cmos_sensor_twobyte(0x602A,0x19CA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr6
write_cmos_sensor_twobyte(0x602A,0x19CE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr7
write_cmos_sensor_twobyte(0x602A,0x1A36);
write_cmos_sensor_twobyte(0x6F12,0x0006);	//aig_lat_set_reg0
write_cmos_sensor_twobyte(0x602A,0x1A3A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_lat_set_reg1
write_cmos_sensor_twobyte(0x602A,0x19D2);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_vda_rst_ptr0
write_cmos_sensor_twobyte(0x602A,0x19D6);
write_cmos_sensor_twobyte(0x6F12,0x000E);	//aig_vda_rst_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A3E);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_vda_rd_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A42);
write_cmos_sensor_twobyte(0x6F12,0x0024);	//aig_vda_rd_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A46);
write_cmos_sensor_twobyte(0x6F12,0x0034);	//aig_vda_rd_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A4A);
write_cmos_sensor_twobyte(0x6F12,0x003A);	//aig_vda_rd_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A4E);
write_cmos_sensor_twobyte(0x6F12,0x004A);	//aig_vda_sh1_s_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A52);
write_cmos_sensor_twobyte(0x6F12,0x0050);	//aig_vda_sh1_s_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A56);
write_cmos_sensor_twobyte(0x6F12,0x0060);	//aig_vda_sh1_s_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A5A);
write_cmos_sensor_twobyte(0x6F12,0x0066);	//aig_vda_sh1_s_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A5E);
write_cmos_sensor_twobyte(0x6F12,0x0076);	//aig_vda_sh2_s_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A62);
write_cmos_sensor_twobyte(0x6F12,0x007C);	//aig_vda_sh2_s_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A66);
write_cmos_sensor_twobyte(0x6F12,0x008C);	//aig_vda_sh2_s_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A6A);
write_cmos_sensor_twobyte(0x6F12,0x0092);	//aig_vda_sh2_s_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A6E);
write_cmos_sensor_twobyte(0x6F12,0x00A2);	//aig_vda_sh1_l_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A72);
write_cmos_sensor_twobyte(0x6F12,0x00A8);	//aig_vda_sh1_l_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A76);
write_cmos_sensor_twobyte(0x6F12,0x00B8);	//aig_vda_sh1_l_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A7A);
write_cmos_sensor_twobyte(0x6F12,0x00BE);	//aig_vda_sh1_l_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A7E);
write_cmos_sensor_twobyte(0x6F12,0x00CE);	//aig_vda_sh2_l_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A82);
write_cmos_sensor_twobyte(0x6F12,0x00D4);	//aig_vda_sh2_l_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A86);
write_cmos_sensor_twobyte(0x6F12,0x00E4);	//aig_vda_sh2_l_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A8A);
write_cmos_sensor_twobyte(0x6F12,0x00EA);	//aig_vda_sh2_l_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A8E);
write_cmos_sensor_twobyte(0x6F12,0x0015);	//aig_vda_pulse_ptr0
write_cmos_sensor_twobyte(0x602A,0x1A92);
write_cmos_sensor_twobyte(0x6F12,0x002B);	//aig_vda_pulse_ptr1
write_cmos_sensor_twobyte(0x602A,0x1A96);
write_cmos_sensor_twobyte(0x6F12,0x0041);	//aig_vda_pulse_ptr2
write_cmos_sensor_twobyte(0x602A,0x1A9A);
write_cmos_sensor_twobyte(0x6F12,0x0057);	//aig_vda_pulse_ptr3
write_cmos_sensor_twobyte(0x602A,0x1A9E);
write_cmos_sensor_twobyte(0x6F12,0x006D);	//aig_vda_pulse_ptr4
write_cmos_sensor_twobyte(0x602A,0x1AA2);
write_cmos_sensor_twobyte(0x6F12,0x0083);	//aig_vda_pulse_ptr5
write_cmos_sensor_twobyte(0x602A,0x1AA6);
write_cmos_sensor_twobyte(0x6F12,0x0099);	//aig_vda_pulse_ptr6
write_cmos_sensor_twobyte(0x602A,0x1AAA);
write_cmos_sensor_twobyte(0x6F12,0x00AF);	//aig_vda_pulse_ptr7
write_cmos_sensor_twobyte(0x602A,0x1AAE);
write_cmos_sensor_twobyte(0x6F12,0x00C5);	//aig_vda_pulse_ptr8
write_cmos_sensor_twobyte(0x602A,0x1AB2);
write_cmos_sensor_twobyte(0x6F12,0x00DB);	//aig_vda_pulse_ptr9
write_cmos_sensor_twobyte(0x602A,0x1AB6);
write_cmos_sensor_twobyte(0x6F12,0x00F1);	//aig_vda_pulse_ptr10

write_cmos_sensor_twobyte(0x602A,0x1ABA);
write_cmos_sensor_twobyte(0x6F12,0x0329);	//VIR_1_NFA_PTR0
write_cmos_sensor_twobyte(0x602A,0x1ABE);
write_cmos_sensor_twobyte(0x6F12,0x0328);	//VIR_1_NFA_PTR1
write_cmos_sensor_twobyte(0x602A,0x1AC2);
write_cmos_sensor_twobyte(0x6F12,0x0620);	//VIR_1_NFA_PTR2
write_cmos_sensor_twobyte(0x602A,0x1AC6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//VIR_1_NFA_PTR3
write_cmos_sensor_twobyte(0x602A,0x1ACA);
write_cmos_sensor_twobyte(0x6F12,0x03D6);	//VIR_MSO_PTR0
write_cmos_sensor_twobyte(0x602A,0x1ACE);
write_cmos_sensor_twobyte(0x6F12,0x05EF);	//VIR_MIN_ADC_COLS
write_cmos_sensor_twobyte(0x602A,0x1AD2);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//VIR_MVF_PTR0

write_cmos_sensor_twobyte(0x602A,0x1AD6);
write_cmos_sensor_twobyte(0x6F12,0x0329);	//VIR_1_NFA_PTR0
write_cmos_sensor_twobyte(0x602A,0x1ADA);
write_cmos_sensor_twobyte(0x6F12,0x0328);	//VIR_1_NFA_PTR1
write_cmos_sensor_twobyte(0x602A,0x1ADE);
write_cmos_sensor_twobyte(0x6F12,0x0620);	//VIR_1_NFA_PTR2
write_cmos_sensor_twobyte(0x602A,0x1AE2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//VIR_1_NFA_PTR3
write_cmos_sensor_twobyte(0x602A,0x1AE6);
write_cmos_sensor_twobyte(0x6F12,0x03D6);	//VIR_MSO_PTR0
write_cmos_sensor_twobyte(0x602A,0x1AEA);
write_cmos_sensor_twobyte(0x6F12,0x05EF);	//VIR_MIN_ADC_COLS
write_cmos_sensor_twobyte(0x602A,0x1AEE);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//VIR_MVF_PTR0

write_cmos_sensor_twobyte(0x602A,0x1AF2);
write_cmos_sensor_twobyte(0x6F12,0x0329);	//VIR_1_NFA_PTR0
write_cmos_sensor_twobyte(0x602A,0x1AF6);
write_cmos_sensor_twobyte(0x6F12,0x0328);	//VIR_1_NFA_PTR1
write_cmos_sensor_twobyte(0x602A,0x1AFA);
write_cmos_sensor_twobyte(0x6F12,0x0620);	//VIR_1_NFA_PTR2
write_cmos_sensor_twobyte(0x602A,0x1AFE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//VIR_1_NFA_PTR3
write_cmos_sensor_twobyte(0x602A,0x1B02);
write_cmos_sensor_twobyte(0x6F12,0x03D6);	//VIR_MSO_PTR0
write_cmos_sensor_twobyte(0x602A,0x1B06);
write_cmos_sensor_twobyte(0x6F12,0x05EF);	//VIR_MIN_ADC_COLS
write_cmos_sensor_twobyte(0x602A,0x1B0A);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//VIR_MVF_PTR0

write_cmos_sensor_twobyte(0x602A,0x1B0E);
write_cmos_sensor_twobyte(0x6F12,0x0329);	//VIR_1_NFA_PTR0
write_cmos_sensor_twobyte(0x602A,0x1B12);
write_cmos_sensor_twobyte(0x6F12,0x0328);	//VIR_1_NFA_PTR1
write_cmos_sensor_twobyte(0x602A,0x1B16);
write_cmos_sensor_twobyte(0x6F12,0x0620);	//VIR_1_NFA_PTR2
write_cmos_sensor_twobyte(0x602A,0x1B1A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//VIR_1_NFA_PTR3
write_cmos_sensor_twobyte(0x602A,0x1B1E);
write_cmos_sensor_twobyte(0x6F12,0x03D6);	//VIR_MSO_PTR0
write_cmos_sensor_twobyte(0x602A,0x1B22);
write_cmos_sensor_twobyte(0x6F12,0x05EF);	//VIR_MIN_ADC_COLS
write_cmos_sensor_twobyte(0x602A,0x1B26);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//VIR_MVF_PTR0

//////////// Analog Tuning 150111 ////////////
write_cmos_sensor_twobyte(0x602A,0x1CD0);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//VRGSL // 20150124 KTY
write_cmos_sensor_twobyte(0x602A,0x1CCE);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//VRGSL // 20150124 KTY
write_cmos_sensor_twobyte(0x602A,0x1CD4);
write_cmos_sensor_twobyte(0x6F12,0x000D);	// 20150113 KTY
write_cmos_sensor_twobyte(0x6F12,0x000D);	// 20150113 KTY
write_cmos_sensor_twobyte(0x6F12,0x000D);	// 20150113 KTY
// pjw write_cmos_sensor_twobyte(0xf480,0x0008); //VTG
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0xF480);
write_cmos_sensor_twobyte(0x6F12,0x0008);	// VTG

// pjw write_cmos_sensor_twobyte(0xf4d0,0x0020); //Atten AGx2
write_cmos_sensor_twobyte(0x602A,0xF4D0);
write_cmos_sensor_twobyte(0x6F12,0x0020);	// Atten AGx2
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x245E);
write_cmos_sensor_twobyte(0x6F12,0x0610);	// 20150113 HHJ
write_cmos_sensor_twobyte(0x6F12,0x0610);	// 20150113 HHJ

//Gain Linearity Compensation
//Short Ramp
////Gr
write_cmos_sensor_twobyte(0x602A,0x3E22);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x103D);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x103D);
write_cmos_sensor_twobyte(0x6F12,0x1051);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1051);
write_cmos_sensor_twobyte(0x6F12,0x107A);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x107A);
write_cmos_sensor_twobyte(0x6F12,0x108F);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x108F);
write_cmos_sensor_twobyte(0x6F12,0x10A3);
////R
write_cmos_sensor_twobyte(0x602A,0x3E62);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1039);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1039);
write_cmos_sensor_twobyte(0x6F12,0x103D);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x103D);
write_cmos_sensor_twobyte(0x6F12,0x1049);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1049);
write_cmos_sensor_twobyte(0x6F12,0x1066);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1066);
write_cmos_sensor_twobyte(0x6F12,0x107A);
////B
write_cmos_sensor_twobyte(0x602A,0x3EA2);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1039);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1039);
write_cmos_sensor_twobyte(0x6F12,0x103D);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x103D);
write_cmos_sensor_twobyte(0x6F12,0x1049);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1049);
write_cmos_sensor_twobyte(0x6F12,0x1066);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1066);
write_cmos_sensor_twobyte(0x6F12,0x107A);
////Gb
write_cmos_sensor_twobyte(0x602A,0x3EE2);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x103D);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x103D);
write_cmos_sensor_twobyte(0x6F12,0x1051);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1051);
write_cmos_sensor_twobyte(0x6F12,0x107A);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x107A);
write_cmos_sensor_twobyte(0x6F12,0x108F);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x108F);
write_cmos_sensor_twobyte(0x6F12,0x10A3);

//long Ramp
////Gr
write_cmos_sensor_twobyte(0x602A,0x3F22);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x103D);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x103D);
write_cmos_sensor_twobyte(0x6F12,0x1051);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1051);
write_cmos_sensor_twobyte(0x6F12,0x107A);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x107A);
write_cmos_sensor_twobyte(0x6F12,0x108F);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x108F);
write_cmos_sensor_twobyte(0x6F12,0x10A3);
////R
write_cmos_sensor_twobyte(0x602A,0x3F62);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1039);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1039);
write_cmos_sensor_twobyte(0x6F12,0x103D);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x103D);
write_cmos_sensor_twobyte(0x6F12,0x1049);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1049);
write_cmos_sensor_twobyte(0x6F12,0x1066);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1066);
write_cmos_sensor_twobyte(0x6F12,0x107A);
////B
write_cmos_sensor_twobyte(0x602A,0x3FA2);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1039);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1039);
write_cmos_sensor_twobyte(0x6F12,0x103D);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x103D);
write_cmos_sensor_twobyte(0x6F12,0x1049);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1049);
write_cmos_sensor_twobyte(0x6F12,0x1066);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1066);
write_cmos_sensor_twobyte(0x6F12,0x107A);
////Gb
write_cmos_sensor_twobyte(0x602A,0x3FE2);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x103D);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x103D);
write_cmos_sensor_twobyte(0x6F12,0x1051);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1051);
write_cmos_sensor_twobyte(0x6F12,0x107A);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x107A);
write_cmos_sensor_twobyte(0x6F12,0x108F);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x108F);
write_cmos_sensor_twobyte(0x6F12,0x10A3);

// ramp pwdn
write_cmos_sensor_twobyte(0x602A,0x1CC8);
write_cmos_sensor_twobyte(0x6F12,0x3EF8);	// F406 address :VBLK_EN on
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0xF4AA);
write_cmos_sensor_twobyte(0x6F12,0x0048);	//RAMP power save @ SL off

mDELAY(10);
}

static void sensor_init_11(void)
{
/*2X8_global(HQ)_1014.sset*/
LOG_INF("Enter s5k2x8 sensor_init.\n");

//Clock Gen
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x6214,0xFFFF);
write_cmos_sensor_twobyte(0x6216,0xFFFF);
write_cmos_sensor_twobyte(0x6218,0x0000);
write_cmos_sensor_twobyte(0x621A,0x0000);

// Start T&P part
// DO NOT DELETE T&P SECTION COMMENTS! They are required to debug T&P related issues.
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0x4DC0);
write_cmos_sensor_twobyte(0x6F12,0x0449);
write_cmos_sensor_twobyte(0x6F12,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x044A);
write_cmos_sensor_twobyte(0x6F12,0x0860);
write_cmos_sensor_twobyte(0x6F12,0x101A);
write_cmos_sensor_twobyte(0x6F12,0x4860);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xFFBA);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x54E8);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xBE60);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0xAE00);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x10B5);
write_cmos_sensor_twobyte(0x6F12,0x0228);
write_cmos_sensor_twobyte(0x6F12,0x03D8);
write_cmos_sensor_twobyte(0x6F12,0x0229);
write_cmos_sensor_twobyte(0x6F12,0x01D8);
write_cmos_sensor_twobyte(0x6F12,0x0124);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x0024);
write_cmos_sensor_twobyte(0x6F12,0xFB48);
write_cmos_sensor_twobyte(0x6F12,0x0078);
write_cmos_sensor_twobyte(0x6F12,0x08B1);
write_cmos_sensor_twobyte(0x6F12,0x0124);
write_cmos_sensor_twobyte(0x6F12,0x04E0);
write_cmos_sensor_twobyte(0x6F12,0x1CB9);
write_cmos_sensor_twobyte(0x6F12,0x0021);
write_cmos_sensor_twobyte(0x6F12,0x5020);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x2AFB);
write_cmos_sensor_twobyte(0x6F12,0x2046);
write_cmos_sensor_twobyte(0x6F12,0x10BD);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF047);
write_cmos_sensor_twobyte(0x6F12,0x0546);
write_cmos_sensor_twobyte(0x6F12,0xDDE9);
write_cmos_sensor_twobyte(0x6F12,0x0897);
write_cmos_sensor_twobyte(0x6F12,0x8846);
write_cmos_sensor_twobyte(0x6F12,0x1646);
write_cmos_sensor_twobyte(0x6F12,0x1C46);
write_cmos_sensor_twobyte(0x6F12,0x022B);
write_cmos_sensor_twobyte(0x6F12,0x15D3);
write_cmos_sensor_twobyte(0x6F12,0xB542);
write_cmos_sensor_twobyte(0x6F12,0x03D2);
write_cmos_sensor_twobyte(0x6F12,0x0146);
write_cmos_sensor_twobyte(0x6F12,0x5120);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x18FB);
write_cmos_sensor_twobyte(0x6F12,0xA81B);
write_cmos_sensor_twobyte(0x6F12,0xA8EB);
write_cmos_sensor_twobyte(0x6F12,0x0601);
write_cmos_sensor_twobyte(0x6F12,0x022C);
write_cmos_sensor_twobyte(0x6F12,0x12D0);
write_cmos_sensor_twobyte(0x6F12,0x80EA);
write_cmos_sensor_twobyte(0x6F12,0x0402);
write_cmos_sensor_twobyte(0x6F12,0x0244);
write_cmos_sensor_twobyte(0x6F12,0x6000);
write_cmos_sensor_twobyte(0x6F12,0xB2FB);
write_cmos_sensor_twobyte(0x6F12,0xF0F5);
write_cmos_sensor_twobyte(0x6F12,0x81EA);
write_cmos_sensor_twobyte(0x6F12,0x0402);
write_cmos_sensor_twobyte(0x6F12,0x1144);
write_cmos_sensor_twobyte(0x6F12,0xB1FB);
write_cmos_sensor_twobyte(0x6F12,0xF0F8);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x4545);
write_cmos_sensor_twobyte(0x6F12,0x0ED2);
write_cmos_sensor_twobyte(0x6F12,0xA8EB);
write_cmos_sensor_twobyte(0x6F12,0x0501);
write_cmos_sensor_twobyte(0x6F12,0xC9F8);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x10E0);
write_cmos_sensor_twobyte(0x6F12,0x80F0);
write_cmos_sensor_twobyte(0x6F12,0x0202);
write_cmos_sensor_twobyte(0x6F12,0x1044);
write_cmos_sensor_twobyte(0x6F12,0x8508);
write_cmos_sensor_twobyte(0x6F12,0x81F0);
write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x0844);
write_cmos_sensor_twobyte(0x6F12,0x4FEA);
write_cmos_sensor_twobyte(0x6F12,0x9008);
write_cmos_sensor_twobyte(0x6F12,0xEDE7);
write_cmos_sensor_twobyte(0x6F12,0xC9F8);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x022C);
write_cmos_sensor_twobyte(0x6F12,0xA5EB);
write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x00D1);
write_cmos_sensor_twobyte(0x6F12,0x4000);
write_cmos_sensor_twobyte(0x6F12,0x3860);
write_cmos_sensor_twobyte(0x6F12,0xBDE8);
write_cmos_sensor_twobyte(0x6F12,0xF087);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF041);
write_cmos_sensor_twobyte(0x6F12,0xD64A);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0x5C83);
write_cmos_sensor_twobyte(0x6F12,0x0446);
write_cmos_sensor_twobyte(0x6F12,0x0D46);
write_cmos_sensor_twobyte(0x6F12,0x1178);
write_cmos_sensor_twobyte(0x6F12,0x98F8);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x6F12,0x8AB0);
write_cmos_sensor_twobyte(0x6F12,0x21B9);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x2510);
write_cmos_sensor_twobyte(0x6F12,0x01B1);
write_cmos_sensor_twobyte(0x6F12,0x00B1);
write_cmos_sensor_twobyte(0x6F12,0x0120);
write_cmos_sensor_twobyte(0x6F12,0x2870);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7400);
write_cmos_sensor_twobyte(0x6F12,0x0228);
write_cmos_sensor_twobyte(0x6F12,0x4BD0);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0xA874);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7610);
write_cmos_sensor_twobyte(0x6F12,0x0229);
write_cmos_sensor_twobyte(0x6F12,0x47D0);
write_cmos_sensor_twobyte(0x6F12,0x0021);
write_cmos_sensor_twobyte(0x6F12,0xE974);
write_cmos_sensor_twobyte(0x6F12,0x00B1);
write_cmos_sensor_twobyte(0x6F12,0x0220);
write_cmos_sensor_twobyte(0x6F12,0x1178);
write_cmos_sensor_twobyte(0x6F12,0x0026);
write_cmos_sensor_twobyte(0x6F12,0x01B1);
write_cmos_sensor_twobyte(0x6F12,0x361F);
write_cmos_sensor_twobyte(0x6F12,0x09AA);
write_cmos_sensor_twobyte(0x6F12,0x05A9);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0012);
write_cmos_sensor_twobyte(0x6F12,0xC54F);
write_cmos_sensor_twobyte(0x6F12,0x04F1);
write_cmos_sensor_twobyte(0x6F12,0x5A04);
write_cmos_sensor_twobyte(0x6F12,0xF988);
write_cmos_sensor_twobyte(0x6F12,0xA37E);
write_cmos_sensor_twobyte(0x6F12,0x01EB);
write_cmos_sensor_twobyte(0x6F12,0x0002);
write_cmos_sensor_twobyte(0x6F12,0x3889);
write_cmos_sensor_twobyte(0x6F12,0x00EB);
write_cmos_sensor_twobyte(0x6F12,0x0601);
write_cmos_sensor_twobyte(0x6F12,0xE088);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xBCFA);
write_cmos_sensor_twobyte(0x6F12,0x08A9);
write_cmos_sensor_twobyte(0x6F12,0x04A8);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x6089);
write_cmos_sensor_twobyte(0x6F12,0x14F8);
write_cmos_sensor_twobyte(0x6F12,0x3E1C);
write_cmos_sensor_twobyte(0x6F12,0xA37E);
write_cmos_sensor_twobyte(0x6F12,0x0144);
write_cmos_sensor_twobyte(0x6F12,0xB889);
write_cmos_sensor_twobyte(0x6F12,0x491E);
write_cmos_sensor_twobyte(0x6F12,0xFA88);
write_cmos_sensor_twobyte(0x6F12,0x3044);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xADFA);
write_cmos_sensor_twobyte(0x6F12,0x07A9);
write_cmos_sensor_twobyte(0x6F12,0x03A8);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x237F);
write_cmos_sensor_twobyte(0x6F12,0x3A88);
write_cmos_sensor_twobyte(0x6F12,0x7989);
write_cmos_sensor_twobyte(0x6F12,0x2089);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xA3FA);
write_cmos_sensor_twobyte(0x6F12,0x06A9);
write_cmos_sensor_twobyte(0x6F12,0x02A8);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0xA089);
write_cmos_sensor_twobyte(0x6F12,0x14F8);
write_cmos_sensor_twobyte(0x6F12,0x3C1C);
write_cmos_sensor_twobyte(0x6F12,0x237F);
write_cmos_sensor_twobyte(0x6F12,0x0144);
write_cmos_sensor_twobyte(0x6F12,0x491E);
write_cmos_sensor_twobyte(0x6F12,0x3A88);
write_cmos_sensor_twobyte(0x6F12,0xF889);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x95FA);
write_cmos_sensor_twobyte(0x6F12,0xA07F);
write_cmos_sensor_twobyte(0x6F12,0xA4F1);
write_cmos_sensor_twobyte(0x6F12,0x5A04);
write_cmos_sensor_twobyte(0x6F12,0x28B1);
write_cmos_sensor_twobyte(0x6F12,0x0498);
write_cmos_sensor_twobyte(0x6F12,0x04E0);
write_cmos_sensor_twobyte(0x6F12,0x0120);
write_cmos_sensor_twobyte(0x6F12,0xB2E7);
write_cmos_sensor_twobyte(0x6F12,0x0121);
write_cmos_sensor_twobyte(0x6F12,0xB6E7);
write_cmos_sensor_twobyte(0x6F12,0x0598);
write_cmos_sensor_twobyte(0x6F12,0x6880);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7900);
write_cmos_sensor_twobyte(0x6F12,0x08B1);
write_cmos_sensor_twobyte(0x6F12,0x0298);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x0398);
write_cmos_sensor_twobyte(0x6F12,0xA880);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6C00);
write_cmos_sensor_twobyte(0x6F12,0x0599);
write_cmos_sensor_twobyte(0x6F12,0x401A);
write_cmos_sensor_twobyte(0x6F12,0x0499);
write_cmos_sensor_twobyte(0x6F12,0x401A);
write_cmos_sensor_twobyte(0x6F12,0x0028);
write_cmos_sensor_twobyte(0x6F12,0x00DC);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0xE881);
write_cmos_sensor_twobyte(0x6F12,0xB4F8);
write_cmos_sensor_twobyte(0x6F12,0x6E00);
write_cmos_sensor_twobyte(0x6F12,0x0399);
write_cmos_sensor_twobyte(0x6F12,0x401A);
write_cmos_sensor_twobyte(0x6F12,0x0299);
write_cmos_sensor_twobyte(0x6F12,0x401A);
write_cmos_sensor_twobyte(0x6F12,0x0028);
write_cmos_sensor_twobyte(0x6F12,0x00DC);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x2882);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0x08B1);
write_cmos_sensor_twobyte(0x6F12,0x0899);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x0999);
write_cmos_sensor_twobyte(0x6F12,0x98F8);
write_cmos_sensor_twobyte(0x6F12,0x1B20);
write_cmos_sensor_twobyte(0x6F12,0x98F8);
write_cmos_sensor_twobyte(0x6F12,0x1930);
write_cmos_sensor_twobyte(0x6F12,0x4046);
write_cmos_sensor_twobyte(0x6F12,0x12FB);
write_cmos_sensor_twobyte(0x6F12,0x03F2);
write_cmos_sensor_twobyte(0x6F12,0x91FB);
write_cmos_sensor_twobyte(0x6F12,0xF2F6);
write_cmos_sensor_twobyte(0x6F12,0x02FB);
write_cmos_sensor_twobyte(0x6F12,0x1611);
write_cmos_sensor_twobyte(0x6F12,0x94F8);
write_cmos_sensor_twobyte(0x6F12,0x7920);
write_cmos_sensor_twobyte(0x6F12,0x0AB1);
write_cmos_sensor_twobyte(0x6F12,0x069A);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x079A);
write_cmos_sensor_twobyte(0x6F12,0x047F);
write_cmos_sensor_twobyte(0x6F12,0x867E);
write_cmos_sensor_twobyte(0x6F12,0x14FB);
write_cmos_sensor_twobyte(0x6F12,0x06F4);
write_cmos_sensor_twobyte(0x6F12,0x91FB);
write_cmos_sensor_twobyte(0x6F12,0xF3F3);
write_cmos_sensor_twobyte(0x6F12,0x92FB);
write_cmos_sensor_twobyte(0x6F12,0xF4F6);
write_cmos_sensor_twobyte(0x6F12,0xEB80);
write_cmos_sensor_twobyte(0x6F12,0x04FB);
write_cmos_sensor_twobyte(0x6F12,0x1622);
write_cmos_sensor_twobyte(0x6F12,0x437E);
write_cmos_sensor_twobyte(0x6F12,0x91FB);
write_cmos_sensor_twobyte(0x6F12,0xF3F4);
write_cmos_sensor_twobyte(0x6F12,0x03FB);
write_cmos_sensor_twobyte(0x6F12,0x1411);
write_cmos_sensor_twobyte(0x6F12,0x6981);
write_cmos_sensor_twobyte(0x6F12,0x817E);
write_cmos_sensor_twobyte(0x6F12,0x92FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F1);
write_cmos_sensor_twobyte(0x6F12,0x2981);
write_cmos_sensor_twobyte(0x6F12,0x98F8);
write_cmos_sensor_twobyte(0x6F12,0x1A00);
write_cmos_sensor_twobyte(0x6F12,0x92FB);
write_cmos_sensor_twobyte(0x6F12,0xF0F1);
write_cmos_sensor_twobyte(0x6F12,0x00FB);
write_cmos_sensor_twobyte(0x6F12,0x1120);
write_cmos_sensor_twobyte(0x6F12,0xA881);
write_cmos_sensor_twobyte(0x6F12,0x0AB0);
write_cmos_sensor_twobyte(0x6F12,0xBDE8);
write_cmos_sensor_twobyte(0x6F12,0xF081);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF34F);
write_cmos_sensor_twobyte(0x6F12,0x7D4E);
write_cmos_sensor_twobyte(0x6F12,0x7D4B);
write_cmos_sensor_twobyte(0x6F12,0x81B0);
write_cmos_sensor_twobyte(0x6F12,0x0021);
write_cmos_sensor_twobyte(0x6F12,0x8A5B);
write_cmos_sensor_twobyte(0x6F12,0x03EB);
write_cmos_sensor_twobyte(0x6F12,0x4104);
write_cmos_sensor_twobyte(0x6F12,0xD5B2);
write_cmos_sensor_twobyte(0x6F12,0x120A);
write_cmos_sensor_twobyte(0x6F12,0xA4F8);
write_cmos_sensor_twobyte(0x6F12,0x8451);
write_cmos_sensor_twobyte(0x6F12,0x891C);
write_cmos_sensor_twobyte(0x6F12,0xA4F8);
write_cmos_sensor_twobyte(0x6F12,0x8621);
write_cmos_sensor_twobyte(0x6F12,0x0829);
write_cmos_sensor_twobyte(0x6F12,0xF3D3);
write_cmos_sensor_twobyte(0x6F12,0x4FF4);
write_cmos_sensor_twobyte(0x6F12,0x8011);
write_cmos_sensor_twobyte(0x6F12,0xB1FB);
write_cmos_sensor_twobyte(0x6F12,0xF0FE);
write_cmos_sensor_twobyte(0x6F12,0x7149);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAEF5);
write_cmos_sensor_twobyte(0x6F12,0x807B);
write_cmos_sensor_twobyte(0x6F12,0xB1F8);
write_cmos_sensor_twobyte(0x6F12,0x8812);
write_cmos_sensor_twobyte(0x6F12,0xA0F5);
write_cmos_sensor_twobyte(0x6F12,0x8078);
write_cmos_sensor_twobyte(0x6F12,0x0091);
write_cmos_sensor_twobyte(0x6F12,0x6F4D);
write_cmos_sensor_twobyte(0x6F12,0x0024);
write_cmos_sensor_twobyte(0x6F12,0x05EB);
write_cmos_sensor_twobyte(0x6F12,0x8203);
write_cmos_sensor_twobyte(0x6F12,0x05EB);
write_cmos_sensor_twobyte(0x6F12,0x420A);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0xA441);
write_cmos_sensor_twobyte(0x6F12,0xAAF8);
write_cmos_sensor_twobyte(0x6F12,0x9441);
write_cmos_sensor_twobyte(0x6F12,0x674C);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x0F01);
write_cmos_sensor_twobyte(0x6F12,0x04EB);
write_cmos_sensor_twobyte(0x6F12,0x4205);
write_cmos_sensor_twobyte(0x6F12,0x05F5);
write_cmos_sensor_twobyte(0x6F12,0x2775);
write_cmos_sensor_twobyte(0x6F12,0x2F8C);
write_cmos_sensor_twobyte(0x6F12,0x2C88);
write_cmos_sensor_twobyte(0x6F12,0xA7EB);
write_cmos_sensor_twobyte(0x6F12,0x0406);
write_cmos_sensor_twobyte(0x6F12,0x06FB);
write_cmos_sensor_twobyte(0x6F12,0x08F6);
write_cmos_sensor_twobyte(0x6F12,0x96FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F6);
write_cmos_sensor_twobyte(0x6F12,0x04EB);
write_cmos_sensor_twobyte(0x6F12,0x2624);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0x2441);
write_cmos_sensor_twobyte(0x6F12,0x2D8A);
write_cmos_sensor_twobyte(0x6F12,0xA7EB);
write_cmos_sensor_twobyte(0x6F12,0x050C);
write_cmos_sensor_twobyte(0x6F12,0x0BFB);
write_cmos_sensor_twobyte(0x6F12,0x0CF6);
write_cmos_sensor_twobyte(0x6F12,0x96FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F6);
write_cmos_sensor_twobyte(0x6F12,0xA7EB);
write_cmos_sensor_twobyte(0x6F12,0x2629);
write_cmos_sensor_twobyte(0x6F12,0x0CFB);
write_cmos_sensor_twobyte(0x6F12,0x08FC);
write_cmos_sensor_twobyte(0x6F12,0x9CFB);
write_cmos_sensor_twobyte(0x6F12,0xF1F6);
write_cmos_sensor_twobyte(0x6F12,0x05EB);
write_cmos_sensor_twobyte(0x6F12,0x2625);
write_cmos_sensor_twobyte(0x6F12,0x4FF0);
write_cmos_sensor_twobyte(0x6F12,0x1006);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0x6491);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0x4451);
write_cmos_sensor_twobyte(0x6F12,0xB6EB);
write_cmos_sensor_twobyte(0x6F12,0x102F);
write_cmos_sensor_twobyte(0x6F12,0x03D8);
write_cmos_sensor_twobyte(0x6F12,0x3C46);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0xA471);
write_cmos_sensor_twobyte(0x6F12,0x19E0);
write_cmos_sensor_twobyte(0x6F12,0x042A);
write_cmos_sensor_twobyte(0x6F12,0x0BD2);
write_cmos_sensor_twobyte(0x6F12,0x0299);
write_cmos_sensor_twobyte(0x6F12,0x4FF4);
write_cmos_sensor_twobyte(0x6F12,0x8016);
write_cmos_sensor_twobyte(0x6F12,0xB6FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F6);
write_cmos_sensor_twobyte(0x6F12,0xA6EB);
write_cmos_sensor_twobyte(0x6F12,0x0E06);
write_cmos_sensor_twobyte(0x6F12,0x651B);
write_cmos_sensor_twobyte(0x6F12,0x6E43);
write_cmos_sensor_twobyte(0x6F12,0x96FB);
write_cmos_sensor_twobyte(0x6F12,0xFBF5);
write_cmos_sensor_twobyte(0x6F12,0x08E0);
write_cmos_sensor_twobyte(0x6F12,0x0299);
write_cmos_sensor_twobyte(0x6F12,0xA9EB);
write_cmos_sensor_twobyte(0x6F12,0x0405);
write_cmos_sensor_twobyte(0x6F12,0x0E1A);
write_cmos_sensor_twobyte(0x6F12,0x7543);
write_cmos_sensor_twobyte(0x6F12,0xC0F5);
write_cmos_sensor_twobyte(0x6F12,0x8056);
write_cmos_sensor_twobyte(0x6F12,0x95FB);
write_cmos_sensor_twobyte(0x6F12,0xF6F5);
write_cmos_sensor_twobyte(0x6F12,0x2C44);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0xA441);
write_cmos_sensor_twobyte(0x6F12,0x0099);
write_cmos_sensor_twobyte(0x6F12,0x521C);
write_cmos_sensor_twobyte(0x6F12,0xA4EB);
write_cmos_sensor_twobyte(0x6F12,0x0124);
write_cmos_sensor_twobyte(0x6F12,0xC3F8);
write_cmos_sensor_twobyte(0x6F12,0xA441);
write_cmos_sensor_twobyte(0x6F12,0xBAF8);
write_cmos_sensor_twobyte(0x6F12,0x8431);
write_cmos_sensor_twobyte(0x6F12,0x6343);
write_cmos_sensor_twobyte(0x6F12,0xC4EB);
write_cmos_sensor_twobyte(0x6F12,0xE311);
write_cmos_sensor_twobyte(0x6F12,0xAAF8);
write_cmos_sensor_twobyte(0x6F12,0x9411);
write_cmos_sensor_twobyte(0x6F12,0x082A);
write_cmos_sensor_twobyte(0x6F12,0x9DD3);
write_cmos_sensor_twobyte(0x6F12,0xBDE8);
write_cmos_sensor_twobyte(0x6F12,0xFE8F);
write_cmos_sensor_twobyte(0x6F12,0x0180);
write_cmos_sensor_twobyte(0x6F12,0x7047);
write_cmos_sensor_twobyte(0x6F12,0x0160);
write_cmos_sensor_twobyte(0x6F12,0x7047);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xF05F);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0xEC90);
write_cmos_sensor_twobyte(0x6F12,0x0446);
write_cmos_sensor_twobyte(0x6F12,0x0E46);
write_cmos_sensor_twobyte(0x6F12,0x1546);
write_cmos_sensor_twobyte(0x6F12,0x4888);
write_cmos_sensor_twobyte(0x6F12,0x99F8);
write_cmos_sensor_twobyte(0x6F12,0x8B20);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0xE0A0);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0xC4B0);
write_cmos_sensor_twobyte(0x6F12,0x0988);
write_cmos_sensor_twobyte(0x6F12,0x1F46);
write_cmos_sensor_twobyte(0x6F12,0xF2B3);
write_cmos_sensor_twobyte(0x6F12,0x6143);
write_cmos_sensor_twobyte(0x6F12,0x01EB);
write_cmos_sensor_twobyte(0x6F12,0x9003);
write_cmos_sensor_twobyte(0x6F12,0xDAF8);
write_cmos_sensor_twobyte(0x6F12,0x7400);
write_cmos_sensor_twobyte(0x6F12,0xD046);
write_cmos_sensor_twobyte(0x6F12,0x9842);
write_cmos_sensor_twobyte(0x6F12,0x0BD9);
write_cmos_sensor_twobyte(0x6F12,0xB0FB);
write_cmos_sensor_twobyte(0x6F12,0xF4F0);
write_cmos_sensor_twobyte(0x6F12,0x80B2);
write_cmos_sensor_twobyte(0x6F12,0x3080);
write_cmos_sensor_twobyte(0x6F12,0xD8F8);
write_cmos_sensor_twobyte(0x6F12,0x7410);
write_cmos_sensor_twobyte(0x6F12,0x00FB);
write_cmos_sensor_twobyte(0x6F12,0x1410);
write_cmos_sensor_twobyte(0x6F12,0x8000);
write_cmos_sensor_twobyte(0x6F12,0x7080);
write_cmos_sensor_twobyte(0x6F12,0xD8F8);
write_cmos_sensor_twobyte(0x6F12,0x7430);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0x7A88);
write_cmos_sensor_twobyte(0x6F12,0x181A);
write_cmos_sensor_twobyte(0x6F12,0xB0FB);
write_cmos_sensor_twobyte(0x6F12,0xF4F1);
write_cmos_sensor_twobyte(0x6F12,0x01FB);
write_cmos_sensor_twobyte(0x6F12,0x1400);
write_cmos_sensor_twobyte(0x6F12,0x2AB3);
write_cmos_sensor_twobyte(0x6F12,0x8242);
write_cmos_sensor_twobyte(0x6F12,0x00D9);
write_cmos_sensor_twobyte(0x6F12,0x491E);
write_cmos_sensor_twobyte(0x6F12,0x1046);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x0120);
write_cmos_sensor_twobyte(0x6F12,0x5E46);
write_cmos_sensor_twobyte(0x6F12,0x72B1);
write_cmos_sensor_twobyte(0x6F12,0xB6F8);
write_cmos_sensor_twobyte(0x6F12,0x04C0);
write_cmos_sensor_twobyte(0x6F12,0x8C45);
write_cmos_sensor_twobyte(0x6F12,0x0AD8);
write_cmos_sensor_twobyte(0x6F12,0xB6F8);
write_cmos_sensor_twobyte(0x6F12,0x06C0);
write_cmos_sensor_twobyte(0x6F12,0x8C45);
write_cmos_sensor_twobyte(0x6F12,0x06D3);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x0260);
write_cmos_sensor_twobyte(0x6F12,0x891B);
write_cmos_sensor_twobyte(0x6F12,0xB1FB);
write_cmos_sensor_twobyte(0x6F12,0xF2F1);
write_cmos_sensor_twobyte(0x6F12,0x01FB);
write_cmos_sensor_twobyte(0x6F12,0x0261);
write_cmos_sensor_twobyte(0x6F12,0x3980);
write_cmos_sensor_twobyte(0x6F12,0xBAF8);
write_cmos_sensor_twobyte(0x6F12,0x7820);
write_cmos_sensor_twobyte(0x6F12,0x01FB);
write_cmos_sensor_twobyte(0x6F12,0x0421);
write_cmos_sensor_twobyte(0x6F12,0x0144);
write_cmos_sensor_twobyte(0x6F12,0x2960);
write_cmos_sensor_twobyte(0x6F12,0x99F8);
write_cmos_sensor_twobyte(0x6F12,0x2800);
write_cmos_sensor_twobyte(0x6F12,0xF8B1);
write_cmos_sensor_twobyte(0x6F12,0x00E0);
write_cmos_sensor_twobyte(0x6F12,0x34E0);
write_cmos_sensor_twobyte(0x6F12,0x0C22);
write_cmos_sensor_twobyte(0x6F12,0x1846);
write_cmos_sensor_twobyte(0x6F12,0x72E0);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x7020);
write_cmos_sensor_twobyte(0x6F12,0x8242);
write_cmos_sensor_twobyte(0x6F12,0x01D9);
write_cmos_sensor_twobyte(0x6F12,0x491E);
write_cmos_sensor_twobyte(0x6F12,0x601E);
write_cmos_sensor_twobyte(0x6F12,0x98F8);
write_cmos_sensor_twobyte(0x6F12,0x6E20);
write_cmos_sensor_twobyte(0x6F12,0x02B3);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x6820);
write_cmos_sensor_twobyte(0x6F12,0x8242);
write_cmos_sensor_twobyte(0x6F12,0x0BD2);
write_cmos_sensor_twobyte(0x6F12,0x98F8);
write_cmos_sensor_twobyte(0x6F12,0x6460);
write_cmos_sensor_twobyte(0x6F12,0x3EB1);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x6A60);
write_cmos_sensor_twobyte(0x6F12,0x8642);
write_cmos_sensor_twobyte(0x6F12,0x03D8);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x6C20);
write_cmos_sensor_twobyte(0x6F12,0x8242);
write_cmos_sensor_twobyte(0x6F12,0x00D2);
write_cmos_sensor_twobyte(0x6F12,0x1046);
write_cmos_sensor_twobyte(0x6F12,0x7880);
write_cmos_sensor_twobyte(0x6F12,0xC2E7);
write_cmos_sensor_twobyte(0x6F12,0x10E0);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0xAB00);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0x14B0);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xC1BE);
write_cmos_sensor_twobyte(0x6F12,0x4000);
write_cmos_sensor_twobyte(0x6F12,0x9526);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xCC70);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0x0670);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xC5C0);
write_cmos_sensor_twobyte(0x6F12,0xB8F8);
write_cmos_sensor_twobyte(0x6F12,0x6220);
write_cmos_sensor_twobyte(0x6F12,0xE8E7);
write_cmos_sensor_twobyte(0x6F12,0x4FF4);
write_cmos_sensor_twobyte(0x6F12,0x8050);
write_cmos_sensor_twobyte(0x6F12,0x6860);
write_cmos_sensor_twobyte(0x6F12,0xBDE8);
write_cmos_sensor_twobyte(0x6F12,0xF09F);
write_cmos_sensor_twobyte(0x6F12,0x99F8);
write_cmos_sensor_twobyte(0x6F12,0x8C20);
write_cmos_sensor_twobyte(0x6F12,0xB0FB);
write_cmos_sensor_twobyte(0x6F12,0xF2F0);
write_cmos_sensor_twobyte(0x6F12,0x01FB);
write_cmos_sensor_twobyte(0x6F12,0x0408);
write_cmos_sensor_twobyte(0x6F12,0xB9F8);
write_cmos_sensor_twobyte(0x6F12,0x2400);
write_cmos_sensor_twobyte(0x6F12,0x401C);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x25F9);
write_cmos_sensor_twobyte(0x6F12,0x7288);
write_cmos_sensor_twobyte(0x6F12,0x99F8);
write_cmos_sensor_twobyte(0x6F12,0x8C10);
write_cmos_sensor_twobyte(0x6F12,0xB2FB);
write_cmos_sensor_twobyte(0x6F12,0xF1F1);
write_cmos_sensor_twobyte(0x6F12,0x99F8);
write_cmos_sensor_twobyte(0x6F12,0x8D20);
write_cmos_sensor_twobyte(0x6F12,0x32B1);
write_cmos_sensor_twobyte(0x6F12,0xB9F8);
write_cmos_sensor_twobyte(0x6F12,0x2420);
write_cmos_sensor_twobyte(0x6F12,0x3388);
write_cmos_sensor_twobyte(0x6F12,0x521C);
write_cmos_sensor_twobyte(0x6F12,0x9342);
write_cmos_sensor_twobyte(0x6F12,0x00D2);
write_cmos_sensor_twobyte(0x6F12,0x0021);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x0120);
write_cmos_sensor_twobyte(0x6F12,0x5B46);
write_cmos_sensor_twobyte(0x6F12,0x5AB1);
write_cmos_sensor_twobyte(0x6F12,0x9E88);
write_cmos_sensor_twobyte(0x6F12,0x8642);
write_cmos_sensor_twobyte(0x6F12,0x08D8);
write_cmos_sensor_twobyte(0x6F12,0xDE88);
write_cmos_sensor_twobyte(0x6F12,0x8642);
write_cmos_sensor_twobyte(0x6F12,0x05D3);
write_cmos_sensor_twobyte(0x6F12,0x9E78);
write_cmos_sensor_twobyte(0x6F12,0x801B);
write_cmos_sensor_twobyte(0x6F12,0xB0FB);
write_cmos_sensor_twobyte(0x6F12,0xF2F0);
write_cmos_sensor_twobyte(0x6F12,0x00FB);
write_cmos_sensor_twobyte(0x6F12,0x0260);
write_cmos_sensor_twobyte(0x6F12,0x3880);
write_cmos_sensor_twobyte(0x6F12,0x7980);
write_cmos_sensor_twobyte(0x6F12,0xBAF8);
write_cmos_sensor_twobyte(0x6F12,0x7860);
write_cmos_sensor_twobyte(0x6F12,0x00FB);
write_cmos_sensor_twobyte(0x6F12,0x0460);
write_cmos_sensor_twobyte(0x6F12,0x0144);
write_cmos_sensor_twobyte(0x6F12,0x4FF4);
write_cmos_sensor_twobyte(0x6F12,0x8050);
write_cmos_sensor_twobyte(0x6F12,0xC5E9);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x5878);
write_cmos_sensor_twobyte(0x6F12,0x0028);
write_cmos_sensor_twobyte(0x6F12,0xC5D0);
write_cmos_sensor_twobyte(0x6F12,0x9BF8);
write_cmos_sensor_twobyte(0x6F12,0x0300);
write_cmos_sensor_twobyte(0x6F12,0x0128);
write_cmos_sensor_twobyte(0x6F12,0x02D0);
write_cmos_sensor_twobyte(0x6F12,0xBAF8);
write_cmos_sensor_twobyte(0x6F12,0x7800);
write_cmos_sensor_twobyte(0x6F12,0x091A);
write_cmos_sensor_twobyte(0x6F12,0x0C22);
write_cmos_sensor_twobyte(0x6F12,0x4046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xF2F8);
write_cmos_sensor_twobyte(0x6F12,0xB8E7);
write_cmos_sensor_twobyte(0x6F12,0x38B5);
write_cmos_sensor_twobyte(0x6F12,0x0446);
write_cmos_sensor_twobyte(0x6F12,0x0122);
write_cmos_sensor_twobyte(0x6F12,0x6946);
write_cmos_sensor_twobyte(0x6F12,0x4FF6);
write_cmos_sensor_twobyte(0x6F12,0xFD70);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xEEF8);
write_cmos_sensor_twobyte(0x6F12,0x9DF8);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x18B9);
write_cmos_sensor_twobyte(0x6F12,0x6148);
write_cmos_sensor_twobyte(0x6F12,0x007A);
write_cmos_sensor_twobyte(0x6F12,0x0128);
write_cmos_sensor_twobyte(0x6F12,0x0AD0);
write_cmos_sensor_twobyte(0x6F12,0x604D);
write_cmos_sensor_twobyte(0x6F12,0x2888);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xE8F8);
write_cmos_sensor_twobyte(0x6F12,0x2046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xEAF8);
write_cmos_sensor_twobyte(0x6F12,0x2888);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xECF8);
write_cmos_sensor_twobyte(0x6F12,0x38BD);
write_cmos_sensor_twobyte(0x6F12,0x5B49);
write_cmos_sensor_twobyte(0x6F12,0xFA20);
write_cmos_sensor_twobyte(0x6F12,0x0968);
write_cmos_sensor_twobyte(0x6F12,0x0880);
write_cmos_sensor_twobyte(0x6F12,0xFEE7);
write_cmos_sensor_twobyte(0x6F12,0x70B5);
write_cmos_sensor_twobyte(0x6F12,0x0446);
write_cmos_sensor_twobyte(0x6F12,0xB0FA);
write_cmos_sensor_twobyte(0x6F12,0x80F0);
write_cmos_sensor_twobyte(0x6F12,0x0D46);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0x0821);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0xDFF8);
write_cmos_sensor_twobyte(0x6F12,0x8440);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0xC540);
write_cmos_sensor_twobyte(0x6F12,0xB4FB);
write_cmos_sensor_twobyte(0x6F12,0xF5F0);
write_cmos_sensor_twobyte(0x6F12,0x05FB);
write_cmos_sensor_twobyte(0x6F12,0x1041);
write_cmos_sensor_twobyte(0x6F12,0xB5EB);
write_cmos_sensor_twobyte(0x6F12,0x410F);
write_cmos_sensor_twobyte(0x6F12,0x00D2);
write_cmos_sensor_twobyte(0x6F12,0x401C);
write_cmos_sensor_twobyte(0x6F12,0x70BD);
write_cmos_sensor_twobyte(0x6F12,0x2DE9);
write_cmos_sensor_twobyte(0x6F12,0xFE4F);
write_cmos_sensor_twobyte(0x6F12,0x0746);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0290);
write_cmos_sensor_twobyte(0x6F12,0x0C46);
write_cmos_sensor_twobyte(0x6F12,0x081D);
write_cmos_sensor_twobyte(0x6F12,0xC030);
write_cmos_sensor_twobyte(0x6F12,0x00BF);
write_cmos_sensor_twobyte(0x6F12,0x8246);
write_cmos_sensor_twobyte(0x6F12,0x201D);
write_cmos_sensor_twobyte(0x6F12,0xEC30);
write_cmos_sensor_twobyte(0x6F12,0x00BF);
write_cmos_sensor_twobyte(0x6F12,0xD0E9);
write_cmos_sensor_twobyte(0x6F12,0x0013);
write_cmos_sensor_twobyte(0x6F12,0x5943);
write_cmos_sensor_twobyte(0x6F12,0xD0E9);
write_cmos_sensor_twobyte(0x6F12,0x0236);
write_cmos_sensor_twobyte(0x6F12,0x090A);
write_cmos_sensor_twobyte(0x6F12,0x5943);
write_cmos_sensor_twobyte(0x6F12,0x4FEA);
write_cmos_sensor_twobyte(0x6F12,0x1129);
write_cmos_sensor_twobyte(0x6F12,0xD0E9);
write_cmos_sensor_twobyte(0x6F12,0x0413);
write_cmos_sensor_twobyte(0x6F12,0x1830);
write_cmos_sensor_twobyte(0x6F12,0x5943);
write_cmos_sensor_twobyte(0x6F12,0x21C8);
write_cmos_sensor_twobyte(0x6F12,0x090A);
write_cmos_sensor_twobyte(0x6F12,0x4143);
write_cmos_sensor_twobyte(0x6F12,0x4FEA);
write_cmos_sensor_twobyte(0x6F12,0x1128);
write_cmos_sensor_twobyte(0x6F12,0x04F5);
write_cmos_sensor_twobyte(0x6F12,0xF66B);
write_cmos_sensor_twobyte(0x6F12,0x4946);
write_cmos_sensor_twobyte(0x6F12,0x4046);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x8FF8);
write_cmos_sensor_twobyte(0x6F12,0xB0FA);
write_cmos_sensor_twobyte(0x6F12,0x80F0);
write_cmos_sensor_twobyte(0x6F12,0xB5FA);
write_cmos_sensor_twobyte(0x6F12,0x85F1);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0xC1F1);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x0844);
write_cmos_sensor_twobyte(0x6F12,0x0021);
write_cmos_sensor_twobyte(0x6F12,0x2038);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x82F8);
write_cmos_sensor_twobyte(0x6F12,0x0828);
write_cmos_sensor_twobyte(0x6F12,0x26FA);
write_cmos_sensor_twobyte(0x6F12,0x00F6);
write_cmos_sensor_twobyte(0x6F12,0x06FB);
write_cmos_sensor_twobyte(0x6F12,0x09F6);
write_cmos_sensor_twobyte(0x6F12,0x07DC);
write_cmos_sensor_twobyte(0x6F12,0xC0F1);
write_cmos_sensor_twobyte(0x6F12,0x0801);
write_cmos_sensor_twobyte(0x6F12,0xCE40);
write_cmos_sensor_twobyte(0x6F12,0xC540);
write_cmos_sensor_twobyte(0x6F12,0x05FB);
write_cmos_sensor_twobyte(0x6F12,0x08F5);
write_cmos_sensor_twobyte(0x6F12,0xCD40);
write_cmos_sensor_twobyte(0x6F12,0x06E0);
write_cmos_sensor_twobyte(0x6F12,0xA0F1);
write_cmos_sensor_twobyte(0x6F12,0x0801);
write_cmos_sensor_twobyte(0x6F12,0x8E40);
write_cmos_sensor_twobyte(0x6F12,0xC540);
write_cmos_sensor_twobyte(0x6F12,0x05FB);
write_cmos_sensor_twobyte(0x6F12,0x08F5);
write_cmos_sensor_twobyte(0x6F12,0x8D40);
write_cmos_sensor_twobyte(0x6F12,0xCDE9);
write_cmos_sensor_twobyte(0x6F12,0x0065);
write_cmos_sensor_twobyte(0x6F12,0xF868);
write_cmos_sensor_twobyte(0x6F12,0x5A46);
write_cmos_sensor_twobyte(0x6F12,0x0168);
write_cmos_sensor_twobyte(0x6F12,0x8B68);
write_cmos_sensor_twobyte(0x6F12,0x6946);
write_cmos_sensor_twobyte(0x6F12,0x9847);
write_cmos_sensor_twobyte(0x6F12,0x04F2);
write_cmos_sensor_twobyte(0x6F12,0xA474);
write_cmos_sensor_twobyte(0x6F12,0xDAF8);
write_cmos_sensor_twobyte(0x6F12,0x1810);
write_cmos_sensor_twobyte(0x6F12,0xA069);
write_cmos_sensor_twobyte(0x6F12,0xFFF7);
write_cmos_sensor_twobyte(0x6F12,0x91FF);
write_cmos_sensor_twobyte(0x6F12,0x2060);
write_cmos_sensor_twobyte(0x6F12,0xE069);
write_cmos_sensor_twobyte(0x6F12,0xDAF8);
write_cmos_sensor_twobyte(0x6F12,0x1810);
write_cmos_sensor_twobyte(0x6F12,0xFFF7);
write_cmos_sensor_twobyte(0x6F12,0x8BFF);
write_cmos_sensor_twobyte(0x6F12,0x6060);
write_cmos_sensor_twobyte(0x6F12,0xD7F8);
write_cmos_sensor_twobyte(0x6F12,0x3801);
write_cmos_sensor_twobyte(0x6F12,0x40F4);
write_cmos_sensor_twobyte(0x6F12,0x0050);
write_cmos_sensor_twobyte(0x6F12,0xC7F8);
write_cmos_sensor_twobyte(0x6F12,0x3801);
write_cmos_sensor_twobyte(0x6F12,0x0298);
write_cmos_sensor_twobyte(0x6F12,0x92E6);
write_cmos_sensor_twobyte(0x6F12,0x10B5);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0xF351);
write_cmos_sensor_twobyte(0x6F12,0x1948);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x6BF8);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0xD751);
write_cmos_sensor_twobyte(0x6F12,0x1748);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x65F8);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x6B51);
write_cmos_sensor_twobyte(0x6F12,0x1548);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x5FF8);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x0541);
write_cmos_sensor_twobyte(0x6F12,0x1348);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x59F8);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x0331);
write_cmos_sensor_twobyte(0x6F12,0x1148);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x53F8);
write_cmos_sensor_twobyte(0x6F12,0x0022);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x7B11);
write_cmos_sensor_twobyte(0x6F12,0x0F48);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x4DF8);
write_cmos_sensor_twobyte(0x6F12,0x064C);
write_cmos_sensor_twobyte(0x6F12,0x0122);
write_cmos_sensor_twobyte(0x6F12,0x2080);
write_cmos_sensor_twobyte(0x6F12,0x0D48);
write_cmos_sensor_twobyte(0x6F12,0x0068);
write_cmos_sensor_twobyte(0x6F12,0xAFF2);
write_cmos_sensor_twobyte(0x6F12,0x1F11);
write_cmos_sensor_twobyte(0x6F12,0x00F0);
write_cmos_sensor_twobyte(0x6F12,0x44F8);
write_cmos_sensor_twobyte(0x6F12,0x6060);
write_cmos_sensor_twobyte(0x6F12,0x10BD);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0xAB00);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x54E0);
write_cmos_sensor_twobyte(0x6F12,0x2000);
write_cmos_sensor_twobyte(0x6F12,0x05D0);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x6F0F);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0xC54D);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0xC59D);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0xB5AD);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x72AD);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x3909);
write_cmos_sensor_twobyte(0x6F12,0x2001);
write_cmos_sensor_twobyte(0x6F12,0x54C0);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0x754C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x4CF2);
write_cmos_sensor_twobyte(0x6F12,0x4D5C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x41F6);
write_cmos_sensor_twobyte(0x6F12,0x8F7C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x41F6);
write_cmos_sensor_twobyte(0x6F12,0x497C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x41F2);
write_cmos_sensor_twobyte(0x6F12,0x757C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0xE12C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x43F6);
write_cmos_sensor_twobyte(0x6F12,0x091C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0xF12C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x41F6);
write_cmos_sensor_twobyte(0x6F12,0x877C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x40F2);
write_cmos_sensor_twobyte(0x6F12,0x013C);
write_cmos_sensor_twobyte(0x6F12,0xC0F2);
write_cmos_sensor_twobyte(0x6F12,0x000C);
write_cmos_sensor_twobyte(0x6F12,0x6047);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x058F);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x2188);
write_cmos_sensor_twobyte(0x6F12,0x07D6);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x005F);

//                                       
// Warning: T&P Parameters Info Unavailable
//                                       
// End T&P part                          
                                         
////////////////////////////////////////////////////////////
//////Analog Setting Start 
////////////////////////////////////////////////////////////

//// ADLC setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x177C);
write_cmos_sensor_twobyte(0x6F12,0x0000);

//// Add mixer setting @ 20150111
write_cmos_sensor_twobyte(0x602A,0x19FA);
write_cmos_sensor_twobyte(0x6F12,0x0101);
write_cmos_sensor_twobyte(0x602A,0x19FC);
write_cmos_sensor_twobyte(0x6F12,0x0000);

//// Add dadlc setting @ 20150118 , pbc
write_cmos_sensor_twobyte(0x602A,0x1718);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

//// Add WDR exposure setting @ 20150118 , pbc
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0xF4FC,0x4D17);	// EVT1.1

//// Clock setting, related with ATOP
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E24);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0E64);
write_cmos_sensor_twobyte(0x6F12,0x004A);	// DBR freq : 100MHz -> 74MHz EVT1.1 0504

//// SHBN setting
write_cmos_sensor_twobyte(0x602A,0x0EB0);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x602A,0x0EB2);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);

write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0xF442);	// DBUS option (No bypass)

//////////////////////////////
//////////////////////////////

//// Main Setting
write_cmos_sensor_twobyte(0x602A,0x0E8A);
write_cmos_sensor_twobyte(0x6F12,0xFDE9);	// Bias Sampling EN & FDB Off &dabx on, [10] FX_EN (F40A)
write_cmos_sensor_twobyte(0x602A,0x0E88);
write_cmos_sensor_twobyte(0x6F12,0x3EB8);	// F406 address

//// ATOP Setting (Option)
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0xF440,0x0000);
write_cmos_sensor_twobyte(0xF4AA,0x0040);	
write_cmos_sensor_twobyte(0xF486,0x0000);
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0xF442,0x0000);
write_cmos_sensor_twobyte(0x6028,0x2000);

//// DBUS setting
write_cmos_sensor_twobyte(0x602A,0x14FE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// DBUS column offset off
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

//// Ramp Setting
write_cmos_sensor_twobyte(0x602A,0x0E36);
write_cmos_sensor_twobyte(0x6F12,0x0006);
write_cmos_sensor_twobyte(0x602A,0x0E38);
write_cmos_sensor_twobyte(0x6F12,0x0606);
write_cmos_sensor_twobyte(0x602A,0x0E3A);
write_cmos_sensor_twobyte(0x6F12,0x060F);
write_cmos_sensor_twobyte(0x602A,0x0E3C);
write_cmos_sensor_twobyte(0x6F12,0x0F0F);
write_cmos_sensor_twobyte(0x602A,0x0E3E);
write_cmos_sensor_twobyte(0x6F12,0x0F30);
write_cmos_sensor_twobyte(0x602A,0x0E40);
write_cmos_sensor_twobyte(0x6F12,0x3030);
write_cmos_sensor_twobyte(0x602A,0x0E42);
write_cmos_sensor_twobyte(0x6F12,0x3030);
write_cmos_sensor_twobyte(0x602A,0x0E46);
write_cmos_sensor_twobyte(0x6F12,0x0301);

write_cmos_sensor_twobyte(0x602A,0x0706);
write_cmos_sensor_twobyte(0x6F12,0x01C0);
write_cmos_sensor_twobyte(0x6F12,0x01C0);
write_cmos_sensor_twobyte(0x6F12,0x01C0);
write_cmos_sensor_twobyte(0x6F12,0x01C0);

//// APS setting
// WRITE 4000F4AC 005A  // ADCsat 720mV // 20150113 KTY
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0xF4AC,0x0062);

/////////ADC Timing is updated below(20150102)
/////////////Type
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0CF4);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0D26);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0D28);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0E80);
write_cmos_sensor_twobyte(0x6F12,0x0100);

////////////////////////////////////////////////////////////
//////Analog Setting End
////////////////////////////////////////////////////////////
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0xAB00);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0xAB02);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0xAB04);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x3092,0x7E50);
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x1F1A);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x1D5E);
write_cmos_sensor_twobyte(0x6F12,0x0358);	// EVT1.1 0429 pjw
write_cmos_sensor_twobyte(0x6F12,0x0001);

// TG Readout
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x06AC);
write_cmos_sensor_twobyte(0x6F12,0x0100);

// FOB Setting
write_cmos_sensor_twobyte(0x602A,0x06A6);
write_cmos_sensor_twobyte(0x6F12,0x0108);
write_cmos_sensor_twobyte(0x602A,0x06A8);
write_cmos_sensor_twobyte(0x6F12,0x0C01);

// Int.Time
write_cmos_sensor_twobyte(0x602A,0x06FA);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6028,0x4000);

write_cmos_sensor_twobyte(0x021E,0x0400);
write_cmos_sensor_twobyte(0x021C,0x0001);
// Digital Gain
write_cmos_sensor_twobyte(0x020E,0x0100);
write_cmos_sensor_twobyte(0x3074,0x0100);

// PSP BDS/HVbin
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0EFA);
write_cmos_sensor_twobyte(0x6F12,0x0100);

write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x0404,0x0010);

// Debug Path - PSP Bypass
// pjw WRITE 400070F2 0001

/////////////////////////////////////////////////
// PSP Normal (All PSP Blocks Bypass)- YJM
/////////////////////////////////////////////////
// TOP BPC DNS Bypass
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0xB4D6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//[15:8] SpotRemoval_MainDenoise_DenoiseMode, Power_0_NoiseIndex_0[7:0]  SpotRemoval_MainDenoise_DenoiseModePower_0_NoiseIndex_1
write_cmos_sensor_twobyte(0x6F12,0x0000);	//[15:8] SpotRemoval_MainDenoise_DenoiseMode, Power_0_NoiseIndex_2[7:0]  SpotRemoval_MainDenoise_DenoiseModePower_0_NoiseIndex_3
write_cmos_sensor_twobyte(0x6F12,0x0000);	//[15:8] SpotRemoval_MainDenoise_DenoiseMode, Power_0_NoiseIndex_4[7:0]  SpotRemoval_MainDenoise_DenoiseModePower_0_NoiseIndex_5
write_cmos_sensor_twobyte(0x6F12,0x0000);	//[15:8] SpotRemoval_MainDenoise_DenoiseMode, Power_0_NoiseIndex_6[7:0]  SpotRemoval_MainDenoise_DenoiseModePower_1_NoiseIndex_0

write_cmos_sensor_twobyte(0x602A,0x6878);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// Despeckle static enable

write_cmos_sensor_twobyte(0x602A,0x58C0);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// GOS bypass

write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x30E6,0x0000);

write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x7500);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//First Gamma (4T)
// WRITE #thStatBayerTuningParams        0001  // ThStatBayer
write_cmos_sensor_twobyte(0x602A,0x78D0);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// Mixer
write_cmos_sensor_twobyte(0x602A,0x6840);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// Despeckle
write_cmos_sensor_twobyte(0x602A,0x5522);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// GRAS
write_cmos_sensor_twobyte(0x602A,0x7CA0);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// DRC
write_cmos_sensor_twobyte(0x602A,0x6A20);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// all gamma block bypass
write_cmos_sensor_twobyte(0x602A,0x6C70);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// all gamma block bypass
write_cmos_sensor_twobyte(0x602A,0x6EC0);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// all gamma block bypass
write_cmos_sensor_twobyte(0x602A,0x8F30);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// ELG

write_cmos_sensor_twobyte(0x602A,0x1006);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// ThStat
write_cmos_sensor_twobyte(0x602A,0x100E);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// YSum
write_cmos_sensor_twobyte(0x602A,0x0EFE);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// RGBY Hist

write_cmos_sensor_twobyte(0x602A,0x69A0);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// DTP

// Mixer
write_cmos_sensor_twobyte(0x602A,0xA8D8);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//[15:8] N/A, [7:0]  Misc_Bypass_DisableMixer
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x3176,0x0000);
// pjw EVT1.1 WRITE #api_info_config_removePedestalMode 0000

// ISPShift
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x9010);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x9018);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x9020);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x9028);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// WRITE #afXtalkTuningParams_bypass             0000

// PDAF BPC Statics
write_cmos_sensor_twobyte(0x602A,0x90B0);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

write_cmos_sensor_twobyte(0x602A,0x58A0);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// Noise Norm
write_cmos_sensor_twobyte(0x602A,0x6880);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// HQDNS

write_cmos_sensor_twobyte(0x602A,0x93B0);
write_cmos_sensor_twobyte(0x6F12,0x0101);

write_cmos_sensor_twobyte(0x602A,0x91A0);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x51E4);
write_cmos_sensor_twobyte(0x6F12,0x0000);	// pjw
write_cmos_sensor_twobyte(0x602A,0x122B);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// pjw WRITE #senHal_PspWaitLines 19
write_cmos_sensor_twobyte(0x602A,0x0687);
write_cmos_sensor_twobyte(0x6F12,0x0005);
/////////////////////////////////////////////////
// PSP END
/////////////////////////////////////////////////


//Analog -tyk 141231
// pjw write_cmos_sensor_twobyte(0x0216,0x0101);
// pjw WRITE 40000216 0101

write_cmos_sensor_twobyte(0x602A,0x072E);
write_cmos_sensor_twobyte(0x6F12,0x004D);	//aig_ld_ptr0
write_cmos_sensor_twobyte(0x602A,0x0732);
write_cmos_sensor_twobyte(0x6F12,0x004A);	//aig_ld_reg0
write_cmos_sensor_twobyte(0x602A,0x0736);
write_cmos_sensor_twobyte(0x6F12,0x002D);	//aig_sl_ptr0
write_cmos_sensor_twobyte(0x602A,0x073A);
write_cmos_sensor_twobyte(0x6F12,0x000A);	//aig_sl_reg0
write_cmos_sensor_twobyte(0x602A,0x0746);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_rx_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x074A);
write_cmos_sensor_twobyte(0x6F12,0x0012);	//aig_rx_p1_reg0
write_cmos_sensor_twobyte(0x602A,0x073E);
write_cmos_sensor_twobyte(0x6F12,0x0024);	//aig_rx_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x0742);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_rx_p0_reg0
write_cmos_sensor_twobyte(0x602A,0x074E);
write_cmos_sensor_twobyte(0x6F12,0x0276);	//aig_tx_ptr0
write_cmos_sensor_twobyte(0x602A,0x0752);
write_cmos_sensor_twobyte(0x6F12,0x030C);	//aig_tx_ptr1
write_cmos_sensor_twobyte(0x602A,0x0756);
write_cmos_sensor_twobyte(0x6F12,0x0026);	//aig_fx_ptr0
write_cmos_sensor_twobyte(0x602A,0x075A);
write_cmos_sensor_twobyte(0x6F12,0x003A);	//aig_fx_ptr1
write_cmos_sensor_twobyte(0x602A,0x075E);
write_cmos_sensor_twobyte(0x6F12,0x002E);	//aig_top_sl_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x0762);
write_cmos_sensor_twobyte(0x6F12,0x0035);	//aig_top_sl_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x0766);
write_cmos_sensor_twobyte(0x6F12,0x0009);	//aig_top_sl_f_reg0
write_cmos_sensor_twobyte(0x602A,0x076A);
write_cmos_sensor_twobyte(0x6F12,0x0002);	//aig_top_sl_f_reg1
write_cmos_sensor_twobyte(0x602A,0x076E);
write_cmos_sensor_twobyte(0x6F12,0x0002);	//aig_top_rx_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x0772);
write_cmos_sensor_twobyte(0x6F12,0x0009);	//aig_top_rx_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x0776);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//aig_top_rx_f_ptr0
write_cmos_sensor_twobyte(0x602A,0x077A);
write_cmos_sensor_twobyte(0x6F12,0x002C);	//aig_top_rx_f_ptr1
write_cmos_sensor_twobyte(0x602A,0x077E);
write_cmos_sensor_twobyte(0x6F12,0x03DA);	//aig_top_rx_r_reg0
write_cmos_sensor_twobyte(0x602A,0x0782);
write_cmos_sensor_twobyte(0x6F12,0x03D3);	//aig_top_rx_r_reg1
write_cmos_sensor_twobyte(0x602A,0x0786);
write_cmos_sensor_twobyte(0x6F12,0x0363);	//aig_top_rx_f_reg0
write_cmos_sensor_twobyte(0x602A,0x078A);
write_cmos_sensor_twobyte(0x6F12,0x035C);	//aig_top_rx_f_reg1
write_cmos_sensor_twobyte(0x602A,0x078E);
write_cmos_sensor_twobyte(0x6F12,0x00C3);	//aig_top_rx_r_reg2
write_cmos_sensor_twobyte(0x602A,0x0792);
write_cmos_sensor_twobyte(0x6F12,0x00BC);	//aig_top_rx_r_reg3
write_cmos_sensor_twobyte(0x602A,0x0796);
write_cmos_sensor_twobyte(0x6F12,0x00C3);	//aig_top_rx_f_reg2
write_cmos_sensor_twobyte(0x602A,0x079A);
write_cmos_sensor_twobyte(0x6F12,0x00BC);	//aig_top_rx_f_reg3
write_cmos_sensor_twobyte(0x602A,0x079E);
write_cmos_sensor_twobyte(0x6F12,0x0015);	//aig_top_rx_r_reg4
write_cmos_sensor_twobyte(0x602A,0x07A2);
write_cmos_sensor_twobyte(0x6F12,0x000E);	//aig_top_rx_r_reg5
write_cmos_sensor_twobyte(0x602A,0x07A6);
write_cmos_sensor_twobyte(0x6F12,0x0015);	//aig_top_rx_f_reg4
write_cmos_sensor_twobyte(0x602A,0x07AA);
write_cmos_sensor_twobyte(0x6F12,0x000E);	//aig_top_rx_f_reg5
write_cmos_sensor_twobyte(0x602A,0x07AE);
write_cmos_sensor_twobyte(0x6F12,0x0031);	//aig_top_rx_r_reg6
write_cmos_sensor_twobyte(0x602A,0x07B2);
write_cmos_sensor_twobyte(0x6F12,0x002A);	//aig_top_rx_r_reg7
write_cmos_sensor_twobyte(0x602A,0x07B6);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//aig_top_rx_f_reg6
write_cmos_sensor_twobyte(0x602A,0x07BA);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_top_rx_f_reg7
write_cmos_sensor_twobyte(0x602A,0x07BE);
write_cmos_sensor_twobyte(0x6F12,0x0277);	//aig_top_tx_l_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x07C2);
write_cmos_sensor_twobyte(0x6F12,0x027E);	//aig_top_tx_l_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x07C6);
write_cmos_sensor_twobyte(0x6F12,0x030D);	//aig_top_tx_l_f_ptr0
write_cmos_sensor_twobyte(0x602A,0x07CA);
write_cmos_sensor_twobyte(0x6F12,0x0314);	//aig_top_tx_l_f_ptr1
write_cmos_sensor_twobyte(0x602A,0x07CE);
write_cmos_sensor_twobyte(0x6F12,0x03CA);	//aig_top_tx_l_r_reg0
write_cmos_sensor_twobyte(0x602A,0x07D2);
write_cmos_sensor_twobyte(0x6F12,0x03C3);	//aig_top_tx_l_r_reg1
write_cmos_sensor_twobyte(0x602A,0x07D6);
write_cmos_sensor_twobyte(0x6F12,0x037F);	//aig_top_tx_l_f_reg0
write_cmos_sensor_twobyte(0x602A,0x07DA);
write_cmos_sensor_twobyte(0x6F12,0x0378);	//aig_top_tx_l_f_reg1
write_cmos_sensor_twobyte(0x602A,0x07DE);
write_cmos_sensor_twobyte(0x6F12,0x00B3);	//aig_top_tx_l_r_reg2
write_cmos_sensor_twobyte(0x602A,0x07E2);
write_cmos_sensor_twobyte(0x6F12,0x00AC);	//aig_top_tx_l_r_reg3
write_cmos_sensor_twobyte(0x602A,0x07E6);
write_cmos_sensor_twobyte(0x6F12,0x001D);	//aig_top_tx_l_f_reg2
write_cmos_sensor_twobyte(0x602A,0x07EA);
write_cmos_sensor_twobyte(0x6F12,0x0016);	//aig_top_tx_l_f_reg3
write_cmos_sensor_twobyte(0x602A,0x07EE);
write_cmos_sensor_twobyte(0x6F12,0x0277);	//aig_top_tx_s_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x07F2);
write_cmos_sensor_twobyte(0x6F12,0x027E);	//aig_top_tx_s_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x07F6);
write_cmos_sensor_twobyte(0x6F12,0x030D);	//aig_top_tx_s_f_ptr0
write_cmos_sensor_twobyte(0x602A,0x07FA);
write_cmos_sensor_twobyte(0x6F12,0x0314);	//aig_top_tx_s_f_ptr1
write_cmos_sensor_twobyte(0x602A,0x07FE);
write_cmos_sensor_twobyte(0x6F12,0x03BE);	//aig_top_tx_s_r_reg0
write_cmos_sensor_twobyte(0x602A,0x0802);
write_cmos_sensor_twobyte(0x6F12,0x03B7);	//aig_top_tx_s_r_reg1
write_cmos_sensor_twobyte(0x602A,0x0806);
write_cmos_sensor_twobyte(0x6F12,0x0373);	//aig_top_tx_s_f_reg0
write_cmos_sensor_twobyte(0x602A,0x080A);
write_cmos_sensor_twobyte(0x6F12,0x036C);	//aig_top_tx_s_f_reg1
write_cmos_sensor_twobyte(0x602A,0x080E);
write_cmos_sensor_twobyte(0x6F12,0x00A7);	//aig_top_tx_s_r_reg2
write_cmos_sensor_twobyte(0x602A,0x0812);
write_cmos_sensor_twobyte(0x6F12,0x00A0);	//aig_top_tx_s_r_reg3
write_cmos_sensor_twobyte(0x602A,0x0816);
write_cmos_sensor_twobyte(0x6F12,0x0011);	//aig_top_tx_s_f_reg2
write_cmos_sensor_twobyte(0x602A,0x081A);
write_cmos_sensor_twobyte(0x6F12,0x000A);	//aig_top_tx_s_f_reg3
write_cmos_sensor_twobyte(0x602A,0x081E);
write_cmos_sensor_twobyte(0x6F12,0x0027);	//aig_top_fx_r_ptr0
write_cmos_sensor_twobyte(0x602A,0x0822);
write_cmos_sensor_twobyte(0x6F12,0x002E);	//aig_top_fx_r_ptr1
write_cmos_sensor_twobyte(0x602A,0x0826);
write_cmos_sensor_twobyte(0x6F12,0x003B);	//aig_top_fx_f_ptr0
write_cmos_sensor_twobyte(0x602A,0x082A);
write_cmos_sensor_twobyte(0x6F12,0x0042);	//aig_top_fx_f_ptr1
write_cmos_sensor_twobyte(0x602A,0x082E);
write_cmos_sensor_twobyte(0x6F12,0x00CB);	//aig_top_fx_r_reg0
write_cmos_sensor_twobyte(0x602A,0x0832);
write_cmos_sensor_twobyte(0x6F12,0x00C4);	//aig_top_fx_r_reg1
write_cmos_sensor_twobyte(0x602A,0x0836);
write_cmos_sensor_twobyte(0x6F12,0x0019);	//aig_top_fx_f_reg0
write_cmos_sensor_twobyte(0x602A,0x083A);
write_cmos_sensor_twobyte(0x6F12,0x0012);	//aig_top_fx_f_reg1
write_cmos_sensor_twobyte(0x602A,0x0B9A);
write_cmos_sensor_twobyte(0x6F12,0x03DB);	//aig_srx_reg0
write_cmos_sensor_twobyte(0x602A,0x0B9E);
write_cmos_sensor_twobyte(0x6F12,0x0364);	//aig_srx_reg1
write_cmos_sensor_twobyte(0x602A,0x0BA2);
write_cmos_sensor_twobyte(0x6F12,0x00C4);	//aig_srx_reg2
write_cmos_sensor_twobyte(0x602A,0x0BA6);
write_cmos_sensor_twobyte(0x6F12,0x0002);	//aig_srx_reg3
write_cmos_sensor_twobyte(0x602A,0x0BAA);
write_cmos_sensor_twobyte(0x6F12,0x03CB);	//aig_dstx_l_reg0
write_cmos_sensor_twobyte(0x602A,0x0BAE);
write_cmos_sensor_twobyte(0x6F12,0x0380);	//aig_dstx_l_reg1
write_cmos_sensor_twobyte(0x602A,0x0BB2);
write_cmos_sensor_twobyte(0x6F12,0x00B4);	//aig_stx_l_reg0
write_cmos_sensor_twobyte(0x602A,0x0BB6);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_stx_l_reg1
write_cmos_sensor_twobyte(0x602A,0x0BCA);
write_cmos_sensor_twobyte(0x6F12,0x03BF);	//aig_dstx_s_reg0
write_cmos_sensor_twobyte(0x602A,0x0BCE);
write_cmos_sensor_twobyte(0x6F12,0x0374);	//aig_dstx_s_reg1
write_cmos_sensor_twobyte(0x602A,0x0BD2);
write_cmos_sensor_twobyte(0x6F12,0x00A8);	//aig_stx_s_reg0
write_cmos_sensor_twobyte(0x602A,0x0BD6);
write_cmos_sensor_twobyte(0x6F12,0x0012);	//aig_stx_s_reg1
write_cmos_sensor_twobyte(0x602A,0x0BEA);
write_cmos_sensor_twobyte(0x6F12,0x00CC);	//aig_sfx_reg0
write_cmos_sensor_twobyte(0x602A,0x0BEE);
write_cmos_sensor_twobyte(0x6F12,0x0006);	//aig_sfx_reg1
write_cmos_sensor_twobyte(0x602A,0x0BBA);
write_cmos_sensor_twobyte(0x6F12,0x03CB);	//aig_dabx_l_reg0
write_cmos_sensor_twobyte(0x602A,0x0BBE);
write_cmos_sensor_twobyte(0x6F12,0x0380);	//aig_dabx_l_reg1
write_cmos_sensor_twobyte(0x602A,0x0BC2);
write_cmos_sensor_twobyte(0x6F12,0x00B4);	//aig_abx_l_reg0
write_cmos_sensor_twobyte(0x602A,0x0BC6);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_abx_l_reg1
write_cmos_sensor_twobyte(0x602A,0x0BDA);
write_cmos_sensor_twobyte(0x6F12,0x03BF);	//aig_dabx_s_reg0
write_cmos_sensor_twobyte(0x602A,0x0BDE);
write_cmos_sensor_twobyte(0x6F12,0x0374);	//aig_dabx_s_reg1
write_cmos_sensor_twobyte(0x602A,0x0BE2);
write_cmos_sensor_twobyte(0x6F12,0x00A8);	//aig_abx_s_reg0
write_cmos_sensor_twobyte(0x602A,0x0BE6);
write_cmos_sensor_twobyte(0x6F12,0x0012);	//aig_abx_s_reg1
write_cmos_sensor_twobyte(0x602A,0x0BF2);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_offs_sh_reg0
write_cmos_sensor_twobyte(0x602A,0x083E);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_sr_ptr0
write_cmos_sensor_twobyte(0x602A,0x0842);
write_cmos_sensor_twobyte(0x6F12,0x059F);	//aig_sr_ptr1
write_cmos_sensor_twobyte(0x602A,0x0846);
write_cmos_sensor_twobyte(0x6F12,0x0276);	//aig_ss_ptr0
write_cmos_sensor_twobyte(0x602A,0x084A);
write_cmos_sensor_twobyte(0x6F12,0x059D);	//aig_ss_ptr1
write_cmos_sensor_twobyte(0x602A,0x084E);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_s1_ptr0
write_cmos_sensor_twobyte(0x602A,0x0852);
write_cmos_sensor_twobyte(0x6F12,0x0324);	//aig_s1_ptr1
write_cmos_sensor_twobyte(0x602A,0x0856);
write_cmos_sensor_twobyte(0x6F12,0x001A);	//aig_s3_ptr0
write_cmos_sensor_twobyte(0x602A,0x085A);
write_cmos_sensor_twobyte(0x6F12,0x0114);	//aig_s3_ptr1
write_cmos_sensor_twobyte(0x602A,0x085E);
write_cmos_sensor_twobyte(0x6F12,0x001A);	//aig_s4_ptr0
write_cmos_sensor_twobyte(0x602A,0x0862);
write_cmos_sensor_twobyte(0x6F12,0x0100);	//aig_s4_ptr1
write_cmos_sensor_twobyte(0x602A,0x0866);
write_cmos_sensor_twobyte(0x6F12,0x001A);	//aig_s4d_ptr0
write_cmos_sensor_twobyte(0x602A,0x086A);
write_cmos_sensor_twobyte(0x6F12,0x010C);	//aig_s4d_ptr1
write_cmos_sensor_twobyte(0x602A,0x086E);
write_cmos_sensor_twobyte(0x6F12,0x001A);	//aig_clp_sl_ptr0
write_cmos_sensor_twobyte(0x602A,0x0872);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_clp_sl_ptr1
write_cmos_sensor_twobyte(0x602A,0x0876);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_pxbst_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x087A);
write_cmos_sensor_twobyte(0x6F12,0x004C);	//aig_pxbst_p0_ptr1
write_cmos_sensor_twobyte(0x602A,0x087E);
write_cmos_sensor_twobyte(0x6F12,0x030C);	//aig_pxbst_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x0882);
write_cmos_sensor_twobyte(0x6F12,0x0324);	//aig_pxbst_p1_ptr1
write_cmos_sensor_twobyte(0x602A,0x0886);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_pxbstob_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x088A);
write_cmos_sensor_twobyte(0x6F12,0x004C);	//aig_pxbstob_p0_ptr1
write_cmos_sensor_twobyte(0x602A,0x088E);
write_cmos_sensor_twobyte(0x6F12,0x030C);	//aig_pxbstob_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x0892);
write_cmos_sensor_twobyte(0x6F12,0x0314);	//aig_pxbstob_p1_ptr1



write_cmos_sensor_twobyte(0x602A,0x08AE);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_lp_hblk_cds_reg0
write_cmos_sensor_twobyte(0x602A,0x08B2);
write_cmos_sensor_twobyte(0x6F12,0x0005);	//aig_lp_hblk_cds_reg1
write_cmos_sensor_twobyte(0x602A,0x08B6);
write_cmos_sensor_twobyte(0x6F12,0x0042);	//aig_vref_smp_ptr0
write_cmos_sensor_twobyte(0x602A,0x08BA);
write_cmos_sensor_twobyte(0x6F12,0x05A5);	//aig_vref_smp_ptr1
write_cmos_sensor_twobyte(0x602A,0x08BE);
write_cmos_sensor_twobyte(0x6F12,0x0204);	//aig_cnt_en_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x08C2);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_cnt_en_p0_ptr1
write_cmos_sensor_twobyte(0x602A,0x08C6);
write_cmos_sensor_twobyte(0x6F12,0x046A);	//aig_cnt_en_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x08CA);
write_cmos_sensor_twobyte(0x6F12,0x059B);	//aig_cnt_en_p1_ptr1
write_cmos_sensor_twobyte(0x602A,0x08CE);
write_cmos_sensor_twobyte(0x6F12,0x0267);	//aig_conv_enb_ptr0
write_cmos_sensor_twobyte(0x602A,0x08D2);
write_cmos_sensor_twobyte(0x6F12,0x027E);	//aig_conv_enb_ptr1
write_cmos_sensor_twobyte(0x602A,0x08D6);
write_cmos_sensor_twobyte(0x6F12,0x026E);	//aig_conv1_ptr0
write_cmos_sensor_twobyte(0x602A,0x08DA);
write_cmos_sensor_twobyte(0x6F12,0x0286);	//aig_conv1_ptr1
write_cmos_sensor_twobyte(0x602A,0x08DE);
write_cmos_sensor_twobyte(0x6F12,0x0276);	//aig_conv2_ptr0
write_cmos_sensor_twobyte(0x602A,0x08E2);
write_cmos_sensor_twobyte(0x6F12,0x0286);	//aig_conv2_ptr1
write_cmos_sensor_twobyte(0x602A,0x08E6);
write_cmos_sensor_twobyte(0x6F12,0x0267);	//aig_lat_lsb_ptr0
write_cmos_sensor_twobyte(0x602A,0x08EA);
write_cmos_sensor_twobyte(0x6F12,0x026C);	//aig_lat_lsb_ptr1
write_cmos_sensor_twobyte(0x602A,0x08EE);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_conv_lsb_ptr0
write_cmos_sensor_twobyte(0x602A,0x08F2);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_conv_lsb_ptr1
write_cmos_sensor_twobyte(0x602A,0x08F6);
write_cmos_sensor_twobyte(0x6F12,0x026E);	//aig_conv_lsb_ptr2
write_cmos_sensor_twobyte(0x602A,0x08FA);
write_cmos_sensor_twobyte(0x6F12,0x0286);	//aig_conv_lsb_ptr3
write_cmos_sensor_twobyte(0x602A,0x08FE);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_rst_div_ptr0
write_cmos_sensor_twobyte(0x602A,0x0902);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_rst_div_ptr1
write_cmos_sensor_twobyte(0x602A,0x0906);
write_cmos_sensor_twobyte(0x6F12,0x0267);	//aig_rst_div_ptr2
write_cmos_sensor_twobyte(0x602A,0x090A);
write_cmos_sensor_twobyte(0x6F12,0x026C);	//aig_rst_div_ptr3
write_cmos_sensor_twobyte(0x602A,0x090E);
write_cmos_sensor_twobyte(0x6F12,0x0204);	//aig_cnt_en_ms_p0_ptr0
write_cmos_sensor_twobyte(0x602A,0x0912);
write_cmos_sensor_twobyte(0x6F12,0x022A);	//aig_cnt_en_ms_p0_ptr1
write_cmos_sensor_twobyte(0x602A,0x0916);
write_cmos_sensor_twobyte(0x6F12,0x023A);	//aig_cnt_en_ms_p0_ptr2
write_cmos_sensor_twobyte(0x602A,0x091A);
write_cmos_sensor_twobyte(0x6F12,0x0262);	//aig_cnt_en_ms_p0_ptr3
write_cmos_sensor_twobyte(0x602A,0x091E);
write_cmos_sensor_twobyte(0x6F12,0x046A);	//aig_cnt_en_ms_p1_ptr0
write_cmos_sensor_twobyte(0x602A,0x0922);
write_cmos_sensor_twobyte(0x6F12,0x04FA);	//aig_cnt_en_ms_p1_ptr1
write_cmos_sensor_twobyte(0x602A,0x092E);
write_cmos_sensor_twobyte(0x6F12,0x050A);	//aig_cnt_en_ms_p1_ptr2
write_cmos_sensor_twobyte(0x602A,0x0932);
write_cmos_sensor_twobyte(0x6F12,0x059B);	//aig_cnt_en_ms_p1_ptr3
write_cmos_sensor_twobyte(0x602A,0x0936);
write_cmos_sensor_twobyte(0x6F12,0x022C);	//aig_conv_enb_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x093A);
write_cmos_sensor_twobyte(0x6F12,0x0231);	//aig_conv_enb_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x093E);
write_cmos_sensor_twobyte(0x6F12,0x0268);	//aig_conv_enb_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x0942);
write_cmos_sensor_twobyte(0x6F12,0x026E);	//aig_conv_enb_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x0946);
write_cmos_sensor_twobyte(0x6F12,0x0279);	//aig_conv_enb_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x094A);
write_cmos_sensor_twobyte(0x6F12,0x027F);	//aig_conv_enb_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x094E);
write_cmos_sensor_twobyte(0x6F12,0x04FC);	//aig_conv_enb_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x0952);
write_cmos_sensor_twobyte(0x6F12,0x0501);	//aig_conv_enb_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x0956);
write_cmos_sensor_twobyte(0x6F12,0x059D);	//aig_conv_enb_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x095A);
write_cmos_sensor_twobyte(0x6F12,0x05A2);	//aig_conv_enb_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x095E);
write_cmos_sensor_twobyte(0x6F12,0x022E);	//aig_conv1_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x0962);
write_cmos_sensor_twobyte(0x6F12,0x0232);	//aig_conv1_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x0966);
write_cmos_sensor_twobyte(0x6F12,0x026A);	//aig_conv1_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x096A);
write_cmos_sensor_twobyte(0x6F12,0x026F);	//aig_conv1_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x096E);
write_cmos_sensor_twobyte(0x6F12,0x027B);	//aig_conv1_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x0972);
write_cmos_sensor_twobyte(0x6F12,0x0280);	//aig_conv1_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x0976);
write_cmos_sensor_twobyte(0x6F12,0x04FE);	//aig_conv1_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x097A);
write_cmos_sensor_twobyte(0x6F12,0x0502);	//aig_conv1_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x097E);
write_cmos_sensor_twobyte(0x6F12,0x059F);	//aig_conv1_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x0982);
write_cmos_sensor_twobyte(0x6F12,0x05A3);	//aig_conv1_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x0986);
write_cmos_sensor_twobyte(0x6F12,0x022F);	//aig_conv2_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x098A);
write_cmos_sensor_twobyte(0x6F12,0x0232);	//aig_conv2_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x098E);
write_cmos_sensor_twobyte(0x6F12,0x026B);	//aig_conv2_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x0992);
write_cmos_sensor_twobyte(0x6F12,0x026F);	//aig_conv2_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x0996);
write_cmos_sensor_twobyte(0x6F12,0x027C);	//aig_conv2_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x099A);
write_cmos_sensor_twobyte(0x6F12,0x0280);	//aig_conv2_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x099E);
write_cmos_sensor_twobyte(0x6F12,0x04FF);	//aig_conv2_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x09A2);
write_cmos_sensor_twobyte(0x6F12,0x0502);	//aig_conv2_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x09A6);
write_cmos_sensor_twobyte(0x6F12,0x05A0);	//aig_conv2_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x09AA);
write_cmos_sensor_twobyte(0x6F12,0x05A3);	//aig_conv2_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x09AE);
write_cmos_sensor_twobyte(0x6F12,0x022D);	//aig_lat_lsb_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x09B2);
write_cmos_sensor_twobyte(0x6F12,0x022F);	//aig_lat_lsb_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x09B6);
write_cmos_sensor_twobyte(0x6F12,0x0265);	//aig_lat_lsb_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x09BA);
write_cmos_sensor_twobyte(0x6F12,0x0268);	//aig_lat_lsb_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x09BE);
write_cmos_sensor_twobyte(0x6F12,0x0276);	//aig_lat_lsb_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x09C2);
write_cmos_sensor_twobyte(0x6F12,0x0279);	//aig_lat_lsb_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x09C6);
write_cmos_sensor_twobyte(0x6F12,0x04FC);	//aig_lat_lsb_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x09CA);
write_cmos_sensor_twobyte(0x6F12,0x04FF);	//aig_lat_lsb_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x09CE);
write_cmos_sensor_twobyte(0x6F12,0x059D);	//aig_lat_lsb_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x09D2);
write_cmos_sensor_twobyte(0x6F12,0x05A0);	//aig_lat_lsb_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x09D6);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_conv_lsb_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x09DA);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_conv_lsb_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x09DE);
write_cmos_sensor_twobyte(0x6F12,0x0231);	//aig_conv_lsb_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x09E2);
write_cmos_sensor_twobyte(0x6F12,0x0234);	//aig_conv_lsb_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x09E6);
write_cmos_sensor_twobyte(0x6F12,0x026A);	//aig_conv_lsb_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x09EA);
write_cmos_sensor_twobyte(0x6F12,0x026F);	//aig_conv_lsb_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x09EE);
write_cmos_sensor_twobyte(0x6F12,0x027B);	//aig_conv_lsb_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x09F2);
write_cmos_sensor_twobyte(0x6F12,0x0280);	//aig_conv_lsb_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x09F6);
write_cmos_sensor_twobyte(0x6F12,0x0501);	//aig_conv_lsb_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x09FA);
write_cmos_sensor_twobyte(0x6F12,0x0504);	//aig_conv_lsb_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x09FE);
write_cmos_sensor_twobyte(0x6F12,0x05A2);	//aig_conv_lsb_ms_ptr10
write_cmos_sensor_twobyte(0x602A,0x0A02);
write_cmos_sensor_twobyte(0x6F12,0x05A5);	//aig_conv_lsb_ms_ptr11
write_cmos_sensor_twobyte(0x602A,0x0A06);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_rst_div_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A0A);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_rst_div_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x0A0E);
write_cmos_sensor_twobyte(0x6F12,0x022C);	//aig_rst_div_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x0A12);
write_cmos_sensor_twobyte(0x6F12,0x022F);	//aig_rst_div_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x0A16);
write_cmos_sensor_twobyte(0x6F12,0x0265);	//aig_rst_div_ms_ptr4
write_cmos_sensor_twobyte(0x602A,0x0A1A);
write_cmos_sensor_twobyte(0x6F12,0x0268);	//aig_rst_div_ms_ptr5
write_cmos_sensor_twobyte(0x602A,0x0A1E);
write_cmos_sensor_twobyte(0x6F12,0x0276);	//aig_rst_div_ms_ptr6
write_cmos_sensor_twobyte(0x602A,0x0A22);
write_cmos_sensor_twobyte(0x6F12,0x0279);	//aig_rst_div_ms_ptr7
write_cmos_sensor_twobyte(0x602A,0x0A26);
write_cmos_sensor_twobyte(0x6F12,0x04FC);	//aig_rst_div_ms_ptr8
write_cmos_sensor_twobyte(0x602A,0x0A2A);
write_cmos_sensor_twobyte(0x6F12,0x04FF);	//aig_rst_div_ms_ptr9
write_cmos_sensor_twobyte(0x602A,0x0A2E);
write_cmos_sensor_twobyte(0x6F12,0x059D);	//aig_rst_div_ms_ptr10
write_cmos_sensor_twobyte(0x602A,0x0A32);
write_cmos_sensor_twobyte(0x6F12,0x05A0);	//aig_rst_div_ms_ptr11
write_cmos_sensor_twobyte(0x602A,0x0A36);
write_cmos_sensor_twobyte(0x6F12,0x0231);	//aig_piv_en_ms_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A3A);
write_cmos_sensor_twobyte(0x6F12,0x026A);	//aig_piv_en_ms_ptr1
write_cmos_sensor_twobyte(0x602A,0x0A3E);
write_cmos_sensor_twobyte(0x6F12,0x0501);	//aig_piv_en_ms_ptr2
write_cmos_sensor_twobyte(0x602A,0x0A42);
write_cmos_sensor_twobyte(0x6F12,0x059F);	//aig_piv_en_ms_ptr3
write_cmos_sensor_twobyte(0x602A,0x0A46);
write_cmos_sensor_twobyte(0x6F12,0x0203);	//aig_comp_en_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A4A);
write_cmos_sensor_twobyte(0x6F12,0x0263);	//aig_comp_en_ptr1
write_cmos_sensor_twobyte(0x602A,0x0A4E);
write_cmos_sensor_twobyte(0x6F12,0x0469);	//aig_comp_en_ptr2
write_cmos_sensor_twobyte(0x602A,0x0A52);
write_cmos_sensor_twobyte(0x6F12,0x059C);	//aig_comp_en_ptr3
write_cmos_sensor_twobyte(0x602A,0x0A56);
write_cmos_sensor_twobyte(0x6F12,0x0001);	//aig_cnt_rst_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A5A);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_cnt_rst_ptr1
write_cmos_sensor_twobyte(0x602A,0x0A5E);
write_cmos_sensor_twobyte(0x6F12,0x000C);	//aig_conv_en_offset_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A62);
write_cmos_sensor_twobyte(0x6F12,0x0014);	//aig_conv_en_offset_ptr1
write_cmos_sensor_twobyte(0x602A,0x0A66);
write_cmos_sensor_twobyte(0x6F12,0x0267);	//aig_conv_en_offset_ptr2
write_cmos_sensor_twobyte(0x602A,0x0A6A);
write_cmos_sensor_twobyte(0x6F12,0x0281);	//aig_conv_en_offset_ptr3
write_cmos_sensor_twobyte(0x602A,0x0A6E);
write_cmos_sensor_twobyte(0x6F12,0x000C);	//aig_lat_lsb_offset_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A72);
write_cmos_sensor_twobyte(0x6F12,0x0014);	//aig_lat_lsb_offset_ptr1
write_cmos_sensor_twobyte(0x602A,0x0A76);
write_cmos_sensor_twobyte(0x6F12,0x0267);	//aig_lat_lsb_offset_ptr2
write_cmos_sensor_twobyte(0x602A,0x0A7A);
write_cmos_sensor_twobyte(0x6F12,0x026C);	//aig_lat_lsb_offset_ptr3
write_cmos_sensor_twobyte(0x602A,0x0A7E);
write_cmos_sensor_twobyte(0x6F12,0x003C);	//aig_lp_hblk_dbs_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A82);
write_cmos_sensor_twobyte(0x6F12,0x000A);	//aig_lp_hblk_dbs_reg1
write_cmos_sensor_twobyte(0x602A,0x0A86);
write_cmos_sensor_twobyte(0x6F12,0x0132);	//aig_off_rst_en_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A8A);
write_cmos_sensor_twobyte(0x6F12,0x05A1);	//aig_off_rst_en_ptr1
write_cmos_sensor_twobyte(0x602A,0x0A8E);
write_cmos_sensor_twobyte(0x6F12,0x0133);	//aig_rmp_rst_ptr0
write_cmos_sensor_twobyte(0x602A,0x0A92);
write_cmos_sensor_twobyte(0x6F12,0x0135);	//aig_rmp_rst_ptr1
write_cmos_sensor_twobyte(0x602A,0x0A96);
write_cmos_sensor_twobyte(0x6F12,0x0264);	//aig_rmp_rst_ptr2
write_cmos_sensor_twobyte(0x602A,0x0A9A);
write_cmos_sensor_twobyte(0x6F12,0x0267);	//aig_rmp_rst_ptr3
write_cmos_sensor_twobyte(0x602A,0x0A9E);
write_cmos_sensor_twobyte(0x6F12,0x059D);	//aig_rmp_rst_ptr4
write_cmos_sensor_twobyte(0x602A,0x0AA2);
write_cmos_sensor_twobyte(0x6F12,0x05A0);	//aig_rmp_rst_ptr5
write_cmos_sensor_twobyte(0x602A,0x0AA6);
write_cmos_sensor_twobyte(0x6F12,0x0264);	//aig_rmp_mode_ptr0
write_cmos_sensor_twobyte(0x602A,0x0AAA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr0
write_cmos_sensor_twobyte(0x602A,0x0AAE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr1
write_cmos_sensor_twobyte(0x602A,0x0AB2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr2
write_cmos_sensor_twobyte(0x602A,0x0AB6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr3
write_cmos_sensor_twobyte(0x602A,0x0ABA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr4
write_cmos_sensor_twobyte(0x602A,0x0ABE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr5
write_cmos_sensor_twobyte(0x602A,0x0AC2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr6
write_cmos_sensor_twobyte(0x602A,0x0AC6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl0_ptr7
write_cmos_sensor_twobyte(0x602A,0x0ACA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr0
write_cmos_sensor_twobyte(0x602A,0x0ACE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr1
write_cmos_sensor_twobyte(0x602A,0x0AD2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr2
write_cmos_sensor_twobyte(0x602A,0x0AD6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr3
write_cmos_sensor_twobyte(0x602A,0x0ADA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr4
write_cmos_sensor_twobyte(0x602A,0x0ADE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr5
write_cmos_sensor_twobyte(0x602A,0x0AE2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr6
write_cmos_sensor_twobyte(0x602A,0x0AE6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl1_ptr7
write_cmos_sensor_twobyte(0x602A,0x0AEA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr0
write_cmos_sensor_twobyte(0x602A,0x0AEE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr1
write_cmos_sensor_twobyte(0x602A,0x0AF2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr2
write_cmos_sensor_twobyte(0x602A,0x0AF6);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr3
write_cmos_sensor_twobyte(0x602A,0x0AFA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr4
write_cmos_sensor_twobyte(0x602A,0x0AFE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr5
write_cmos_sensor_twobyte(0x602A,0x0B02);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr6
write_cmos_sensor_twobyte(0x602A,0x0B06);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_optl2_ptr7
write_cmos_sensor_twobyte(0x602A,0x0B52);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr0
write_cmos_sensor_twobyte(0x602A,0x0B56);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr1
write_cmos_sensor_twobyte(0x602A,0x0B5A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr2
write_cmos_sensor_twobyte(0x602A,0x0B5E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr3
write_cmos_sensor_twobyte(0x602A,0x0B62);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr4
write_cmos_sensor_twobyte(0x602A,0x0B66);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr5
write_cmos_sensor_twobyte(0x602A,0x0B6A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr6
write_cmos_sensor_twobyte(0x602A,0x0B6E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth0_ptr7
write_cmos_sensor_twobyte(0x602A,0x0B72);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr0
write_cmos_sensor_twobyte(0x602A,0x0B76);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr1
write_cmos_sensor_twobyte(0x602A,0x0B7A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr2
write_cmos_sensor_twobyte(0x602A,0x0B7E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr3
write_cmos_sensor_twobyte(0x602A,0x0B82);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr4
write_cmos_sensor_twobyte(0x602A,0x0B86);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr5
write_cmos_sensor_twobyte(0x602A,0x0B8A);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr6
write_cmos_sensor_twobyte(0x602A,0x0B8E);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_opth1_ptr7
write_cmos_sensor_twobyte(0x602A,0x0BF6);
write_cmos_sensor_twobyte(0x6F12,0x0006);	//aig_lat_set_reg0
write_cmos_sensor_twobyte(0x602A,0x0BFA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//aig_lat_set_reg1
write_cmos_sensor_twobyte(0x602A,0x0B92);
write_cmos_sensor_twobyte(0x6F12,0x0008);	//aig_vda_rst_ptr0
write_cmos_sensor_twobyte(0x602A,0x0B96);
write_cmos_sensor_twobyte(0x6F12,0x000E);	//aig_vda_rst_ptr1
write_cmos_sensor_twobyte(0x602A,0x0BFE);
write_cmos_sensor_twobyte(0x6F12,0x001E);	//aig_vda_rd_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C02);
write_cmos_sensor_twobyte(0x6F12,0x0024);	//aig_vda_rd_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C06);
write_cmos_sensor_twobyte(0x6F12,0x0034);	//aig_vda_rd_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C0A);
write_cmos_sensor_twobyte(0x6F12,0x003A);	//aig_vda_rd_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C0E);
write_cmos_sensor_twobyte(0x6F12,0x004A);	//aig_vda_sh1_s_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C12);
write_cmos_sensor_twobyte(0x6F12,0x0050);	//aig_vda_sh1_s_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C16);
write_cmos_sensor_twobyte(0x6F12,0x0060);	//aig_vda_sh1_s_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C1A);
write_cmos_sensor_twobyte(0x6F12,0x0066);	//aig_vda_sh1_s_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C1E);
write_cmos_sensor_twobyte(0x6F12,0x0076);	//aig_vda_sh2_s_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C22);
write_cmos_sensor_twobyte(0x6F12,0x007C);	//aig_vda_sh2_s_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C26);
write_cmos_sensor_twobyte(0x6F12,0x008C);	//aig_vda_sh2_s_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C2A);
write_cmos_sensor_twobyte(0x6F12,0x0092);	//aig_vda_sh2_s_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C2E);
write_cmos_sensor_twobyte(0x6F12,0x00A2);	//aig_vda_sh1_l_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C32);
write_cmos_sensor_twobyte(0x6F12,0x00A8);	//aig_vda_sh1_l_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C36);
write_cmos_sensor_twobyte(0x6F12,0x00B8);	//aig_vda_sh1_l_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C3A);
write_cmos_sensor_twobyte(0x6F12,0x00BE);	//aig_vda_sh1_l_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C3E);
write_cmos_sensor_twobyte(0x6F12,0x00CE);	//aig_vda_sh2_l_e_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C42);
write_cmos_sensor_twobyte(0x6F12,0x00D4);	//aig_vda_sh2_l_e_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C46);
write_cmos_sensor_twobyte(0x6F12,0x00E4);	//aig_vda_sh2_l_o_set_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C4A);
write_cmos_sensor_twobyte(0x6F12,0x00EA);	//aig_vda_sh2_l_o_set_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C4E);
write_cmos_sensor_twobyte(0x6F12,0x0015);	//aig_vda_pulse_ptr0
write_cmos_sensor_twobyte(0x602A,0x0C52);
write_cmos_sensor_twobyte(0x6F12,0x002B);	//aig_vda_pulse_ptr1
write_cmos_sensor_twobyte(0x602A,0x0C56);
write_cmos_sensor_twobyte(0x6F12,0x0041);	//aig_vda_pulse_ptr2
write_cmos_sensor_twobyte(0x602A,0x0C5A);
write_cmos_sensor_twobyte(0x6F12,0x0057);	//aig_vda_pulse_ptr3
write_cmos_sensor_twobyte(0x602A,0x0C5E);
write_cmos_sensor_twobyte(0x6F12,0x006D);	//aig_vda_pulse_ptr4
write_cmos_sensor_twobyte(0x602A,0x0C62);
write_cmos_sensor_twobyte(0x6F12,0x0083);	//aig_vda_pulse_ptr5
write_cmos_sensor_twobyte(0x602A,0x0C66);
write_cmos_sensor_twobyte(0x6F12,0x0099);	//aig_vda_pulse_ptr6
write_cmos_sensor_twobyte(0x602A,0x0C6A);
write_cmos_sensor_twobyte(0x6F12,0x00AF);	//aig_vda_pulse_ptr7
write_cmos_sensor_twobyte(0x602A,0x0C6E);
write_cmos_sensor_twobyte(0x6F12,0x00C5);	//aig_vda_pulse_ptr8
write_cmos_sensor_twobyte(0x602A,0x0C72);
write_cmos_sensor_twobyte(0x6F12,0x00DB);	//aig_vda_pulse_ptr9
write_cmos_sensor_twobyte(0x602A,0x0C76);
write_cmos_sensor_twobyte(0x6F12,0x00F1);	//aig_vda_pulse_ptr10


write_cmos_sensor_twobyte(0x602A,0x0C7A);
write_cmos_sensor_twobyte(0x6F12,0x0329);	//VIR_1_NFA_PTR0
write_cmos_sensor_twobyte(0x602A,0x0C7E);
write_cmos_sensor_twobyte(0x6F12,0x0328);	//VIR_1_NFA_PTR1
write_cmos_sensor_twobyte(0x602A,0x0C82);
write_cmos_sensor_twobyte(0x6F12,0x064E);	//VIR_1_NFA_PTR2
write_cmos_sensor_twobyte(0x602A,0x0C86);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//VIR_1_NFA_PTR3
write_cmos_sensor_twobyte(0x602A,0x0C8A);
write_cmos_sensor_twobyte(0x6F12,0x03BE);	//VIR_MSO_PTR0
write_cmos_sensor_twobyte(0x602A,0x0C8E);
write_cmos_sensor_twobyte(0x6F12,0x05EF);	//VIR_MIN_ADC_COLS
write_cmos_sensor_twobyte(0x602A,0x0C92);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//VIR_MVF_PTR0
write_cmos_sensor_twobyte(0x602A,0x0C96);
write_cmos_sensor_twobyte(0x6F12,0x0329);	//VIR_1_NFA_PTR0
write_cmos_sensor_twobyte(0x602A,0x0C9A);
write_cmos_sensor_twobyte(0x6F12,0x0328);	//VIR_1_NFA_PTR1
write_cmos_sensor_twobyte(0x602A,0x0C9E);
write_cmos_sensor_twobyte(0x6F12,0x064E);	//VIR_1_NFA_PTR2
write_cmos_sensor_twobyte(0x602A,0x0CA2);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//VIR_1_NFA_PTR3
write_cmos_sensor_twobyte(0x602A,0x0CA6);
write_cmos_sensor_twobyte(0x6F12,0x03BE);	//VIR_MSO_PTR0
write_cmos_sensor_twobyte(0x602A,0x0CAA);
write_cmos_sensor_twobyte(0x6F12,0x05EF);	//VIR_MIN_ADC_COLS
write_cmos_sensor_twobyte(0x602A,0x0CAE);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//VIR_MVF_PTR0
write_cmos_sensor_twobyte(0x602A,0x0CB2);
write_cmos_sensor_twobyte(0x6F12,0x0329);	//VIR_1_NFA_PTR0
write_cmos_sensor_twobyte(0x602A,0x0CB6);
write_cmos_sensor_twobyte(0x6F12,0x0328);	//VIR_1_NFA_PTR1
write_cmos_sensor_twobyte(0x602A,0x0CBA);
write_cmos_sensor_twobyte(0x6F12,0x064E);	//VIR_1_NFA_PTR2
write_cmos_sensor_twobyte(0x602A,0x0CBE);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//VIR_1_NFA_PTR3
write_cmos_sensor_twobyte(0x602A,0x0CC2);
write_cmos_sensor_twobyte(0x6F12,0x03BE);	//VIR_MSO_PTR0
write_cmos_sensor_twobyte(0x602A,0x0CC6);
write_cmos_sensor_twobyte(0x6F12,0x05EF);	//VIR_MIN_ADC_COLS
write_cmos_sensor_twobyte(0x602A,0x0CCA);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//VIR_MVF_PTR0
write_cmos_sensor_twobyte(0x602A,0x0CCE);
write_cmos_sensor_twobyte(0x6F12,0x0329);	//VIR_1_NFA_PTR0
write_cmos_sensor_twobyte(0x602A,0x0CD2);
write_cmos_sensor_twobyte(0x6F12,0x0328);	//VIR_1_NFA_PTR1
write_cmos_sensor_twobyte(0x602A,0x0CD6);
write_cmos_sensor_twobyte(0x6F12,0x064E);	//VIR_1_NFA_PTR2
write_cmos_sensor_twobyte(0x602A,0x0CDA);
write_cmos_sensor_twobyte(0x6F12,0x0000);	//VIR_1_NFA_PTR3
write_cmos_sensor_twobyte(0x602A,0x0CDE);
write_cmos_sensor_twobyte(0x6F12,0x03BE);	//VIR_MSO_PTR0
write_cmos_sensor_twobyte(0x602A,0x0CE2);
write_cmos_sensor_twobyte(0x6F12,0x05EF);	//VIR_MIN_ADC_COLS
write_cmos_sensor_twobyte(0x602A,0x0CE6);
write_cmos_sensor_twobyte(0x6F12,0x0025);	//VIR_MVF_PTR0

                         
//////////// Analog Tuning 150111 ////////////
write_cmos_sensor_twobyte(0x602A,0x0E90);
write_cmos_sensor_twobyte(0x6F12,0x0004);	//VRGSL // 20150124 KTY
write_cmos_sensor_twobyte(0x602A,0x0E8E);
write_cmos_sensor_twobyte(0x6F12,0x0004);	//VRGSL // 20150124 KTY
write_cmos_sensor_twobyte(0x602A,0x0E94);
write_cmos_sensor_twobyte(0x6F12,0x000D);	// 20150113 KTY
write_cmos_sensor_twobyte(0x6F12,0x000D);	// 20150113 KTY
write_cmos_sensor_twobyte(0x6F12,0x000D);	// 20150113 KTY
// pjw write_cmos_sensor_twobyte(0xf480,0x0008); //VTG
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0xF480,0x0010);
write_cmos_sensor_twobyte(0xF4D0,0x0020);
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x150E);
write_cmos_sensor_twobyte(0x6F12,0x0610);	// 20150113 HHJ
write_cmos_sensor_twobyte(0x6F12,0x0610);	// 20150113 HHJ

//Gain Linearity Compensation

//Short Ramp
////Gr
write_cmos_sensor_twobyte(0x602A,0x3E32);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1070);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1070);
write_cmos_sensor_twobyte(0x6F12,0x1080);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1080);
write_cmos_sensor_twobyte(0x6F12,0x1090);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1090);
write_cmos_sensor_twobyte(0x6F12,0x10A0);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x10A0);
write_cmos_sensor_twobyte(0x6F12,0x10A0);
////R
write_cmos_sensor_twobyte(0x602A,0x3E72);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1040);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1040);
write_cmos_sensor_twobyte(0x6F12,0x1040);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1040);
write_cmos_sensor_twobyte(0x6F12,0x1020);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1020);
write_cmos_sensor_twobyte(0x6F12,0x1020);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1020);
write_cmos_sensor_twobyte(0x6F12,0x1020);
////B
write_cmos_sensor_twobyte(0x602A,0x3EB2);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1040);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1040);
write_cmos_sensor_twobyte(0x6F12,0x1040);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1040);
write_cmos_sensor_twobyte(0x6F12,0x1020);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1020);
write_cmos_sensor_twobyte(0x6F12,0x1020);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1020);
write_cmos_sensor_twobyte(0x6F12,0x1020);
////Gb
write_cmos_sensor_twobyte(0x602A,0x3EF2);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1070);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1070);
write_cmos_sensor_twobyte(0x6F12,0x1080);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1080);
write_cmos_sensor_twobyte(0x6F12,0x1090);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1090);
write_cmos_sensor_twobyte(0x6F12,0x10A0);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x10A0);
write_cmos_sensor_twobyte(0x6F12,0x10A0);

//long Ramp
////Gr
write_cmos_sensor_twobyte(0x602A,0x3F32);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1070);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1070);
write_cmos_sensor_twobyte(0x6F12,0x1080);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1080);
write_cmos_sensor_twobyte(0x6F12,0x1090);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1090);
write_cmos_sensor_twobyte(0x6F12,0x10A0);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x10A0);
write_cmos_sensor_twobyte(0x6F12,0x10A0);
////R
write_cmos_sensor_twobyte(0x602A,0x3F72);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1040);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1040);
write_cmos_sensor_twobyte(0x6F12,0x1040);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1040);
write_cmos_sensor_twobyte(0x6F12,0x1020);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1020);
write_cmos_sensor_twobyte(0x6F12,0x1020);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1020);
write_cmos_sensor_twobyte(0x6F12,0x1020);
////B
write_cmos_sensor_twobyte(0x602A,0x3FB2);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1040);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1040);
write_cmos_sensor_twobyte(0x6F12,0x1040);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1040);
write_cmos_sensor_twobyte(0x6F12,0x1020);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1020);
write_cmos_sensor_twobyte(0x6F12,0x1020);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1020);
write_cmos_sensor_twobyte(0x6F12,0x1020);
////Gb
write_cmos_sensor_twobyte(0x602A,0x3FF2);
write_cmos_sensor_twobyte(0x6F12,0x0100);
write_cmos_sensor_twobyte(0x6F12,0x01FF);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1070);

write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x03FF);
write_cmos_sensor_twobyte(0x6F12,0x1070);
write_cmos_sensor_twobyte(0x6F12,0x1080);

write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x6F12,0x07FF);
write_cmos_sensor_twobyte(0x6F12,0x1080);
write_cmos_sensor_twobyte(0x6F12,0x1090);

write_cmos_sensor_twobyte(0x6F12,0x0800);
write_cmos_sensor_twobyte(0x6F12,0x0FFF);
write_cmos_sensor_twobyte(0x6F12,0x1090);
write_cmos_sensor_twobyte(0x6F12,0x10A0);

write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x1000);
write_cmos_sensor_twobyte(0x6F12,0x10A0);
write_cmos_sensor_twobyte(0x6F12,0x10A0);

// ramp pwdn
write_cmos_sensor_twobyte(0x602A,0x0E88);
write_cmos_sensor_twobyte(0x6F12,0x3EF8);	// F406 address :VBLK_EN on
write_cmos_sensor_twobyte(0x6028,0x4000);

write_cmos_sensor_twobyte(0xF4AA,0x0048);

mDELAY(10);

}    /*    sensor_init  */

static void preview_setting_10(void)
{

//$MIPI[Width:2816,Height:2112,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:2816,Height:2112,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]


// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x1C87);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C88);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C89);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C8A);
write_cmos_sensor(0x6F12,0x03);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x1C8B);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8C);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8D);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8E);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x1C8F);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C90);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C91);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C92);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current

write_cmos_sensor_twobyte(0x602A,0x426A);
write_cmos_sensor(0x6F12,0x04);
write_cmos_sensor_twobyte(0x602A,0x42FD);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x42FE);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x42FF);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4300);
write_cmos_sensor(0x6F12,0x29);
write_cmos_sensor_twobyte(0x602A,0x4305);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4306);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4307);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4308);
write_cmos_sensor(0x6F12,0x00);


//// ADLC setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x25D8);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// 0011 lkh 20140318
write_cmos_sensor_twobyte(0x6F12,0x0001);	// 0011 lkh 20140318 // ??
write_cmos_sensor_twobyte(0x602A,0x26C8);
write_cmos_sensor(0x6F12,0x00);

//// Clock setting, related with ATOP
write_cmos_sensor_twobyte(0x602A,0x1C64);
write_cmos_sensor(0x6F12,0x01);	// For 600MHz CCLK
write_cmos_sensor_twobyte(0x602A,0x1C65);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x1CA4);
write_cmos_sensor_twobyte(0x6F12,0x0064);	// DBR freq : 100MHz

write_cmos_sensor_twobyte(0x602A,0x1520);
write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x602A,0x1524);
write_cmos_sensor_twobyte(0x6F12,0x1000);

// PSP BDS/HVbin
write_cmos_sensor_twobyte(0x602A,0x0F9A);
write_cmos_sensor(0x6F12,0x01);	// BDS

// TG Readout
write_cmos_sensor_twobyte(0x602A,0x14EC);
write_cmos_sensor(0x6F12,0x01);	// 0:Normal 1:CSR

// Int.Time
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0202);
write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x602A,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x021E);
write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x602A,0x021C);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// Analog Gain
write_cmos_sensor_twobyte(0x602A,0x0204);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);

//CASE : 6M(2x2) 4:3 2816x2112
write_cmos_sensor_twobyte(0x602A,0x0344);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x161B);
write_cmos_sensor_twobyte(0x602A,0x0346);
write_cmos_sensor_twobyte(0x6F12,0x0008);
write_cmos_sensor_twobyte(0x602A,0x034A);
write_cmos_sensor_twobyte(0x6F12,0x1097);
write_cmos_sensor_twobyte(0x6F12,0x0B00);
write_cmos_sensor_twobyte(0x6F12,0x0840);
write_cmos_sensor_twobyte(0x602A,0x0342);
write_cmos_sensor_twobyte(0x6F12,0x1AD0);
write_cmos_sensor_twobyte(0x602A,0x0340);
write_cmos_sensor_twobyte(0x6F12,0x08FC);//091A

write_cmos_sensor_twobyte(0x602A,0x0900);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0901);
write_cmos_sensor(0x6F12,0x22);
write_cmos_sensor_twobyte(0x602A,0x0380);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x0404);
write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

// CropAndPad
write_cmos_sensor_twobyte(0x602A,0x0408);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

///////////////////////////////////////////////////////////
//PLL Sys = 560 , Sec = 1392
write_cmos_sensor_twobyte(0x602A,0x0136);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x602A,0x0304);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x00E1);
write_cmos_sensor_twobyte(0x602A,0x030C);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0302);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0300);
write_cmos_sensor_twobyte(0x6F12,0x0005);
write_cmos_sensor_twobyte(0x602A,0x030A);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0308);
write_cmos_sensor_twobyte(0x6F12,0x000A);

write_cmos_sensor_twobyte(0x602A,0x0318);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x00A5);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0316);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0314);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x030E);
write_cmos_sensor_twobyte(0x6F12,0x0004);
write_cmos_sensor_twobyte(0x6F12,0x00F2);	//00E9
write_cmos_sensor_twobyte(0x6F12,0x0000);

// OIF Setting
write_cmos_sensor_twobyte(0x602A,0x0111);
write_cmos_sensor(0x6F12,0x02);	// PVI, 2: MIPI
write_cmos_sensor_twobyte(0x602A,0x0114);
write_cmos_sensor(0x6F12,0x03);
write_cmos_sensor_twobyte(0x602A,0x0112);
write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

// AF
write_cmos_sensor_twobyte(0x602A,0x0B0E);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x3069);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0B08);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x0B05);
write_cmos_sensor(0x6F12,0x00);

// Stream On
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x01);



mDELAY(10);



}    /*    preview_setting  */




static void preview_setting_11(void)
{

//$MIPI[Width:2816,Height:2112,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:2816,Height:2112,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]



// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);


////1. ADLC setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x168C);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning



//2. Clock setting, related with ATOP remove
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E24);
write_cmos_sensor(0x6F12,0x01);	// For 600MHz CCLK

//3. ATOP Setting (Option)
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E9C);
write_cmos_sensor_twobyte(0x6F12,0x0081);	// RDV option // binning

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E47);
write_cmos_sensor(0x6F12,0x00);	// CDS current // EVT1.1 0429 lkh
write_cmos_sensor_twobyte(0x602A,0x0E48);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E49);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E4A);
write_cmos_sensor(0x6F12,0x03);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x0E4B);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4C);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4D);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4E);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x0E4F);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E50);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E51);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E52);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current


// EVT1.1 TnP
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0xAB00);
write_cmos_sensor(0x6F12,0x00);

//Af correction for 4x4 binning
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x14C9);
write_cmos_sensor(0x6F12,0x10);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CA);
write_cmos_sensor(0x6F12,0x10);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CD);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CE);
write_cmos_sensor(0x6F12,0x05);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CF);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D0);
write_cmos_sensor(0x6F12,0x09);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D1);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D2);
write_cmos_sensor(0x6F12,0x09);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D3);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D4);
write_cmos_sensor(0x6F12,0x0D);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14DE);
write_cmos_sensor_twobyte(0x6F12,0x1BE4); // EVT1.1
write_cmos_sensor_twobyte(0x6F12,0xB14E);	// EVT1.1


//MSM gain for 4x4 binning
write_cmos_sensor_twobyte(0x602A,0x427A);
write_cmos_sensor(0x6F12,0x04);
write_cmos_sensor_twobyte(0x602A,0x430D);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x430E);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x430F);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4310);
write_cmos_sensor(0x6F12,0x29);
write_cmos_sensor_twobyte(0x602A,0x4315);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x4316);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4317);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4318);
write_cmos_sensor(0x6F12,0x00);



// AF
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0B0E);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x3069);
write_cmos_sensor(0x6F12,0x01);

// BPC
write_cmos_sensor_twobyte(0x602A,0x0B08);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x0B05);
write_cmos_sensor(0x6F12,0x01);


// WDR
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0216);
write_cmos_sensor(0x6F12,0x00); //1	// smiaRegs_rw_wdr_multiple_exp_mode, EVT1.1
write_cmos_sensor_twobyte(0x602A,0x021B);
write_cmos_sensor(0x6F12,0x00);	// smiaRegs_rw_wdr_exposure_order, EVT1.1


write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0202);
write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x602A,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// Analog Gain
write_cmos_sensor_twobyte(0x602A,0x0204);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);



//4. CASE : 6M(2x2) 4:3 2816x2112
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0344);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x161B);
write_cmos_sensor_twobyte(0x602A,0x0346);
write_cmos_sensor_twobyte(0x6F12,0x0008);
write_cmos_sensor_twobyte(0x602A,0x034A);
write_cmos_sensor_twobyte(0x6F12,0x1097);
write_cmos_sensor_twobyte(0x6F12,0x0B00);
write_cmos_sensor_twobyte(0x6F12,0x0840);
write_cmos_sensor_twobyte(0x602A,0x0342);
write_cmos_sensor_twobyte(0x6F12,0x1AD0);
write_cmos_sensor_twobyte(0x602A,0x0340);
write_cmos_sensor_twobyte(0x6F12,0x08FC);	// 091A

write_cmos_sensor_twobyte(0x602A,0x0900);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0901);
write_cmos_sensor(0x6F12,0x22);
write_cmos_sensor_twobyte(0x602A,0x0380);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x06E0);
write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x602A,0x06E4);
write_cmos_sensor_twobyte(0x6F12,0x1000);

// PSP BDS/HVbin
write_cmos_sensor_twobyte(0x602A,0x0EFA);
write_cmos_sensor(0x6F12,0x01);	// BDS
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0404);
write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

// CropAndPad
write_cmos_sensor_twobyte(0x602A,0x0408);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

///////////////////////////////////////////////////////////
//PLL Sys = 560 , Sec = 1392
write_cmos_sensor_twobyte(0x602A,0x0136);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x602A,0x0304);
write_cmos_sensor_twobyte(0x6F12,0x0005);	// 3->5 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0173);	// 225->371 EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0x030C);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0302);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0300);
write_cmos_sensor_twobyte(0x6F12,0x0005);
write_cmos_sensor_twobyte(0x602A,0x030A);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0308);
write_cmos_sensor_twobyte(0x6F12,0x000A);

write_cmos_sensor_twobyte(0x602A,0x0318);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x00A4);	// A5 -> A4 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0316);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0314);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x030E);
write_cmos_sensor_twobyte(0x6F12,0x0004);
write_cmos_sensor_twobyte(0x6F12,0x00F2);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// OIF Setting
write_cmos_sensor_twobyte(0x602A,0x0111);
write_cmos_sensor(0x6F12,0x02);	// PVI, 2: MIPI
write_cmos_sensor_twobyte(0x602A,0x0114);
write_cmos_sensor(0x6F12,0x03);
write_cmos_sensor_twobyte(0x602A,0x0112);
write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

write_cmos_sensor_twobyte(0x602A,0xB0CA);
write_cmos_sensor_twobyte(0x6F12,0x7E00);	// M_DPHYCTL[30:25] = 6'11_1111 // EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0xB136);
write_cmos_sensor_twobyte(0x6F12,0x2000);	// B_DPHYCTL[62:60] = 3'b010 // EVT1.1 0429

write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0xF4A0);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0xF4A2);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// Stream On
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x01);





mDELAY(10);



}    /*    preview_setting  */

static void capture_setting_WDR(kal_uint16 currefps)
{
//$MIPI[Width:5632,Height:4224,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:5632,Height:4224,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]

    LOG_INF("Capture WDR(fps = %d)",currefps);
	if(currefps == 214)
 	{
		////1. ADLC setting
		write_cmos_sensor_twobyte(0x6028,0x2000);
		write_cmos_sensor_twobyte(0x602A,0x168C);
		write_cmos_sensor_twobyte(0x6F12,0x0010);
		write_cmos_sensor_twobyte(0x6F12,0x0011);
		write_cmos_sensor_twobyte(0x6F12,0x0011);
		
		//2. Clock setting, related with ATOP remove
		write_cmos_sensor_twobyte(0x602A,0x0E24);
		write_cmos_sensor_twobyte(0x6F12,0x0001); // For 450MHz CCLK
		
		//3. ATOP Setting (Option)
		write_cmos_sensor_twobyte(0x602A,0x0E9C);
		write_cmos_sensor_twobyte(0x6F12,0x0084);	// RDV option
		
		//// CDS Current Setting
		write_cmos_sensor_twobyte(0x602A,0x0E46);
		write_cmos_sensor_twobyte(0x6F12,0x0301); // CDS current // EVT1.1 0429 lkh
		write_cmos_sensor_twobyte(0x602A,0x0E48);
		write_cmos_sensor_twobyte(0x6F12,0x0303); // CDS current
		write_cmos_sensor_twobyte(0x602A,0x0E4A);
		write_cmos_sensor_twobyte(0x6F12,0x0306); // CDS current
		write_cmos_sensor_twobyte(0x602A,0x0E4C);
		write_cmos_sensor_twobyte(0x6F12,0x0a0a); // Pixel Bias current
		write_cmos_sensor_twobyte(0x602A,0x0E4E);
		write_cmos_sensor_twobyte(0x6F12,0x0A0F); // Pixel Bias current
		write_cmos_sensor_twobyte(0x602A,0x0E50);
		write_cmos_sensor_twobyte(0x6F12,0x0707); // Pixel Boost current
		write_cmos_sensor_twobyte(0x602A,0x0E52);
		write_cmos_sensor_twobyte(0x6F12,0x0700); // Pixel Boost current
		
		// EVT1.1 TnP
		write_cmos_sensor_twobyte(0x6028,0x2001);
		write_cmos_sensor_twobyte(0x602A,0xAB00);
		write_cmos_sensor_twobyte(0x6F12,0x0000);
		
		
		//Af correction for 4x4 binning
		write_cmos_sensor_twobyte(0x6028,0x2000);
		write_cmos_sensor_twobyte(0x602A,0x14C8);
		write_cmos_sensor_twobyte(0x6F12,0x0010);
		write_cmos_sensor_twobyte(0x602A,0x14CA);
		write_cmos_sensor_twobyte(0x6F12,0x1004);
		write_cmos_sensor_twobyte(0x602A,0x14CC);
		write_cmos_sensor_twobyte(0x6F12,0x0406);
		write_cmos_sensor_twobyte(0x602A,0x14CE);
		write_cmos_sensor_twobyte(0x6F12,0x0506);
		write_cmos_sensor_twobyte(0x602A,0x14D0);
		write_cmos_sensor_twobyte(0x6F12,0x0906);
		write_cmos_sensor_twobyte(0x602A,0x14D2);
		write_cmos_sensor_twobyte(0x6F12,0x0906);
		write_cmos_sensor_twobyte(0x602A,0x14D4);
		write_cmos_sensor_twobyte(0x6F12,0x0D0A);
		write_cmos_sensor_twobyte(0x602A,0x14DE);
		write_cmos_sensor_twobyte(0x6F12,0x1BE4);
		write_cmos_sensor_twobyte(0x6F12,0xB14E);
		
		//MSM gain for 4x4 binning
		write_cmos_sensor_twobyte(0x602A,0x427A);
		write_cmos_sensor_twobyte(0x6F12,0x0440);
		write_cmos_sensor_twobyte(0x602A,0x430C);
		write_cmos_sensor_twobyte(0x6F12,0x1B20);
		write_cmos_sensor_twobyte(0x602A,0x430E);
		write_cmos_sensor_twobyte(0x6F12,0x2128);
		write_cmos_sensor_twobyte(0x602A,0x4310);
		write_cmos_sensor_twobyte(0x6F12,0x2900);
		write_cmos_sensor_twobyte(0x602A,0x4314);
		write_cmos_sensor_twobyte(0x6F12,0x0000);
		write_cmos_sensor_twobyte(0x602A,0x4316);
		write_cmos_sensor_twobyte(0x6F12,0x0000);
		write_cmos_sensor_twobyte(0x602A,0x4318);
		write_cmos_sensor_twobyte(0x6F12,0x0000);
		
		// AF
		write_cmos_sensor_twobyte(0x6028,0x4000);
		write_cmos_sensor_twobyte(0x0B0E,0x0000);
		//
		write_cmos_sensor_twobyte(0x3068, 0x0001);
		write_cmos_sensor_twobyte(0x0B08, 0x0000);
		write_cmos_sensor_twobyte(0x0B04, 0x0101);
		//write_cmos_sensor_twobyte(0x0216, 0x0101); /*For WDR*/
		//write_cmos_sensor_twobyte(0x0218, 0x0101);
		write_cmos_sensor_twobyte(0x021A, 0x0100);
		write_cmos_sensor_twobyte(0x0202, 0x0400);
		write_cmos_sensor_twobyte(0x0200, 0x0001);
		write_cmos_sensor_twobyte(0x0086, 0x0200);
		write_cmos_sensor_twobyte(0x0204, 0x0020);
		write_cmos_sensor_twobyte(0x0344, 0x0008);
		write_cmos_sensor_twobyte(0x0348, 0x161F);
		write_cmos_sensor_twobyte(0x0346, 0x0010);
		write_cmos_sensor_twobyte(0x034A, 0x1097);
		write_cmos_sensor_twobyte(0x034C, 0x1600);
		write_cmos_sensor_twobyte(0x034E, 0x1080);
		write_cmos_sensor_twobyte(0x0342, 0x1DD8);
		write_cmos_sensor_twobyte(0x0340, 0x10FE);
		write_cmos_sensor_twobyte(0x0900, 0x0111);
		write_cmos_sensor_twobyte(0x0380, 0x0001);
		write_cmos_sensor_twobyte(0x0382, 0x0001);
		write_cmos_sensor_twobyte(0x0384, 0x0001);
		write_cmos_sensor_twobyte(0x0386, 0x0001);
		write_cmos_sensor_twobyte(0x6028, 0x2000);
		write_cmos_sensor_twobyte(0x602A, 0x6944);
		write_cmos_sensor_twobyte(0x6F12, 0x0000);
		write_cmos_sensor_twobyte(0x602A, 0x06A4);
		write_cmos_sensor_twobyte(0x6F12, 0x0080);
		write_cmos_sensor_twobyte(0x602A, 0x06AC);
		write_cmos_sensor_twobyte(0x6F12, 0x0100);
		write_cmos_sensor_twobyte(0x602A, 0x06E0);
		write_cmos_sensor_twobyte(0x6F12, 0x0200);
		write_cmos_sensor_twobyte(0x602A, 0x06E4);
		write_cmos_sensor_twobyte(0x6F12, 0x1000);
		write_cmos_sensor_twobyte(0x6028, 0x4000);
		write_cmos_sensor_twobyte(0x0408, 0x000E);
		write_cmos_sensor_twobyte(0x040A, 0x0000);
		write_cmos_sensor_twobyte(0x0136, 0x1800);
		write_cmos_sensor_twobyte(0x0304, 0x0005);
		write_cmos_sensor_twobyte(0x0306, 0x0173);
		write_cmos_sensor_twobyte(0x030C, 0x0000);
		write_cmos_sensor_twobyte(0x0302, 0x0001);
		write_cmos_sensor_twobyte(0x0300, 0x0005);
		write_cmos_sensor_twobyte(0x030A, 0x0001);
		write_cmos_sensor_twobyte(0x0308, 0x000A);
		write_cmos_sensor_twobyte(0x0318, 0x0003);
		write_cmos_sensor_twobyte(0x031A, 0x00A4);
		write_cmos_sensor_twobyte(0x031C, 0x0001);
		write_cmos_sensor_twobyte(0x0316, 0x0001);
		write_cmos_sensor_twobyte(0x0314, 0x0003);
		write_cmos_sensor_twobyte(0x030E, 0x0004);
		write_cmos_sensor_twobyte(0x0310, 0x00F2);
		write_cmos_sensor_twobyte(0x0312, 0x0000);
		write_cmos_sensor_twobyte(0x0110, 0x0002);
		write_cmos_sensor_twobyte(0x0114, 0x0300);
		write_cmos_sensor_twobyte(0x0112, 0x0A0A);
		write_cmos_sensor_twobyte(0xB0CA, 0x7E00);
		write_cmos_sensor_twobyte(0xB136, 0x2000);
		write_cmos_sensor_twobyte(0xD0D0, 0x1000);
		
		if(imgsensor.hdr_mode == 9)
		{
			/*it would write 0x216 = 0x1, 0x217=0x00*/
			/*0x216=1 , Enable WDR*/
			/*0x217=0x00, Use Manual mode to set short /long exp */
			write_cmos_sensor_twobyte(0x0216, 0x0100); /*For WDR*/
			write_cmos_sensor_twobyte(0x0218, 0x0101);
			write_cmos_sensor_twobyte(0x602A, 0x6944); 
			write_cmos_sensor_twobyte(0x6F12, 0x0000);
		}
		else
		{
			write_cmos_sensor_twobyte(0x0216, 0x0000);
			write_cmos_sensor_twobyte(0x0218, 0x0000);
		}
		/*Streaming  output */
		write_cmos_sensor_twobyte(0x0100, 0x0100);

	}
	else
	{
		////1. ADLC setting
		write_cmos_sensor_twobyte(0x6028,0x2000);
		write_cmos_sensor_twobyte(0x602A,0x168C);
		write_cmos_sensor_twobyte(0x6F12,0x0010);
		write_cmos_sensor_twobyte(0x6F12,0x0011);
		write_cmos_sensor_twobyte(0x6F12,0x0011);
		
		//2. Clock setting, related with ATOP remove
		write_cmos_sensor_twobyte(0x602A,0x0E24);
		write_cmos_sensor_twobyte(0x6F12,0x0101); /*for 24fps*/
		
		//3. ATOP Setting (Option)
		write_cmos_sensor_twobyte(0x602A,0x0E9C);
		write_cmos_sensor_twobyte(0x6F12,0x0084);	// RDV option
		
		//// CDS Current Setting
		write_cmos_sensor_twobyte(0x602A,0x0E46);
		write_cmos_sensor_twobyte(0x6F12,0x0301); // CDS current // EVT1.1 0429 lkh
		write_cmos_sensor_twobyte(0x602A,0x0E48);
		write_cmos_sensor_twobyte(0x6F12,0x0303); // CDS current
		write_cmos_sensor_twobyte(0x602A,0x0E4A);
		write_cmos_sensor_twobyte(0x6F12,0x0306); // CDS current
		write_cmos_sensor_twobyte(0x602A,0x0E4C);
		write_cmos_sensor_twobyte(0x6F12,0x0a0a); // Pixel Bias current
		write_cmos_sensor_twobyte(0x602A,0x0E4E);
		write_cmos_sensor_twobyte(0x6F12,0x0A0F); // Pixel Bias current
		write_cmos_sensor_twobyte(0x602A,0x0E50);
		write_cmos_sensor_twobyte(0x6F12,0x0707); // Pixel Boost current
		write_cmos_sensor_twobyte(0x602A,0x0E52);
		write_cmos_sensor_twobyte(0x6F12,0x0700); // Pixel Boost current
		
		// EVT1.1 TnP
		write_cmos_sensor_twobyte(0x6028,0x2001);
		write_cmos_sensor_twobyte(0x602A,0xAB00);
		write_cmos_sensor_twobyte(0x6F12,0x0000);
		
		
		//Af correction for 4x4 binning
		write_cmos_sensor_twobyte(0x6028,0x2000);
		write_cmos_sensor_twobyte(0x602A,0x14C8);
		write_cmos_sensor_twobyte(0x6F12,0x0010);
		write_cmos_sensor_twobyte(0x602A,0x14CA);
		write_cmos_sensor_twobyte(0x6F12,0x1004);
		write_cmos_sensor_twobyte(0x602A,0x14CC);
		write_cmos_sensor_twobyte(0x6F12,0x0406);
		write_cmos_sensor_twobyte(0x602A,0x14CE);
		write_cmos_sensor_twobyte(0x6F12,0x0506);
		write_cmos_sensor_twobyte(0x602A,0x14D0);
		write_cmos_sensor_twobyte(0x6F12,0x0906);
		write_cmos_sensor_twobyte(0x602A,0x14D2);
		write_cmos_sensor_twobyte(0x6F12,0x0906);
		write_cmos_sensor_twobyte(0x602A,0x14D4);
		write_cmos_sensor_twobyte(0x6F12,0x0D0A);
		write_cmos_sensor_twobyte(0x602A,0x14DE);
		write_cmos_sensor_twobyte(0x6F12,0x1BE4);
		write_cmos_sensor_twobyte(0x6F12,0xB14E);
		
		//MSM gain for 4x4 binning
		write_cmos_sensor_twobyte(0x602A,0x427A);
		write_cmos_sensor_twobyte(0x6F12,0x0440);
		write_cmos_sensor_twobyte(0x602A,0x430C);
		write_cmos_sensor_twobyte(0x6F12,0x1B20);
		write_cmos_sensor_twobyte(0x602A,0x430E);
		write_cmos_sensor_twobyte(0x6F12,0x2128);
		write_cmos_sensor_twobyte(0x602A,0x4310);
		write_cmos_sensor_twobyte(0x6F12,0x2900);
		write_cmos_sensor_twobyte(0x602A,0x4314);
		write_cmos_sensor_twobyte(0x6F12,0x0000);
		write_cmos_sensor_twobyte(0x602A,0x4316);
		write_cmos_sensor_twobyte(0x6F12,0x0000);
		write_cmos_sensor_twobyte(0x602A,0x4318);
		write_cmos_sensor_twobyte(0x6F12,0x0000);
		
		// AF
		write_cmos_sensor_twobyte(0x6028,0x4000);
		write_cmos_sensor_twobyte(0x0B0E,0x0100); /*for 24fps*/
		//
		write_cmos_sensor_twobyte(0x3068, 0x0001);
		write_cmos_sensor_twobyte(0x0B08, 0x0000);
		write_cmos_sensor_twobyte(0x0B04, 0x0101);
		//write_cmos_sensor_twobyte(0x0216, 0x0101); /*For WDR*/
		//write_cmos_sensor_twobyte(0x0218, 0x0101);
		write_cmos_sensor_twobyte(0x021A, 0x0100);
		write_cmos_sensor_twobyte(0x0202, 0x0400);
		write_cmos_sensor_twobyte(0x0200, 0x0001);
		write_cmos_sensor_twobyte(0x0086, 0x0200);
		write_cmos_sensor_twobyte(0x0204, 0x0020);
		write_cmos_sensor_twobyte(0x0344, 0x0010);/*for 24fps*/
		write_cmos_sensor_twobyte(0x0348, 0x160F);/*for 24fps*/
		write_cmos_sensor_twobyte(0x0346, 0x0010);
		write_cmos_sensor_twobyte(0x034A, 0x108f);/*for 24fps*/
		write_cmos_sensor_twobyte(0x034C, 0x1600);
		write_cmos_sensor_twobyte(0x034E, 0x1080);
		write_cmos_sensor_twobyte(0x0342, 0x1ACC);/*for 24fps*/
		write_cmos_sensor_twobyte(0x0340, 0x10E4);/*for 24fps*/
		write_cmos_sensor_twobyte(0x0900, 0x0111);
		write_cmos_sensor_twobyte(0x0380, 0x0001);
		write_cmos_sensor_twobyte(0x0382, 0x0001);
		write_cmos_sensor_twobyte(0x0384, 0x0001);
		write_cmos_sensor_twobyte(0x0386, 0x0001);
		write_cmos_sensor_twobyte(0x6028, 0x2000);
		
		//write_cmos_sensor_twobyte(0x602A, 0x6944);/*For WDR*/
		//write_cmos_sensor_twobyte(0x6F12, 0x0000);
		write_cmos_sensor_twobyte(0x602A, 0x06A4);
		write_cmos_sensor_twobyte(0x6F12, 0x0080);
		write_cmos_sensor_twobyte(0x602A, 0x06AC);
		write_cmos_sensor_twobyte(0x6F12, 0x0100);
		write_cmos_sensor_twobyte(0x602A, 0x06E0);
		write_cmos_sensor_twobyte(0x6F12, 0x0200);
		write_cmos_sensor_twobyte(0x602A, 0x06E4);
		write_cmos_sensor_twobyte(0x6F12, 0x1000);
		write_cmos_sensor_twobyte(0x6028, 0x4000);
		write_cmos_sensor_twobyte(0x0408, 0x0000);/*for 24fps*/
		write_cmos_sensor_twobyte(0x040A, 0x0000);
		write_cmos_sensor_twobyte(0x0136, 0x1800);
		write_cmos_sensor_twobyte(0x0304, 0x0005);
		write_cmos_sensor_twobyte(0x0306, 0x0173);
		write_cmos_sensor_twobyte(0x030C, 0x0000);
		write_cmos_sensor_twobyte(0x0302, 0x0001);
		write_cmos_sensor_twobyte(0x0300, 0x0005);
		write_cmos_sensor_twobyte(0x030A, 0x0001);
		write_cmos_sensor_twobyte(0x0308, 0x000A);
		write_cmos_sensor_twobyte(0x0318, 0x0003);
		write_cmos_sensor_twobyte(0x031A, 0x00A4);
		write_cmos_sensor_twobyte(0x031C, 0x0001);
		write_cmos_sensor_twobyte(0x0316, 0x0001);
		write_cmos_sensor_twobyte(0x0314, 0x0003);
		write_cmos_sensor_twobyte(0x030E, 0x0004);
		write_cmos_sensor_twobyte(0x0310, 0x0110);/*for 24fps*/
		write_cmos_sensor_twobyte(0x0312, 0x0000);
		write_cmos_sensor_twobyte(0x0110, 0x0002);
		write_cmos_sensor_twobyte(0x0114, 0x0300);
		write_cmos_sensor_twobyte(0x0112, 0x0A0A);
		write_cmos_sensor_twobyte(0xB0CA, 0x7E00);
		write_cmos_sensor_twobyte(0xB136, 0x2000);
		write_cmos_sensor_twobyte(0xD0D0, 0x1000);
		
		if(imgsensor.hdr_mode == 9)
		{
			/*it would write 0x216 = 0x1, 0x217=0x00*/
			/*0x216=1 , Enable WDR*/
			/*0x217=0x00, Use Manual mode to set short /long exp */
			write_cmos_sensor_twobyte(0x0216, 0x0100); /*For WDR*/
			write_cmos_sensor_twobyte(0x0218, 0x0101);
			write_cmos_sensor_twobyte(0x602A, 0x6944); 
			write_cmos_sensor_twobyte(0x6F12, 0x0000);
		}
		else
		{
			write_cmos_sensor_twobyte(0x0216, 0x0000);
			write_cmos_sensor_twobyte(0x0218, 0x0000);
		}
		/*Streaming  output */
		write_cmos_sensor_twobyte(0x0100, 0x0100);


	}
	
}


static void normal_video_setting_10(kal_uint16 currefps)
{
	//$MIPI[Width:5632,Height:3168,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
	//$MV1[MCLK:24,Width:5632,Height:3168,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]

// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x1C87);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C88);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C89);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C8A);
write_cmos_sensor(0x6F12,0x03);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x1C8B);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8C);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8D);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8E);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x1C8F);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C90);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C91);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C92);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current

write_cmos_sensor_twobyte(0x602A,0x426A);
write_cmos_sensor(0x6F12,0x04);
write_cmos_sensor_twobyte(0x602A,0x42FD);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x42FE);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x42FF);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4300);
write_cmos_sensor(0x6F12,0x29);
write_cmos_sensor_twobyte(0x602A,0x4305);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4306);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4307);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4308);
write_cmos_sensor(0x6F12,0x00);


	//// ADLC setting
	write_cmos_sensor_twobyte(0x6028,0x2000);
	write_cmos_sensor_twobyte(0x602A,0x25D8);
	write_cmos_sensor_twobyte(0x6F12,0x0010);
	write_cmos_sensor_twobyte(0x6F12,0x0011);
	write_cmos_sensor_twobyte(0x6F12,0x0011);
	write_cmos_sensor_twobyte(0x602A,0x26C8);
	write_cmos_sensor(0x6F12,0x00);

	//// Clock setting, related with ATOP
	write_cmos_sensor_twobyte(0x602A,0x1C64);
	write_cmos_sensor(0x6F12,0x00); // For 450MHz CCLK
	write_cmos_sensor_twobyte(0x602A,0x1C65);
	write_cmos_sensor(0x6F12,0x01);
	write_cmos_sensor_twobyte(0x602A,0x1CA4);
	write_cmos_sensor_twobyte(0x6F12,0x0064);	// DBR freq : 100MHz

	//write_cmos_sensor_twobyte(0x602A,0x1520);
	//write_cmos_sensor_twobyte(0x6F12,0x0200);
	//write_cmos_sensor_twobyte(0x602A,0x1524);
	//write_cmos_sensor_twobyte(0x6F12,0x1000);

	// PSP BDS/HVbin
	write_cmos_sensor_twobyte(0x602A,0x0F9A);
	write_cmos_sensor(0x6F12,0x01); // BDS

	// TG Readout
	write_cmos_sensor_twobyte(0x602A,0x14EC);
	write_cmos_sensor(0x6F12,0x01); // 0:Normal 1:CSR

		// FOB Setting
  write_cmos_sensor_twobyte(0x602A,0x14E7);
	write_cmos_sensor(0x6F12,0x08);
  write_cmos_sensor_twobyte(0x602A,0x14E8);
	write_cmos_sensor(0x6F12,0x0C);

	// Int.Time
	write_cmos_sensor_twobyte(0x6028,0x4000);
	write_cmos_sensor_twobyte(0x602A,0x0202);
	write_cmos_sensor_twobyte(0x6F12,0x0400);
	write_cmos_sensor_twobyte(0x602A,0x0200);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x021E);
	write_cmos_sensor_twobyte(0x6F12,0x0400);
	write_cmos_sensor_twobyte(0x602A,0x021C);
	write_cmos_sensor_twobyte(0x6F12,0x0001);

	// Analog Gain
	write_cmos_sensor_twobyte(0x602A,0x0204);
	write_cmos_sensor_twobyte(0x6F12,0x0020);
	write_cmos_sensor_twobyte(0x6F12,0x0020);

	//CASE : 18M 16:9 5632X3168
	write_cmos_sensor_twobyte(0x602A,0x0344);
	write_cmos_sensor_twobyte(0x6F12,0x0008);
	write_cmos_sensor_twobyte(0x602A,0x0348);
	write_cmos_sensor_twobyte(0x6F12,0x161F);
	write_cmos_sensor_twobyte(0x602A,0x0346);
	write_cmos_sensor_twobyte(0x6F12,0x021C);	//0220
	write_cmos_sensor_twobyte(0x602A,0x034A);
	write_cmos_sensor_twobyte(0x6F12,0x0E83);	//0E87
	write_cmos_sensor_twobyte(0x6F12,0x1600);
	write_cmos_sensor_twobyte(0x6F12,0x0C60);
	write_cmos_sensor_twobyte(0x602A,0x0342);
	write_cmos_sensor_twobyte(0x6F12,0x1DD8);	//1ECC
	write_cmos_sensor_twobyte(0x602A,0x0340);
	write_cmos_sensor_twobyte(0x6F12,0x0CDC);	//0DB8

	write_cmos_sensor_twobyte(0x602A,0x0900);
	write_cmos_sensor(0x6F12,0x01);
	write_cmos_sensor_twobyte(0x602A,0x0901);
	write_cmos_sensor(0x6F12,0x11);
	write_cmos_sensor_twobyte(0x602A,0x0380);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x6F12,0x0001);

	write_cmos_sensor_twobyte(0x602A,0x0404);
	write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

	// CropAndPad
	write_cmos_sensor_twobyte(0x602A,0x0408);
	write_cmos_sensor_twobyte(0x6F12,0x000E);
	write_cmos_sensor_twobyte(0x6F12,0x0000);

	///////////////////////////////////////////////////////////
	//PLL Sys = 560 , Sec = 1392
	write_cmos_sensor_twobyte(0x602A,0x0136);
	write_cmos_sensor_twobyte(0x6F12,0x1800);
	write_cmos_sensor_twobyte(0x602A,0x0304);
	write_cmos_sensor_twobyte(0x6F12,0x0003);
	write_cmos_sensor_twobyte(0x6F12,0x00E1);
	write_cmos_sensor_twobyte(0x602A,0x030C);
	write_cmos_sensor_twobyte(0x6F12,0x0000);
	write_cmos_sensor_twobyte(0x602A,0x0302);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x0300);
	write_cmos_sensor_twobyte(0x6F12,0x0005);
	write_cmos_sensor_twobyte(0x602A,0x030A);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x0308);
	write_cmos_sensor_twobyte(0x6F12,0x000A);

	write_cmos_sensor_twobyte(0x602A,0x0318);
	write_cmos_sensor_twobyte(0x6F12,0x0003);
	write_cmos_sensor_twobyte(0x6F12,0x00A5);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x0316);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x0314);
	write_cmos_sensor_twobyte(0x6F12,0x0003);

	write_cmos_sensor_twobyte(0x602A,0x030E);
	write_cmos_sensor_twobyte(0x6F12,0x0004);
	write_cmos_sensor_twobyte(0x6F12,0x00F2);	//00E9
	write_cmos_sensor_twobyte(0x6F12,0x0000);

	// OIF Setting
	write_cmos_sensor_twobyte(0x602A,0x0111);
	write_cmos_sensor(0x6F12,0x02); // PVI, 2: MIPI
	write_cmos_sensor_twobyte(0x602A,0x0114);
	write_cmos_sensor(0x6F12,0x03);
	write_cmos_sensor_twobyte(0x602A,0x0112);
	write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

	// AF
	write_cmos_sensor_twobyte(0x602A,0x0B0E);
	write_cmos_sensor(0x6F12,0x00);
	write_cmos_sensor_twobyte(0x602A,0x3069);
	write_cmos_sensor(0x6F12,0x01);
	write_cmos_sensor_twobyte(0x602A,0x0B08);
	write_cmos_sensor(0x6F12,0x00);
	write_cmos_sensor_twobyte(0x602A,0x0B05);
	write_cmos_sensor(0x6F12,0x00);

	// Stream On
	write_cmos_sensor_twobyte(0x602A,0x0100);
	write_cmos_sensor(0x6F12,0x01);


mDELAY(10);

}

static void normal_video_setting_11(kal_uint16 currefps)
{
//$MIPI[Width:5632,Height:3168,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:5632,Height:3168,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]


// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);

////1. ADLC setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x168C);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x0011);
write_cmos_sensor_twobyte(0x6F12,0x0011);



//2. Clock setting, related with ATOP remove
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E24);
write_cmos_sensor(0x6F12,0x00);	// For 450MHz CCLK

//3. ATOP Setting (Option)
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E9C);
write_cmos_sensor_twobyte(0x6F12,0x0084);	// RDV option

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E47);
write_cmos_sensor(0x6F12,0x01);	// CDS current // EVT1.1 0429 lkh
write_cmos_sensor_twobyte(0x602A,0x0E48);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E49);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E4A);
write_cmos_sensor(0x6F12,0x03);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x0E4B);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4C);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4D);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4E);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x0E4F);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E50);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E51);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E52);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current


// EVT1.1 TnP
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0xAB00);
write_cmos_sensor(0x6F12,0x00);

//Af correction for 4x4 binning
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x14C9);
write_cmos_sensor(0x6F12,0x10);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CA);
write_cmos_sensor(0x6F12,0x10);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CD);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CE);
write_cmos_sensor(0x6F12,0x05);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CF);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D0);
write_cmos_sensor(0x6F12,0x09);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D1);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D2);
write_cmos_sensor(0x6F12,0x09);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D3);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D4);
write_cmos_sensor(0x6F12,0x0D);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14DE);
write_cmos_sensor_twobyte(0x6F12,0x1BE4); // EVT1.1
write_cmos_sensor_twobyte(0x6F12,0xB14E);	// EVT1.1


//MSM gain for 4x4 binning
write_cmos_sensor_twobyte(0x602A,0x427A);
write_cmos_sensor(0x6F12,0x04);
write_cmos_sensor_twobyte(0x602A,0x430D);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x430E);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x430F);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4310);
write_cmos_sensor(0x6F12,0x29);
write_cmos_sensor_twobyte(0x602A,0x4315);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x4316);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4317);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4318);
write_cmos_sensor(0x6F12,0x00);



// AF
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0B0E);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x3069);
write_cmos_sensor(0x6F12,0x01);

// BPC
write_cmos_sensor_twobyte(0x602A,0x0B08);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x0B05);
write_cmos_sensor(0x6F12,0x01);


// WDR
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0216);
write_cmos_sensor(0x6F12,0x00); //1	// smiaRegs_rw_wdr_multiple_exp_mode, EVT1.1
write_cmos_sensor_twobyte(0x602A,0x021B);
write_cmos_sensor(0x6F12,0x00);	// smiaRegs_rw_wdr_exposure_order, EVT1.1


write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0202);
write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x602A,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// Analog Gain
write_cmos_sensor_twobyte(0x602A,0x0204);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);


//4. CASE : 18M 16:9 5632X3168
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0344);
write_cmos_sensor_twobyte(0x6F12,0x0008);
write_cmos_sensor_twobyte(0x602A,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x161F);
write_cmos_sensor_twobyte(0x602A,0x0346);
write_cmos_sensor_twobyte(0x6F12,0x021C);
write_cmos_sensor_twobyte(0x602A,0x034A);
write_cmos_sensor_twobyte(0x6F12,0x0E83);
write_cmos_sensor_twobyte(0x6F12,0x1600);
write_cmos_sensor_twobyte(0x6F12,0x0C60);
write_cmos_sensor_twobyte(0x602A,0x0342);
write_cmos_sensor_twobyte(0x6F12,0x1DD8);	// 1ECC
write_cmos_sensor_twobyte(0x602A,0x0340);
write_cmos_sensor_twobyte(0x6F12,0x0CDC);	// 0CF8

write_cmos_sensor_twobyte(0x602A,0x0900);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0901);
write_cmos_sensor(0x6F12,0x11);
write_cmos_sensor_twobyte(0x602A,0x0380);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x06E0);
write_cmos_sensor_twobyte(0x6F12,0x0200);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x06E4);
write_cmos_sensor_twobyte(0x6F12,0x1000);	// EVT1.1

// PSP BDS/HVbin
write_cmos_sensor_twobyte(0x602A,0x0EFA);
write_cmos_sensor(0x6F12,0x01);	// BDS
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0404);
write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

// CropAndPad
write_cmos_sensor_twobyte(0x602A,0x0408);
write_cmos_sensor_twobyte(0x6F12,0x000E);
write_cmos_sensor_twobyte(0x6F12,0x0000);

///////////////////////////////////////////////////////////
//PLL Sys = 560 , Sec = 1392
write_cmos_sensor_twobyte(0x602A,0x0136);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x602A,0x0304);
write_cmos_sensor_twobyte(0x6F12,0x0005);	// 3->5 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0173);	// 225->371 EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0x030C);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0302);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0300);
write_cmos_sensor_twobyte(0x6F12,0x0005);
write_cmos_sensor_twobyte(0x602A,0x030A);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0308);
write_cmos_sensor_twobyte(0x6F12,0x000A);

write_cmos_sensor_twobyte(0x602A,0x0318);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x00A4);	// A5 -> A4 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0316);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0314);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x030E);
write_cmos_sensor_twobyte(0x6F12,0x0004);
write_cmos_sensor_twobyte(0x6F12,0x00F2);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// OIF Setting
write_cmos_sensor_twobyte(0x602A,0x0111);
write_cmos_sensor(0x6F12,0x02);	// PVI, 2: MIPI
write_cmos_sensor_twobyte(0x602A,0x0114);
write_cmos_sensor(0x6F12,0x03);
write_cmos_sensor_twobyte(0x602A,0x0112);
write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

write_cmos_sensor_twobyte(0x602A,0xB0CA);
write_cmos_sensor_twobyte(0x6F12,0x7E00);	// M_DPHYCTL[30:25] = 6'11_1111 // EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0xB136);
write_cmos_sensor_twobyte(0x6F12,0x2000);	// B_DPHYCTL[62:60] = 3'b010 // EVT1.1 0429




// Stream On
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x01);


mDELAY(10);

}

static void hs_video_setting_10(void)
{
	//$MIPI[Width:2816,Height:1584,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1400,useEmbData:0]
	//$MV1[MCLK:24,Width:2816,Height:1584,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]

// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x1C87);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C88);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C89);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x1C8A);
write_cmos_sensor(0x6F12,0x03);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x1C8B);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8C);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8D);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x1C8E);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x1C8F);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C90);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C91);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x1C92);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current

write_cmos_sensor_twobyte(0x602A,0x426A);
write_cmos_sensor(0x6F12,0x04);
write_cmos_sensor_twobyte(0x602A,0x42FD);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x42FE);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x42FF);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4300);
write_cmos_sensor(0x6F12,0x29);
write_cmos_sensor_twobyte(0x602A,0x4305);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4306);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4307);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4308);
write_cmos_sensor(0x6F12,0x00);


	//// ADLC setting
	write_cmos_sensor_twobyte(0x6028,0x2000);
	write_cmos_sensor_twobyte(0x602A,0x25D8);
	write_cmos_sensor_twobyte(0x6F12,0x0010);
	write_cmos_sensor_twobyte(0x6F12,0x0001);	// 0011 lkh 20140318
	write_cmos_sensor_twobyte(0x6F12,0x0001);	// 0011 lkh 20140318 // ??
	write_cmos_sensor_twobyte(0x602A,0x26C8);
	write_cmos_sensor(0x6F12,0x00);

	//// Clock setting, related with ATOP
	write_cmos_sensor_twobyte(0x602A,0x1C64);
	write_cmos_sensor(0x6F12,0x01); // For 600MHz CCLK
	write_cmos_sensor_twobyte(0x602A,0x1C65);
	write_cmos_sensor(0x6F12,0x01);
	write_cmos_sensor_twobyte(0x602A,0x1CA4);
	write_cmos_sensor_twobyte(0x6F12,0x0064);	// DBR freq : 100MHz

	write_cmos_sensor_twobyte(0x602A,0x1520);
	write_cmos_sensor_twobyte(0x6F12,0x0200);
	write_cmos_sensor_twobyte(0x602A,0x1524);
	write_cmos_sensor_twobyte(0x6F12,0x1000);

	// PSP BDS/HVbin
	write_cmos_sensor_twobyte(0x602A,0x0F9A);
	write_cmos_sensor(0x6F12,0x01); // BDS

	// TG Readout
	write_cmos_sensor_twobyte(0x602A,0x14EC);
	write_cmos_sensor(0x6F12,0x01); // 0:Normal 1:CSR

	// Int.Time
	write_cmos_sensor_twobyte(0x6028,0x4000);
	write_cmos_sensor_twobyte(0x602A,0x0202);
	write_cmos_sensor_twobyte(0x6F12,0x0400);
	write_cmos_sensor_twobyte(0x602A,0x0200);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x021E);
	write_cmos_sensor_twobyte(0x6F12,0x0400);
	write_cmos_sensor_twobyte(0x602A,0x021C);
	write_cmos_sensor_twobyte(0x6F12,0x0001);

	// Analog Gain
	write_cmos_sensor_twobyte(0x602A,0x0204);
	write_cmos_sensor_twobyte(0x6F12,0x0020);
	write_cmos_sensor_twobyte(0x6F12,0x0020);

	//CASE : 2816x1584
	write_cmos_sensor_twobyte(0x602A,0x0344);
	write_cmos_sensor_twobyte(0x6F12,0x0012);
	write_cmos_sensor_twobyte(0x602A,0x0348);
	write_cmos_sensor_twobyte(0x6F12,0x1621);
	write_cmos_sensor_twobyte(0x602A,0x0346);
	write_cmos_sensor_twobyte(0x6F12,0x021C);
	write_cmos_sensor_twobyte(0x602A,0x034A);
	write_cmos_sensor_twobyte(0x6F12,0x0E8B);
	write_cmos_sensor_twobyte(0x6F12,0x0B00);
	write_cmos_sensor_twobyte(0x6F12,0x0630);
	write_cmos_sensor_twobyte(0x602A,0x0342);
	write_cmos_sensor_twobyte(0x6F12,0x1AD0);
	write_cmos_sensor_twobyte(0x602A,0x0340);
	write_cmos_sensor_twobyte(0x6F12,0x06B4);//06D6

	write_cmos_sensor_twobyte(0x602A,0x0900);
	write_cmos_sensor(0x6F12,0x01);
	write_cmos_sensor_twobyte(0x602A,0x0901);
	write_cmos_sensor(0x6F12,0x22);
	write_cmos_sensor_twobyte(0x602A,0x0380);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x6F12,0x0003);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x6F12,0x0003);

	write_cmos_sensor_twobyte(0x602A,0x0404);
	write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

	// CropAndPad
	write_cmos_sensor_twobyte(0x602A,0x0408);
	write_cmos_sensor_twobyte(0x6F12,0x0000);
	write_cmos_sensor_twobyte(0x6F12,0x0000);

	///////////////////////////////////////////////////////////
	//PLL Sys = 560 , Sec = 1392
	write_cmos_sensor_twobyte(0x602A,0x0136);
	write_cmos_sensor_twobyte(0x6F12,0x1800);
	write_cmos_sensor_twobyte(0x602A,0x0304);
	write_cmos_sensor_twobyte(0x6F12,0x0003);
	write_cmos_sensor_twobyte(0x6F12,0x00E1);
	write_cmos_sensor_twobyte(0x602A,0x030C);
	write_cmos_sensor_twobyte(0x6F12,0x0000);
	write_cmos_sensor_twobyte(0x602A,0x0302);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x0300);
	write_cmos_sensor_twobyte(0x6F12,0x0005);
	write_cmos_sensor_twobyte(0x602A,0x030A);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x0308);
	write_cmos_sensor_twobyte(0x6F12,0x000A);

	write_cmos_sensor_twobyte(0x602A,0x0318);
	write_cmos_sensor_twobyte(0x6F12,0x0003);
	write_cmos_sensor_twobyte(0x6F12,0x00A5);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x0316);
	write_cmos_sensor_twobyte(0x6F12,0x0001);
	write_cmos_sensor_twobyte(0x602A,0x0314);
	write_cmos_sensor_twobyte(0x6F12,0x0003);

	write_cmos_sensor_twobyte(0x602A,0x030E);
	write_cmos_sensor_twobyte(0x6F12,0x0004);
	write_cmos_sensor_twobyte(0x6F12,0x00F2);	//00E9
	write_cmos_sensor_twobyte(0x6F12,0x0000);

	// OIF Setting
	write_cmos_sensor_twobyte(0x602A,0x0111);
	write_cmos_sensor(0x6F12,0x02); // PVI, 2: MIPI
	write_cmos_sensor_twobyte(0x602A,0x0114);
	write_cmos_sensor(0x6F12,0x03);
	write_cmos_sensor_twobyte(0x602A,0x0112);
	write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

	// AF
	write_cmos_sensor_twobyte(0x602A,0x0B0E);
	write_cmos_sensor(0x6F12,0x00);
	write_cmos_sensor_twobyte(0x602A,0x3069);
	write_cmos_sensor(0x6F12,0x01);
	write_cmos_sensor_twobyte(0x602A,0x0B08);
	write_cmos_sensor(0x6F12,0x00);
	write_cmos_sensor_twobyte(0x602A,0x0B05);
	write_cmos_sensor(0x6F12,0x00);

	// Stream On
	write_cmos_sensor_twobyte(0x602A,0x0100);
	write_cmos_sensor(0x6F12,0x01);


mDELAY(10);

}

static void hs_video_setting_11(void)
{
//$MIPI[Width:2816,Height:1584,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:2816,Height:1584,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]

// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);

////1. ADLC setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x168C);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning



//2. Clock setting, related with ATOP remove
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E24);
write_cmos_sensor(0x6F12,0x01);	// For 600MHz CCLK

//3. ATOP Setting (Option)
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E9C);
write_cmos_sensor_twobyte(0x6F12,0x0081);	// RDV option // binning

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E47);
write_cmos_sensor(0x6F12,0x01);	// CDS current // EVT1.1 0429 lkh
write_cmos_sensor_twobyte(0x602A,0x0E48);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E49);
write_cmos_sensor(0x6F12,0x03);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E4A);
write_cmos_sensor(0x6F12,0x03);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x0E4B);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4C);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4D);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4E);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x0E4F);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E50);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E51);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E52);
write_cmos_sensor(0x6F12,0x07);	// Pixel Boost current


// EVT1.1 TnP
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0xAB00);
write_cmos_sensor(0x6F12,0x00);

//Af correction for 4x4 binning
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x14C9);
write_cmos_sensor(0x6F12,0x10);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CA);
write_cmos_sensor(0x6F12,0x10);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CD);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CE);
write_cmos_sensor(0x6F12,0x05);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CF);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D0);
write_cmos_sensor(0x6F12,0x09);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D1);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D2);
write_cmos_sensor(0x6F12,0x09);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D3);
write_cmos_sensor(0x6F12,0x06);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D4);
write_cmos_sensor(0x6F12,0x0D);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14DE);
write_cmos_sensor_twobyte(0x6F12,0x1BE4); // EVT1.1
write_cmos_sensor_twobyte(0x6F12,0xB14E);	// EVT1.1


//MSM gain for 4x4 binning
write_cmos_sensor_twobyte(0x602A,0x427A);
write_cmos_sensor(0x6F12,0x04);
write_cmos_sensor_twobyte(0x602A,0x430D);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x430E);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x430F);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4310);
write_cmos_sensor(0x6F12,0x29);
write_cmos_sensor_twobyte(0x602A,0x4315);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x4316);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4317);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x4318);
write_cmos_sensor(0x6F12,0x00);



// AF
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0B0E);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x3069);
write_cmos_sensor(0x6F12,0x01);

// BPC
write_cmos_sensor_twobyte(0x602A,0x0B08);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x0B05);
write_cmos_sensor(0x6F12,0x01);


// WDR
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0216);
write_cmos_sensor(0x6F12,0x00); //1	// smiaRegs_rw_wdr_multiple_exp_mode, EVT1.1
write_cmos_sensor_twobyte(0x602A,0x021B);
write_cmos_sensor(0x6F12,0x00);	// smiaRegs_rw_wdr_exposure_order, EVT1.1


write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0202);
write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x602A,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// Analog Gain
write_cmos_sensor_twobyte(0x602A,0x0204);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);


//4. CASE : FHD 1920x1080(2816x1584)
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0344);
write_cmos_sensor_twobyte(0x6F12,0x0012);
write_cmos_sensor_twobyte(0x602A,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x1621);
write_cmos_sensor_twobyte(0x602A,0x0346);
write_cmos_sensor_twobyte(0x6F12,0x021C);
write_cmos_sensor_twobyte(0x602A,0x034A);
write_cmos_sensor_twobyte(0x6F12,0x0E8B);
write_cmos_sensor_twobyte(0x6F12,0x0B00);
write_cmos_sensor_twobyte(0x6F12,0x0630);
write_cmos_sensor_twobyte(0x602A,0x0342);
write_cmos_sensor_twobyte(0x6F12,0x1AD0);
write_cmos_sensor_twobyte(0x602A,0x0340);
write_cmos_sensor_twobyte(0x6F12,0x06B4);	// 06D6

write_cmos_sensor_twobyte(0x602A,0x0900);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0901);
write_cmos_sensor(0x6F12,0x22);
write_cmos_sensor_twobyte(0x602A,0x0380);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x06E0);
write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x602A,0x06E4);
write_cmos_sensor_twobyte(0x6F12,0x1000);

// PSP BDS/HVbin
write_cmos_sensor_twobyte(0x602A,0x0EFA);
write_cmos_sensor(0x6F12,0x01);	// BDS
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0404);
write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

// CropAndPad
write_cmos_sensor_twobyte(0x602A,0x0408);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

///////////////////////////////////////////////////////////
//PLL Sys = 560 , Sec = 1392
write_cmos_sensor_twobyte(0x602A,0x0136);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x602A,0x0304);
write_cmos_sensor_twobyte(0x6F12,0x0005);	// 3->5 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0173);	// 225->371 EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0x030C);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0302);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0300);
write_cmos_sensor_twobyte(0x6F12,0x0005);
write_cmos_sensor_twobyte(0x602A,0x030A);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0308);
write_cmos_sensor_twobyte(0x6F12,0x000A);

write_cmos_sensor_twobyte(0x602A,0x0318);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x00A4);	// A5 -> A4 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0316);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0314);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x030E);
write_cmos_sensor_twobyte(0x6F12,0x0004);
write_cmos_sensor_twobyte(0x6F12,0x00F2);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// OIF Setting
write_cmos_sensor_twobyte(0x602A,0x0111);
write_cmos_sensor(0x6F12,0x02);	// PVI, 2: MIPI
write_cmos_sensor_twobyte(0x602A,0x0114);
write_cmos_sensor(0x6F12,0x03);
write_cmos_sensor_twobyte(0x602A,0x0112);
write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

write_cmos_sensor_twobyte(0x602A,0xB0CA);
write_cmos_sensor_twobyte(0x6F12,0x7E00);	// M_DPHYCTL[30:25] = 6'11_1111 // EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0xB136);
write_cmos_sensor_twobyte(0x6F12,0x2000);	// B_DPHYCTL[62:60] = 3'b010 // EVT1.1 0429





// Stream On
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x01);


mDELAY(10);

}


static void slim_video_setting(void)
{
//$MIPI[Width:1408,Height:792,Format:RAW10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:4,DataRate:1452,useEmbData:0]
//$MV1[MCLK:24,Width:1408,Height:792,Format:MIPI_RAW10,mipi_lane:4,mipi_hssettle:23,pvi_pclk_inverse:0]


// Stream Off
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x00);

////1. ADLC setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x168C);
write_cmos_sensor_twobyte(0x6F12,0x0010);
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning
write_cmos_sensor_twobyte(0x6F12,0x0001);	// binning



//2. Clock setting, related with ATOP remove
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E24);
write_cmos_sensor(0x6F12,0x01);	// For 600MHz CCLK

//3. ATOP Setting (Option)
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E9C);
write_cmos_sensor_twobyte(0x6F12,0x0080);	// RDV option // binning

//// CDS Current Setting
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x0E47);
write_cmos_sensor(0x6F12,0x05);	// CDS current // EVT1.1 0429 lkh
write_cmos_sensor_twobyte(0x602A,0x0E48);
write_cmos_sensor(0x6F12,0x05);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E49);
write_cmos_sensor(0x6F12,0x05);	// CDS current
write_cmos_sensor_twobyte(0x602A,0x0E4A);
write_cmos_sensor(0x6F12,0x05);	// CDS current

write_cmos_sensor_twobyte(0x602A,0x0E4B);
write_cmos_sensor(0x6F12,0x0F);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4C);
write_cmos_sensor(0x6F12,0x0F);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4D);
write_cmos_sensor(0x6F12,0x0F);	// Pixel Bias current
write_cmos_sensor_twobyte(0x602A,0x0E4E);
write_cmos_sensor(0x6F12,0x0F);	// Pixel Bias current

write_cmos_sensor_twobyte(0x602A,0x0E4F);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E50);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E51);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Boost current
write_cmos_sensor_twobyte(0x602A,0x0E52);
write_cmos_sensor(0x6F12,0x0A);	// Pixel Boost current


// EVT1.1 TnP
write_cmos_sensor_twobyte(0x6028,0x2001);
write_cmos_sensor_twobyte(0x602A,0xAB00);
write_cmos_sensor(0x6F12,0x01);

//Af correction for 4x4 binning
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x14C9);
write_cmos_sensor(0x6F12,0x04);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CA);
write_cmos_sensor(0x6F12,0x04);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CD);
write_cmos_sensor(0x6F12,0x02);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CE);
write_cmos_sensor(0x6F12,0x01);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14CF);
write_cmos_sensor(0x6F12,0x02);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D0);
write_cmos_sensor(0x6F12,0x03);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D1);
write_cmos_sensor(0x6F12,0x02);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D2);
write_cmos_sensor(0x6F12,0x03);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D3);
write_cmos_sensor(0x6F12,0x02);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14D4);
write_cmos_sensor(0x6F12,0x03);	// EVT1.1
write_cmos_sensor_twobyte(0x602A,0x14DE);
write_cmos_sensor_twobyte(0x6F12,0x1414); // EVT1.1
write_cmos_sensor_twobyte(0x6F12,0x4141);	// EVT1.1


//MSM gain for 4x4 binning
write_cmos_sensor_twobyte(0x602A,0x427A);
write_cmos_sensor(0x6F12,0x10);
write_cmos_sensor_twobyte(0x602A,0x430D);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x430E);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x430F);
write_cmos_sensor(0x6F12,0x08);
write_cmos_sensor_twobyte(0x602A,0x4310);
write_cmos_sensor(0x6F12,0x09);
write_cmos_sensor_twobyte(0x602A,0x4315);
write_cmos_sensor(0x6F12,0x20);
write_cmos_sensor_twobyte(0x602A,0x4316);
write_cmos_sensor(0x6F12,0x21);
write_cmos_sensor_twobyte(0x602A,0x4317);
write_cmos_sensor(0x6F12,0x28);
write_cmos_sensor_twobyte(0x602A,0x4318);
write_cmos_sensor(0x6F12,0x29);



// AF
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0B0E);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x3069);
write_cmos_sensor(0x6F12,0x00);

// BPC
write_cmos_sensor_twobyte(0x602A,0x0B08);
write_cmos_sensor(0x6F12,0x00);
write_cmos_sensor_twobyte(0x602A,0x0B05);
write_cmos_sensor(0x6F12,0x01);


// WDR
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0216);
write_cmos_sensor(0x6F12,0x00); //1	// smiaRegs_rw_wdr_multiple_exp_mode, EVT1.1
write_cmos_sensor_twobyte(0x602A,0x021B);
write_cmos_sensor(0x6F12,0x00);	// smiaRegs_rw_wdr_exposure_order, EVT1.1


write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0202);
write_cmos_sensor_twobyte(0x6F12,0x0400);
write_cmos_sensor_twobyte(0x602A,0x0200);
write_cmos_sensor_twobyte(0x6F12,0x0001);

// Analog Gain
write_cmos_sensor_twobyte(0x602A,0x0204);
write_cmos_sensor_twobyte(0x6F12,0x0020);
write_cmos_sensor_twobyte(0x6F12,0x0020);


//4. CASE : HD 1280x720(1408x792)
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0344);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0348);
write_cmos_sensor_twobyte(0x6F12,0x161F);
write_cmos_sensor_twobyte(0x602A,0x0346);
write_cmos_sensor_twobyte(0x6F12,0x0214);
write_cmos_sensor_twobyte(0x602A,0x034A);
write_cmos_sensor_twobyte(0x6F12,0x0E93);
write_cmos_sensor_twobyte(0x6F12,0x0580);
write_cmos_sensor_twobyte(0x6F12,0x0318);
write_cmos_sensor_twobyte(0x602A,0x0342);
write_cmos_sensor_twobyte(0x6F12,0x23F0);
write_cmos_sensor_twobyte(0x602A,0x0340);
write_cmos_sensor_twobyte(0x6F12,0x0368);	// 06D6

write_cmos_sensor_twobyte(0x602A,0x0900);
write_cmos_sensor(0x6F12,0x01);
write_cmos_sensor_twobyte(0x602A,0x0901);
write_cmos_sensor(0x6F12,0x44);
write_cmos_sensor_twobyte(0x602A,0x0380);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0007);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x6F12,0x0007);
write_cmos_sensor_twobyte(0x6028,0x2000);
write_cmos_sensor_twobyte(0x602A,0x06E0);
write_cmos_sensor_twobyte(0x6F12,0x0200);
write_cmos_sensor_twobyte(0x602A,0x06E4);
write_cmos_sensor_twobyte(0x6F12,0x1000);

// PSP BDS/HVbin
write_cmos_sensor_twobyte(0x602A,0x0EFA);
write_cmos_sensor(0x6F12,0x01);	// BDS
write_cmos_sensor_twobyte(0x6028,0x4000);
write_cmos_sensor_twobyte(0x602A,0x0404);
write_cmos_sensor_twobyte(0x6F12,0x0010);	// x1.7

// CropAndPad
write_cmos_sensor_twobyte(0x602A,0x0408);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x6F12,0x0000);

///////////////////////////////////////////////////////////
//PLL Sys = 560 , Sec = 1392
write_cmos_sensor_twobyte(0x602A,0x0136);
write_cmos_sensor_twobyte(0x6F12,0x1800);
write_cmos_sensor_twobyte(0x602A,0x0304);
write_cmos_sensor_twobyte(0x6F12,0x0005);	// 3->5 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0173);	// 225->371 EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0x030C);
write_cmos_sensor_twobyte(0x6F12,0x0000);
write_cmos_sensor_twobyte(0x602A,0x0302);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0300);
write_cmos_sensor_twobyte(0x6F12,0x0005);
write_cmos_sensor_twobyte(0x602A,0x030A);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0308);
write_cmos_sensor_twobyte(0x6F12,0x000A);

write_cmos_sensor_twobyte(0x602A,0x0318);
write_cmos_sensor_twobyte(0x6F12,0x0003);
write_cmos_sensor_twobyte(0x6F12,0x00A4);	// A5 -> A4 EVT1.1 0429
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0316);
write_cmos_sensor_twobyte(0x6F12,0x0001);
write_cmos_sensor_twobyte(0x602A,0x0314);
write_cmos_sensor_twobyte(0x6F12,0x0003);

write_cmos_sensor_twobyte(0x602A,0x030E);
write_cmos_sensor_twobyte(0x6F12,0x0004);
write_cmos_sensor_twobyte(0x6F12,0x00F2);
write_cmos_sensor_twobyte(0x6F12,0x0000);

// OIF Setting
write_cmos_sensor_twobyte(0x602A,0x0111);
write_cmos_sensor(0x6F12,0x02);	// PVI, 2: MIPI
write_cmos_sensor_twobyte(0x602A,0x0114);
write_cmos_sensor(0x6F12,0x03);
write_cmos_sensor_twobyte(0x602A,0x0112);
write_cmos_sensor_twobyte(0x6F12,0x0A0A);	// data format

write_cmos_sensor_twobyte(0x602A,0xB0CA);
write_cmos_sensor_twobyte(0x6F12,0x7E00);	// M_DPHYCTL[30:25] = 6'11_1111 // EVT1.1 0429
write_cmos_sensor_twobyte(0x602A,0xB136);
write_cmos_sensor_twobyte(0x6F12,0x2000);	// B_DPHYCTL[62:60] = 3'b010 // EVT1.1 0429





// Stream On
write_cmos_sensor_twobyte(0x602A,0x0100);
write_cmos_sensor(0x6F12,0x01);


mDELAY(10);

}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	/********************************************************

	*0x5040[7]: 1 enable,  0 disable
	*0x5040[3:2]; color bar style 00 standard color bar
	*0x5040[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
	********************************************************/


	if (enable)
	{
		write_cmos_sensor(0x5040, 0x80);
	}
	else
	{
		write_cmos_sensor(0x5040, 0x00);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    get_imgsensor_id
*
* DESCRIPTION
*    This function get the sensor ID
*
* PARAMETERS
*    *sensorID : return the sensor ID
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 5;

    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
        	 write_cmos_sensor_twobyte(0x602C,0x4000);
     	     write_cmos_sensor_twobyte(0x602E, 0x0000);
	         *sensor_id = read_cmos_sensor_twobyte(0x6F12);
		LOG_INF("JEFF get_imgsensor_id-read sensor ID (0x%x)\n", *sensor_id );


               if (*sensor_id == imgsensor_info.sensor_id) {
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);


                return ERROR_NONE;
            }
            LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
            retry--;
        } while(retry > 0);
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
*    open
*
* DESCRIPTION
*    This function initialize the registers of CMOS sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 5;
    kal_uint32 sensor_id = 0;
    //kal_uint32 chip_id = 0;
    LOG_1;
    //LOG_2;
    #if 1
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);

LOG_INF("JEFF xxxxxx140 20fps\n");
        do {
        	 write_cmos_sensor_twobyte(0x602C,0x4000);
     	     write_cmos_sensor_twobyte(0x602E,0x0000);
	         sensor_id = read_cmos_sensor_twobyte(0x6F12);
		LOG_INF("JEFF get_imgsensor_id-read sensor ID (0x%x)\n", sensor_id );

				  write_cmos_sensor_twobyte(0x602C,0x4000);
     	    write_cmos_sensor_twobyte(0x602E,0x001A);
	        chip_id = read_cmos_sensor_twobyte(0x6F12);
	//chip_id = read_cmos_sensor_twobyte(0x001A);
	LOG_INF("JEFF get_imgsensor_id-read chip_id (0x%x)\n", chip_id );

            if (sensor_id == imgsensor_info.sensor_id) {
                LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
                break;
            }
            LOG_INF("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
            retry--;
        } while(retry > 0);
        i++;
        if (sensor_id == imgsensor_info.sensor_id)
            break;
        retry = 2;
    }
    if (imgsensor_info.sensor_id != sensor_id)
        return ERROR_SENSOR_CONNECT_FAIL;
#endif

					write_cmos_sensor_twobyte(0x602C,0x4000);
     	    write_cmos_sensor_twobyte(0x602E,0x001A);
	        chip_id = read_cmos_sensor_twobyte(0x6F12);
	//chip_id = read_cmos_sensor_twobyte(0x001A);
	LOG_INF("JEFF get_imgsensor_id-read chip_id (0x%x)\n", chip_id );
	/* initail sequence write in  */
	if(chip_id == 0x0203)
		{
			sensor_init_10();
		}
	else if(chip_id == 0x022C)
			{
				sensor_init_11();
			}
		else
				{
					sensor_init_11();
				}


#ifdef	USE_OIS
	//OIS_on(RUMBA_OIS_CAP_SETTING);//pangfei OIS
	LOG_INF("pangfei capture OIS setting\n");
	OIS_write_cmos_sensor(0x0002,0x05);
	OIS_write_cmos_sensor(0x0002,0x00);
	OIS_write_cmos_sensor(0x0000,0x01);
#endif

    spin_lock(&imgsensor_drv_lock);

    imgsensor.autoflicker_en= KAL_FALSE;
    imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.dummy_pixel = 0;
    imgsensor.dummy_line = 0;
    imgsensor.hdr_mode = KAL_FALSE;
    imgsensor.test_pattern = KAL_FALSE;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
    spin_unlock(&imgsensor_drv_lock);

    return ERROR_NONE;
}    /*    open  */



/*************************************************************************
* FUNCTION
*    close
*
* DESCRIPTION
*
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
    LOG_INF("E\n");

    /*No Need to implement this function*/

    return ERROR_NONE;
}    /*    close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*    This function start the sensor preview.
*
* PARAMETERS
*    *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
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
   	if(chip_id == 0x0203)
			{
				preview_setting_10();
			}
			else if(chip_id == 0x022C)
					{
						preview_setting_11();
					}
					else
							{
								preview_setting_11();
							}

	set_mirror_flip(IMAGE_NORMAL);


#ifdef USE_OIS
	//OIS_on(RUMBA_OIS_PRE_SETTING);	//pangfei OIS
	LOG_INF("pangfei preview OIS setting\n");
	OIS_write_cmos_sensor(0x0002,0x05);
	OIS_write_cmos_sensor(0x0002,0x00);
	OIS_write_cmos_sensor(0x0000,0x01);
#endif
    return ERROR_NONE;
}    /*    preview   */

/*************************************************************************
* FUNCTION
*    capture
*
* DESCRIPTION
*    This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*    None
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
	if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate)
	{//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
		imgsensor.pclk = imgsensor_info.cap1.pclk;
		imgsensor.line_length = imgsensor_info.cap1.linelength;
		imgsensor.frame_length = imgsensor_info.cap1.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	else
	{
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
		LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
		imgsensor.pclk = imgsensor_info.cap.pclk;
		imgsensor.line_length = imgsensor_info.cap.linelength;
		imgsensor.frame_length = imgsensor_info.cap.framelength;
		imgsensor.min_frame_length = imgsensor_info.cap.framelength;
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	if(imgsensor.hdr_mode == 9)
		capture_setting_WDR(imgsensor.current_fps);
	else
		capture_setting_WDR(imgsensor.current_fps);

    set_mirror_flip(IMAGE_NORMAL);
#ifdef	USE_OIS
	//OIS_on(RUMBA_OIS_CAP_SETTING);//pangfei OIS
	LOG_INF("pangfei capture OIS setting\n");
	OIS_write_cmos_sensor(0x0002,0x05);
	OIS_write_cmos_sensor(0x0002,0x00);
	OIS_write_cmos_sensor(0x0000,0x01);
#endif
	return ERROR_NONE;
	}    /* capture() */
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
    if(chip_id == 0x0203)
		{
	 		normal_video_setting_10(imgsensor.current_fps);
		}
	else if(chip_id == 0x022C)
			{
	 			normal_video_setting_11(imgsensor.current_fps);
			}
			else
					{
	 					normal_video_setting_11(imgsensor.current_fps);
					}

	set_mirror_flip(IMAGE_NORMAL);
    return ERROR_NONE;
}    /*    normal_video   */

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
	    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    if(chip_id == 0x0203)
		{
			hs_video_setting_10();
		}
	else if(chip_id == 0x022C)
			{
				hs_video_setting_11();
			}
			else
				{
					hs_video_setting_11();
				}

	set_mirror_flip(IMAGE_NORMAL);
    return ERROR_NONE;
}    /*    hs_video   */

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
	set_mirror_flip(IMAGE_NORMAL);

    return ERROR_NONE;
}    /*    slim_video     */

static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
    LOG_INF("E\n");
    sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
    sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

    sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
    sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

    sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
    sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


    sensor_resolution->SensorHighSpeedVideoWidth     = imgsensor_info.hs_video.grabwindow_width;
    sensor_resolution->SensorHighSpeedVideoHeight     = imgsensor_info.hs_video.grabwindow_height;

    sensor_resolution->SensorSlimVideoWidth     = imgsensor_info.slim_video.grabwindow_width;
    sensor_resolution->SensorSlimVideoHeight     = imgsensor_info.slim_video.grabwindow_height;
	
    return ERROR_NONE;
}    /*    get_resolution    */

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

    sensor_info->SensorMasterClockSwitch = 0; /* not use */
    sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

    sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;          /* The frame of setting shutter default 0 for TG int */
    sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;    /* The frame of setting sensor gain */
    sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
    sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
    sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
    sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
    sensor_info->PDAF_Support = 1;

    /*0: no support, 1: G0,R0.B0, 2: G0,R0.B1, 3: G0,R1.B0, 4: G0,R1.B1*/
    /*                    5: G1,R0.B0, 6: G1,R0.B1, 7: G1,R1.B0, 8: G1,R1.B1*/
    sensor_info->ZHDR_MODE = 7; 

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
    sensor_info->SensorHightSampling = 0;    // 0 is default 1x
    sensor_info->SensorPacketECCOrder = 1;

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
}    /*    get_info  */


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
}    /* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{//This Function not used after ROME
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
            //set_dummy();
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
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	  if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {
                frame_length = imgsensor_info.cap1.pclk / framerate * 10 / imgsensor_info.cap1.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap1.framelength) ? (frame_length - imgsensor_info.cap1.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap1.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            } else {
        		    if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
                    LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",framerate,imgsensor_info.cap.max_framerate/10);
                frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
                spin_lock(&imgsensor_drv_lock);
		            imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		            imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		            imgsensor.min_frame_length = imgsensor.frame_length;
		            spin_unlock(&imgsensor_drv_lock);
            }
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
            frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
            frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
            imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
            break;
        default:  //coding with  preview scenario by default
            frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
            spin_lock(&imgsensor_drv_lock);
            imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
            imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
            imgsensor.min_frame_length = imgsensor.frame_length;
            spin_unlock(&imgsensor_drv_lock);
            //set_dummy();
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

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
                             UINT8 *feature_para,UINT32 *feature_para_len)
{
    UINT16 *feature_return_para_16=(UINT16 *) feature_para;
    UINT16 *feature_data_16=(UINT16 *) feature_para;
    UINT32 *feature_return_para_32=(UINT32 *) feature_para;
    UINT32 *feature_data_32=(UINT32 *) feature_para;
    unsigned long long *feature_data=(unsigned long long *) feature_para;
    //unsigned long long *feature_return_para=(unsigned long long *) feature_para;

    SET_PD_BLOCK_INFO_T *PDAFinfo;
    SENSOR_WINSIZE_INFO_STRUCT *wininfo;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

    LOG_INF("feature_id = %d\n", feature_id);
    switch (feature_id) {
        case SENSOR_FEATURE_GET_PERIOD:
            *feature_return_para_16++ = imgsensor.line_length;
            *feature_return_para_16 = imgsensor.frame_length;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            LOG_INF("feature_Control imgsensor.pclk = %d,imgsensor.current_fps = %d\n", imgsensor.pclk,imgsensor.current_fps);
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
	    case SENSOR_FEATURE_GET_PDAF_DATA:
            //add for s5k2x8 pdaf
            LOG_INF("s5k2x8_read_otp_pdaf_data \n");
			s5k2x8_read_otp_pdaf_data((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
			break;
		case SENSOR_FEATURE_SET_TEST_PATTERN:
            set_test_pattern_mode((BOOL)*feature_data);
            break;
        case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: //for factory mode auto testing
            *feature_return_para_32 = imgsensor_info.checksum_value;
            *feature_para_len=4;
            break;
        case SENSOR_FEATURE_SET_FRAMERATE:
            LOG_INF("current fps :%lld\n", *feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
     /*   case SENSOR_FEATURE_SET_SENSOR_OTP_AWB_CMD:
            LOG_INF("Update sensor awb from otp :%d\n", (BOOL)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.update_sensor_otp_awb = (BOOL)*feature_data;
            spin_unlock(&imgsensor_drv_lock);
            if(0 != imgsensor.update_sensor_otp_awb || 0 != imgsensor.update_sensor_otp_lsc) {
                otp_update(imgsensor.update_sensor_otp_awb, imgsensor.update_sensor_otp_lsc);
            }
            break;

        case SENSOR_FEATURE_SET_SENSOR_OTP_LSC_CMD:
            LOG_INF("Update sensor lsc from otp :%d\n", (BOOL)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.update_sensor_otp_lsc = (BOOL)*feature_data;
            spin_unlock(&imgsensor_drv_lock);
            if(0 != imgsensor.update_sensor_otp_awb || 0 != imgsensor.update_sensor_otp_lsc) {
                otp_update(imgsensor.update_sensor_otp_awb, imgsensor.update_sensor_otp_lsc);
            }
            break;
*/
        case SENSOR_FEATURE_SET_HDR:
            LOG_INF("hdr mode :%d\n", (BOOL)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.hdr_mode = (BOOL)*feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case SENSOR_FEATURE_GET_CROP_INFO:
            LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%lld\n", *feature_data);

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

	    //add for s5k2x8 pdaf
        case SENSOR_FEATURE_GET_PDAF_INFO:
            LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%lld\n", *feature_data);
            PDAFinfo= (SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data+1));

            switch (*feature_data) {
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                    memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(SET_PD_BLOCK_INFO_T));
                    break;
                case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
                case MSDK_SCENARIO_ID_SLIM_VIDEO:
                case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
                default:
                    break;
            }
            break;

        case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
            LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%lld\n", *feature_data);
            //PDAF capacity enable or not, s5k2x8 only full size support PDAF
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
                    *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1; ///preview support
                    break;
                default:
                    *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
                    break;
            }
            break;
		case SENSOR_FEATURE_SET_HDR_SHUTTER:
            LOG_INF("SENSOR_FEATURE_SET_HDR_SHUTTER LE=%d, SE=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1));
            hdr_write_shutter((UINT16)*feature_data,(UINT16)*(feature_data+1));
            break;
        default:
            break;
    }

    return ERROR_NONE;
}   /*  feature_control()  */

static SENSOR_FUNCTION_STRUCT sensor_func = {
    open,
    get_info,
    get_resolution,
    feature_control,
    control,
    close
};

UINT32 S5K2X8_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}    /*    s5k2x8_MIPI_RAW_SensorInit    */
