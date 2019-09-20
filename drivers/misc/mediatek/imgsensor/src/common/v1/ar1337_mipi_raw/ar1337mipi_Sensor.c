/*****************************************************************************
 *
 * Filename:
 * ---------
 *     AR1337mipi_Sensor.c
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
//#include <asm/system.h>
//#include <linux/xlog.h>

#if 1
#include <linux/module.h>       
#include <linux/kernel.h>       
#include <linux/init.h>         
#include <linux/clk.h>      
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/io.h>         
#include <linux/types.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <asm/io.h>		/* inb/outb */
#include <asm/uaccess.h>
#endif
#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ar1337mipi_Sensor.h"



#define AR1337_AWBOTP_SIZE 9
#define AR1337_AWB_addr  43
#define AR1337_AFOTP_SIZE 7
#define AR1337_AF_addr  58	
#define AR1337_LSCOTP_SIZE 1868
#define AR1337_LSC_addr  76	

u8 AR1337_OTPData[AR1337_LSCOTP_SIZE]={0};

#if 0
//
extern int iRead16Data_ar1337(unsigned short ui4_offset, unsigned int  ui4_length, unsigned char * pinputdata);
#endif

//#define TEST_PATTERN
#define PE_16
//#define PE_10

/*VC FIX */
#define VC_FIX 1
#define LEN_H_THRESHOLD 4358
#define LLPCK_ORIG 0x12B8
#define LLPCK_LONG 0x1A00 

/****************************Modify Following Strings for Debug****************************/
#define PFX "AR1337_camera_sensor"
#define LOG_1 LOG_INF("AR1337,MIPI 4LANE\n")
#define LOG_2 LOG_INF("preview 2106*1560@30fps,640Mbps/lane; video 4192*3104@30fps,1.2Gbps/lane; capture 13M@30fps,1.2Gbps/lane\n")
/****************************   Modify end    *******************************************/

#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOG_CORN(format, args...)    pr_err(PFX "[%s] " format, __FUNCTION__, ##args)

#if VC_FIX
static int cit_long = 0;
static int cit_orig = 0;
#endif

static DEFINE_SPINLOCK(imgsensor_drv_lock);

//sensor otp
extern void otp_cali(unsigned char writeid);

static imgsensor_info_struct imgsensor_info = {
    .sensor_id = AR1337_SENSOR_ID,        //record sensor id defined in Kd_imgsensor.h

    .checksum_value = 0xbde6b5f8,//0xf86cfdf4,        //checksum value for Camera Auto Test

    .pre = {
        .pclk = 454400000,                //record different mode's pclk
        .linelength = 4680,                //record different mode's linelength
        .framelength = 3220,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 4208,        //record different mode's width of grabwindow
        .grabwindow_height = 3120,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
		
		//.mipi_pixel_rate = 454400000,
		//.mipi_data_rate = 1136000000,
    },
    .cap = {
        .pclk = 454400000,                //record different mode's pclk
        .linelength = 4680,                //record different mode's linelength
        .framelength = 3220,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 4208,        //record different mode's width of grabwindow
        .grabwindow_height = 3120,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
		//.mipi_pixel_rate = 454400000,
		//.mipi_data_rate = 1136000000,
    },
    .cap1 = {
        .pclk = 454400000,                //record different mode's pclk
        .linelength = 4680,                //record different mode's linelength
        .framelength = 3220,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 4208,        //record different mode's width of grabwindow
        .grabwindow_height = 3120,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
		//.mipi_pixel_rate = 454400000,
		//.mipi_data_rate = 1136000000,
    },
    .normal_video = {
        .pclk = 454400000,                //record different mode's pclk
        .linelength = 4680,                //record different mode's linelength
        .framelength = 3220,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 4208,        //record different mode's width of grabwindow
        .grabwindow_height = 3120,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
		//.mipi_pixel_rate = 454400000,
		//.mipi_data_rate = 1136000000,
    },
    .hs_video = {
        .pclk = 454400000,                //record different mode's pclk
        .linelength = 4680,                //record different mode's linelength
        .framelength = 3220,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 4208,        //record different mode's width of grabwindow
        .grabwindow_height = 3120,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
		//.mipi_pixel_rate = 454400000,
		//.mipi_data_rate = 1136000000,
    },
    .slim_video = {
        .pclk = 454400000,                //record different mode's pclk
        .linelength = 4680,                //record different mode's linelength
        .framelength = 3220,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 4208,        //record different mode's width of grabwindow
        .grabwindow_height = 3120,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 85,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
		//.mipi_pixel_rate = 454400000,
		//.mipi_data_rate = 1136000000,
    },
    .margin = 8,            //sensor framelength & shutter margin
    .min_shutter = 0x3,        //min shutter
    .max_frame_length = 0x7fff,//max framelength by sensor register's limitation
    .ae_shut_delay_frame = 0,    //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
    .ae_sensor_gain_delay_frame = 1,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
    .ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
    .ihdr_support = 0,     // 1, support; 0,not support
    .ihdr_le_firstline = 0,  // 1,le first ; 0, se first
    .sensor_mode_num = 5,      //support sensor mode num

    .cap_delay_frame = 3,        //enter capture delay frame num
    .pre_delay_frame = 2,         //enter preview delay frame num
    .video_delay_frame = 2,        //enter video delay frame num
    .hs_video_delay_frame = 2,    //enter high speed video  delay frame num
    .slim_video_delay_frame = 2,//enter slim video delay frame num

    .isp_driving_current = ISP_DRIVING_8MA, //mclk driving current
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
    .sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gr,//sensor output first pixel color
    .mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
    .mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
    .i2c_addr_table = {0x6c, 0xff, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
};


static imgsensor_struct imgsensor = {
    .mirror = IMAGE_HV_MIRROR,
    .sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
    .shutter = 0x3D0,                    //current shutter
    .gain = 0x100,                        //current gain
    .dummy_pixel = 0,                    //current dummypixel
    .dummy_line = 0,                    //current dummyline
    .current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
    .autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
    .test_pattern = KAL_FALSE,        //test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
    .current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
    .ihdr_en = 0, //sensor need support LE, SE with HDR feature
    .i2c_write_id = 0x00,//record current sensor's i2c write id
};


/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =	 
{{ 4208, 3120,    0,    0, 4208, 3120, 4208, 3120, 0000, 0000, 4208, 3120,      0,    0, 4208, 3120}, // Preview 
	//{ 4208, 3120, 0, 0, 4208, 3120, 2104, 1560, 0, 0, 2104, 1560, 0, 0, 2096, 1552}, // Preview 
	//{ 4208, 3120, 0, 0, 4208, 3120, 2104, 1560, 0, 0, 2104, 1560, 0, 0, 2096, 1552}, // Preview 

 { 4208, 3120,    0,    0, 4208, 3120, 4208, 3120, 0000, 0000, 4208, 3120,      0,    0, 4208, 3120}, // capture 
  { 4208, 3120,    0,    0, 4208, 3120, 4208, 3120, 0000, 0000, 4208, 3120,      0,    0, 4208, 3120}, // video 
{ 4208, 3120,    0,    0, 4208, 3120, 4208, 3120, 0000, 0000, 4208, 3120,      0,    0, 4208, 3120}, //hight speed video 
// { 4208, 3120, 0, 0, 4208, 3120, 2104, 1560, 0, 0, 2104, 1560, 2, 2, 1280, 720}, // slim video 
	{ 4208, 3120,    0,    0, 4208, 3120, 4208, 3120, 0000, 0000, 4208, 3120,      0,    0, 4208, 3120}, 
};

/*VC1 for HDR(DT=0X35) , VC2 for PDAF(DT=0X36), unit : 8bit*/
static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[3]=
{/* Preview mode setting */
  {0x05, 0x0a,   0x00,   0x00, 0x00, 0x00, 
    0x00, 0x2b, 4208, 3120, 0x01, 0x0, 0, 0,
    0x00, 0x36, 1920, 15, 0x03, 0x00, 0x0000, 0x0000},
  /* Capture mode setting */
  {0x05, 0x0a,	 0x00,	 0x00, 0x00, 0x00,
	  0x00, 0x2b, 4208, 3120, 0x01, 0x0, 0, 0,
	  0x00, 0x36, 1920, 15, 0x03, 0x00, 0x0000, 0x0000},
  /* Video mode setting */
  {0x02, 0x0a,	 0x00,	 0x00, 0x00, 0x00,
    0x00, 0x2b, 4208, 3120, 0x01, 0x00, 0x0000, 0x0000,
    0x02, 0x00, 0x0000, 0x0000, 0x03, 0x00, 0x0000, 0x0000}
};

#define AR1337MIPI_MaxGainIndex (49)
kal_uint16 AR1337MIPI_sensorGainMapping[AR1337MIPI_MaxGainIndex][2] ={	
	{64 ,0x2010},
	{68 ,0x2011},
	{72 ,0x2012},
	{76 ,0x2013},
	{80 ,0x2014},
	{84 ,0x2015},
	{88 ,0x2016},
	{92 ,0x2017},
	{96 ,0x2018},
	{100,0x2019},
	{104,0x201A},
	{108,0x201B},
	{112,0x201C},
	{116,0x201D},
	{120,0x201E},
	{124,0x201F},
	{128,0x2020},
	{136,0x2021},
	{144,0x2022},
	{152,0x2023},
	{160,0x2024},
	{168,0x2025},
	{176,0x2026},
	{184,0x2027},
	{192,0x2028},
	{200,0x2029},
	{208,0x202A},
	{216,0x202B},
	{224,0x202C},
	{232,0x202D},
	{240,0x202E},
	{248,0x202F},
	{256,0x2030},
	{272,0x2031},
	{288,0x2032},
	{304,0x2033},
	{320,0x2034},
	{336,0x2035},
	{352,0x2036},
	{368,0x2037},
	{384,0x2038},
	{400,0x2039},
	{416,0x203A},
	{432,0x203B},
	{448,0x203C},
	{464,0x203D},
	{480,0x203E},
	{496,0x203F},
	{512,0x213F},
};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;

    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

    return get_byte;
}
/*
static kal_uint16 read_cmos_sensor_16bit(kal_uint32 addr)
{
    kal_uint16 get_byte=0;

    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);

    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}
*/
static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    //kal_uint16 get_byte=0;
    //char pu_send_cmd_read[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	//kdSetI2CSpeed(100);

    iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);
    //iReadRegI2C(pu_send_cmd_read, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
    //LOG_CORN("[Corn]reg addr = 0x%x, para = 0x%x \n", addr, get_byte);
	
}

static void write_cmos_sensor_16bit(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)};
    //kal_uint16 get_byte=0;
    //char pu_send_cmd_read[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	//kdSetI2CSpeed(100);
	
    //LOG_INF("addr = 0x%x, para = 0x%x \n", addr, para);
    iWriteRegI2C(pu_send_cmd, 4, imgsensor.i2c_write_id);
    //iReadRegI2C(pu_send_cmd_read, 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
    //LOG_CORN("[Corn]reg addr = 0x%x, para = 0x%x \n", addr, ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff));
	
}

static void set_dummy(void)
{
    LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
    /* you can set dummy by imgsensor.dummy_line and imgsensor.dummy_pixel, or you can set dummy by imgsensor.frame_length and imgsensor.line_length */
	write_cmos_sensor(0x0104, 0x01); 
	   
	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);	  
	write_cmos_sensor(0x0342, imgsensor.line_length >> 8);
	write_cmos_sensor(0x0343, imgsensor.line_length & 0xFF);

	write_cmos_sensor(0x0104, 0x00);
}    /*    set_dummy  */

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor(0x3000) << 8) | (read_cmos_sensor(0x3001)));
}

static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
    kal_uint32 frame_length = imgsensor.frame_length;
    //unsigned long flags;

    LOG_INF("framerate = %d, min framelength should enable = %d \n", framerate,min_framelength_en);


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
}    /*    set_max_framerate  */



/*************************************************************************
* FUNCTION
*    set_shutter
*
* DESCRIPTION
*    This function set e-shutter of sensor to change exposure time.
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
    unsigned long flags;
    kal_uint16 realtime_fps = 0;
    #if VC_FIX
    typedef enum {LOW, HIGH} State;
    static State s_state = HIGH;
    #endif
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;

    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	LOG_INF("Enter! shutter =%d \n", shutter);

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
    cit_orig = shutter;

    if (imgsensor.autoflicker_en) {
        realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
        if(realtime_fps >= 297 && realtime_fps <= 305)
            set_max_framerate(296,0);
        else if(realtime_fps >= 147 && realtime_fps <= 150)
            set_max_framerate(146,0);
        else {
        // Extend frame length
 		write_cmos_sensor(0x0104, 0x01); 
		write_cmos_sensor_16bit(0x0340, imgsensor.frame_length);
		write_cmos_sensor(0x0104, 0x00);
        }
    } else {
        // Extend frame length
		write_cmos_sensor(0x0104, 0x01); 
		write_cmos_sensor_16bit(0x0340, imgsensor.frame_length);
		write_cmos_sensor(0x0104, 0x00);
    }

    // Update Shutter
    write_cmos_sensor(0x0104, 0x01);
    #if VC_FIX
      if (imgsensor.sensor_mode == IMGSENSOR_MODE_CAPTURE) {
         if(s_state == HIGH) {
		if(shutter > LEN_H_THRESHOLD) {
		        write_cmos_sensor_16bit(0x0342, 0x1A00);
		        write_cmos_sensor_16bit(0x32CA, 0x0680);
                        //Calculated new long CIT will be applied.
                        cit_long = (LLPCK_ORIG * cit_orig)/LLPCK_LONG;
                        write_cmos_sensor_16bit(0x0202, cit_long);
			
                        s_state = LOW;
			LOG_CORN("ABN-S1: NON-OVERLAP MODE setting done and changing state to LOW cit_long =%x\n", cit_long);
		} else {
			LOG_CORN("ABN-S2: No Extra Setting to apply in s_state: HIGH\n");
                        write_cmos_sensor_16bit(0x0202, shutter);
                }
      } else if(s_state == LOW) {
		if(shutter < LEN_H_THRESHOLD) {
		        write_cmos_sensor_16bit(0x0342, 0x12B8);
		        write_cmos_sensor_16bit(0x32CA, 0x04AA);
                        write_cmos_sensor_16bit(0x0202, shutter);
			
                        s_state = HIGH;
			LOG_CORN("ABN-S3: OVERLAP MODE setting done, and changing state to HIGH\n");
		} else {
                        //Calculated new long CIT will be applied.
                        cit_long = (LLPCK_ORIG * cit_orig)/LLPCK_LONG;
                        write_cmos_sensor_16bit(0x0202, cit_long);
    			
			LOG_CORN("ABN-S4: New long CIT=%x is applied s_state: LOW\n", cit_long);
                }
      }
    }
    else {
       LOG_CORN("ABN: Not ZSD\n");
       write_cmos_sensor_16bit(0x0202, shutter);
    }
    #else
       LOG_CORN("ABN: Not ZSD\n");
       write_cmos_sensor_16bit(0x0202, shutter);
    #endif
    write_cmos_sensor(0x0104, 0x00); 

    LOG_INF("Exit! shutter =%d, framelength =%d\n", shutter,imgsensor.frame_length);

}    /*    set_shutter */



static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint8 iI;
    LOG_INF("[AR1337MIPI]enter AR1337gain2reg function\n");
    for (iI = 0; iI < AR1337MIPI_MaxGainIndex; iI++) 
	{
		if(gain < AR1337MIPI_sensorGainMapping[iI][0])
		{                
			return AR1337MIPI_sensorGainMapping[iI][1];       
		}
			

    }
	if(iI != AR1337MIPI_MaxGainIndex)
	{
    	if(gain != AR1337MIPI_sensorGainMapping[iI][0])
    	{
        	 LOG_INF("Gain mapping don't correctly:%d %d \n", gain, AR1337MIPI_sensorGainMapping[iI][0]);
    	}
    }
	LOG_INF("[AR1337MIPI]exit AR1337gain2reg function\n");
    return AR1337MIPI_sensorGainMapping[iI-1][1];
}

