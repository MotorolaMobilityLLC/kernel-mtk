/*****************************************************************************
 *
 * Filename:
 * ---------
 *     C2590mipi_Sensor.c
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

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"


#include "c2590mipi_Sensor.h"
#include <linux/hardware_info.h>
/****************************Modify Following Strings for Debug****************************/
#define PFX "C2590_camera_sensor"

/****************************   Modify end    *******************************************/

#define SIOSENSOR_INFO(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)

static DEFINE_SPINLOCK(imgsensor_drv_lock);


static imgsensor_info_struct imgsensor_info = {
    .sensor_id = C2590_SENSOR_ID,        //record sensor id defined in Kd_imgsensor.h

    .checksum_value = 0x45fb7f0a,        //checksum value for Camera Auto Test

    .pre = {
        .pclk = 42600000,                //record different mode's pclk
        .linelength = 1714,                //record different mode's linelength
        .framelength =1224,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1600,        //record different mode's width of grabwindow
        .grabwindow_height = 1200,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 10,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
    .cap = {
        .pclk = 42600000,
        .linelength = 1714,
        .framelength = 1224,
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1600,        //record different mode's width of grabwindow
        .grabwindow_height = 1200,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 10,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 150,
    },
    .normal_video = {
        .pclk = 64800000,
        .linelength = 2908,                //record different mode's linelength
        .framelength = 742,
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 1280,
        .grabwindow_height = 720,
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 10,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
    .hs_video = {
        .pclk = 53400000,                //record different mode's pclk
        .linelength = 2908,                //record different mode's linelength
        .framelength = 612,            //record different mode's framelength
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 800,        //record different mode's width of grabwindow
        .grabwindow_height = 600,        //record different mode's height of grabwindow
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 10,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
    .slim_video = {
        .pclk = 53400000,                //record different mode's pclk
        .linelength = 2908,                //record different mode's linelength
        .framelength = 612,
        .startx = 0,                    //record different mode's startx of grabwindow
        .starty = 0,                    //record different mode's starty of grabwindow
        .grabwindow_width = 800,
        .grabwindow_height = 600,
        /*     following for MIPIDataLowPwr2HighSpeedSettleDelayCount by different scenario    */
        .mipi_data_lp2hs_settle_dc = 10,//unit , ns
        /*     following for GetDefaultFramerateByScenario()    */
        .max_framerate = 300,
    },
    .margin = 2,            //sensor framelength & shutter margin
	.min_shutter = 1,		//min shutter
    .max_frame_length = 0x7fff,//max framelength by sensor register's limitation
    .ae_shut_delay_frame = 0,    //shutter delay frame for AE cycle, 2 frame with ispGain_delay-shut_delay=2-0=2
    .ae_sensor_gain_delay_frame = 0,//sensor gain delay frame for AE cycle,2 frame with ispGain_delay-sensor_gain_delay=2-0=2
    .ae_ispGain_delay_frame = 2,//isp gain delay frame for AE cycle
    .ihdr_support = 0,      //1, support; 0,not support
    .ihdr_le_firstline = 0,  //1,le first ; 0, se first
    .sensor_mode_num = 5,      //support sensor mode num

    .cap_delay_frame = 0,        //enter capture delay frame num
    .pre_delay_frame = 0,         //enter preview delay frame num
    .video_delay_frame = 3,        //enter video delay frame num
    .hs_video_delay_frame = 3,    //enter high speed video  delay frame num
    .slim_video_delay_frame = 3,//enter slim video delay frame num

    .isp_driving_current = ISP_DRIVING_4MA, //mclk driving current
    .sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,//sensor_interface_type
    .mipi_sensor_type = MIPI_OPHY_NCSI2, //0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2
    .mipi_settle_delay_mode = MIPI_SETTLEDELAY_AUTO,//0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_Gb,//sensor output first pixel color
    .mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
    .mipi_lane_num = SENSOR_MIPI_1_LANE,//mipi lane num
    .i2c_addr_table = {0x6c, 0xff},//record sensor support all write id addr, only supprt 4must end with 0xff
};


static imgsensor_struct imgsensor = {
    .mirror = IMAGE_NORMAL,                //mirrorflip information
    .sensor_mode = IMGSENSOR_MODE_INIT, //IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video
    .shutter = 0x3D0,                    //current shutter
    .gain = 0x100,                        //current gain
    .min_frame_length = 1250,//add zsf cista
    .dummy_pixel = 0,                    //current dummypixel
    .dummy_line = 0,                    //current dummyline
    .current_fps = 300,  //full size current fps : 24fps for PIP, 30fps for Normal or ZSD
    .autoflicker_en = KAL_FALSE,  //auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker
    .test_pattern = KAL_FALSE,        //test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output
    .current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,//current scenario id
    .ihdr_en = 0, //sensor need support LE, SE with HDR feature
    .i2c_write_id = 0x6c,//record current sensor's i2c write id
};


/* Sensor output window information */
static SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{
	{
		(1616),(1212),
		0,0,(1615),(1211),(1600),(1200),
		0,0,(1600),(1200),
		0,0,1600,1200
	},//Preview

	{
		(1616),(1212),
		0,0,(1615),(1211),(1600),(1200),
		0,0,(1600),(1200),
		0,0,1600,1200
	},//Capture
	{
		(1616),(1212),
		0,0,(1615),(1211),(1280),(720),
		8,14,(1280),(720),
		0,0,1280,720
	},//Video
	{
		(1616),(1212),
		0,0,(1615),(1211),(800),(600),
		0,0,(800),(600),
		0,0,800,600
	},//High Speed Video
	{
		(1616),(1212),
		0,0,(1615),(1211),(800),(600),
		8,14,(800),(600),
		0,0,800,600
	},//Slim Video

};

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);

	return get_byte;

}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pu_send_cmd, 3, imgsensor.i2c_write_id);

}
static kal_uint32 return_sensor_id(void)
{
    return ((read_cmos_sensor(0x0000) << 8) | read_cmos_sensor(0x0001));
}