/*************************************************************************
* FUNCTION
*    set_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    iGain : sensor global gain(base: 0x40)
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

    /* 0x350A[0:1], 0x350B[0:7] AGC real gain */
    /* [0:3] = N meams N /16 X    */
    /* [4:9] = M meams M X         */
    /* Total gain = M + N /16 X   */

    //
	if (gain < BASEGAIN || gain > 8 * BASEGAIN) {
        LOG_INF("Error gain setting");

        if (gain < BASEGAIN)
            gain = BASEGAIN;
		else if (gain > 8 * BASEGAIN)
			gain = 8 * BASEGAIN;		 
    }
	    

    reg_gain = gain2reg(gain);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.gain = reg_gain;
    spin_unlock(&imgsensor_drv_lock);
    LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor(0x0104, 0x01);
    write_cmos_sensor_16bit(0x305E, reg_gain);
    write_cmos_sensor(0x0104, 0x00);

    return gain;
}    /*    set_gain  */

static void ihdr_write_shutter_gain(kal_uint16 le, kal_uint16 se, kal_uint16 gain)
{
	return;
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
					break;
				case IMAGE_H_MIRROR:
					break;
				case IMAGE_V_MIRROR:
					break;
				case IMAGE_HV_MIRROR:
					break;
				default:
					LOG_INF("Error image_mirror setting\n");
    }

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