//  end	


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{
    //kal_int16 dummy_line;
    kal_uint32 frame_length = imgsensor.frame_length;
    //unsigned long flags;
    SIOSENSOR_INFO(">>> set_max_framerate():framerate = %d, min framelength should enable = %d\n", framerate,min_framelength_en);

    frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
    spin_lock(&imgsensor_drv_lock);
    imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ? frame_length : imgsensor.min_frame_length;
	
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;

    if (min_framelength_en)
        imgsensor.min_frame_length = imgsensor.frame_length;
    spin_unlock(&imgsensor_drv_lock);

	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
	SIOSENSOR_INFO(">>> set_max_framerate():framerate = %d, min framelength should enable = %d\n", framerate,min_framelength_en);
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
    //kal_uint16 realtime_fps = 0;
    //kal_uint32 frame_length = 0;
    spin_lock_irqsave(&imgsensor_drv_lock, flags);
    imgsensor.shutter = shutter;
    spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

    // if shutter bigger than frame_length, should extend frame length first
    spin_lock(&imgsensor_drv_lock);
    if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
        imgsensor.frame_length = shutter + imgsensor_info.margin;
    else
        imgsensor.frame_length = imgsensor.min_frame_length;
    if (imgsensor.frame_length > imgsensor_info.max_frame_length)
        imgsensor.frame_length = imgsensor_info.max_frame_length;
    spin_unlock(&imgsensor_drv_lock);
    SIOSENSOR_INFO("set_shutter(): shutter =%d\n", shutter);
    shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
    shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin)) ? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	// Update Shutter
	write_cmos_sensor(0x0340, imgsensor.frame_length >> 8);
	
	//Update Shutter
	write_cmos_sensor(0x0341, imgsensor.frame_length & 0xFF);
	write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);	
	write_cmos_sensor(0x0203, (shutter) & 0xFF);

	SIOSENSOR_INFO("set_shutter(): shutter =%d, framelength =%d,mini_framelegth=%d\n", shutter,imgsensor.frame_length,imgsensor.min_frame_length);

}    /*    set_shutter */



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
	kal_uint8 iReg=0x00;
	kal_uint8 iRegH=0x00;
	kal_uint8 iRegL=0x00;

	kal_uint16 iGain=gain;
	if(iGain>BASEGAIN)
		iRegH=(iGain/BASEGAIN)-1;
	iRegL=(iGain%BASEGAIN)/4;
	
	iReg=((iRegH<<4)&0xF0) | (iRegL & 0x0F);
	
	write_cmos_sensor(0x0205, (iReg & 0xFF));
		//analog gain

	SIOSENSOR_INFO(" C2590 add0205 gain = %x\n",(iReg & 0xFF));

		//analog gain
		//analog gain
		//analog gain
		//analog gain
		//analog gain
		//analog gain
    return (iReg & 0xFF);

}    /*    set_gain  */





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
	SIOSENSOR_INFO("sensor_init begin");
	
	/*system*/
	write_cmos_sensor(0x0103,0x01);		
	//mirror
	write_cmos_sensor(0x0101,0x03);
	write_cmos_sensor(0x3009,0x04);
	write_cmos_sensor(0x300b,0x03);
	
	/*ANALOG & CISCTL*/
	write_cmos_sensor(0x0303,0x01);         
	write_cmos_sensor(0x0304,0x00);         
	write_cmos_sensor(0x0305,0x03);         
	write_cmos_sensor(0x0307,0x47);   //47 for 20fps  58 for 25fps  
	write_cmos_sensor(0x0309,0x10);         
	write_cmos_sensor(0x3087,0x90);          
	write_cmos_sensor(0x0202,0x04);          
	write_cmos_sensor(0x3108,0x01);          
	write_cmos_sensor(0x3109,0x04);          
	//bpc               
	write_cmos_sensor(0x3d00,0xad);         
	write_cmos_sensor(0x3d01,0x15);         
	write_cmos_sensor(0x3d07,0x40);         
	write_cmos_sensor(0x3d08,0x40);         
	write_cmos_sensor(0x3d0a,0x30);         
	write_cmos_sensor(0x3280,0x06);  //00 v11       
	write_cmos_sensor(0x3281,0x05);         
	write_cmos_sensor(0x3282,0x76);
	write_cmos_sensor(0x3283,0xd3);         
	write_cmos_sensor(0x3287,0x40);         
	write_cmos_sensor(0x3288,0x57);         
	write_cmos_sensor(0x3289,0x0c);         
	write_cmos_sensor(0x328A,0x21);
	write_cmos_sensor(0x328B,0x44);         
	write_cmos_sensor(0x328C,0x34);         
	write_cmos_sensor(0x328D,0x55);         
	write_cmos_sensor(0x328E,0x00);         
	write_cmos_sensor(0x308a,0x00);         
	write_cmos_sensor(0x308b,0x00);         
	write_cmos_sensor(0x320C,0x00);         
	write_cmos_sensor(0x320E,0x08);         
	write_cmos_sensor(0x3210,0x22);         
	write_cmos_sensor(0x3211,0x14);         
	write_cmos_sensor(0x3212,0x28);         
	write_cmos_sensor(0x3213,0x14);         
	write_cmos_sensor(0x3214,0x14);         
	write_cmos_sensor(0x3215,0x40);         
	write_cmos_sensor(0x3216,0x1a);         
	write_cmos_sensor(0x3217,0x50);         
	write_cmos_sensor(0x3218,0x31);         
	write_cmos_sensor(0x3219,0x12);         

	/*ISP*/
	write_cmos_sensor(0x321A,0x00);         
	write_cmos_sensor(0x321B,0x04);         
	write_cmos_sensor(0x321C,0x00);         
	//async             
	write_cmos_sensor(0x3180,0xd0);         
	write_cmos_sensor(0x3181,0x40);         
	//format            
	write_cmos_sensor(0x3c01,0x10);          
	write_cmos_sensor(0x3905,0x01);   
		 
		 
	//write_cmos_sensor(0x0100,0x01);//stream on

	SIOSENSOR_INFO("sensor_init end");

}    /*    sensor_init  */


	/*Dark Sun*/     
	/*MIPI*/