static void sensor_init(void)
{
    LOG_INF("E\n");
	write_cmos_sensor(0x0103, 0x01);  // SOFTWARE_RESET
	mdelay(50);
	//LOG_INF("E\n");
	write_cmos_sensor_16bit(0x3042, 0x500C);  // DARK_CONTROL2
	write_cmos_sensor_16bit(0x3044, 0x0500);  // DARK_CONTROL
	write_cmos_sensor_16bit(0x30EE, 0x5133);  // DARK_CONTROL3
	write_cmos_sensor_16bit(0x30D2, 0x0000);  // CRM_CONTROL
	write_cmos_sensor_16bit(0x3C50, 0x0001);  // BPC_SETUP
	write_cmos_sensor_16bit(0x31E0, 0x0001);  // PIX_DEF_ID
	write_cmos_sensor_16bit(0x3180, 0x9434);  // FINEDIGCORR_CONTROL
	write_cmos_sensor_16bit(0x3F2C, 0x2028);  // GTH_THRES_RTN
	write_cmos_sensor_16bit(0x3C58, 0x03FF);  // BPC_BLOOM_TH
	write_cmos_sensor_16bit(0x3C5A, 0x00FD);  // BPC_TAG_MODE
	write_cmos_sensor_16bit(0x3C5C, 0x000A);  // BPC_QR_SETUP
	write_cmos_sensor_16bit(0x3C5E, 0x000A);  // BPC_GP_NUM_OF
	write_cmos_sensor_16bit(0x3C60, 0x0500);  // BPC_GP_SIGMA_SETUP
	write_cmos_sensor_16bit(0x3C62, 0xECA0);  // BPC_PC_MASK
	write_cmos_sensor_16bit(0x3C64, 0x000F);  // BPC_PC_SIGMA
	write_cmos_sensor_16bit(0x3C66, 0x002A);  // BPC_NOISE_LIFT
	write_cmos_sensor_16bit(0x3C68, 0x0000);  // BPC_BOISE_CEOF
	write_cmos_sensor_16bit(0x3C6A, 0x0006);  // BPC_NOISE_FLOOR0
	write_cmos_sensor_16bit(0x3C6C, 0x000A);  // BPC_NOISE_FLOOR1
	write_cmos_sensor_16bit(0x3C6E, 0x000E);  // BPC_NOISE_FLOOR2
	write_cmos_sensor_16bit(0x3C70, 0x0012);  // BPC_NOISE_FLOOR3
	write_cmos_sensor_16bit(0x3C72, 0x2010);  // BPC_NOISE_GAIN_TH10
	write_cmos_sensor_16bit(0x3C74, 0x0040);  // BPC_NOISE_GAIN_TH2
	write_cmos_sensor_16bit(0x3C78, 0x0000);  // BPC_NOISE_ADD_GP
	write_cmos_sensor_16bit(0x3C7A, 0x0210);  // BPC_DEFECT
	write_cmos_sensor_16bit(0x3C7C, 0x000A);  // BPC_SF_SIGMA
	write_cmos_sensor_16bit(0x3C7E, 0x20FF);  // BPC_DF_SIGMA10
	write_cmos_sensor_16bit(0x3C80, 0x0810);  // BPC_DF_SIGMA32
	write_cmos_sensor_16bit(0x3C82, 0x20FF);  // BPC_DC_SIGMA10
	write_cmos_sensor_16bit(0x3C84, 0x0810);  // BPC_DC_SIGMA32
	write_cmos_sensor_16bit(0x3C86, 0x4210);  // BPC_DC_F
	write_cmos_sensor_16bit(0x3C88, 0x0000);  // BPC_DEN_EN10
	write_cmos_sensor_16bit(0x3C8A, 0x0000);  // BPC_DEN_EN32
	write_cmos_sensor_16bit(0x3C8C, 0x0010);  // BPC_DEN_SIGMA
	write_cmos_sensor_16bit(0x3C8E, 0x03F5);  // BPC_DEN_DIS_TH
	write_cmos_sensor_16bit(0x3C90, 0x0000);  // BPC_TRI_ABYP_SIGMA
	write_cmos_sensor_16bit(0x3C94, 0x47E0);  // BPC_PDAF_REC_SETUP1
	write_cmos_sensor_16bit(0x3C96, 0x6000);  // BPC_PDAF_TAG_GRAD_EN
	write_cmos_sensor_16bit(0x3C98, 0x0000);  // BPC_PDAF_REC_TOL_ABS0
	write_cmos_sensor_16bit(0x3C9A, 0x0000);  // BPC_PDAF_REC_TOL_ABS1
	write_cmos_sensor_16bit(0x3C9C, 0x0000);  // BPC_PDAF_REC_TOL_ABS2
	write_cmos_sensor_16bit(0x3C9E, 0x0000);  // BPC_PDAF_REC_TOL_ABS3
	write_cmos_sensor_16bit(0x3CA0, 0x0000);  // BPC_PDAF_REC_TOL_ABS4
	write_cmos_sensor_16bit(0x3CA2, 0x0000);  // BPC_PDAF_REC_TOL_ABS5
	write_cmos_sensor_16bit(0x3CA4, 0x0000);  // BPC_PDAF_REC_TOL_ABS6
	write_cmos_sensor_16bit(0x3CA6, 0x0000);  // BPC_PDAF_REC_TOL_ABS7
	write_cmos_sensor_16bit(0x3CC0, 0x0000);  // BPC_PDAF_REC_TOL_DARK
	write_cmos_sensor_16bit(0x3CC2, 0x03E8);  // BPC_BM_T0
	write_cmos_sensor_16bit(0x3CC4, 0x07D0);  // BPC_BM_T1
	write_cmos_sensor_16bit(0x3CC6, 0x0BB8);  // BPC_BM_T2
	write_cmos_sensor_16bit(0x3222, 0xA0C0);  // PDAF_ROW_CONTROL
	write_cmos_sensor_16bit(0x322E, 0x0000);  // PDAF_PEDESTAL
	write_cmos_sensor_16bit(0x3230, 0x0FF0);  // PDAF_SAT_TH
	write_cmos_sensor_16bit(0x32BC, 0x0019);  // PDAF_DC_TH_ABS
	write_cmos_sensor_16bit(0x32BE, 0x0013);  // PDAF_DC_TH_REL
	write_cmos_sensor_16bit(0x32C0, 0x0010);  // PDAF_DC_DIFF_FACTOR
	write_cmos_sensor_16bit(0x3D00, 0x0446);  // DYNAMIC_SEQRAM_00
	write_cmos_sensor_16bit(0x3D02, 0x4C66);  // DYNAMIC_SEQRAM_02
	write_cmos_sensor_16bit(0x3D04, 0xFFFF);  // DYNAMIC_SEQRAM_04
	write_cmos_sensor_16bit(0x3D06, 0xFFFF);  // DYNAMIC_SEQRAM_06
	write_cmos_sensor_16bit(0x3D08, 0x5E40);  // DYNAMIC_SEQRAM_08
	write_cmos_sensor_16bit(0x3D0A, 0x1106);  // DYNAMIC_SEQRAM_0A
	write_cmos_sensor_16bit(0x3D0C, 0x8041);  // DYNAMIC_SEQRAM_0C
	write_cmos_sensor_16bit(0x3D0E, 0x4F83);  // DYNAMIC_SEQRAM_0E
	write_cmos_sensor_16bit(0x3D10, 0x4200);  // DYNAMIC_SEQRAM_10
	write_cmos_sensor_16bit(0x3D12, 0xC055);  // DYNAMIC_SEQRAM_12
	write_cmos_sensor_16bit(0x3D14, 0x805B);  // DYNAMIC_SEQRAM_14
	write_cmos_sensor_16bit(0x3D16, 0x8360);  // DYNAMIC_SEQRAM_16
	write_cmos_sensor_16bit(0x3D18, 0x845A);  // DYNAMIC_SEQRAM_18
	write_cmos_sensor_16bit(0x3D1A, 0x8D00);  // DYNAMIC_SEQRAM_1A
	write_cmos_sensor_16bit(0x3D1C, 0xC083);  // DYNAMIC_SEQRAM_1C
	write_cmos_sensor_16bit(0x3D1E, 0x428F);  // DYNAMIC_SEQRAM_1E
	write_cmos_sensor_16bit(0x3D20, 0x5281);  // DYNAMIC_SEQRAM_20
	write_cmos_sensor_16bit(0x3D22, 0x5A85);  // DYNAMIC_SEQRAM_22
	write_cmos_sensor_16bit(0x3D24, 0x5364);  // DYNAMIC_SEQRAM_24
	write_cmos_sensor_16bit(0x3D26, 0x1030);  // DYNAMIC_SEQRAM_26
	write_cmos_sensor_16bit(0x3D28, 0x801C);  // DYNAMIC_SEQRAM_28
	write_cmos_sensor_16bit(0x3D2A, 0x00C3);  // DYNAMIC_SEQRAM_2A
	write_cmos_sensor_16bit(0x3D2C, 0x5756);  // DYNAMIC_SEQRAM_2C
	write_cmos_sensor_16bit(0x3D2E, 0x9651);  // DYNAMIC_SEQRAM_2E
	write_cmos_sensor_16bit(0x3D30, 0x5080);  // DYNAMIC_SEQRAM_30
	write_cmos_sensor_16bit(0x3D32, 0x4D43);  // DYNAMIC_SEQRAM_32
	write_cmos_sensor_16bit(0x3D34, 0x8252);  // DYNAMIC_SEQRAM_34
	write_cmos_sensor_16bit(0x3D36, 0x8058);  // DYNAMIC_SEQRAM_36
	write_cmos_sensor_16bit(0x3D38, 0x58A4);  // DYNAMIC_SEQRAM_38
	write_cmos_sensor_16bit(0x3D3A, 0x439A);  // DYNAMIC_SEQRAM_3A
	write_cmos_sensor_16bit(0x3D3C, 0x4592);  // DYNAMIC_SEQRAM_3C
	write_cmos_sensor_16bit(0x3D3E, 0x45A7);  // DYNAMIC_SEQRAM_3E
	write_cmos_sensor_16bit(0x3D40, 0x5294);  // DYNAMIC_SEQRAM_40
	write_cmos_sensor_16bit(0x3D42, 0x51E6);  // DYNAMIC_SEQRAM_42
	write_cmos_sensor_16bit(0x3D44, 0x5186);  // DYNAMIC_SEQRAM_44
	write_cmos_sensor_16bit(0x3D46, 0x10C4);  // DYNAMIC_SEQRAM_46
	write_cmos_sensor_16bit(0x3D48, 0x9C4F);  // DYNAMIC_SEQRAM_48
	write_cmos_sensor_16bit(0x3D4A, 0x8659);  // DYNAMIC_SEQRAM_4A
	write_cmos_sensor_16bit(0x3D4C, 0x59E6);  // DYNAMIC_SEQRAM_4C
	write_cmos_sensor_16bit(0x3D4E, 0x4293);  // DYNAMIC_SEQRAM_4E
	write_cmos_sensor_16bit(0x3D50, 0x6182);  // DYNAMIC_SEQRAM_50
	write_cmos_sensor_16bit(0x3D52, 0x6283);  // DYNAMIC_SEQRAM_52
	write_cmos_sensor_16bit(0x3D54, 0x4281);  // DYNAMIC_SEQRAM_54
	write_cmos_sensor_16bit(0x3D56, 0x4164);  // DYNAMIC_SEQRAM_56
	write_cmos_sensor_16bit(0x3D58, 0xFFFF);  // DYNAMIC_SEQRAM_58
	write_cmos_sensor_16bit(0x3D5A, 0xB740);  // DYNAMIC_SEQRAM_5A
	write_cmos_sensor_16bit(0x3D5C, 0x8140);  // DYNAMIC_SEQRAM_5C
	write_cmos_sensor_16bit(0x3D5E, 0x8041);  // DYNAMIC_SEQRAM_5E
	write_cmos_sensor_16bit(0x3D60, 0x8042);  // DYNAMIC_SEQRAM_60
	write_cmos_sensor_16bit(0x3D62, 0x8043);  // DYNAMIC_SEQRAM_62
	write_cmos_sensor_16bit(0x3D64, 0x8D44);  // DYNAMIC_SEQRAM_64
	write_cmos_sensor_16bit(0x3D66, 0xBA44);  // DYNAMIC_SEQRAM_66
	write_cmos_sensor_16bit(0x3D68, 0x8843);  // DYNAMIC_SEQRAM_68
	write_cmos_sensor_16bit(0x3D6A, 0x8042);  // DYNAMIC_SEQRAM_6A
	write_cmos_sensor_16bit(0x3D6C, 0x4181);  // DYNAMIC_SEQRAM_6C
	write_cmos_sensor_16bit(0x3D6E, 0x4082);  // DYNAMIC_SEQRAM_6E
	write_cmos_sensor_16bit(0x3D70, 0x4080);  // DYNAMIC_SEQRAM_70
	write_cmos_sensor_16bit(0x3D72, 0x4180);  // DYNAMIC_SEQRAM_72
	write_cmos_sensor_16bit(0x3D74, 0x4280);  // DYNAMIC_SEQRAM_74
	write_cmos_sensor_16bit(0x3D76, 0x438D);  // DYNAMIC_SEQRAM_76
	write_cmos_sensor_16bit(0x3D78, 0x44BA);  // DYNAMIC_SEQRAM_78
	write_cmos_sensor_16bit(0x3D7A, 0x4487);  // DYNAMIC_SEQRAM_7A
	write_cmos_sensor_16bit(0x3D7C, 0x5E43);  // DYNAMIC_SEQRAM_7C
	write_cmos_sensor_16bit(0x3D7E, 0x5442);  // DYNAMIC_SEQRAM_7E
	write_cmos_sensor_16bit(0x3D80, 0x4181);  // DYNAMIC_SEQRAM_80
	write_cmos_sensor_16bit(0x3D82, 0x4081);  // DYNAMIC_SEQRAM_82
	write_cmos_sensor_16bit(0x3D84, 0x5B81);  // DYNAMIC_SEQRAM_84
	write_cmos_sensor_16bit(0x3D86, 0x6026);  // DYNAMIC_SEQRAM_86
	write_cmos_sensor_16bit(0x3D88, 0x0055);  // DYNAMIC_SEQRAM_88
	write_cmos_sensor_16bit(0x3D8A, 0x8070);  // DYNAMIC_SEQRAM_8A
	write_cmos_sensor_16bit(0x3D8C, 0x8040);  // DYNAMIC_SEQRAM_8C
	write_cmos_sensor_16bit(0x3D8E, 0x4C81);  // DYNAMIC_SEQRAM_8E
	write_cmos_sensor_16bit(0x3D90, 0x45C3);  // DYNAMIC_SEQRAM_90
	write_cmos_sensor_16bit(0x3D92, 0x4581);  // DYNAMIC_SEQRAM_92
	write_cmos_sensor_16bit(0x3D94, 0x4C40);  // DYNAMIC_SEQRAM_94
	write_cmos_sensor_16bit(0x3D96, 0x8070);  // DYNAMIC_SEQRAM_96
	write_cmos_sensor_16bit(0x3D98, 0x8040);  // DYNAMIC_SEQRAM_98
	write_cmos_sensor_16bit(0x3D9A, 0x4C85);  // DYNAMIC_SEQRAM_9A
	write_cmos_sensor_16bit(0x3D9C, 0x6CA8);  // DYNAMIC_SEQRAM_9C
	write_cmos_sensor_16bit(0x3D9E, 0x6C8C);  // DYNAMIC_SEQRAM_9E
	write_cmos_sensor_16bit(0x3DA0, 0x000E);  // DYNAMIC_SEQRAM_A0
	write_cmos_sensor_16bit(0x3DA2, 0xBE44);  // DYNAMIC_SEQRAM_A2
	write_cmos_sensor_16bit(0x3DA4, 0x8844);  // DYNAMIC_SEQRAM_A4
	write_cmos_sensor_16bit(0x3DA6, 0xBC78);  // DYNAMIC_SEQRAM_A6
	write_cmos_sensor_16bit(0x3DA8, 0x0900);  // DYNAMIC_SEQRAM_A8
	write_cmos_sensor_16bit(0x3DAA, 0x8904);  // DYNAMIC_SEQRAM_AA
	write_cmos_sensor_16bit(0x3DAC, 0x8080);  // DYNAMIC_SEQRAM_AC
	write_cmos_sensor_16bit(0x3DAE, 0x0240);  // DYNAMIC_SEQRAM_AE
	write_cmos_sensor_16bit(0x3DB0, 0x8609);  // DYNAMIC_SEQRAM_B0
	write_cmos_sensor_16bit(0x3DB2, 0x008E);  // DYNAMIC_SEQRAM_B2
	write_cmos_sensor_16bit(0x3DB4, 0x0900);  // DYNAMIC_SEQRAM_B4
	write_cmos_sensor_16bit(0x3DB6, 0x8002);  // DYNAMIC_SEQRAM_B6
	write_cmos_sensor_16bit(0x3DB8, 0x4080);  // DYNAMIC_SEQRAM_B8
	write_cmos_sensor_16bit(0x3DBA, 0x0480);  // DYNAMIC_SEQRAM_BA
	write_cmos_sensor_16bit(0x3DBC, 0x887C);  // DYNAMIC_SEQRAM_BC
	write_cmos_sensor_16bit(0x3DBE, 0xAA86);  // DYNAMIC_SEQRAM_BE
	write_cmos_sensor_16bit(0x3DC0, 0x0900);  // DYNAMIC_SEQRAM_C0
	write_cmos_sensor_16bit(0x3DC2, 0x877A);  // DYNAMIC_SEQRAM_C2
	write_cmos_sensor_16bit(0x3DC4, 0x000E);  // DYNAMIC_SEQRAM_C4
	write_cmos_sensor_16bit(0x3DC6, 0xC379);  // DYNAMIC_SEQRAM_C6
	write_cmos_sensor_16bit(0x3DC8, 0x4C40);  // DYNAMIC_SEQRAM_C8
	write_cmos_sensor_16bit(0x3DCA, 0xBF70);  // DYNAMIC_SEQRAM_CA
	write_cmos_sensor_16bit(0x3DCC, 0x5E40);  // DYNAMIC_SEQRAM_CC
	write_cmos_sensor_16bit(0x3DCE, 0x114E);  // DYNAMIC_SEQRAM_CE
	write_cmos_sensor_16bit(0x3DD0, 0x5D41);  // DYNAMIC_SEQRAM_D0
	write_cmos_sensor_16bit(0x3DD2, 0x5383);  // DYNAMIC_SEQRAM_D2
	write_cmos_sensor_16bit(0x3DD4, 0x4200);  // DYNAMIC_SEQRAM_D4
	write_cmos_sensor_16bit(0x3DD6, 0xC055);  // DYNAMIC_SEQRAM_D6
	write_cmos_sensor_16bit(0x3DD8, 0xA400);  // DYNAMIC_SEQRAM_D8
	write_cmos_sensor_16bit(0x3DDA, 0xC083);  // DYNAMIC_SEQRAM_DA
	write_cmos_sensor_16bit(0x3DDC, 0x4288);  // DYNAMIC_SEQRAM_DC
	write_cmos_sensor_16bit(0x3DDE, 0x6083);  // DYNAMIC_SEQRAM_DE
	write_cmos_sensor_16bit(0x3DE0, 0x5B80);  // DYNAMIC_SEQRAM_E0
	write_cmos_sensor_16bit(0x3DE2, 0x5A64);  // DYNAMIC_SEQRAM_E2
	write_cmos_sensor_16bit(0x3DE4, 0x1030);  // DYNAMIC_SEQRAM_E4
	write_cmos_sensor_16bit(0x3DE6, 0x801C);  // DYNAMIC_SEQRAM_E6
	write_cmos_sensor_16bit(0x3DE8, 0x00A5);  // DYNAMIC_SEQRAM_E8
	write_cmos_sensor_16bit(0x3DEA, 0x5697);  // DYNAMIC_SEQRAM_EA
	write_cmos_sensor_16bit(0x3DEC, 0x57A5);  // DYNAMIC_SEQRAM_EC
	write_cmos_sensor_16bit(0x3DEE, 0x5180);  // DYNAMIC_SEQRAM_EE
	write_cmos_sensor_16bit(0x3DF0, 0x505A);  // DYNAMIC_SEQRAM_F0
	write_cmos_sensor_16bit(0x3DF2, 0x814D);  // DYNAMIC_SEQRAM_F2
	write_cmos_sensor_16bit(0x3DF4, 0x8358);  // DYNAMIC_SEQRAM_F4
	write_cmos_sensor_16bit(0x3DF6, 0x8058);  // DYNAMIC_SEQRAM_F6
	write_cmos_sensor_16bit(0x3DF8, 0xA943);  // DYNAMIC_SEQRAM_F8
	write_cmos_sensor_16bit(0x3DFA, 0x8345);  // DYNAMIC_SEQRAM_FA
	write_cmos_sensor_16bit(0x3DFC, 0xB045);  // DYNAMIC_SEQRAM_FC
	write_cmos_sensor_16bit(0x3DFE, 0x8343);  // DYNAMIC_SEQRAM_FE
	write_cmos_sensor_16bit(0x3E00, 0xA351);  // DYNAMIC_SEQRAM_100
	write_cmos_sensor_16bit(0x3E02, 0xE251);  // DYNAMIC_SEQRAM_102
	write_cmos_sensor_16bit(0x3E04, 0x8C59);  // DYNAMIC_SEQRAM_104
	write_cmos_sensor_16bit(0x3E06, 0x8059);  // DYNAMIC_SEQRAM_106
	write_cmos_sensor_16bit(0x3E08, 0x8A5F);  // DYNAMIC_SEQRAM_108
	write_cmos_sensor_16bit(0x3E0A, 0xEC7C);  // DYNAMIC_SEQRAM_10A
	write_cmos_sensor_16bit(0x3E0C, 0xCC84);  // DYNAMIC_SEQRAM_10C
	write_cmos_sensor_16bit(0x3E0E, 0x6182);  // DYNAMIC_SEQRAM_10E
	write_cmos_sensor_16bit(0x3E10, 0x6283);  // DYNAMIC_SEQRAM_110
	write_cmos_sensor_16bit(0x3E12, 0x4283);  // DYNAMIC_SEQRAM_112
	write_cmos_sensor_16bit(0x3E14, 0x10CC);  // DYNAMIC_SEQRAM_114
	write_cmos_sensor_16bit(0x3E16, 0x6496);  // DYNAMIC_SEQRAM_116
	write_cmos_sensor_16bit(0x3E18, 0x4281);  // DYNAMIC_SEQRAM_118
	write_cmos_sensor_16bit(0x3E1A, 0x41BB);  // DYNAMIC_SEQRAM_11A
	write_cmos_sensor_16bit(0x3E1C, 0x4082);  // DYNAMIC_SEQRAM_11C
	write_cmos_sensor_16bit(0x3E1E, 0x407E);  // DYNAMIC_SEQRAM_11E
	write_cmos_sensor_16bit(0x3E20, 0xCC41);  // DYNAMIC_SEQRAM_120
	write_cmos_sensor_16bit(0x3E22, 0x8042);  // DYNAMIC_SEQRAM_122
	write_cmos_sensor_16bit(0x3E24, 0x8043);  // DYNAMIC_SEQRAM_124
	write_cmos_sensor_16bit(0x3E26, 0x8300);  // DYNAMIC_SEQRAM_126
	write_cmos_sensor_16bit(0x3E28, 0xC088);  // DYNAMIC_SEQRAM_128
	write_cmos_sensor_16bit(0x3E2A, 0x44BA);  // DYNAMIC_SEQRAM_12A
	write_cmos_sensor_16bit(0x3E2C, 0x4488);  // DYNAMIC_SEQRAM_12C
	write_cmos_sensor_16bit(0x3E2E, 0x00C8);  // DYNAMIC_SEQRAM_12E
	write_cmos_sensor_16bit(0x3E30, 0x8042);  // DYNAMIC_SEQRAM_130
	write_cmos_sensor_16bit(0x3E32, 0x4181);  // DYNAMIC_SEQRAM_132
	write_cmos_sensor_16bit(0x3E34, 0x4082);  // DYNAMIC_SEQRAM_134
	write_cmos_sensor_16bit(0x3E36, 0x4080);  // DYNAMIC_SEQRAM_136
	write_cmos_sensor_16bit(0x3E38, 0x4180);  // DYNAMIC_SEQRAM_138
	write_cmos_sensor_16bit(0x3E3A, 0x4280);  // DYNAMIC_SEQRAM_13A
	write_cmos_sensor_16bit(0x3E3C, 0x4383);  // DYNAMIC_SEQRAM_13C
	write_cmos_sensor_16bit(0x3E3E, 0x00C0);  // DYNAMIC_SEQRAM_13E
	write_cmos_sensor_16bit(0x3E40, 0x8844);  // DYNAMIC_SEQRAM_140
	write_cmos_sensor_16bit(0x3E42, 0xBA44);  // DYNAMIC_SEQRAM_142
	write_cmos_sensor_16bit(0x3E44, 0x8800);  // DYNAMIC_SEQRAM_144
	write_cmos_sensor_16bit(0x3E46, 0xC880);  // DYNAMIC_SEQRAM_146
	write_cmos_sensor_16bit(0x3E48, 0x4241);  // DYNAMIC_SEQRAM_148
	write_cmos_sensor_16bit(0x3E4A, 0x8240);  // DYNAMIC_SEQRAM_14A
	write_cmos_sensor_16bit(0x3E4C, 0x8140);  // DYNAMIC_SEQRAM_14C
	write_cmos_sensor_16bit(0x3E4E, 0x8041);  // DYNAMIC_SEQRAM_14E
	write_cmos_sensor_16bit(0x3E50, 0x8042);  // DYNAMIC_SEQRAM_150
	write_cmos_sensor_16bit(0x3E52, 0x8043);  // DYNAMIC_SEQRAM_152
	write_cmos_sensor_16bit(0x3E54, 0x8300);  // DYNAMIC_SEQRAM_154
	write_cmos_sensor_16bit(0x3E56, 0xC088);  // DYNAMIC_SEQRAM_156
	write_cmos_sensor_16bit(0x3E58, 0x44BA);  // DYNAMIC_SEQRAM_158
	write_cmos_sensor_16bit(0x3E5A, 0x4488);  // DYNAMIC_SEQRAM_15A
	write_cmos_sensor_16bit(0x3E5C, 0x00C8);  // DYNAMIC_SEQRAM_15C
	write_cmos_sensor_16bit(0x3E5E, 0x8042);  // DYNAMIC_SEQRAM_15E
	write_cmos_sensor_16bit(0x3E60, 0x4181);  // DYNAMIC_SEQRAM_160
	write_cmos_sensor_16bit(0x3E62, 0x4082);  // DYNAMIC_SEQRAM_162
	write_cmos_sensor_16bit(0x3E64, 0x4080);  // DYNAMIC_SEQRAM_164
	write_cmos_sensor_16bit(0x3E66, 0x4180);  // DYNAMIC_SEQRAM_166
	write_cmos_sensor_16bit(0x3E68, 0x4280);  // DYNAMIC_SEQRAM_168
	write_cmos_sensor_16bit(0x3E6A, 0x4383);  // DYNAMIC_SEQRAM_16A
	write_cmos_sensor_16bit(0x3E6C, 0x00C0);  // DYNAMIC_SEQRAM_16C
	write_cmos_sensor_16bit(0x3E6E, 0x8844);  // DYNAMIC_SEQRAM_16E
	write_cmos_sensor_16bit(0x3E70, 0xBA44);  // DYNAMIC_SEQRAM_170
	write_cmos_sensor_16bit(0x3E72, 0x8800);  // DYNAMIC_SEQRAM_172
	write_cmos_sensor_16bit(0x3E74, 0xC880);  // DYNAMIC_SEQRAM_174
	write_cmos_sensor_16bit(0x3E76, 0x4241);  // DYNAMIC_SEQRAM_176
	write_cmos_sensor_16bit(0x3E78, 0x8140);  // DYNAMIC_SEQRAM_178
	write_cmos_sensor_16bit(0x3E7A, 0x9F5E);  // DYNAMIC_SEQRAM_17A
	write_cmos_sensor_16bit(0x3E7C, 0x8A54);  // DYNAMIC_SEQRAM_17C
	write_cmos_sensor_16bit(0x3E7E, 0x8620);  // DYNAMIC_SEQRAM_17E
	write_cmos_sensor_16bit(0x3E80, 0x2881);  // DYNAMIC_SEQRAM_180
	write_cmos_sensor_16bit(0x3E82, 0x6026);  // DYNAMIC_SEQRAM_182
	write_cmos_sensor_16bit(0x3E84, 0x8055);  // DYNAMIC_SEQRAM_184
	write_cmos_sensor_16bit(0x3E86, 0x8070);  // DYNAMIC_SEQRAM_186
	write_cmos_sensor_16bit(0x3E88, 0x0000);  // DYNAMIC_SEQRAM_188
	write_cmos_sensor_16bit(0x3E8A, 0x0000);  // DYNAMIC_SEQRAM_18A
	write_cmos_sensor_16bit(0x3E8C, 0x0000);  // DYNAMIC_SEQRAM_18C
	write_cmos_sensor_16bit(0x3E8E, 0x0000);  // DYNAMIC_SEQRAM_18E
	write_cmos_sensor_16bit(0x3E90, 0x0000);  // DYNAMIC_SEQRAM_190
	write_cmos_sensor_16bit(0x3E92, 0x0000);  // DYNAMIC_SEQRAM_192
	write_cmos_sensor_16bit(0x3E94, 0x0000);  // DYNAMIC_SEQRAM_194
	write_cmos_sensor_16bit(0x3E96, 0x0000);  // DYNAMIC_SEQRAM_196
	write_cmos_sensor_16bit(0x3E98, 0x0000);  // DYNAMIC_SEQRAM_198
	write_cmos_sensor_16bit(0x3E9A, 0x0000);  // DYNAMIC_SEQRAM_19A
	write_cmos_sensor_16bit(0x3E9C, 0x0000);  // DYNAMIC_SEQRAM_19C
	write_cmos_sensor_16bit(0x3E9E, 0x0000);  // DYNAMIC_SEQRAM_19E
	write_cmos_sensor_16bit(0x3EA0, 0x0000);  // DYNAMIC_SEQRAM_1A0
	write_cmos_sensor_16bit(0x3EA2, 0x0000);  // DYNAMIC_SEQRAM_1A2
	write_cmos_sensor_16bit(0x3EA4, 0x0000);  // DYNAMIC_SEQRAM_1A4
	write_cmos_sensor_16bit(0x3EA6, 0x0000);  // DYNAMIC_SEQRAM_1A6
	write_cmos_sensor_16bit(0x3EA8, 0x0000);  // DYNAMIC_SEQRAM_1A8
	write_cmos_sensor_16bit(0x3EAA, 0x0000);  // DYNAMIC_SEQRAM_1AA
	write_cmos_sensor_16bit(0x3EAC, 0x0000);  // DYNAMIC_SEQRAM_1AC
	write_cmos_sensor_16bit(0x3EAE, 0x0000);  // DYNAMIC_SEQRAM_1AE
	write_cmos_sensor_16bit(0x3EB0, 0x0000);  // DYNAMIC_SEQRAM_1B0
	write_cmos_sensor_16bit(0x3EB2, 0x0000);  // DYNAMIC_SEQRAM_1B2
	write_cmos_sensor_16bit(0x3EB4, 0x0000);  // DYNAMIC_SEQRAM_1B4
	write_cmos_sensor_16bit(0x3EB6, 0x004D);  // DAC_LD_0_1
	write_cmos_sensor_16bit(0x3EBA, 0x21AB);  // DAC_LD_4_5
	write_cmos_sensor_16bit(0x3EBC, 0xAA06);  // DAC_LD_6_7
	write_cmos_sensor_16bit(0x3EC0, 0x1300);  // DAC_LD_10_11
	write_cmos_sensor_16bit(0x3EC2, 0x7000);  // DAC_LD_12_13
	write_cmos_sensor_16bit(0x3EC4, 0x1C08);  // DAC_LD_14_15
	write_cmos_sensor_16bit(0x3EC6, 0xE244);  // DAC_LD_16_17
	write_cmos_sensor_16bit(0x3EC8, 0x0F0F);  // DAC_LD_18_19
	write_cmos_sensor_16bit(0x3ECA, 0x0F4A);  // DAC_LD_20_21
	write_cmos_sensor_16bit(0x3ECC, 0x0706);  // DAC_LD_22_23
	write_cmos_sensor_16bit(0x3ECE, 0x443B);  // DAC_LD_24_25
	write_cmos_sensor_16bit(0x3ED0, 0x12F0);  // DAC_LD_26_27
	write_cmos_sensor_16bit(0x3ED2, 0x0039);  // DAC_LD_28_29
	write_cmos_sensor_16bit(0x3ED4, 0x862F);  // DAC_LD_30_31
	write_cmos_sensor_16bit(0x3ED6, 0x4880);  // DAC_LD_32_33
	write_cmos_sensor_16bit(0x3ED8, 0x0423);  // DAC_LD_34_35
	write_cmos_sensor_16bit(0x3EDA, 0xF882);  // DAC_LD_36_37
	write_cmos_sensor_16bit(0x3EDC, 0x8282);  // DAC_LD_38_39
	write_cmos_sensor_16bit(0x3EDE, 0x8205);  // DAC_LD_40_41
	write_cmos_sensor_16bit(0x316A, 0x8200);  // DAC_RSTLO
	write_cmos_sensor_16bit(0x316C, 0x8200);  // DAC_TXLO
	write_cmos_sensor_16bit(0x316E, 0x8200);  // DAC_ECL
	write_cmos_sensor_16bit(0x3EF0, 0x5165);  // DAC_LD_ECL
	write_cmos_sensor_16bit(0x3EF2, 0x0101);  // DAC_LD_FSC
	write_cmos_sensor_16bit(0x3EF6, 0x030A);  // DAC_LD_RSTD
	write_cmos_sensor_16bit(0x3EFA, 0x0F0F);  // DAC_LD_TXLO
	write_cmos_sensor_16bit(0x3EFC, 0x070F);  // DAC_LD_TXLO1
	write_cmos_sensor_16bit(0x3EFE, 0x0F0F);  // DAC_LD_TXLO2

#ifdef PE_16
	write_cmos_sensor_16bit(0x32D0, 0x6000); // PE_PARAM_ADDR
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00AC); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x189E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA948); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x189F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A48); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x188A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A48); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0806); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x188A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A48); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0806); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2828); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFFF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7FFF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x009E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA948); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x189E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA948); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x189F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A48); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x188A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A48); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0806); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x188A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A48); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0806); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2AC0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D18); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D98); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x401E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0002); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2818); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x11B1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x16AC); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB6AC); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x11B1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0B44); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x020C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x09B4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB224); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x034D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x021C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x09BC); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0150); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x081A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x09B4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x16B4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01B2); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D08); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2828); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x019E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x11B1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0082); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x018A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA082); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2820); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00BE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x003F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00AE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA130); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA130); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xC000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2830); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x020D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA1C0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x032E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA1B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x120F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00AE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA6B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x120F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x003C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x041A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x006E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA1B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D98); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D18); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2828); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB36A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0F6A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0200); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0C1E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x11B1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2230); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2438); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x019E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x819E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA495); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x819F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA495); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1C1D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x819E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0495); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1C1C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x819E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0495); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1C1C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x81DE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0495); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1DBC); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x81FE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA495); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBB34); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBB54); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x020F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFF0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3040); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4007); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2818); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x003E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAD8A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x003F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAD8A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAD8A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x160F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAD8A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x160F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x203E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAD8A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2820); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00AE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAE0A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1811); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAE1A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1411); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x003C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0F0A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x202A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1818); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1402); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x15BE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00AA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x15BE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAE1A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x11B1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x003E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA93C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x11B1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7052); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xABA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFC1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFAA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2AC0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA120); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x02A0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A2); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA2A0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00E3); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x011E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF9F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2AC0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA120); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x02A0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A2); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA2A0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00E3); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x011E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA920); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA922); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FBA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0006); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA024); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0200); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA124); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0BAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA124); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x805E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x011E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF8B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x011E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF86); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3052); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x009E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2DD0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x8000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2820); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x019E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x04C8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x019E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA948); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA4C8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0018); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0946); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x818A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0936); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0200); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x04C8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FBA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xC000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2820); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x011E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x014E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x140F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x800E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x140F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x004E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x140F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x140F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x004E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x140F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x140F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x004E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x15AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x30F4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAABA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAEA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7009); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2428); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2228); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFCE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2620); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FA1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E01); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7009); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7002); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x004E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0201); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFCF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3040); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFFE); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFFF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0DAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0C0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x085E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0DAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0003); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0040); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF35); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF31); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x6010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x6010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF2D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x6010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x6010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x6010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF29); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x6010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7006); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAABA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAEA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA93A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA93A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA93A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA93A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3019); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3026); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7017); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FA1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E01); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E01); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FA1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7004); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7009); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2420); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF3A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2220); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7017); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7018); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2628); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2630); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x09A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7017); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7009); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7004); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7005); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x004E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0201); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFB0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3040); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7005); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x020F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7051); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3019); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3026); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3033); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2AC0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x303F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB236); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB6B6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA936); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA936); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA936); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0800); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2AC0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2228); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2420); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x009E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB248); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x009E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x10C8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB0C8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB242); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x040F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x12C2); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x05A0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x12CA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x05A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x12CA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A0A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2CA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x17B7); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x12CA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1616); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x12CA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1616); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x124A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1616); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x006A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB24A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0017); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x004A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB24A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x04B7); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x802A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB24A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x05B7); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7004); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFE76); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7050); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2638); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9C8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A2); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x89A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB24A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01B7); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB24A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0017); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x002A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB24A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x04B7); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x802A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB24A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x05B7); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7004); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFE5B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2638); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9C8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A2); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x89A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D18); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB948); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBA28); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBAA8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBAA8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBAA8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7051); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7051); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0201); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF9E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3040); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7005); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFE29); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB224); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB224); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB224); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFE20); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB224); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB224); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB224); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1201); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7011); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x13A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1201); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x201E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x13A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFE00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x045E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x13A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x13A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFDFB); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x6010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x105E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x13A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x13A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2CA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBB4A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0201); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBB6C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x021A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x003A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x12EC); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03B6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1076); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB26C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A1A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x003A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x12CA); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03B2); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x047E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB26C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBE5E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x05A5); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1E5E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0404); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0002); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x060F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFDF5); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x201E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x05AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0100); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2AC0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x09AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x080F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB2A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x09AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3019); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3026); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3033); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2AC0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x303F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xADB6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB236); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB6B6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA936); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA936); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0E0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA936); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7005); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B6); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x020F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7051); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3018); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7003); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAAA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAEA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2428); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x009E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA8A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x201E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2430); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x009E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA8A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x081E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7004); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2400); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFDEB); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2428); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x009E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA8A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x201E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2430); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x009E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA8A4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x081E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7010); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7004); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7009); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2400); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFDC7); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704D); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x704E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2D18); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB948); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xB9A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBA28); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBAA8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBAA8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xBAA8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7009); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7004); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7051); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7051); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0201); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFF8E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3040); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3018); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x061E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAAA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAEA4); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA24); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7009); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0006); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A80); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0002); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0C1E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A3); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA0B0); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA030); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x1A0F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA1A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x120F); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x200E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA1A8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x03AF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFD65); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x4000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2620); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7009); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x601E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7008); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0A38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x007E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0014); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x005E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x010E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x801E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA925); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x700A); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A40); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7002); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2418); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x008E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x181E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA924); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0FAF); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xAA38); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x004E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0201); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFFC9); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3040); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x001E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xA9B8); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x01A1); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x00E7); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2A00); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x43FC); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2618); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0xFDA2); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x3600); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x7018); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2218); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x2000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D0, 0x3000); // PE_PARAM_ADDR
	write_cmos_sensor_16bit(0x32D4, 0x004E); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0066); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000B); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0048); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0068); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x000C); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0014); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0001); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x32D4, 0x0000); // PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x3220, 0x0C13);  // PDAF_CONTROL
	write_cmos_sensor_16bit(0x30EC, 0xFB08);  // CTX_RD_DATA //Corn
	//LOG_CORN("[Corn]PE 16BIT \n");	
#endif
#ifdef PE_10
	write_cmos_sensor_16bit(0x32D0, 0x6000); // PE_PARAM_ADDR
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x00B0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x189E);
	write_cmos_sensor_16bit(0x32D4, 0xA948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x189F);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0E0E);
	write_cmos_sensor_16bit(0x32D4, 0x188A);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0806);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2828);
	write_cmos_sensor_16bit(0x32D4, 0xFFFF);
	write_cmos_sensor_16bit(0x32D4, 0x7FFF);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x189E);
	write_cmos_sensor_16bit(0x32D4, 0xA948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x189F);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0E0E);
	write_cmos_sensor_16bit(0x32D4, 0x188A);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0806);
	write_cmos_sensor_16bit(0x32D4, 0x188A);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0806);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D98);
	write_cmos_sensor_16bit(0x32D4, 0x401E);
	write_cmos_sensor_16bit(0x32D4, 0x0002);
	write_cmos_sensor_16bit(0x32D4, 0x2818);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001F);
	write_cmos_sensor_16bit(0x32D4, 0x16AC);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB6AC);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0B44);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0xA9B4);
	write_cmos_sensor_16bit(0x32D4, 0x010D);
	write_cmos_sensor_16bit(0x32D4, 0x020C);
	write_cmos_sensor_16bit(0x32D4, 0x09B4);
	write_cmos_sensor_16bit(0x32D4, 0x01A4);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x034D);
	write_cmos_sensor_16bit(0x32D4, 0x021C);
	write_cmos_sensor_16bit(0x32D4, 0xBA24);
	write_cmos_sensor_16bit(0x32D4, 0x0E0D);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x09BC);
	write_cmos_sensor_16bit(0x32D4, 0x0150);
	write_cmos_sensor_16bit(0x32D4, 0x081A);
	write_cmos_sensor_16bit(0x32D4, 0x09B4);
	write_cmos_sensor_16bit(0x32D4, 0x01AE);
	write_cmos_sensor_16bit(0x32D4, 0x001A);
	write_cmos_sensor_16bit(0x32D4, 0x16B4);
	write_cmos_sensor_16bit(0x32D4, 0x01B2);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D08);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2828);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001F);
	write_cmos_sensor_16bit(0x32D4, 0x0082);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x018A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0xA082);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2238);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2438);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2638);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x00BE);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x003F);
	write_cmos_sensor_16bit(0x32D4, 0x00A4);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x00AE);
	write_cmos_sensor_16bit(0x32D4, 0xA130);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x002C);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x002E);
	write_cmos_sensor_16bit(0x32D4, 0xA130);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0xC000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2830);
	write_cmos_sensor_16bit(0x32D4, 0x020D);
	write_cmos_sensor_16bit(0x32D4, 0xA1C0);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x032E);
	write_cmos_sensor_16bit(0x32D4, 0xA1B0);
	write_cmos_sensor_16bit(0x32D4, 0x120F);
	write_cmos_sensor_16bit(0x32D4, 0x00AE);
	write_cmos_sensor_16bit(0x32D4, 0xA6B0);
	write_cmos_sensor_16bit(0x32D4, 0x120F);
	write_cmos_sensor_16bit(0x32D4, 0x003C);
	write_cmos_sensor_16bit(0x32D4, 0x00B0);
	write_cmos_sensor_16bit(0x32D4, 0x041A);
	write_cmos_sensor_16bit(0x32D4, 0x006E);
	write_cmos_sensor_16bit(0x32D4, 0xA1B0);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D98);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2828);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB36A);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0F6A);
	write_cmos_sensor_16bit(0x32D4, 0x0200);
	write_cmos_sensor_16bit(0x32D4, 0x0C1E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x2230);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x2438);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x819E);
	write_cmos_sensor_16bit(0x32D4, 0xA495);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x819F);
	write_cmos_sensor_16bit(0x32D4, 0xA495);
	write_cmos_sensor_16bit(0x32D4, 0x1C1D);
	write_cmos_sensor_16bit(0x32D4, 0x819E);
	write_cmos_sensor_16bit(0x32D4, 0x0495);
	write_cmos_sensor_16bit(0x32D4, 0x1C1C);
	write_cmos_sensor_16bit(0x32D4, 0x819E);
	write_cmos_sensor_16bit(0x32D4, 0x0495);
	write_cmos_sensor_16bit(0x32D4, 0x1C1C);
	write_cmos_sensor_16bit(0x32D4, 0x81DE);
	write_cmos_sensor_16bit(0x32D4, 0x0495);
	write_cmos_sensor_16bit(0x32D4, 0x1DBC);
	write_cmos_sensor_16bit(0x32D4, 0x81FE);
	write_cmos_sensor_16bit(0x32D4, 0xA495);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xBB34);
	write_cmos_sensor_16bit(0x32D4, 0x03AF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xBB54);
	write_cmos_sensor_16bit(0x32D4, 0x020F);
	write_cmos_sensor_16bit(0x32D4, 0xFFF0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D00);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4007);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2818);
	write_cmos_sensor_16bit(0x32D4, 0x003E);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x003F);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x002E);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x160F);
	write_cmos_sensor_16bit(0x32D4, 0x002E);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x160F);
	write_cmos_sensor_16bit(0x32D4, 0x203E);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x00AE);
	write_cmos_sensor_16bit(0x32D4, 0xAE0A);
	write_cmos_sensor_16bit(0x32D4, 0x1811);
	write_cmos_sensor_16bit(0x32D4, 0x002F);
	write_cmos_sensor_16bit(0x32D4, 0xAE1A);
	write_cmos_sensor_16bit(0x32D4, 0x1411);
	write_cmos_sensor_16bit(0x32D4, 0x003C);
	write_cmos_sensor_16bit(0x32D4, 0x0F0A);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x202A);
	write_cmos_sensor_16bit(0x32D4, 0x0E0A);
	write_cmos_sensor_16bit(0x32D4, 0x1818);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0x0E0C);
	write_cmos_sensor_16bit(0x32D4, 0x1402);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0x0E0C);
	write_cmos_sensor_16bit(0x32D4, 0x15BE);
	write_cmos_sensor_16bit(0x32D4, 0x00AA);
	write_cmos_sensor_16bit(0x32D4, 0x0E0C);
	write_cmos_sensor_16bit(0x32D4, 0x15BE);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAE1A);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x003E);
	write_cmos_sensor_16bit(0x32D4, 0xA93C);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x7052);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xABA4);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0xFFBE);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFFA7);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA120);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x02A0);
	write_cmos_sensor_16bit(0x32D4, 0x01A2);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA2A0);
	write_cmos_sensor_16bit(0x32D4, 0x00E3);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFF9C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA120);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x02A0);
	write_cmos_sensor_16bit(0x32D4, 0x01A2);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA2A0);
	write_cmos_sensor_16bit(0x32D4, 0x00E3);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA920);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA922);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x0FBA);
	write_cmos_sensor_16bit(0x32D4, 0x0006);
	write_cmos_sensor_16bit(0x32D4, 0xA024);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x0200);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA124);
	write_cmos_sensor_16bit(0x32D4, 0x0BAF);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA124);
	write_cmos_sensor_16bit(0x32D4, 0x0A0F);
	write_cmos_sensor_16bit(0x32D4, 0x805E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFF88);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x704D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFF83);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x3052);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2DD0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x8000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x04C8);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0xA948);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x801F);
	write_cmos_sensor_16bit(0x32D4, 0xA4C8);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0018);
	write_cmos_sensor_16bit(0x32D4, 0x0946);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x818A);
	write_cmos_sensor_16bit(0x32D4, 0x0936);
	write_cmos_sensor_16bit(0x32D4, 0x0200);
	write_cmos_sensor_16bit(0x32D4, 0x4008);
	write_cmos_sensor_16bit(0x32D4, 0x04C8);
	write_cmos_sensor_16bit(0x32D4, 0x0FBA);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0002);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x0003);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x03FC);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0x0925);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x0011);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x8006);
	write_cmos_sensor_16bit(0x32D4, 0x0DB7);
	write_cmos_sensor_16bit(0x32D4, 0x0014);
	write_cmos_sensor_16bit(0x32D4, 0x0008);
	write_cmos_sensor_16bit(0x32D4, 0x0DC8);
	write_cmos_sensor_16bit(0x32D4, 0x0E12);
	write_cmos_sensor_16bit(0x32D4, 0x0040);
	write_cmos_sensor_16bit(0x32D4, 0x0DC8);
	write_cmos_sensor_16bit(0x32D4, 0x0FB2);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x1248);
	write_cmos_sensor_16bit(0x32D4, 0x0E14);
	write_cmos_sensor_16bit(0x32D4, 0x0050);
	write_cmos_sensor_16bit(0x32D4, 0x1248);
	write_cmos_sensor_16bit(0x32D4, 0x0DB4);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x1248);
	write_cmos_sensor_16bit(0x32D4, 0x0C14);
	write_cmos_sensor_16bit(0x32D4, 0x0050);
	write_cmos_sensor_16bit(0x32D4, 0x1248);
	write_cmos_sensor_16bit(0x32D4, 0x0DB4);
	write_cmos_sensor_16bit(0x32D4, 0x0016);
	write_cmos_sensor_16bit(0x32D4, 0xB248);
	write_cmos_sensor_16bit(0x32D4, 0x0C0F);
	write_cmos_sensor_16bit(0x32D4, 0x0056);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0DAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x30F4);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x7000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAABA);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAEA4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2428);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2228);
	write_cmos_sensor_16bit(0x32D4, 0xFFCA);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2620);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xADB8);
	write_cmos_sensor_16bit(0x32D4, 0x0FA1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x0E01);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x700B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x7002);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFFCF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x7800);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFFFE);
	write_cmos_sensor_16bit(0x32D4, 0xFFFF);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x700C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A4);
	write_cmos_sensor_16bit(0x32D4, 0x0DAF);
	write_cmos_sensor_16bit(0x32D4, 0x001C);
	write_cmos_sensor_16bit(0x32D4, 0xADA4);
	write_cmos_sensor_16bit(0x32D4, 0x0C0F);
	write_cmos_sensor_16bit(0x32D4, 0x085E);
	write_cmos_sensor_16bit(0x32D4, 0xADA4);
	write_cmos_sensor_16bit(0x32D4, 0x0DAF);
	write_cmos_sensor_16bit(0x32D4, 0x0003);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0040);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0xFF30);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0xFF2E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0xFF2C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x2010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0xFF2A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x2010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x7006);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAABA);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAEA4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA93A);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA93A);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA93A);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA93A);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x3019);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x3026);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x7017);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FA1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xADA4);
	write_cmos_sensor_16bit(0x32D4, 0x0E01);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xADA4);
	write_cmos_sensor_16bit(0x32D4, 0x0E01);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xADA4);
	write_cmos_sensor_16bit(0x32D4, 0x0FA1);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2420);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFF3E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2220);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x704C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7017);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7018);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2628);
	write_cmos_sensor_16bit(0x32D4, 0x704D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2630);
	write_cmos_sensor_16bit(0x32D4, 0x704E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x09A8);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x7017);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFFB0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B6);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B6);
	write_cmos_sensor_16bit(0x32D4, 0x020F);
	write_cmos_sensor_16bit(0x32D4, 0x7051);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x3019);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x3026);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x3033);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x303F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xADB6);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB236);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB6B6);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x0800);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x704F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2228);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2420);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xB248);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0x10C8);
	write_cmos_sensor_16bit(0x32D4, 0x0E0E);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xB0C8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x010A);
	write_cmos_sensor_16bit(0x32D4, 0xB242);
	write_cmos_sensor_16bit(0x32D4, 0x040F);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12C2);
	write_cmos_sensor_16bit(0x32D4, 0x05A0);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x05A4);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x0A0A);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0xB2CA);
	write_cmos_sensor_16bit(0x32D4, 0x17B7);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x1616);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x1616);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x124A);
	write_cmos_sensor_16bit(0x32D4, 0x1616);
	write_cmos_sensor_16bit(0x32D4, 0x006A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x0017);
	write_cmos_sensor_16bit(0x32D4, 0x004A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x04B7);
	write_cmos_sensor_16bit(0x32D4, 0x802A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x05B7);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFE77);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x7050);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2638);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9C8);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x03A2);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x89A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A0);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x704F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x01B7);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x0017);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x04B7);
	write_cmos_sensor_16bit(0x32D4, 0x802A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x05B7);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFE5C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x704F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2638);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9C8);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x03A2);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x89A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A0);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x704D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x811E);
	write_cmos_sensor_16bit(0x32D4, 0xB949);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x609E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x189E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB9A8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBA28);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x7051);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7051);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFFA3);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFE2F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFE26);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2B8);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xB2B8);
	write_cmos_sensor_16bit(0x32D4, 0x1201);
	write_cmos_sensor_16bit(0x32D4, 0x700F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D40);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7006);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x0A0F);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x12A4);
	write_cmos_sensor_16bit(0x32D4, 0x0A0E);
	write_cmos_sensor_16bit(0x32D4, 0x0118);
	write_cmos_sensor_16bit(0x32D4, 0x12A4);
	write_cmos_sensor_16bit(0x32D4, 0x0BA0);
	write_cmos_sensor_16bit(0x32D4, 0x0018);
	write_cmos_sensor_16bit(0x32D4, 0x0524);
	write_cmos_sensor_16bit(0x32D4, 0x0BB2);
	write_cmos_sensor_16bit(0x32D4, 0x4018);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x0013);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D80);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2400);
	write_cmos_sensor_16bit(0x32D4, 0xFDFD);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x7011);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x045E);
	write_cmos_sensor_16bit(0x32D4, 0xADB8);
	write_cmos_sensor_16bit(0x32D4, 0x13A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xADB8);
	write_cmos_sensor_16bit(0x32D4, 0x13A1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D00);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAB24);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAB24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0xFDF5);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2400);
	write_cmos_sensor_16bit(0x32D4, 0x105E);
	write_cmos_sensor_16bit(0x32D4, 0xADB8);
	write_cmos_sensor_16bit(0x32D4, 0x13A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xADB8);
	write_cmos_sensor_16bit(0x32D4, 0x13A1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D80);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2CA);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xBB4A);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xBB6C);
	write_cmos_sensor_16bit(0x32D4, 0x021A);
	write_cmos_sensor_16bit(0x32D4, 0x003A);
	write_cmos_sensor_16bit(0x32D4, 0x12EC);
	write_cmos_sensor_16bit(0x32D4, 0x03B6);
	write_cmos_sensor_16bit(0x32D4, 0x1076);
	write_cmos_sensor_16bit(0x32D4, 0xB26C);
	write_cmos_sensor_16bit(0x32D4, 0x1A1A);
	write_cmos_sensor_16bit(0x32D4, 0x003A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x03B2);
	write_cmos_sensor_16bit(0x32D4, 0x047E);
	write_cmos_sensor_16bit(0x32D4, 0xB26C);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xBE5E);
	write_cmos_sensor_16bit(0x32D4, 0x05A5);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x1E5E);
	write_cmos_sensor_16bit(0x32D4, 0x0404);
	write_cmos_sensor_16bit(0x32D4, 0x0002);
	write_cmos_sensor_16bit(0x32D4, 0xA9A4);
	write_cmos_sensor_16bit(0x32D4, 0x060F);
	write_cmos_sensor_16bit(0x32D4, 0xFF00);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0xFDEF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x201E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A4);
	write_cmos_sensor_16bit(0x32D4, 0x05AF);
	write_cmos_sensor_16bit(0x32D4, 0x00FF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x09AF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x080F);
	write_cmos_sensor_16bit(0x32D4, 0x7010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x09AF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x3019);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x3026);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x3033);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x303F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xADB6);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB236);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB6B6);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B6);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B6);
	write_cmos_sensor_16bit(0x32D4, 0x020F);
	write_cmos_sensor_16bit(0x32D4, 0x7051);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x3018);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x7003);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAAA4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAEA4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2428);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA8A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x201E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x704D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2430);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA8A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x081E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D00);
	write_cmos_sensor_16bit(0x32D4, 0x700F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2400);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2600);
	write_cmos_sensor_16bit(0x32D4, 0xFDE8);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x704C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2428);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA8A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x201E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x704E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2430);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA8A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x081E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D00);
	write_cmos_sensor_16bit(0x32D4, 0x700F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2400);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2600);
	write_cmos_sensor_16bit(0x32D4, 0xFDC4);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x704D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x811E);
	write_cmos_sensor_16bit(0x32D4, 0xB949);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x609E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x189E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB9A8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBA28);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7051);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7051);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFF92);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x3018);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x7000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAAA4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAEA4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0006);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0002);
	write_cmos_sensor_16bit(0x32D4, 0x0C1E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A3);
	write_cmos_sensor_16bit(0x32D4, 0x7001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA1A8);
	write_cmos_sensor_16bit(0x32D4, 0x120F);
	write_cmos_sensor_16bit(0x32D4, 0x200E);
	write_cmos_sensor_16bit(0x32D4, 0xA1A8);
	write_cmos_sensor_16bit(0x32D4, 0x03AF);
	write_cmos_sensor_16bit(0x32D4, 0x700B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFD66);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2620);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0014);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x7002);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFFC9);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x00E7);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x43FC);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0xFDA3);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x7018);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D0, 0x3000);
	write_cmos_sensor_16bit(0x32D4, 0x005C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0066);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x000B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0050);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0068);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x001C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x3220, 0x0C12);
	write_cmos_sensor_16bit(0x32CA, 0x048A);  // PDAF_ODP_LLENGTH
	write_cmos_sensor_16bit(0x30EC, 0xFB08);	// CTX_RD_DATA
	write_cmos_sensor_16bit(0x31D6, 0x336B);	 //MIPI_JPEG_PN9_DATA_TYPE
	write_cmos_sensor_16bit(0x32C2, 0x03FC);		//pdaf_dma_start=PDAF_ZONE_PER_LINE*(PDAF_NUMBER_OF_CC + 1)*4 = 1020 = 0x03FC
	write_cmos_sensor_16bit(0x32C4, 0x03C0);	// PDAF_DMA_SIZE
	write_cmos_sensor_16bit(0x32C6, 0x0F00);	// PDAF_DMA_Y
	write_cmos_sensor_16bit(0x32C8, 0x0890);	// PDAF_SEQ_START
	write_cmos_sensor_16bit(0x32D0, 0x0001);	// PE_PARAM_ADDR
	write_cmos_sensor_16bit(0x32D4, 0x0000);	// PE_PARAM_VALUE
	write_cmos_sensor_16bit(0x034E, 0x0C3F);	 // Y_OUTPUT_SIZE
	LOG_CORN("[Corn]PE 10bit \n");	