static void preview_setting(kal_uint16 currefps)
{

  SIOSENSOR_INFO("E! preview_setting currefps:%d\n",currefps);                                                               
	write_cmos_sensor(0x0307,0x47);                                    
	write_cmos_sensor(0x0340,0x04); //output window size 1600*1200 15fps 
	write_cmos_sensor(0x0341,0xc8);                                    
	write_cmos_sensor(0x0347,0x00);                                    
	write_cmos_sensor(0x034a,0x04);                                    
	write_cmos_sensor(0x034b,0xbb);                                    
	write_cmos_sensor(0x034c,0x06);                                    
	write_cmos_sensor(0x034d,0x40);                                    
	write_cmos_sensor(0x034e,0x04);                                    
	write_cmos_sensor(0x034f,0xb0);                                    
	write_cmos_sensor(0x3009,0x04);
	write_cmos_sensor(0x3021,0x00);                                    
	write_cmos_sensor(0x3022,0x01);                                    
	write_cmos_sensor(0x0100,0x01);  //streaming starts                 
                      
}    /*    preview_setting  */

static void capture_setting(kal_uint16 currefps)
{
    SIOSENSOR_INFO("E! capture_setting currefps:%d\n",currefps);
	write_cmos_sensor(0x0307,0x47);                                    
	write_cmos_sensor(0x0340,0x04); //output window size 1600*1200 15fps 
	write_cmos_sensor(0x0341,0xc8);                                    
	write_cmos_sensor(0x0347,0x00);                                     
	write_cmos_sensor(0x034a,0x04);                                     
	write_cmos_sensor(0x034b,0xbb);                                     
	write_cmos_sensor(0x034c,0x06);                                     
	write_cmos_sensor(0x034d,0x40);                                     
	write_cmos_sensor(0x034e,0x04);                                     
	write_cmos_sensor(0x034f,0xb0);                                     
	write_cmos_sensor(0x3009,0x04);
	write_cmos_sensor(0x3021,0x00);                                     
	write_cmos_sensor(0x3022,0x01);                                     
	write_cmos_sensor(0x0100,0x01);  //streaming starts                 

}