#endif		
#ifdef TEST_PATTERN
// Generated by: ./Support_files/pe_gen.py Support_files/pefw_test_ramp4.hex
//[PE test]
	write_cmos_sensor_16bit(0x32D0, 0x6000);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x00DF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x189E);
	write_cmos_sensor_16bit(0x32D4, 0xA948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x189F);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0E0E);
	write_cmos_sensor_16bit(0x32D4, 0x188A);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0806);
	write_cmos_sensor_16bit(0x32D4, 0x188A);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0806);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D98);
	write_cmos_sensor_16bit(0x32D4, 0x401E);
	write_cmos_sensor_16bit(0x32D4, 0x0002);
	write_cmos_sensor_16bit(0x32D4, 0x2818);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001F);
	write_cmos_sensor_16bit(0x32D4, 0x16AC);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB6AC);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0B44);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0xA9B4);
	write_cmos_sensor_16bit(0x32D4, 0x010D);
	write_cmos_sensor_16bit(0x32D4, 0x020C);
	write_cmos_sensor_16bit(0x32D4, 0x09B4);
	write_cmos_sensor_16bit(0x32D4, 0x01A4);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x034D);
	write_cmos_sensor_16bit(0x32D4, 0x021C);
	write_cmos_sensor_16bit(0x32D4, 0xBA24);
	write_cmos_sensor_16bit(0x32D4, 0x0E0D);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x09BC);
	write_cmos_sensor_16bit(0x32D4, 0x0150);
	write_cmos_sensor_16bit(0x32D4, 0x081A);
	write_cmos_sensor_16bit(0x32D4, 0x09B4);
	write_cmos_sensor_16bit(0x32D4, 0x01AE);
	write_cmos_sensor_16bit(0x32D4, 0x001A);
	write_cmos_sensor_16bit(0x32D4, 0x16B4);
	write_cmos_sensor_16bit(0x32D4, 0x01B2);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D08);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2828);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001F);
	write_cmos_sensor_16bit(0x32D4, 0x0082);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x018A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0xA082);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x00BE);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x003F);
	write_cmos_sensor_16bit(0x32D4, 0x00A4);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x00AE);
	write_cmos_sensor_16bit(0x32D4, 0xA130);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x002C);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x002E);
	write_cmos_sensor_16bit(0x32D4, 0xA130);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0xC000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2830);
	write_cmos_sensor_16bit(0x32D4, 0x020D);
	write_cmos_sensor_16bit(0x32D4, 0xA1C0);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x032E);
	write_cmos_sensor_16bit(0x32D4, 0xA1B0);
	write_cmos_sensor_16bit(0x32D4, 0x120F);
	write_cmos_sensor_16bit(0x32D4, 0x00AE);
	write_cmos_sensor_16bit(0x32D4, 0xA6B0);
	write_cmos_sensor_16bit(0x32D4, 0x120F);
	write_cmos_sensor_16bit(0x32D4, 0x003C);
	write_cmos_sensor_16bit(0x32D4, 0x00B0);
	write_cmos_sensor_16bit(0x32D4, 0x041A);
	write_cmos_sensor_16bit(0x32D4, 0x006E);
	write_cmos_sensor_16bit(0x32D4, 0xA1B0);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D98);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2828);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB36A);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0F6A);
	write_cmos_sensor_16bit(0x32D4, 0x0200);
	write_cmos_sensor_16bit(0x32D4, 0x0C1E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x2230);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x2438);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x819E);
	write_cmos_sensor_16bit(0x32D4, 0xA495);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x819F);
	write_cmos_sensor_16bit(0x32D4, 0xA495);
	write_cmos_sensor_16bit(0x32D4, 0x1C1D);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2828);
	write_cmos_sensor_16bit(0x32D4, 0xFFFF);
	write_cmos_sensor_16bit(0x32D4, 0x7FFF);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x189E);
	write_cmos_sensor_16bit(0x32D4, 0xA948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x189F);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0E0E);
	write_cmos_sensor_16bit(0x32D4, 0x188A);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0806);
	write_cmos_sensor_16bit(0x32D4, 0x188A);
	write_cmos_sensor_16bit(0x32D4, 0x0A48);
	write_cmos_sensor_16bit(0x32D4, 0x0806);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D98);
	write_cmos_sensor_16bit(0x32D4, 0x401E);
	write_cmos_sensor_16bit(0x32D4, 0x0002);
	write_cmos_sensor_16bit(0x32D4, 0x2818);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001F);
	write_cmos_sensor_16bit(0x32D4, 0x16AC);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB6AC);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0B44);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0xA9B4);
	write_cmos_sensor_16bit(0x32D4, 0x010D);
	write_cmos_sensor_16bit(0x32D4, 0x020C);
	write_cmos_sensor_16bit(0x32D4, 0x09B4);
	write_cmos_sensor_16bit(0x32D4, 0x01A4);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x034D);
	write_cmos_sensor_16bit(0x32D4, 0x021C);
	write_cmos_sensor_16bit(0x32D4, 0xBA24);
	write_cmos_sensor_16bit(0x32D4, 0x0E0D);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x09BC);
	write_cmos_sensor_16bit(0x32D4, 0x0150);
	write_cmos_sensor_16bit(0x32D4, 0x081A);
	write_cmos_sensor_16bit(0x32D4, 0x09B4);
	write_cmos_sensor_16bit(0x32D4, 0x01AE);
	write_cmos_sensor_16bit(0x32D4, 0x001A);
	write_cmos_sensor_16bit(0x32D4, 0x16B4);
	write_cmos_sensor_16bit(0x32D4, 0x01B2);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D08);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2828);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x001F);
	write_cmos_sensor_16bit(0x32D4, 0x0082);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x018A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0xA082);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x00BE);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x003F);
	write_cmos_sensor_16bit(0x32D4, 0x00A4);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x00AE);
	write_cmos_sensor_16bit(0x32D4, 0xA130);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x002C);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x002E);
	write_cmos_sensor_16bit(0x32D4, 0xA130);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0xC000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2830);
	write_cmos_sensor_16bit(0x32D4, 0x020D);
	write_cmos_sensor_16bit(0x32D4, 0xA1C0);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x032E);
	write_cmos_sensor_16bit(0x32D4, 0xA1B0);
	write_cmos_sensor_16bit(0x32D4, 0x120F);
	write_cmos_sensor_16bit(0x32D4, 0x00AE);
	write_cmos_sensor_16bit(0x32D4, 0xA6B0);
	write_cmos_sensor_16bit(0x32D4, 0x120F);
	write_cmos_sensor_16bit(0x32D4, 0x003C);
	write_cmos_sensor_16bit(0x32D4, 0x00B0);
	write_cmos_sensor_16bit(0x32D4, 0x041A);
	write_cmos_sensor_16bit(0x32D4, 0x006E);
	write_cmos_sensor_16bit(0x32D4, 0xA1B0);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D98);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2828);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB36A);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0F6A);
	write_cmos_sensor_16bit(0x32D4, 0x0200);
	write_cmos_sensor_16bit(0x32D4, 0x0C1E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x2230);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x2438);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x819E);
	write_cmos_sensor_16bit(0x32D4, 0xA495);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x819F);
	write_cmos_sensor_16bit(0x32D4, 0xA495);
	write_cmos_sensor_16bit(0x32D4, 0x1C1D);
	write_cmos_sensor_16bit(0x32D4, 0x819E);
	write_cmos_sensor_16bit(0x32D4, 0x0495);
	write_cmos_sensor_16bit(0x32D4, 0x1C1C);
	write_cmos_sensor_16bit(0x32D4, 0x819E);
	write_cmos_sensor_16bit(0x32D4, 0x0495);
	write_cmos_sensor_16bit(0x32D4, 0x1C1C);
	write_cmos_sensor_16bit(0x32D4, 0x81DE);
	write_cmos_sensor_16bit(0x32D4, 0x0495);
	write_cmos_sensor_16bit(0x32D4, 0x1DBC);
	write_cmos_sensor_16bit(0x32D4, 0x81FE);
	write_cmos_sensor_16bit(0x32D4, 0xA495);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xBB34);
	write_cmos_sensor_16bit(0x32D4, 0x03AF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xBB54);
	write_cmos_sensor_16bit(0x32D4, 0x020F);
	write_cmos_sensor_16bit(0x32D4, 0xFFF0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D00);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4007);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2818);
	write_cmos_sensor_16bit(0x32D4, 0x003E);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x003F);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x002E);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x160F);
	write_cmos_sensor_16bit(0x32D4, 0x002E);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x160F);
	write_cmos_sensor_16bit(0x32D4, 0x203E);
	write_cmos_sensor_16bit(0x32D4, 0xAD8A);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x00AE);
	write_cmos_sensor_16bit(0x32D4, 0xAE0A);
	write_cmos_sensor_16bit(0x32D4, 0x1811);
	write_cmos_sensor_16bit(0x32D4, 0x002F);
	write_cmos_sensor_16bit(0x32D4, 0xAE1A);
	write_cmos_sensor_16bit(0x32D4, 0x1411);
	write_cmos_sensor_16bit(0x32D4, 0x003C);
	write_cmos_sensor_16bit(0x32D4, 0x0F0A);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x202A);
	write_cmos_sensor_16bit(0x32D4, 0x0E0A);
	write_cmos_sensor_16bit(0x32D4, 0x1818);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0x0E0C);
	write_cmos_sensor_16bit(0x32D4, 0x1402);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0x0E0C);
	write_cmos_sensor_16bit(0x32D4, 0x15BE);
	write_cmos_sensor_16bit(0x32D4, 0x00AA);
	write_cmos_sensor_16bit(0x32D4, 0x0E0C);
	write_cmos_sensor_16bit(0x32D4, 0x15BE);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAE1A);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x003E);
	write_cmos_sensor_16bit(0x32D4, 0xA93C);
	write_cmos_sensor_16bit(0x32D4, 0x11B1);
	write_cmos_sensor_16bit(0x32D4, 0x704C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xABA4);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0xFFC1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7048);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFFAA);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA120);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x02A0);
	write_cmos_sensor_16bit(0x32D4, 0x01A2);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA2A0);
	write_cmos_sensor_16bit(0x32D4, 0x00E3);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7048);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFF9F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA120);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x02A0);
	write_cmos_sensor_16bit(0x32D4, 0x01A2);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA2A0);
	write_cmos_sensor_16bit(0x32D4, 0x00E3);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA920);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA922);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x0FBA);
	write_cmos_sensor_16bit(0x32D4, 0x0006);
	write_cmos_sensor_16bit(0x32D4, 0xA024);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x0200);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA124);
	write_cmos_sensor_16bit(0x32D4, 0x0BAF);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA124);
	write_cmos_sensor_16bit(0x32D4, 0x0A0F);
	write_cmos_sensor_16bit(0x32D4, 0x805E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFF8B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x7047);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFF86);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x304C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2DD0);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x8000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x04C8);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x019E);
	write_cmos_sensor_16bit(0x32D4, 0xA948);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x801F);
	write_cmos_sensor_16bit(0x32D4, 0xA4C8);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0018);
	write_cmos_sensor_16bit(0x32D4, 0x0946);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x818A);
	write_cmos_sensor_16bit(0x32D4, 0x0936);
	write_cmos_sensor_16bit(0x32D4, 0x0200);
	write_cmos_sensor_16bit(0x32D4, 0x4008);
	write_cmos_sensor_16bit(0x32D4, 0x04C8);
	write_cmos_sensor_16bit(0x32D4, 0x0FBA);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xC000);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x2820);
	write_cmos_sensor_16bit(0x32D4, 0x011E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0x0949);
	write_cmos_sensor_16bit(0x32D4, 0x1010);
	write_cmos_sensor_16bit(0x32D4, 0x0011);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0006);
	write_cmos_sensor_16bit(0x32D4, 0x0936);
	write_cmos_sensor_16bit(0x32D4, 0x0014);
	write_cmos_sensor_16bit(0x32D4, 0x0040);
	write_cmos_sensor_16bit(0x32D4, 0x0948);
	write_cmos_sensor_16bit(0x32D4, 0x0FB2);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0948);
	write_cmos_sensor_16bit(0x32D4, 0x0FB2);
	write_cmos_sensor_16bit(0x32D4, 0x0070);
	write_cmos_sensor_16bit(0x32D4, 0x0948);
	write_cmos_sensor_16bit(0x32D4, 0x0FB4);
	write_cmos_sensor_16bit(0x32D4, 0x0110);
	write_cmos_sensor_16bit(0x32D4, 0x0948);
	write_cmos_sensor_16bit(0x32D4, 0x0FB4);
	write_cmos_sensor_16bit(0x32D4, 0x8070);
	write_cmos_sensor_16bit(0x32D4, 0x0949);
	write_cmos_sensor_16bit(0x32D4, 0x0FB4);
	write_cmos_sensor_16bit(0x32D4, 0x0010);
	write_cmos_sensor_16bit(0x32D4, 0x0924);
	write_cmos_sensor_16bit(0x32D4, 0x0FB4);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3800);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x44FA);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x427D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2818);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11AF);
	write_cmos_sensor_16bit(0x32D4, 0x001F);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x100F);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x44FA);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x6000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x427D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2818);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x11AF);
	write_cmos_sensor_16bit(0x32D4, 0x001F);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x100F);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7800);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFFFE);
	write_cmos_sensor_16bit(0x32D4, 0xFFFF);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x700C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A4);
	write_cmos_sensor_16bit(0x32D4, 0x0DAF);
	write_cmos_sensor_16bit(0x32D4, 0x001C);
	write_cmos_sensor_16bit(0x32D4, 0xADA4);
	write_cmos_sensor_16bit(0x32D4, 0x0C0F);
	write_cmos_sensor_16bit(0x32D4, 0x085E);
	write_cmos_sensor_16bit(0x32D4, 0xADA4);
	write_cmos_sensor_16bit(0x32D4, 0x0DAF);
	write_cmos_sensor_16bit(0x32D4, 0x7007);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x0012);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3020);
	write_cmos_sensor_16bit(0x32D4, 0x0003);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0040);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x4010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x4010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0xFF56);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x4010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x4010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x4010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0xFF52);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x4010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x6010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x6010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0xFF4E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x6010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x6010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x6010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0xFF4A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x6010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x7006);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAABA);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAEA4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x3013);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x7011);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x3020);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x7012);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2420);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFF53);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2220);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7046);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7011);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7012);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2628);
	write_cmos_sensor_16bit(0x32D4, 0x7047);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB2A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2630);
	write_cmos_sensor_16bit(0x32D4, 0x7048);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x09A8);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x7011);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFFAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B6);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B6);
	write_cmos_sensor_16bit(0x32D4, 0x020F);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x3013);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x3020);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x302D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x3039);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xADB6);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB236);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB6B6);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7046);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x0800);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x7049);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2228);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2420);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xB248);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0x10C8);
	write_cmos_sensor_16bit(0x32D4, 0x0E0E);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xB0C8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x010A);
	write_cmos_sensor_16bit(0x32D4, 0xB242);
	write_cmos_sensor_16bit(0x32D4, 0x040F);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12C2);
	write_cmos_sensor_16bit(0x32D4, 0x05A0);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x05A4);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x0A0A);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0xB2CA);
	write_cmos_sensor_16bit(0x32D4, 0x17B7);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x1616);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x12CA);
	write_cmos_sensor_16bit(0x32D4, 0x1616);
	write_cmos_sensor_16bit(0x32D4, 0x000A);
	write_cmos_sensor_16bit(0x32D4, 0x124A);
	write_cmos_sensor_16bit(0x32D4, 0x1616);
	write_cmos_sensor_16bit(0x32D4, 0x006A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x0017);
	write_cmos_sensor_16bit(0x32D4, 0x004A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x04B7);
	write_cmos_sensor_16bit(0x32D4, 0x802A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x05B7);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFE8E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x704A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7047);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2638);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9C8);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x03A2);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x89A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A0);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x7049);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x01B7);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x0017);
	write_cmos_sensor_16bit(0x32D4, 0x002A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x04B7);
	write_cmos_sensor_16bit(0x32D4, 0x802A);
	write_cmos_sensor_16bit(0x32D4, 0xB24A);
	write_cmos_sensor_16bit(0x32D4, 0x05B7);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFE73);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x7049);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7048);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2638);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9C8);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x03A2);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x89A8);
	write_cmos_sensor_16bit(0x32D4, 0x01A0);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xA9A8);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7046);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7047);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7048);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB9A8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBA28);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFF9E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFE41);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFE38);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB224);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB2B8);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xB2B8);
	write_cmos_sensor_16bit(0x32D4, 0x1201);
	write_cmos_sensor_16bit(0x32D4, 0x700F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0100);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x7010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x3013);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x3020);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x302D);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2AC0);
	write_cmos_sensor_16bit(0x32D4, 0x3039);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xADB6);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB236);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xB6B6);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0E0F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA936);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7005);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B6);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B6);
	write_cmos_sensor_16bit(0x32D4, 0x020F);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x3018);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x7003);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAAA4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAEA4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2428);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA8A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x201E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7047);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2430);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA8A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x081E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D00);
	write_cmos_sensor_16bit(0x32D4, 0x700F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2400);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2600);
	write_cmos_sensor_16bit(0x32D4, 0xFE22);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x7046);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2428);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA8A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x201E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7048);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2430);
	write_cmos_sensor_16bit(0x32D4, 0x009E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA8A4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x081E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7010);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D00);
	write_cmos_sensor_16bit(0x32D4, 0x700F);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2400);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2600);
	write_cmos_sensor_16bit(0x32D4, 0xFDFE);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x7045);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7046);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7047);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7048);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2D18);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB948);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xB9A8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBA28);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x000F);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xBAA8);
	write_cmos_sensor_16bit(0x32D4, 0x01AF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7004);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x704B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFF8E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x03A1);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x3018);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x7000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x061E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAAA4);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAEA4);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA24);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x0006);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A80);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0002);
	write_cmos_sensor_16bit(0x32D4, 0x0C1E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A3);
	write_cmos_sensor_16bit(0x32D4, 0x7001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA0B0);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA030);
	write_cmos_sensor_16bit(0x32D4, 0x1A0F);
	write_cmos_sensor_16bit(0x32D4, 0x000E);
	write_cmos_sensor_16bit(0x32D4, 0xA1A8);
	write_cmos_sensor_16bit(0x32D4, 0x120F);
	write_cmos_sensor_16bit(0x32D4, 0x200E);
	write_cmos_sensor_16bit(0x32D4, 0xA1A8);
	write_cmos_sensor_16bit(0x32D4, 0x03AF);
	write_cmos_sensor_16bit(0x32D4, 0x700B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0xFD9C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x4000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2620);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7009);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x601E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x7008);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0x0A38);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x007E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0014);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x005E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x010E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x801E);
	write_cmos_sensor_16bit(0x32D4, 0xA925);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x700A);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x7002);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2418);
	write_cmos_sensor_16bit(0x32D4, 0x008E);
	write_cmos_sensor_16bit(0x32D4, 0xA000);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x181E);
	write_cmos_sensor_16bit(0x32D4, 0xA924);
	write_cmos_sensor_16bit(0x32D4, 0x0FAF);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xAA38);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0201);
	write_cmos_sensor_16bit(0x32D4, 0xFFC9);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3040);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x01A1);
	write_cmos_sensor_16bit(0x32D4, 0x000B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A00);
	write_cmos_sensor_16bit(0x32D4, 0x0015);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2A40);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0BA1);
	write_cmos_sensor_16bit(0x32D4, 0x001E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0A01);
	write_cmos_sensor_16bit(0x32D4, 0x800E);
	write_cmos_sensor_16bit(0x32D4, 0xA9B8);
	write_cmos_sensor_16bit(0x32D4, 0x0BA1);
	write_cmos_sensor_16bit(0x32D4, 0x43FB);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2618);
	write_cmos_sensor_16bit(0x32D4, 0xFDD5);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x3600);
	write_cmos_sensor_16bit(0x32D4, 0x7018);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2218);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x2000);
	write_cmos_sensor_16bit(0x32D0, 0x3000);
	write_cmos_sensor_16bit(0x32D4, 0x004E);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0066);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x000B);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0048);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0068);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x000C);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0014);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x32D4, 0x0001);
	write_cmos_sensor_16bit(0x32D4, 0x0000);
	write_cmos_sensor_16bit(0x3220, 0x0C12);  // PDAF_CONTROL

	write_cmos_sensor_16bit(0x30EC, 0xFB08);  // CTX_RD_DATA //Corn
	//PD
	write_cmos_sensor_16bit(0x034E, 0x0C3F);  // Y_OUTPUT_SIZE	
	write_cmos_sensor_16bit(0x301A, 0x021C);  // RESET_REGISTER 
	write_cmos_sensor_16bit(0x32CA, 0x04AA);  // PDAF_ODP_LLENGTH
	write_cmos_sensor_16bit(0x30EC, 0xFB08);  // CTX_RD_DATA  //Corn
	//write_cmos_sensor_16bit(0x3124, 0x01BA);	// TEMPSENS_DATA_REG
	//write_cmos_sensor_16bit(0x31E4, 0x2CAD);	// PIX_DEF_ID_RAM_READ	
	write_cmos_sensor_16bit(0x31D6, 0x3336);  // MIPI_JPEG_PN9_DATA_TYPE
	write_cmos_sensor_16bit(0x32C2, 0x0);  // PDAF_DMA_START//0x03FC
	write_cmos_sensor_16bit(0x32C4, 0x03C0);  // PDAF_DMA_SIZE //0x0F30
	write_cmos_sensor_16bit(0x32C6, 0x0F00);  // PDAF_DMA_Y
	write_cmos_sensor_16bit(0x32C8, 0x0890);  // PDAF_SEQ_START
	write_cmos_sensor_16bit(0x32D0, 0x0001);  // PE_PARAM_ADDR
	write_cmos_sensor_16bit(0x32D4, 0x0000);  // PE_PARAM_VALUE //Corn
	LOG_CORN("[Corn]PE test_pattern \n");
#endif
	//write_cmos_sensor_16bit(0x3780, 0x8000);  // poly_sc_enable //Corn  //mark by scorpio20170629
	
#if 1	
	write_cmos_sensor_16bit(0x3232, 0x0620);
	write_cmos_sensor_16bit(0x3234, 0x0840);
	write_cmos_sensor_16bit(0x3236, 0xA458);
	write_cmos_sensor_16bit(0x3238, 0xB447);
	write_cmos_sensor_16bit(0x323A, 0xB44D);
	write_cmos_sensor_16bit(0x323C, 0xA449);
	write_cmos_sensor_16bit(0x323E, 0xC428);
	write_cmos_sensor_16bit(0x3240, 0xC422);
	write_cmos_sensor_16bit(0x3242, 0xC426);
	write_cmos_sensor_16bit(0x3244, 0xB421);
	write_cmos_sensor_16bit(0x3246, 0xA43A);
	write_cmos_sensor_16bit(0x3248, 0xB42C);
	write_cmos_sensor_16bit(0x324A, 0xB42D);
	write_cmos_sensor_16bit(0x324C, 0xA435);
	write_cmos_sensor_16bit(0x324E, 0xC428);
	write_cmos_sensor_16bit(0x3250, 0xC422);
	write_cmos_sensor_16bit(0x3252, 0xC415);
	write_cmos_sensor_16bit(0x3254, 0xB41B);
	write_cmos_sensor_16bit(0x3256, 0xE3A1);
	write_cmos_sensor_16bit(0x3258, 0xE3A1);
	write_cmos_sensor_16bit(0x325A, 0xD3C7);
	write_cmos_sensor_16bit(0x325C, 0xD3A1);
	write_cmos_sensor_16bit(0x325E, 0xD3A1);
	write_cmos_sensor_16bit(0x3260, 0xC436);
	write_cmos_sensor_16bit(0x3262, 0x33F9);
	write_cmos_sensor_16bit(0x3264, 0x12C0);
	write_cmos_sensor_16bit(0x3266, 0x36DD);
	write_cmos_sensor_16bit(0x3268, 0x06C6);
	write_cmos_sensor_16bit(0x326A, 0x4BED);
	write_cmos_sensor_16bit(0x326C, 0x2BBC);
	write_cmos_sensor_16bit(0x326E, 0x3BE0);
	write_cmos_sensor_16bit(0x3270, 0xFCCA);
	write_cmos_sensor_16bit(0x3272, 0x30FD);
	write_cmos_sensor_16bit(0x3274, 0x0FBC);
	write_cmos_sensor_16bit(0x3276, 0x3AF8);
	write_cmos_sensor_16bit(0x3278, 0x03CC);
	write_cmos_sensor_16bit(0x327A, 0x4BED);
	write_cmos_sensor_16bit(0x327C, 0x2BBC);
	write_cmos_sensor_16bit(0x327E, 0x34FD);
	write_cmos_sensor_16bit(0x3280, 0x06D1);
	write_cmos_sensor_16bit(0x3282, 0xC97D);
	write_cmos_sensor_16bit(0x3284, 0xC97D);
	write_cmos_sensor_16bit(0x3286, 0xE6C4);
	write_cmos_sensor_16bit(0x3288, 0x0256);
	write_cmos_sensor_16bit(0x328A, 0x0256);
	write_cmos_sensor_16bit(0x328C, 0xB45F);
	write_cmos_sensor_16bit(0x328E, 0x45F8);
	write_cmos_sensor_16bit(0x3290, 0x65BE);
	write_cmos_sensor_16bit(0x3292, 0x72B8);
	write_cmos_sensor_16bit(0x3294, 0x69F5);
	write_cmos_sensor_16bit(0x3296, 0x67F0);
	write_cmos_sensor_16bit(0x3298, 0x63A9);
	write_cmos_sensor_16bit(0x329A, 0x7499);
	write_cmos_sensor_16bit(0x329C, 0x5AF8);
	write_cmos_sensor_16bit(0x329E, 0x06BF);
	write_cmos_sensor_16bit(0x32A0, 0x3995);
	write_cmos_sensor_16bit(0x32A2, 0x3188);
	write_cmos_sensor_16bit(0x32A4, 0xFFAC);
	write_cmos_sensor_16bit(0x32A6, 0x67F0);
	write_cmos_sensor_16bit(0x32A8, 0x63A9);
	write_cmos_sensor_16bit(0x32AA, 0x3D8A);
	write_cmos_sensor_16bit(0x32AC, 0x07AE);
	write_cmos_sensor_16bit(0x32AE, 0x366E);
	write_cmos_sensor_16bit(0x32B0, 0x366E);
	write_cmos_sensor_16bit(0x32B2, 0x67CC);
	write_cmos_sensor_16bit(0x32B4, 0x0CDE);
	write_cmos_sensor_16bit(0x32B6, 0x0CDE);
	write_cmos_sensor_16bit(0x32B8, 0x0BFA);
	write_cmos_sensor_16bit(0x3C92, 0x175B);
	
	//tolerance
	write_cmos_sensor_16bit(0x3CA8, 0x0000);
	write_cmos_sensor_16bit(0x3CAA, 0x0003);
	write_cmos_sensor_16bit(0x3CAC, 0x0000);
	write_cmos_sensor_16bit(0x3CAE, 0x0000);
	write_cmos_sensor_16bit(0x3CB0, 0x0C0D);
	write_cmos_sensor_16bit(0x3CB2, 0x0501);
	write_cmos_sensor_16bit(0x3CB4, 0x000C);
	write_cmos_sensor_16bit(0x3CB6, 0x0001);
	write_cmos_sensor_16bit(0x3CB8, 0x0505);
	write_cmos_sensor_16bit(0x3CBA, 0x0004);
	write_cmos_sensor_16bit(0x3CBC, 0x0402);
	write_cmos_sensor_16bit(0x3CBE, 0x0604);