static void normal_video_setting(kal_uint16 currefps)
{
	write_cmos_sensor(0x0307,0x6c);
	write_cmos_sensor(0x0340,0x02); //output window size 1280x720 30fps
	write_cmos_sensor(0x0341,0xe6);
	write_cmos_sensor(0x0347,0xf0);
	write_cmos_sensor(0x034a,0x03);
	write_cmos_sensor(0x034b,0xc7);
	write_cmos_sensor(0x034c,0x05);
	write_cmos_sensor(0x034d,0x00);
	write_cmos_sensor(0x034e,0x02);
	write_cmos_sensor(0x034f,0xd0);
	write_cmos_sensor(0x3009,0xa8);
	write_cmos_sensor(0x3021,0x00);
	write_cmos_sensor(0x3022,0x01);
	write_cmos_sensor(0x0100,0x01); //streaming starts
}

static void hs_video_setting(kal_uint16 currefps)
{
	preview_setting(300);

}

static void slim_video_setting(kal_uint16 currefps)
{
	preview_setting(300);

}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    SIOSENSOR_INFO("enable: %d\n", enable);
    // 0x503D[8]: 1 enable,  0 disable
    // 0x503D[1:0]; 00 Color bar, 01 Random Data, 10 Square
	if(enable)
        {
        write_cmos_sensor(0x3c00,0x07);
        //write_cmos_sensor(0x0d,0x01);
		}
    else
		{
		write_cmos_sensor(0x3c00,0x00);
        //write_cmos_sensor(0x0d,0x00);
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

    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            *sensor_id = return_sensor_id();
            if (*sensor_id == imgsensor_info.sensor_id) {
				hardwareinfo_set_prop(HARDWARE_FRONT_CAM_ID, "bolixin");
                SIOSENSOR_INFO("get_imgsensor_id OK,i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
                return ERROR_NONE;
            }
            SIOSENSOR_INFO("get_imgsensor_id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,*sensor_id);
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

    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
        spin_lock(&imgsensor_drv_lock);
        imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
        spin_unlock(&imgsensor_drv_lock);
        do {
            sensor_id = return_sensor_id();
            if (sensor_id == imgsensor_info.sensor_id) {
                SIOSENSOR_INFO("open:Read sensor id OK,i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
                break;
            }
            SIOSENSOR_INFO("Read sensor id fail, write id: 0x%x, id: 0x%x\n", imgsensor.i2c_write_id,sensor_id);
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
    SIOSENSOR_INFO("E\n");

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
    SIOSENSOR_INFO(">>> preview()\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
    imgsensor.autoflicker_en = KAL_FALSE;
    imgsensor.pclk = imgsensor_info.pre.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.pre.linelength;
    imgsensor.frame_length = imgsensor_info.pre.framelength;
    imgsensor.min_frame_length = imgsensor_info.pre.framelength;

    spin_unlock(&imgsensor_drv_lock);
    preview_setting(imgsensor.current_fps);
    SIOSENSOR_INFO("<<< preview()\n");
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
    SIOSENSOR_INFO(">>> capture()\n");
	
    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;

        imgsensor.pclk = imgsensor_info.cap.pclk;
        imgsensor.line_length = imgsensor_info.cap.linelength;
        imgsensor.frame_length = imgsensor_info.cap.framelength;
        imgsensor.min_frame_length = imgsensor_info.cap.framelength;
    spin_unlock(&imgsensor_drv_lock);
    capture_setting(imgsensor.current_fps);
    SIOSENSOR_INFO("<<< capture()\n");
    return ERROR_NONE;
}    /* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SIOSENSOR_INFO("<<< normal_video()\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
    imgsensor.autoflicker_en = KAL_FALSE;
    imgsensor.pclk = imgsensor_info.normal_video.pclk;
    imgsensor.line_length = imgsensor_info.normal_video.linelength;
    imgsensor.frame_length = imgsensor_info.normal_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
    //imgsensor.current_fps = 300;
    spin_unlock(&imgsensor_drv_lock);
    normal_video_setting(imgsensor.current_fps);	
    SIOSENSOR_INFO("<<< normal_video()\n");
    return ERROR_NONE;
}    /*    normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SIOSENSOR_INFO("<<< hs_video()\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
    imgsensor.autoflicker_en = KAL_FALSE;
    imgsensor.pclk = imgsensor_info.hs_video.pclk;
    //imgsensor.video_mode = KAL_TRUE;
    imgsensor.line_length = imgsensor_info.hs_video.linelength;
    imgsensor.frame_length = imgsensor_info.hs_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
    spin_unlock(&imgsensor_drv_lock);
    hs_video_setting(imgsensor.current_fps);
    SIOSENSOR_INFO("<<< hs_video()\n");
    return ERROR_NONE;
}    /*    hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SIOSENSOR_INFO("<<< slim_video()\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
    imgsensor.autoflicker_en = KAL_FALSE;
    imgsensor.pclk = imgsensor_info.slim_video.pclk;
    imgsensor.line_length = imgsensor_info.slim_video.linelength;
    imgsensor.frame_length = imgsensor_info.slim_video.framelength;
    imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
    spin_unlock(&imgsensor_drv_lock);
    slim_video_setting(imgsensor.current_fps);
    SIOSENSOR_INFO("<<< slim_video()\n");
    return ERROR_NONE;
}    /*    slim_video     */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
    SIOSENSOR_INFO(">>> get_resolution()\n");
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
    SIOSENSOR_INFO(">>> feature_control()\n");
    return ERROR_NONE;
}    /*    get_resolution    */

static kal_uint32 get_info(MSDK_SCENARIO_ID_ENUM scenario_id,
                      MSDK_SENSOR_INFO_STRUCT *sensor_info,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SIOSENSOR_INFO(">>> get_info():scenario_id = %d\n", scenario_id);


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

	SIOSENSOR_INFO("<<< get_info()\n");
    return ERROR_NONE;
}    /*    get_info  */


static kal_uint32 control(MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SIOSENSOR_INFO(">>> control() scenario_id = %d\n", scenario_id);
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
            SIOSENSOR_INFO("Error ScenarioId setting");
            preview(image_window, sensor_config_data);
            return ERROR_INVALID_SCENARIO_ID;
    }
	SIOSENSOR_INFO("<<< control()\n");
    return ERROR_NONE;
}    /* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{//This Function not used after ROME
    SIOSENSOR_INFO("framerate = %d\n ", framerate);
    // SetVideoMode Function should fix framerate
    if (framerate == 0)
        // Dynamic frame rate
        return ERROR_NONE;
    spin_lock(&imgsensor_drv_lock);
        imgsensor.current_fps = framerate;
    spin_unlock(&imgsensor_drv_lock);
    set_max_framerate(imgsensor.current_fps,1);

    return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
    SIOSENSOR_INFO(">>>set_auto_flicker_mode():enable = %d, framerate = %d \n", enable, framerate);
    spin_lock(&imgsensor_drv_lock);
    if (enable) //enable auto flicker
        imgsensor.autoflicker_en = KAL_TRUE;
    else //Cancel Auto flick
        imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    SIOSENSOR_INFO("<<< set_auto_flicker_mode():enable = %d, framerate = %d \n", enable, framerate);
    return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
   // kal_uint32 frame_length;

    SIOSENSOR_INFO(">>>>set_max_framerate_by_scenario():scenario_id = %d, framerate = %d\n", scenario_id, framerate);

    switch (scenario_id) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			imgsensor.pclk= imgsensor_info.pre.pclk;
			imgsensor.line_length=imgsensor_info.pre.linelength;
			imgsensor.frame_length=imgsensor_info.pre.framelength;
			imgsensor.current_fps=framerate;
			imgsensor.min_frame_length = imgsensor_info.pre.framelength;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			imgsensor.pclk= imgsensor_info.normal_video.pclk;
			imgsensor.line_length=imgsensor_info.normal_video.linelength;
			imgsensor.frame_length=imgsensor_info.normal_video.framelength;
			imgsensor.current_fps=framerate;
			imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			imgsensor.pclk= imgsensor_info.cap.pclk;
			imgsensor.line_length=imgsensor_info.cap.linelength;
			imgsensor.frame_length=imgsensor_info.cap.framelength;
			imgsensor.current_fps=framerate;

			
			imgsensor.min_frame_length = imgsensor_info.cap.framelength;
            break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			imgsensor.pclk= imgsensor_info.hs_video.pclk;
			imgsensor.line_length=imgsensor_info.hs_video.linelength;
			imgsensor.frame_length=imgsensor_info.hs_video.framelength;
			imgsensor.current_fps=framerate;
			imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
            break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
			imgsensor.pclk= imgsensor_info.slim_video.pclk;
			imgsensor.line_length=imgsensor_info.slim_video.linelength;
			imgsensor.frame_length=imgsensor_info.slim_video.framelength;
			imgsensor.current_fps=framerate;
            imgsensor.min_frame_length = imgsensor.frame_length;
            break;
        default:  //coding with  preview scenario by default
			imgsensor.pclk= imgsensor_info.pre.pclk;
			imgsensor.line_length=imgsensor_info.pre.linelength;
			imgsensor.frame_length=imgsensor_info.pre.framelength;
			imgsensor.current_fps=framerate;
			imgsensor.min_frame_length = imgsensor_info.pre.framelength;
            break;
    }
	set_max_framerate(imgsensor.current_fps,1);
    SIOSENSOR_INFO("<<<set_max_framerate_by_scenario()\n");
    return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
    SIOSENSOR_INFO("scenario_id = %d\n", scenario_id);

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

    SENSOR_WINSIZE_INFO_STRUCT *wininfo;
    MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data=(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

    SIOSENSOR_INFO(">>> feature_control():feature_id = %d\n", feature_id);
    switch (feature_id) {
        case SENSOR_FEATURE_GET_PERIOD:
            *feature_return_para_16++ = imgsensor.line_length;
            *feature_return_para_16 = imgsensor.frame_length;
            *feature_para_len=4;
			printk("[C2590_SensroDriver]%d,%d\n",imgsensor.line_length,imgsensor.frame_length);
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            *feature_return_para_32 = imgsensor.pclk;
            *feature_para_len=4;
			printk("[C2590_SensroDriver]%d\n",imgsensor.pclk);
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
            SIOSENSOR_INFO("current fps :%d\n", (UINT32)*feature_data);
            spin_lock(&imgsensor_drv_lock);
            imgsensor.current_fps = *feature_data;
            spin_unlock(&imgsensor_drv_lock);
            break;
        case SENSOR_FEATURE_GET_CROP_INFO:
            SIOSENSOR_INFO("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data);

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
        default:
            break;
    }
    SIOSENSOR_INFO("<<< feature_control()\n");

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

UINT32 C2590_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&sensor_func;
    return ERROR_NONE;
}    /*    C2590MIPI_RAW_SensorInit    */