#endif
write_cmos_sensor_16bit(0x301A, 0x021C);  // RESET_REGISTER 
	write_cmos_sensor(0x0100, 0x00);  // STREAM_OFF
	mdelay(70);

	LOG_INF("X\n");
}    /*    sensor_init  */


static void preview_setting(void)
{
	write_cmos_sensor(0x0100, 0x00);  // STREAM_OFF
	mdelay(70);
        //PLL CONFIG
	write_cmos_sensor_16bit(0x0300, 0x0005);  // VT_PIX_CLK_DIV
	write_cmos_sensor_16bit(0x0302, 0x0001);  // VT_SYS_CLK_DIV
	write_cmos_sensor_16bit(0x0304, 0x0303);  // PRE_PLL_CLK_DIV
	write_cmos_sensor_16bit(0x0306, 0x8E8E);  // PLL_MULTIPLIER
	write_cmos_sensor_16bit(0x0308, 0x000A);  // OP_PIX_CLK_DIV
	write_cmos_sensor_16bit(0x030A, 0x0001);  // OP_SYS_CLK_DIV
	write_cmos_sensor_16bit(0x0112, 0x0A0A);  // CCP_DATA_FORMAT
	write_cmos_sensor_16bit(0x3016, 0x0101);  // ROW_SPEED

        //MIPI CONFIG
	write_cmos_sensor_16bit(0x31B0, 0x005F);  // FRAME_PREAMBLE
	write_cmos_sensor_16bit(0x31B2, 0x002E);  // LINE_PREAMBLE
	write_cmos_sensor_16bit(0x31B4, 0x33D4);  // MIPI_TIMING_0
	write_cmos_sensor_16bit(0x31B6, 0x244B);  //  MIPI_TIMING_1
	write_cmos_sensor_16bit(0x31B8, 0x2413);  // MIPI_TIMING_2
	write_cmos_sensor_16bit(0x31BA, 0x2070); //  MIPI_TIMING_3
	write_cmos_sensor_16bit(0x31BC, 0x870B);  // MIPI_TIMING_4

        //RESOLUTION MODE CONFIG
	write_cmos_sensor_16bit(0x0344, 0x0008);  // X_ADDR_START
	write_cmos_sensor_16bit(0x0348, 0x1077);  // X_ADDR_END
	write_cmos_sensor_16bit(0x0346, 0x0008);  // Y_ADDR_START
	write_cmos_sensor_16bit(0x034A, 0x0C37);  // Y_ADDR_END
	write_cmos_sensor_16bit(0x034C,   4208);  // X_OUTPUT_SIZE
	write_cmos_sensor_16bit(0x034E,   3120);  // Y_OUTPUT_SIZE
	write_cmos_sensor_16bit(0x3040, 0x0041);  // READ_MODE
	
		//Binning Configuration
	write_cmos_sensor_16bit(0x3172, 0x0206);  // ANALOG_CONTROL2 //Corn
	write_cmos_sensor_16bit(0x317A, 0x416E);  // ANALOG_CONTROL6
	write_cmos_sensor_16bit(0x3F3C, 0x0003);  // ANALOG_CONTROL9
	
		//Timing Configuration
	write_cmos_sensor_16bit(0x0342, 0x1248);  // LINE_LENGTH_PCK 4672
	write_cmos_sensor_16bit(0x0340, 0x0C94);  // FRAME_LENGTH_LINES 3140
	write_cmos_sensor_16bit(0x0202, 0x0C84);  // COARSE_INTEGRATION_TIME
	
	
	write_cmos_sensor_16bit(0x3226, 0x00E0);  // PDAF ROW START 
	write_cmos_sensor_16bit(0x3228, 0x0B60);  // PDAF ROW END
	write_cmos_sensor_16bit(0x3222, 0xA0C0);  // PDAF ROW CONTROL 
	write_cmos_sensor_16bit(0x32BA, 0x0000);  // PDAF SC VISUAL TAG 
	write_cmos_sensor_16bit(0x32CA, 0x04AA);  // PDAF_ODP_LLENGTH

        //PDAF VC
	write_cmos_sensor_16bit(0x30EC, 0xFB08);  // CTX_RD_DATA  //Corn
	//write_cmos_sensor_16bit(0x3124, 0x01BA);	// TEMPSENS_DATA_REG
	//write_cmos_sensor_16bit(0x31E4, 0x2CAD);	// PIX_DEF_ID_RAM_READ	
	write_cmos_sensor_16bit(0x31D6, 0x3336);  // MIPI_JPEG_PN9_DATA_TYPE
	write_cmos_sensor_16bit(0x32C2, 0x03FC);  // PDAF_DMA_START
	write_cmos_sensor_16bit(0x32C4, 0x3C0);  // PDAF_DMA_SIZE //0x0F30
	write_cmos_sensor_16bit(0x32C6, 0x0F00);  // PDAF_DMA_Y      //0x0F00  modify by scorpio20170629
	write_cmos_sensor_16bit(0x32C8, 0x0890);  // PDAF_SEQ_START
	write_cmos_sensor_16bit(0x32D0, 0x0001);  // PE_PARAM_ADDR
	write_cmos_sensor_16bit(0x32D4, 0x0000);  // PE_PARAM_VALUE //Corn
     
        //PDAF specific to 13M
	write_cmos_sensor_16bit(0x034E, 0x0C3F);  // Y_OUTPUT_SIZE	
	write_cmos_sensor_16bit(0x32C8, 0x0840);  // PDAF_SEQ_START
        write_cmos_sensor_16bit(0x3023, 0x0001); //  0x3023 is 16bit reg
     write_cmos_sensor_16bit(0x301A, 0x021C);  // RESET_REGISTER     
	write_cmos_sensor(0x0100, 0x01);  // STREAM_ON
}    /*    preview_setting  */

//int capture_first_flag = 0;
//int pre_currefps = 0;
static void capture_setting(kal_uint16 currefps)
{
	//kal_uint16 temp,i;

	write_cmos_sensor(0x0100, 0x00);  // STREAM_OFF
	mdelay(70);
        //PLL CONFIG
	write_cmos_sensor_16bit(0x0300, 0x0005);  // VT_PIX_CLK_DIV
	write_cmos_sensor_16bit(0x0302, 0x0001);  // VT_SYS_CLK_DIV
	write_cmos_sensor_16bit(0x0304, 0x0303);  // PRE_PLL_CLK_DIV
	write_cmos_sensor_16bit(0x0306, 0x8E8E);  // PLL_MULTIPLIER
	write_cmos_sensor_16bit(0x0308, 0x000A);  // OP_PIX_CLK_DIV
	write_cmos_sensor_16bit(0x030A, 0x0001);  // OP_SYS_CLK_DIV
	write_cmos_sensor_16bit(0x0112, 0x0A0A);  // CCP_DATA_FORMAT
	write_cmos_sensor_16bit(0x3016, 0x0101);  // ROW_SPEED

        //MIPI CONFIG
	write_cmos_sensor_16bit(0x31B0, 0x005F);  // FRAME_PREAMBLE
	write_cmos_sensor_16bit(0x31B2, 0x002E);  // LINE_PREAMBLE
	write_cmos_sensor_16bit(0x31B4, 0x33D4);  // MIPI_TIMING_0
	write_cmos_sensor_16bit(0x31B6, 0x244B);  //  MIPI_TIMING_1
	write_cmos_sensor_16bit(0x31B8, 0x2413);  // MIPI_TIMING_2
	write_cmos_sensor_16bit(0x31BA, 0x2070); //  MIPI_TIMING_3
	write_cmos_sensor_16bit(0x31BC, 0x870B);  // MIPI_TIMING_4

        //RESOLUTION MODE CONFIG
	write_cmos_sensor_16bit(0x0344, 0x0008);  // X_ADDR_START
	write_cmos_sensor_16bit(0x0348, 0x1077);  // X_ADDR_END
	write_cmos_sensor_16bit(0x0346, 0x0008);  // Y_ADDR_START
	write_cmos_sensor_16bit(0x034A, 0x0C37);  // Y_ADDR_END
	write_cmos_sensor_16bit(0x034C,   4208);  // X_OUTPUT_SIZE
	write_cmos_sensor_16bit(0x034E,   3120);  // Y_OUTPUT_SIZE
	write_cmos_sensor_16bit(0x3040, 0x0041);  // READ_MODE
	
	
		//Binning Configuration
	write_cmos_sensor_16bit(0x3172, 0x0206);  // ANALOG_CONTROL2 //Corn
	write_cmos_sensor_16bit(0x317A, 0x416E);  // ANALOG_CONTROL6
	write_cmos_sensor_16bit(0x3F3C, 0x0003);  // ANALOG_CONTROL9
	
		//Timing Configuration
	write_cmos_sensor_16bit(0x0342, 0x1248);  // LINE_LENGTH_PCK 4672
	write_cmos_sensor_16bit(0x0340, 0x0C94);  // FRAME_LENGTH_LINES 3140
	write_cmos_sensor_16bit(0x0202, 0x0C84);  // COARSE_INTEGRATION_TIME
	
	write_cmos_sensor_16bit(0x3226, 0x00E0);  // PDAF ROW START 
	write_cmos_sensor_16bit(0x3228, 0x0B60);  // PDAF ROW END
	write_cmos_sensor_16bit(0x3222, 0xA0C0);  // PDAF ROW CONTROL 
	write_cmos_sensor_16bit(0x32BA, 0x0000);  // PDAF SC VISUAL TAG 
	write_cmos_sensor_16bit(0x32CA, 0x04AA);  // PDAF_ODP_LLENGTH
        
        //PDAF VC
	write_cmos_sensor_16bit(0x30EC, 0xFB08);  // CTX_RD_DATA  //Corn
	//write_cmos_sensor_16bit(0x3124, 0x01BA);	// TEMPSENS_DATA_REG
	//write_cmos_sensor_16bit(0x31E4, 0x2CAD);	// PIX_DEF_ID_RAM_READ	
	write_cmos_sensor_16bit(0x31D6, 0x3336);  // MIPI_JPEG_PN9_DATA_TYPE
	write_cmos_sensor_16bit(0x32C2, 0x03FC);  // PDAF_DMA_START
	write_cmos_sensor_16bit(0x32C4, 0x3C0);  // PDAF_DMA_SIZE //0x0F30
	write_cmos_sensor_16bit(0x32C6, 0x0F00);  // PDAF_DMA_Y      //0x0F00  modify by scorpio20170629
	write_cmos_sensor_16bit(0x32C8, 0x0890);  // PDAF_SEQ_START
	write_cmos_sensor_16bit(0x32D0, 0x0001);  // PE_PARAM_ADDR
	write_cmos_sensor_16bit(0x32D4, 0x0000);  // PE_PARAM_VALUE //Corn
     
        //PDAF specific to 13M
	write_cmos_sensor_16bit(0x034E, 0x0C3F);  // Y_OUTPUT_SIZE	
	write_cmos_sensor_16bit(0x32C8, 0x0840);  // PDAF_SEQ_START
        write_cmos_sensor_16bit(0x3023, 0x0001); //0x3023 is 16bit reg
		write_cmos_sensor_16bit(0x301A, 0x021C);  // RESET_REGISTER 
	write_cmos_sensor(0x0100, 0x01);  // STREAM_ON
}


static void normal_video_setting(kal_uint16 currefps)
{
        //PLL CONFIG
	write_cmos_sensor_16bit(0x0300, 0x0005);  // VT_PIX_CLK_DIV
	write_cmos_sensor_16bit(0x0302, 0x0001);  // VT_SYS_CLK_DIV
	write_cmos_sensor_16bit(0x0304, 0x0303);  // PRE_PLL_CLK_DIV
	write_cmos_sensor_16bit(0x0306, 0x8E8E);  // PLL_MULTIPLIER
	write_cmos_sensor_16bit(0x0308, 0x000A);  // OP_PIX_CLK_DIV
	write_cmos_sensor_16bit(0x030A, 0x0001);  // OP_SYS_CLK_DIV
	write_cmos_sensor_16bit(0x0112, 0x0A0A);  // CCP_DATA_FORMAT
	write_cmos_sensor_16bit(0x3016, 0x0101);  // ROW_SPEED	

        //MIPI CONFIG
	write_cmos_sensor_16bit(0x31B0, 0x005F);  // FRAME_PREAMBLE
	write_cmos_sensor_16bit(0x31B2, 0x002E);  // LINE_PREAMBLE
	write_cmos_sensor_16bit(0x31B4, 0x33D4);  // MIPI_TIMING_0
	write_cmos_sensor_16bit(0x31B6, 0x244B);  //  MIPI_TIMING_1
	write_cmos_sensor_16bit(0x31B8, 0x2413);  // MIPI_TIMING_2
	write_cmos_sensor_16bit(0x31BA, 0x2070); //  MIPI_TIMING_3
	write_cmos_sensor_16bit(0x31BC, 0x870B);  // MIPI_TIMING_4

        //RESOLUTION MODE CONFIG
	write_cmos_sensor_16bit(0x0344, 0x0008);  // X_ADDR_START
	write_cmos_sensor_16bit(0x0348, 0x1077);  // X_ADDR_END
	write_cmos_sensor_16bit(0x0346, 0x0008);  // Y_ADDR_START
	write_cmos_sensor_16bit(0x034A, 0x0C37);  // Y_ADDR_END
	write_cmos_sensor_16bit(0x034C,  4208);  // X_OUTPUT_SIZE
	write_cmos_sensor_16bit(0x034E,  3120);  // Y_OUTPUT_SIZE
	
			//Binning Configuration
	write_cmos_sensor_16bit(0x3172, 0x0206);  // ANALOG_CONTROL2 //Corn
	write_cmos_sensor_16bit(0x317A, 0x416E);  // ANALOG_CONTROL6
	write_cmos_sensor_16bit(0x3F3C, 0x0003);  // ANALOG_CONTROL9
	
		//Timing Configuration
	write_cmos_sensor_16bit(0x0342, 0x1248);  // LINE_LENGTH_PCK 4672
	write_cmos_sensor_16bit(0x0340, 0x0C94);  // FRAME_LENGTH_LINES 3140
	write_cmos_sensor_16bit(0x0202, 0x0C84);  // COARSE_INTEGRATION_TIME
	
	write_cmos_sensor_16bit(0x3226, 0x00E0);  // PDAF ROW START 
	write_cmos_sensor_16bit(0x3228, 0x0B60);  // PDAF ROW END
	write_cmos_sensor_16bit(0x3222, 0xA0C0);  // PDAF ROW CONTROL 
	write_cmos_sensor_16bit(0x32BA, 0x0000);  // PDAF SC VISUAL TAG 
	write_cmos_sensor_16bit(0x32CA, 0x04AA);  // PDAF_ODP_LLENGTH
        
        //PDAF VC
	write_cmos_sensor_16bit(0x30EC, 0xFB08);  // CTX_RD_DATA  //Corn
	//write_cmos_sensor_16bit(0x3124, 0x01BA);	// TEMPSENS_DATA_REG
	//write_cmos_sensor_16bit(0x31E4, 0x2CAD);	// PIX_DEF_ID_RAM_READ	
	write_cmos_sensor_16bit(0x31D6, 0x3336);  // MIPI_JPEG_PN9_DATA_TYPE
	write_cmos_sensor_16bit(0x32C2, 0x03FC);  // PDAF_DMA_START
	write_cmos_sensor_16bit(0x32C4, 0x3C0);  // PDAF_DMA_SIZE //0x0F30
	write_cmos_sensor_16bit(0x32C6, 0x0F00);  // PDAF_DMA_Y
	write_cmos_sensor_16bit(0x32C8, 0x0890);  // PDAF_SEQ_START
	write_cmos_sensor_16bit(0x32D0, 0x0001);  // PE_PARAM_ADDR
	write_cmos_sensor_16bit(0x32D4, 0x0000);  // PE_PARAM_VALUE //Corn
     
        //PDAF specific to 13M
	write_cmos_sensor_16bit(0x034E, 0x0C3F);  // Y_OUTPUT_SIZE	
	write_cmos_sensor_16bit(0x32C8, 0x0840);  // PDAF_SEQ_START
        write_cmos_sensor_16bit(0x3023, 0x0001); //0x3023 is 16bit reg
		write_cmos_sensor_16bit(0x301A, 0x021C);  // RESET_REGISTER 
	write_cmos_sensor(0x0100, 0x01);  // STREAM_ON
}

static void hs_video_setting(void)
{
	write_cmos_sensor(0x0100, 0x01);  // STREAM_ON
}


static void slim_video_setting(void)
{
	write_cmos_sensor(0x0100, 0x01);  // STREAM_ON
}


static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    LOG_INF("enable: %d\n", enable);

    if (enable) {
        // 0x5E00[8]: 1 enable,  0 disable
        // 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
        //write_cmos_sensor_16bit(0x3070, 0x0002);
    } else {
        // 0x5E00[8]: 1 enable,  0 disable
        // 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK
       //write_cmos_sensor_16bit(0x3070, 0x0000);
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
    kal_uint8 retry = 2;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if (*sensor_id == imgsensor_info.sensor_id) {
				//iRead16Data_ar1337(AR1337_LSC_addr,AR1337_LSCOTP_SIZE,AR1337_OTPData);	
	
					
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
    kal_uint8 retry = 2;
    kal_uint32 sensor_id = 0;
    LOG_1;
    LOG_2;
    //sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address
    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            sensor_id = return_sensor_id();
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
        
    /* initail sequence write in  */
    sensor_init();
	  //for OTP
	  //otp_cali(imgsensor.i2c_write_id);
	  //write_cmos_sensor(0x0100, 0x00);
	  
    spin_lock(&imgsensor_drv_lock);

    imgsensor.autoflicker_en= KAL_FALSE;
    imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;
    imgsensor.dummy_pixel = 0;
    imgsensor.dummy_line = 0;
    imgsensor.ihdr_en = 0;
    imgsensor.test_pattern = KAL_FALSE;
    imgsensor.current_fps = imgsensor_info.pre.max_framerate;
    //for zsd multi capture setting
	//capture_first_flag = 0;
	//pre_currefps = 0;
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
    preview_setting();
    set_mirror_flip(imgsensor.mirror);
	//set_mirror_flip(sensor_config_data->SensorImageMirror);


	if (0)
	{
		int i = 0;
		
		for (i = 0; i < 1864; i = i +8 )
		{
			printk("0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,\n", AR1337_OTPData[i],AR1337_OTPData[i +1], AR1337_OTPData[i +2], AR1337_OTPData[i +3], AR1337_OTPData[i +4],AR1337_OTPData[i +5], AR1337_OTPData[i +6], AR1337_OTPData[i +7]);
				
		}
		
		printk("0x%x,0x%x,0x%x,0x%x,\n", AR1337_OTPData[i],AR1337_OTPData[i +1], AR1337_OTPData[i +2], AR1337_OTPData[i +3]);
	}



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
    if (imgsensor.current_fps == imgsensor_info.cap1.max_framerate) {//PIP capture: 24fps for less than 13M, 20fps for 16M,15fps for 20M
        imgsensor.pclk = imgsensor_info.cap1.pclk;
        imgsensor.line_length = imgsensor_info.cap1.linelength;
        imgsensor.frame_length = imgsensor_info.cap1.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap1.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    } else {
        if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
            LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n",imgsensor.current_fps,imgsensor_info.cap.max_framerate/10);
        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
        imgsensor.autoflicker_en = KAL_FALSE;
    }
    spin_unlock(&imgsensor_drv_lock);
    capture_setting(imgsensor.current_fps);
    set_mirror_flip(imgsensor.mirror);
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
    normal_video_setting(imgsensor.current_fps);	
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
		set_mirror_flip(imgsensor.mirror);
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
    hs_video_setting();	
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
		set_mirror_flip(imgsensor.mirror);
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
	//set_mirror_flip(sensor_config_data->SensorImageMirror);
		set_mirror_flip(imgsensor.mirror);
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

    sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;          /* The frame of setting shutter default 0 for TG int */
    sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;    /* The frame of setting sensor gain */
    sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
    sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
    sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
    sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
  	sensor_info->PDAF_Support = 2; /*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode*/

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


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    LOG_INF("scenario_id = %d\n", scenario_id);
    spin_lock(&imgsensor_drv_lock);
    imgsensor.current_scenario_id = scenario_id;
    spin_unlock(&imgsensor_drv_lock);
    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			//capture_first_flag = 0;
			//pre_currefps = 0;
            preview(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            capture(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			//capture_first_flag = 0;
			//pre_currefps = 0;
            normal_video(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			//capture_first_flag = 0;
			//pre_currefps = 0;
            hs_video(image_window, sensor_config_data);
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
			//capture_first_flag = 0;
			//pre_currefps = 0;
            slim_video(image_window, sensor_config_data);
            break;
        default:
            LOG_INF("Error ScenarioId setting");
			//capture_first_flag = 0;
			//pre_currefps = 0;
            preview(image_window, sensor_config_data);
            return ERROR_INVALID_SCENARIO_ID;
    }
    return ERROR_NONE;
}    /* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{//
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
            set_dummy();
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
            imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
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


static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
                             UINT8 *feature_para,UINT32 *feature_para_len)
{
    UINT16 *feature_return_para_16=(UINT16 *) feature_para;
    UINT16 *feature_data_16=(UINT16 *) feature_para;
    UINT32 *feature_return_para_32=(UINT32 *) feature_para;
    UINT32 *feature_data_32=(UINT32 *) feature_para;
    unsigned long long *feature_data=(unsigned long long *) feature_para;
    //unsigned long long *feature_return_para=(unsigned long long *) feature_para;

    struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;	
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

    LOG_INF("ar1337 feature_id = %d\n", feature_id);
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
		case SENSOR_FEATURE_GET_PDAF_DATA:	
			LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n"); 
			//read_ar1337_DCC((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
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
                case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
                default:
                    memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
                    break;
            }
        case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
            LOG_INF("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",(UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            ihdr_write_shutter_gain((UINT16)*feature_data,(UINT16)*(feature_data+1),(UINT16)*(feature_data+2));
            break;
    case SENSOR_FEATURE_GET_VC_INFO:
      //LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n", (UINT16)*feature_data);
      pvcinfo = (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
      switch (*feature_data_32) {
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
          memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[1],sizeof( struct SENSOR_VC_INFO_STRUCT));
          break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
          memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[2],sizeof(struct SENSOR_VC_INFO_STRUCT));
          break;
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        default:
          memcpy((void *)pvcinfo,(void *)&SENSOR_VC_INFO[0],sizeof (struct SENSOR_VC_INFO_STRUCT));
          break;
      }
      break;
      /*PDAF CMD*/
    case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
      //LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n", *feature_data);
      //PDAF capacity enable or not, 2p8 only full size support PDAF
      switch (*feature_data) {
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
          *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
          break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
          *(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0; 
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
    case SENSOR_FEATURE_SET_PDAF:
      //LOG_INF("PDAF mode :%d\n", *feature_data_16);
      imgsensor.pdaf_mode= *feature_data_16;
      break;      
	  /*End of PDAF*/
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

UINT32 AR1337_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}    /*    AR1337_MIPI_RAW_SensorInit    */
